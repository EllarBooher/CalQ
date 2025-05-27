#pragma once

#include "mathstatement.h"
#include <cstdint>
#include <expected>
#include <optional>
#include <stack>
#include <string>
#include <variant>

namespace calqmath
{
class StatementParser
{
public:
    explicit StatementParser(std::string const& rawInput);

    auto execute(FunctionDatabase const& functions) -> std::optional<Statement>;

private:
    enum class IncrementResult : uint8_t
    {
        Continue,
        Finished,
        Error,
    };

    struct ParseStateOpened
    {
    };
    struct ParseStateClosed
    {
    };
    struct ParseStateOperator
    {
        BinaryOp mathOp;
    };
    struct ParseStateNumber
    {
        size_t numberStartIndex{0};
        bool afterDecimal{false};
        std::optional<BinaryOp> mathOp;
    };

    using ParseState = std::variant<
        ParseStateOpened,
        ParseStateClosed,
        ParseStateNumber,
        ParseStateOperator>;

    [[nodiscard]] auto characterAt(size_t index) const -> std::optional<char>;

    /**
     * Starting at the current m_index, grabs all contiguous alphabetic
     * characters and returns the name of the function. So if the stream is "...
     * function(...) ..." with the m_index at f, then "function" will be
     * returned. If non-alphabetic characters are encountered before the
     * paranthesis is encountered, null is returned.
     *
     * This method mutates the m_index, and moves it to the open paranthesis if
     * successful.
     * If null is returned, m_index is not mutated.
     *
     * @brief seekToEndOfFunctionName
     * @return
     */
    [[nodiscard]] auto seekToEndOfFunctionName() -> std::optional<std::string>;

    /**
     * Increments the index into the string we are parsing, expanding the
     * resulting MathStatement with new terms as they are parsed.
     *
     * @brief increment - Increments parsing by a step.
     * @return Returns the result of the increment operation, including if an
     * error occured or if parsing is finished.
     */
    // NOLINTNEXTLINE
    auto increment(FunctionDatabase const& functions) -> IncrementResult;

    /**
     * There is usually dangling state to clean-up, such as when equations end
     * in a digit. This method finishes parsing all that, and returns the final
     * Statement if valid.
     *
     * @brief finish - Returns the final result of parsing.
     * @return Returns whether or not the result is valid.
     */
    auto finish() -> std::optional<Statement>;

    std::string const m_trimmed;

    /**
     * We store the root-level statement as a value that gets default
     * destructed. statementDepthStack[0] contains a pointer to m_rootStatement,
     * and should generally be modified there.
     */
    Statement m_rootStatement;

    /**
     * A Statement is a tree-like structure, where individual terms can be
     * statements. As we build statements and add terms, we store a stack of the
     * path to the current deepest statement we are building.
     *
     * The stack should always have at least one element: the root statement at
     * index 0. Keeping this pointer to rootStatement simplifies
     * some of the access logic.
     */
    std::stack<Statement*> m_statementDepthStack;

    ParseState m_state{ParseStateOpened{}};
    size_t m_index{0};
};
} // namespace calqmath
