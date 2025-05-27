#include "mathinterpreter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>

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
    // These cases are lacking and should be expanded
    std::vector<std::tuple<std::string, double>> const successTestCases{
        {"5", 5.0},
        {"12345", 12345.0},
        {"0+0", 0.0},
        {"1+0", 1.0},
        {"0+2", 2.0},
        {"1/2", 0.5},
        {"1/3", 1.0 / 3.0},
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
    std::vector<std::tuple<std::string, double>> const PEMDASTestCases{
        {"1 * 2 + 3 / 4 - 5", (1.0 * 2.0) + (3.0 / 4.0) - 5.0},
        {"1 - 2 * 3 + 4 / 5", 1.0 - (2.0 * 3.0) + (4.0 / 5.0)},
        {"1 / 2 - 3 * 4 + 5", (1.0 / 2.0) - (3.0 * 4.0) + 5.0},
        {"1 + 2 / 3 - 4 * 5", 1.0 + (2.0 / 3.0) - (4.0 * 5.0)},
    };
    for (auto const& [input, output] : PEMDASTestCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
    }
}

void testFunctions(MathInterpreter const& interpreter)
{
    std::vector<std::string> const invalidTestCases{
        "id()", "id(id())", "0.0 + id()", "id() + 0.0"
    };
    for (auto const& input : invalidTestCases)
    {
        QVERIFY(!interpreter.parse(input).has_value());
    }

    std::vector<std::tuple<std::string, double>> const testCases{
        {"id(1)", 1.0},
        {"id(id(2))", 2.0},
        {"id(id(id(3)))", 3.0},
        {"id(1.0 + 3.0)", 4.0},
        {"id(1.0 + id(4.0))", 5.0},
        {"id(id(4.0)+id(2.0))", 6.0},
        {"4.0 + id(3.0)", 7.0},
    };

    for (auto const& [input, output] : testCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
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
    testFunctions(interpreter);
}

QTEST_MAIN(TestMathInterpreter)
#include "mathtest.moc"
