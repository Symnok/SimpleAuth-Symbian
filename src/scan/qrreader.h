// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Finds and reads a QR code in a picture, through ZXing's QR reader (3rdparty/zxing).
#ifndef QRREADER_H
#define QRREADER_H

#include <QByteArray>
#include <QString>

class QImage;

namespace QrReader
{
    /// 8-bit luminance of an image, row by row, scaled down so the longer side is at most
    /// maxSide (a QR code needs far fewer pixels than a camera gives, and ZXing's time grows
    /// with the area). *width and *height receive the result's size.
    QByteArray luminance(const QImage &image, int maxSide, int *width, int *height);

    /// The text of the QR code in a luminance picture, or an empty string if none is found.
    /// smooth first averages each pixel with its neighbours (3x3), which rescues grainy
    /// pictures (a dim room) at some cost in sharpness; the camera page alternates.
    QString decode(const QByteArray &luminance, int width, int height, bool smooth = false);

    /// Both steps at once, trying as is and then smoothed.
    QString decode(const QImage &image, int maxSide = 800);
}

#endif // QRREADER_H
