#include "dashboardwidgets.h"
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QEnterEvent>

// ---------------------------------------------------------------
StatCard::StatCard(QWidget *parent)
    : QFrame(parent)
{
    setMinimumHeight(108);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 14);
    layout->setSpacing(6);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("CardTitle");

    m_valueLabel = new QLabel(this);
    m_valueLabel->setObjectName("CardValue");

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setObjectName("CardSubtitlePositive");
    m_subtitleLabel->setVisible(false);

    layout->addWidget(m_titleLabel);
    layout->addWidget(m_valueLabel);
    layout->addWidget(m_subtitleLabel);
    layout->addStretch();
}

void StatCard::setData(const QString &title, const QString &value,
                       const QString &subtitle, bool trendPositive)
{
    m_titleLabel->setText(title);
    m_valueLabel->setText(value);

    if (subtitle.isEmpty()) {
        m_subtitleLabel->setVisible(false);
        return;
    }
    m_subtitleLabel->setText(subtitle);
    m_subtitleLabel->setObjectName(trendPositive ? "CardSubtitlePositive"
                                                 : "CardSubtitleNegative");
    style()->unpolish(m_subtitleLabel);
    style()->polish(m_subtitleLabel);
    m_subtitleLabel->setVisible(true);
}

// ---------------------------------------------------------------
MiniCard::MiniCard(QWidget *parent)
    : QFrame(parent)
{
    setMinimumHeight(72);
    setCursor(Qt::PointingHandCursor);

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(16, 12, 16, 12);
    outer->setSpacing(14);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    auto *textCol = new QVBoxLayout();
    textCol->setSpacing(2);
    m_textLabel = new QLabel(this);
    m_textLabel->setObjectName("MiniCardLabel");
    m_valueLabel = new QLabel(this);
    m_valueLabel->setObjectName("MiniCardValue");
    textCol->addWidget(m_textLabel);
    textCol->addWidget(m_valueLabel);

    outer->addWidget(m_iconLabel);
    outer->addLayout(textCol);
    outer->addStretch();
    
    updateStyle(false);
}

void MiniCard::setData(const QString &iconText, const QColor &iconBg,
                       const QString &label, const QString &value)
{
    m_iconLabel->setText(iconText);
    m_iconLabel->setStyleSheet(QString(
                                   "background-color: %1; border-radius: 10px; font-size: 16px; color: #1B2559;")
                                   .arg(iconBg.name()));
    m_textLabel->setText(label);
    m_valueLabel->setText(value);
}

void MiniCard::setActive(bool active)
{
    if (m_isActive == active) return;
    m_isActive = active;
    updateStyle(false);
}

void MiniCard::updateStyle(bool hover)
{
    m_textLabel->setStyleSheet("background: transparent; " + QString(m_isActive ? "color: #FFFFFF; font-weight: bold;" : ""));
    m_valueLabel->setStyleSheet("background: transparent; " + QString(m_isActive ? "color: #FFFFFF; font-weight: bold;" : "")); 
    
    if (m_isActive) {
        if (hover) {
            setStyleSheet("MiniCard { background-color: #1874CD; border-radius: 12px; border: 1px solid #1874CD; }");
        } else {
            setStyleSheet("MiniCard { background-color: #1E90FF; border-radius: 12px; border: 1px solid #1E90FF; }");
        }
    } else {
        if (hover) {
            setStyleSheet("MiniCard { background-color: #F1F5F9; border-radius: 12px; border: 1px solid #DCE6F5; }");
        } else {
            setStyleSheet("MiniCard { background-color: #FFFFFF; border-radius: 12px; border: 1px solid #DCE6F5; }");
        }
    }
}

void MiniCard::enterEvent(QEnterEvent *event)
{
    updateStyle(true);
    QFrame::enterEvent(event);
}

void MiniCard::leaveEvent(QEvent *event)
{
    updateStyle(false); 
    QFrame::leaveEvent(event);
}

void MiniCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool willBeActive = !m_isActive;
        
        if (willBeActive && parentWidget()) {
            QList<MiniCard*> siblings = parentWidget()->findChildren<MiniCard*>();
            for (MiniCard* sibling : siblings) {
                if (sibling != this) {
                    sibling->setActive(false);
                }
            }
        }
        
        setActive(willBeActive);
        emit clicked();
    }
    QFrame::mousePressEvent(event);
}

void MiniCard::mouseReleaseEvent(QMouseEvent *event)
{
    QFrame::mouseReleaseEvent(event);
}

// ---------------------------------------------------------------
RoomListItem::RoomListItem(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("RoomListItem");

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(14, 10, 14, 10);
    outer->setSpacing(12);

    auto *textCol = new QVBoxLayout();
    textCol->setSpacing(2);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("RoomTitle");
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setObjectName("RoomSubtitle");
    textCol->addWidget(m_titleLabel);
    textCol->addWidget(m_subtitleLabel);

    m_badgeLabel = new QLabel(this);
    m_badgeLabel->setAlignment(Qt::AlignCenter);

    outer->addLayout(textCol);
    outer->addStretch();
    outer->addWidget(m_badgeLabel);
}

void RoomListItem::setData(const QString &roomTitle, const QString &roomSubtitle,
                           const QString &badgeText, const QColor &badgeColor)
{
    m_titleLabel->setText(roomTitle);
    m_subtitleLabel->setText(roomSubtitle);
    m_badgeLabel->setText(badgeText);
    m_badgeLabel->setStyleSheet(QString(
                                    "background-color: %1; color: white; border-radius: 12px;"
                                    "padding: 4px 12px; font-size: 11px; font-weight: 600;")
                                    .arg(badgeColor.name()));
}
