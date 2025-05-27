#include "mathinterpreter.h"

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace
{
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

enum class StatementCharacterType : uint8_t
{
    MathOperator,
    Digit,
    Decimal,
    OpenParenthesis,
    CloseParenthesis,
};

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

    if (checkIsDigit(character))
    {
        return StatementCharacterType::Digit;
    }

    if (checkIsOperator(character))
    {
        return StatementCharacterType::MathOperator;
    }

    if (checkIsDecimal(character))
    {
        return StatementCharacterType::Decimal;
    }

    return std::nullopt;
}

auto parseOperator(char const character) -> std::optional<MathOp>
{
    if (character == '+')
    {
        return MathOp::Plus;
    }

    if (character == '-')
    {
        return MathOp::Minus;
    }

    if (character == '*')
    {
        return MathOp::Multiply;
    }

    if (character == '/')
    {
        return MathOp::Divide;
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

auto MathInterpreter::prettify(std::string const& rawInput) -> std::string
{
    return trim(rawInput);
}

struct MathStatementParser
{
    explicit MathStatementParser(std::string const& rawInput)
        : trimmed(trim(rawInput))
    {
    }

    auto execute() -> std::optional<MathStatement>
    {
        IncrementResult incrementResult{IncrementResult::Continue};
        assert(statementDepthStack.empty());
        rootStatement = {};
        statementDepthStack.push(&rootStatement);

        while (incrementResult == IncrementResult::Continue)
        {
            incrementResult = increment();
        }

        if (incrementResult == IncrementResult::Error)
        {
            return std::nullopt;
        }

        return finish();
    }

private:
    enum class IncrementResult : uint8_t
    {
        Continue,
        Finished,
        Error,
    };

    enum class ParseState : uint8_t
    {
        OpenedStatement,
        NumberPreDecimal,
        NumberPostDecimal,
        Operator,
        ClosedStatement,
    };

    /**
     * Increments the index into the string we are parsing, expanding the
     * resulting MathStatement with new terms as they are parsed.
     *
     * @brief increment - Increments parsing by a step.
     * @return Returns the result of the increment operation, including if an
     * error occured or if parsing is finished.
     */
    // NOLINTNEXTLINE
    auto increment() -> IncrementResult
    {
        if (statementDepthStack.empty() || statementDepthStack.top() == nullptr
            || !statementDepthStack.top()->valid())
        {
            return IncrementResult::Error;
        }

        if (index >= trimmed.size())
        {
            return IncrementResult::Finished;
        }

        size_t const currentIndex = index++;
        char const currentChar = trimmed.at(currentIndex);
        auto const typeResult = parseCharacter(currentChar);
        if (!typeResult.has_value())
        {
            return IncrementResult::Error;
        }
        StatementCharacterType const type = typeResult.value();

        switch (state)
        {
        case ParseState::OpenedStatement:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
                return IncrementResult::Error;
            case StatementCharacterType::Digit:
                state = ParseState::NumberPreDecimal;
                numberStartIndex = currentIndex;
                break;
            case StatementCharacterType::Decimal:
                state = ParseState::NumberPostDecimal;
                numberStartIndex = currentIndex;
                break;
            case StatementCharacterType::OpenParenthesis:
            {
                auto& topLevelStatement = *statementDepthStack.top();
                assert(!mathOp.has_value() && topLevelStatement.empty());

                statementDepthStack.push(&topLevelStatement.reset(MathStatement{
                }));
                break;
            }
            case StatementCharacterType::CloseParenthesis:
                // Empty groups are invalid
                return IncrementResult::Error;
            }
            break;
        case ParseState::ClosedStatement:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
                mathOp = parseOperator(currentChar).value();
                state = ParseState::Operator;
                break;
            case StatementCharacterType::CloseParenthesis:
                // We cannot close if we have no remaining open statements.
                // At the bottom of the stack is the first statement, and it has
                // no open Parenthesis so we cannot pop that either.
                if (statementDepthStack.size() <= 1)
                {
                    return IncrementResult::Error;
                }
                statementDepthStack.pop();
                break;
            default:
                return IncrementResult::Error;
            }
            break;
        case ParseState::NumberPreDecimal:
            switch (type)
            {
            case StatementCharacterType::Digit:
                break;
            case StatementCharacterType::Decimal:
                state = ParseState::NumberPostDecimal;
                break;
            case StatementCharacterType::MathOperator:
            case StatementCharacterType::CloseParenthesis:
            {
                double const number = std::stod(trimmed.substr(
                    numberStartIndex, currentIndex - numberStartIndex
                ));

                auto& topLevelStatement = *statementDepthStack.top();
                if (!topLevelStatement.empty() && mathOp == std::nullopt)
                {
                    // This case occurs when the statement looks like (NUMBER1
                    // NUMBER2). Generally impossible since we cannot start a
                    // number after a number, but we check.
                    return IncrementResult::Error;
                }
                if (mathOp.has_value())
                {
                    topLevelStatement.append(mathOp.value()) = number;
                }
                else
                {
                    topLevelStatement.reset(number);
                }

                if (type == StatementCharacterType::MathOperator)
                {
                    mathOp = parseOperator(currentChar).value();
                    state = ParseState::Operator;
                }
                else
                {
                    assert(type == StatementCharacterType::CloseParenthesis);

                    if (statementDepthStack.size() == 1)
                    {
                        return IncrementResult::Error;
                    }

                    statementDepthStack.pop();

                    mathOp = std::nullopt;
                    state = ParseState::ClosedStatement;
                }
                break;
            }
            default:
                return IncrementResult::Error;
            }
            break;
        case ParseState::Operator:
            switch (type)
            {
            case StatementCharacterType::Digit:
                state = ParseState::NumberPreDecimal;
                numberStartIndex = currentIndex;
                break;
            case StatementCharacterType::Decimal:
                state = ParseState::NumberPostDecimal;
                numberStartIndex = currentIndex;
                break;
            case StatementCharacterType::OpenParenthesis:
            {
                // Valid parantheses occur after an operator e.g. 0 + (1 + 2)
                // This is narrow but we can expand later
                if (mathOp == std::nullopt)
                {
                    return IncrementResult::Error;
                }
                assert(!statementDepthStack.empty());

                auto* deeperStatement =
                    &statementDepthStack.top()->appendStatement(mathOp.value());

                statementDepthStack.push(deeperStatement);

                mathOp = std::nullopt;
                state = ParseState::OpenedStatement;
                break;
            }
            default:
                return IncrementResult::Error;
            }
            break;
        case ParseState::NumberPostDecimal:
            switch (type)
            {
            case StatementCharacterType::Digit:
                break;
            case StatementCharacterType::MathOperator:
            case StatementCharacterType::CloseParenthesis:
            {
                double const number = std::stod(trimmed.substr(
                    numberStartIndex, currentIndex - numberStartIndex
                ));

                auto& topLevelStatement = *statementDepthStack.top();
                if (!topLevelStatement.empty() && mathOp == std::nullopt)
                {
                    // This case occurs when the statement looks like (NUMBER1
                    // NUMBER2). Generally impossible since we cannot start a
                    // number after a number, but we check.
                    return IncrementResult::Error;
                }
                if (mathOp.has_value())
                {
                    topLevelStatement.append(mathOp.value()) = number;
                }
                else
                {
                    topLevelStatement.reset(number);
                }

                if (type == StatementCharacterType::MathOperator)
                {
                    mathOp = parseOperator(currentChar).value();
                    state = ParseState::Operator;
                }
                else
                {
                    assert(type == StatementCharacterType::CloseParenthesis);

                    if (statementDepthStack.size() == 1)
                    {
                        return IncrementResult::Error;
                    }

                    statementDepthStack.pop();

                    mathOp = std::nullopt;
                    state = ParseState::ClosedStatement;
                }
                break;
            }
            default:
                return IncrementResult::Error;
            }
        }

        return IncrementResult::Continue;
    }

    /**
     * There is usually dangling state to clean-up, such as when equations end
     * in a digit. This method finishes parsing all that, and returns the final
     * MathStatement if valid.
     *
     * @brief finish - Returns the final result of parsing.
     * @return Returns whether or not the result is valid.
     */
    auto finish() -> std::optional<MathStatement>
    {
        if (statementDepthStack.size() != 1)
        {
            return std::nullopt;
        }

        switch (state)
        {
        case ParseState::OpenedStatement:
        case ParseState::ClosedStatement:
            break;
        case ParseState::NumberPreDecimal:
        case ParseState::NumberPostDecimal:
        {
            try
            {
                double const number = std::stod(
                    trimmed.substr(numberStartIndex, index - numberStartIndex)
                );

                auto& topLevelStatement = *statementDepthStack.top();
                if (!topLevelStatement.empty() && mathOp == std::nullopt)
                {
                    // This case occurs when the statement looks like (NUMBER1
                    // NUMBER2). Generally impossible since we cannot start a
                    // number after a number, but we check.
                    return std::nullopt;
                }
                if (mathOp.has_value())
                {
                    topLevelStatement.append(mathOp.value()) = number;
                }
                else
                {
                    topLevelStatement.reset(number);
                }
            }
            catch (std::invalid_argument const& e)
            {
                return std::nullopt;
            }
            catch (std::out_of_range const& e)
            {
                return std::nullopt;
            }
            break;
        }
        default:
            return std::nullopt;
        }

        if (!statementDepthStack.top()->valid())
        {
            return std::nullopt;
        }

        return std::move(*statementDepthStack.top());
    }

    std::string const trimmed;

    /**
     * We store the root-level statement as a value that gets default
     * destructed. statementDepthStack[0] contains a pointer to rootStatement,
     * and should generally be modified there.
     */
    MathStatement rootStatement;

    /**
     * A MathStatement is a tree-like structure, where individual terms can be
     * statements. As we build statements and add terms, we store a stack of the
     * path to the current deepest statement we are building.
     *
     * The stack should always have at least one element: the root statement at
     * index 0. Keeping this pointer to rootStatement simplifies
     * some of the access logic.
     */
    std::stack<MathStatement*> statementDepthStack;

    ParseState state{ParseState::OpenedStatement};
    size_t numberStartIndex{0};
    size_t index{0};
    std::optional<MathOp> mathOp;
};

