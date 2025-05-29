#pragma once

#include "mpfr.h"
#include <cstdint>
#include <string>
#include <utility>

// Do not include this file outside of the math module.

namespace detail
{
class ScalarImpl
{
public:
    // Values derived from mpfr documentation of mpfr_strtofr
    // https://www.mpfr.org/mpfr-current/mpfr.html. Bases outside this range are
    // undefined behavior.
    static constexpr size_t MIN_BASE = 2;
    static constexpr size_t MAX_BASE = 62;

    static constexpr size_t MIN_PRECISION = MPFR_PREC_MIN;
    static constexpr size_t MAX_PRECISION = MPFR_PREC_MAX;

    ScalarImpl(std::string const& representation, uint16_t base)
    {
        mpfr_init2(value, mpfr_get_default_prec());
        mpfr_set_str(
            value,
            representation.c_str(),
            int32_t{base},
            mpfr_get_default_rounding_mode()
        );
    }

    ScalarImpl(mpfr_t const& source)
    {
        mpfr_init2(value, mpfr_get_prec(source));
        mpfr_set(value, source, mpfr_get_default_rounding_mode());
    }

    ScalarImpl(mpfr_t&& source)
    {
        source->_mpfr_d = nullptr;
        mpfr_swap(value, source);
    }

    ScalarImpl()
    {
        mpfr_init2(value, mpfr_get_default_prec());
        mpfr_set_zero(value, 1);
    }

    ScalarImpl(mpfr_prec_t precision)
    {
        mpfr_init2(value, precision);
        mpfr_set_zero(value, 1);
    }

    ~ScalarImpl() { mpfr_clear(value); }

    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    mpfr_t value{};
};

static_assert(ptrdiff_t{ScalarImpl::MIN_BASE} > 0);
static_assert(ptrdiff_t{ScalarImpl::MAX_BASE} > 0);
static_assert(ScalarImpl::MIN_BASE < ScalarImpl::MAX_BASE);

static_assert(ptrdiff_t{ScalarImpl::MIN_PRECISION} > 0);
static_assert(ptrdiff_t{ScalarImpl::MAX_PRECISION} > 0);
static_assert(ScalarImpl::MIN_PRECISION < ScalarImpl::MAX_PRECISION);

inline auto clampPrecision(ptrdiff_t const precision) -> size_t
{
    if (std::cmp_less(precision, ScalarImpl::MIN_PRECISION))
    {
        return ScalarImpl::MIN_PRECISION;
    }

    if (std::cmp_greater(precision, ScalarImpl::MAX_PRECISION))
    {
        return ScalarImpl::MAX_PRECISION;
    }

    return static_cast<size_t>(precision);
}
} // namespace detail
