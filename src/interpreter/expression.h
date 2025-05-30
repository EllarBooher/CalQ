#pragma once

#include "function_database.h"
#include <algorithm>
#include <cassert>
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

class Expression;
using Term = std::variant<Expression, Scalar>;

/*
 * An AST of a mathematical expression, where the nodes are terms in
 * the mathematical sense.
 *
 * Can be written to, adding extra nodes/terms.
 * Can be read from, evaluating the result of the calculation it represents.
 */
class Expression
{
public:
    Expression() = default;

    auto operator=(Expression const& other) -> Expression&;
    Expression(Expression const& other);

    auto operator=(Expression&& other) noexcept -> Expression&;
    Expression(Expression&& other) noexcept;

    auto operator==(Expression const& rhs) const -> bool;

    /**
     * @brief valid - Checks the invariants of the Expression
     * @return Whether or not the Expression is valid e.g. can evaluate to a
     * result
     */
    [[nodiscard]] auto valid() const -> bool;

    /**
     * @brief empty - Checks if the Expression is empty.
     * @return Whether or not the Expression is empty e.g. has no terms and
     * result is identically zero/null.
     */
    [[nodiscard]] auto empty() const -> bool;

    /**
     * @brief string - Converts the Expression into a human-readable string
     * format.
     * @return The Expression as a string.
     */
    [[nodiscard]] auto string() const -> std::string;

    /**
     * @brief evaluate - Evaluates the result of the expression, combining all
     * terms.
     *
     * No memoization, this calculation costs the same each time.
     *
     * @return The result of the evaluation. If the tree was invalid or some
     * other error occured, returns nullopt.
     */
    [[nodiscard]] auto evaluate() const -> std::optional<Scalar>;

    [[nodiscard]] auto termCount() const -> size_t;

    /**
     * @brief reset - Clears this Expression and leaves in its place a single
     * term.
     * @param initial - The term to replace all original terms with.
     */
    void reset(Term&& initial);

    /**
     * @brief reset - Clears this Expression and leaves in its place a single
     * term of type Expression.
     * @param initial - The expression to replace all original terms with.
     */
    auto reset(Expression&& initial) -> Expression&;

    /**
     * @brief setNegate - Set whether or not the expression is negated at the
     * very end of evaluation.
     *
     * As an example, consider 1 + -(1 + 1). The term -(1 + 1) is an Expression
     * with negation turned on.
     *
     * @param negate
     */
    void setNegate(bool negate);

    /**
     * @brief setFunction - Sets the unary function that takes all terms as
     * input.
     *
     * As an example, consider 1 + sin(1 + 1). The term sin(1 + 1) is an
     * Expression whose function is `sine`.
     *
     * Possible negation occurs after function evaluation, e.g. -sin(1) = -1 *
     * sin(1).
     *
     * @param function
     */
    void setFunction(UnaryFunction&& function);

    /**
     * @brief backTerm - Gets the last term in this expressions list of terms,
     * for writing purposes.
     */
    auto backTerm() -> Term&;

    /**
     * @brief append - Append a new term prepended by an operator
     * @param mathOp - The binary operator that will come before the term.
     * PEMDAS order applies to the overall expression.
     */
    auto append(BinaryOp mathOp) -> Term&;

    /**
     * @brief appendExpression - Append a new expresion (as a term) prepended by
     * an operator.
     * @see Expression::append
     */
    auto appendExpression(BinaryOp mathOp) -> Expression&;

private:
    [[nodiscard]] auto stringTerm(size_t index) const -> std::string;
    [[nodiscard]] auto evaluateTerm(size_t index) const
        -> std::optional<Scalar>;

    // Negate the expression's evaluated value as the final step.
    bool m_negate{false};

    // A function that is run as the expressions final result. null optional
    // indicates the identity function, so a no-op.
    std::optional<UnaryFunction> m_function;

    // A valid expression interleaves terms and operators, or is completely
    // empty
    std::vector<std::unique_ptr<Term>> m_terms;
    std::vector<BinaryOp> m_operators;
};
} // namespace calqmath
