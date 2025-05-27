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

size_t constexpr DEFAULT_BASE = 10;

auto getBignumBackendPrecision(size_t base = DEFAULT_BASE) -> size_t;
void initBignumBackend();

class Scalar
{
public:
    explicit Scalar(double const number) = delete;
    explicit Scalar(
        std::string const& representation, uint16_t base = DEFAULT_BASE
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
    [[nodiscard]] auto toString() const -> std::string;

    auto operator==(Scalar const& rhs) const -> bool;
    auto operator!=(Scalar const& rhs) const -> bool;

    auto operator+(Scalar const& rhs) const -> Scalar;
    auto operator-(Scalar const& rhs) const -> Scalar;
    auto operator*(Scalar const& rhs) const -> Scalar;
    auto operator/(Scalar const& rhs) const -> Scalar;

    auto operator-() const -> Scalar;

    friend Functions;

private:
    explicit Scalar(detail::ScalarImpl&& impl);
    Scalar();

    std::unique_ptr<detail::ScalarImpl> m_impl;
};
} // namespace calqmath
