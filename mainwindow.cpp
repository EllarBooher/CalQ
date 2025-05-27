#include "mainwindow.h"
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

    m_messages->append(newMessage);
    m_messagesModel->setStringList(*m_messages);
    ui->lineEdit->clear();
};
