#include "functions.h"

#include "gmpxx.h"
#include "numberimpl.h"

namespace calqmath
{
auto Functions::id(Scalar const& number) -> Scalar { return number; }
auto Functions::sqrt(Scalar const& number) -> Scalar
{
    return Scalar{detail::ScalarImpl(::sqrt(number.m_impl->value))};
}
} // namespace calqmath
