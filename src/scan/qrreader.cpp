// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "qrreader.h"

#include <QImage>

#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>
#include <zxing/Exception.h>
#include <zxing/Result.h>
#include <zxing/common/GreyscaleLuminanceSource.h>
#include <zxing/common/HybridBinarizer.h>
#include <zxing/qrcode/QRCodeReader.h>

QByteArray QrReader::luminance(const QImage &image, int maxSide, int *width, int *height)
{
    QImage img = image;
    if (qMax(img.width(), img.height()) > maxSide)
        img = img.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::FastTransformation);
    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_RGB32);

    const int w = img.width(), h = img.height();
    QByteArray out(w * h, '\0');
    uchar *dst = reinterpret_cast<uchar *>(out.data());
    for (int y = 0; y < h; ++y) {
        const QRgb *row = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            const QRgb p = row[x];
            // ITU-R 601 weights in fixed point (they sum to 1024).
            *dst++ = uchar((qRed(p) * 306 + qGreen(p) * 601 + qBlue(p) * 117) >> 10);
        }
    }
    *width = w;
    *height = h;
    return out;
}

namespace
{
/// 3x3 box filter; the one-pixel border is copied as is.
void smooth3x3(const uchar *src, uchar *dst, int w, int h)
{
    memcpy(dst, src, w * h);
    for (int y = 1; y < h - 1; ++y) {
        const uchar *a = src + (y - 1) * w, *b = src + y * w, *c = src + (y + 1) * w;
        uchar *o = dst + y * w;
        for (int x = 1; x < w - 1; ++x) {
            const int sum = a[x - 1] + a[x] + a[x + 1] + b[x - 1] + b[x] + b[x + 1] + c[x - 1] + c[x] + c[x + 1];
            o[x] = uchar((sum * 7282) >> 16);   // sum / 9
        }
    }
}
}

QString QrReader::decode(const QByteArray &luminance, int width, int height, bool smooth)
{
    if (width <= 0 || height <= 0 || luminance.size() < width * height) return QString();
    try {
        zxing::ArrayRef<char> grey(width * height);
        if (smooth)
            smooth3x3(reinterpret_cast<const uchar *>(luminance.constData()), reinterpret_cast<uchar *>(&grey[0]), width, height);
        else
            memcpy(&grey[0], luminance.constData(), width * height);
        zxing::Ref<zxing::LuminanceSource> source(
            new zxing::GreyscaleLuminanceSource(grey, width, height, 0, 0, width, height));
        zxing::Ref<zxing::Binarizer> binarizer(new zxing::HybridBinarizer(source));
        zxing::Ref<zxing::BinaryBitmap> bitmap(new zxing::BinaryBitmap(binarizer));
        zxing::qrcode::QRCodeReader reader;
        zxing::DecodeHints hints(zxing::DecodeHints::QR_CODE_HINT);
        hints.setTryHarder(true);
        zxing::Ref<zxing::Result> result = reader.decode(bitmap, hints);
        return QString::fromUtf8(result->getText()->getText().c_str());
    } catch (const zxing::Exception &) {
        // No code in sight, or one that could not be read: the usual case between frames.
    } catch (const std::exception &) {
    }
    return QString();
}

QString QrReader::decode(const QImage &image, int maxSide)
{
    int w = 0, h = 0;
    const QByteArray lum = luminance(image, maxSide, &w, &h);
    const QString text = decode(lum, w, h, false);
    return text.isEmpty() ? decode(lum, w, h, true) : text;
}
