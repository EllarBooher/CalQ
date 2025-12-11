#pragma once

#include "types.h"
#include <map>
#include <optional>
#include <ranges>
#include <string>

namespace calqmath
{
/**
 * @brief The FunctionDatabase class stores loaded functions for easy lookup by
 * the interpreter.
 */
class FunctionDatabase
{
public:
    // Creates the database loading every function possible, for normal use.
    static auto createWithDefaults() -> FunctionDatabase;

    /**
     * @brief lookup - Potentially many-to-one lookup by a string.
     *
     * For example, the string "sin" will return the trigonometric sine
     * function alongside its canonical name.
     *
     * @return Returns the function, or null if no function by that name exists.
     */
    [[nodiscard]] auto lookup(std::string const&) const
        -> std::optional<std::shared_ptr<UnaryFunction const>>;

    [[nodiscard]] auto unaryNames() const
    {
        return std::views::values(m_unaryFunctions);
    }

private:
    FunctionDatabase();

    std::map<std::string, std::shared_ptr<UnaryFunction const>>
        m_unaryFunctions;
};
} // namespace calqmath
