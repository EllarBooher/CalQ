#include "calqgraph.h"

#include "math/functions.h"

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

constexpr double MINOR_DENSITY_FADE_MIN = 0.3;
constexpr double MINOR_DENSITY_FADE_MAX = 0.1;

constexpr double MAJOR_DISTANCE_MATH_UNITS = 1.0;
constexpr double MATH_UNITS_PER_GRAPH_UNITS =
    MAJOR_DISTANCE_MATH_UNITS / MAJOR_DISTANCE_GRAPH_UNITS;

namespace
{
void draw(
    QPainter& painter,
    QPen const& functionEndpointPen,
    QPen const& functionPen,
    QRectF const& rectGraph,
    QRectF const& rectViewport,
    double graphScale,
    calqmath::Scalar const& currentX,
    calqmath::Scalar const& currentY,
    calqmath::Scalar const& nextX,
    calqmath::Scalar const& nextY
)
{
    QPointF const viewportStart{
        ((QPointF{currentX.toDouble(), -currentY.toDouble()}
          / MATH_UNITS_PER_GRAPH_UNITS)
         - rectGraph.center())
            / graphScale
        + rectViewport.center()
    };
    QPointF const viewportEnd{
        ((QPointF{nextX.toDouble(), -nextY.toDouble()}
          / MATH_UNITS_PER_GRAPH_UNITS)
         - rectGraph.center())
            / graphScale
        + rectViewport.center()
    };

    painter.setPen(functionEndpointPen);
    painter.drawPoint(viewportStart);
    painter.drawPoint(viewportEnd);

    painter.setPen(functionPen);
    painter.drawLine(viewportStart, viewportEnd);
}
} // namespace

void calqapp::CalQGraph::paintGL()
{
    QOpenGLFunctions* glFunc = QOpenGLContext::currentContext()->functions();
    glFunc->glClearColor(1.0, 1.0, 1.0, 1.0);
    glFunc->glClear(GL_COLOR_BUFFER_BIT);
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

    QPen const functionPen{Qt::red, 2, Qt::SolidLine};
    QPen const functionEndpointPen{Qt::red, 4, Qt::SolidLine};
    QPen const axisPen{QColor{25, 25, 25}, 2, Qt::SolidLine};
    QPen const majorPen{QColor{70, 70, 70}, 1, Qt::SolidLine};

    auto const minorDensity = m_graphScale / MINOR_DISTANCE_GRAPH_UNITS;
    double const minorFade{std::clamp(
        (minorDensity - MINOR_DENSITY_FADE_MIN)
            / (MINOR_DENSITY_FADE_MAX - MINOR_DENSITY_FADE_MIN),
        0.0,
        1.0
    )};
    QPen const minorPen{
        QColor{150, 150, 150, int(255 * minorFade)}, 0.5, Qt::SolidLine
    };

    {
        /*
         * Draw origin label separately, to avoid overdraw when drawing X and Y
         * ticks
         */
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

        auto const minorXMin = static_cast<ptrdiff_t>(
            std::floor(rectGraph.left() / MINOR_DISTANCE_GRAPH_UNITS)
        );
        auto const minorXMax = static_cast<ptrdiff_t>(
            std::ceil(rectGraph.right() / MINOR_DISTANCE_GRAPH_UNITS)
        );

        for (auto xIdx = minorXMin; xIdx <= minorXMax; xIdx += 1)
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

        auto const minorYMin = static_cast<ptrdiff_t>(
            std::floor(rectGraph.top() / MINOR_DISTANCE_GRAPH_UNITS)
        );
        auto const minorYMax = static_cast<ptrdiff_t>(
            std::ceil(rectGraph.bottom() / MINOR_DISTANCE_GRAPH_UNITS)
        );
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
    }

    {
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
    }

    if (m_expression.has_value() && rectViewport.width() > 0.0)
    {
        auto const& expression = m_expression.value();

        auto const xMin{rectGraph.left() * MATH_UNITS_PER_GRAPH_UNITS};
        auto const xMax{rectGraph.right() * MATH_UNITS_PER_GRAPH_UNITS};

        auto const middleXDelta{2.0 * (xMax - xMin) / rectViewport.width()};

        auto curve{calqmath::GraphCurve::generateUnitLine(
            calqmath::Scalar{xMin},
            calqmath::Scalar{xMax},
            calqmath::Scalar{middleXDelta}
        )};

        curve = expression.graph(curve);

        using calqmath::Scalar;

        for (auto const& chunk : curve.chunks)
        {
            ptrdiff_t const gridIdxBegin{
                calqmath::Functions::floor(chunk.beginX / chunk.middleXDelta)
                    .toSignedInt()
                + 1
            };
            ptrdiff_t const gridIdxEnd{
                calqmath::Functions::ceil(chunk.endX / chunk.middleXDelta)
                    .toSignedInt()
                - 1
            };

            draw(
                painter,
                functionEndpointPen,
                functionPen,
                rectGraph,
                rectViewport,
                m_graphScale,
                chunk.beginX,
                chunk.beginY,
                calqmath::Scalar{gridIdxBegin} * chunk.middleXDelta,
                chunk.middleY.front()
            );

            for (ptrdiff_t gridIdx = gridIdxBegin; gridIdx <= gridIdxEnd - 1;
                 gridIdx++)
            {
                auto const middleIdx{gridIdx - gridIdxBegin};

                draw(
                    painter,
                    functionEndpointPen,
                    functionPen,
                    rectGraph,
                    rectViewport,
                    m_graphScale,
                    calqmath::Scalar{gridIdx} * chunk.middleXDelta,
                    chunk.middleY.at(middleIdx),
                    calqmath::Scalar{gridIdx + 1} * chunk.middleXDelta,
                    chunk.middleY.at(middleIdx + 1)
                );
            }

            draw(
                painter,
                functionEndpointPen,
                functionPen,
                rectGraph,
                rectViewport,
                m_graphScale,
                calqmath::Scalar{gridIdxEnd} * chunk.middleXDelta,
                chunk.middleY.back(),
                chunk.endX,
                chunk.endY
            );
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
        m_graphScale - m_graphScale * 0.001 * event->angleDelta().y(),
        0.01,
        10.0
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
