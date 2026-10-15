// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "securerandom.h"

#if defined(Q_OS_SYMBIAN)
#include <e32math.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <wincrypt.h>
#else
#include <QFile>
#endif

QByteArray SecureRandom::bytes(int count)
{
    QByteArray out(count, '\0');
#if defined(Q_OS_SYMBIAN)
    for (int i = 0; i < count; i += 4) {
        TUint32 r = Math::Random();
        for (int j = 0; j < 4 && i + j < count; ++j) out[i + j] = char(r >> (8 * j));
    }
#elif defined(Q_OS_WIN)
    HCRYPTPROV prov = 0;
    if (!CryptAcquireContextW(&prov, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
        return QByteArray();
    const BOOL ok = CryptGenRandom(prov, DWORD(count), reinterpret_cast<BYTE *>(out.data()));
    CryptReleaseContext(prov, 0);
    if (!ok) return QByteArray();
#else
    QFile f(QLatin1String("/dev/urandom"));
    if (!f.open(QIODevice::ReadOnly) || f.read(out.data(), count) != count) return QByteArray();
#endif
    return out;
}
