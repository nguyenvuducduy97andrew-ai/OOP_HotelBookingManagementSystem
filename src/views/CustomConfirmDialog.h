#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

class CustomConfirmDialog : public QDialog {
private:
    bool m_confirmed;

public:
    explicit CustomConfirmDialog(const QString& title, const QString& message, bool isWarning = false, QWidget* parent = nullptr) 
        : QDialog(parent), m_confirmed(false) {
        setWindowTitle(title);
        setFixedSize(360, 200);
        
        // Hide help button
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        // Styling colors based on warning state
        QString confirmBtnColor = isWarning ? "#EF4444" : "#005BFE"; // Red for delete, Blue for normal
        QString confirmBtnHover = isWarning ? "#DC2626" : "#2B7BFF";

        setStyleSheet(QString(R"(
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
        )").arg(confirmBtnColor, confirmBtnHover));

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 20, 24, 20);
        layout->setSpacing(14);
        layout->setAlignment(Qt::AlignCenter);

        auto* iconLabel = new QLabel(isWarning ? "⚠️" : "❓", this);
        iconLabel->setObjectName("iconLabel");
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);

        auto* msgLabel = new QLabel(message, this);
        msgLabel->setObjectName("msgLabel");
        msgLabel->setWordWrap(true);
        msgLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(msgLabel);

        // Buttons row
        auto* btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(14);
        btnLayout->setAlignment(Qt::AlignCenter);

        auto* btnCancel = new QPushButton("Cancel", this);
        btnCancel->setObjectName("btnCancel");
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

        auto* btnConfirm = new QPushButton("Confirm", this);
        btnConfirm->setObjectName("btnConfirm");
        connect(btnConfirm, &QPushButton::clicked, this, [=]() {
            m_confirmed = true;
            accept();
        });

        btnLayout->addWidget(btnCancel);
        btnLayout->addWidget(btnConfirm);
        layout->addLayout(btnLayout);
    }

    bool isConfirmed() const { return m_confirmed; }
};
