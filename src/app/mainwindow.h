#pragma once

#include <QMainWindow>
#include <QQuickWidget>
#include <QScatterSeries>
#include <QStringList>
#include <QStringListModel>
// #include <QWidget>

namespace calqmath
{
class Interpreter;
} // namespace calqmath

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
    std::unique_ptr<calqmath::Interpreter> m_interpreter;
    std::unique_ptr<QScatterSeries> m_series;
    std::unique_ptr<QQuickWidget> m_graph;
};
