#pragma once
#include <memory>
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

class Customer;
class QLabel;

class CustomerDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomerDialog(QWidget* parent = nullptr);
    explicit CustomerDialog(const std::shared_ptr<Customer>& customer, QWidget* parent = nullptr);

    QString getCustomerId() const;
    QString getCustomerName() const;
    QString getCustomerPhone() const;

private slots:
    void onAccept();

private:
    void setupUI();
    void setupStyle();
    void populateFields();
    bool validate();
    void setFieldError(QLineEdit* field, bool hasError);
    void updateIdPlaceholder();
    void updatePhonePlaceholder();
    void normalizePhoneInput();

    std::shared_ptr<Customer> m_customer;
    QComboBox* m_idCountry;
    QLineEdit* m_idEdit;
    QLineEdit* m_lastNameEdit;
    QLineEdit* m_firstNameEdit;
    QComboBox* m_phoneCountryCode;
    QLineEdit* m_phoneLocalEdit;
    QPushButton* m_okButton;
    QLabel* m_errorLabel;
};
