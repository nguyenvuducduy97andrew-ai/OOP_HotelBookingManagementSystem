#include "LoginWindow.h"

#include "StaffSession.h"

#include <QAction>
#include <QCryptographicHash>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const QByteArray kAdminPasswordHash = QByteArray::fromHex(
    "d82494f05d6917ba02f7aaa29689ccb444bb73f20380876cb05d1f37537b7892");

QByteArray passwordHash(const QString& password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
}

QIcon passwordVisibilityIcon(bool passwordIsVisible)
{
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#6E86B5"), 1.6));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(2.0, 5.0, 14.0, 8.0));
    painter.setBrush(QColor("#6E86B5"));
    painter.drawEllipse(QRectF(7.0, 7.0, 4.0, 4.0));

    if (!passwordIsVisible) {
        painter.setPen(QPen(QColor("#6E86B5"), 1.8));
        painter.drawLine(QPointF(2.5, 2.5), QPointF(15.5, 15.5));
    }
    return QIcon(pixmap);
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
    auto* passwordVisibilityAction = m_passwordEdit->addAction(
        passwordVisibilityIcon(false), QLineEdit::TrailingPosition);
    passwordVisibilityAction->setToolTip("Show password");
    // Modified: let staff verify a password before submitting without exposing it by default.
    connect(passwordVisibilityAction, &QAction::triggered, this, [this, passwordVisibilityAction] {
        const bool showPassword = m_passwordEdit->echoMode() == QLineEdit::Password;
        m_passwordEdit->setEchoMode(showPassword ? QLineEdit::Normal : QLineEdit::Password);
        passwordVisibilityAction->setIcon(passwordVisibilityIcon(showPassword));
        passwordVisibilityAction->setToolTip(showPassword ? "Hide password" : "Show password");
    });

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
    // Modified: avoid a duplicate sign-in attempt when Return emits both the editor and default-button signals.
    m_loginButton->setAutoDefault(false);
    m_loginButton->setDefault(false);
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
        setError("Incorrect username or password.");
        m_passwordEdit->setFocus();
        m_passwordEdit->selectAll();
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
