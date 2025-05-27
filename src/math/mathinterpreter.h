#ifndef MATHINTERPRETER_H
#define MATHINTERPRETER_H

#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class MathInterpretationError : uint8_t
{
    ParseError,
    EvaluationError,
};

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

    // A valid statement interleaves terms and operators, or is completely empty
    std::vector<std::unique_ptr<MathTerm>> m_terms;
    std::vector<MathOp> m_operators;
};

/**
 * Parses the given plaintext string, evaluating it as a mathematical
 * statement, with the following grammar:
 *
 *     statement ::= term , { operator , term } ;
 *     operator ::= "+" | "-" | "*" | "/"
 *     term ::= number
 *        | "(" , statement , ")"
 *        | "(" , term , ")" ;
 *     number ::= { digit }
 *        | { digit } , "." , { digit }
 *
 * Whitespace is eliminated and has no impact on the parsing or evaluation.
 * Mathematical evaluation uses standard BEDMAS/PEMDAS order. Thus
 * evaluation is depth first, with nesting indicated by parenthesis.
 */
class MathInterpreter
{
public:
    /**
     * @brief prettify - Converts the input string into a prettier form.
     *
     * Any string can be converted, and this method does not check for being a
     * valid mathematical statement. This is for echoing user input in a
     * standardized form.
     *
     * @param rawInput - The string to prettify.
     * @return Returns the string, with whitespace removed.
     */
    static auto prettify(std::string const& rawInput) -> std::string;

    static auto parse(std::string const& rawInput)
        -> std::optional<MathStatement>;

    /**
     * @brief interpret - Parses user input as a mathematical statement and
     * returns the evaluated answer.
     *
     * Chains all methods, to get from raw user input to the final mathematical
     * result or error.
     *
     * @param rawInput - The stringified equation.
     */
    static auto interpret(std::string const& rawInput)
        -> std::expected<double, MathInterpretationError>;
};

#endif // MATHINTERPRETER_H
