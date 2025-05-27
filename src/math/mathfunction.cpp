#include "mathfunction.h"

#include "backend/functions.h"
#include <cmath>

namespace calqmath
{
FunctionDatabase::FunctionDatabase()
{
    m_unaryFunctions = std::map<std::string, UnaryFunction>{
        {"id", Functions::id},
        {"sqrt", Functions::sqrt},
        {"exp", Functions::exp},
        {"log", Functions::log},
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
