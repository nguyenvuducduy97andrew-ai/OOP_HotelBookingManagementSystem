#pragma once

#include <QList>
#include <QRegularExpression>
#include <QString>

// Modified and optimized performance: centralize country-specific ID and phone rules so every form validates identically.
struct CountryInputRule
{
    QString key;
    QString name;
    QString callingCode;
    QString idHint;
    QString phoneHint;
    QRegularExpression idPattern;
    int idMaxLength;
    int phoneDigits;
};

inline const QList<CountryInputRule>& countryInputRules()
{
    static const QList<CountryInputRule> rules = {
        {"VN", "Vietnam", "+84", "12 digits", "9 digits", QRegularExpression(R"(^\d{12}$)"), 12, 9},
        {"US", "United States", "+1", "9 digits", "10 digits", QRegularExpression(R"(^\d{9}$)"), 9, 10},
        {"MY", "Malaysia", "+60", "12 digits", "9 digits", QRegularExpression(R"(^\d{12}$)"), 12, 9},
        // Modified and optimized performance: replace abstract letter/digit abbreviations with compact ID examples that users can recognize immediately.
        {"GB", "United Kingdom", "+44", "e.g. AB123456C", "10 digits", QRegularExpression(R"(^[A-CEGHJ-PR-TW-Z]{2}\d{6}[A-D]$)"), 9, 10},
        {"JP", "Japan", "+81", "12 digits", "10 digits", QRegularExpression(R"(^\d{12}$)"), 12, 10},
        {"SG", "Singapore", "+65", "e.g. S1234567A", "8 digits", QRegularExpression(R"(^[STFGM]\d{7}[A-Z]$)"), 9, 8},
        {"KR", "South Korea", "+82", "13 digits", "10 digits", QRegularExpression(R"(^\d{13}$)"), 13, 10},
        {"TH", "Thailand", "+66", "13 digits", "9 digits", QRegularExpression(R"(^\d{13}$)"), 13, 9},
        {"AU", "Australia", "+61", "10 digits", "9 digits", QRegularExpression(R"(^\d{10}$)"), 10, 9},
        {"DE", "Germany", "+49", "e.g. A1B2C3D4E", "10 digits", QRegularExpression(R"(^[A-Z0-9]{9}$)"), 9, 10}
    };
    return rules;
}

inline const CountryInputRule& countryInputRule(const QString& key)
{
    for (const auto& rule : countryInputRules()) {
        if (rule.key == key) {
            return rule;
        }
    }
    return countryInputRules().first();
}

inline QString normalizeLocalPhoneNumber(QString value)
{
    // Modified and optimized performance: normalize local input once and remove trunk zeroes before composing E.164 storage values.
    value.remove(QRegularExpression(R"([\s\-\(\)])"));
    while (value.startsWith('0')) {
        value.remove(0, 1);
    }
    return value;
}
