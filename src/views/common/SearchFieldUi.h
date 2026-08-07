#pragma once

#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>

inline void addSearchIcon(QLineEdit* field)
{
    QPixmap image(18, 18);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#64748B"), 1.7, Qt::SolidLine, Qt::RoundCap));
    painter.drawEllipse(QRectF(2.5, 2.5, 9, 9));
    painter.drawLine(QPointF(10, 10), QPointF(15, 15));
    field->addAction(QIcon(image), QLineEdit::LeadingPosition);
}
