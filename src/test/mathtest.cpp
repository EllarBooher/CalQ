#include "interpreter/lexer.h"
#include "interpreter/interpreter.h"
#include "interpreter/parser.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <QtLogging>

#include "math/number.h"
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

template <> auto QTest::toString(calqmath::Expression const& expression) -> char*
{
    QString const output = QString::fromStdString(expression.string());

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <> auto QTest::toString(calqmath::Scalar const& number) -> char*
{
    QString const output = QString::fromStdString(number.toString());

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <> auto QTest::toString(calqmath::InterpretError const& error) -> char*
{
    switch (error)
    {
    case calqmath::InterpretError::ParseError:
        return qstrdup("Parse Error");
    case calqmath::InterpretError::EvaluationError:
        return qstrdup("Evaluation Error");
    default:
        return qstrdup("Unknown Error");
    }
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

template <class... Ts> struct overloads : Ts...
{
    using Ts::operator()...;
};

template <>
auto QTest::toString(std::vector<calqmath::Token> const& tokens) -> char*
{
    QString output{};

    for (size_t i = 0; i < tokens.size(); i++)
    {
        auto const visitors{overloads{
            [&](calqmath::TokenIdentifier const& name)
        {
            output += "f'";
            output += name.m_functionName;
        },
            [&](calqmath::TokenNumber const& name)
        {
            output += "n'";
            output += name.m_decimalRepresentation;
        },
            [&](calqmath::TokenOpenBracket const&) { output += "("; },
            [&](calqmath::TokenClosedBracket const&) { output += ")"; },
            [&](calqmath::TokenOperator const& mathOperator)
        {
            output += "o'";
            switch (mathOperator)
            {
            case calqmath::TokenOperator::Plus:
                output += "+";
                break;
            case calqmath::TokenOperator::Minus:
                output += "-";
                break;
            case calqmath::TokenOperator::Multiply:
                output += "*";
                break;
            case calqmath::TokenOperator::Divide:
                output += "/";
                break;
            }
        }
        }};

        std::visit(visitors, tokens[i]);
        if (i < tokens.size() - 1)
        {
            output += ",";
        }
    }

    QByteArray const bytes = output.toUtf8();
    return qstrdup(bytes);
}

template <>
auto QTest::toString(std::optional<std::vector<calqmath::Token>> const& result)
    -> char*
{
    QString output{};

    if (result.has_value())
    {
        output += "some: ";
        output += QTest::toString(result.value());
    }
    else
    {
        output += "nullopt";
    }

    QByteArray const bytes = output.toUtf8();
    return qstrdup(bytes);
}

template <>
auto QTest::toString(std::optional<calqmath::Expression> const& expression)
    -> char*
{
    QString output{};

    if (expression.has_value())
    {
        output += "some: ";
        output += QTest::toString(expression.value());
    }
    else
    {
        output += "nullopt";
    }

    QByteArray const bytes = output.toUtf8();

    return qstrdup(bytes);
}

template <>
auto QTest::toString(std::optional<calqmath::Scalar> const& scalar) -> char*
{
    QString output{};

    if (scalar.has_value())
    {
        output += "some: ";
        output += QTest::toString(scalar.value());
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
    std::vector<std::tuple<std::string, calqmath::Scalar>> const testCases{
        {"id(1)", calqmath::Scalar{"1.0"}},
        {"id(id(2))", calqmath::Scalar{"2.0"}},
        {"id(id(id(3)))", calqmath::Scalar{"3.0"}},
        {"id(1.0 + 3.0)", calqmath::Scalar{"4.0"}},
        {"id(1.0 + id(4.0))", calqmath::Scalar{"5.0"}},
        {"id(id(4.0)+id(2.0))", calqmath::Scalar{"6.0"}},
        {"4.0 + id(3.0)", calqmath::Scalar{"7.0"}},
    };

    for (auto const& [input, output] : testCases)
    {
        QCOMPARE(interpreter.interpret(input), output);
    }
}

void testAllFunctions(
    calqmath::FunctionDatabase const& functions,
    calqmath::Interpreter const& interpreter
)
{
    for (auto const& function : functions.unaryNames())
    {
        QVERIFY(function != nullptr);
        QVERIFY(interpreter.interpret(function->name + "(1.0)").has_value());
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

void testScalarOperators()
{
    calqmath::Scalar const minusOne{"-1"};
    calqmath::Scalar const oneHalf{"0.5"};
    calqmath::Scalar const one{"1"};
    calqmath::Scalar const two{"2"};

    QCOMPARE(one + one, two);
    QCOMPARE(-one, minusOne);

    QCOMPARE(two - one, one);
    QCOMPARE(one - two, minusOne);

    QCOMPARE(one * two, two);
    QCOMPARE(two * one, two);

    QCOMPARE(two / one, two);
    QCOMPARE(one / two, oneHalf);
}

void testNonOrdinaryScalarStringify()
{
    QCOMPARE(
        calqmath::Scalar::zero().toString(),
        calqmath::Scalar::ZERO_REPRESENTATION
    );

    QCOMPARE(
        calqmath::Scalar::nan().toString(), calqmath::Scalar::NAN_REPRESENTATION
    );

    QCOMPARE(
        calqmath::Scalar::positiveInf().toString(),
        calqmath::Scalar::POSITIVE_INFINITY_REPRESENTATION
    );

    QCOMPARE(
        calqmath::Scalar::negativeInf().toString(),
        calqmath::Scalar::NEGATIVE_INFINITY_REPRESENTATION
    );
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

void testLexerWhitespace()
{
    using TestCase =
        std::tuple<std::vector<std::string>, std::vector<calqmath::Token>>;
    std::vector<TestCase> const cases{
        {{" 0 - 1 + 2 / 3 * 4 ",
          "   0   -  1  +  2  /  3  *  4  ",
          "0-1  +2/3  *4",
          "0  -1+2  /3*4",
          "  0-1  +2/3*4",
          "0  -1+2/3*4  "},
         {calqmath::Lexer::convert("0-1+2/3*4").value()}}
    };
    for (auto const& [inputs, output] : cases)
    {
        for (auto const& input : inputs)
        {
            auto const actual = calqmath::Lexer::convert(input);
            QCOMPARE(actual, output);
        }
    }
}
void testLexerNumbers()
{
    using calqmath::TokenNumber;
    using TestCase = std::tuple<std::string, std::vector<calqmath::Token>>;
    std::vector<TestCase> const cases{
        {"0.0", {TokenNumber{"0.0"}}},
        {"1.0", {TokenNumber{"1.0"}}},
        {"0.123", {TokenNumber{"0.123"}}},
        {"123.0", {TokenNumber{"123.0"}}},
        {".123", {TokenNumber{".123"}}},
        {"123.", {TokenNumber{"123."}}},
        {"123456789.0", {TokenNumber{"123456789.0"}}},
        {"1.2.3", {TokenNumber{"1.2"}, TokenNumber{".3"}}},
        {"123.456.789", {TokenNumber{"123.456"}, TokenNumber{".789"}}},
        {"1.2.3.4.5.6.7.8.9",
         {TokenNumber{"1.2"},
          TokenNumber{".3"},
          TokenNumber{".4"},
          TokenNumber{".5"},
          TokenNumber{".6"},
          TokenNumber{".7"},
          TokenNumber{".8"},
          TokenNumber{".9"}}}
    };
    for (auto const& [input, output] : cases)
    {
        auto const actual = calqmath::Lexer::convert(input);
        QCOMPARE(actual, output);
    }
}
void testLexerFunctionsAndNumbers()
{
    using calqmath::TokenIdentifier;
    using calqmath::TokenNumber;
    using TestCase = std::tuple<std::string, std::vector<calqmath::Token>>;
    std::vector<TestCase> const cases{
        {"sin", {TokenIdentifier{"sin"}}},
        {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
         {TokenIdentifier{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
         }}},
        {"sin12345678901234567890", {TokenIdentifier{"sin12345678901234567890"}}
        },
        {"123sin", {TokenNumber{"123"}, TokenIdentifier{"sin"}}},
        {"sin123", {TokenIdentifier{"sin123"}}},
        {"sin123sin", {TokenIdentifier{"sin123sin"}}},
        {"sin123.456", {TokenIdentifier{"sin123"}, TokenNumber{".456"}}},
        {"0.0sin", {TokenNumber{"0.0"}, TokenIdentifier{"sin"}}},
    };
    for (auto const& [input, output] : cases)
    {
        auto const actual = calqmath::Lexer::convert(input);
        QCOMPARE(actual, output);
    }
}

void testLexerSingleCharacterTokens()
{
    using calqmath::TokenClosedBracket;
    using calqmath::TokenOpenBracket;
    using calqmath::TokenOperator;
    auto const actual = calqmath::Lexer::convert("+-*/()");
    std::vector<calqmath::Token> const expected{
        TokenOperator::Plus,
        TokenOperator::Minus,
        TokenOperator::Multiply,
        TokenOperator::Divide,
        TokenOpenBracket{},
        TokenClosedBracket{},
    };
    QCOMPARE(actual, expected);
}
void testLexerMisc()
{
    // Valid token streams, but invalid when parsed to an expression
    std::vector<std::string> const invalidTestCases{
        "0..",
        ".",
        ".0.",
        "..0",
    };

    for (auto const& input : invalidTestCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(!tokens.has_value());
    }
}
void testLexerVariable()
{
    calqmath::TokenIdentifier const variableToken{
        .m_functionName = calqmath::InputVariable::RESERVED_NAME
    };

    using TestCase = std::pair<std::string, std::vector<calqmath::Token>>;
    std::vector<TestCase> const testCases{
        {"x", {variableToken}},
        {"1+x",
         {calqmath::TokenNumber{.m_decimalRepresentation = "1"},
          calqmath::TokenOperator::Plus,
          variableToken}},
    };

    for (auto const& [input, expected] : testCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QCOMPARE(tokens, expected);
    }
}

void testInterpretVariable(calqmath::FunctionDatabase const& functions)
{
    using calqmath::Scalar;

    Scalar const variable{"2.5"};

    using TestCase = std::pair<std::string, calqmath::Scalar>;
    std::vector<TestCase> const testCases{
        {"x", Scalar{"2.5"}},
        {"1+x", Scalar{"3.5"}},
        {"x+1", Scalar{"3.5"}},
        {"2 * x", Scalar{"5.0"}},
        {"x / 2", Scalar{"1.25"}},
    };

    for (auto const& [input, expected] : testCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const expression =
            calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(expression.has_value());

        auto const result = expression.value().evaluate(variable);
        QCOMPARE(result, expected);
    }
}

void testParserFunctions(calqmath::FunctionDatabase const& functions)
{
    std::vector<std::string> const invalidTestCases{
        "id()",
        "id(id())",
        "0.0 + id()",
        "id() + 0.0",
        "id(",
        "5.0 + id(",
        "id())",
        "id(5.0",
        "5.0 + id(5.0"
    };

    for (auto const& input : invalidTestCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const expression =
            calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(!expression.has_value());
    }
}

void testParserMisc(calqmath::FunctionDatabase const& functions)
{
    // Valid token streams, but invalid when parsed to an expression
    std::vector<std::string> const invalidTestCases{
        "+-*/",
        "0+",
        "+0",
        "++",
        "+",
        "0-",
        "--",
        "-",
        "0*",
        "*0",
        "**",
        "*",
        "0/",
        "/0",
        "//",
        "/",
        "",
    };

    for (auto const& input : invalidTestCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(!actual.has_value());
    }

    std::vector<std::tuple<std::string, size_t>> const termCountCases{
        {"1", 1}, {"123", 1}, {"1+2", 2}, {"123+456", 2}
    };
    for (auto const& [input, termCount] : termCountCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(actual.has_value());
        // Test both functions, although this would be redundant in actual
        // code
        QCOMPARE(actual.value().termCount(), termCount);
        QVERIFY(actual.value().empty() == (termCount == 0));
    }
}

void testInterpretNonOrdinaryScalars(calqmath::FunctionDatabase const& functions
)
{
    auto const nan{calqmath::Scalar::nan()};
    auto const positiveInf{calqmath::Scalar::positiveInf()};
    auto const negativeInf{calqmath::Scalar::negativeInf()};

    using TestCase = std::tuple<std::string, calqmath::Scalar>;
    std::vector<TestCase> const infinityCases{
        {"1 / 0", positiveInf},
        {"-1 / 0", negativeInf},
        {"-(1/0)", negativeInf},
        {"1.0 + 1 / 0", positiveInf},
        {"1.0 - 1 / 0", negativeInf},
        {"1 / 0 + 1.0", positiveInf},
        {"1 / 0 - 1.0", positiveInf},
        {"1 / 0 + 1 / 0", positiveInf},

        {"1 / 0 * 1 / 0", positiveInf},
        {"1 / 0 / 1 / 0", positiveInf},

    };

    for (auto const& [input, expected] : infinityCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(actual.has_value());

        auto const actualResult = actual.value().evaluate();
        QCOMPARE(actualResult, expected);
    }

    std::vector<std::string> const nanCases{
        "1 / 0 - 1 / 0",
        "0 / 0",
    };

    for (auto const& input : nanCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(actual.has_value());

        auto const actualResult = actual.value().evaluate();
        QVERIFY(actualResult.has_value());
        QVERIFY(actualResult.value().isNaN());
    }
}

void testInterpretMixedNegation(calqmath::FunctionDatabase const& functions)
{
    using TestCase = std::tuple<std::string, std::string>;
    std::vector<TestCase> const validTestCases{
        {"-1", "-1"},
        {"id(1)", "1"},
        {"-id(1)", "-1"},
        {"-(1)", "-1"},
        {"-(-1)", "1"},
        {"(-1)", "-1"},
        {"-(id(1))", "-1"},
        {"-(-id(1))", "1"},
        {"(-id(1))", "-1"},
    };

    for (auto const& [input, outputDecimalRepresentation] : validTestCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(actual.has_value());

        auto const actualResult = actual.value().evaluate();
        auto const expectedResult =
            calqmath::Scalar{outputDecimalRepresentation};

        QCOMPARE(actualResult, expectedResult);
    }
}

void testParserParantheses(calqmath::FunctionDatabase const& functions)
{
    std::vector<std::string> const invalid{
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
    for (auto const& input : invalid)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(!actual.has_value());
    }

    std::vector<std::string> const validTestCases{
        "(1.1)",
        "((1.1))",
        "(((1.1)))",

        "1.0 + (2.0)",
        "(1.0) + 2.0",
        "3.0 * (2.0)",
        "(3.0) * (2.0)",

        "0.0 + (1.0 + (2.0 + (3.0 + (4.0 + (5.0)))))",
        "((((((0.0) + 1.0) + 2.0) + 3.0) + 4.0) + 5.0)",

        "2.0 * (3.0 + 4.0)",
    };
    for (auto const& input : validTestCases)
    {
        auto const tokens = calqmath::Lexer::convert(input);
        QVERIFY(tokens.has_value());

        auto const actual = calqmath::Parser::parse(functions, tokens.value());
        QVERIFY(actual.has_value());
    }
}
} // namespace

void TestMathInterpreter::test()
{
    calqmath::Interpreter const interpreter{};

    // Test this first, since a lot, including debugging, relies on being
    // able to stringify properly.
    testScalarStringify();
    testScalarOperators();
    testNonOrdinaryScalarStringify();

    // Test components in order of dependency

    testLexerWhitespace();
    testLexerNumbers();
    testLexerFunctionsAndNumbers();
    testLexerSingleCharacterTokens();
    testLexerMisc();
    testLexerVariable();

    auto const functions{calqmath::FunctionDatabase::createWithDefaults()};
    testParserParantheses(functions);
    testParserMisc(functions);
    testParserFunctions(functions);

    testInterpretVariable(functions);
    testInterpretNonOrdinaryScalars(functions);
    testInterpretMixedNegation(functions);
    testInterpret(interpreter);
    testOrderOfOperators(interpreter);
    testFunctionParsing(interpreter);
    testAllFunctions(functions, interpreter);
    testMinimalPrecision(interpreter);
}

QTEST_MAIN(TestMathInterpreter)
#include "mathtest.moc"
