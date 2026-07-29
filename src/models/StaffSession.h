#pragma once

#include <QString>

// Stores the authenticated staff identity for the lifetime of the application process.
class StaffSession
{
public:
    static StaffSession& instance();

    void start(const QString& username, const QString& displayName, const QString& role);
    void clear();

    bool isAuthenticated() const;
    QString username() const;
    QString displayName() const;
    QString role() const;

private:
    StaffSession() = default;

    bool m_authenticated = false;
    QString m_username;
    QString m_displayName;
    QString m_role;
};
