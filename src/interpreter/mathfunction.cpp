#include "mathfunction.h"

#include "math/functions.h"
#include <cmath>

namespace calqmath
{
FunctionDatabase::FunctionDatabase() = default;

auto FunctionDatabase::createWithDefaults() -> FunctionDatabase
{
    FunctionDatabase result{};

    result.m_unaryFunctions = std::map<std::string, UnaryFunction>{
        {"id", Functions::id},       {"abs", Functions::abs},
        {"ceil", Functions::ceil},   {"floor", Functions::floor},
        {"round", Functions::round}, {"roundeven", Functions::roundeven},
        {"trunc", Functions::trunc}, {"sqrt", Functions::sqrt},
        {"cbrt", Functions::cbrt},   {"exp", Functions::exp},
        {"log", Functions::log},     {"log2", Functions::log2},
        {"erf", Functions::erf},     {"erfc", Functions::erfc},
        {"gamma", Functions::gamma}, {"sin", Functions::sin},
        {"csc", Functions::csc},     {"asin", Functions::asin},
        {"cos", Functions::cos},     {"sec", Functions::sec},
        {"acos", Functions::acos},   {"tan", Functions::tan},
        {"cot", Functions::cot},     {"atan", Functions::atan},
        {"sinh", Functions::sinh},   {"cosh", Functions::cosh},
        {"tanh", Functions::tanh},   {"asinh", Functions::asinh},
        {"acosh", Functions::acosh}, {"atanh", Functions::atanh},
    };

    return result;
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
