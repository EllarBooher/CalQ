#include "lexer.h"

#include <cassert>
#include <deque>
#include <ranges>
#include <utility>

namespace calqmath
{
auto operator==(TokenIdentifier const& lhs, TokenIdentifier const& rhs) -> bool
{
    return lhs.m_functionName == rhs.m_functionName;
};
auto operator==(TokenNumber const& lhs, TokenNumber const& rhs) -> bool
{
    return lhs.m_decimalRepresentation == rhs.m_decimalRepresentation;
}

auto operator==(
    TokenOpenBracket const& /*lhs*/, TokenOpenBracket const& /*rhs*/
) -> bool
{
    return true;
}
auto operator==(
    TokenClosedBracket const& /*lhs*/, TokenClosedBracket const& /*rhs*/
) -> bool
{
    return true;
}
} // namespace calqmath

namespace
{
auto isAlpha(char const character) -> bool
{
    return std::isalpha(character) != 0;
}

auto trim(std::string const& rawInput) -> std::deque<char>
{
    auto const isnotwhitespace = [](char character)
    { return !std::isspace(character); };

    auto trimmed{rawInput | std::views::filter(isnotwhitespace)};

    return {trimmed.begin(), trimmed.end()};
}

auto popTokenOffFront(std::deque<char>& trimmed)
    -> std::optional<calqmath::Token>
{
    static std::string const digits{"0123456789"};
    static char constexpr decimal{'.'};

    auto const character = trimmed.front();
    trimmed.pop_front();
    std::optional<calqmath::Token> emitted;

    if (character == '+')
    {
        emitted = calqmath::TokenOperator::Plus;
    }
    else if (character == '-')
    {
        emitted = calqmath::TokenOperator::Minus;
    }
    else if (character == '*')
    {
        emitted = calqmath::TokenOperator::Multiply;
    }
    else if (character == '/')
    {
        emitted = calqmath::TokenOperator::Divide;
    }
    else if (character == '(')
    {
        emitted = calqmath::TokenOpenBracket{};
    }
    else if (character == ')')
    {
        emitted = calqmath::TokenClosedBracket{};
    }
    else if (isAlpha(character))
    {
        std::string identifier{character};
        while (!trimmed.empty()
               && (isAlpha(trimmed.front()) || digits.contains(trimmed.front()))
        )
        {
            identifier += trimmed.front();
            trimmed.pop_front();
        };
        emitted = calqmath::TokenIdentifier{std::move(identifier)};
    }
    else if (digits.contains(character) || character == decimal)
    {
        std::string decimalRepresentation{character};
        bool fractional{character == decimal};
        while (!trimmed.empty()
               && (digits.contains(trimmed.front())
                   || (trimmed.front() == decimal && !fractional)))
        {
            decimalRepresentation += trimmed.front();
            fractional |= trimmed.front() == decimal;
            trimmed.pop_front();
        };

        if (decimalRepresentation == ".")
        {
            return std::nullopt;
        }

        emitted = calqmath::TokenNumber{std::move(decimalRepresentation)};
    }

    return emitted;
}
} // namespace

auto calqmath::Lexer::convert(std::string const& rawInput)
    -> std::optional<std::vector<calqmath::Token>>
{
    std::vector<calqmath::Token> tokens{};

    std::deque<char> trimmed{trim(rawInput)};

    while (!trimmed.empty())
    {
        auto const emitted = popTokenOffFront(trimmed);

        if (!emitted.has_value())
        {
            return std::nullopt;
        }

        tokens.push_back(emitted.value());
    }

    return tokens;
}
