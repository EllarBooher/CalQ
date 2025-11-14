#include "mainwindow.h"
#include "interpreter/interpreter.h"
#include "ui_mainwindow.h"

#include <QLineEdit>
#include <QMainWindow>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickItem>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QWidget>

#include <QtNumeric>

#include <algorithm>
#include <cassert>
#include <expected>
#include <memory>
#include <ranges>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    connect(
        m_ui->input,
        &QLineEdit::returnPressed,
        this,
        &MainWindow::onLineEnterPressed
    );
    connect(
        m_ui->input,
        &QLineEdit::textEdited,
        this,
        &MainWindow::onLineTextUpdated
    );

    m_messages = std::make_unique<QStringList>();
    m_messagesModel = std::make_unique<QStringListModel>();
    m_messagesModel->setStringList(*m_messages);
    m_interpreter = std::make_unique<calqmath::Interpreter>();

    m_graph = std::make_unique<QQuickWidget>();

    m_series = std::make_unique<QScatterSeries>();
    m_xAxis = std::make_unique<QValueAxis>();
    m_yAxis = std::make_unique<QValueAxis>();
    m_graph->engine()->rootContext()->setContextProperty(
        "series", m_series.get()
    );
    m_graph->engine()->rootContext()->setContextProperty(
        "x_axis", m_xAxis.get()
    );
    m_graph->engine()->rootContext()->setContextProperty(
        "y_axis", m_yAxis.get()
    );
    m_graph->setSource(QUrl("qrc:/src/app/graph.qml"));

    // QSize const screenSize = m_graph->screen()->size();
    m_graph->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_ui->parentHLayout->layout()->addWidget(m_graph.get());

    m_ui->history->setModel(m_messagesModel.get());

    resetPreviewLabels();
}

MainWindow::~MainWindow() = default;

namespace
{
auto toString(
    std::expected<calqmath::Expression, calqmath::InterpretError> const& result
) -> QString
{
    if (result.has_value())
    {
        return QString::fromStdString(result.value().string());
    }

    switch (result.error())
    {
    case calqmath::InterpretError::ParseError:
        return "Parse Error";
    case calqmath::InterpretError::EvaluationError:
        return "Evaluation Error";
    default:
        return "Unknown Error";
    }
}

auto toString(
    std::expected<calqmath::Scalar, calqmath::InterpretError> const& result
) -> QString
{
    if (result.has_value())
    {
        return QString::fromStdString(result.value().toString());
    }

    switch (result.error())
    {
    case calqmath::InterpretError::ParseError:
        return "Parse Error";
    case calqmath::InterpretError::EvaluationError:
        return "Evaluation Error";
    default:
        return "Unknown Error";
    }
}
} // namespace

void MainWindow::onLineEnterPressed()
{
    QString const newMessage = m_ui->input->text().trimmed();
    if(newMessage.isEmpty())
    {
        return;
    }

    auto const messageStd = newMessage.toStdString();

    auto const prettified = "> " + calqmath::Interpreter::prettify(messageStd);
    m_messages->append(QString::fromUtf8(prettified));

    auto const mathResult = m_interpreter->expression(messageStd);
    m_messages->append(::toString(mathResult));

    if (mathResult.has_value())
    {
        setGraphedExpression(mathResult.value());
    }

    m_messagesModel->setStringList(*m_messages);
    m_ui->input->clear();

    resetPreviewLabels();
}

void MainWindow::onLineTextUpdated(QString const& newText)
{
    if (newText.isEmpty())
    {
        return;
    }

    auto const messageStd = newText.toStdString();

    auto const equation =
        QString::fromUtf8(calqmath::Interpreter::prettify(messageStd));
    auto const result = ::toString(m_interpreter->interpret(messageStd));

    setPreviewLabels(equation, result);
}

void MainWindow::setPreviewLabels(
    QString const& equation, QString const& result
)
{
    m_ui->equation->setText("> " + equation);
    m_ui->result->setText(result);
}

void MainWindow::resetPreviewLabels()
{
    m_ui->equation->setText("> [Equation Preview]");
    m_ui->result->setText(" [Result Preview]");
}

void MainWindow::setGraphedExpression(calqmath::Expression const& expression)
{
    constexpr auto POINT_COUNT = 21;
    constexpr auto X_AXIS_MIN = -2.0;
    constexpr auto X_AXIS_MAX = 2.0;
    constexpr auto X_AXIS_TICK_INTERVAL = 0.5;
    constexpr auto Y_AXIS_MINMAX_MARGIN = 1.5;

    auto const applyExpression = [&](int64_t index) -> std::optional<QPointF>
    {
        auto const xValue =
            calqmath::Scalar{double(index - 10)} / calqmath::Scalar{5.0};
        auto const result = expression.evaluate(xValue);
        if (!result.has_value())
        {
            return std::nullopt;
        }
        return QPointF{xValue.toDouble(), result.value().toDouble()};
    };

    auto const isValid = [](std::optional<QPointF> point)
    { return point.has_value() && !qIsNaN(point->x()) && !qIsNaN(point->y()); };

    auto const unwrap = [](std::optional<QPointF> point)
    { return point.value(); };

    QList<QPointF> points{};
    for (QPointF const& point : std::views::iota(0, POINT_COUNT)
                                    | std::views::transform(applyExpression)
                                    | std::views::filter(isValid)
                                    | std::views::transform(unwrap))
    {
        points.append(point);
    };

    m_xAxis->setRange(X_AXIS_MIN, X_AXIS_MAX);
    m_xAxis->setTickInterval(X_AXIS_TICK_INTERVAL);

    auto const yBounds = std::ranges::minmax(
        points, {}, [](QPointF const& point) { return point.y(); }
    );

    m_yAxis->setRange(
        Y_AXIS_MINMAX_MARGIN * yBounds.min.y(),
        Y_AXIS_MINMAX_MARGIN * yBounds.max.y()
    );
    m_series->replace(points);
};
