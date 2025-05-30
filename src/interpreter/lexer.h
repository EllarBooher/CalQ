#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace calqmath
{
// Function name identifier. We have no general identifier type.
struct TokenFunction
{
    std::string m_functionName;
};

/*
 * Number literal. The only literals right now are decimals of the form
 * "123.456", "-123.456", "123.", ".456", or "123".
 */
struct TokenNumber
{
    std::string m_decimalRepresentation;
};

// Operators, of any n-nary-ness
enum class TokenOperator : uint8_t
{
    Plus,
    Minus,
    Multiply,
    Divide
};

/*
 * Split up the bracket types, since they are fundamentally different and not
 * semantically interchangeable. This simplifies parsing.
 */

struct TokenOpenBracket
{
};

struct TokenClosedBracket
{
};

using Token = std::variant<
    TokenFunction,
    TokenNumber,
    TokenOpenBracket,
    TokenClosedBracket,
    TokenOperator>;

auto operator==(TokenFunction const& lhs, TokenFunction const& rhs) -> bool;
auto operator==(TokenNumber const& lhs, TokenNumber const& rhs) -> bool;
auto operator==(TokenOpenBracket const& lhs, TokenOpenBracket const& rhs)
    -> bool;
auto operator==(TokenClosedBracket const& lhs, TokenClosedBracket const& rhs)
    -> bool;

/**
 * A lexer geared heavily towards the sort of input for a calculator, not a
 * general programming language.
 *
 * Convert a raw user input string representing a
 * mathematical expression into an array of tokens.
 *
 * For example, "5.0+(7.0--5.0)" becomes ["5.0","+","(","7.0","-","-",
 * "5.0",")"]. This example uses strings, but the tokens are actual values,
 * see calqmath::Token.
 *
 * The grammar is not known at this stage, so incorrect streams may be
 * emitted. For example, several literals in a row with no operators.
 */
class Lexer
{
public:
    static auto convert(std::string const& rawInput)
        -> std::optional<std::vector<Token>>;
};
} // namespace calqmath
