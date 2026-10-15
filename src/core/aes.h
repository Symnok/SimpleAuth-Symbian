// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// AES-256 (FIPS-197) encryption and CTR mode (SP 800-38A), for the encrypted account
// store. CTR only ever runs the cipher forwards, so there is no decryption half. The table
// code comes from Symbigram.
#ifndef AES_H
#define AES_H

#include <QByteArray>

namespace Aes
{
    /// One 16-byte block under a 32-byte key (for the self test).
    QByteArray encryptBlock(const QByteArray &key32, const QByteArray &block16);
    /// CTR mode: data XOR the keystream from a 16-byte initial counter block, incremented as
    /// a 128-bit big-endian number. The same call encrypts and decrypts.
    QByteArray ctr(const QByteArray &key32, const QByteArray &counter16, const QByteArray &data);
}

#endif // AES_H
