#include "mathinterpreter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

class TestMathInterpreter : public QObject
{
    Q_OBJECT
private slots:
    static void test();
};

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

template <> auto QTest::toString(MathStatement const& statement) -> char*
{
    QString output{};

    if (!statement.isValid() || statement.terms.empty())
    {
        output += "Invalid";
    }
    else
    {
        output += QString::number(statement.terms[0]);
        for (size_t i = 0; i < statement.operators.size(); i++)
        {
            output += ',';
            output += mathOperatorToString(statement.operators[i]);
            output += ',';
            output += QString::number(statement.terms[i + 1]);
        }
    }

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <>
auto QTest::toString(std::optional<MathStatement> const& statement) -> char*
{
    QString output{};

    if (statement.has_value())
    {
        output += "some: ";
        output += QTest::toString(statement.value());
    }
    else
    {
        output += "nullopt";
    }

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

namespace
{
void testParse()
{
    std::vector<std::tuple<std::string, std::optional<MathStatement>>> const
        parseTestCases{
            {"0+1",
             MathStatement{
                 .terms = {0.0, 1.0},
                 .operators = {MathOp::Plus},
             }},
            {"0+1-2*3/4",
             MathStatement{
                 .terms = {0.0, 1.0, 2.0, 3.0, 4.0},
                 .operators =
                     {MathOp::Plus,
                      MathOp::Minus,
                      MathOp::Multiply,
                      MathOp::Divide},
             }},
            {"0-1+2/3*4",
             MathStatement{
                 .terms = {0.0, 1.0, 2.0, 3.0, 4.0},
                 .operators =
                     {MathOp::Minus,
                      MathOp::Plus,
                      MathOp::Divide,
                      MathOp::Multiply},
             }},
            {"0.1234+3.4567",
             MathStatement{
                 .terms = {0.1234, 3.4567},
                 .operators = {MathOp::Plus},
             }},
            {"0.+0+0.0+0.0000000000000000000000000000000000000000000000",
             MathStatement{
                 .terms = {0.0, 0.0, 0.0, 0.0},
                 .operators = {MathOp::Plus, MathOp::Plus, MathOp::Plus},
             }},
            {"123456789.123456789+123456789.123456789",
             MathStatement{
                 .terms = {123456789.123456789, 123456789.123456789},
                 .operators = {MathOp::Plus},
             }},
            {"0..0+0", std::nullopt},
            {"0..+0", std::nullopt},
            {"0+0..0", std::nullopt},
            {"0+0..", std::nullopt},
            {".", std::nullopt},
            {"..", std::nullopt},
            {"+-*/", std::nullopt},
            {"0+", std::nullopt},
            {"+0", std::nullopt},
            {"++", std::nullopt},
            {"+", std::nullopt},
            {"0-", std::nullopt},
            {"-0", std::nullopt},
            {"--", std::nullopt},
            {"-", std::nullopt},
            {"0*", std::nullopt},
            {"*0", std::nullopt},
            {"**", std::nullopt},
            {"*", std::nullopt},
            {"0/", std::nullopt},
            {"/0", std::nullopt},
            {"//", std::nullopt},
            {"/", std::nullopt},
        };
    for (auto const& [input, output] : parseTestCases)
    {
        QCOMPARE(MathInterpreter::parse(input), output);
    }
}

void testWhitespace()
{
    std::vector<std::tuple<std::vector<std::string>, MathStatement>> const
        whitespaceParseCases{
            {{"0-1+2/3*4",
              " 0 - 1 + 2 / 3 * 4 ",
              "   0   -  1  +  2  /  3  *  4  ",
              "0-1  +2/3  *4",
              "0  -1+2  /3*4",
              "  0-1  +2/3*4",
              "0  -1+2/3*4  ",
              "\n0\n-\n1\n+\n2\n/\n3\n*\n4\n"},
             MathStatement{
                 .terms = {0.0, 1.0, 2.0, 3.0, 4.0},
                 .operators =
                     {MathOp::Minus,
                      MathOp::Plus,
                      MathOp::Divide,
                      MathOp::Multiply},
             }}
        };
    for (auto const& [inputs, output] : whitespaceParseCases)
    {
        for (auto const& input : inputs)
        {
            QCOMPARE(MathInterpreter::parse(input), output);
        }
    }
}

void testEvaluation()
{
    std::vector<std::tuple<MathStatement, std::optional<double>>> const
        evaluateTestCases{
            {MathStatement{.terms = {0.0}, .operators = {MathOp::Plus}},
             std::nullopt},
            {MathStatement{.terms = {0.0, 0.0}, .operators = {MathOp::Plus}},
             0.0},
            {MathStatement{.terms = {1.0, 0.0}, .operators = {MathOp::Plus}},
             1.0},
            {MathStatement{.terms = {0.0, 2.0}, .operators = {MathOp::Plus}},
             2.0},
            {MathStatement{.terms = {1.0, 2.0}, .operators = {MathOp::Divide}},
             0.5},
            {MathStatement{.terms = {1.0, 3.0}, .operators = {MathOp::Divide}},
             1.0 / 3.0},
            {MathStatement{
                 .terms = {1.0, 2.0, 3.0, 4.0, 5.0},
                 .operators =
                     {MathOp::Multiply,
                      MathOp::Multiply,
                      MathOp::Multiply,
                      MathOp::Multiply}
             },
             1.0 * 2.0 * 3.0 * 4.0 * 5.0},

        };
    for (auto const& [input, output] : evaluateTestCases)
    {
        QCOMPARE(MathInterpreter::evaluate(input), output);
    }
}

void testOrderOfOperators()
{
    std::vector<std::tuple<std::vector<MathOp>, double>> const PEMDASTestCases{
        {{
             MathOp::Multiply,
             MathOp::Plus,
             MathOp::Divide,
             MathOp::Minus,
         },
         (1.0 * 2.0) + (3.0 / 4.0) - 5.0},
        {{
             MathOp::Minus,
             MathOp::Multiply,
             MathOp::Plus,
             MathOp::Divide,
         },
         1.0 - (2.0 * 3.0) + (4.0 / 5.0)},
        {{
             MathOp::Divide,
             MathOp::Minus,
             MathOp::Multiply,
             MathOp::Plus,
         },
         (1.0 / 2.0) - (3.0 * 4.0) + 5.0},
        {{
             MathOp::Plus,
             MathOp::Divide,
             MathOp::Minus,
             MathOp::Multiply,
         },
         1.0 + (2.0 / 3.0) - (4.0 * 5.0)},
    };
    for (auto const& [ops, output] : PEMDASTestCases)
    {
        MathStatement const testStatement{
            .terms = {1.0, 2.0, 3.0, 4.0, 5.0}, .operators = ops
        };
        QCOMPARE(MathInterpreter::evaluate(testStatement), output);
    }
}

} // namespace

void TestMathInterpreter::test()
{
    testParse();
    testWhitespace();
    testEvaluation();
    testOrderOfOperators();
}

QTEST_MAIN(TestMathInterpreter)
#include "mathtest.moc"
