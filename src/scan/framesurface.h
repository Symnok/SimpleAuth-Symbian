// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The video surface the camera draws its viewfinder into (Symbian builds only).
#ifndef FRAMESURFACE_H
#define FRAMESURFACE_H

#include <QAbstractVideoSurface>
#include <QVideoFrame>
#include <QImage>
#include <QVideoSurfaceFormat>



/// Receives the viewfinder frames and hands each on as a QImage of its own (the camera
/// reuses the frame's buffer as soon as present() returns).
class FrameSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit FrameSurface(QObject *parent) : QAbstractVideoSurface(parent), m_format(QImage::Format_Invalid) {}

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
    {
        // RGB only: QImage takes these as they are, and ZXing wants luminance anyway.
        if (type != QAbstractVideoBuffer::NoHandle) return QList<QVideoFrame::PixelFormat>();
        return QList<QVideoFrame::PixelFormat>()
            << QVideoFrame::Format_RGB32 << QVideoFrame::Format_ARGB32
            << QVideoFrame::Format_ARGB32_Premultiplied << QVideoFrame::Format_RGB565;
    }

    bool start(const QVideoSurfaceFormat &format)
    {
        m_format = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
        if (m_format == QImage::Format_Invalid || format.frameSize().isEmpty()) return false;
        return QAbstractVideoSurface::start(format);
    }

    bool present(const QVideoFrame &frame)
    {
        QVideoFrame f(frame);
        if (!f.map(QAbstractVideoBuffer::ReadOnly)) return false;
        const QImage image(f.bits(), f.width(), f.height(), f.bytesPerLine(), m_format);
        emit frameReady(image.copy());
        f.unmap();
        return true;
    }

signals:
    void frameReady(const QImage &frame);

private:
    QImage::Format m_format;
};
#endif // FRAMESURFACE_H
