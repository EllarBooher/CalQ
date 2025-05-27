#include "mathstatementparser.h"

#include "mathfunction.h"
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>

// Helper for std::visit
// See example at https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

namespace
{
enum class StatementCharacterType : uint8_t
{
    BinaryOp,
    Digit,
    Decimal,
    OpenParenthesis,
    CloseParenthesis,
    Alphabetic,
};

auto checkIsDigit(char const character) -> bool
{
    static std::unordered_set<char> const digits{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };

    return digits.contains(character);
}
auto checkIsDecimal(char const character) -> bool { return character == '.'; }
auto checkIsOperator(char const character) -> bool
{
    static std::unordered_set<char> const operators{'+', '-', '*', '/'};
    return operators.contains(character);
}
auto checkIsAlpha(char const character) -> bool
{
    return std::isalpha(character) != 0;
}

auto parseCharacter(char const character)
    -> std::optional<StatementCharacterType>
{
    if (character == '(')
    {
        return StatementCharacterType::OpenParenthesis;
    }

    if (character == ')')
    {
        return StatementCharacterType::CloseParenthesis;
    }

    if (checkIsAlpha(character))
    {
        return StatementCharacterType::Alphabetic;
    }

    if (checkIsDigit(character))
    {
        return StatementCharacterType::Digit;
    }

    if (checkIsOperator(character))
    {
        return StatementCharacterType::BinaryOp;
    }

    if (checkIsDecimal(character))
    {
        return StatementCharacterType::Decimal;
    }

    return std::nullopt;
}

auto parseOperator(char const character) -> std::optional<calqmath::BinaryOp>
{
    if (character == '+')
    {
        return calqmath::BinaryOp::Plus;
    }

    if (character == '-')
    {
        return calqmath::BinaryOp::Minus;
    }

    if (character == '*')
    {
        return calqmath::BinaryOp::Multiply;
    }

    if (character == '/')
    {
        return calqmath::BinaryOp::Divide;
    }

    return std::nullopt;
}

auto trim(std::string const& rawInput) -> std::string
{
    std::string output = rawInput;
    std::erase_if(
        output, [](unsigned char character) { return std::isspace(character); }
    );
    return output;
}
} // namespace

