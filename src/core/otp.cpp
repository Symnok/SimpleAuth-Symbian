// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "otp.h"

#include <QCoreApplication>
#include <QStringList>
#include <QUrl>

namespace
{
    const char Base32Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    QString tr(const char *text) { return QCoreApplication::translate("Otp", text); }

    /// otpauth links percent-encode the label and values; some writers use '+' for spaces.
    QString decodeComponent(const QString &text)
    {
        QByteArray bytes = text.toUtf8();
        bytes.replace('+', ' ');
        return QUrl::fromPercentEncoding(bytes);
    }

    QString encodeComponent(const QString &text)
    {
        return QString::fromLatin1(QUrl::toPercentEncoding(text, "@"));
    }
}

QByteArray Otp::base32Decode(const QString &text, bool *ok)
{
    QByteArray out;
    quint32 buffer = 0;
    int bits = 0;
    int symbols = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i).toUpper();
        if (c == QLatin1Char(' ') || c == QLatin1Char('-') || c == QLatin1Char('=') || c == QLatin1Char('\t')) continue;
        int value;
        if (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) value = c.unicode() - 'A';
        else if (c >= QLatin1Char('2') && c <= QLatin1Char('7')) value = c.unicode() - '2' + 26;
        else {
            if (ok) *ok = false;
            return QByteArray();
        }
        buffer = (buffer << 5) | quint32(value);
        bits += 5;
        ++symbols;
        if (bits >= 8) {
            bits -= 8;
            out.append(char((buffer >> bits) & 0xff));
        }
    }
    if (ok) *ok = symbols > 0;
    return out;
}

QString Otp::base32Encode(const QByteArray &data)
{
    QString out;
    quint32 buffer = 0;
    int bits = 0;
    for (int i = 0; i < data.size(); ++i) {
        buffer = (buffer << 8) | uchar(data.at(i));
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            out.append(QLatin1Char(Base32Alphabet[(buffer >> bits) & 31]));
        }
    }
    if (bits > 0) out.append(QLatin1Char(Base32Alphabet[(buffer << (5 - bits)) & 31]));
    return out;
}

QString Otp::hotp(const QByteArray &secret, quint64 counter, int digits, Hash::Algorithm algorithm)
{
    QByteArray message(8, '\0');
    for (int i = 7; i >= 0; --i) {
        message[i] = char(counter & 0xff);
        counter >>= 8;
    }
    const QByteArray mac = Hash::hmac(algorithm, secret, message);
    // Dynamic truncation (RFC 4226 5.3): the low nibble of the last byte picks 4 bytes.
    const int offset = uchar(mac.at(mac.size() - 1)) & 0x0f;
    const quint32 binary = (quint32(uchar(mac.at(offset)) & 0x7f) << 24)
                         | (quint32(uchar(mac.at(offset + 1))) << 16)
                         | (quint32(uchar(mac.at(offset + 2))) << 8)
                         |  quint32(uchar(mac.at(offset + 3)));
    quint32 modulus = 1;
    for (int i = 0; i < digits; ++i) modulus *= 10;
    return QString::number(binary % modulus).rightJustified(digits, QLatin1Char('0'));
}

QString Otp::totp(const QByteArray &secret, quint64 unixTime, int period, int digits, Hash::Algorithm algorithm)
{
    return hotp(secret, unixTime / quint64(period > 0 ? period : 30), digits, algorithm);
}

QString Otp::code(const OtpAccount &account, quint64 unixTime)
{
    if (account.hotp) return hotp(account.secret, account.counter, account.digits, account.algorithm);
    return totp(account.secret, unixTime, account.period, account.digits, account.algorithm);
}

QString Otp::validate(const OtpAccount &account)
{
    if (account.secret.isEmpty()) return tr("The secret key is empty or not valid Base32 (letters A-Z and digits 2-7).");
    if (account.digits < 6 || account.digits > 8) return tr("The code length must be 6, 7 or 8 digits.");
    if (!account.hotp && (account.period < 1 || account.period > 3600)) return tr("The period must be between 1 and 3600 seconds.");
    if (account.issuer.trimmed().isEmpty() && account.name.trimmed().isEmpty()) return tr("Enter a name for the account.");
    return QString();
}

