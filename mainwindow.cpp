#include "mainwindow.h"
#include "math/mathinterpreter.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::onLineEnterPressed);

    m_messages = std::make_unique<QStringList>();
    m_messagesModel = std::make_unique<QStringListModel>();
    m_messagesModel->setStringList(*m_messages);

    ui->listView->setModel(m_messagesModel.get());
}

MainWindow::~MainWindow() {}

void MainWindow::onLineEnterPressed()
{
    QString const newMessage = ui->lineEdit->text().trimmed();
    if(newMessage.isEmpty())
    {
        return;
    }

    auto const mathResult =
        MathInterpreter::interpret(newMessage.toStdString());

    if (mathResult.has_value())
    {
        m_messages->append(QString::number(mathResult.value()));
    }
    else
    {
        switch (mathResult.error())
        {
        case MathInterpretationError::ParseError:
            m_messages->append("Parse Error");
            break;
        case MathInterpretationError::EvaluationError:
            m_messages->append("Evaluation Error");
            break;
        }
    }

    m_messagesModel->setStringList(*m_messages);
    ui->lineEdit->clear();
};
