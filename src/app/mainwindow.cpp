#include "mainwindow.h"
#include "interpreter/interpreter.h"
#include "ui_mainwindow.h"

// #include <QFileInfo>
#include <QQmlContext>
// #include <QQmlEngine>
// #include <QQuickWidget>
// #include <QScatterDataProxy>

#include <QLineEdit>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QWidget>

#include <expected>
#include <memory>

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
    m_series->append(QList<QPointF>{{0.5F, 0.5F}, {-0.3F, -0.5F}, {0.0F, -0.3F}}
    );

    m_graph->engine()->rootContext()->setContextProperty(
        "series", m_series.get()
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

    auto const mathResult = m_interpreter->interpret(messageStd);
    m_messages->append(::toString(mathResult));

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
};
