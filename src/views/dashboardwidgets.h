#pragma once

#include <QFrame>
#include <QString>
#include <QColor>

class QLabel;

// ---------------------------------------------------------------
// StatCard: the four large cards at the top ("Total rooms", "Occupancy rate", etc.).
// The default constructor supports promoted widgets.
// Call setData(...) after setupUi() to populate the content.
// ---------------------------------------------------------------
class StatCard : public QFrame
{
    Q_OBJECT
public:
    explicit StatCard(QWidget *parent = nullptr);

    void setData(const QString &title,
                 const QString &value,
                 const QString &subtitle = QString(),
                 bool trendPositive = true);

private:
    QLabel *m_titleLabel;
    QLabel *m_valueLabel;
    QLabel *m_subtitleLabel;
};

// ---------------------------------------------------------------
// MiniCard: the four compact cards ("Upcoming", "In house", "Completed", "Cancelled").
// ---------------------------------------------------------------
class MiniCard : public QFrame
{
    Q_OBJECT
public:
    explicit MiniCard(QWidget *parent = nullptr);

    void setData(const QString &iconText,
                 const QColor &iconBg,
                 const QString &label,
                 const QString &value);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QLabel *m_valueLabel;
};

// ---------------------------------------------------------------
// RoomListItem: one row in "Featured rooms this month".
// ---------------------------------------------------------------
class RoomListItem : public QFrame
{
    Q_OBJECT
public:
    explicit RoomListItem(QWidget *parent = nullptr);

    void setData(const QString &roomTitle,
                 const QString &roomSubtitle,
                 const QString &badgeText,
                 const QColor &badgeColor);

private:
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QLabel *m_badgeLabel;
};


