#include "mathstringify.h"

#include <cassert>
#include <cstdint>
#include <format>

struct ScalarStringDecomposition
{
    std::string preDecimal;
    std::string postDecimal;
    std::optional<int64_t> exponent;
};

namespace
{
auto decompose(Scalar const& number) -> ScalarStringDecomposition
{
    size_t constexpr PRECISION_DIGITS = 10;
    size_t constexpr BASE = 10;

    mp_exp_t exponent;
    // Extract base-10 mantissa
    auto mantissa = number.get_str(exponent, BASE, PRECISION_DIGITS);

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

    if (decomposition.exponent.has_value())
    {
        return std::format(
            "{}.{}e{}",
            decomposition.preDecimal,
            decomposition.postDecimal,
            decomposition.exponent.value()
        );
    }

    assert(
        !decomposition.preDecimal.empty() || !decomposition.postDecimal.empty()
    );

    if (decomposition.preDecimal.empty())
    {
        return std::format("0.{}", decomposition.postDecimal);
    }

    if (decomposition.postDecimal.empty())
    {
        return decomposition.preDecimal;
    }

    return std::format(
        "{}.{}", decomposition.preDecimal, decomposition.postDecimal
    );
}
} // namespace

auto calqmath::toString(Scalar const& number) -> std::string
{
    auto decomposition = decompose(number);
    return format(decomposition);
}
