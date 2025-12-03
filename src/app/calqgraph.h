#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>

#include "interpreter/expression.h"

namespace calqapp
{
class CalQGraph : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit CalQGraph(QWidget* parent = nullptr);

    void setExpression(calqmath::Expression const&);

protected:
    // void paintEvent(QPaintEvent*) override;

    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    std::optional<calqmath::Expression> m_expression;

    /*
     * Factor to multiply pixel-size of features by. For example, minor tick
     * lines may be 10 pixels apart at 1.0, but at 2.0 they would be 20 pixels
     * apart.
     */
    qreal m_graphScale{1.0};

    // The origin of the graph, in pixels.
    QPointF m_graphTranslation;

    std::optional<QPointF> m_mousePreviousPosition;

    int m_width;
    int m_height;
};
} // namespace calqapp