auto MathInterpreter::parse(std::string const& rawInput)
    -> std::optional<MathStatement>
{
    MathStatementParser parser{rawInput};
    return parser.execute();
}

auto MathInterpreter::interpret(std::string const& rawInput)
    -> std::expected<double, MathInterpretationError>
{
    auto const parsed = parse(rawInput);
    if (!parsed.has_value())
    {
        return std::unexpected(MathInterpretationError::ParseError);
    }

    MathStatement const& statement = parsed.value();
    auto const evaluated = statement.evaluate();
    if (!evaluated.has_value())
    {
        return std::unexpected(MathInterpretationError::EvaluationError);
    }

    auto const result = evaluated.value();

    return result;
}

auto MathStatement::operator=(MathStatement const& other) -> MathStatement&
{
    m_terms.clear();

    for (auto const& term : other.m_terms)
    {
        m_terms.push_back(std::make_unique<MathTerm>(*term));
    }
    m_operators = other.m_operators;

    return *this;
}

MathStatement::MathStatement(MathStatement const& other) { *this = other; }

auto MathStatement::operator=(MathStatement&& other) noexcept -> MathStatement&
{
    m_terms = std::move(other).m_terms;
    m_operators = std::move(other).m_operators;

    return *this;
}

MathStatement::MathStatement(MathStatement&& other) noexcept
{
    *this = std::move(other);
}

