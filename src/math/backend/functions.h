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
    // Absolute value
    static auto abs(Scalar const& argument) -> Scalar;
    // Ceiling function.
    static auto ceil(Scalar const& argument) -> Scalar;
    // Floor function.
    static auto floor(Scalar const& argument) -> Scalar;
    // Round to nearest integer, ties round away from zero.
    static auto round(Scalar const& argument) -> Scalar;
    // Round to nearest integer, with ties going to the even integer.
    static auto roundeven(Scalar const& argument) -> Scalar;
    // Integer truncation function.
    static auto trunc(Scalar const& argument) -> Scalar;
    // Square root.
    static auto sqrt(Scalar const& argument) -> Scalar;
    // Cube root.
    static auto cbrt(Scalar const& argument) -> Scalar;
    // Natural exponentation, of Euler's constant raised to exponent.
    static auto exp(Scalar const& exponent) -> Scalar;
    // Exponentation of arbitrary base.
    static auto pow(Scalar const& base, Scalar const& exponent) -> Scalar;
    // Natural logarithm, which is the logarithm with base Euler's constant.
    static auto log(Scalar const& argument) -> Scalar;
    // Logarithm base-2.
    static auto log2(Scalar const& argument) -> Scalar;
    // Logarithm of arbitrary base.
    static auto logn(Scalar const& base, Scalar const& argument) -> Scalar;
    // Error function.
    static auto erf(Scalar const& argument) -> Scalar;
    // Complementary error function, equal to 1 - erf.
    static auto erfc(Scalar const& argument) -> Scalar;
    // Gamma function, the analytic continuation of factorial.
    static auto gamma(Scalar const& argument) -> Scalar;

    // Trigonometric sine.
    static auto sin(Scalar const& radians) -> Scalar;
    // Trigonometric cosecant, which is the reciprocal of sine.
    static auto csc(Scalar const& radians) -> Scalar;
    // Trigonometric arcsin.
    static auto asin(Scalar const& argument) -> Scalar;
    // Trigonometric cosine.
    static auto cos(Scalar const& radians) -> Scalar;
    // Trigonometric secant, which is the reciprocal of cosine.
    static auto sec(Scalar const& radians) -> Scalar;
    // Trigonometric arccos.
    static auto acos(Scalar const& argument) -> Scalar;
    // Trigonometric tangent.
    static auto tan(Scalar const& radians) -> Scalar;
    // Trigonometric cotangent, which is the reciprocal of tangent.
    static auto cot(Scalar const& radians) -> Scalar;
    // Trigonometric arctan, with result in the range [-pi/2, pi/2].
    static auto atan(Scalar const& argument) -> Scalar;
    // Hyperbolic sine.
    static auto sinh(Scalar const& argument) -> Scalar;
    // Hyperbolic cosine.
    static auto cosh(Scalar const& argument) -> Scalar;
    // Hyperbolic tangent.
    static auto tanh(Scalar const& argument) -> Scalar;
    // Hyperbolic sine inverse.
    static auto asinh(Scalar const& argument) -> Scalar;
    // Hyperbolic cosine inverse.
    static auto acosh(Scalar const& argument) -> Scalar;
    // Hyperbolic tangent inverse.
    static auto atanh(Scalar const& argument) -> Scalar;
};
} // namespace calqmath
