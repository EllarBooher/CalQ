#include "mathfunction.h"

#include <cmath>

#define UNARY_OVERLOAD(name, fnPointer)                                        \
    {name, static_cast<double (*)(double)>(fnPointer)}

#define UNARY(name, input, output)                                             \
    /* NOLINTBEGIN(readability-identifier-length)*/                            \
    {name, [](double const input) { return output; }}                          \
    /* NOLINTEND(readability-identifier-length)*/

MathFunctionDatabase::MathFunctionDatabase()
{
    m_unaryFunctions = std::map<std::string, MathUnaryFunction>{
        UNARY("id", x, x),
        UNARY_OVERLOAD("sqrt", std::sqrt),
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
