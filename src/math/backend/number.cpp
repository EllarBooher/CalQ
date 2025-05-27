#include "number.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>

#include "gmpxx.h"
#include "numberimpl.h"

namespace calqmath
{
void initBignumBackend()
{
    // Such a high default may have performance implications, but we aren't
    // performing a lot of calculations. Most user input statements will have a
    // couple dozen calculations at most.
    auto constexpr DEFAULT_MINIMUM_PRECISION{500};
    mpf_set_default_prec(static_cast<mp_bitcnt_t>(DEFAULT_MINIMUM_PRECISION));
}

auto getBignumBackendPrecision(size_t const base) -> size_t
{
    assert(base > 0);
    return static_cast<size_t>(mpf_get_default_prec()) * std::numbers::ln2
         / std::log(base);
}

Scalar::Scalar(std::string const& representation, uint16_t const base)
{
    m_impl = std::make_unique<detail::ScalarImpl>(
        mpf_class{representation, mpf_get_default_prec(), base}
    );
}

auto Scalar::operator=(Scalar&& other) noexcept -> Scalar&
{
    m_impl = std::exchange(other.m_impl, nullptr);
    return *this;
}
auto Scalar::operator=(Scalar const& other) -> Scalar&
{
    auto value = other.m_impl->value;
    m_impl = std::make_unique<detail::ScalarImpl>(std::move(value));
    return *this;
}

Scalar::Scalar(Scalar&& other) noexcept { *this = std::move(other); }

Scalar::Scalar(Scalar const& other) { *this = other; }

Scalar::~Scalar() = default;

auto Scalar::toMantissaExponent() const -> std::tuple<std::string, ptrdiff_t>
{
    size_t constexpr PRECISION_DIGITS = 10;
    mp_exp_t exponent;
    auto mantissa =
        m_impl->value.get_str(exponent, DEFAULT_BASE, PRECISION_DIGITS);

    return std::make_tuple(mantissa, exponent);
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
    else if (static_cast<size_t>(exponent) >= mantissa.size())
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

    for (ptrdiff_t i = decomposition.preDecimal.size() - 3; i >= 1; i -= 3)
    {
        decomposition.preDecimal.insert(
            static_cast<size_t>(i), 1, DIGIT_SEPARATOR
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
    return m_impl->value == rhs.m_impl->value;
}

auto Scalar::operator!=(Scalar const& rhs) const -> bool
{
    return m_impl->value != rhs.m_impl->value;
}

auto Scalar::operator+(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    result.m_impl = std::make_unique<detail::ScalarImpl>(
        this->m_impl->value + rhs.m_impl->value
    );
    return result;
}
auto Scalar::operator-(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    result.m_impl = std::make_unique<detail::ScalarImpl>(
        this->m_impl->value - rhs.m_impl->value
    );
    return result;
}
auto Scalar::operator*(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    result.m_impl = std::make_unique<detail::ScalarImpl>(
        this->m_impl->value * rhs.m_impl->value
    );
    return result;
}
auto Scalar::operator/(Scalar const& rhs) const -> Scalar
{
    Scalar result{};
    result.m_impl = std::make_unique<detail::ScalarImpl>(
        this->m_impl->value / rhs.m_impl->value
    );
    return result;
}

Scalar::Scalar(detail::ScalarImpl&& impl)
{
    m_impl = std::make_unique<detail::ScalarImpl>(std::move(impl.value));
    impl.value = mpf_class{};
}

Scalar::Scalar() = default;
} // namespace calqmath
