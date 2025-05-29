#include "functions.h"

#include "numberimpl.h"

#define WRAP_UNARY_SCALAR(func, arg1)                                          \
    auto Functions::func(Scalar const& arg1) -> Scalar                         \
    {                                                                          \
        Scalar result{};                                                       \
        mpfr_##func(                                                           \
            result.m_impl->value,                                              \
            arg1.m_impl->value,                                                \
            mpfr_get_default_rounding_mode()                                   \
        );                                                                     \
        return result;                                                         \
    }

#define WRAP_UNARY_SCALAR_NO_ROUND(func, arg1)                                 \
    auto Functions::func(Scalar const& arg1) -> Scalar                         \
    {                                                                          \
        Scalar result{};                                                       \
        mpfr_##func(result.m_impl->value, arg1.m_impl->value);                 \
        return result;                                                         \
    }

#define WRAP_BINARY_SCALAR(func, arg1, arg2)                                   \
    auto Functions::func(Scalar const& arg1, Scalar const& arg2) -> Scalar     \
    {                                                                          \
        auto result = mpfr::func(arg1.m_impl->value, arg2.m_impl->value);      \
        return Scalar{detail::ScalarImpl(std::move(result))};                  \
    }

namespace calqmath
{
auto Functions::id(Scalar const& number) -> Scalar { return number; }
WRAP_UNARY_SCALAR(abs, argument);

/**
 * For our implementation of rounding functions:
 * For now we do not allow controlling the precision of the output. Thus, the
 * question of single vs double rounding does not matter. Thus, the family of
 * rint_* is not needed at this time.
 */

WRAP_UNARY_SCALAR_NO_ROUND(ceil, argument);
WRAP_UNARY_SCALAR_NO_ROUND(floor, argument);
WRAP_UNARY_SCALAR_NO_ROUND(round, argument);
auto Functions::roundeven(Scalar const& argument) -> Scalar
{
    Scalar result{};
    mpfr_rint(result.m_impl->value, argument.m_impl->value, MPFR_RNDN);
    return result;
}
WRAP_UNARY_SCALAR_NO_ROUND(trunc, argument);

WRAP_UNARY_SCALAR(sqrt, argument);
WRAP_UNARY_SCALAR(cbrt, argument);

WRAP_UNARY_SCALAR(exp, exponent);

WRAP_UNARY_SCALAR(log, argument);
WRAP_UNARY_SCALAR(log2, argument);

auto Functions::logn(Scalar const& base, Scalar const& argument) -> Scalar
{
    auto const precision{std::max(
        mpfr_get_prec(base.m_impl->value), mpfr_get_prec(argument.m_impl->value)
    )};
    Scalar result{detail::clampPrecision(precision)};

    auto numerator = log(argument);
    auto denominator = log(base);

    mpfr_div(
        result.m_impl->value,
        numerator.m_impl->value,
        denominator.m_impl->value,
        mpfr_get_default_rounding_mode()
    );

    return result;
}

WRAP_UNARY_SCALAR(erf, argument);
WRAP_UNARY_SCALAR(erfc, argument);
WRAP_UNARY_SCALAR(gamma, argument);

WRAP_UNARY_SCALAR(sin, radians);
WRAP_UNARY_SCALAR(csc, radians);
WRAP_UNARY_SCALAR(asin, argument);
WRAP_UNARY_SCALAR(cos, radians);
WRAP_UNARY_SCALAR(sec, radians);
WRAP_UNARY_SCALAR(acos, argument);
WRAP_UNARY_SCALAR(tan, radians);
WRAP_UNARY_SCALAR(cot, radians);
WRAP_UNARY_SCALAR(atan, argument);

WRAP_UNARY_SCALAR(sinh, argument);
WRAP_UNARY_SCALAR(cosh, argument);
WRAP_UNARY_SCALAR(tanh, argument);
WRAP_UNARY_SCALAR(asinh, argument);
WRAP_UNARY_SCALAR(acosh, argument);
WRAP_UNARY_SCALAR(atanh, argument);

} // namespace calqmath
