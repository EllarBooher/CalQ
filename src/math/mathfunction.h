#pragma once

#include "number.h"
#include <functional>
#include <map>
#include <optional>
#include <string>

using MathUnaryFunction = std::function<Scalar(Scalar)>;

class MathFunctionDatabase
{
public:
    MathFunctionDatabase();

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
        -> std::optional<MathUnaryFunction>;

private:
    std::map<std::string, MathUnaryFunction> m_unaryFunctions;
};
