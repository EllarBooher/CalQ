#pragma once

#include "gmpxx.h"

// Do not include this file outside of the math/backend folder.

namespace detail
{
class ScalarImpl
{
public:
    ScalarImpl(mpf_class&& value)
        : value(std::move(value))
    {
    }

    ~ScalarImpl() = default;

    // NOLINTNEXTLINE (misc-non-private-member-variables-in-classes)
    mpf_class value{};
};
} // namespace detail
