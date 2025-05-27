#include "mathfunction.h"

#include <cmath>

MathFunctionDatabase::MathFunctionDatabase()
{
    m_unaryFunctions = std::map<std::string, MathUnaryFunction>{
        {"id", [](double const input) { return input; }}
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
