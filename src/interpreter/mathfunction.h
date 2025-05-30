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

class FunctionDatabase
{
public:
    FunctionDatabase();

    /**
     * For example, the string "sin" will return the trigonometric sine
     * function.
     *
     * @brief lookup - Look up unary function by its identitifer.
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
    std::map<std::string, UnaryFunction> m_unaryFunctions;
};
} // namespace calqmath