bool Otp::parseUri(const QString &uri, OtpAccount *account, QString *error)
{
    const QString text = uri.trimmed();
    const QString scheme = QString::fromLatin1("otpauth://");
    if (!text.startsWith(scheme, Qt::CaseInsensitive)) {
        *error = tr("Not an otpauth:// link.");
        return false;
    }
    QString rest = text.mid(scheme.size());
    QString query;
    const int q = rest.indexOf(QLatin1Char('?'));
    if (q >= 0) {
        query = rest.mid(q + 1);
        rest = rest.left(q);
    }
    const int slash = rest.indexOf(QLatin1Char('/'));
    const QString type = (slash >= 0 ? rest.left(slash) : rest).toLower();
    const QString label = slash >= 0 ? decodeComponent(rest.mid(slash + 1)) : QString();

    OtpAccount a;
    if (type == QLatin1String("totp")) a.hotp = false;
    else if (type == QLatin1String("hotp")) a.hotp = true;
    else {
        *error = tr("Unknown one-time password type \"%1\".").arg(type);
        return false;
    }

    // The label is "Issuer:account" or just "account"; the issuer parameter wins if both.
    const int colon = label.indexOf(QLatin1Char(':'));
    if (colon >= 0) {
        a.issuer = label.left(colon).trimmed();
        a.name = label.mid(colon + 1).trimmed();
    } else {
        a.name = label.trimmed();
    }

    bool haveSecret = false, haveCounter = false;
    const QStringList pairs = query.split(QLatin1Char('&'), QString::SkipEmptyParts);
    for (int i = 0; i < pairs.size(); ++i) {
        const int eq = pairs.at(i).indexOf(QLatin1Char('='));
        const QString key = decodeComponent(eq >= 0 ? pairs.at(i).left(eq) : pairs.at(i)).toLower();
        const QString value = eq >= 0 ? decodeComponent(pairs.at(i).mid(eq + 1)) : QString();
        bool ok = true;
        if (key == QLatin1String("secret")) {
            a.secret = base32Decode(value, &ok);
            if (!ok) {
                *error = tr("The link's secret is not valid Base32.");
                return false;
            }
            haveSecret = true;
        } else if (key == QLatin1String("issuer")) {
            if (!value.trimmed().isEmpty()) a.issuer = value.trimmed();
        } else if (key == QLatin1String("algorithm")) {
            if (!Hash::fromName(value, &a.algorithm)) {
                *error = tr("Unsupported algorithm \"%1\".").arg(value);
                return false;
            }
        } else if (key == QLatin1String("digits")) {
            a.digits = value.toInt(&ok);
        } else if (key == QLatin1String("period")) {
            a.period = value.toInt(&ok);
        } else if (key == QLatin1String("counter")) {
            a.counter = value.toULongLong(&ok);
            haveCounter = ok;
        }
        if (!ok) {
            *error = tr("The link's %1 value \"%2\" is not a number.").arg(key, value);
            return false;
        }
    }
    // An issuer given as a parameter may itself hold a colon: the label then reads
    // "Iss:uer:name", and the name is what follows the whole issuer.
    if (!a.issuer.isEmpty() && label.startsWith(a.issuer + QLatin1Char(':')))
        a.name = label.mid(a.issuer.size() + 1).trimmed();
    if (!haveSecret) {
        *error = tr("The link has no secret.");
        return false;
    }
    if (a.hotp && !haveCounter) {
        *error = tr("The HOTP link has no counter.");
        return false;
    }
    const QString problem = validate(a);
    if (!problem.isEmpty()) {
        *error = problem;
        return false;
    }
    *account = a;
    return true;
}

QString Otp::toUri(const OtpAccount &account)
{
    // Always an "issuer:" prefix, empty if there is none, so a colon inside the name is never
    // taken for the separator when the link is read back.
    QString label = encodeComponent(account.issuer) + QLatin1Char(':') + encodeComponent(account.name);
    if (account.issuer.isEmpty() && !account.name.contains(QLatin1Char(':'))) label = encodeComponent(account.name);
    QString uri = QString::fromLatin1("otpauth://%1/%2?secret=%3")
                      .arg(QLatin1String(account.hotp ? "hotp" : "totp"), label, base32Encode(account.secret));
    if (!account.issuer.isEmpty()) uri += QLatin1String("&issuer=") + encodeComponent(account.issuer);
    if (account.algorithm != Hash::Sha1) uri += QLatin1String("&algorithm=") + Hash::name(account.algorithm);
    if (account.digits != 6) uri += QLatin1String("&digits=") + QString::number(account.digits);
    if (account.hotp) uri += QLatin1String("&counter=") + QString::number(account.counter);
    else if (account.period != 30) uri += QLatin1String("&period=") + QString::number(account.period);
    return uri;
}
