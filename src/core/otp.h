// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// One-time passwords as Google Authenticator makes them: HOTP (RFC 4226), TOTP (RFC 6238),
// Base32 secrets (RFC 4648) and otpauth:// links (the format inside the QR codes sites show).
#ifndef OTP_H
#define OTP_H

#include "hash.h"

#include <QByteArray>
#include <QString>

struct OtpAccount
{
    QString issuer;             ///< the service, e.g. "Google"
    QString name;               ///< the account at that service, e.g. "alice@gmail.com"
    QByteArray secret;          ///< the decoded key
    bool hotp;                  ///< counter-based instead of time-based
    Hash::Algorithm algorithm;
    int digits;                 ///< 6..8
    int period;                 ///< TOTP step in seconds
    quint64 counter;            ///< HOTP counter: the code shown is for this value

    OtpAccount() : hotp(false), algorithm(Hash::Sha1), digits(6), period(30), counter(0) {}
};

namespace Otp
{
    /// Decodes a Base32 secret as sites print it: any case, spaces, dashes and '=' padding
    /// are ignored. Returns an empty array (and *ok false) on anything else.
    QByteArray base32Decode(const QString &text, bool *ok = 0);
    QString base32Encode(const QByteArray &data);

    /// The code for one counter value, zero-padded to digits.
    QString hotp(const QByteArray &secret, quint64 counter, int digits, Hash::Algorithm algorithm);
    /// The code for a Unix time (seconds, UTC).
    QString totp(const QByteArray &secret, quint64 unixTime, int period, int digits, Hash::Algorithm algorithm);

    /// The account's current code: TOTP at unixTime, or HOTP at its counter.
    QString code(const OtpAccount &account, quint64 unixTime);

    /// Checks an account's fields; returns an empty string when it is usable, or the reason.
    QString validate(const OtpAccount &account);

    /// Parses otpauth://totp/Issuer:name?secret=...&issuer=...&algorithm=...&digits=...&period=...
    /// (and otpauth://hotp/...&counter=...). Returns false with a reason in *error.
    bool parseUri(const QString &uri, OtpAccount *account, QString *error);
    /// The link for an account, for the self test and a future "show QR".
    QString toUri(const OtpAccount &account);
}

#endif // OTP_H
