#include "mathinterpreter.h"

#include <cassert>
#include <deque>
#include <stdexcept>
#include <unordered_set>

auto checkIsDigit(char const character) -> bool
{
    static std::unordered_set<char> digits{
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };

    return digits.contains(character);
}
auto checkIsDecimal(char const character) -> bool { return character == '.'; }
auto checkIsOperator(char const character) -> bool
{
    static std::unordered_set<char> operators{'+', '-', '*', '/'};
    return operators.contains(character);
}

enum class StatementCharacterType
{
    MathOperator,
    Digit,
    Decimal
};
auto parseCharacter(char const character)
    -> std::optional<StatementCharacterType>
{
    if (checkIsDigit(character))
        return StatementCharacterType::Digit;
    else if (checkIsOperator(character))
        return StatementCharacterType::MathOperator;
    else if (checkIsDecimal(character))
        return StatementCharacterType::Decimal;

    return std::nullopt;
}

auto parseOperator(char const character) -> std::optional<MathOp>
{
    if (character == '+')
        return MathOp::Plus;
    else if (character == '-')
        return MathOp::Minus;
    else if (character == '*')
        return MathOp::Multiply;
    else if (character == '/')
        return MathOp::Divide;

    return std::nullopt;
}

auto trim(std::string const& rawInput) -> std::string
{
    std::string output = rawInput;
    std::erase_if(output, isspace);
    return output;
}

std::string MathInterpreter::prettify(std::string const& rawInput)
{
    return trim(rawInput);
}

auto MathInterpreter::parse(std::string const& rawInput)
    -> std::optional<MathStatement>
{
    enum class ParseState
    {
        None,
        NumberPreDecimal,
        NumberPostDecimal,
        Operator,
    };

    struct OperatorPlusTerm
    {
        MathOp op;
        double term;
    };

    std::string const trimmed = trim(rawInput);

    std::vector<double> terms{};
    std::vector<MathOp> operators;

    ParseState state = ParseState::None;
    size_t numberStartIndex = 0;
    size_t index = 0;
    while (index < trimmed.size())
    {
        assert(
            operators.size() == terms.size()
            || (operators.empty() && terms.empty())
        );

        char const currentChar = trimmed.at(index);
        auto const typeResult = parseCharacter(currentChar);
        if (!typeResult.has_value())
        {
            return std::nullopt;
        }
        StatementCharacterType const type = typeResult.value();

        switch (state)
        {
        case ParseState::None:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
                return std::nullopt;
            case StatementCharacterType::Digit:
                state = ParseState::NumberPreDecimal;
                numberStartIndex = index;
                break;
            case StatementCharacterType::Decimal:
                state = ParseState::NumberPostDecimal;
                numberStartIndex = index;
                break;
            }
            break;
        case ParseState::NumberPreDecimal:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
                terms.push_back(std::stod(
                    trimmed.substr(numberStartIndex, index - numberStartIndex)
                ));
                operators.push_back(parseOperator(currentChar).value());
                state = ParseState::Operator;
                break;
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
                return std::nullopt;
            case StatementCharacterType::Digit:
                state = ParseState::NumberPreDecimal;
                numberStartIndex = index;
                break;
            case StatementCharacterType::Decimal:
                state = ParseState::NumberPostDecimal;
                numberStartIndex = index;
                break;
            }
            break;
        case ParseState::NumberPostDecimal:
            switch (type)
            {
            case StatementCharacterType::MathOperator:
                terms.push_back(std::stod(
                    trimmed.substr(numberStartIndex, index - numberStartIndex)
                ));
                operators.push_back(parseOperator(currentChar).value());
                state = ParseState::Operator;
                break;
            case StatementCharacterType::Digit:
                break;
            case StatementCharacterType::Decimal:
                return std::nullopt;
            }
            break;
        }

        index += 1;
    }

    if (state == ParseState::NumberPreDecimal
        || state == ParseState::NumberPostDecimal)
    {
        try
        {
            terms.push_back(std::stod(
                trimmed.substr(numberStartIndex, index - numberStartIndex)
            ));
        }
        catch (std::invalid_argument const& e)
        {
            return std::nullopt;
        }
        catch (std::out_of_range const& e)
        {
            return std::nullopt;
        }
    }

    MathStatement const statement{.terms = terms, .operators = operators};

    if (!statement.isValid())
    {
        return std::nullopt;
    }

    return statement;
}

auto MathInterpreter::evaluate(MathStatement const& statement)
    -> std::optional<double>
{
    if (!statement.isValid())
        return std::nullopt;

    std::deque<double> terms{statement.terms.begin(), statement.terms.end()};
    std::deque<MathOp> operators{
        statement.operators.begin(), statement.operators.end()
    };

    // Reduce while evaluating operators for adjacent terms.
    // Multiplication and division first
    size_t i = 0;
    while (i < operators.size())
    {
        MathOp const mathOperator = operators[i];
        if (mathOperator == MathOp::Plus || mathOperator == MathOp::Minus)
        {
            i++;
            continue;
        }
        assert(
            mathOperator == MathOp::Multiply || mathOperator == MathOp::Divide
        );

        double const firstTerm = terms[i];
        double const secondTerm = terms[i + 1];

        terms.erase(terms.begin() + i);
        operators.erase(operators.begin() + i);

        if (mathOperator == MathOp::Multiply)
        {
            terms.at(i) = firstTerm * secondTerm;
        }
        else
        {
            terms.at(i) = firstTerm / secondTerm;
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
        i -= 1;

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

auto MathInterpreter::interpret(std::string const& rawInput)
    -> std::expected<double, MathInterpretationError>
{
    auto const parsed = parse(rawInput);
    if (!parsed.has_value())
        return std::unexpected(MathInterpretationError::ParseError);

    MathStatement const statement = parsed.value();
    auto const evaluated = evaluate(statement);
    if (!evaluated.has_value())
        return std::unexpected(MathInterpretationError::EvaluationError);

    auto const result = evaluated.value();

    return result;
}

bool operator==(MathStatement const& lhs, MathStatement const& rhs)
{
    return lhs.terms == rhs.terms && lhs.operators == rhs.operators;
}
