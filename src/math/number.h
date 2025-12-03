#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace detail
{
class ScalarImpl;
} // namespace detail

namespace calqmath
{
class Functions;

// Has a major, roughly linear, impact on performance
size_t constexpr DEFAULT_BASE_2_PRECISION = 128;

// For string interpretation
size_t constexpr DEFAULT_BASE = 10;

auto getBignumBackendPrecision(size_t base = DEFAULT_BASE) -> size_t;
void initBignumBackend();

enum class Sign : uint8_t
{
    NEGATIVE,
    ZERO,
    POSITIVE,
};

class Scalar
{
public:
    static auto precisionMin() -> size_t;
    static auto precisionMax() -> size_t;

    static auto baseMin() -> size_t;
    static auto baseMax() -> size_t;

    /*
     * Base gets clamped to the range returned by baseMax and baseMin.
     */
    explicit Scalar(
        std::string const& representation,
        size_t precision = DEFAULT_BASE_2_PRECISION,
        size_t base = DEFAULT_BASE
    );

    explicit Scalar(
        double number = 0.0, size_t precision = DEFAULT_BASE_2_PRECISION
    );

    Scalar(Scalar&& other) noexcept;
    Scalar(Scalar const& other);

    auto operator=(Scalar&& other) noexcept -> Scalar&;
    auto operator=(Scalar const& other) -> Scalar&;

    ~Scalar();

    /**
     * The precision of the mantissa is finite, but the string can be shorter if
     * the number is. The default precision is 10.
     *
     * @brief toMantissaExponent Returns a pair of mantissa (as a base-10
     * string) plus exponent.
     * @return
     */
    [[nodiscard]] auto toMantissaExponent() const
        -> std::tuple<std::string, ptrdiff_t>;

    static auto zero() -> Scalar;
    static auto nan() -> Scalar;
    static auto positiveInf() -> Scalar;
    static auto negativeInf() -> Scalar;

    static constexpr char const* ZERO_REPRESENTATION = "0";
    static constexpr char const* NAN_REPRESENTATION = "NaN";
    static constexpr char const* POSITIVE_INFINITY_REPRESENTATION = "Inf";
    static constexpr char const* NEGATIVE_INFINITY_REPRESENTATION = "-Inf";

    [[nodiscard]] auto toString() const -> std::string;

    [[nodiscard]] auto sign() const -> Sign;
    [[nodiscard]] auto isNaN() const -> bool;

    [[nodiscard]] auto toDouble() const -> double;

    auto operator==(Scalar const& rhs) const -> bool;
    auto operator!=(Scalar const& rhs) const -> bool;

    auto operator+(Scalar const& rhs) const -> Scalar;
    auto operator-(Scalar const& rhs) const -> Scalar;
    auto operator*(Scalar const& rhs) const -> Scalar;
    auto operator/(Scalar const& rhs) const -> Scalar;

    auto operator-() const -> Scalar;

    friend Functions;

private:
    struct no_set
    {
    };

    explicit Scalar(no_set, size_t precision = DEFAULT_BASE_2_PRECISION);

    std::unique_ptr<detail::ScalarImpl> p_impl;
};
} // namespace calqmath
