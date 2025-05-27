#pragma once

#include "mathfunction.h"
#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace calqmath
{
enum class BinaryOp : uint8_t
{
    Plus,
    Minus,
    Multiply,
    Divide
};

class Statement;
using Term = std::variant<Statement, Scalar>;

class Statement
{
public:
    Statement() = default;

    auto operator=(Statement const& other) -> Statement&;
    Statement(Statement const& other);

    auto operator=(Statement&& other) noexcept -> Statement&;
    Statement(Statement&& other) noexcept;

    [[nodiscard]] auto valid() const -> bool
    {
        bool const completelyEmpty = m_terms.empty() && m_operators.empty();
        bool const oneOpBetweenAllConsecutiveTerms =
            m_operators.size() == m_terms.size() - 1;
        bool const noNullChildren =
            std::count(m_terms.begin(), m_terms.end(), nullptr) == 0;

        return (
            (completelyEmpty || oneOpBetweenAllConsecutiveTerms)
            && noNullChildren
        );
    }
    [[nodiscard]] auto empty() const -> bool { return length() == 0; }

    auto operator==(Statement const& rhs) const -> bool;
    [[nodiscard]] auto string() const -> std::string;
    [[nodiscard]] auto evaluate() const -> std::optional<Scalar>;

    [[nodiscard]] auto length() const -> size_t;

    void reset(Term&& initial);
    auto reset(Statement&& initial) -> Statement&;

    void setFunction(UnaryFunction&& function);

    /**
     * @brief append - Append a new term prepended by an operator
     * @param mathOp - The binary operator that will come before the term.
     * PEMDAS order applies to the overall statement.
     */
    [[nodiscard]] auto append(BinaryOp mathOp) -> Term&;
    [[nodiscard]] auto appendStatement(BinaryOp mathOp) -> Statement&;

private:
    [[nodiscard]] auto stringTerm(size_t index) const -> std::string;
    [[nodiscard]] auto evaluateTerm(size_t index) const
        -> std::optional<Scalar>;

    // A function that is run as the statements final result. null optional
    // indicates the identity function, so a no-op.
    std::optional<UnaryFunction> m_function;
    // A valid statement interleaves terms and operators, or is completely empty
    std::vector<std::unique_ptr<Term>> m_terms;
    std::vector<BinaryOp> m_operators;
};
} // namespace calqmath
