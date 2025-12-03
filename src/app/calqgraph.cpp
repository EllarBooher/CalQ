#include "calqgraph.h"

#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QPaintEvent>
#include <QPainter>
#include <QtLogging>

calqapp::CalQGraph::CalQGraph(QWidget* parent)
    : QOpenGLWidget{parent}
{
    QSurfaceFormat fmt;

    auto constexpr GRAPH_MULTISAMPLE_COUNT{8};
    fmt.setSamples(GRAPH_MULTISAMPLE_COUNT);
    setFormat(fmt);
}

void calqapp::CalQGraph::setExpression(calqmath::Expression const& expression)
{
    m_expression = expression;
}

void calqapp::CalQGraph::resizeGL(int const width, int const height)
{
    QOpenGLFunctions* glFunc = QOpenGLContext::currentContext()->functions();
    glFunc->glViewport(0, 0, width, height);

    m_width = width;
    m_height = height;
}

constexpr int32_t MINOR_PER_MAJOR = 5;
constexpr int32_t MINOR_DISTANCE_GRAPH_UNITS = 20;
constexpr int32_t MAJOR_DISTANCE_GRAPH_UNITS =
    MINOR_PER_MAJOR * MINOR_DISTANCE_GRAPH_UNITS;

constexpr double MAJOR_DISTANCE_MATH_UNITS = 1.0;
constexpr double MATH_UNITS_PER_GRAPH_UNITS =
    MAJOR_DISTANCE_MATH_UNITS / MAJOR_DISTANCE_GRAPH_UNITS;