namespace calqmath
{
StatementParser::StatementParser(std::string const& rawInput)
    : m_trimmed(trim(rawInput))
{
}

auto StatementParser::execute(FunctionDatabase const& functions)
    -> std::optional<Statement>
{
    IncrementResult incrementResult{IncrementResult::Continue};
    assert(m_statementDepthStack.empty());
    m_rootStatement = {};
    m_statementDepthStack.push(&m_rootStatement);

    while (incrementResult == IncrementResult::Continue)
    {
        incrementResult = increment(functions);
    }

    if (incrementResult == IncrementResult::Error)
    {
        return std::nullopt;
    }

    return finish();
}

enum class IncrementResult : uint8_t
{
    Continue,
    Finished,
    Error,
};

struct ParseStateOpened
{
};
struct ParseStateClosed
{
};
struct ParseStateOperator
{
    BinaryOp mathOp;
};
struct ParseStateNumber
{
    size_t numberStartIndex{0};
    bool afterDecimal{false};
    std::optional<BinaryOp> mathOp;
};

using ParseState = std::variant<
    ParseStateOpened,
    ParseStateClosed,
    ParseStateNumber,
    ParseStateOperator>;

auto StatementParser::characterAt(size_t const index) const
    -> std::optional<char>
{
    if (index >= m_trimmed.size())
    {
        return std::nullopt;
    }

    return m_trimmed.at(index);
}

/**
 * Starting at the current m_index, grabs all contiguous alphabetic
 * characters and returns the name of the function. So if the stream is "...
 * function(...) ..." with the m_index at f, then "function" will be
 * returned. If non-alphabetic characters are encountered before the
 * paranthesis is encountered, null is returned.
 *
 * This method mutates the m_index, and moves it to the open paranthesis if
 * successful.
 * If null is returned, m_index is not mutated.
 *
 * @brief seekToEndOfFunctionName
 * @return
 */
auto StatementParser::seekToEndOfFunctionName() -> std::optional<std::string>
{
    // This is possibly a function name, composed of alphabetic
    // characters.
    // We seek for parantheses indicating the end.
    size_t const functionNameStartIndex = m_index;
    size_t functionNameSize = 0;
    auto currentCharacter =
        characterAt(functionNameStartIndex + functionNameSize);
    while (currentCharacter.has_value())
    {
        char const character = currentCharacter.value();

        if (character == '(')
        {
            break;
        }

        if (!checkIsAlpha(character))
        {
            return std::nullopt;
        }

        functionNameSize++;
        currentCharacter =
            characterAt(functionNameStartIndex + functionNameSize);
    }

    if (functionNameStartIndex + functionNameSize >= m_trimmed.size())
    {
        return std::nullopt;
    }

    // +1 for the paranthesis
    m_index += functionNameSize;

    try
    {
        return m_trimmed.substr(functionNameStartIndex, functionNameSize);
    }
    catch (std::out_of_range const&)
    {
        return std::nullopt;
    }
}

/**
 * Increments the index into the string we are parsing, expanding the
 * resulting MathStatement with new terms as they are parsed.
 *
 * @brief increment - Increments parsing by a step.
 * @return Returns the result of the increment operation, including if an
 * error occured or if parsing is finished.
 */
// NOLINTNEXTLINE
auto StatementParser::increment(FunctionDatabase const& functions)
    -> IncrementResult
{
    assert(
        !m_statementDepthStack.empty() && m_statementDepthStack.top() != nullptr
    );

    if (!m_statementDepthStack.top()->valid())
    {
        return IncrementResult::Error;
    }

    if (m_index >= m_trimmed.size())
    {
        return IncrementResult::Finished;
    }

    size_t const currentIndex = m_index;
    char const currentChar = m_trimmed.at(currentIndex);
    auto const typeResult = parseCharacter(currentChar);
    if (!typeResult.has_value())
    {
        return IncrementResult::Error;
    }
    StatementCharacterType const type = typeResult.value();

    auto const incrementStateVisitor{overloads{
        [&](ParseStateClosed const&) -> std::optional<ParseState>
    {
        switch (type)
        {
        case StatementCharacterType::BinaryOp:
            return ParseStateOperator{
                .mathOp = parseOperator(currentChar).value()
            };
        case StatementCharacterType::CloseParenthesis:
            // We cannot close if we have no remaining open statements.
            // At the bottom of the stack is the first statement, and it has
            // no open Parenthesis so we cannot pop that either.
            if (m_statementDepthStack.size() <= 1)
            {
                return std::nullopt;
            }
            m_statementDepthStack.pop();
            return ParseStateClosed{};
        default:
            return std::nullopt;
        }
    },
        [&](ParseStateOpened const&) -> std::optional<ParseState>
    {
        switch (type)
        {
        case StatementCharacterType::Digit:
        case StatementCharacterType::Decimal:
            return ParseStateNumber{
                .numberStartIndex = currentIndex,
                .afterDecimal = type == StatementCharacterType::Decimal,
                .mathOp = std::nullopt
            };
        case StatementCharacterType::Alphabetic:
        case StatementCharacterType::OpenParenthesis:
        {
            auto& deeperStatement =
                m_statementDepthStack.top()->reset(Statement{});
            m_statementDepthStack.push(&deeperStatement);

            if (type == StatementCharacterType::Alphabetic)
            {
                auto const functionNameResult = seekToEndOfFunctionName();

                if (!functionNameResult.has_value())
                {
                    return std::nullopt;
                }

                auto functionLookupResult =
                    functions.lookup(functionNameResult.value());

                if (!functionLookupResult.has_value())
                {
                    return std::nullopt;
                }

                deeperStatement.setFunction(
                    std::move(functionLookupResult.value())
                );
            }

            return ParseStateOpened();
        }
        default:
            // Empty groups are invalid
            return std::nullopt;
        }
    },
        [&](ParseStateOperator const& state) -> std::optional<ParseState>
    {
        switch (type)
        {
        case StatementCharacterType::Digit:
        case StatementCharacterType::Decimal:
            return ParseStateNumber{
                .numberStartIndex = currentIndex,
                .afterDecimal = type == StatementCharacterType::Decimal,
                .mathOp = state.mathOp
            };
        case StatementCharacterType::Alphabetic:
        case StatementCharacterType::OpenParenthesis:
        {
            auto& deeperStatement =
                m_statementDepthStack.top()->appendStatement(state.mathOp);
            m_statementDepthStack.push(&deeperStatement);

            if (type == StatementCharacterType::Alphabetic)
            {
                auto const functionNameResult = seekToEndOfFunctionName();

                if (!functionNameResult.has_value())
                {
                    return std::nullopt;
                }

                auto functionLookupResult =
                    functions.lookup(functionNameResult.value());

                if (!functionLookupResult.has_value())
                {
                    return std::nullopt;
                }

                deeperStatement.setFunction(
                    std::move(functionLookupResult.value())
                );
            }

            return ParseStateOpened();
        }
        default:
            return std::nullopt;
        }
    },
        [&](ParseStateNumber const& state) -> std::optional<ParseState>
    {
        switch (type)
        {
        case StatementCharacterType::Digit:
            return state;
        case StatementCharacterType::Decimal:
        {
            if (state.afterDecimal)
            {
                return std::nullopt;
            }

            return ParseStateNumber{
                .numberStartIndex = state.numberStartIndex,
                .afterDecimal = true,
                .mathOp = state.mathOp,
            };
        }
        case StatementCharacterType::BinaryOp:
        case StatementCharacterType::CloseParenthesis:
        {
            Scalar const number{m_trimmed.substr(
                state.numberStartIndex, currentIndex - state.numberStartIndex
            )};

            auto& topLevelStatement = *m_statementDepthStack.top();
            if (!topLevelStatement.empty() && state.mathOp == std::nullopt)
            {
                // This case occurs when the statement looks like (NUMBER1
                // NUMBER2). Generally impossible since we cannot start a
                // number after a number, but we check.
                return std::nullopt;
            }
            if (state.mathOp.has_value())
            {
                topLevelStatement.append(state.mathOp.value()) = number;
            }
            else
            {
                topLevelStatement.reset(number);
            }

            if (type == StatementCharacterType::BinaryOp)
            {
                return ParseStateOperator{
                    .mathOp = parseOperator(currentChar).value()
                };
            }

            assert(type == StatementCharacterType::CloseParenthesis);

            if (m_statementDepthStack.size() <= 1)
            {
                return std::nullopt;
            }
            m_statementDepthStack.pop();
            return ParseStateClosed{};
        }
        default:
            return std::nullopt;
        }
    },
    }};

    std::optional<ParseState> const nextState =
        std::visit(incrementStateVisitor, m_state);
    if (!nextState.has_value())
    {
        return IncrementResult::Error;
    }
    m_state = nextState.value();
    m_index++;

    return IncrementResult::Continue;
}

auto StatementParser::finish() -> std::optional<Statement>
{
    if (m_statementDepthStack.size() != 1)
    {
        return std::nullopt;
    }

    auto const finishVisitor{overloads{
        [&](ParseStateOpened const&) -> bool { return true; },
        [&](ParseStateClosed const&) -> bool { return true; },
        [&](ParseStateNumber const& state) -> bool
    {
        try
        {
            auto const str = m_trimmed.substr(
                state.numberStartIndex, m_index - state.numberStartIndex
            );
            if (std::ranges::find_if(str, checkIsDigit) == str.end())
            {
                return false;
            }
            Scalar const number{str};

            auto& topLevelStatement = *m_statementDepthStack.top();
            if (!topLevelStatement.empty() && state.mathOp == std::nullopt)
            {
                // This case occurs when the statement looks like
                // (NUMBER1 NUMBER2). Generally impossible since we
                // cannot start a number after a number, but we
                // check.
                return false;
            }

            if (state.mathOp.has_value())
            {
                topLevelStatement.append(state.mathOp.value()) = number;
            }
            else
            {
                topLevelStatement.reset(number);
            }

            return true;
        }
        catch (std::invalid_argument const& e)
        {
            return false;
        }
        catch (std::out_of_range const& e)
        {
            return false;
        }
    },
        [&](ParseStateOperator const&) -> bool { return false; },
    }};

    bool const finishValid = std::visit(finishVisitor, m_state);

    if (!finishValid || !m_statementDepthStack.top()->valid())
    {
        return std::nullopt;
    }

    return std::move(*m_statementDepthStack.top());
}
} // namespace calqmath
