#include "parser.h"

#include <deque>
#include <stack>
#include <utility>
#include <variant>

template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

namespace
{
auto tokenToOperator(calqmath::TokenOperator const token) -> calqmath::BinaryOp
{
    switch (token)
    {
    case calqmath::TokenOperator::Plus:
        return calqmath::BinaryOp::Plus;
    case calqmath::TokenOperator::Minus:
        return calqmath::BinaryOp::Minus;
    case calqmath::TokenOperator::Multiply:
        return calqmath::BinaryOp::Multiply;
    case calqmath::TokenOperator::Divide:
        return calqmath::BinaryOp::Divide;
    }
    std::unreachable();
}

auto tokenIsOperator(calqmath::Token const& token) -> bool
{
    auto const* const pOperator = std::get_if<calqmath::TokenOperator>(&token);
    return pOperator != nullptr;
}
auto tokenIsMinus(calqmath::Token const& token) -> bool
{
    auto const* const pOperator = std::get_if<calqmath::TokenOperator>(&token);

    return pOperator != nullptr
        && (*pOperator) == calqmath::TokenOperator::Minus;
}

auto tokenIsFunction(calqmath::Token const& token) -> bool
{
    auto const* const pFunction = std::get_if<calqmath::TokenFunction>(&token);
    return pFunction != nullptr;
}

auto tokenIsOpenBracket(calqmath::Token const& token) -> bool
{
    auto const* const pOpenBracket =
        std::get_if<calqmath::TokenOpenBracket>(&token);
    return pOpenBracket != nullptr;
}

auto tokenIsClosedBracket(calqmath::Token const& token) -> bool
{
    auto const* const pClosedBracket =
        std::get_if<calqmath::TokenClosedBracket>(&token);
    return pClosedBracket != nullptr;
}

auto tokenIsNumber(calqmath::Token const& token) -> bool
{
    auto const* const pNumber = std::get_if<calqmath::TokenNumber>(&token);
    return pNumber != nullptr;
}
} // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto calqmath::Parser::parse(
    FunctionDatabase const& functions, std::span<Token const> const input
) -> std::optional<calqmath::Statement>
{
    std::optional<Statement> result{Statement{}};

    std::stack<Statement*> depthStack{{&result.value()}};

    std::deque<Token> tokens{input.begin(), input.end()};

    /*
     * This flag controls whether or not the next token is expected to initiate
     * a new term. This decides the valid set of tokens at any given step as we
     * process them.
     *
     * Tokens that create a new term are number literals, function
     * names, open parantheses, etc.
     *
     * Tokens that don't create a new term are
     * operators, closed parantheses, etc. They are usually followed immediately
     * by another term.
     *
     * An exception to this is the unary operator '-'. '-' is overloaded to be
     * subtraction and negation, and requires special handling.
     */
    bool expectNewTerm = true;

    while (!tokens.empty())
    {
        if (expectNewTerm)
        {
            bool const negate{::tokenIsMinus(tokens.front())};
            if (negate)
            {
                tokens.pop_front();
            }

            if (tokens.empty())
            {
                return std::nullopt;
            }

            if (::tokenIsFunction(tokens.front())
                || ::tokenIsOpenBracket(tokens.front()))
            {
                std::optional<std::string> functionName{};
                if (::tokenIsFunction(tokens.front()))
                {
                    TokenFunction const function{
                        std::get<TokenFunction>(tokens.front())
                    };
                    tokens.pop_front();

                    functionName = function.m_functionName;
                }

                if (tokens.empty() || !::tokenIsOpenBracket(tokens.front()))
                {
                    return std::nullopt;
                }
                tokens.pop_front();

                auto& newStatement = std::get<Statement>(
                    depthStack.top()->backTerm() = Statement{}
                );
                depthStack.push(&newStatement);

                newStatement.setNegate(negate);

                if (functionName.has_value())
                {
                    auto functionLookup{functions.lookup(functionName.value())};
                    if (!functionLookup.has_value())
                    {
                        return std::nullopt;
                    }

                    newStatement.setFunction(std::move(functionLookup).value());
                }

                expectNewTerm = true;
            }
            else if (::tokenIsNumber(tokens.front()))
            {
                TokenNumber const number{std::get<TokenNumber>(tokens.front())};
                tokens.pop_front();

                if (negate)
                {
                    depthStack.top()->backTerm() =
                        -Scalar{number.m_decimalRepresentation};
                }
                else
                {
                    depthStack.top()->backTerm() =
                        Scalar{number.m_decimalRepresentation};
                }

                expectNewTerm = false;
            }
            else
            {
                return std::nullopt;
            }

            continue;
        }

        if (tokens.empty())
        {
            return std::nullopt;
        }

        if (::tokenIsOperator(tokens.front()))
        {
            BinaryOp const mathOperator{
                ::tokenToOperator(std::get<TokenOperator>(tokens.front()))
            };
            tokens.pop_front();

            depthStack.top()->append(mathOperator);
            expectNewTerm = true;
        }
        else if (::tokenIsClosedBracket(tokens.front())
                 && depthStack.size() > 1)
        {
            tokens.pop_front();

            depthStack.pop();
            expectNewTerm = false;
        }
        else
        {
            return std::nullopt;
        }
    }

    if (expectNewTerm || depthStack.size() > 1)
    {
        return std::nullopt;
    }

    return result;
}
