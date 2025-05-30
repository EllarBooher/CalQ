#pragma once

#include "math/number.h"
#include <functional>
#include <map>
#include <optional>
#include <ranges>
#include <string>

namespace calqmath
{
using UnaryFunction = std::function<Scalar(Scalar)>;

/**
 * @brief The FunctionDatabase class stores loaded functions for easy lookup by
 * the interpreter.
 */
class FunctionDatabase
{
public:
    // Creates the database loading every function possible, for normal use.
    static auto createWithDefaults() -> FunctionDatabase;

    /**
     * @brief lookup - Look up unary function by its identitifer.
     *
     * For example, the string "sin" will return the trigonometric sine
     * function.
     *
     * @param rawInput - Raw string input
     * @return Returns the function that was found. Returns null if no such
     *  function is loaded.
     */
    [[nodiscard]] auto lookup(std::string const& identifier) const
        -> std::optional<UnaryFunction>;

    [[nodiscard]] auto unaryNames() const
    {
        return std::views::keys(m_unaryFunctions) | std::views::as_const;
    }

private:
    FunctionDatabase();

    std::map<std::string, UnaryFunction> m_unaryFunctions;
};
} // namespace calqmath
