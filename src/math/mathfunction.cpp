#include "mathfunction.h"

#include "backend/functions.h"
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
        UNARY("id", x, Functions::id(x)),
        UNARY("sqrt", x, Functions::sqrt(x)),
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
