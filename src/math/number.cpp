#include "number.h"

#include <cassert>
#include <cmath>

void calqmath::initBignumBackend()
{
    // Such a high default may have performance implications, but we aren't
    // performing a lot of calculations. Most user input statements will have a
    // couple dozen calculations at most.
    auto constexpr DEFAULT_MINIMUM_PRECISION{500};
    mpf_set_default_prec(static_cast<mp_bitcnt_t>(DEFAULT_MINIMUM_PRECISION));
}

auto calqmath::getBignumBackendPrecision(size_t const base) -> size_t
{
    assert(base > 0);
    return static_cast<size_t>(mpf_get_default_prec()) * std::numbers::ln2
         / std::log(base);
}
