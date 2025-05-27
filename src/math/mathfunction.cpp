#include "mathfunction.h"

#include <cmath>

#define UNARY(name, input, output)                                             \
    /* NOLINTBEGIN(readability-identifier-length)*/                            \
    {name, [](Scalar const& input) { return output; }}                         \
    /* NOLINTEND(readability-identifier-length)*/

namespace calqmath
{
FunctionDatabase::FunctionDatabase()
{
    m_unaryFunctions = std::map<std::string, UnaryFunction>{
        UNARY("id", x, x),
        UNARY("sqrt", x, sqrt(x)),
    };
}

auto FunctionDatabase::lookup(std::string const& identifier) const
    -> std::optional<UnaryFunction>
{
    if (!m_unaryFunctions.contains(identifier))
    {
        return std::nullopt;
    }

    return m_unaryFunctions.at(identifier);
}
} // namespace calqmath
