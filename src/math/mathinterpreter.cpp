#include "mathinterpreter.h"

#include "backend/number.h"
#include "lexer.h"
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

    auto const statement = Parser::parse(m_functions, tokens.value());
    if (!statement.has_value())
    {
        return std::unexpected(InterpretError::ParseError);
    }

    auto const evaluated = statement.value().evaluate();
    if (!evaluated.has_value())
    {
        return std::unexpected(InterpretError::EvaluationError);
    }

    return evaluated.value();
}

auto Interpreter::functions() const -> FunctionDatabase const&
{
    return m_functions;
}
} // namespace calqmath
