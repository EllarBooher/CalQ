#pragma once

#include "interpreter/interpreter.h"
#include <QMainWindow>
#include <QQuickWidget>
#include <QScatterSeries>
#include <QStringList>
#include <QStringListModel>
#include <QValueAxis>
// #include <QWidget>

namespace calqmath
{
class Interpreter;
} // namespace calqmath

namespace calqapp
{
class CalQGraph;

namespace Ui
{
class MainWindow;
} // namespace Ui

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onLineEnterPressed();
    void onLineTextUpdated(QString const& newText);
    void setPreviewLabels(QString const& equation, QString const& result);
    void resetPreviewLabels();

    // NOLINTNEXTLINE (readability-redundant-access-specifiers)
private:
    void setGraphedExpression(calqmath::Expression const& expression);

    std::unique_ptr<Ui::MainWindow> m_ui;
    std::unique_ptr<CalQGraph> m_graph;

    std::unique_ptr<QStringList> m_messages;
    std::unique_ptr<QStringListModel> m_messagesModel;
    std::unique_ptr<calqmath::Interpreter> m_interpreter;
};
} // namespace calqapp
