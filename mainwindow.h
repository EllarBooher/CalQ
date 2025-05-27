#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QStringListModel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLineEnterPressed();

private:
    std::unique_ptr<Ui::MainWindow> ui;

    std::unique_ptr<QStringList> m_messages;
    std::unique_ptr<QStringListModel> m_messagesModel;
};
#endif // MAINWINDOW_H
