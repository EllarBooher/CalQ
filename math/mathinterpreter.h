#ifndef MATHINTERPRETER_H
#define MATHINTERPRETER_H

#include <expected>
#include <optional>
#include <string>
#include <vector>

enum class MathInterpretationError
{
    ParseError,
    EvaluationError,
};

enum class MathOp
{
    Plus,
    Minus,
    Multiply,
    Divide
};

struct MathStatement
{
    std::vector<double> terms;
    std::vector<MathOp> operators;

    auto isValid() const -> bool
    {
        return (
            terms.empty() && operators.empty()
            || operators.size() == terms.size() - 1
        );
    }
};

auto operator==(MathStatement const& lhs, MathStatement const& rhs) -> bool;

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
    static auto parse(std::string const& rawInput)
        -> std::optional<MathStatement>;
    static auto evaluate(MathStatement const& statement)
        -> std::optional<double>;

    /**
     * Chains all methods, to get from raw user input to the final mathematical
     * result or error.
     *
     * @brief interpret - Parses user input as a mathematical statement and
     * returns the evaluated answer.
     *
     * @param rawInput - The stringified equation.
     */
    static auto interpret(std::string const& rawInput)
        -> std::expected<double, MathInterpretationError>;
};

#endif // MATHINTERPRETER_H
