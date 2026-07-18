#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class CustomSuccessDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomSuccessDialog(const QString& message, QWidget* parent = nullptr) 
        : QDialog(parent) {
        setWindowTitle("Thành công");
        setFixedSize(340, 190);
        
        // Hide standard window icon, keep close button
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        setStyleSheet(R"(
            QDialog {
                background-color: #FFFFFF;
            }
            QLabel#iconLabel {
                font-size: 42px;
            }
            QLabel#msgLabel {
                font-size: 13px;
                color: #2B3674;
                font-weight: 600;
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
        layout->setContentsMargins(24, 20, 24, 20);
        layout->setSpacing(14);
        layout->setAlignment(Qt::AlignCenter);

        auto* iconLabel = new QLabel("🎉", this);
        iconLabel->setObjectName("iconLabel");
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);

        auto* msgLabel = new QLabel(message, this);
        msgLabel->setObjectName("msgLabel");
        msgLabel->setWordWrap(true);
        msgLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(msgLabel);

        auto* btnOk = new QPushButton("Tuyệt vời!", this);
        btnOk->setObjectName("btnOk");
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(btnOk, 0, Qt::AlignCenter);
    }
};
