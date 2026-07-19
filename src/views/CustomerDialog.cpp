#include "CustomerDialog.h"
#include "Customer.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFormLayout>

CustomerDialog::CustomerDialog(QWidget* parent)
    : QDialog(parent), m_customer(nullptr)
{
    setupStyle();
    setupUI();
    setWindowTitle("Add Customer");
}

CustomerDialog::CustomerDialog(const std::shared_ptr<Customer>& customer, QWidget* parent)
    : QDialog(parent), m_customer(customer)
{
    setupStyle();
    setupUI();
    setWindowTitle("Edit Customer");
    populateFields();
    if (m_customer) {
        m_idEdit->setEnabled(false);
    }
}

void CustomerDialog::setupStyle() {
    // Match CustomConfirmDialog's window conventions
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(420, 380);

    setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
        }
        QLabel {
            font-size: 13px;
            color: #2B3674;
            font-weight: 600;
        }
        QLabel#dialogTitle {
            font-size: 18px;
            font-weight: 800;
            color: #2B3674;
        }
        QLabel#dialogDescription {
            font-size: 12px;
            font-weight: 500;
            color: #A3AED0;
            padding-bottom: 4px;
        }
        QLineEdit {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            font-weight: 500;
            color: #2B3674;
        }
        QLineEdit:focus {
            border: 1px solid #005BFE;
        }
        QLineEdit:disabled {
            background-color: #F1F5F9;
            color: #A3AED0;
        }
        QPushButton#btnSave {
            background-color: #005BFE;
            color: #FFFFFF;
            font-weight: 700;
            border-radius: 8px;
            padding: 8px 22px;
            border: none;
            font-size: 13px;
            min-width: 90px;
        }
        QPushButton#btnSave:hover {
            background-color: #2B7BFF;
        }
        QPushButton#btnCancelDialog {
            background-color: #E9EDF7;
            color: #2B3674;
            font-weight: 700;
            border-radius: 8px;
            padding: 8px 22px;
            border: none;
            font-size: 13px;
            min-width: 90px;
        }
        QPushButton#btnCancelDialog:hover {
            background-color: #D3DDF4;
        }
    )");
}

void CustomerDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(12);

    auto* titleLabel = new QLabel(m_customer ? "Edit customer details" : "Add a new customer", this);
    titleLabel->setObjectName("dialogTitle");
    mainLayout->addWidget(titleLabel);

    auto* descriptionLabel = new QLabel("Enter customer information below. Customer ID cannot be changed after creation.", this);
    descriptionLabel->setObjectName("dialogDescription");
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignLeft);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(14);

    m_idEdit = new QLineEdit(this);
    m_idEdit->setPlaceholderText("e.g. CUST001");
    formLayout->addRow("Customer ID:", m_idEdit);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Full name");
    formLayout->addRow("Name:", m_nameEdit);

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText("Phone number");
    formLayout->addRow("Phone:", m_phoneEdit);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    m_okButton = new QPushButton("Save", this);
    m_okButton->setObjectName("btnSave");
    auto* cancelButton = new QPushButton("Cancel", this);
    cancelButton->setObjectName("btnCancelDialog");
    buttonRow->addWidget(cancelButton);
    buttonRow->addWidget(m_okButton);
    mainLayout->addLayout(buttonRow);

    connect(m_okButton, &QPushButton::clicked, this, &CustomerDialog::onAccept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void CustomerDialog::populateFields() {
    if (!m_customer)
        return;

    m_idEdit->setText(QString::fromStdString(m_customer->getCustomerId()));
    m_nameEdit->setText(QString::fromStdString(m_customer->getName()));
    m_phoneEdit->setText(QString::fromStdString(m_customer->getPhoneNumber()));
}

QString CustomerDialog::getCustomerId() const {
    return m_idEdit->text().trimmed();
}

QString CustomerDialog::getCustomerName() const {
    return m_nameEdit->text().trimmed();
}

QString CustomerDialog::getCustomerPhone() const {
    return m_phoneEdit->text().trimmed();
}

void CustomerDialog::onAccept() {
    if (m_idEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation error", "Customer ID is required.");
        return;
    }
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation error", "Customer name is required.");
        return;
    }
    if (m_phoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation error", "Customer phone number is required.");
        return;
    }
    accept();
}