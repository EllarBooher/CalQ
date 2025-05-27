#pragma once

#include "gmpxx.h"

// Include this file as the source of numeric types and the interface with GMP.

namespace calqmath
{
size_t constexpr DEFAULT_BASE = 10;

auto getBignumBackendPrecision(size_t base = DEFAULT_BASE) -> size_t;
void initBignumBackend();

using Scalar = mpf_class;
} // namespace calqmath
