#pragma once

#include "mathfunction.h"
#include "mathstatement.h"
#include <cstdint>
#include <expected>
#include <optional>
#include <string>

enum class MathInterpretationError : uint8_t
{
    ParseError,
    EvaluationError,
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
    MathInterpreter();

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

    [[nodiscard]] auto parse(std::string const& rawInput) const
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
    [[nodiscard]] auto interpret(std::string const& rawInput) const
        -> std::expected<double, MathInterpretationError>;

private:
    MathFunctionDatabase m_functions;
};
