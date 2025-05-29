#pragma once

#include "mpfr.h"
#include <cassert>
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

    ScalarImpl(ScalarImpl&& source) noexcept
    {
        value->_mpfr_d = nullptr;
        mpfr_swap(value, source.value);
    }

    ScalarImpl(ScalarImpl const&) = delete;
    auto operator=(ScalarImpl&&) -> ScalarImpl& = delete;
    auto operator=(ScalarImpl const&) -> ScalarImpl& = delete;

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

    static auto nan() -> ScalarImpl
    {
        ScalarImpl result{};
        mpfr_set_nan(result.value);
        return result;
    }

    static auto positiveInf() -> ScalarImpl
    {
        ScalarImpl result{};
        mpfr_set_inf(result.value, 1);
        return result;
    }

    static auto negativeInf() -> ScalarImpl
    {
        ScalarImpl result{};
        mpfr_set_inf(result.value, -1);
        return result;
    }

    ~ScalarImpl()
    {
        // Support the C++ style move constructor by checking if this value was
        // moved from.
        if (value->_mpfr_d == nullptr)
        {
            return;
        }
        mpfr_clear(value);
    }

    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    mpfr_t value{};
};

// Check against the types that mpfr uses

static_assert(int{ScalarImpl::MIN_BASE} > 0);
static_assert(int{ScalarImpl::MAX_BASE} > 0);
static_assert(ScalarImpl::MIN_BASE < ScalarImpl::MAX_BASE);

static_assert(long{ScalarImpl::MIN_PRECISION} > 0);
static_assert(long{ScalarImpl::MAX_PRECISION} > 0);
static_assert(ScalarImpl::MIN_PRECISION < ScalarImpl::MAX_PRECISION);

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
    assert(
        precision >= ScalarImpl::MIN_PRECISION
        && precision <= ScalarImpl::MAX_PRECISION
    );

    if (std::cmp_less(precision, ScalarImpl::MIN_PRECISION))
    {
        return ScalarImpl::MIN_PRECISION;
    }

    if (std::cmp_greater(precision, ScalarImpl::MAX_PRECISION))
    {
        return ScalarImpl::MAX_PRECISION;
    }

    return precision;
}

inline auto clampBaseForMPFR(size_t const base) -> int
{
    assert(base >= ScalarImpl::MIN_BASE && base <= ScalarImpl::MAX_BASE);

    if (base < ScalarImpl::MIN_BASE)
    {
        return ScalarImpl::MIN_BASE;
    }

    if (base > ScalarImpl::MAX_BASE)
    {
        return ScalarImpl::MAX_BASE;
    }

    return base;
}
} // namespace detail
