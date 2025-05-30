#pragma once

#include "expression.h"
#include "lexer.h"
#include <span>

namespace calqmath
{
/**
 * A parser geared heavily towards the sort of input for a calculator, not a
 * general programming language.
 *
 * Converts a stream of mathematical tokens into an AST that can be evaluated to
 * result. A grammer is enforced, see calqmath::Interpreter for a specification
 * of the grammar.
 */
class Parser
{
public:
    static auto
    parse(FunctionDatabase const& functions, std::span<Token const> input)
        -> std::optional<Expression>;
};
} // namespace calqmath
