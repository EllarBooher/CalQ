#include "interpreter/interpreter.h"

#include "math/functions.h"
#include "math/number.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTest>
#include <QtLogging>

#include <expected>

class CalQBenchmark : public QObject
{
    Q_OBJECT
private slots:
    static void benchmarkEvaluation_data();
    static void benchmarkEvaluation();

    static void benchmarkScalarInit();

    static void benchmarkFunctions();
};

void CalQBenchmark::benchmarkEvaluation_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<size_t>("count");
    QTest::newRow("erf") << "erf(x)" << 10000ULL;
    QTest::newRow("erf^3") << "erf(erf(erf(x)))" << 10000ULL;
    QTest::newRow("constant") << "1" << 100000ULL;
    QTest::newRow("id") << "x" << 100000ULL;
    QTest::newRow("id^9") << "id(id(id(id(id(id(id(id(id(x)))))))))"
                          << 100000ULL;
    QTest::newRow("unit multiply 1")
        << "1 * 1 * 1 * 1 * 1 * 1 * 1 * x" << 100000ULL;
    QTest::newRow("unit multiply 2")
        << "x * 1 * 1 * 1 * 1 * 1 * 1 * 1" << 100000ULL;
    QTest::newRow("deep arithmatic")
        << "1 + x * (1 + x * (1 + x * (1 + x * (1 + x))))" << 100000ULL;
}

void CalQBenchmark::benchmarkEvaluation()
{
    calqmath::Interpreter const interpreter{};

    QFETCH(QString, input);
    QFETCH(size_t, count);

    auto const expressionResult{interpreter.expression(input.toStdString())};
    QVERIFY(expressionResult.has_value());

    auto const& expression{expressionResult.value()};

    QBENCHMARK
    {
        for (size_t i = 0; i < count; i++)
        {
            auto const result{expression.evaluate(
                calqmath::Scalar{i / static_cast<double>(count)}
            )};
            Q_UNUSED(result);
        }
    }
}

void CalQBenchmark::benchmarkScalarInit()
{
    auto const count{1000000};

    QBENCHMARK
    {
        for (size_t i = 0; i < count; i++)
        {
            calqmath::Scalar const result{i / static_cast<double>(count)};
            Q_UNUSED(result);
        }
    }
}

void CalQBenchmark::benchmarkFunctions()
{
    auto const count{1000000};

    QBENCHMARK
    {
        for (size_t i = 0; i < count; i++)
        {
            calqmath::Scalar const input{i / double(count)};
            Q_UNUSED(input);
        }
    }

    QBENCHMARK
    {
        for (size_t i = 0; i < count; i++)
        {
            calqmath::Scalar const input{i / double(count)};
            calqmath::Functions::sin(input);
        }
    }
}

QTEST_MAIN(CalQBenchmark)
#include "benchmark.moc"
