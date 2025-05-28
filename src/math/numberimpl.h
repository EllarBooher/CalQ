#pragma once

#include "mpfr.h"
#include <string>

// Do not include this file outside of the math/backend folder.

namespace detail
{
class ScalarImpl
{
public:
    ScalarImpl(std::string const& representation, uint16_t base)
    {
        mpfr_init2(value, mpfr_get_default_prec());
        mpfr_set_str(
            value,
            representation.c_str(),
            static_cast<int32_t>(base),
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
} // namespace detail
