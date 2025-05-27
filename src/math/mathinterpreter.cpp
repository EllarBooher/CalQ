#include "mathinterpreter.h"

#include "backend/number.h"
#include "mathstatementparser.h"
#include <cassert>
#include <cctype>
#include <expected>
#include <optional>
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

auto Interpreter::parse(std::string const& rawInput) const
    -> std::optional<Statement>
{
    StatementParser parser{rawInput};
    return parser.execute(m_functions);
}

auto Interpreter::interpret(std::string const& rawInput) const
    -> std::expected<Scalar, InterpretError>
{
    auto const parsed = parse(rawInput);
    if (!parsed.has_value())
    {
        return std::unexpected(InterpretError::ParseError);
    }

    Statement const& statement = parsed.value();
    auto const evaluated = statement.evaluate();
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
