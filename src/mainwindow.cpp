#include "mainwindow.h"
#include "math/mathinterpreter.h"
#include "math/mathstringify.h"
#include "ui_mainwindow.h"

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
        m_ui->lineEdit,
        &QLineEdit::returnPressed,
        this,
        &MainWindow::onLineEnterPressed
    );
    connect(
        m_ui->lineEdit,
        &QLineEdit::textEdited,
        this,
        &MainWindow::onLineTextUpdated
    );

    m_messages = std::make_unique<QStringList>();
    m_messagesModel = std::make_unique<QStringListModel>();
    m_messagesModel->setStringList(*m_messages);
    m_interpreter = std::make_unique<calqmath::Interpreter>();

    m_ui->listView->setModel(m_messagesModel.get());

    resetPreviewLabels();
}

MainWindow::~MainWindow() = default;

namespace
{
auto toString(
    std::expected<calqmath::Scalar, calqmath::InterpretError> const result
) -> QString
{
    if (result.has_value())
    {
        return QString::fromStdString(calqmath::toString(result.value()));
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
    QString const newMessage = m_ui->lineEdit->text().trimmed();
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
    m_ui->lineEdit->clear();

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
    m_ui->labelEquation->setText("> " + equation);
    m_ui->labelResult->setText(result);
}

void MainWindow::resetPreviewLabels()
{
    m_ui->labelEquation->setText("> [Equation Preview]");
    m_ui->labelResult->setText(" [Result Preview]");
};
