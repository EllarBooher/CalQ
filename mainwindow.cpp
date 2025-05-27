#include "mainwindow.h"
#include "math/mathinterpreter.h"
#include "ui_mainwindow.h"

#include <QLineEdit>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QWidget>

#include <expected>
#include <memory>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::onLineEnterPressed);
    connect(
        ui->lineEdit,
        &QLineEdit::textEdited,
        this,
        &MainWindow::onLineTextUpdated
    );

    m_messages = std::make_unique<QStringList>();
    m_messagesModel = std::make_unique<QStringListModel>();
    m_messagesModel->setStringList(*m_messages);

    ui->listView->setModel(m_messagesModel.get());

    resetPreviewLabels();
}

MainWindow::~MainWindow() = default;

namespace
{
auto mathResultToQString(
    std::expected<double, MathInterpretationError> const result
) -> QString
{
    if (result.has_value())
    {
        return QString::number(result.value());
    }

    switch (result.error())
    {
    case MathInterpretationError::ParseError:
        return "Parse Error";
    case MathInterpretationError::EvaluationError:
        return "Evaluation Error";
    default:
        return "Unknown Error";
    }
}
} // namespace

void MainWindow::onLineEnterPressed()
{
    QString const newMessage = ui->lineEdit->text().trimmed();
    if(newMessage.isEmpty())
    {
        return;
    }

    auto const messageStd = newMessage.toStdString();

    auto const prettified = "> " + MathInterpreter::prettify(messageStd);
    m_messages->append(QString::fromUtf8(prettified));

    auto const mathResult = MathInterpreter::interpret(messageStd);
    m_messages->append(mathResultToQString(mathResult));

    m_messagesModel->setStringList(*m_messages);
    ui->lineEdit->clear();

    resetPreviewLabels();
}

void MainWindow::onLineTextUpdated(QString const& text)
{
    if (text.isEmpty())
    {
        return;
    }

    auto const messageStd = text.toStdString();

    auto const equation =
        QString::fromUtf8(MathInterpreter::prettify(messageStd));
    auto const result =
        mathResultToQString(MathInterpreter::interpret(messageStd));

    setPreviewLabels(equation, result);
}

void MainWindow::setPreviewLabels(
    QString const& equation, QString const& result
)
{
    ui->labelEquation->setText("> " + equation);
    ui->labelResult->setText(result);
}

void MainWindow::resetPreviewLabels()
{
    ui->labelEquation->setText("> [Equation Preview]");
    ui->labelResult->setText(" [Result Preview]");
};
