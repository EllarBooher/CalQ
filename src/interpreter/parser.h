#pragma once

#include "lexer.h"
#include "mathstatement.h"
#include <span>

namespace calqmath
{
class Parser
{
public:
    static auto
    parse(FunctionDatabase const& functions, std::span<Token const> input)
        -> std::optional<Statement>;
};
} // namespace calqmath
