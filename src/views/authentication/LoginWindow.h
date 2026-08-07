#pragma once

#include <QDialog>

class QLineEdit;
class QPushButton;

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

private slots:
    void attemptLogin();

private:
    void setupUi();
    void setError(const QString& message);

    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QPushButton* m_loginButton = nullptr;
};
