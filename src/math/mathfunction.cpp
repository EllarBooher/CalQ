#include "mathfunction.h"

#include <cmath>

#define UNARY(name, input, output)                                             \
    /* NOLINTBEGIN(readability-identifier-length)*/                            \
    {name, [](Scalar const& input) { return output; }}                         \
    /* NOLINTEND(readability-identifier-length)*/

MathFunctionDatabase::MathFunctionDatabase()
{
    m_unaryFunctions = std::map<std::string, MathUnaryFunction>{
        UNARY("id", x, x),
        UNARY("sqrt", x, sqrt(x)),
    };
}

auto MathFunctionDatabase::lookup(std::string const& identifier) const
    -> std::optional<MathUnaryFunction>
{
    if (!m_unaryFunctions.contains(identifier))
    {
        return std::nullopt;
    }

    return m_unaryFunctions.at(identifier);
}
