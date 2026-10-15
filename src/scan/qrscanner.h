// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// A QML item that shows the camera's viewfinder and reads the QR codes in it. The frames
// come from the camera through a video surface (Qt Mobility, Symbian builds: SA_CAMERA);
// decoding runs on a worker thread so the picture keeps moving. On the PC there is no
// camera: SA_SCAN_IMAGES (files separated by ';') are played as frames instead, for tests.
#ifndef QRSCANNER_H
#define QRSCANNER_H

#include <QDeclarativeItem>
#include <QImage>
#include <QStringList>
#include <QTime>

class QThread;
class QTimer;
class QrScanWorker;

#ifdef SA_CAMERA
#include <QCamera>
class FrameSurface;
#endif

class QrScanner : public QDeclarativeItem
{
    Q_OBJECT
    /// True while the page is on screen and the app in front: runs the camera.
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    /// "off", "starting", "scanning" or "error".
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY statusChanged)
    /// Degrees (0, 90, 180, 270) the picture is turned clockwise on screen: some firmwares
    /// hand the viewfinder over upright, others sideways (reading works either way).
    Q_PROPERTY(int frameRotation READ frameRotation WRITE setFrameRotation NOTIFY frameRotationChanged)
    /// The focus mode asked for: "" chooses by itself (continuous if the camera has it, else
    /// autofocus now and then), or one of focusModes. Cameras differ (the Nokia 808's does
    /// not focus close up by default), so the scan page lets the user pick.
    Q_PROPERTY(QString focusMode READ focusMode WRITE setFocusMode NOTIFY focusModeChanged)
    /// The modes this camera reports: "auto", "continuous", "macro", "hyperfocal", "infinity".
    Q_PROPERTY(QStringList focusModes READ focusModes NOTIFY cameraInfoChanged)
    /// Digital zoom, 1 = none. Lets the phone stay back beyond its closest focus distance.
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(qreal maximumZoom READ maximumZoom NOTIFY cameraInfoChanged)
    /// What the camera reported, one line, for finding out what a phone needs.
    Q_PROPERTY(QString cameraInfo READ cameraInfo NOTIFY cameraInfoChanged)

public:
    explicit QrScanner(QDeclarativeItem *parent = 0);
    ~QrScanner();

    bool isActive() const { return m_active; }
    void setActive(bool active);
    QString status() const { return m_status; }
    QString errorString() const { return m_error; }
    int frameRotation() const { return m_frameRotation; }
    void setFrameRotation(int degrees);
    QString focusMode() const { return m_focusMode; }
    void setFocusMode(const QString &mode);
    QStringList focusModes() const { return m_focusModes; }
    qreal zoom() const { return m_zoom; }
    void setZoom(qreal zoom);
    qreal maximumZoom() const { return m_maximumZoom; }
    QString cameraInfo() const;

    /// Focuses again on whatever is in the middle (tap on the picture).
    Q_INVOKABLE void refocus();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

signals:
    void activeChanged();
    void statusChanged();
    void frameRotationChanged();
    void focusModeChanged();
    void zoomChanged();
    void cameraInfoChanged();
    /// A QR code was read. The same text is not repeated for a few seconds.
    void found(const QString &text);

private slots:
    void onFrame(const QImage &frame);
    void onScanned(const QString &text);
    void nextTestImage();
#ifdef SA_CAMERA
    void onCameraState(QCamera::State state);
    void onCameraError(QCamera::Error error);
#endif

private:
    void start();
    void stop();
    void setStatus(const QString &status, const QString &error = QString());
    void applyFocus();
    void applyZoom();

    bool m_active;
    int m_frameRotation;
    QString m_focusMode;
    QStringList m_focusModes;
    qreal m_zoom;
    qreal m_maximumZoom;
    bool m_canLockFocus;
    QString m_frameInfo;
    QString m_status;
    QString m_error;
    QImage m_frame;

    QThread *m_thread;
    QrScanWorker *m_worker;
    bool m_busy;
    bool m_smooth;
    QString m_lastText;
    QTime m_lastTime;

    QTimer *m_focusTimer;
    QTimer *m_testTimer;
    QStringList m_testImages;
    int m_testIndex;

#ifdef SA_CAMERA
    QCamera *m_camera;
    FrameSurface *m_surface;
#endif
};

/// Decodes on the worker thread, one frame at a time.
class QrScanWorker : public QObject
{
    Q_OBJECT
public slots:
    void scan(const QImage &frame, bool smooth);
signals:
    void scanned(const QString &text);
};

#endif // QRSCANNER_H
