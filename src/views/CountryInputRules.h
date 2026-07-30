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
    int phoneMinDigits;
    int phoneMaxDigits;
};

inline const QList<CountryInputRule>& countryInputRules()
{
    static const QList<CountryInputRule> rules = {
        {"VN", "Vietnam", "+84", "12 digits", "9 digits", QRegularExpression(R"(^\d{12}$)"), 12, 9, 9},
        {"US", "United States", "+1", "9 digits", "10 digits", QRegularExpression(R"(^\d{9}$)"), 9, 10, 10},
        {"MY", "Malaysia", "+60", "12 digits", "7–10 digits", QRegularExpression(R"(^\d{12}$)"), 12, 7, 10},
        // Modified and optimized performance: replace abstract letter/digit abbreviations with compact ID examples that users can recognize immediately.
        {"GB", "United Kingdom", "+44", "e.g. AB123456C", "9–10 digits", QRegularExpression(R"(^[A-CEGHJ-PR-TW-Z]{2}\d{6}[A-D]$)"), 9, 9, 10},
        {"JP", "Japan", "+81", "12 digits", "9–10 digits", QRegularExpression(R"(^\d{12}$)"), 12, 9, 10},
        {"SG", "Singapore", "+65", "e.g. S1234567A", "8 digits", QRegularExpression(R"(^[STFGM]\d{7}[A-Z]$)"), 9, 8, 8},
        {"KR", "South Korea", "+82", "13 digits", "9–10 digits", QRegularExpression(R"(^\d{13}$)"), 13, 9, 10},
        {"TH", "Thailand", "+66", "13 digits", "8–9 digits", QRegularExpression(R"(^\d{13}$)"), 13, 8, 9},
        {"AU", "Australia", "+61", "10 digits", "9 digits", QRegularExpression(R"(^\d{10}$)"), 10, 9, 9},
        {"DE", "Germany", "+49", "e.g. A1B2C3D4E", "5–11 digits", QRegularExpression(R"(^[A-Z0-9]{9}$)"), 9, 5, 11}
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

inline bool isValidDocumentNumber(const QString& documentType, const QString& countryKey, const QString& number)
{
    const QString normalized = number.trimmed().toUpper();
    if (documentType.compare("Passport", Qt::CaseInsensitive) == 0) {
        return QRegularExpression(QStringLiteral(R"(^[A-Z0-9]{6,20}$)")).match(normalized).hasMatch();
    }
    if (documentType.compare("Other", Qt::CaseInsensitive) == 0) {
        return QRegularExpression(QStringLiteral(R"(^[A-Z0-9-]{3,30}$)")).match(normalized).hasMatch();
    }
    return countryInputRule(countryKey).idPattern.match(normalized).hasMatch();
}

inline QString documentNumberHint(const QString& documentType, const QString& countryKey)
{
    if (documentType.compare("Passport", Qt::CaseInsensitive) == 0) return QStringLiteral("6–20 letters or digits");
    if (documentType.compare("Other", Qt::CaseInsensitive) == 0) return QStringLiteral("3–30 letters, digits, or hyphens");
    return countryInputRule(countryKey).idHint;
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
