// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Secure random bytes from the platform (salts and IVs of the encrypted store): the kernel
// random pool on Symbian (Math::Random), CryptGenRandom on Windows, /dev/urandom elsewhere.
// qrand() is never used. From Symbigram.
#ifndef SECURERANDOM_H
#define SECURERANDOM_H

#include <QByteArray>

namespace SecureRandom
{
    /// count random bytes, or an empty array if the platform source failed.
    QByteArray bytes(int count);
}

#endif // SECURERANDOM_H
