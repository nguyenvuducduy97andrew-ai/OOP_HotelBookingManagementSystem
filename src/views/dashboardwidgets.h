#pragma once

#include <QFrame>
#include <QString>
#include <QColor>

class QLabel;

// ---------------------------------------------------------------
// StatCard: 4 thẻ to trên cùng ("Tổng số phòng", "Tỷ lệ lấp đầy" ...)
// Constructor không tham số để dùng được với Promoted Widgets.
// Gọi setData(...) sau khi setupUi() để đổ nội dung.
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
// MiniCard: 4 thẻ nhỏ ("Sắp tới", "Đang ở", "Hoàn tất", "Đã hủy")
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

private:
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QLabel *m_valueLabel;
};

// ---------------------------------------------------------------
// RoomListItem: một dòng trong "Phòng nổi bật tháng này"
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


