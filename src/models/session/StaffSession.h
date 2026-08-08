#pragma once

#include <string>

// Stores the authenticated staff identity for the lifetime of the application process.
class StaffSession
{
public:
    static StaffSession& instance();

    void start(const std::string& username, const std::string& displayName, const std::string& role);
    void clear();

    bool isAuthenticated() const;
    const std::string& username() const;
    const std::string& displayName() const;
    const std::string& role() const;

    StaffSession(const StaffSession&) = delete;
    StaffSession& operator=(const StaffSession&) = delete;
    StaffSession(StaffSession&&) = delete;
    StaffSession& operator=(StaffSession&&) = delete;

private:
    StaffSession() = default;

    bool m_authenticated = false;
    std::string m_username;
    std::string m_displayName;
    std::string m_role;
};
