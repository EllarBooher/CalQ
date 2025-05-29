#include "number.h"

#include "mpfr.h"
#include "numberimpl.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <numbers>
#include <optional>
#include <utility>

namespace calqmath
{
void initBignumBackend()
{
    // Such a high default may have performance implications, but we aren't
    // performing a lot of calculations. Most user input statements will have a
    // couple dozen calculations at most.
    auto constexpr DEFAULT_MINIMUM_PRECISION{500};
    mpfr_set_default_prec(mpfr_prec_t{DEFAULT_MINIMUM_PRECISION});
}

auto getBignumBackendPrecision(size_t const base) -> size_t
{
    assert(base > 0);
    auto const precision{detail::clampPrecision(mpfr_get_default_prec())};
    return precision * std::numbers::ln2 / std::log(base);
}

auto Scalar::precisionMin() -> size_t
{
    return detail::ScalarImpl::MIN_PRECISION;
}
auto Scalar::precisionMax() -> size_t
{
    return detail::ScalarImpl::MAX_PRECISION;
}

Scalar::Scalar(size_t precision)
{
    assert(std::cmp_less(precision, std::numeric_limits<mpfr_prec_t>::max()));
    m_impl =
        std::make_unique<detail::ScalarImpl>(static_cast<mpfr_prec_t>(precision)
        );
}

auto Scalar::baseMin() -> size_t { return detail::ScalarImpl::MIN_BASE; }
auto Scalar::baseMax() -> size_t { return detail::ScalarImpl::MAX_BASE; }

Scalar::Scalar(std::string const& representation, size_t const base)
{
    m_impl = std::make_unique<detail::ScalarImpl>(
        representation, std::clamp(base, baseMin(), baseMax())
    );
}

auto Scalar::operator=(Scalar&& other) noexcept -> Scalar&
{
    m_impl = std::exchange(other.m_impl, nullptr);
    return *this;
}
auto Scalar::operator=(Scalar const& other) -> Scalar&
{
    m_impl = std::make_unique<detail::ScalarImpl>(other.m_impl->value);
    return *this;
}

Scalar::Scalar(Scalar&& other) noexcept { *this = std::move(other); }

Scalar::Scalar(Scalar const& other) { *this = other; }

Scalar::Scalar() { m_impl = std::make_unique<detail::ScalarImpl>(); }
Scalar::~Scalar() = default;

auto Scalar::toMantissaExponent() const -> std::tuple<std::string, ptrdiff_t>
{
    std::tuple<std::string, ptrdiff_t> result{};

    size_t constexpr PRECISION_DIGITS = 10;
    mpfr_exp_t exponent;

    auto* const pMantissa = mpfr_get_str(
        nullptr,
        &exponent,
        DEFAULT_BASE,
        PRECISION_DIGITS,
        m_impl->value,
        mpfr_get_default_rounding_mode()
    );

    std::get<0>(result) = pMantissa;
    std::get<1>(result) = ptrdiff_t{exponent};

    auto& str = std::get<0>(result);
    str.erase(str.find_last_not_of('0') + 1);

    mpfr_free_str(pMantissa);

    return result;
}

struct ScalarStringDecomposition
{
    bool negative;
    std::string preDecimal;
    std::string postDecimal;
    std::optional<int64_t> exponent;
};

namespace
{
auto decompose(calqmath::Scalar const& number) -> ScalarStringDecomposition
{
    auto [mantissa, exponent] = number.toMantissaExponent();

    /*
     * GMP scientific notation format:
     *      0.MANTISSA x BASE^exponent
     * What we want:
     *      M.ANTISSA x BASE^(exponent-1)
     */

    ptrdiff_t constexpr READABLE_MIN = -2;
    ptrdiff_t constexpr READABLE_MAX = 8;

    ScalarStringDecomposition decomposition{};

    if (mantissa.empty())
    {
        assert(exponent == 0);
        decomposition.preDecimal = "0";
        return decomposition;
    }

    // Should we check the number itself? e.g. number < 0
    if (mantissa.at(0) == '-')
    {
        decomposition.negative = true;
        mantissa.erase(0, 1);
    }

    // Scientific notation, easiest case, looks like M.ANTISSAeEXPONENT
    if (exponent <= READABLE_MIN || exponent >= READABLE_MAX)
    {
        if (mantissa.size() == 1)
        {
            mantissa += '0';
        }

        decomposition.preDecimal = mantissa.substr(0, 1);
        decomposition.postDecimal = mantissa.substr(1);
        decomposition.exponent = exponent - 1;
    }
    else if (exponent <= 0)
    {
        // Numbers like 0.0000MANTISSA
        decomposition.postDecimal =
            std::string(std::abs(exponent), '0') + mantissa;
    }
    else if (std::cmp_greater_equal(exponent, mantissa.size()))
    {
        // Numbers like MANTISSA0000
        decomposition.preDecimal =
            mantissa
            + std::string(static_cast<size_t>(exponent) - mantissa.size(), '0');
    }
    else
    {
        // Numbers like MANT.ISSA
        decomposition.preDecimal = mantissa.substr(0, exponent);
        decomposition.postDecimal = mantissa.substr(exponent);
    }

    return decomposition;
}
auto format(ScalarStringDecomposition decomposition) -> std::string
{
    auto constexpr DIGIT_SEPARATOR = '_';
    auto constexpr NEGATIVE_SIGN = '-';

    for (size_t i = 3; i < decomposition.preDecimal.size(); i += 4)
    {
        decomposition.preDecimal.insert(
            decomposition.preDecimal.size() - i, 1, DIGIT_SEPARATOR
        );
    }

    for (size_t i = 3; i < decomposition.postDecimal.size(); i += 4)
    {
        decomposition.postDecimal.insert(i, 1, DIGIT_SEPARATOR);
    }

    assert(
        !decomposition.preDecimal.empty() || !decomposition.postDecimal.empty()
    );

    char const* pattern;

    if (decomposition.exponent.has_value())
    {
        pattern = decomposition.negative ? "{0}{1}.{2}e{3}" : "{1}.{2}e{3}";
    }
    else if (decomposition.preDecimal.empty())
    {
        pattern = decomposition.negative ? "{0}0.{2}" : "0.{2}";
    }
    else if (decomposition.postDecimal.empty())
    {
        pattern = decomposition.negative ? "{0}{1}" : "{1}";
    }
    else
    {
        pattern = decomposition.negative ? "{0}{1}.{2}" : "{1}.{2}";
    }

    auto const exponent{decomposition.exponent.value_or(0)};

    return std::vformat(
        pattern,
        std::make_format_args(
            NEGATIVE_SIGN,
            decomposition.preDecimal,
            decomposition.postDecimal,
            exponent
        )
    );
}
} // namespace

auto Scalar::toString() const -> std::string
{
    auto decomposition = decompose(*this);
    return format(decomposition);
}

auto Scalar::operator==(Scalar const& rhs) const -> bool
{
    return mpfr_equal_p(m_impl->value, rhs.m_impl->value) != 0;
}

auto Scalar::operator!=(Scalar const& rhs) const -> bool
{
    return !(*this == rhs);
}

auto Scalar::operator+(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    mpfr_add(
        result.m_impl->value,
        m_impl->value,
        rhs.m_impl->value,
        mpfr_get_default_rounding_mode()
    );
    return result;
}

auto Scalar::operator-(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    mpfr_sub(
        result.m_impl->value,
        m_impl->value,
        rhs.m_impl->value,
        mpfr_get_default_rounding_mode()
    );
    return result;
}

auto Scalar::operator*(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    mpfr_mul(
        result.m_impl->value,
        m_impl->value,
        rhs.m_impl->value,
        mpfr_get_default_rounding_mode()
    );
    return result;
}
auto Scalar::operator/(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    mpfr_div(
        result.m_impl->value,
        m_impl->value,
        rhs.m_impl->value,
        mpfr_get_default_rounding_mode()
    );
    return result;
}

auto Scalar::operator-() const -> Scalar
{
    Scalar result{};
    mpfr_neg(
        result.m_impl->value, m_impl->value, mpfr_get_default_rounding_mode()
    );
    return result;
}

} // namespace calqmath
