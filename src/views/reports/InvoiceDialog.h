#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include "DialogWindowBehavior.h"

class InvoiceDialog : public QDialog {
public:
    explicit InvoiceDialog(const QString& invoiceDetails, QWidget* parent = nullptr) 
        : QDialog(parent) {
        // Modified: Name the document as an invoice because this screen does not record or confirm a payment.
        setWindowTitle("Booking Invoice");
        lockDialogToWorkingArea(this, QSize(650, 640));
        // Modified: Replace the platform title bar with an opaque invoice header that matches application dialogs.
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

        setStyleSheet(R"(
            QDialog {
                background-color: #FFFFFF;
                border: 1px solid #CBD5E1;
                border-radius: 16px;
            }
            QLabel#titleLabel {
                font-size: 18px;
                color: #2B3674;
                font-weight: 800;
                background: transparent;
                border: none;
            }
            QLabel#subtitleLabel {
                font-size: 12px;
                color: #64748B;
                background: transparent;
                border: none;
            }
            QPushButton#invoiceClose {
                background: transparent;
                color: #64748B;
                border: none;
                border-radius: 8px;
                font-size: 20px;
                min-width: 30px;
                padding: 0;
            }
            QPushButton#invoiceClose:hover { background: #F1F5F9; color: #1B3F83; }
            QTextEdit {
                background-color: #F4F7FE;
                border: 1px solid #E9EDF7;
                border-radius: 12px;
                padding: 18px;
                font-family: 'Segoe UI';
                font-size: 13px;
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
        layout->setContentsMargins(30, 26, 30, 24);
        layout->setSpacing(12);

        // Modified: Match invoice viewing with the Reservation dialog system: clear title, supporting subtitle, and an unambiguous close action.
        auto* titleRow = new QHBoxLayout();
        auto* invoiceIcon = new QLabel("▣", this);
        invoiceIcon->setStyleSheet("color:#3B58FF; font-size:17px; background:transparent; border:none;");
        auto* titleLabel = new QLabel("Booking invoice", this);
        titleLabel->setObjectName("titleLabel");
        auto* invoiceClose = new QPushButton(this);
        invoiceClose->setObjectName("invoiceClose");
        styleDialogCloseButton(invoiceClose);
        auto* subtitleLabel = new QLabel("Issued financial record for a completed stay. This screen is read-only.", this);
        subtitleLabel->setObjectName("subtitleLabel");
        subtitleLabel->setWordWrap(true);
        titleRow->addWidget(invoiceIcon);
        titleRow->addSpacing(8);
        titleRow->addWidget(titleLabel);
        titleRow->addStretch();
        titleRow->addWidget(invoiceClose);
        enableDialogHeaderDrag(this, titleLabel);
        layout->addLayout(titleRow);
        layout->addWidget(subtitleLabel);

        auto* textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true);
        textEdit->setHtml(invoiceDetails);
        // Modified: Let the invoice details use the dialog width instead of a narrow receipt-like column, improving visual balance and readability.
        layout->addWidget(textEdit, 1);

        auto* btnConfirm = new QPushButton("Close invoice", this);
        btnConfirm->setObjectName("btnConfirm");
        connect(btnConfirm, &QPushButton::clicked, this, &QDialog::accept);
        connect(invoiceClose, &QPushButton::clicked, this, &QDialog::reject);
        layout->addWidget(btnConfirm, 0, Qt::AlignRight);
    }
};
