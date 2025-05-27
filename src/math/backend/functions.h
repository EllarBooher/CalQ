#pragma once

#include "number.h"

namespace calqmath
{
class Functions
{
public:
    static auto id(Scalar const& number) -> Scalar;
    static auto sqrt(Scalar const& number) -> Scalar;
};
} // namespace calqmath
