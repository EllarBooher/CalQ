#include "mathinterpreter.h"

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
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
    Decimal
};

auto parseCharacter(char const character)
    -> std::optional<StatementCharacterType>
{
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

    /**
     * @brief increment - Increments the index into the math statement string,
     * and transitions the state.
     * @return Returns the result of the increment operation, including if an
     * error occured or if parsing is finished.
     */
    auto increment() -> IncrementResult
    {
        if (!statement.valid())
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
        case ParseState::None:
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
            }
            break;
        case ParseState::NumberPreDecimal:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
            {
                double const number = std::stod(trimmed.substr(
                    numberStartIndex, currentIndex - numberStartIndex
                ));

                if (mathOp.has_value())
                {
                    statement.append(mathOp.value()) = number;
                }
                else if (!statement.empty())
                {
                    return IncrementResult::Error;
                }
                else
                {
                    statement.reset(number);
                }

                mathOp = parseOperator(currentChar).value();
                state = ParseState::Operator;
                break;
            }
            case StatementCharacterType::Digit:
                break;
            case StatementCharacterType::Decimal:
                state = ParseState::NumberPostDecimal;
                break;
            }
            break;
        case ParseState::Operator:
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
            }
            break;
        case ParseState::NumberPostDecimal:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
            {
                // TODO: this is duplicated from NumberPreDecimal transition,
                // the states should probably be combined with pre/post decimal
                // being a boolean flag as part of the state
                double const number = std::stod(trimmed.substr(
                    numberStartIndex, currentIndex - numberStartIndex
                ));

                if (mathOp.has_value())
                {
                    statement.append(mathOp.value()) = number;
                }
                else if (!statement.empty())
                {
                    return IncrementResult::Error;
                }
                else
                {
                    statement.reset(number);
                }

                mathOp = parseOperator(currentChar).value();
                state = ParseState::Operator;
                break;
            }
            case StatementCharacterType::Digit:
                break;
            case StatementCharacterType::Decimal:
                return IncrementResult::Error;
            }
            break;
        }

        return IncrementResult::Continue;
    }

    /**
     * @brief increment - Increments the index into the math statement string,
     * and transitions the state.
     * @return Returns whether or not the result is valid.
     */
    auto finish() -> std::optional<MathStatement>
    {
        switch (state)
        {
        case ParseState::None:
            break;
        case ParseState::NumberPreDecimal:
        case ParseState::NumberPostDecimal:
        {
            try
            {
                double const number = std::stod(
                    trimmed.substr(numberStartIndex, index - numberStartIndex)
                );

                if (mathOp.has_value())
                {
                    statement.append(mathOp.value()) = number;
                }
                else if (!statement.empty())
                {
                    return std::nullopt;
                }
                else
                {
                    statement.reset(number);
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

        if (!statement.valid())
        {
            return std::nullopt;
        }

        return statement;
    }

    std::string const trimmed;

    MathStatement statement;

    enum class ParseState : uint8_t
    {
        None,
        NumberPreDecimal,
        NumberPostDecimal,
        Operator,
    };

    ParseState state{ParseState::None};
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

auto MathStatement::operator==(MathStatement const& rhs) const -> bool
{
    return m_terms == rhs.m_terms && m_operators == rhs.m_operators;
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

    output += std::to_string(m_terms[0]);
    for (size_t i = 0; i < m_operators.size(); i++)
    {
        output += ',';
        output += mathOperatorToString(m_operators[i]);
        output += ',';
        output += std::to_string(m_terms[i + 1]);
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

    if (m_terms.size() == 1)
    {
        return m_terms[0];
    }

    std::deque<MathTerm> terms{m_terms.begin(), m_terms.end()};
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

void MathStatement::reset(MathTerm initial)
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(initial);
}

auto MathStatement::append(MathOp mathOp) -> MathTerm&
{
    m_operators.push_back(mathOp);
    m_terms.push_back(0.0);
    return m_terms.back();
}
