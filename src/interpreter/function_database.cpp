#include "function_database.h"

#include "math/functions.h"
#include <cassert>

[[maybe_unused]]
constexpr char const* RESERVED_FUNCTION_NAME = "x"; // Identifier for variable

namespace calqmath
{
FunctionDatabase::FunctionDatabase() = default;

auto FunctionDatabase::createWithDefaults() -> FunctionDatabase
{
    FunctionDatabase result{};

    std::initializer_list<UnaryFunction> const functions = {
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

    result.m_unaryFunctions =
        std::map<std::string, std::shared_ptr<UnaryFunction const>>();

    for (UnaryFunction const& function : functions)
    {
        assert(function.name != RESERVED_FUNCTION_NAME);

        result.m_unaryFunctions[function.name] =
            std::make_shared<UnaryFunction const>(function);
    }

    return result;
}

auto FunctionDatabase::lookup(std::string const& identifier) const
    -> std::optional<std::shared_ptr<UnaryFunction const>>
{
    if (!m_unaryFunctions.contains(identifier))
    {
        return std::nullopt;
    }

    return m_unaryFunctions.at(identifier);
}
} // namespace calqmath
