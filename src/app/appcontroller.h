// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// What QML reaches as "app": the lock (setting, checking and changing the PIN or
// passphrase, auto-lock), the account list and its encrypted storage, adding accounts, the
// clock correction, the camera settings, the clipboard and the notice banner.
#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "vault.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

class AccountsModel;
struct OtpAccount;
class QSettings;
class QWidget;

class AppController : public QObject
{
    Q_OBJECT
    /// "setup" (no PIN yet: one must be set before anything else), "locked" or "unlocked".
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    /// "pin" (digits) or "passphrase": which keyboard the unlock page offers.
    Q_PROPERTY(QString secretKind READ secretKind NOTIFY stateChanged)
    /// True on the first start after an update from a version without the lock: the setup
    /// page then says that the existing accounts will be encrypted.
    Q_PROPERTY(bool hasUnprotectedAccounts READ hasUnprotectedAccounts NOTIFY stateChanged)
    /// Seconds in the background before the app locks itself; 0 = at once.
    Q_PROPERTY(int autoLock READ autoLock WRITE setAutoLock NOTIFY autoLockChanged)
    /// Where backups are written and looked for, e.g. "E:/SimpleAuth".
    Q_PROPERTY(QString backupDir READ backupDir WRITE setBackupDir NOTIFY backupDirChanged)
    Q_PROPERTY(QObject *accounts READ accounts CONSTANT)
    Q_PROPERTY(int timeOffset READ timeOffset WRITE setTimeOffset NOTIFY timeOffsetChanged)
    /// How far the scan page turns the camera picture (degrees clockwise). Until the user
    /// turns it, a guess from the firmware: Qt 4.8 (Belle FP1/FP2) delivers the viewfinder
    /// upright, Qt 4.7 (Anna, Belle Refresh) sideways.
    Q_PROPERTY(int cameraRotation READ cameraRotation WRITE setCameraRotation NOTIFY cameraRotationChanged)
    /// The scan page's focus mode ("" = automatic) and zoom, remembered per phone.
    Q_PROPERTY(QString cameraFocusMode READ cameraFocusMode WRITE setCameraFocusMode NOTIFY cameraSettingsChanged)
    Q_PROPERTY(qreal cameraZoom READ cameraZoom WRITE setCameraZoom NOTIFY cameraSettingsChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    /// The Qt the phone runs (the firmware's, or a newer one installed over it).
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)

public:
    AppController(QSettings *settings, QObject *parent = 0);
    ~AppController();

    QString state() const { return m_state; }
    QString secretKind() const;
    bool hasUnprotectedAccounts() const;
    int autoLock() const;
    void setAutoLock(int seconds);
    QString backupDir() const;
    void setBackupDir(const QString &dir);
    QObject *accounts() const;
    int timeOffset() const { return m_timeOffset; }
    void setTimeOffset(int seconds);
    int cameraRotation() const;
    void setCameraRotation(int degrees);
    QString cameraFocusMode() const;
    void setCameraFocusMode(const QString &mode);
    qreal cameraZoom() const;
    void setCameraZoom(qreal zoom);
    QString version() const;
    QString qtVersion() const { return QString::fromLatin1(qVersion()); }
    void setView(QWidget *view) { m_view = view; }

    // -- the lock (each call takes about half a second: the key derivation is made slow on
    //    purpose, against guessing) --

    /// First start: sets the PIN (kind "pin", 4+ digits) or passphrase (8+ characters),
    /// entered twice, and encrypts any accounts kept by an older version.
    Q_INVOKABLE bool setupSecret(const QString &secret, const QString &confirm, const QString &kind);
    Q_INVOKABLE bool unlock(const QString &secret);
    Q_INVOKABLE void lock();
    Q_INVOKABLE bool changeSecret(const QString &current, const QString &secret, const QString &confirm, const QString &kind);
    /// Seconds before another unlock attempt is accepted (after repeated wrong ones).
    Q_INVOKABLE int retrySeconds() const;
    /// The app came to the front or went behind (auto-lock).
    Q_INVOKABLE void setForeground(bool foreground);

    // -- backup --

    /// The folders a backup can go to: "SimpleAuth" on each writable drive.
    Q_INVOKABLE QStringList backupDirChoices() const;
    /// Writes all accounts, encrypted under the current PIN or passphrase, to
    /// backupDir/yyyy-MM-dd-hh-mm-ss.sauth. Returns the file name, or "" (a notice says why).
    Q_INVOKABLE QString backup();
    /// The .sauth files in backupDir, newest first.
    Q_INVOKABLE QStringList backupFiles() const;
    /// "pin" or "passphrase" for a backup file (which keyboard to offer), "" if unreadable.
    Q_INVOKABLE QString backupKind(const QString &fileName) const;
    /// Opens a backup with the PIN or passphrase it was made under and adds the accounts
    /// this phone lacks (for counter-based ones already here, the higher counter wins).
    Q_INVOKABLE bool restore(const QString &fileName, const QString &secret);

    // -- accounts --

    /// Adds an account typed in by hand. type is "totp" or "hotp", algorithm "SHA1"/"SHA256"/
    /// "SHA512". Returns true when added; otherwise a notice says why.
    Q_INVOKABLE bool addAccount(const QString &issuer, const QString &name, const QString &secret,
                                const QString &type, const QString &algorithm, int digits, int period,
                                const QString &counter);
    /// Adds the account an otpauth:// link describes (the text inside a site's QR code).
    Q_INVOKABLE bool addFromLink(const QString &link);

    Q_INVOKABLE void copyCode(int row);
    /// Puts text on the clipboard (the edit page's "Copy" of the secret key).
    Q_INVOKABLE void copyText(const QString &text);
    /// Shows text in the banner (for messages QML makes itself).
    Q_INVOKABLE void showNotice(const QString &text) { emit notice(text); }
    /// The corrected UTC time, "HH:mm:ss", for checking the phone's clock in Settings.
    Q_INVOKABLE QString utcTime() const;
    /// The phone's own time zone offset from UTC, "+03:00" - a wrong zone gives wrong codes.
    Q_INVOKABLE QString zoneOffset() const;

    /// Desktop testing only: saves a picture of the window (SA_SHOT_DIR).
    Q_INVOKABLE void screenshot(const QString &name);

signals:
    void stateChanged();
    void autoLockChanged();
    void backupDirChanged();
    void timeOffsetChanged();
    void cameraRotationChanged();
    void cameraSettingsChanged();
    /// A short message for the banner.
    void notice(const QString &text);

private slots:
    void saveAccounts();

private:
    void setState(const QString &state);
    bool checkNewSecret(const QString &secret, const QString &confirm, const QString &kind);
    /// Derives keys for a new secret and stores its salt, iteration count and check value.
    bool storeNewSecret(const QString &secret, const QString &kind);
    /// The keys, if secret is the right one (slow).
    bool verify(const QString &secret, VaultKeys *keys) const;
    QList<OtpAccount> loadUnprotected() const;
    static QByteArray linksOf(const QList<OtpAccount> &accounts);
    static QList<OtpAccount> accountsOf(const QByteArray &links);

    QSettings *m_settings;
    AccountsModel *m_accounts;
    QWidget *m_view;
    int m_timeOffset;
    QString m_state;
    VaultKeys m_keys;
    qint64 m_backgroundSince;
};

#endif // APPCONTROLLER_H
