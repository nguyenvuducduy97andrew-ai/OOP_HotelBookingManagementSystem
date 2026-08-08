#pragma once

#include "DialogWindowBehavior.h"

#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

// Modified: Provide a consistent one-action alert dialog so validation messages never fall back to an unstyled system message box.
class CustomAlertDialog final : public QDialog {
public:
    explicit CustomAlertDialog(const QString& title,
                               const QString& message,
                               QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(title);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setMinimumWidth(420);
        setStyleSheet(R"(
            QDialog {
                background:#FFFFFF;
                border:1px solid #CBD5E1;
                border-radius:16px;
            }
            QFrame#alertHeader {
                background:#FFFFFF;
                border:none;
                border-bottom:1px solid #E7EDF7;
                border-top-left-radius:15px;
                border-top-right-radius:15px;
            }
            QFrame#alertBody {
                background:#FFFFFF;
                border:none;
                border-bottom-left-radius:15px;
                border-bottom-right-radius:15px;
            }
            QLabel#alertHeaderIcon, QLabel#alertWindowTitle,
            QLabel#alertIcon, QLabel#alertMessage {
                background:transparent;
                border:none;
            }
            QLabel#alertHeaderIcon { color:#D97706; font-size:16px; font-weight:800; }
            QLabel#alertWindowTitle { color:#1B3F83; font-size:13px; font-weight:800; }
            QLabel#alertIcon { color:#D97706; font-size:30px; font-weight:800; }
            QLabel#alertMessage { color:#2B3674; font-size:13px; font-weight:500; }
            QPushButton#alertOk {
                background:#005BFE;
                color:#FFFFFF;
                border:none;
                border-radius:9px;
                min-width:88px;
                min-height:34px;
                padding:0 18px;
                font-size:13px;
                font-weight:800;
            }
            QPushButton#alertOk:hover { background:#2B7BFF; }
            QPushButton#alertOk:pressed { background:#0048D8; }
        )");

        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setSizeConstraint(QLayout::SetFixedSize);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        auto* header = new QFrame(this);
        header->setObjectName("alertHeader");
        header->setFixedHeight(48);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(18, 0, 10, 0);
        headerLayout->setSpacing(8);
        auto* headerIcon = new QLabel(QStringLiteral("!"), header);
        headerIcon->setObjectName("alertHeaderIcon");
        auto* headerTitle = new QLabel(title, header);
        headerTitle->setObjectName("alertWindowTitle");
        auto* closeButton = new QPushButton(header);
        styleDialogCloseButton(closeButton);
        headerLayout->addWidget(headerIcon);
        headerLayout->addWidget(headerTitle);
        headerLayout->addStretch();
        headerLayout->addWidget(closeButton);
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
        enableDialogHeaderDrag(this, header);
        rootLayout->addWidget(header);

        auto* body = new QFrame(this);
        body->setObjectName("alertBody");
        auto* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(24, 18, 24, 20);
        bodyLayout->setSpacing(16);

        auto* messageRow = new QHBoxLayout();
        messageRow->setSpacing(14);
        auto* iconLabel = new QLabel(QStringLiteral("!"), body);
        iconLabel->setObjectName("alertIcon");
        iconLabel->setAlignment(Qt::AlignCenter);
        auto* messageLabel = new QLabel(message, body);
        messageLabel->setObjectName("alertMessage");
        messageLabel->setWordWrap(true);
        messageLabel->setMinimumWidth(300);
        messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        messageRow->addWidget(iconLabel, 0, Qt::AlignTop);
        messageRow->addWidget(messageLabel, 1);
        bodyLayout->addLayout(messageRow);

        auto* okButton = new QPushButton(QStringLiteral("OK"), body);
        okButton->setObjectName("alertOk");
        okButton->setDefault(true);
        okButton->setAutoDefault(true);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        bodyLayout->addWidget(okButton, 0, Qt::AlignRight);
        rootLayout->addWidget(body);

        // Modified: Size the alert from its wrapped content while keeping it bounded by the owning application window.
        lockDialogToWorkingArea(this, rootLayout->sizeHint());
    }
};
