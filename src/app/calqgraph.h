#pragma once

#include <QWidget>
#include <qquickwidget.h>

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

private:
    std::optional<calqmath::Expression> m_expression;
};
} // namespace calqapp