auto MathStatement::operator==(MathStatement const& rhs) const -> bool
{
    if (length() != rhs.length())
    {
        return false;
    }

    if (m_operators != rhs.m_operators)
    {
        return false;
    }

    for (size_t index = 0; index < length(); index++)
    {
        auto const& lhsTerm{m_terms[index]};
        auto const& rhsTerm{rhs.m_terms[index]};
        if (lhsTerm == nullptr && rhsTerm == nullptr)
        {
            continue;
        }
        if (lhsTerm == nullptr || rhsTerm == nullptr)
        {
            return false;
        }

        if (*lhsTerm != *rhsTerm)
        {
            return false;
        }
    }

    return true;
}

namespace
{
auto mathOperatorToString(MathOp const mathOp) -> char const*
{
    switch (mathOp)
    {
    case MathOp::Plus:
        return "+";
    case MathOp::Minus:
        return "-";
    case MathOp::Multiply:
        return "*";
    case MathOp::Divide:
        return "/";
    }

    return "?";
}
} // namespace

template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

auto MathStatement::string() const -> std::string
{
    if (!valid())
    {
        return "Invalid";
    }

    if (empty())
    {
        return "Empty";
    }

    std::string output{};

    output += stringTerm(0);
    for (size_t i = 0; i < m_operators.size(); i++)
    {
        output += ',';
        output += mathOperatorToString(m_operators[i]);
        output += ',';
        output += stringTerm(i + 1);
    }

    return output;
}

