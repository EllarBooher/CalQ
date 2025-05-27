#pragma once

#include "mpreal.h"

// Do not include this file outside of the math/backend folder.

namespace detail
{
class ScalarImpl
{
public:
    ScalarImpl(mpfr::mpreal&& value)
        : value(std::move(value))
    {
    }
    ScalarImpl() = default;

    ~ScalarImpl() = default;

    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    mpfr::mpreal value{};
};
} // namespace detail
