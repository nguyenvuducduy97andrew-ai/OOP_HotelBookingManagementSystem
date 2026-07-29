#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>

class InvoiceDialog : public QDialog {
public:
    explicit InvoiceDialog(const QString& invoiceDetails, QWidget* parent = nullptr) 
        : QDialog(parent) {
        // Modified: Name the document as an invoice because this screen does not record or confirm a payment.
        setWindowTitle("Booking Invoice");
        setFixedSize(460, 560);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        setStyleSheet(R"(
            QDialog {
                background-color: #FFFFFF;
            }
            QLabel#titleLabel {
                font-size: 16px;
                color: #2B3674;
                font-weight: 800;
            }
            QTextEdit {
                background-color: #F4F7FE;
                border: 1px solid #E9EDF7;
                border-radius: 12px;
                padding: 16px;
                font-family: 'Courier New', monospace;
                font-size: 12px;
                color: #2B3674;
            }
            QPushButton#btnConfirm {
                background-color: #05CD99;
                color: #FFFFFF;
                font-weight: 700;
                border-radius: 8px;
                padding: 10px 30px;
                border: none;
                font-size: 13px;
                min-width: 140px;
            }
            QPushButton#btnConfirm:hover {
                background-color: #04B184;
            }
        )");

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(24, 20, 24, 20);
        layout->setSpacing(14);

        auto* titleLabel = new QLabel("📄 BOOKING INVOICE DETAILS", this);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        auto* textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true);
        textEdit->setHtml(invoiceDetails);
        layout->addWidget(textEdit);

        auto* btnConfirm = new QPushButton("Complete", this);
        btnConfirm->setObjectName("btnConfirm");
        connect(btnConfirm, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(btnConfirm, 0, Qt::AlignCenter);
    }
};
