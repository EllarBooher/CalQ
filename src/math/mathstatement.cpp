#include "mathstatement.h"

#include "mathfunction.h"
#include "mathstringify.h"
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
auto Statement::operator=(Statement const& other) -> Statement&
{
    m_terms.clear();

    for (auto const& term : other.m_terms)
    {
        m_terms.push_back(std::make_unique<Term>(*term));
    }
    m_operators = other.m_operators;

    return *this;
}

Statement::Statement(Statement const& other) { *this = other; }

auto Statement::operator=(Statement&& other) noexcept -> Statement&
{
    m_terms = std::move(other).m_terms;
    m_operators = std::move(other).m_operators;

    return *this;
}

Statement::Statement(Statement&& other) noexcept { *this = std::move(other); }

auto Statement::operator==(Statement const& rhs) const -> bool
{
    if (length() != rhs.length())
    {
        return false;
    }

    if (m_operators != rhs.m_operators)
    {
        return false;
    }

    for (size_t index = 0; index < length(); index++)
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

    return true;
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

auto Statement::string() const -> std::string
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

    return output;
}

auto Statement::evaluate() const -> std::optional<Scalar>
{

    if (!valid())
    {
        return std::nullopt;
    }

    if (empty())
    {
        return 0.0;
    }

    std::deque<Scalar> terms{};
    for (size_t termIndex = 0; termIndex < m_terms.size(); termIndex++)
    {
        auto const evaluateResult = evaluateTerm(termIndex);
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
        index -= 1;

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

    if (m_function.has_value())
    {
        assert(m_function.value() != nullptr);
        return m_function.value()(result);
    }

    return result;
}

auto Statement::length() const -> size_t { return m_terms.size(); }

void Statement::reset(Term&& initial)
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<Term>(std::move(initial)));
}

auto Statement::reset(Statement&& initial) -> Statement&
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<Term>(std::move(initial)));

    return std::get<Statement>(*m_terms.back());
}

void Statement::setFunction(UnaryFunction&& function)
{
    m_function = std::move(function);
}

auto Statement::append(BinaryOp mathOp) -> Term&
{
    m_terms.push_back(std::make_unique<Term>(0.0));
    m_operators.push_back(mathOp);

    return *m_terms.back();
}

auto Statement::appendStatement(BinaryOp mathOp) -> Statement&
{
    auto& term = append(mathOp);
    term = Statement();
    return std::get<Statement>(term);
}

auto Statement::stringTerm(size_t index) const -> std::string
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](Scalar const& number) { return calqmath::toString(number); },
        [](Statement const& statement)
    { return "(" + statement.string() + ")"; },
    };

    return std::visit(visitor, *m_terms[index]);
}

auto Statement::evaluateTerm(size_t index) const -> std::optional<Scalar>
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](Scalar const& number) { return std::optional{number}; },
        [](Statement const& statement) { return statement.evaluate(); }
    };

    return std::visit(visitor, *m_terms[index]);
}
} // namespace calqmath
