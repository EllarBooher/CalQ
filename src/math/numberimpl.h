#pragma once

#include "mpfr.h"
#include <cassert>
#include <utility>

// Do not include this file outside of the math module.

namespace detail
{
class ScalarImpl : public __mpfr_struct
{
};

// Values derived from mpfr documentation of mpfr_strtofr
// https://www.mpfr.org/mpfr-current/mpfr.html. Bases and precision outside
// these ranges are undefined behavior, so we clamp them to avoid UB.
static constexpr size_t MIN_BASE = 2;
static constexpr size_t MAX_BASE = 62;

static constexpr size_t MIN_PRECISION = MPFR_PREC_MIN;
static constexpr size_t MAX_PRECISION = MPFR_PREC_MAX;

// Check against the types that mpfr uses

static_assert(int{MIN_BASE} > 0);
static_assert(int{MAX_BASE} > 0);
static_assert(MIN_BASE < MAX_BASE);

static_assert(long{MIN_PRECISION} > 0);
static_assert(long{MAX_PRECISION} > 0);
static_assert(MIN_PRECISION < MAX_PRECISION);

inline auto clampPrecisionFromMPFR(mpfr_prec_t const precision) -> size_t
{
    assert(precision > 0);

    if (precision < 0)
    {
        return 0;
    }

    return precision;
}

inline auto clampPrecisionForMPFR(size_t const precision) -> mpfr_prec_t
{
    assert(precision >= MIN_PRECISION && precision <= MAX_PRECISION);

    if (std::cmp_less(precision, MIN_PRECISION))
    {
        return MIN_PRECISION;
    }

    if (std::cmp_greater(precision, MAX_PRECISION))
    {
        return MAX_PRECISION;
    }

    return precision;
}

inline auto clampBaseForMPFR(size_t const base) -> int
{
    assert(base >= MIN_BASE && base <= MAX_BASE);

    if (base < MIN_BASE)
    {
        return MIN_BASE;
    }

    if (base > MAX_BASE)
    {
        return MAX_BASE;
    }

    return base;
}
} // namespace detail
