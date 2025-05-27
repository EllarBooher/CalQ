#include "mathinterpreter.h"

#include "mathstatementparser.h"
#include <cassert>
#include <cctype>
#include <expected>
#include <optional>
#include <string>
#include <vector>

#include <gmpxx.h>

namespace
{
// TODO: rewrite this an support locales

auto trim(std::string const& rawInput) -> std::string
{
    std::string output = rawInput;
    std::erase_if(
        output, [](unsigned char character) { return std::isspace(character); }
    );
    return output;
}
} // namespace

MathInterpreter::MathInterpreter() = default;

auto MathInterpreter::prettify(std::string const& rawInput) -> std::string
{
    return trim(rawInput);
}

auto MathInterpreter::parse(std::string const& rawInput) const
    -> std::optional<MathStatement>
{
    MathStatementParser parser{rawInput};
    return parser.execute(m_functions);
}

auto MathInterpreter::interpret(std::string const& rawInput) const
    -> std::expected<Scalar, MathInterpretationError>
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

    return evaluated.value();
}
