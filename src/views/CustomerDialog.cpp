#include "CustomerDialog.h"
#include "Customer.h"
#include "CustomerIdentity.h"
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
        m_documentType->setEnabled(false);
        m_idCountry->setEnabled(false);
    }
}

void CustomerDialog::setupStyle() {
    // Match CustomConfirmDialog's window conventions
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    // Modified: Reduce dialog height after replacing the two-part name form with one legal-name field.
    // Modified: Widen the customer dialog so document type, issuing country, and number remain readable on one row.
    setFixedSize(540, 500);

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

    auto* descriptionLabel = new QLabel("Choose the ID and phone countries, then enter the customer's legal name and matching details.", this);
    descriptionLabel->setObjectName("dialogDescription");
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignLeft);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(14);

    // Modified: Collect document type and issuing country so foreign guest identity is not reduced to one global national-ID field.
    auto* idRow = new QHBoxLayout();
    idRow->setSpacing(8);
    m_documentType = new QComboBox(this);
    m_documentType->addItems({"National ID", "Passport", "Other"});
    m_idCountry = new QComboBox(this);
    for (const auto& rule : countryInputRules()) {
        m_idCountry->addItem(rule.name, rule.key);
    }
    m_idEdit = new QLineEdit(this);
    idRow->addWidget(m_documentType, 0);
    idRow->addWidget(m_idCountry, 0);
    idRow->addWidget(m_idEdit, 1);
    formLayout->addRow("Identity document:", idRow);

    // Modified: Capture the legal name as one field so international names are not forced into surname and given-name parts.
    m_fullNameEdit = new QLineEdit(this);
    m_fullNameEdit->setPlaceholderText("Full legal name as shown on identification");
    formLayout->addRow("Full legal name:", m_fullNameEdit);

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
    connect(m_documentType, &QComboBox::currentIndexChanged, this, [this]() { updateIdPlaceholder(); });
    connect(m_idCountry, &QComboBox::currentIndexChanged, this, [this]() { updateIdPlaceholder(); });
    connect(m_idEdit, &QLineEdit::textChanged, this, [this]() { setFieldError(m_idEdit, false); });
    connect(m_fullNameEdit, &QLineEdit::textChanged, this, [this]() { setFieldError(m_fullNameEdit, false); });
    connect(m_phoneCountryCode, &QComboBox::currentIndexChanged, this, [this]() { updatePhonePlaceholder(); normalizePhoneInput(); });
    connect(m_phoneLocalEdit, &QLineEdit::textChanged, this, [this]() { normalizePhoneInput(); setFieldError(m_phoneLocalEdit, false); });
}

void CustomerDialog::populateFields() {
    if (!m_customer)
        return;

    const QString existingId = QString::fromStdString(m_customer->getDocumentNumber()).trimmed().toUpper();
    const QString existingType = QString::fromStdString(m_customer->getDocumentType());
    const QString existingCountry = QString::fromStdString(m_customer->getIssuingCountry());
    const int typeIndex = m_documentType->findText(existingType);
    if (typeIndex >= 0) {
        m_documentType->setCurrentIndex(typeIndex);
    }
    for (int i = 0; i < m_idCountry->count(); ++i) {
        if (m_idCountry->itemData(i).toString() == existingCountry) {
            m_idCountry->setCurrentIndex(i);
            break;
        }
    }
    m_idEdit->setText(existingId);

    // Modified: Preserve the stored legal name exactly instead of attempting culturally unsafe name-part splitting.
    m_fullNameEdit->setText(QString::fromStdString(m_customer->getName()).simplified());

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
    if (m_customer) {
        return QString::fromStdString(m_customer->getCustomerId());
    }
    return customerIdentityKey(getDocumentType(), getIssuingCountry(), getDocumentNumber());
}

QString CustomerDialog::getDocumentType() const { return m_documentType->currentText(); }
QString CustomerDialog::getIssuingCountry() const { return m_idCountry->currentData().toString(); }
QString CustomerDialog::getDocumentNumber() const { return m_idEdit->text().trimmed().toUpper(); }

QString CustomerDialog::getCustomerName() const {
    return m_fullNameEdit->text().simplified();
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
    const QString documentType = m_documentType->currentText();
    const auto& rule = countryInputRule(m_idCountry->currentData().toString());
    m_idEdit->setPlaceholderText(documentNumberHint(documentType, rule.key));
    m_idEdit->setMaxLength(documentType.compare("Passport", Qt::CaseInsensitive) == 0 ? 20
        : (documentType.compare("Other", Qt::CaseInsensitive) == 0 ? 30 : rule.idMaxLength));
}

void CustomerDialog::updatePhonePlaceholder()
{
    const auto& rule = countryInputRule(m_phoneCountryCode->currentData().toString());
    m_phoneLocalEdit->setPlaceholderText(rule.phoneHint);
    m_phoneLocalEdit->setMaxLength(rule.phoneMaxDigits + 1);
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
    setFieldError(m_fullNameEdit, false);
    setFieldError(m_phoneLocalEdit, false);

    QStringList errors;
    QLineEdit* firstInvalid = nullptr;

    const auto& idRule = countryInputRule(m_idCountry->currentData().toString());
    const QString documentType = getDocumentType();
    const auto& phoneRule = countryInputRule(m_phoneCountryCode->currentData().toString());
    const QString idText = m_idEdit->text().trimmed().toUpper();
    const QString fullName = m_fullNameEdit->text().simplified();
    const QString phoneLocal = normalizeLocalPhoneNumber(m_phoneLocalEdit->text());
    const QString fullPhone = phoneRule.callingCode + phoneLocal;

    m_idEdit->setText(idText);
    m_fullNameEdit->setText(fullName);
    m_phoneLocalEdit->setText(phoneLocal);

    if (!m_customer && !isValidDocumentNumber(documentType, idRule.key, idText)) {
        errors << QString("%1 for %2 must be %3.").arg(documentType, idRule.name, documentNumberHint(documentType, idRule.key));
        setFieldError(m_idEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_idEdit;
    }

    if (fullName.isEmpty()) {
        errors << "Full legal name is required.";
        setFieldError(m_fullNameEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_fullNameEdit;
    }

    if (!fullName.isEmpty() && !HotelManager::isValidCustomerNameFormat(fullName.toStdString())) {
        errors << "Enter a valid legal name using letters, spaces, apostrophes, hyphens, or initials.";
        setFieldError(m_fullNameEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_fullNameEdit;
    }

    static const QRegularExpression digitsPattern(QStringLiteral(R"(^\d+$)"));
    if (!digitsPattern.match(phoneLocal).hasMatch()) {
        errors << "Phone number must contain digits only.";
        setFieldError(m_phoneLocalEdit, true);
        firstInvalid = firstInvalid ? firstInvalid : m_phoneLocalEdit;
    }

    if (phoneLocal.size() < phoneRule.phoneMinDigits || phoneLocal.size() > phoneRule.phoneMaxDigits) {
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
