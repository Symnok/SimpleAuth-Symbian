// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// A QR Code encoder - byte mode, error correction level M, versions 1 to 15 - to show an
// account's otpauth:// link for another authenticator to scan. From Symbigram, where it was
// verified bit for bit against a reference encoder; tools/qr-test round-trips it through
// the app's own reader.
#ifndef QRCODE_H
#define QRCODE_H

#include <QByteArray>
#include <QString>
#include <QVector>

class QrCode
{
public:
    /// Encodes text; empty on failure (text too long). The matrix is size x size modules,
    /// row-major, true = dark. forcedMask is for tests (-1 chooses the best mask).
    static bool encode(const QString &text, QVector<bool> &modules, int &size, int forcedMask = -1);
};

#endif // QRCODE_H
