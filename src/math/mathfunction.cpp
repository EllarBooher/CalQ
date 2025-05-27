#include "mathfunction.h"

#include "backend/functions.h"
#include <cmath>

namespace calqmath
{
FunctionDatabase::FunctionDatabase()
{
    m_unaryFunctions = std::map<std::string, UnaryFunction>{
        {"id", Functions::id},       {"abs", Functions::abs},
        {"ceil", Functions::ceil},   {"floor", Functions::floor},
        {"round", Functions::round}, {"roundeven", Functions::roundeven},
        {"trunc", Functions::trunc}, {"sqrt", Functions::sqrt},
        {"cbrt", Functions::cbrt},   {"exp", Functions::exp},
        {"log", Functions::log},     {"erf", Functions::erf},
        {"erfc", Functions::erfc},   {"gamma", Functions::gamma},
        {"sin", Functions::sin},     {"csc", Functions::csc},
        {"asin", Functions::asin},   {"cos", Functions::cos},
        {"sec", Functions::sec},     {"acos", Functions::acos},
        {"tan", Functions::tan},     {"cot", Functions::cot},
        {"atan", Functions::atan},   {"sinh", Functions::sinh},
        {"cosh", Functions::cosh},   {"tanh", Functions::tanh},
        {"asinh", Functions::asinh}, {"acosh", Functions::acosh},
        {"atanh", Functions::atanh},
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
