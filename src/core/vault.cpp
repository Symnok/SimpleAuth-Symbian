// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "vault.h"
#include "aes.h"
#include "hash.h"
#include "securerandom.h"

#include <QTime>

namespace
{
const int IvSize = 16;
const int MacSize = 32;
const int MinIterations = 5000;
const int MaxIterations = 2000000;
}

void VaultKeys::clear()
{
    encryption.fill('\0');
    authentication.fill('\0');
    encryption.clear();
    authentication.clear();
}

VaultKeys Vault::derive(const QString &secret, const QByteArray &salt, int iterations)
{
    QByteArray password = secret.toUtf8();
    QByteArray material = Hash::pbkdf2Sha256(password, salt, iterations, 64);
    password.fill('\0');
    VaultKeys keys;
    keys.encryption = material.left(32);
    keys.authentication = material.mid(32, 32);
    material.fill('\0');
    return keys;
}

QByteArray Vault::checkValue(const VaultKeys &keys)
{
    return Hash::hmac(Hash::Sha256, keys.authentication, QByteArray("SimpleAuth vault check v1"));
}

QByteArray Vault::seal(const VaultKeys &keys, const QByteArray &plain)
{
    const QByteArray iv = SecureRandom::bytes(IvSize);
    if (iv.size() != IvSize || keys.isNull()) return QByteArray();
    const QByteArray body = iv + Aes::ctr(keys.encryption, iv, plain);
    return body + Hash::hmac(Hash::Sha256, keys.authentication, body);
}

bool Vault::open(const VaultKeys &keys, const QByteArray &sealed, QByteArray *plain)
{
    if (keys.isNull() || sealed.size() < IvSize + MacSize) return false;
    const QByteArray body = sealed.left(sealed.size() - MacSize);
    if (!Hash::equalConstantTime(Hash::hmac(Hash::Sha256, keys.authentication, body), sealed.right(MacSize)))
        return false;
    *plain = Aes::ctr(keys.encryption, body.left(IvSize), body.mid(IvSize));
    return true;
}

namespace
{
const char BackupMagic[] = "SAUTHBK1";
const int BackupMagicSize = 8;
const int BackupHeaderSize = BackupMagicSize + 1 + 4 + Vault::SaltSize;
}

QByteArray Vault::packBackup(const Backup &backup)
{
    QByteArray out(BackupMagic, BackupMagicSize);
    out.append(char(backup.passphrase ? 1 : 0));
    const quint32 n = quint32(backup.iterations);
    out.append(char(n >> 24)).append(char(n >> 16)).append(char(n >> 8)).append(char(n));
    out.append(backup.salt);
    out.append(backup.sealed);
    return out;
}

bool Vault::unpackBackup(const QByteArray &data, Backup *backup)
{
    if (data.size() < BackupHeaderSize + IvSize + MacSize || !data.startsWith(QByteArray(BackupMagic, BackupMagicSize)))
        return false;
    const uchar kind = uchar(data.at(BackupMagicSize));
    if (kind > 1) return false;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData()) + BackupMagicSize + 1;
    const quint32 n = (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]);
    if (n == 0 || n > quint32(MaxIterations)) return false;
    backup->passphrase = kind == 1;
    backup->iterations = int(n);
    backup->salt = data.mid(BackupMagicSize + 5, SaltSize);
    backup->sealed = data.mid(BackupHeaderSize);
    return true;
}

int Vault::calibrate(int targetMs)
{
    // Grow the trial until it lasts long enough to time reliably.
    int trial = 1000;
    int elapsed = 0;
    for (;;) {
        QTime t;
        t.start();
        Hash::pbkdf2Sha256("calibration", "0123456789abcdef", trial, 64);
        elapsed = t.elapsed();
        if (elapsed >= 50 || trial >= MaxIterations) break;
        trial *= 4;
    }
    // Derivation makes 64 bytes = two PBKDF2 blocks, as the trial did, so the scale holds.
    const qint64 n = elapsed > 0 ? qint64(trial) * targetMs / elapsed : MaxIterations;
    return int(qBound(qint64(MinIterations), n, qint64(MaxIterations)));
}
