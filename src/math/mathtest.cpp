#include "mathinterpreter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <QtLogging>

#include "mathstringify.h"
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

template <> auto QTest::toString(MathStatement const& statement) -> char*
{
    QString const output = QString::fromStdString(statement.string());

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <> auto QTest::toString(Scalar const& number) -> char*
{
    QString const output = QString::fromStdString(calqmath::toString(number));

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <>
auto QTest::toString(
    std::expected<Scalar, MathInterpretationError> const& result
) -> char*
{
    QString output{};

    if (result.has_value())
    {
        output += "expected: ";
        output += QTest::toString(result.value());
    }
    else
    {
        output += "unexpected: ";
        output += QTest::toString(result.error());
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
void testParse(MathInterpreter const& interpreter)
{
    std::vector<std::string> const invalidTestCases{
        "0..0+0", "0..+0", "0+0..0", "0+0..", ".",  "..", "+-*/", "0+",
        "+0",     "++",    "+",      "0-",    "-0", "--", "-",    "0*",
        "*0",     "**",    "*",      "0/",    "/0", "//", "/",
    };

    for (auto const& input : invalidTestCases)
    {
        QCOMPARE(interpreter.parse(input), std::nullopt);
    }

    std::vector<std::tuple<std::string, size_t>> const termCountCases{
        {"", 0}, {"1", 1}, {"123", 1}, {"1+2", 2}, {"123+456", 2}
    };
    for (auto const& [input, termCount] : termCountCases)
    {
        auto const statementResult{interpreter.parse(input)};
        QVERIFY(statementResult.has_value());

        auto const& statement{statementResult.value()};
        QCOMPARE(statement.length(), termCount);
        QVERIFY(statement.empty() == (termCount == 0));
    }
}

void testParentheses(MathInterpreter const& interpreter)
{
    std::vector<std::string> const invalidTestCases{
        "()",
        "(())",
        "((()))",

        "(",
        "(()",
        "())",
        ")",

        "0(",
        ")0",
        "0)",
        "0(",
        "0()",

        "0+(",
        "(+)",
        "(+0",
        "(+",

        "0.(",
        "0.0 + 0.0(",

        "(((((0.0) + 1.0) + 2.0) + 3.0) + 4.0) + 5.0)",
    };

    for (auto const& input : invalidTestCases)
    {
        QCOMPARE(interpreter.parse(input), std::nullopt);
    }

    std::vector<std::tuple<std::string, double>> const interpretTestCases{
        {"(1.1)", 1.1},
        {"((1.1))", 1.1},
        {"(((1.1)))", 1.1},

        {"1.0 + (2.0)", 3.0},
        {"(1.0) + 2.0", 3.0},
        {"3.0 * (2.0)", 6.0},
        {"(3.0) * (2.0)", 6.0},

        {"0.0 + (1.0 + (2.0 + (3.0 + (4.0 + (5.0)))))", 15.0},
        {"((((((0.0) + 1.0) + 2.0) + 3.0) + 4.0) + 5.0)", 15.0},

        {"2.0 * (3.0 + 4.0)", 14.0},
    };
    for (auto const& [input, output] : interpretTestCases)
    {
        auto const statementResult{interpreter.parse(input)};
        QVERIFY(statementResult.has_value());

        QCOMPARE(interpreter.interpret(input), output);
    }
}

void testWhitespace(MathInterpreter const& interpreter)
{
    std::vector<std::string> const whitespaceParseCases{
        " 0 - 1 + 2 / 3 * 4 ",
        "   0   -  1  +  2  /  3  *  4  ",
        "0-1  +2/3  *4",
        "0  -1+2  /3*4",
        "  0-1  +2/3*4",
        "0  -1+2/3*4  ",
        "\n0\n-\n1\n+\n2\n/\n3\n*\n4\n",
    };
    auto const exemplar{interpreter.parse("0-1+2/3*4")};
    for (auto const& input : whitespaceParseCases)
    {
        QCOMPARE(interpreter.parse(input), exemplar);
    }
}

void testInterpret(MathInterpreter const& interpreter)
{
    std::vector<std::tuple<std::string, Scalar>> const successTestCases{
        {"5", 5.0},
        {"12345", 12345.0},
        {"0+0", 0.0},
        {"1+0", 1.0},
        {"0+2", 2.0},
        {"1/2", 0.5},
        {"1/3", 1.0 / Scalar{3.0}},
        {"1*2*3*4*5", 1.0 * 2.0 * 3.0 * 4.0 * 5.0},
    };

    for (auto const& [input, output] : successTestCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
    }

    std::vector<std::tuple<std::string, MathInterpretationError>> const
        failureTestCases{
            {"0+", MathInterpretationError::ParseError},
        };

    for (auto const& [input, output] : failureTestCases)
    {
        QCOMPARE(interpreter.interpret(input), std::unexpected(output));
    }
}

void testOrderOfOperators(MathInterpreter const& interpreter)
{
    std::vector<std::tuple<std::string, Scalar>> const PEMDASTestCases{
        {"1 * 2 + 3 / 4 - 5", (1.0 * 2.0) + (3.0 / Scalar{4.0}) - 5.0},
        {"1 - 2 * 3 + 4 / 5", 1.0 - (2.0 * 3.0) + (4.0 / Scalar{5.0})},
        {"1 / 2 - 3 * 4 + 5", (1.0 / Scalar{2.0}) - (3.0 * 4.0) + 5.0},
        {"1 + 2 / 3 - 4 * 5", 1.0 + (2.0 / Scalar{3.0}) - (4.0 * 5.0)},
    };
    for (auto const& [input, output] : PEMDASTestCases)
    {
        auto const actual = interpreter.interpret(input);
        QCOMPARE(actual, output);
    }
}

void testFunctionParsing(MathInterpreter const& interpreter)
{
    std::vector<std::string> const invalidTestCases{
        "id()", "id(id())", "0.0 + id()", "id() + 0.0"
    };
    for (auto const& input : invalidTestCases)
    {
        QVERIFY(!interpreter.parse(input).has_value());
    }

    std::vector<std::tuple<std::string, Scalar>> const testCases{
        {"id(1)", 1.0},
        {"id(id(2))", 2.0},
        {"id(id(id(3)))", 3.0},
        {"id(1.0 + 3.0)", 4.0},
        {"id(1.0 + id(4.0))", 5.0},
        {"id(id(4.0)+id(2.0))", 6.0},
        {"4.0 + id(3.0)", 7.0},
        {"sqrt(2.0)", sqrt(Scalar{2.0})}
    };

    for (auto const& [input, output] : testCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
    }
}

void testScalarStringify()
{
    std::vector<std::tuple<Scalar, std::string>> const testCases{
        {Scalar{0.00123}, "1.23e-3"},
        {Scalar{0.0123}, "0.012_3"},
        {Scalar{0.123}, "0.123"},
        {Scalar{1.23}, "1.23"},
        {Scalar{12.3}, "12.3"},
        {Scalar{123.0}, "123"},
        {Scalar{1230.0}, "1_230"},
        {Scalar{12300.0}, "12_300"},
        {Scalar{123000.0}, "123_000"},
        {Scalar{1230000.0}, "1_230_000"},
        {Scalar{12300000.0}, "1.23e7"},
        {Scalar{123000000.0}, "1.23e8"},
        {Scalar{1230000000.0}, "1.23e9"},
        {Scalar{12300000000.0}, "1.23e10"},
        {Scalar{123000000000.0}, "1.23e11"},

        {Scalar{0.1234567890123}, "0.123_456_789"},
        {Scalar{1234567891234.5}, "1.234_567_891e12"},

        {Scalar{0}, "0"},
        {Scalar{0.0}, "0"},
    };

    for (auto const& [input, output] : testCases)
    {
        QCOMPARE(calqmath::toString(input), output);
    }
}

} // namespace

void TestMathInterpreter::test()
{
    MathInterpreter const interpreter{};

    testParse(interpreter);
    testWhitespace(interpreter);
    testParentheses(interpreter);
    testInterpret(interpreter);
    testOrderOfOperators(interpreter);
    testFunctionParsing(interpreter);

    testScalarStringify();
}

QTEST_MAIN(TestMathInterpreter)
#include "mathtest.moc"
