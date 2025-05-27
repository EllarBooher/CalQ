#pragma once

#include "number.h"

namespace calqmath
{
/**
 * @brief Contains a basic set of functions operating on the underlying
 * multiple-precision type.
 *
 * Most of these functions are thin wrappers around the underlying mp library,
 * but some are implemented using details only known to the implementation.
 */
class Functions
{
public:
    // Identity.
    static auto id(Scalar const& number) -> Scalar;
    // Square root.
    static auto sqrt(Scalar const& number) -> Scalar;
    // Natural exponentation, of Euler's constant raised to exponent.
    static auto exp(Scalar const& exponent) -> Scalar;
    // Natural logarithm, which is the logarithm with base Euler's constant.
    static auto log(Scalar const& argument) -> Scalar;
    // Exponentation of arbitrary base.
    static auto pow(Scalar const& base, Scalar const& exponent) -> Scalar;
    // Logarithm of arbitrary base.
    static auto logn(Scalar const& base, Scalar const& argument) -> Scalar;
};
} // namespace calqmath
