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
    static auto evaluate(MathStatement const& statement)
        -> std::optional<double>;

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
