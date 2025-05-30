#pragma once

#include "function_database.h"
#include <cstdint>
#include <expected>
#include <string>

namespace calqmath
{
enum class InterpretError : uint8_t
{
    LexError,
    ParseError,
    EvaluationError,
};

/**
 * Parses the given plaintext string, evaluating it as a mathematical
 * expression, with the following grammar:
 *
 *     letter     ::= ? ASCII characters a-z and A-Z ?
 *     digit      ::= ? ASCII characters 0-9 ?
 *     function   ::= letter,{letter | digit}
 *     operator   ::= "+" | "-" | "*" | "/"
 *
 *     number     ::= ( {digit} ["."] {digit} ) - "."
 *
 *     term       ::= number | expression
 *     expression ::= ["-"] [function] "(" term {operator term} ")"
 *
 * Whitespace is eliminated and has no impact on the parsing or evaluation.
 * Mathematical evaluation uses standard BEDMAS/PEMDAS order. Thus
 * evaluation is depth first, with nesting indicated by parenthesis.
 *
 * The outermost expression has implied parantheses, and these do not need to be
 * present in the user input.
 */
class Interpreter
{
public:
    Interpreter();

    /**
     * @brief prettify - Converts the input string into a prettier form.
     *
     * Any string can be converted, and this method does not check for being a
     * valid mathematical expression. This is for echoing user input in a
     * standardized form.
     *
     * @param rawInput - The string to prettify.
     * @return Returns the string, with whitespace removed.
     */
    static auto prettify(std::string const& rawInput) -> std::string;

    /**
     * @brief interpret - Parses user input as a mathematical expression and
     * returns the evaluated answer.
     *
     * Chains all methods, to get from raw user input to the final mathematical
     * result or error.
     *
     * @param rawInput - The stringified equation.
     */
    [[nodiscard]] auto interpret(std::string const& rawInput) const
        -> std::expected<Scalar, InterpretError>;

private:
    FunctionDatabase m_functions;
};
} // namespace calqmath
