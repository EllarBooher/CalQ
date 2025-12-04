#include "mainwindow.h"
#include "interpreter/interpreter.h"
#include "ui_mainwindow.h"

#include <QLineEdit>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QWidget>

#include <QtNumeric>

#include "calqgraph.h"

#include <cassert>
#include <expected>
#include <memory>

calqapp::MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new calqapp::Ui::MainWindow)
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

    m_graph = std::make_unique<calqapp::CalQGraph>(this);

    m_ui->parentHLayout->layout()->addWidget(m_graph.get());
    m_graph->setSizePolicy(
        QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding
    );
    QSize constexpr GRAPH_MINIMUM_SIZE{500, 500};
    m_graph->setMinimumSize(GRAPH_MINIMUM_SIZE);
    m_graph->setExpression(m_interpreter->expression("sin(x)").value());

    m_ui->history->setModel(m_messagesModel.get());

    resetPreviewLabels();
}

calqapp::MainWindow::~MainWindow() = default;

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
    case calqmath::InterpretError::LexError:
        return "Lexical Error";
    case calqmath::InterpretError::ParseError:
        return "Parse Error";
    case calqmath::InterpretError::EvaluationError:
        return "Evaluation Error";
    default:
        return "Unknown Error";
    }
}

auto toString(std::optional<calqmath::Scalar> const& result) -> QString
{
    if (result.has_value())
    {
        return QString::fromStdString(result.value().toString());
    }

    return "Evaluation Error";
}
} // namespace

void calqapp::MainWindow::onLineEnterPressed()
{
    QString const newMessage = m_ui->input->text().trimmed();
    if (newMessage.isEmpty())
    {
        return;
    }

    auto const messageStd = newMessage.toStdString();

    auto const pretty = "> " + calqmath::Interpreter::prettify(messageStd);
    auto const expression = m_interpreter->expression(messageStd);

    if (!expression.has_value())
    {
        return;
    }

    setGraphedExpression(expression.value());

    m_messages->append(QString::fromUtf8(pretty));
    m_messages->append(::toString(expression));

    m_messagesModel->setStringList(*m_messages);
    m_ui->input->clear();

    resetPreviewLabels();
}

void calqapp::MainWindow::onLineTextUpdated(QString const& newText)
{
    if (newText.isEmpty())
    {
        return;
    }

    auto const messageStd = newText.toStdString();

    auto const pretty =
        QString::fromUtf8(calqmath::Interpreter::prettify(messageStd));

    auto const expression = m_interpreter->expression(messageStd);

    if (!expression.has_value() || expression->hasVariable())
    {
        setPreviewLabels(pretty, ::toString(expression));
        return;
    }

    auto const evaluated = expression->evaluate();
    setPreviewLabels(pretty, ::toString(evaluated));
}

void calqapp::MainWindow::setPreviewLabels(
    QString const& equation, QString const& result
)
{
    m_ui->equation->setText("> " + equation);
    m_ui->result->setText(result);
}

void calqapp::MainWindow::resetPreviewLabels()
{
    m_ui->equation->setText("> [Equation Preview]");
    m_ui->result->setText(" [Result Preview]");
}

void calqapp::MainWindow::setGraphedExpression(
    calqmath::Expression const& expression
)
{
    m_graph->setExpression(expression);
    m_graph->update();
};
