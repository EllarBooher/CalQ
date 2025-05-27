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
    ~MainWindow() override;

private slots:
    void onLineEnterPressed();
    void onLineTextUpdated(QString const& newText);
    void setPreviewLabels(QString const& equation, QString const& result);
    void resetPreviewLabels();

    // NOLINTNEXTLINE (readability-redundant-access-specifiers)
private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    std::unique_ptr<QStringList> m_messages;
    std::unique_ptr<QStringListModel> m_messagesModel;
};
#endif // MAINWINDOW_H
