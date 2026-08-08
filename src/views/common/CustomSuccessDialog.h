#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include "DialogWindowBehavior.h"

class CustomSuccessDialog : public QDialog {
public:
    explicit CustomSuccessDialog(const QString& message, QWidget* parent = nullptr) 
        : QDialog(parent) {
        setWindowTitle("Success");
        lockDialogToWorkingArea(this, QSize(360, 220));
        // Modified: Use the same opaque in-app header as confirmation dialogs instead of the system-dark title bar.
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

        setStyleSheet(R"(
            QDialog {
                background-color: #FFFFFF;
                border: 1px solid #CBD5E1;
                border-radius: 16px;
            }
            QFrame#successHeader { background:#FFFFFF; border:none; border-bottom:1px solid #E7EDF7; border-top-left-radius:15px; border-top-right-radius:15px; }
            QFrame#successBody { background:#FFFFFF; border:none; border-bottom-left-radius:15px; border-bottom-right-radius:15px; }
            QLabel#successWindowTitle { color:#1B3F83; font-size:13px; font-weight:800; background:transparent; border:none; }
            QLabel#successHeaderIcon { color: #059669; font-size:16px; background:transparent; border:none; }
            QPushButton#successWindowClose { background:transparent; color:#64748B; border:none; border-radius:8px; font-size:20px; }
            QPushButton#successWindowClose:hover { background:#F1F5F9; color:#1B3F83; }
            QLabel#iconLabel {
                font-size: 42px;
                color: #059669;
                background: transparent;
                border: none;
            }
            QLabel#msgLabel {
                font-size: 13px;
                color: #2B3674;
                font-weight: 600;
                background: transparent;
                border: none;
            }
            QPushButton#btnOk {
                background-color: #05CD99;
                color: #FFFFFF;
                font-weight: 700;
                border-radius: 8px;
                padding: 8px 28px;
                border: none;
                font-size: 13px;
            }
            QPushButton#btnOk:hover {
                background-color: #04B184;
            }
        )");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        auto* header = new QFrame(this);
        header->setObjectName("successHeader");
        header->setFixedHeight(48);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(18, 0, 10, 0);
        auto* headerIcon = new QLabel("✓", header);
        headerIcon->setObjectName("successHeaderIcon");
        auto* headerTitle = new QLabel("Operation completed", header);
        headerTitle->setObjectName("successWindowTitle");
        auto* close = new QPushButton(header);
        close->setObjectName("successWindowClose");
        styleDialogCloseButton(close);
        headerLayout->addWidget(headerIcon);
        headerLayout->addSpacing(8);
        headerLayout->addWidget(headerTitle);
        headerLayout->addStretch();
        headerLayout->addWidget(close);
        connect(close, &QPushButton::clicked, this, &QDialog::reject);
        enableDialogHeaderDrag(this, header);
        layout->addWidget(header);
        auto* body = new QFrame(this);
        body->setObjectName("successBody");
        auto* bodyLayout = new QVBoxLayout(body);
        bodyLayout->setContentsMargins(24, 14, 24, 18);
        bodyLayout->setSpacing(10);
        bodyLayout->setAlignment(Qt::AlignCenter);

        auto* iconLabel = new QLabel("✓", body);
        iconLabel->setObjectName("iconLabel");
        iconLabel->setAlignment(Qt::AlignCenter);
        bodyLayout->addWidget(iconLabel);

        auto* msgLabel = new QLabel(message, body);
        msgLabel->setObjectName("msgLabel");
        msgLabel->setWordWrap(true);
        msgLabel->setAlignment(Qt::AlignCenter);
        bodyLayout->addWidget(msgLabel);

        auto* btnOk = new QPushButton("Close", body);
        btnOk->setObjectName("btnOk");
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        bodyLayout->addWidget(btnOk, 0, Qt::AlignCenter);
        layout->addWidget(body);
    }
};
