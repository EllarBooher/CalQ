#include "interpreter.h"

#include "lexer.h"
#include "math/number.h"
#include "parser.h"
#include <cassert>
#include <cctype>
#include <expected>
#include <string>
#include <vector>

namespace
{
// TODO: rewrite this and support locales

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
Interpreter::Interpreter()
    : m_functions{FunctionDatabase::createWithDefaults()}
{
    // TODO: hoist this somewhere more reasonable, for now interpreter acts as
    // the library frontend.
    calqmath::initBignumBackend();
}

auto Interpreter::prettify(std::string const& rawInput) -> std::string
{
    return trim(rawInput);
}

auto Interpreter::interpret(std::string const& rawInput) const
    -> std::expected<Scalar, InterpretError>
{
    auto const tokens = Lexer::convert(rawInput);
    if (!tokens.has_value())
    {
        return std::unexpected(InterpretError::LexError);
    }

    auto const expression = Parser::parse(m_functions, tokens.value());
    if (!expression.has_value())
    {
        return std::unexpected(InterpretError::ParseError);
    }

    auto const evaluated = expression.value().evaluate();
    if (!evaluated.has_value())
    {
        return std::unexpected(InterpretError::EvaluationError);
    }

    return evaluated.value();
}

auto Interpreter::expression(std::string const& rawInput) const
    -> std::expected<Expression, InterpretError>
{
    auto const tokens = Lexer::convert(rawInput);
    if (!tokens.has_value())
    {
        return std::unexpected(InterpretError::LexError);
    }

    auto const expression = Parser::parse(m_functions, tokens.value());
    if (!expression.has_value())
    {
        return std::unexpected(InterpretError::ParseError);
    }

    return expression.value();
}
} // namespace calqmath
