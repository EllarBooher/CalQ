#pragma once

#include "math/number.h"
#include <functional>

namespace calqmath
{
enum class BinaryOp : uint8_t
{
    Plus,
    Minus,
    Multiply,
    Divide
};

struct UnaryFunction
{
    std::string name;
    std::function<Scalar(Scalar)> function;
};
} // namespace calqmath
