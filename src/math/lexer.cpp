#include "lexer.h"

#include <cassert>
#include <cstring>
#include <deque>
#include <ranges>

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
} // namespace

auto calqmath::Lexer::convert(std::string const& rawInput)
    -> std::optional<std::vector<calqmath::Token>>
{
    std::vector<calqmath::Token> tokens{};

    std::deque<char> trimmed{trim(rawInput)};

    static std::string constexpr digits{"0123456789"};
    static char constexpr decimal{'.'};

    while (!trimmed.empty())
    {
        auto const character = trimmed.front();
        trimmed.pop_front();
        std::optional<calqmath::Token> emitted;

        if (character == '+')
        {
            emitted = TokenOperator::Plus;
        }
        else if (character == '-')
        {
            emitted = TokenOperator::Minus;
        }
        else if (character == '*')
        {
            emitted = TokenOperator::Multiply;
        }
        else if (character == '/')
        {
            emitted = TokenOperator::Divide;
        }
        else if (character == '(')
        {
            emitted = TokenParanthesis::Open;
        }
        else if (character == ')')
        {
            emitted = TokenParanthesis::Close;
        }
        else if (isAlpha(character))
        {
            TokenFunction name{.value = {character}};
            while (!trimmed.empty()
                   && (isAlpha(trimmed.front())
                       || digits.contains(trimmed.front())))
            {
                name.value += trimmed.front();
                trimmed.pop_front();
            };
            emitted = name;
        }
        else if (digits.contains(character) || character == decimal)
        {
            TokenNumber number{.value = {character}};
            bool fractional{character == decimal};
            while (!trimmed.empty()
                   && (digits.contains(trimmed.front())
                       || (trimmed.front() == decimal && !fractional)))
            {
                number.value += trimmed.front();
                fractional |= trimmed.front() == decimal;
                trimmed.pop_front();
            };
            emitted = number;
        }

        if (!emitted.has_value())
        {
            return std::nullopt;
        }

        tokens.push_back(emitted.value());
    }

    return tokens;
}
