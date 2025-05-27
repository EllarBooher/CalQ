#include "mathinterpreter.h"
#include <QTest>

class TestMathInterpreter : public QObject
{
    Q_OBJECT
private slots:
    void test();
};

auto mathOperatorToString(MathOp const op) -> char const*
{
    switch (op)
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

    QByteArray const ba = output.toUtf8();

    return qstrdup(ba);
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

    QByteArray const ba = output.toUtf8();

    return qstrdup(ba);
}

void TestMathInterpreter::test()
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
    for (auto& [input, output] : parseTestCases)
    {
        QCOMPARE(MathInterpreter::parse(input), output);
    }

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
    for (auto& [input, output] : evaluateTestCases)
    {
        QCOMPARE(MathInterpreter::evaluate(input), output);
    }

    std::vector<std::tuple<std::vector<MathOp>, double>> const PEMDASTestCases{
        {{
             MathOp::Multiply,
             MathOp::Plus,
             MathOp::Divide,
             MathOp::Minus,
         },
         1.0 * 2.0 + 3.0 / 4.0 - 5.0},
        {{
             MathOp::Minus,
             MathOp::Multiply,
             MathOp::Plus,
             MathOp::Divide,
         },
         1.0 - 2.0 * 3.0 + 4.0 / 5.0},
        {{
             MathOp::Divide,
             MathOp::Minus,
             MathOp::Multiply,
             MathOp::Plus,
         },
         1.0 / 2.0 - 3.0 * 4.0 + 5.0},
        {{
             MathOp::Plus,
             MathOp::Divide,
             MathOp::Minus,
             MathOp::Multiply,
         },
         1.0 + 2.0 / 3.0 - 4.0 * 5.0},
    };
    for (auto& [ops, output] : PEMDASTestCases)
    {
        QCOMPARE(
            MathInterpreter::evaluate(MathStatement{
                .terms = {1.0, 2.0, 3.0, 4.0, 5.0}, .operators = ops
            }),
            output
        );
    }
}

QTEST_MAIN(TestMathInterpreter)
#include "mathtest.moc"
