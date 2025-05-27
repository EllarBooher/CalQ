#include "functions.h"

#include "mpreal.h"
#include "numberimpl.h"

#define WRAP_UNARY(func, arg1)                                                 \
    auto result = func(arg1.m_impl->value);                                    \
    return Scalar{detail::ScalarImpl(std::move(result))};

#define WRAP_BINARY(func, arg1, arg2)                                          \
    auto result = func(arg1.m_impl->value, arg2.m_impl->value);                \
    return Scalar{detail::ScalarImpl(std::move(result))};

namespace calqmath
{
auto Functions::id(Scalar const& number) -> Scalar { return number; }
auto Functions::sqrt(Scalar const& number) -> Scalar
{
    WRAP_UNARY(mpfr::sqrt, number);
}
auto Functions::exp(Scalar const& exponent) -> Scalar
{
    WRAP_UNARY(mpfr::exp, exponent);
}
auto Functions::log(Scalar const& argument) -> Scalar
{
    WRAP_UNARY(mpfr::log, argument);
}
auto Functions::pow(Scalar const& base, Scalar const& exponent) -> Scalar
{
    WRAP_BINARY(mpfr::pow, base, exponent);
}
auto Functions::logn(Scalar const& base, Scalar const& argument) -> Scalar
{
    auto numerator = mpfr::log(argument.m_impl->value);
    auto denominator = mpfr::log(base.m_impl->value);

    numerator /= denominator;

    return Scalar{detail::ScalarImpl(std::move(numerator))};
}
} // namespace calqmath
