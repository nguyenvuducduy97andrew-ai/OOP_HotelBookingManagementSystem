#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include "DialogWindowBehavior.h"

class CustomConfirmDialog : public QDialog {
private:
    bool m_confirmed;

public:
    explicit CustomConfirmDialog(const QString& title, const QString& message, bool isWarning = false, QWidget* parent = nullptr,
                                 const QString& confirmText = QStringLiteral("Confirm"),
                                 const QString& cancelText = QStringLiteral("Cancel"),
                                 bool useSoftWarningTone = false)
        : QDialog(parent), m_confirmed(false) {
        setWindowTitle(title);
        setMinimumWidth(460);
        // Modified: Replace the platform-dark title bar with an opaque in-app header; no translucent surface is used.
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

        const QString confirmBtnColor = useSoftWarningTone ? "#D97706" : (isWarning ? "#EF4444" : "#3B58FF");
        const QString confirmBtnHover = useSoftWarningTone ? "#B45309" : (isWarning ? "#DC2626" : "#4F6BFF");
        const QString accentBackground = useSoftWarningTone ? "#FFFBEB" : "#F4F7FE";
        const QString accentBorder = useSoftWarningTone ? "#FDE68A" : "#DCE6F5";

        setStyleSheet(QString(R"(
            QDialog {
                background-color: #FFFFFF;
                border: 1px solid #CBD5E1;
                border-radius: 16px;
            }
            QFrame#dialogHeader {
                background: #FFFFFF;
                border: none;
                border-bottom: 1px solid #E7EDF7;
                border-top-left-radius: 15px;
                border-top-right-radius: 15px;
            }
            QLabel#windowTitle { color: #1B3F83; font-size: 13px; font-weight: 800; background: transparent; border: none; }
            QPushButton#windowClose {
                background: transparent; color: #64748B; border: none; border-radius: 8px; font-size: 20px; font-weight: 400;
            }
            QPushButton#windowClose:hover { background: #F1F5F9; color: #1B3F83; }
            QLabel#headerIcon { color: #3B58FF; font-size: 16px; background: transparent; border: none; }
            QFrame#dialogBody {
                background: #FFFFFF;
                border: none;
                border-bottom-left-radius: 15px;
                border-bottom-right-radius: 15px;
            }
            QFrame#noticePanel {
                background-color: %3;
                border: 1px solid %4;
                border-radius: 14px;
            }
            QFrame#noticePanel QLabel { background: transparent; border: none; }
            QLabel#iconLabel {
                font-size: 30px;
                min-width: 42px;
                color: %1;
            }
            QLabel#msgLabel {
                font-size: 13px;
                color: #2B3674;
                font-weight: 600;
            }
            QLabel#secondaryLabel {
                font-size: 12px;
                color: #A3AED0;
                font-weight: 500;
            }
            QPushButton#btnConfirm {
                background-color: %1;
                color: #FFFFFF;
                font-weight: 700;
                border-radius: 8px;
                padding: 8px 22px;
                border: none;
                font-size: 13px;
                min-width: 95px;
            }
            QPushButton#btnConfirm:hover {
                background-color: %2;
            }
            QPushButton#btnCancel {
                background-color: #E9EDF7;
                color: #2B3674;
                font-weight: 700;
                border-radius: 8px;
                padding: 8px 22px;
                border: none;
                font-size: 13px;
                min-width: 95px;
            }
            QPushButton#btnCancel:hover {
                background-color: #D3DDF4;
            }
        )").arg(confirmBtnColor, confirmBtnHover, accentBackground, accentBorder));

        auto* layout = new QVBoxLayout(this);
        // Modified: Let operational warnings grow to fit affected-booking details instead of clipping important consequences in a fixed-height dialog.
        layout->setSizeConstraint(QLayout::SetFixedSize);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        auto* header = new QFrame(this);
        header->setObjectName("dialogHeader");
        header->setFixedHeight(48);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(18, 0, 10, 0);
        auto* headerIcon = new QLabel(isWarning || useSoftWarningTone ? "⚠" : "ⓘ", header);
        headerIcon->setObjectName("headerIcon");
        auto* windowTitle = new QLabel(title, header);
        windowTitle->setObjectName("windowTitle");
        auto* close = new QPushButton(header);
        close->setObjectName("windowClose");
        styleDialogCloseButton(close);
        headerLayout->addWidget(headerIcon);
        headerLayout->addSpacing(8);
        headerLayout->addWidget(windowTitle);
        headerLayout->addStretch();
        headerLayout->addWidget(close);
        connect(close, &QPushButton::clicked, this, &QDialog::reject);
        enableDialogHeaderDrag(this, header);
        layout->addWidget(header);

        auto* body = new QFrame(this);
        body->setObjectName("dialogBody");
        auto* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(24, 20, 24, 20);
        bodyLayout->setSpacing(16);

        auto* noticePanel = new QFrame(this);
        noticePanel->setObjectName("noticePanel");
        auto* noticeLayout = new QHBoxLayout(noticePanel);
        noticeLayout->setContentsMargins(16, 14, 16, 14);
        noticeLayout->setSpacing(12);
        auto* iconLabel = new QLabel(useSoftWarningTone || isWarning ? "⚠" : "ⓘ", noticePanel);
        iconLabel->setObjectName("iconLabel");
        iconLabel->setAlignment(Qt::AlignCenter);
        noticeLayout->addWidget(iconLabel, 0, Qt::AlignTop);
        auto* textLayout = new QVBoxLayout();
        textLayout->setSpacing(5);
        auto* titleLabel = new QLabel(title, noticePanel);
        titleLabel->setObjectName("titleLabel");
        textLayout->addWidget(titleLabel);

        auto* msgLabel = new QLabel(message, noticePanel);
        msgLabel->setObjectName("msgLabel");
        msgLabel->setWordWrap(true);
        msgLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        textLayout->addWidget(msgLabel);
        noticeLayout->addLayout(textLayout, 1);
        bodyLayout->addWidget(noticePanel);

        // Buttons row
        auto* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(14);
        btnLayout->setAlignment(Qt::AlignCenter);

        auto* btnCancel = new QPushButton(cancelText, this);
        btnCancel->setObjectName("btnCancel");
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

        auto* btnConfirm = new QPushButton(confirmText, this);
        btnConfirm->setObjectName("btnConfirm");
        connect(btnConfirm, &QPushButton::clicked, this, [=]() {
            m_confirmed = true;
            accept();
        });

        btnLayout->addWidget(btnCancel);
        btnLayout->addWidget(btnConfirm);
        bodyLayout->addLayout(btnLayout);
        layout->addWidget(body);
        // Modified: Constrain confirmation windows to the active application area after their word-wrapped content determines the required height.
        lockDialogToWorkingArea(this, layout->sizeHint());
    }

    bool isConfirmed() const { return m_confirmed; }
};
