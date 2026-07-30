#pragma once

#include <QString>

inline QString customerIdentityKey(const QString& documentType, const QString& issuingCountry, const QString& documentNumber)
{
    return QStringLiteral("%1|%2|%3")
        .arg(documentType.trimmed(), issuingCountry.trimmed().toUpper(), documentNumber.trimmed().toUpper());
}
