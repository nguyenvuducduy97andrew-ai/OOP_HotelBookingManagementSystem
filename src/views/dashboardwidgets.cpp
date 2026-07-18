#include "dashboardwidgets.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>

// ---------------------------------------------------------------
StatCard::StatCard(QWidget *parent)
    : QFrame(parent)
{
    setMinimumHeight(90);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(5);

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
    setMinimumHeight(58);

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(14, 8, 14, 8);
    outer->setSpacing(12);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(34, 34);
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
}

void MiniCard::setData(const QString &iconText, const QColor &iconBg,
                       const QString &label, const QString &value)
{
    m_iconLabel->setText(iconText);
    m_iconLabel->setStyleSheet(QString(
                                   "background-color: %1; border-radius: 8px; font-size: 14px;")
                                   .arg(iconBg.name()));
    m_textLabel->setText(label);
    m_valueLabel->setText(value);
}

// ---------------------------------------------------------------
RoomListItem::RoomListItem(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("RoomListItem");

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(12, 8, 12, 8);

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
                                    "background-color: %1; color: white; border-radius: 10px;"
                                    "padding: 3px 10px; font-size: 12px; font-weight: 600;")
                                    .arg(badgeColor.name()));
}
