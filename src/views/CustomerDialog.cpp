#include "CustomerDialog.h"
#include "Customer.h"
#include "CountryInputRules.h"
#include "HotelManager.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStringList>
#include <QRegularExpression>
#include <QStyle>

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
    setFixedSize(460, 560);

    // Modified and optimized performance: explicitly style the country selector popup so its item text remains visible on every platform theme.
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
        QComboBox {
            background-color: #F4F7FE;
            border: 1px solid #E9EDF7;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            font-weight: 500;
            color: #2B3674;
        }
        QComboBox:focus {
            border: 1px solid #005BFE;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            color: #2B3674;
            border: 1px solid #D9E2F2;
            outline: 0;
            padding: 4px;
            selection-background-color: #E6F0FF;
            selection-color: #005BFE;
        }
        QComboBox QAbstractItemView::item {
            min-height: 30px;
            padding: 4px 10px;
            color: #2B3674;
        }
        QComboBox QAbstractItemView::item:hover,
        QComboBox QAbstractItemView::item:selected {
            background-color: #E6F0FF;
            color: #005BFE;
        }
        QLineEdit:disabled {
            background-color: #F1F5F9;
            color: #A3AED0;
        }
        QLineEdit[error="true"] {
            border: 1px solid #EF4444;
            background-color: #FEF2F2;
        }
        QLabel#errorLabel {
            font-size: 12px;
            font-weight: 600;
            color: #EF4444;
            background-color: #FEF2F2;
            border: 1px solid #FECACA;
            border-radius: 8px;
            padding: 8px 10px;
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

    auto* descriptionLabel = new QLabel("Choose the ID and phone countries, then enter the matching customer details.", this);
    descriptionLabel->setObjectName("dialogDescription");
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignLeft);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(14);

    // Modified and optimized performance: select the ID country before applying its compact input rule.
    auto* idRow = new QHBoxLayout();
    idRow->setSpacing(8);
    m_idCountry = new QComboBox(this);
    for (const auto& rule : countryInputRules()) {
        m_idCountry->addItem(rule.name, rule.key);
    }
    m_idEdit = new QLineEdit(this);
    idRow->addWidget(m_idCountry, 0);
    idRow->addWidget(m_idEdit, 1);
    formLayout->addRow("Customer ID:", idRow);

    m_lastNameEdit = new QLineEdit(this);
    m_lastNameEdit->setPlaceholderText("Surname / middle name");
    formLayout->addRow("Last name:", m_lastNameEdit);

    m_firstNameEdit = new QLineEdit(this);
    m_firstNameEdit->setPlaceholderText("Given name");
    formLayout->addRow("First name:", m_firstNameEdit);

    // Modified and optimized performance: reuse the same country catalogue for normalized phone input.
    auto* phoneRow = new QHBoxLayout();
    phoneRow->setSpacing(8);
    m_phoneCountryCode = new QComboBox(this);
    for (const auto& rule : countryInputRules()) {
        m_phoneCountryCode->addItem(QString("%1 (%2)").arg(rule.name, rule.callingCode), rule.key);
    }
    m_phoneLocalEdit = new QLineEdit(this);
    phoneRow->addWidget(m_phoneCountryCode, 0);
    phoneRow->addWidget(m_phoneLocalEdit, 1);
    formLayout->addRow("Phone:", phoneRow);

    updateIdPlaceholder();
    updatePhonePlaceholder();

    mainLayout->addLayout(formLayout);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);

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

    // Clear a field's red border as soon as the user edits it; re-validated on next Save.
    connect(m_idCountry, &QComboBox::currentIndexChanged, this, [this]() { updateIdPlaceholder(); });
    connect(m_idEdit, &QLineEdit::textChanged, this, [this]() { setFieldError(m_idEdit, false); });
    connect(m_lastNameEdit, &QLineEdit::textChanged, this, [this]() { setFieldError(m_lastNameEdit, false); });
    connect(m_firstNameEdit, &QLineEdit::textChanged, this, [this]() { setFieldError(m_firstNameEdit, false); });
    connect(m_phoneCountryCode, &QComboBox::currentIndexChanged, this, [this]() { updatePhonePlaceholder(); normalizePhoneInput(); });
    connect(m_phoneLocalEdit, &QLineEdit::textChanged, this, [this]() { normalizePhoneInput(); setFieldError(m_phoneLocalEdit, false); });
}

void CustomerDialog::populateFields() {
    if (!m_customer)
        return;

    const QString existingId = QString::fromStdString(m_customer->getCustomerId()).trimmed().toUpper();
    for (int i = 0; i < m_idCountry->count(); ++i) {
        const auto& rule = countryInputRule(m_idCountry->itemData(i).toString());
        if (rule.idPattern.match(existingId).hasMatch()) {
            m_idCountry->setCurrentIndex(i);
            break;
        }
    }
    m_idEdit->setText(existingId);

    const QString existingName = QString::fromStdString(m_customer->getName()).simplified();
    const QStringList nameParts = existingName.split(' ', Qt::SkipEmptyParts);
    if (nameParts.size() >= 2) {
        m_firstNameEdit->setText(nameParts.last());
        m_lastNameEdit->setText(nameParts.mid(0, nameParts.size() - 1).join(' '));
    } else {
        m_lastNameEdit->setText(existingName);
        m_firstNameEdit->clear();
    }

    const QString existingPhone = QString::fromStdString(m_customer->getPhoneNumber()).trimmed();
    bool matchedCode = false;
    for (int i = 0; i < m_phoneCountryCode->count(); ++i) {
        const auto& rule = countryInputRule(m_phoneCountryCode->itemData(i).toString());
        if (existingPhone.startsWith(rule.callingCode)) {
            m_phoneCountryCode->setCurrentIndex(i);
            m_phoneLocalEdit->setText(existingPhone.mid(rule.callingCode.size()));
            matchedCode = true;
            break;
        }
    }

    if (!matchedCode) {
        m_phoneLocalEdit->setText(existingPhone);
    }
}

