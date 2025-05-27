#include "mathinterpreter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <QtLogging>

#include "backend/functions.h"
#include "backend/number.h"
#include <expected>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

/*
 * When testing GMP, one must be mindful of floating point precision. A double
 * literal like '1.1' will round to the nearest representable value, which is
 * something like '1.100000...00088...'. So constructing an arbitrary float like
 * Scalar{1.1} will not give the desired value. Instead, use the string overload
 * Scalar{"1.1"} for much more precision (depending on how the backend is
 * configured).
 *
 * These tests are usually written in a way such that the expected and actual
 * values are identical, instead of using relative error.
 */

class TestMathInterpreter : public QObject
{
    Q_OBJECT
private slots:
    static void test();
};

template <> auto QTest::toString(calqmath::Statement const& statement) -> char*
{
    QString const output = QString::fromStdString(statement.string());

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <> auto QTest::toString(calqmath::Scalar const& number) -> char*
{
    QString const output = QString::fromStdString(number.toString());

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <>
auto QTest::toString(
    std::expected<calqmath::Scalar, calqmath::InterpretError> const& result
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
auto QTest::toString(std::optional<calqmath::Statement> const& statement)
    -> char*
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
void testParse(calqmath::Interpreter const& interpreter)
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

void testParentheses(calqmath::Interpreter const& interpreter)
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

    std::vector<std::tuple<std::string, std::string>> const interpretTestCases{
        {"(1.1)", "1.1"},
        {"((1.1))", "1.1"},
        {"(((1.1)))", "1.1"},

        {"1.0 + (2.0)", "3.0"},
        {"(1.0) + 2.0", "3.0"},
        {"3.0 * (2.0)", "6.0"},
        {"(3.0) * (2.0)", "6.0"},

        {"0.0 + (1.0 + (2.0 + (3.0 + (4.0 + (5.0)))))", "15.0"},
        {"((((((0.0) + 1.0) + 2.0) + 3.0) + 4.0) + 5.0)", "15.0"},

        {"2.0 * (3.0 + 4.0)", "14.0"},
    };
    for (auto const& [input, outputRepr] : interpretTestCases)
    {
        auto const statementResult{interpreter.parse(input)};
        QVERIFY(statementResult.has_value());
        auto const interpretResult{interpreter.interpret(input)};
        calqmath::Scalar const output{outputRepr};
        QCOMPARE(interpretResult, output);
    }
}

void testWhitespace(calqmath::Interpreter const& interpreter)
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

void testInterpret(calqmath::Interpreter const& interpreter)
{
    std::vector<std::tuple<std::string, calqmath::Scalar>> const
        successTestCases{
            {"5", calqmath::Scalar{"5.0"}},
            {"12345", calqmath::Scalar{"12345.0"}},
            {"0+0", calqmath::Scalar{"0.0"}},
            {"1+0", calqmath::Scalar{"1.0"}},
            {"0+2", calqmath::Scalar{"2.0"}},
            {"1/2", calqmath::Scalar{"0.5"}},
            {"1/3", calqmath::Scalar{"1.0"} / calqmath::Scalar{"3.0"}},
            {"1*2*3*4*5", calqmath::Scalar{"120.0"}},
        };

    for (auto const& [input, output] : successTestCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
    }

    std::vector<std::tuple<std::string, calqmath::InterpretError>> const
        failureTestCases{
            {"0+", calqmath::InterpretError::ParseError},
        };

    for (auto const& [input, output] : failureTestCases)
    {
        QCOMPARE(interpreter.interpret(input), std::unexpected(output));
    }
}

void testOrderOfOperators(calqmath::Interpreter const& interpreter)
{
    std::vector<std::tuple<std::string, calqmath::Scalar>> const
        PEMDASTestCases{
            {"1 * 2 + 3 / 4 - 5", calqmath::Scalar{"-2.25"}},
            {"1 - 2 * 3 + 4 / 5",
             calqmath::Scalar{"-5"}
                 + calqmath::Scalar{"4"} / calqmath::Scalar{"5"}},
            {"1 / 2 - 3 * 4 + 5", calqmath::Scalar{"-6.5"}},
            {"1 + 2 / 3 - 4 * 5",
             calqmath::Scalar{"1.0"}
                 + calqmath::Scalar{"2.0"} / calqmath::Scalar{"3.0"}
                 + calqmath::Scalar{"-20.0"}},
        };
    for (auto const& [input, output] : PEMDASTestCases)
    {
        auto const actual = interpreter.interpret(input);

        QCOMPARE(actual, output);
    }
}

void testFunctionParsing(calqmath::Interpreter const& interpreter)
{
    std::vector<std::string> const invalidTestCases{
        "id()", "id(id())", "0.0 + id()", "id() + 0.0"
    };
    for (auto const& input : invalidTestCases)
    {
        QVERIFY(!interpreter.parse(input).has_value());
    }

    std::vector<std::tuple<std::string, calqmath::Scalar>> const testCases{
        {"id(1)", calqmath::Scalar{"1.0"}},
        {"id(id(2))", calqmath::Scalar{"2.0"}},
        {"id(id(id(3)))", calqmath::Scalar{"3.0"}},
        {"id(1.0 + 3.0)", calqmath::Scalar{"4.0"}},
        {"id(1.0 + id(4.0))", calqmath::Scalar{"5.0"}},
        {"id(id(4.0)+id(2.0))", calqmath::Scalar{"6.0"}},
        {"4.0 + id(3.0)", calqmath::Scalar{"7.0"}},
        {"sqrt(2.0)", calqmath::Functions::sqrt(calqmath::Scalar{"2.0"})}
    };

    for (auto const& [input, output] : testCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
    }
}

void testScalarStringify()
{
    std::vector<std::tuple<std::string, std::string>> const signedCases{
        {"0.00123", "1.23e-3"},
        {"0.0123", "0.012_3"},
        {"0.123", "0.123"},
        {"1.23", "1.23"},
        {"12.3", "12.3"},
        {"123.0", "123"},
        {"1230.0", "1_230"},
        {"12300.0", "12_300"},
        {"123000.0", "123_000"},
        {"1230000.0", "1_230_000"},
        {"12300000.0", "1.23e7"},
        {"123000000.0", "1.23e8"},
        {"1230000000.0", "1.23e9"},
        {"12300000000.0", "1.23e10"},
        {"123000000000.0", "1.23e11"},

        {"0.1234567890123", "0.123_456_789"},
        {"1234567891234.5", "1.234_567_891e12"},
    };

    std::vector<std::tuple<std::string, std::string>> const testCases{
        {"0", "0"},
        {"0.0", "0"},
    };

    for (auto const& [input, output] : signedCases)
    {
        QCOMPARE(calqmath::Scalar{input}.toString(), output);
        QCOMPARE(calqmath::Scalar{"-" + input}.toString(), "-" + output);
    }
    for (auto const& [input, output] : testCases)
    {
        QCOMPARE(calqmath::Scalar{input}.toString(), output);
    }
}

void testMinimalPrecision(calqmath::Interpreter const& interpreter)
{
    for (size_t i = 0; i < calqmath::getBignumBackendPrecision(); i++)
    {
        std::string const input{std::format("1{0}+1-1{0}", std::string(i, '0'))
        };
        QCOMPARE(interpreter.interpret(input), calqmath::Scalar{"1"});
    }
}

} // namespace

void TestMathInterpreter::test()
{
    calqmath::Interpreter const interpreter{};

    testParse(interpreter);
    testWhitespace(interpreter);
    testParentheses(interpreter);
    testInterpret(interpreter);
    testOrderOfOperators(interpreter);
    testFunctionParsing(interpreter);
    testMinimalPrecision(interpreter);

    testScalarStringify();
}

QTEST_MAIN(TestMathInterpreter)
#include "mathtest.moc"
