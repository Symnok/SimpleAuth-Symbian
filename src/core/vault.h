// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The encrypted account store's crypto. The PIN (or passphrase) and a random salt go
// through PBKDF2-HMAC-SHA256 to 64 bytes: an AES-256 key and an HMAC-SHA256 key. A sealed
// blob is IV (16) || AES-256-CTR ciphertext || HMAC-SHA256(IV || ciphertext) (32): encrypt
// then MAC, so a wrong PIN or a changed file is detected before anything is decrypted.
#ifndef VAULT_H
#define VAULT_H

#include <QByteArray>
#include <QString>

struct VaultKeys
{
    QByteArray encryption;      ///< 32 bytes
    QByteArray authentication;  ///< 32 bytes
    bool isNull() const { return encryption.isEmpty(); }
    void clear();
};

namespace Vault
{
    const int SaltSize = 16;

    VaultKeys derive(const QString &secret, const QByteArray &salt, int iterations);
    /// A value stored beside the salt to tell a right PIN from a wrong one.
    QByteArray checkValue(const VaultKeys &keys);
    /// Empty if the platform could not supply random bytes for the IV.
    QByteArray seal(const VaultKeys &keys, const QByteArray &plain);
    /// False if the blob was sealed with other keys or has been changed.
    bool open(const VaultKeys &keys, const QByteArray &sealed, QByteArray *plain);

    /// The iteration count that takes about targetMs on this phone (measured now), within
    /// limits that keep weak phones usable and fast PCs from going overboard.
    int calibrate(int targetMs);

    /// A backup file (.sauth): "SAUTHBK1", the secret's kind (0 = PIN, 1 = passphrase), the
    /// iteration count (4 bytes, big-endian), the salt, then a sealed blob. Everything needed
    /// to open it with the PIN or passphrase that was in use when it was made.
    struct Backup
    {
        bool passphrase;
        int iterations;
        QByteArray salt;
        QByteArray sealed;
        Backup() : passphrase(false), iterations(0) {}
    };
    QByteArray packBackup(const Backup &backup);
    /// False if data is not a backup file (or is cut short).
    bool unpackBackup(const QByteArray &data, Backup *backup);
}

#endif // VAULT_H
