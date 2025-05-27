#include "mathstringify.h"

#include <cassert>
#include <format>

auto calqmath::toString(Scalar const& number) -> std::string
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

    char constexpr DIGIT_SEPARATOR = '_';

    std::string preDecimal{};
    std::string postDecimal{};

    // Scientific notation, easiest case, looks like M.ANTISSAeEXPONENT
    if (exponent <= READABLE_MIN || exponent >= READABLE_MAX)
    {
        if (mantissa.size() == 1)
        {
            mantissa += '0';
        }
        preDecimal = mantissa.substr(0, 1);
        postDecimal = mantissa.substr(1);

        for (size_t i = 3; i < postDecimal.size(); i += 4)
        {
            postDecimal.insert(i, 1, DIGIT_SEPARATOR);
        }

        return std::format("{}.{}e{}", preDecimal, postDecimal, exponent - 1);
    }

    if (exponent <= 0)
    {
        // Numbers like 0.000MANTISSA
        postDecimal = std::string(std::abs(exponent), '0') + mantissa;
    }
    else if (static_cast<size_t>(exponent) >= mantissa.size())
    {
        // Numbers like MANTISSA0000
        preDecimal =
            mantissa
            + std::string(static_cast<size_t>(exponent) - mantissa.size(), '0');
    }
    else
    {
        // Numbers like MANT.ISSA
        preDecimal = mantissa.substr(0, exponent);
        postDecimal = mantissa.substr(exponent);
    }

    for (ptrdiff_t i = preDecimal.size() - 3; i >= 1; i -= 3)
    {
        preDecimal.insert(static_cast<size_t>(i), 1, DIGIT_SEPARATOR);
    }
    for (size_t i = 3; i < postDecimal.size(); i += 4)
    {
        postDecimal.insert(i, 1, DIGIT_SEPARATOR);
    }

    assert(!preDecimal.empty() || !postDecimal.empty());
    if (postDecimal.empty())
    {
        return preDecimal;
    }

    if (preDecimal.empty())
    {
        return std::format("0.{}", postDecimal);
    }

    return std::format("{}.{}", preDecimal, postDecimal);
}
