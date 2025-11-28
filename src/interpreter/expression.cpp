#include "expression.h"

#include "function_database.h"
#include <cassert>
#include <cctype>
#include <cstddef>
#include <deque>
#include <expected>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Helper for std::visit
// See example at https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

namespace calqmath
{
auto Expression::operator=(Expression const& other) -> Expression&
{
    m_terms.clear();

    for (auto const& term : other.m_terms)
    {
        m_terms.push_back(std::make_unique<Term>(*term));
    }
    m_operators = other.m_operators;
    m_function = other.m_function;

    return *this;
}

Expression::Expression(Expression const& other) { *this = other; }

auto Expression::operator=(Expression&& other) noexcept -> Expression&
{
    m_terms = std::move(other).m_terms;
    m_operators = std::move(other).m_operators;
    m_function = std::move(other).m_function;

    return *this;
}

Expression::Expression(Expression&& other) noexcept { *this = std::move(other); }

auto Expression::operator==(Expression const& rhs) const -> bool
{
    if (termCount() != rhs.termCount())
    {
        return false;
    }

    if (m_operators != rhs.m_operators)
    {
        return false;
    }

    for (size_t index = 0; index < termCount(); index++)
    {
        auto const& lhsTerm{m_terms[index]};
        auto const& rhsTerm{rhs.m_terms[index]};
        if (lhsTerm == nullptr && rhsTerm == nullptr)
        {
            continue;
        }
        if (lhsTerm == nullptr || rhsTerm == nullptr)
        {
            return false;
        }

        if (*lhsTerm != *rhsTerm)
        {
            return false;
        }
    }

    return m_function == rhs.m_function;
}

namespace
{
auto mathOperatorToString(BinaryOp const binaryOp) -> char const*
{
    switch (binaryOp)
    {
    case BinaryOp::Plus:
        return "+";
    case BinaryOp::Minus:
        return "-";
    case BinaryOp::Multiply:
        return "*";
    case BinaryOp::Divide:
        return "/";
    }

    return "?";
}
} // namespace

auto Expression::string() const -> std::string
{
    if (!valid())
    {
        return "Invalid";
    }

    if (empty())
    {
        return "Empty";
    }

    std::string output{};

    output += stringTerm(0);
    for (size_t i = 0; i < m_operators.size(); i++)
    {
        output += ',';
        output += mathOperatorToString(m_operators[i]);
        output += ',';
        output += stringTerm(i + 1);
    }

    if (m_function != nullptr)
    {
        assert(m_function->function != nullptr);

        return m_function->name + "(" + output + ")";
    }

    return output;
}

auto Expression::evaluate(Scalar const& variable) const -> std::optional<Scalar>
{
    if (!valid())
    {
        return std::nullopt;
    }

    if (empty())
    {
        return Scalar{"0.0"};
    }

    std::deque<Scalar> terms{};
    for (size_t termIndex = 0; termIndex < m_terms.size(); termIndex++)
    {
        auto const evaluateResult = evaluateTerm(termIndex, variable);
        if (!evaluateResult.has_value())
        {
            return std::nullopt;
        }
        terms.push_back(evaluateResult.value());
    }

    std::deque<BinaryOp> operators{m_operators.begin(), m_operators.end()};

    // Reduce while evaluating operators for adjacent terms.
    // Multiplication and division first
    size_t index = 0;
    while (index < operators.size())
    {
        BinaryOp const mathOperator = operators[index];
        if (mathOperator == BinaryOp::Plus || mathOperator == BinaryOp::Minus)
        {
            index++;
            continue;
        }
        assert(
            mathOperator == BinaryOp::Multiply
            || mathOperator == BinaryOp::Divide
        );

        Scalar const firstTerm = terms[index];
        Scalar const secondTerm = terms[index + 1];

        terms.erase(terms.begin() + index);
        operators.erase(operators.begin() + index);

        if (mathOperator == BinaryOp::Multiply)
        {
            terms.at(index) = firstTerm * secondTerm;
        }
        else
        {
            terms.at(index) = firstTerm / secondTerm;
        }
    }

    // Addition and subtraction next
    while (!operators.empty())
    {
        BinaryOp const mathOperator = operators[0];
        assert(
            mathOperator == BinaryOp::Plus || mathOperator == BinaryOp::Minus
        );

        Scalar const firstTerm = terms[0];
        Scalar const secondTerm = terms[1];

        terms.pop_front();
        operators.pop_front();

        if (mathOperator == BinaryOp::Plus)
        {
            terms[0] = firstTerm + secondTerm;
        }
        else
        {
            terms[0] = firstTerm - secondTerm;
        }
    }

    assert(terms.size() == 1);
    Scalar result = std::move(terms)[0];

    // Potentially lots of function overhead here
    if (m_function != nullptr)
    {
        assert(m_function->function != nullptr);

        result = m_function->function(result);
    }

    if (m_negate)
    {
        result = -result;
    }

    return result;
}

auto Expression::termCount() const -> size_t { return m_terms.size(); }

void Expression::reset(Term&& initial)
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<Term>(std::move(initial)));
}

auto Expression::reset(Expression&& initial) -> Expression&
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<Term>(std::move(initial)));

    return std::get<Expression>(*m_terms.back());
}

void Expression::setNegate(bool negate) { m_negate = negate; }

void Expression::setFunction(std::shared_ptr<UnaryFunction const> functionID)
{
    m_function = std::move(functionID);
}

auto Expression::backTerm() -> Term&
{
    if (empty())
    {
        m_terms.push_back(std::make_unique<Term>());
    }

    assert(m_terms.size() == m_operators.size() + 1);

    return *m_terms.back();
}

auto Expression::append(BinaryOp mathOp) -> Term&
{
    m_terms.push_back(std::make_unique<Term>(Scalar{"0.0"}));
    m_operators.push_back(mathOp);

    return *m_terms.back();
}

auto Expression::appendExpression(BinaryOp mathOp) -> Expression&
{
    auto& term = append(mathOp);
    term = Expression();
    return std::get<Expression>(term);
}

auto Expression::stringTerm(size_t index) const -> std::string
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](Scalar const& number) { return number.toString(); },
        [&](Expression const& expression)
    { return "(" + expression.string() + ")"; },
        [](InputVariable const&)
    { return std::string{InputVariable::RESERVED_NAME}; }
    };

    return std::visit(visitor, *m_terms[index]);
}

auto Expression::evaluateTerm(size_t index, Scalar const& variable) const
    -> std::optional<Scalar>
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](Scalar const& number) { return std::optional{number}; },
        [&](Expression const& expression)
    { return expression.evaluate(variable); },
        [&](InputVariable const&) { return std::optional{variable}; }
    };

    return std::visit(visitor, *m_terms[index]);
}

auto Expression::valid() const -> bool
{
    bool const completelyEmpty = m_terms.empty() && m_operators.empty();
    bool const oneOpBetweenAllConsecutiveTerms =
        m_operators.size() == m_terms.size() - 1;
    bool const noNullChildren =
        std::count(m_terms.begin(), m_terms.end(), nullptr) == 0;

    return (
        (completelyEmpty || oneOpBetweenAllConsecutiveTerms) && noNullChildren
    );
}

auto Expression::empty() const -> bool { return termCount() == 0; }

} // namespace calqmath
