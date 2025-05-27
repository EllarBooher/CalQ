#include "functions.h"

#include "mpreal.h"
#include "numberimpl.h"

// We almost don't need mpreal, it covers a lot of functionality we probably
// won't need such as conversions from all C++ integral types.

#define WRAP_UNARY_SCALAR(func, arg1)                                          \
    auto Functions::func(Scalar const& arg1) -> Scalar                         \
    {                                                                          \
        auto result = mpfr::func(arg1.m_impl->value);                          \
        return Scalar{detail::ScalarImpl(std::move(result))};                  \
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

WRAP_UNARY_SCALAR(ceil, argument);
WRAP_UNARY_SCALAR(floor, argument);
WRAP_UNARY_SCALAR(round, argument);
auto Functions::roundeven(Scalar const& argument) -> Scalar
{
    auto result = mpfr::rint(argument.m_impl->value, MPFR_RNDN);
    return Scalar{detail::ScalarImpl(std::move(result))};
}
WRAP_UNARY_SCALAR(trunc, argument);

WRAP_UNARY_SCALAR(sqrt, argument);
WRAP_UNARY_SCALAR(cbrt, argument);

WRAP_UNARY_SCALAR(exp, exponent);

WRAP_UNARY_SCALAR(log, argument);
WRAP_UNARY_SCALAR(log2, argument);

auto Functions::logn(Scalar const& base, Scalar const& argument) -> Scalar
{
    Scalar result{detail::ScalarImpl{mpfr::mpreal{
        0,
        std::max(
            base.m_impl->value.getPrecision(),
            argument.m_impl->value.getPrecision()
        )
    }}};

    auto numerator = mpfr::log(argument.m_impl->value);
    auto denominator = mpfr::log(base.m_impl->value);

    mpfr_div(
        result.m_impl->value.mpfr_ptr(),
        numerator.mpfr_srcptr(),
        denominator.mpfr_srcptr(),
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