QString CustomerDialog::getCustomerId() const {
    return m_idEdit->text().trimmed().toUpper();
}

QString CustomerDialog::getCustomerName() const {
    return QString("%1 %2").arg(m_lastNameEdit->text().trimmed(), m_firstNameEdit->text().trimmed()).simplified();
}

QString CustomerDialog::getCustomerPhone() const {
    const auto& rule = countryInputRule(m_phoneCountryCode->currentData().toString());
    return rule.callingCode + normalizeLocalPhoneNumber(m_phoneLocalEdit->text());
}

void CustomerDialog::setFieldError(QLineEdit* field, bool hasError) {
    field->setProperty("error", hasError);
    field->style()->unpolish(field);
    field->style()->polish(field);
}

void CustomerDialog::updateIdPlaceholder()
{
    const auto& rule = countryInputRule(m_idCountry->currentData().toString());
    m_idEdit->setPlaceholderText(rule.idHint);
    m_idEdit->setMaxLength(rule.idMaxLength);
}

void CustomerDialog::updatePhonePlaceholder()
{
    const auto& rule = countryInputRule(m_phoneCountryCode->currentData().toString());
    m_phoneLocalEdit->setPlaceholderText(rule.phoneHint);
    m_phoneLocalEdit->setMaxLength(rule.phoneDigits + 1);
}

void CustomerDialog::normalizePhoneInput()
{
    // Modified and optimized performance: remove formatting and accidental leading zeroes while the user types.
    const QString normalized = normalizeLocalPhoneNumber(m_phoneLocalEdit->text());
    if (normalized != m_phoneLocalEdit->text()) {
        m_phoneLocalEdit->setText(normalized);
    }
}

bool CustomerDialog::validate() {
    setFieldError(m_idEdit, false);
    setFieldError(m_lastNameEdit, false);
    setFieldError(m_firstNameEdit, false);
    setFieldError(m_phoneLocalEdit, false);

    QStringList errors;
    QLineEdit* firstInvalid = nullptr;

    const auto& idRule = countryInputRule(m_idCountry->currentData().toString());
    const auto& phoneRule = countryInputRule(m_phoneCountryCode->currentData().toString());
    const QString idText = m_idEdit->text().trimmed().toUpper();
    const QString lastName = m_lastNameEdit->text().trimmed();
    const QString firstName = m_firstNameEdit->text().trimmed();
    const QString phoneLocal = normalizeLocalPhoneNumber(m_phoneLocalEdit->text());
    const QString fullName = QString("%1 %2").arg(lastName, firstName).simplified();
    const QString fullPhone = phoneRule.callingCode + phoneLocal;

    m_idEdit->setText(idText);
    m_phoneLocalEdit->setText(phoneLocal);

    if (!idRule.idPattern.match(idText).hasMatch()) {
        errors << QString("Customer ID for %1 must be %2.").arg(idRule.name, idRule.idHint);
        setFieldError(m_idEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_idEdit;
    }

    if (lastName.isEmpty()) {
        errors << "Last name is required.";
        setFieldError(m_lastNameEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_lastNameEdit;
    }

    if (firstName.isEmpty()) {
        errors << "First name is required.";
        setFieldError(m_firstNameEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_firstNameEdit;
    }

    if (!HotelManager::isValidCustomerNameFormat(fullName.toStdString())) {
        errors << "Customer name must contain at least 2 words and include both uppercase and lowercase letters.";
        setFieldError(m_lastNameEdit, true);
        setFieldError(m_firstNameEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_firstNameEdit;
    }

    static const QRegularExpression digitsPattern(QStringLiteral(R"(^\d+$)"));
    if (!digitsPattern.match(phoneLocal).hasMatch()) {
        errors << "Phone number must contain digits only.";
        setFieldError(m_phoneLocalEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_phoneLocalEdit;
    }

    if (phoneLocal.size() != phoneRule.phoneDigits) {
        errors << QString("Phone number for %1 must be %2.").arg(phoneRule.name, phoneRule.phoneHint);
        setFieldError(m_phoneLocalEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_phoneLocalEdit;
    }

    if (!HotelManager::isValidPhoneNumberFormat(fullPhone.toStdString())) {
        errors << "Phone number format is invalid.";
        setFieldError(m_phoneLocalEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_phoneLocalEdit;
    }

    if (!errors.isEmpty()) {
        m_errorLabel->setText(errors.join("\n"));
        m_errorLabel->setVisible(true);
        if (!firstInvalid) {
            firstInvalid = m_idEdit;
        }
        firstInvalid->setFocus();
        firstInvalid->selectAll();
        return false;
    }

    m_errorLabel->clear();
    m_errorLabel->setVisible(false);
    return true;
}

void CustomerDialog::onAccept() {
    if (!validate()) {
        return;
    }
    accept();
}
