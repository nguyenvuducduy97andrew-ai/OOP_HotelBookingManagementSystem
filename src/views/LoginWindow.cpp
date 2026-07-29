#include "LoginWindow.h"

#include "StaffSession.h"

#include <QCryptographicHash>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const QByteArray kAdminPasswordHash = QByteArray::fromHex(
    "d82494f05d6917ba02f7aaa29689ccb444bb73f20380876cb05d1f37537b7892");

QByteArray passwordHash(const QString& password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
}
}

LoginWindow::LoginWindow(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
}

void LoginWindow::setupUi()
{
    setWindowTitle("Hotel Management System — Sign in");
    setModal(true);
    setFixedSize(420, 360);

    setStyleSheet(
        "QDialog { background: #FFFFFF; }"
        "QLabel#title { color: #102A66; font-size: 24px; font-weight: 700; }"
        "QLabel#subtitle { color: #6E86B5; font-size: 13px; }"
        "QLineEdit { border: 1px solid #D8E2F1; border-radius: 9px; padding: 10px 12px; color: #102A66; background: #F7F9FD; }"
        "QLineEdit:focus { border: 2px solid #0B63F6; }"
        "QPushButton { background: #0B63F6; color: white; border: none; border-radius: 9px; font-weight: 700; padding: 11px; }"
        "QPushButton:hover { background: #0854D3; }"
        "QPushButton:disabled { background: #AFC8F8; }");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(38, 30, 38, 30);
    rootLayout->setSpacing(10);

    auto* title = new QLabel("Welcome back", this);
    title->setObjectName("title");
    auto* subtitle = new QLabel("Sign in to continue to hotel operations.", this);
    subtitle->setObjectName("subtitle");
    rootLayout->addWidget(title);
    rootLayout->addWidget(subtitle);
    rootLayout->addSpacing(12);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Enter username");
    m_usernameEdit->setClearButtonEnabled(true);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Enter password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    auto* usernameLabel = new QLabel("Username", this);
    auto* passwordLabel = new QLabel("Password", this);
    usernameLabel->setStyleSheet("color: #102A66; font-weight: 600;");
    passwordLabel->setStyleSheet("color: #102A66; font-weight: 600;");

    // Modified: Stack labels above full-width fields so every sign-in element shares the same visual left and right margins.
    rootLayout->addWidget(usernameLabel);
    rootLayout->addWidget(m_usernameEdit);
    rootLayout->addSpacing(4);
    rootLayout->addWidget(passwordLabel);
    rootLayout->addWidget(m_passwordEdit);

    m_loginButton = new QPushButton("Sign in", this);
    m_loginButton->setDefault(true);
    rootLayout->addSpacing(6);
    rootLayout->addWidget(m_loginButton);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginWindow::attemptLogin);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::attemptLogin);
    connect(m_usernameEdit, &QLineEdit::returnPressed, m_passwordEdit, QOverload<>::of(&QLineEdit::setFocus));
    m_usernameEdit->setFocus();
}

void LoginWindow::attemptLogin()
{
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        setError("Enter both username and password.");
        return;
    }

    // Modified: Authenticate before opening the main window and keep password text out of the source code.
    if (username.compare("admin", Qt::CaseInsensitive) != 0 || passwordHash(password) != kAdminPasswordHash) {
        m_passwordEdit->clear();
        setError("Incorrect username or password.");
        return;
    }

    StaffSession::instance().start("admin", "Administrator", "Administrator");
    accept();
}

void LoginWindow::setError(const QString& message)
{
    QMessageBox errorDialog(QMessageBox::Warning, "Sign-in failed", message, QMessageBox::Ok, this);
    errorDialog.setStyleSheet(
        "QMessageBox { background: #FFFFFF; }"
        "QLabel#qt_msgbox_label { color: #102A66; font-size: 14px; min-width: 260px; }"
        "QPushButton { background: #0B63F6; color: #FFFFFF; border: none; border-radius: 7px; font-weight: 700; padding: 6px 14px; }"
        "QPushButton:hover { background: #0854D3; }");
    errorDialog.button(QMessageBox::Ok)->setFixedSize(78, 34);

    // Modified: Apply an explicit local message-box palette so global styles cannot render error text white on white.
    errorDialog.exec();
}
