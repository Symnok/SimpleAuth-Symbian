// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The hashes OTP needs. Qt 4.7's QCryptographicHash stops at SHA-1 and has no HMAC, so
// SHA-256/512 and HMAC are plain C++ here - the same code on the phone and on the PC.
#ifndef HASH_H
#define HASH_H

#include <QByteArray>

namespace Hash
{
    enum Algorithm { Sha1, Sha256, Sha512 };

    QByteArray sha1(const QByteArray &data);
    QByteArray sha256(const QByteArray &data);
    QByteArray sha512(const QByteArray &data);

    QByteArray hash(Algorithm algorithm, const QByteArray &data);
    /// HMAC (RFC 2104) over any of the three.
    QByteArray hmac(Algorithm algorithm, const QByteArray &key, const QByteArray &data);

    /// PBKDF2-HMAC-SHA256 (RFC 8018), turning the PIN into keys. Each iteration costs two
    /// SHA-256 blocks and no allocation, so a phone affords many in the time allowed.
    QByteArray pbkdf2Sha256(const QByteArray &password, const QByteArray &salt, int iterations, int length);

    /// Compares without an early exit, for MACs.
    bool equalConstantTime(const QByteArray &a, const QByteArray &b);

    /// "SHA1", "SHA256", "SHA512" - the spelling otpauth:// links use.
    QString name(Algorithm algorithm);
    /// Parses that spelling (case-insensitive, "SHA-1" too); false if unknown.
    bool fromName(const QString &name, Algorithm *algorithm);
}

#endif // HASH_H
