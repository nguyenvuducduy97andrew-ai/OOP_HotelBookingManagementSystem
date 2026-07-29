#include "StaffSession.h"

StaffSession& StaffSession::instance()
{
    static StaffSession session;
    return session;
}

void StaffSession::start(const QString& username, const QString& displayName, const QString& role)
{
    // Modified: Keep a single authenticated staff context so later audit records use the real signed-in operator.
    m_authenticated = true;
    m_username = username;
    m_displayName = displayName;
    m_role = role;
}

void StaffSession::clear()
{
    m_authenticated = false;
    m_username.clear();
    m_displayName.clear();
    m_role.clear();
}

bool StaffSession::isAuthenticated() const
{
    return m_authenticated;
}

QString StaffSession::username() const
{
    return m_username;
}

QString StaffSession::displayName() const
{
    return m_displayName;
}

QString StaffSession::role() const
{
    return m_role;
}
