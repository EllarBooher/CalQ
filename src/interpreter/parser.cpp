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
     */
    bool expectNewTerm = true;

    bool constexpr SUCCESS = true;
    bool constexpr FAILURE = false;

    auto const visitors{overloads{
        [&](TokenOperator const& token)
    {
        if (expectNewTerm && !tokens.empty())
        {
            // Unary negative sign e.g. make a number negative
            if (token == TokenOperator::Minus
                && std::holds_alternative<TokenNumber>(tokens.front()))
            {
                TokenNumber const number =
                    std::get<TokenNumber>(tokens.front());
                tokens.pop_front();

                depthStack.top()->backTerm() =
                    -Scalar{number.m_decimalRepresentation};
                expectNewTerm = false;
                return SUCCESS;
            }

            return FAILURE;
        }

        BinaryOp const mathOperator{::tokenToOperator(token)};

        depthStack.top()->append(mathOperator);
        expectNewTerm = true;
        return SUCCESS;
    },
        [&](TokenClosedBracket const&)
    {
        // Root level statement has no parantheses, so it cannot ever be popped.
        if (expectNewTerm || depthStack.size() < 2)
        {
            return FAILURE;
        }

        depthStack.pop();
        expectNewTerm = false;
        return SUCCESS;
    },
        [&](TokenNumber const& number)
    {
        if (!expectNewTerm)
        {
            return FAILURE;
        }

        depthStack.top()->backTerm() = Scalar{number.m_decimalRepresentation};
        expectNewTerm = false;
        return SUCCESS;
    },
        [&](TokenFunction const& function)
    {
        if (!expectNewTerm)
        {
            return FAILURE;
        }

        if (tokens.empty()
            || !std::holds_alternative<TokenOpenBracket>(tokens.front()))
        {
            return FAILURE;
        }
        tokens.pop_front();

        Term& backTerm = depthStack.top()->backTerm();
        backTerm = Statement{};

        auto functionLookup{functions.lookup(function.m_functionName)};
        if (!functionLookup.has_value())
        {
            return FAILURE;
        }

        std::get<Statement>(backTerm).setFunction(
            std::move(functionLookup).value()
        );
        depthStack.push(&std::get<Statement>(backTerm));
        expectNewTerm = true;
        return SUCCESS;
    },
        [&](TokenOpenBracket const&)
    {
        if (!expectNewTerm)
        {
            return FAILURE;
        }

        Term& backTerm = depthStack.top()->backTerm();
        backTerm = Statement{};

        depthStack.push(&std::get<Statement>(backTerm));
        expectNewTerm = true;
        return SUCCESS;
    }
    }};

    while (!tokens.empty())
    {
        Token const token{tokens.front()};
        tokens.pop_front();
        if (std::visit(visitors, token) == FAILURE)
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
