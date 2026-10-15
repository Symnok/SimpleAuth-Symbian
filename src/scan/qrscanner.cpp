// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "qrscanner.h"
#include "qrreader.h"

#include <QPainter>
#include <QThread>
#include <QTimer>

#ifdef SA_CAMERA
#include "framesurface.h"
#include <QCameraFocus>

#endif

namespace
{
/// The longer side frames are decoded at: plenty for a QR code filling part of the picture,
/// and small enough for a 2011 phone to try a few times a second.
const int DecodeSide = 480;
/// A code read again within this time is not reported again.
const int RepeatMs = 3000;
/// Autofocus cameras without continuous focus: focus again this often while nothing is found.
const int RefocusMs = 4000;

#ifdef SA_CAMERA
struct FocusModeName { QCameraFocus::FocusMode mode; const char *name; };
const FocusModeName FocusModeNames[] = {
    { QCameraFocus::AutoFocus, "auto" },
    { QCameraFocus::ContinuousFocus, "continuous" },
    { QCameraFocus::MacroFocus, "macro" },
    { QCameraFocus::HyperfocalFocus, "hyperfocal" },
    { QCameraFocus::InfinityFocus, "infinity" },
    { QCameraFocus::ManualFocus, "manual" }
};
const int FocusModeCount = sizeof FocusModeNames / sizeof FocusModeNames[0];
#endif
}

void QrScanWorker::scan(const QImage &frame, bool smooth)
{
    int w = 0, h = 0;
    const QByteArray lum = QrReader::luminance(frame, DecodeSide, &w, &h);
    emit scanned(QrReader::decode(lum, w, h, smooth));
}

QrScanner::QrScanner(QDeclarativeItem *parent)
    : QDeclarativeItem(parent), m_active(false), m_frameRotation(0), m_zoom(1), m_maximumZoom(1),
      m_canLockFocus(false), m_status(QLatin1String("off")),
      m_thread(new QThread(this)), m_worker(new QrScanWorker), m_busy(false), m_smooth(false),
      m_focusTimer(new QTimer(this)), m_testTimer(new QTimer(this)), m_testIndex(0)
#ifdef SA_CAMERA
      , m_camera(0), m_surface(0)
#endif
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setClip(true);

    // ZXing keeps some tables on the stack; Symbian's default thread stack is small.
    m_thread->setStackSize(0x40000);
    m_worker->moveToThread(m_thread);
    connect(m_thread, SIGNAL(finished()), m_worker, SLOT(deleteLater()));
    connect(m_worker, SIGNAL(scanned(QString)), this, SLOT(onScanned(QString)));
    m_thread->start(QThread::LowPriority);

    m_focusTimer->setInterval(RefocusMs);
    connect(m_focusTimer, SIGNAL(timeout()), this, SLOT(refocus()));

#ifndef SA_CAMERA
    const QString images = QString::fromLocal8Bit(qgetenv("SA_SCAN_IMAGES"));
    m_testImages = images.split(QLatin1Char(';'), QString::SkipEmptyParts);
    m_testTimer->setInterval(400);
    connect(m_testTimer, SIGNAL(timeout()), this, SLOT(nextTestImage()));
#endif
}

QrScanner::~QrScanner()
{
    stop();
    m_thread->quit();
    m_thread->wait();
}

void QrScanner::setActive(bool active)
{
    if (active == m_active) return;
    m_active = active;
    if (active) start();
    else stop();
    emit activeChanged();
}

void QrScanner::setStatus(const QString &status, const QString &error)
{
    if (status == m_status && error == m_error) return;
    m_status = status;
    m_error = error;
    emit statusChanged();
}

