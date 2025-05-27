#include "functions.h"

#include "gmpxx.h"
#include "numberimpl.h"

namespace calqmath
{
auto Functions::id(Scalar const& number) -> Scalar { return number; }
auto Functions::sqrt(Scalar const& number) -> Scalar
{
    Scalar result{};
    result.m_impl = new detail::ScalarImpl(::sqrt(number.m_impl->value));

    return result;
}
} // namespace calqmath
