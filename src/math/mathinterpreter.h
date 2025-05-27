#ifndef MATHINTERPRETER_H
#define MATHINTERPRETER_H

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
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

using MathTerm = double;

class MathStatement
{
public:
    [[nodiscard]] auto valid() const -> bool
    {
        return (
            m_terms.empty() && m_operators.empty()
            || m_operators.size() == m_terms.size() - 1
        );
    }
    [[nodiscard]] auto empty() const -> bool { return length() == 0; }

    auto operator==(MathStatement const& rhs) const -> bool;
    [[nodiscard]] auto string() const -> std::string;
    [[nodiscard]] auto evaluate() const -> std::optional<double>;

    [[nodiscard]] auto length() const -> size_t;

    void reset(MathTerm initial);

    /**
     * @brief append - Append a new term prepended by an operator
     * @param mathOp - The binary operator that will come before the term.
     * PEMDAS order applies to the overall statement.
     */
    [[nodiscard]] auto append(MathOp mathOp) -> MathTerm&;

private:
    // A valid statement interleaves terms and operators, or is completely empty
    std::vector<MathTerm> m_terms;
    std::vector<MathOp> m_operators;
};

/**
 * Note: parantheses are not yet supported.
 *
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
 * evaluation is depth first, with nesting indicated by paranthesis.
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