void QrScanner::start()
{
    m_lastText.clear();
#ifdef SA_CAMERA
    if (!m_camera) {
        m_camera = new QCamera(this);
        connect(m_camera, SIGNAL(stateChanged(QCamera::State)), this, SLOT(onCameraState(QCamera::State)));
        connect(m_camera, SIGNAL(error(QCamera::Error)), this, SLOT(onCameraError(QCamera::Error)));
        m_surface = new FrameSurface(this);
        connect(m_surface, SIGNAL(frameReady(QImage)), this, SLOT(onFrame(QImage)));
        m_camera->setViewfinder(m_surface);
        m_camera->setCaptureMode(QCamera::CaptureStillImage);
    }
    setStatus(QLatin1String("starting"));
    m_camera->start();
#else
    if (m_testImages.isEmpty()) {
        setStatus(QLatin1String("error"), tr("There is no camera in this build."));
        return;
    }
    setStatus(QLatin1String("scanning"));
    m_testIndex = 0;
    nextTestImage();
    m_testTimer->start();
#endif
}

void QrScanner::stop()
{
    m_focusTimer->stop();
    m_testTimer->stop();
#ifdef SA_CAMERA
    if (m_camera) m_camera->stop();
#endif
    m_frame = QImage();
    setStatus(QLatin1String("off"));
    update();
}

void QrScanner::refocus()
{
#ifdef SA_CAMERA
    if (!m_camera || m_camera->state() != QCamera::ActiveState || !m_canLockFocus) return;
    m_camera->unlock(QCamera::LockFocus);
    m_camera->searchAndLock(QCamera::LockFocus);
#endif
}

void QrScanner::setFocusMode(const QString &mode)
{
    if (mode == m_focusMode) return;
    m_focusMode = mode;
    applyFocus();
    emit focusModeChanged();
}

void QrScanner::setZoom(qreal zoom)
{
    zoom = qMax(qreal(1), zoom);
    if (qAbs(zoom - m_zoom) < 0.01) return;
    m_zoom = zoom;
    applyZoom();
    emit zoomChanged();
}

void QrScanner::applyFocus()
{
#ifdef SA_CAMERA
    if (!m_camera || m_camera->state() != QCamera::ActiveState) return;
    m_focusTimer->stop();
    QCameraFocus *focus = m_camera->focus();
    // The mode asked for, if this camera has it; otherwise continuous focus where the camera
    // has it, else autofocus now and then (a fixed-focus camera does neither).
    int chosen = -1;
    for (int i = 0; i < FocusModeCount; ++i)
        if (m_focusMode == QLatin1String(FocusModeNames[i].name) && focus->isFocusModeSupported(FocusModeNames[i].mode))
            chosen = i;
    QCameraFocus::FocusMode mode;
    if (chosen >= 0) mode = FocusModeNames[chosen].mode;
    else if (focus->isFocusModeSupported(QCameraFocus::ContinuousFocus)) mode = QCameraFocus::ContinuousFocus;
    else if (focus->isFocusModeSupported(QCameraFocus::AutoFocus)) mode = QCameraFocus::AutoFocus;
    else return;
    if (m_canLockFocus) m_camera->unlock(QCamera::LockFocus);
    focus->setFocusMode(mode);
    // Auto and macro focus once per lock: now, and again while nothing has been read.
    if (mode == QCameraFocus::AutoFocus || mode == QCameraFocus::MacroFocus) {
        refocus();
        m_focusTimer->start();
    }
    emit cameraInfoChanged();
#endif
}

void QrScanner::applyZoom()
{
#ifdef SA_CAMERA
    if (!m_camera || m_camera->state() != QCamera::ActiveState) return;
    m_camera->focus()->zoomTo(1.0, qBound(qreal(1), m_zoom, m_maximumZoom));
#endif
}

