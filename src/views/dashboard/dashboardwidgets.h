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

    bool isActive() const { return m_isActive; }
    void setActive(bool active);
    void setAlert(bool alert);

signals:
    void clicked();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QLabel *m_valueLabel;
    bool m_isActive = false;
    bool m_isAlert = false;
    void updateStyle(bool hover);
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


