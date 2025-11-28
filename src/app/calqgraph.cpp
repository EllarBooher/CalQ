#include "calqgraph.h"

#include <QPainter>
#include <QtLogging>

calqapp::CalQGraph::CalQGraph(QWidget* parent)
    : QWidget{parent}
{
}

void calqapp::CalQGraph::setExpression(calqmath::Expression const& expression)
{
    m_expression = expression;
}

constexpr int32_t MINOR_PER_MAJOR = 5;
constexpr int32_t DISTANCE_MINOR = 20;
constexpr int32_t DISTANCE_MAJOR = MINOR_PER_MAJOR * DISTANCE_MINOR;

// Multiplicative conversion factor between
// - pixels ( as input to QPainter calls )
// - math coordinates ( as in input to calqmath::Expression )
constexpr double PIXEL_PER_MATH = DISTANCE_MAJOR;

void calqapp::CalQGraph::paintEvent(QPaintEvent* event)
{
    QPainter painter{this};

    painter.setRenderHint(QPainter::Antialiasing, true);

    auto const rect = event->rect();
    auto const minorXTickCount = rect.width() / DISTANCE_MINOR;
    auto const minorYTickCount = rect.height() / DISTANCE_MINOR;

    QPen const axisPen{QColor{25, 25, 25}, 2, Qt::SolidLine};
    QPen const majorPen{QColor{70, 70, 70}, 1, Qt::SolidLine};
    QPen const minorPen{QColor{150, 150, 150}, 0.5, Qt::SolidLine};

    // Draw origin label separately, to avoid overdraw when drawing X and Y
    // ticks
    {
        QString const label{"0"};
        QRectF const bounds{
            rect.center().x() - 12.0, qreal(rect.center().y()), 10, 20
        };
        QMarginsF const clearMargins{3.0, 3.0, 3.0, 4.0};

        painter.setPen(QPen{Qt::white});
        painter.setBrush(QBrush{Qt::white});
        painter.drawRect(bounds - clearMargins);

        painter.setPen(QPen{Qt::black});
        painter.drawText(
            bounds, label, QTextOption{Qt::AlignRight | Qt::AlignVCenter}
        );
    }

    for (int32_t xIdx = -((minorXTickCount + 1) / 2);
         xIdx <= ((minorXTickCount + 1) / 2);
         xIdx += 1)
    {
        bool const isMajor = (xIdx % MINOR_PER_MAJOR) == 0;
        auto const isAxis{xIdx == 0};

        auto const yStart = rect.top();
        auto const yEnd = rect.bottom();
        auto const xCoord = rect.center().x() + (xIdx * DISTANCE_MINOR);

        if (isMajor && !isAxis)
        {
            painter.setPen(majorPen);
            painter.drawLine(xCoord, yStart, xCoord, yEnd);

            auto const label = QString{"%1"}.arg(xIdx / MINOR_PER_MAJOR);
            QRectF const bounds{xCoord - 7.5, qreal(rect.center().y()), 15, 20};
            QMarginsF const clearMargins{0.0, 3.0, 3.0, 4.0};

            painter.setPen(QPen{Qt::white});
            painter.setBrush(QBrush{Qt::white});
            painter.drawRect(bounds - clearMargins);

            painter.setPen(QPen{Qt::black});
            painter.drawText(bounds, label, QTextOption{Qt::AlignCenter});
        }
        else if (!isMajor)
        {
            painter.setPen(minorPen);
            painter.drawLine(xCoord, yStart, xCoord, yEnd);
        }
    }

    for (int32_t yIdx = -((minorYTickCount + 1) / 2);
         yIdx <= ((minorYTickCount + 1) / 2);
         yIdx += 1)
    {
        bool const isMajor = (yIdx % MINOR_PER_MAJOR) == 0;
        bool const isAxis = yIdx == 0;

        auto const xStart = rect.left();
        auto const xEnd = rect.right();
        auto const yCoord = rect.center().y() + (yIdx * DISTANCE_MINOR);

        if (isMajor && !isAxis)
        {
            painter.setPen(majorPen);
            painter.drawLine(xStart, yCoord, xEnd, yCoord);
            auto const label = QString{"%1"}.arg(yIdx / MINOR_PER_MAJOR);
            auto const bounds =
                QRectF{rect.center().x() - 12.0, yCoord - 10.0, 10, 20};

            painter.setPen(QPen{Qt::white});
            painter.setBrush(QBrush{Qt::white});
            painter.drawRect(bounds - QMarginsF{0.0, 0.0, 0.0, 0.0});

            painter.setPen(QPen{Qt::black});
            painter.drawText(
                bounds, label, QTextOption{Qt::AlignRight | Qt::AlignVCenter}
            );
        }
        else if (!isMajor)
        {
            painter.setPen(minorPen);
            painter.drawLine(xStart, yCoord, xEnd, yCoord);
        }
    }

    painter.setPen(axisPen);
    painter.drawLine(
        rect.center().x(), rect.top(), rect.center().x(), rect.bottom()
    );
    painter.drawLine(
        rect.left(), rect.center().y(), rect.right(), rect.center().y()
    );

    if (m_expression.has_value())
    {
        auto const& expression = m_expression.value();

        painter.setPen(QPen{Qt::red});
        auto const xPadding = 0.1;
        auto const xMin = -rect.width() * 0.5 / PIXEL_PER_MATH - xPadding;
        auto const xMax = +rect.width() * 0.5 / PIXEL_PER_MATH + xPadding;

        QPointF mathStart{
            qreal(xMin),
            expression.evaluate(calqmath::Scalar{qreal(xMin)})
                .value()
                .toDouble()
        };

        auto const SAMPLE_COUNT{100};
        for (auto i = 0; i < SAMPLE_COUNT; i++)
        {
            auto const xSample =
                xMin + ((xMax - xMin) * (qreal(i) / (SAMPLE_COUNT - 1)));
            QPointF const mathEnd{
                xSample,
                expression.evaluate(calqmath::Scalar{xSample})
                    .value()
                    .toDouble()
            };

            QPointF const pixelStart{
                (DISTANCE_MAJOR * mathStart.x()) + rect.center().x(),
                (-DISTANCE_MAJOR * mathStart.y()) + rect.center().y(),
            };
            QPointF const pixelEnd{
                (DISTANCE_MAJOR * mathEnd.x()) + rect.center().x(),
                (-DISTANCE_MAJOR * mathEnd.y()) + rect.center().y(),
            };

            painter.drawLine(pixelStart, pixelEnd);

            mathStart = mathEnd;
        }
    }
}