QString QrScanner::cameraInfo() const
{
    QStringList parts;
    if (!m_frameInfo.isEmpty()) parts << m_frameInfo;
#ifdef SA_CAMERA
    if (m_camera && m_camera->state() == QCamera::ActiveState) {
        const QCameraFocus::FocusMode current = m_camera->focus()->focusMode();
        QString currentName = QString::number(int(current));
        for (int i = 0; i < FocusModeCount; ++i)
            if (FocusModeNames[i].mode == current) currentName = QLatin1String(FocusModeNames[i].name);
        parts << QString::fromLatin1("focus %1 of [%2]").arg(currentName, m_focusModes.join(QLatin1String("/")));
        parts << QLatin1String(m_canLockFocus ? "AF lock" : "no AF lock");
        parts << QString::fromLatin1("zoom to %1x").arg(m_maximumZoom, 0, 'f', 1);
    }
#else
    parts << QLatin1String("test images, no camera");
#endif
    return parts.join(QLatin1String(", "));
}

#ifdef SA_CAMERA
void QrScanner::onCameraState(QCamera::State state)
{
    if (state != QCamera::ActiveState || !m_active) return;
    setStatus(QLatin1String("scanning"));
    // What this camera can do, for the scan page's focus and zoom controls.
    QCameraFocus *focus = m_camera->focus();
    m_focusModes.clear();
    for (int i = 0; i < FocusModeCount; ++i)
        if (focus->isFocusModeSupported(FocusModeNames[i].mode))
            m_focusModes << QLatin1String(FocusModeNames[i].name);
    m_canLockFocus = m_camera->supportedLocks() & QCamera::LockFocus;
    m_maximumZoom = qMax(qreal(1), focus->maximumDigitalZoom());
    emit cameraInfoChanged();
    applyFocus();
    applyZoom();
}

void QrScanner::onCameraError(QCamera::Error error)
{
    Q_UNUSED(error);
    setStatus(QLatin1String("error"), m_camera->errorString().isEmpty()
                                          ? tr("The camera could not be started.") : m_camera->errorString());
}
#endif

void QrScanner::nextTestImage()
{
    if (m_testImages.isEmpty()) return;
    const QImage image(m_testImages.at(m_testIndex % m_testImages.size()));
    ++m_testIndex;
    if (!image.isNull()) onFrame(image);
}

void QrScanner::onFrame(const QImage &frame)
{
    if (!m_active) return;
    if (frame.size() != m_frame.size()) {
        m_frameInfo = QString::fromLatin1("%1x%2").arg(frame.width()).arg(frame.height());
        emit cameraInfoChanged();
    }
    m_frame = frame;
    update();
    if (m_busy) return;
    // Alternate plain and smoothed attempts: the second rescues grainy low-light frames.
    m_busy = true;
    QMetaObject::invokeMethod(m_worker, "scan", Qt::QueuedConnection, Q_ARG(QImage, frame), Q_ARG(bool, m_smooth));
    m_smooth = !m_smooth;
}

void QrScanner::onScanned(const QString &text)
{
    m_busy = false;
    if (text.isEmpty() || !m_active) return;
    if (text == m_lastText && m_lastTime.elapsed() < RepeatMs) return;
    m_lastText = text;
    m_lastTime.start();
    emit found(text);
}

void QrScanner::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    const QRectF r = boundingRect();
    painter->fillRect(r, Qt::black);
    if (m_frame.isNull()) return;

    // Turned as frameRotation says (Belle FP2 hands frames over upright, Belle Refresh
    // sideways), then scaled to fit, whole.
    const bool quarter = m_frameRotation == 90 || m_frameRotation == 270;
    const QSizeF target = quarter ? QSizeF(r.height(), r.width()) : r.size();
    const qreal scale = qMin(target.width() / m_frame.width(), target.height() / m_frame.height());
    const QSizeF size(m_frame.width() * scale, m_frame.height() * scale);

    painter->save();
    painter->translate(r.center());
    if (m_frameRotation) painter->rotate(m_frameRotation);
    painter->drawImage(QRectF(-size.width() / 2, -size.height() / 2, size.width(), size.height()), m_frame);
    painter->restore();
}

void QrScanner::setFrameRotation(int degrees)
{
    degrees = ((degrees % 360) + 360) % 360 / 90 * 90;
    if (degrees == m_frameRotation) return;
    m_frameRotation = degrees;
    update();
    emit frameRotationChanged();
}