auto MathStatement::evaluate() const -> std::optional<double>
{

    if (!valid())
    {
        return std::nullopt;
    }

    if (empty())
    {
        return 0.0;
    }

    std::deque<double> terms{};
    for (size_t termIndex = 0; termIndex < m_terms.size(); termIndex++)
    {
        auto const evaluateResult = evaluateTerm(termIndex);
        if (!evaluateResult.has_value())
        {
            return std::nullopt;
        }
        terms.push_back(evaluateResult.value());
    }

    if (terms.size() == 1)
    {
        return terms[0];
    }

    std::deque<MathOp> operators{m_operators.begin(), m_operators.end()};

    // Reduce while evaluating operators for adjacent terms.
    // Multiplication and division first
    size_t index = 0;
    while (index < operators.size())
    {
        MathOp const mathOperator = operators[index];
        if (mathOperator == MathOp::Plus || mathOperator == MathOp::Minus)
        {
            index++;
            continue;
        }
        assert(
            mathOperator == MathOp::Multiply || mathOperator == MathOp::Divide
        );

        double const firstTerm = terms[index];
        double const secondTerm = terms[index + 1];

        terms.erase(terms.begin() + index);
        operators.erase(operators.begin() + index);

        if (mathOperator == MathOp::Multiply)
        {
            terms.at(index) = firstTerm * secondTerm;
        }
        else
        {
            terms.at(index) = firstTerm / secondTerm;
        }
    }

    // Addition and subtraction next
    while (!operators.empty())
    {
        MathOp const mathOperator = operators[0];
        assert(mathOperator == MathOp::Plus || mathOperator == MathOp::Minus);

        double const firstTerm = terms[0];
        double const secondTerm = terms[1];

        terms.pop_front();
        operators.pop_front();
        index -= 1;

        if (mathOperator == MathOp::Plus)
        {
            terms[0] = firstTerm + secondTerm;
        }
        else
        {
            terms[0] = firstTerm - secondTerm;
        }
    }

    assert(terms.size() == 1);
    return terms[0];
}

auto MathStatement::length() const -> size_t { return m_terms.size(); }

void MathStatement::reset(MathTerm&& initial)
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<MathTerm>(std::move(initial)));
}

auto MathStatement::reset(MathStatement&& initial) -> MathStatement&
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<MathTerm>(std::move(initial)));

    return std::get<MathStatement>(*m_terms.back());
}

auto MathStatement::append(MathOp mathOp) -> MathTerm&
{
    m_terms.push_back(std::make_unique<MathTerm>(0.0));
    m_operators.push_back(mathOp);

    return *m_terms.back();
}

auto MathStatement::appendStatement(MathOp mathOp) -> MathStatement&
{
    auto& term = append(mathOp);
    term = MathStatement();
    return std::get<MathStatement>(term);
}

auto MathStatement::stringTerm(size_t index) const -> std::string
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](double const& number) { return std::to_string(number); },
        [](MathStatement const& statement)
    { return "(" + statement.string() + ")"; },
    };

    return std::visit(visitor, *m_terms[index]);
}

auto MathStatement::evaluateTerm(size_t index) const -> std::optional<double>
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](double const& number) { return std::optional{number}; },
        [](MathStatement const& statement) { return statement.evaluate(); }
    };

    return std::visit(visitor, *m_terms[index]);
}
