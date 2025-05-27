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

enum class MathOp : uint8_t
{
    Plus,
    Minus,
    Multiply,
    Divide
};

class MathStatement;
using MathTerm = std::variant<MathStatement, double>;

class MathStatement
{
public:
    MathStatement() = default;

    auto operator=(MathStatement const& other) -> MathStatement&;
    MathStatement(MathStatement const& other);

    auto operator=(MathStatement&& other) noexcept -> MathStatement&;
    MathStatement(MathStatement&& other) noexcept;

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

    auto operator==(MathStatement const& rhs) const -> bool;
    [[nodiscard]] auto string() const -> std::string;
    [[nodiscard]] auto evaluate() const -> std::optional<double>;

    [[nodiscard]] auto length() const -> size_t;

    void reset(MathTerm&& initial);
    auto reset(MathStatement&& initial) -> MathStatement&;

    void setFunction(MathUnaryFunction&& function);

    /**
     * @brief append - Append a new term prepended by an operator
     * @param mathOp - The binary operator that will come before the term.
     * PEMDAS order applies to the overall statement.
     */
    [[nodiscard]] auto append(MathOp mathOp) -> MathTerm&;
    [[nodiscard]] auto appendStatement(MathOp mathOp) -> MathStatement&;

private:
    [[nodiscard]] auto stringTerm(size_t index) const -> std::string;
    [[nodiscard]] auto evaluateTerm(size_t index) const
        -> std::optional<double>;

    // A function that is run as the statements final result. null optional
    // indicates the identity function, so a no-op.
    std::optional<MathUnaryFunction> m_function;
    // A valid statement interleaves terms and operators, or is completely empty
    std::vector<std::unique_ptr<MathTerm>> m_terms;
    std::vector<MathOp> m_operators;
};
