#pragma once

#include <QWidget>

#include "interpreter/expression.h"

namespace calqapp
{
class CalQGraph : public QWidget
{
    Q_OBJECT
public:
    explicit CalQGraph(QWidget* parent = nullptr);

    void setExpression(calqmath::Expression const&);

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    std::optional<calqmath::Expression> m_expression;
    // The origin of the graph, in pixels.
    QPointF m_pixelOrigin;
    std::optional<QPointF> m_mousePreviousPosition;
};
} // namespace calqapp
