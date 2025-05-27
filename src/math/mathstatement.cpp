#include "mathstatement.h"

#include "mathfunction.h"
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

namespace
{

} // namespace

auto MathStatement::operator=(MathStatement const& other) -> MathStatement&
{
    m_terms.clear();

    for (auto const& term : other.m_terms)
    {
        m_terms.push_back(std::make_unique<MathTerm>(*term));
    }
    m_operators = other.m_operators;

    return *this;
}

MathStatement::MathStatement(MathStatement const& other) { *this = other; }

auto MathStatement::operator=(MathStatement&& other) noexcept -> MathStatement&
{
    m_terms = std::move(other).m_terms;
    m_operators = std::move(other).m_operators;

    return *this;
}

MathStatement::MathStatement(MathStatement&& other) noexcept
{
    *this = std::move(other);
}

auto MathStatement::operator==(MathStatement const& rhs) const -> bool
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
auto mathOperatorToString(MathOp const mathOp) -> char const*
{
    switch (mathOp)
    {
    case MathOp::Plus:
        return "+";
    case MathOp::Minus:
        return "-";
    case MathOp::Multiply:
        return "*";
    case MathOp::Divide:
        return "/";
    }

    return "?";
}
} // namespace

auto MathStatement::string() const -> std::string
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

auto MathStatement::evaluate() const -> std::optional<double>
{

    if (!valid())
    {
        return std::nullopt;
    }

    if (empty())
    {
        return 0.0;
    }

    std::deque<double> terms{};
    for (size_t termIndex = 0; termIndex < m_terms.size(); termIndex++)
    {
        auto const evaluateResult = evaluateTerm(termIndex);
        if (!evaluateResult.has_value())
        {
            return std::nullopt;
        }
        terms.push_back(evaluateResult.value());
    }

    if (terms.size() == 1)
    {
        return terms[0];
    }

    std::deque<MathOp> operators{m_operators.begin(), m_operators.end()};

    // Reduce while evaluating operators for adjacent terms.
    // Multiplication and division first
    size_t index = 0;
    while (index < operators.size())
    {
        MathOp const mathOperator = operators[index];
        if (mathOperator == MathOp::Plus || mathOperator == MathOp::Minus)
        {
            index++;
            continue;
        }
        assert(
            mathOperator == MathOp::Multiply || mathOperator == MathOp::Divide
        );

        double const firstTerm = terms[index];
        double const secondTerm = terms[index + 1];

        terms.erase(terms.begin() + index);
        operators.erase(operators.begin() + index);

        if (mathOperator == MathOp::Multiply)
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
        MathOp const mathOperator = operators[0];
        assert(mathOperator == MathOp::Plus || mathOperator == MathOp::Minus);

        double const firstTerm = terms[0];
        double const secondTerm = terms[1];

        terms.pop_front();
        operators.pop_front();
        index -= 1;

        if (mathOperator == MathOp::Plus)
        {
            terms[0] = firstTerm + secondTerm;
        }
        else
        {
            terms[0] = firstTerm - secondTerm;
        }
    }

    assert(terms.size() == 1);
    double const result = terms[0];

    if (m_function.has_value())
    {
        assert(m_function.value() != nullptr);
        return m_function.value()(result);
    }

    return result;
}

auto MathStatement::length() const -> size_t { return m_terms.size(); }

void MathStatement::reset(MathTerm&& initial)
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<MathTerm>(std::move(initial)));
}

auto MathStatement::reset(MathStatement&& initial) -> MathStatement&
{
    m_terms.clear();
    m_operators.clear();

    m_terms.push_back(std::make_unique<MathTerm>(std::move(initial)));

    return std::get<MathStatement>(*m_terms.back());
}

void MathStatement::setFunction(MathUnaryFunction&& function)
{
    m_function = std::move(function);
}

auto MathStatement::append(MathOp mathOp) -> MathTerm&
{
    m_terms.push_back(std::make_unique<MathTerm>(0.0));
    m_operators.push_back(mathOp);

    return *m_terms.back();
}

auto MathStatement::appendStatement(MathOp mathOp) -> MathStatement&
{
    auto& term = append(mathOp);
    term = MathStatement();
    return std::get<MathStatement>(term);
}

auto MathStatement::stringTerm(size_t index) const -> std::string
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](double const& number) { return std::to_string(number); },
        [](MathStatement const& statement)
    { return "(" + statement.string() + ")"; },
    };

    return std::visit(visitor, *m_terms[index]);
}

auto MathStatement::evaluateTerm(size_t index) const -> std::optional<double>
{
    assert(index < m_terms.size() || m_terms[index] != nullptr);

    auto const visitor = overloads{
        [](double const& number) { return std::optional{number}; },
        [](MathStatement const& statement) { return statement.evaluate(); }
    };

    return std::visit(visitor, *m_terms[index]);
}