void calqapp::CalQGraph::paintGL()
{
    QOpenGLFunctions* glFunc = QOpenGLContext::currentContext()->functions();
    glFunc->glClearColor(1.0, 1.0, 1.0, 1.0);
    glFunc->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glFunc->glEnable(GL_MULTISAMPLE);
    glFunc->glEnable(GL_LINE_SMOOTH);

    QPainter painter{this};

    painter.setRenderHint(QPainter::Antialiasing);

    auto const rectViewport = QRectF{
        0.0, 0.0, static_cast<qreal>(m_width), static_cast<qreal>(m_height)
    };
    auto const rectGraph = QRectF{
        (rectViewport.topLeft() - rectViewport.center()) * m_graphScale
            + m_graphTranslation,
        rectViewport.size() * m_graphScale
    };

    QPen const axisPen{QColor{25, 25, 25}, 2, Qt::SolidLine};
    QPen const majorPen{QColor{70, 70, 70}, 1, Qt::SolidLine};
    QPen const minorPen{QColor{150, 150, 150}, 0.5, Qt::SolidLine};

    // Draw origin label separately, to avoid overdraw when drawing X and Y
    // ticks
    {
        QString const label{"0"};
        QRectF const boundsViewport{
            ((0.0 - rectGraph.center().x()) / m_graphScale)
                + rectViewport.center().x() - 12.0,
            ((0.0 - rectGraph.center().y()) / m_graphScale)
                + rectViewport.center().y(),
            10,
            20
        };
        QMarginsF const clearMargins{3.0, 3.0, 3.0, 4.0};

        painter.setPen(QPen{Qt::white});
        painter.setBrush(QBrush{Qt::white});
        painter.drawRect(boundsViewport - clearMargins);

        painter.setPen(QPen{Qt::black});
        painter.drawText(
            boundsViewport,
            label,
            QTextOption{Qt::AlignRight | Qt::AlignVCenter}
        );
    }

    auto const minorXMin =
        std::floor(rectGraph.left() / MINOR_DISTANCE_GRAPH_UNITS);
    auto const minorXMax =
        std::ceil(rectGraph.right() / MINOR_DISTANCE_GRAPH_UNITS);
    for (int32_t xIdx = minorXMin; xIdx <= minorXMax; xIdx += 1)
    {
        bool const isMajor = (xIdx % MINOR_PER_MAJOR) == 0;
        auto const isAxis{xIdx == 0};

        auto const yViewportStart = rectViewport.top();
        auto const yViewportEnd = rectViewport.bottom();
        auto const xGraph = xIdx * MINOR_DISTANCE_GRAPH_UNITS;
        auto const xViewport =
            ((xGraph - rectGraph.center().x()) / m_graphScale)
            + rectViewport.center().x();

        if (isMajor && !isAxis)
        {
            painter.setPen(majorPen);
            painter.drawLine(
                xViewport, yViewportStart, xViewport, yViewportEnd
            );

            auto const xMath = xGraph * MATH_UNITS_PER_GRAPH_UNITS;
            auto const label = QString{"%1"}.arg(xMath);
            QRectF const boundsViewport{
                xViewport - 7.5,
                ((0.0 - rectGraph.center().y()) / m_graphScale)
                    + rectViewport.center().y(),
                15,
                20
            };
            QMarginsF const clearMargins{0.0, 3.0, 3.0, 4.0};

            painter.setPen(QPen{Qt::white});
            painter.setBrush(QBrush{Qt::white});
            painter.drawRect(boundsViewport - clearMargins);

            painter.setPen(QPen{Qt::black});
            painter.drawText(
                boundsViewport, label, QTextOption{Qt::AlignCenter}
            );
        }
        else if (!isMajor)
        {
            painter.setPen(minorPen);
            painter.drawLine(
                xViewport, yViewportStart, xViewport, yViewportEnd
            );
        }
    }

    auto const minorYMin =
        std::floor(rectGraph.top() / MINOR_DISTANCE_GRAPH_UNITS);
    auto const minorYMax =
        std::ceil(rectGraph.bottom() / MINOR_DISTANCE_GRAPH_UNITS);
    for (int32_t yIdx = minorYMin; yIdx <= minorYMax; yIdx += 1)
    {
        bool const isMajor = (yIdx % MINOR_PER_MAJOR) == 0;
        bool const isAxis = yIdx == 0;

        auto const xViewportStart = rectViewport.left();
        auto const xViewportEnd = rectViewport.right();
        auto const yGraph = yIdx * MINOR_DISTANCE_GRAPH_UNITS;
        auto const yViewport =
            ((yGraph - rectGraph.center().y()) / m_graphScale)
            + rectViewport.center().y();

        if (isMajor && !isAxis)
        {
            painter.setPen(majorPen);
            painter.drawLine(
                xViewportStart, yViewport, xViewportEnd, yViewport
            );

            auto const yMath = -yGraph * MATH_UNITS_PER_GRAPH_UNITS;
            auto const label = QString{"%1"}.arg(yMath);
            auto const boundsViewport = QRectF{
                (0.0 - rectGraph.center().x() / m_graphScale)
                    + rectViewport.center().x() - 12.0,
                yViewport - 10.0,
                10,
                20
            };
            QMarginsF const clearMargins{0.0, 3.0, 1.0, 4.0};

            painter.setPen(QPen{Qt::white});
            painter.setBrush(QBrush{Qt::white});
            painter.drawRect(boundsViewport - clearMargins);

            painter.setPen(QPen{Qt::black});
            painter.drawText(
                boundsViewport,
                label,
                QTextOption{Qt::AlignRight | Qt::AlignVCenter}
            );
        }
        else if (!isMajor)
        {
            painter.setPen(minorPen);
            painter.drawLine(
                xViewportStart, yViewport, xViewportEnd, yViewport
            );
        }
    }

    painter.setPen(axisPen);
    painter.drawLine(
        ((0.0 - rectGraph.center().x()) / m_graphScale)
            + rectViewport.center().x(),
        rectViewport.top(),
        ((0.0 - rectGraph.center().x()) / m_graphScale)
            + rectViewport.center().x(),
        rectViewport.bottom()
    );
    painter.drawLine(
        rectViewport.left(),
        ((0.0 - rectGraph.center().y()) / m_graphScale)
            + rectViewport.center().y(),
        rectViewport.right(),
        ((0.0 - rectGraph.center().y()) / m_graphScale)
            + rectViewport.center().y()
    );

    if (m_expression.has_value())
    {
        auto const& expression = m_expression.value();

        auto const xMin{rectGraph.left() * MATH_UNITS_PER_GRAPH_UNITS};
        auto const xMax{rectGraph.right() * MATH_UNITS_PER_GRAPH_UNITS};

        auto const deltaFractionX{0.5 / rectViewport.width()};

        QPointF prev{0.0, 0.0};
        QPointF next{
            xMin, expression.evaluate(calqmath::Scalar{xMin})->toDouble()
        };

        painter.setPen(QPen{Qt::red});
        auto fractionX{0.0};
        while (fractionX < 1.0)
        {
            fractionX += deltaFractionX;

            prev = next;

            auto const xNext{(fractionX * (xMax - xMin)) + xMin};
            next = {
                xNext, expression.evaluate(calqmath::Scalar{xNext})->toDouble()
            };

            QPointF const viewportStart{
                ((QPointF{prev.x(), -prev.y()} / MATH_UNITS_PER_GRAPH_UNITS)
                 - rectGraph.center())
                    / m_graphScale
                + rectViewport.center()
            };
            QPointF const viewportEnd{
                ((QPointF{next.x(), -next.y()} / MATH_UNITS_PER_GRAPH_UNITS)
                 - rectGraph.center())
                    / m_graphScale
                + rectViewport.center()
            };

            painter.drawLine(viewportStart, viewportEnd);
        }
    }
}

void calqapp::CalQGraph::mouseReleaseEvent(QMouseEvent*)
{
    m_mousePreviousPosition = std::nullopt;
}

void calqapp::CalQGraph::mouseMoveEvent(QMouseEvent* event)
{
    if (m_mousePreviousPosition.has_value())
    {
        auto const deltaViewport =
            event->pos() - m_mousePreviousPosition.value();

        m_graphTranslation -= deltaViewport * m_graphScale;
        update();
    }

    m_mousePreviousPosition = event->pos();
}

void calqapp::CalQGraph::wheelEvent(QWheelEvent* event)
{
    auto const newZoom{std::clamp(
        m_graphScale - m_graphScale * 0.001 * event->angleDelta().y(), 0.1, 10.0
    )};
    if (newZoom != m_graphScale)
    {
        auto const pointerDeltaFromViewportCenter =
            (event->position() - rect().toRectF().center());
        // This must stay fixed in the viewport, in order to give the illusion
        // that the graph stretches away from the pointer
        auto const pointerPositionInGraph =
            pointerDeltaFromViewportCenter * m_graphScale + m_graphTranslation;

        m_graphTranslation =
            pointerPositionInGraph - pointerDeltaFromViewportCenter * newZoom;

        m_graphScale = newZoom;

        update();
    }
}
