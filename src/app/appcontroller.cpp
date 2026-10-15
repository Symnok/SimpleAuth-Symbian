// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "appcontroller.h"
#include "accountsmodel.h"
#include "otp.h"
#include "securerandom.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>
#include <QSettings>
#include <QStringList>
#include <QWidget>

#define SA_STRINGIFY2(x) #x
#define SA_STRINGIFY(x) SA_STRINGIFY2(x)

namespace
{
const char *const KeyTimeOffset = "time/offset";
const char *const KeyCameraRotation = "camera/rotation";
const char *const KeyCameraFocus = "camera/focus";
const char *const KeyCameraZoom = "camera/zoom";
const char *const KeyAutoLock = "security/autoLock";
const char *const KeyBackupDir = "backup/dir";
// The encrypted store (see vault.h). data holds the accounts' otpauth:// links, one per line.
const char *const KeyVaultKind = "vault/kind";
const char *const KeyVaultSalt = "vault/salt";
const char *const KeyVaultIterations = "vault/iterations";
const char *const KeyVaultCheck = "vault/check";
const char *const KeyVaultData = "vault/data";
const char *const KeyVaultFailures = "vault/failures";
const char *const KeyVaultRetryAt = "vault/retryAt";
// Versions before the lock kept the links in plain text here.
const char *const KeyUnprotected = "accounts";

/// How long deriving the keys should take on this phone: long enough to slow guessing,
/// short enough not to annoy.
const int DeriveMs = 500;
const int DefaultAutoLock = 60;
/// Wrong attempts allowed before waits start: 30 s, then doubling, up to an hour.
const int FreeAttempts = 4;

qint64 nowSeconds() { return QDateTime::currentMSecsSinceEpoch() / 1000; }
}

AppController::AppController(QSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings), m_view(0), m_backgroundSince(0)
{
    m_accounts = new AccountsModel(this);
    connect(m_accounts, SIGNAL(edited()), this, SLOT(saveAccounts()));
    m_timeOffset = settings->value(QLatin1String(KeyTimeOffset), 0).toInt();
    m_accounts->setTimeOffset(m_timeOffset);
    m_state = settings->contains(QLatin1String(KeyVaultCheck)) ? QLatin1String("locked") : QLatin1String("setup");
}

AppController::~AppController()
{
    m_keys.clear();
}

QObject *AppController::accounts() const
{
    return m_accounts;
}

// -- the lock -----------------------------------------------------------------------------------------

void AppController::setState(const QString &state)
{
    if (state == m_state) return;
    m_state = state;
    emit stateChanged();
}

QString AppController::secretKind() const
{
    return m_settings->value(QLatin1String(KeyVaultKind), QLatin1String("pin")).toString();
}

bool AppController::hasUnprotectedAccounts() const
{
    return m_state == QLatin1String("setup") && !loadUnprotected().isEmpty();
}

int AppController::autoLock() const
{
    return m_settings->value(QLatin1String(KeyAutoLock), DefaultAutoLock).toInt();
}

void AppController::setAutoLock(int seconds)
{
    if (seconds == autoLock()) return;
    m_settings->setValue(QLatin1String(KeyAutoLock), qMax(0, seconds));
    m_settings->sync();
    emit autoLockChanged();
}

bool AppController::checkNewSecret(const QString &secret, const QString &confirm, const QString &kind)
{
    if (kind == QLatin1String("pin")) {
        for (int i = 0; i < secret.size(); ++i)
            if (!secret.at(i).isDigit()) {
                emit notice(tr("A PIN is digits only."));
                return false;
            }
        if (secret.size() < 4) {
            emit notice(tr("The PIN must have at least 4 digits."));
            return false;
        }
    } else if (secret.size() < 8) {
        emit notice(tr("The passphrase must have at least 8 characters."));
        return false;
    }
    if (secret != confirm) {
        emit notice(kind == QLatin1String("pin") ? tr("The two PINs differ.") : tr("The two passphrases differ."));
        return false;
    }
    return true;
}

bool AppController::storeNewSecret(const QString &secret, const QString &kind)
{
    const QByteArray salt = SecureRandom::bytes(Vault::SaltSize);
    if (salt.size() != Vault::SaltSize) {
        emit notice(tr("The phone gave no random numbers; nothing was changed."));
        return false;
    }
    const int iterations = Vault::calibrate(DeriveMs);
    VaultKeys keys = Vault::derive(secret, salt, iterations);
    m_settings->setValue(QLatin1String(KeyVaultKind), kind == QLatin1String("pin") ? QLatin1String("pin") : QLatin1String("passphrase"));
    m_settings->setValue(QLatin1String(KeyVaultSalt), salt.toBase64());
    m_settings->setValue(QLatin1String(KeyVaultIterations), iterations);
    m_settings->setValue(QLatin1String(KeyVaultCheck), Vault::checkValue(keys).toBase64());
    m_settings->remove(QLatin1String(KeyVaultFailures));
    m_settings->remove(QLatin1String(KeyVaultRetryAt));
    m_keys.clear();
    m_keys = keys;
    return true;
}

bool AppController::verify(const QString &secret, VaultKeys *keys) const
{
    const QByteArray salt = QByteArray::fromBase64(m_settings->value(QLatin1String(KeyVaultSalt)).toByteArray());
    const int iterations = m_settings->value(QLatin1String(KeyVaultIterations)).toInt();
    const QByteArray check = QByteArray::fromBase64(m_settings->value(QLatin1String(KeyVaultCheck)).toByteArray());
    if (salt.isEmpty() || iterations <= 0 || check.isEmpty()) return false;
    VaultKeys k = Vault::derive(secret, salt, iterations);
    if (!Hash::equalConstantTime(Vault::checkValue(k), check)) {
        k.clear();
        return false;
    }
    *keys = k;
    return true;
}

bool AppController::setupSecret(const QString &secret, const QString &confirm, const QString &kind)
{
    if (m_state != QLatin1String("setup") || !checkNewSecret(secret, confirm, kind)) return false;
    const QList<OtpAccount> old = loadUnprotected();
    if (!storeNewSecret(secret, kind)) return false;
    m_accounts->setAccounts(old);
    saveAccounts();
    // Only once the accounts are safely encrypted does the plain copy go.
    if (m_settings->contains(QLatin1String(KeyVaultData))) m_settings->remove(QLatin1String(KeyUnprotected));
    m_settings->sync();
    setState(QLatin1String("unlocked"));
    return true;
}

int AppController::retrySeconds() const
{
    const qint64 wait = m_settings->value(QLatin1String(KeyVaultRetryAt), 0).toLongLong() - nowSeconds();
    // A wait longer than the longest the rules give means the clock went back: allow it.
    return wait > 0 && wait <= 3600 ? int(wait) : 0;
}

bool AppController::unlock(const QString &secret)
{
    if (m_state != QLatin1String("locked")) return m_state == QLatin1String("unlocked");
    if (retrySeconds() > 0) {
        emit notice(tr("Too many wrong attempts. Try again in %1 s.").arg(retrySeconds()));
        return false;
    }
    VaultKeys keys;
    if (!verify(secret, &keys)) {
        const int failures = m_settings->value(QLatin1String(KeyVaultFailures), 0).toInt() + 1;
        m_settings->setValue(QLatin1String(KeyVaultFailures), failures);
        if (failures > FreeAttempts) {
            const int wait = qMin(3600, 30 << qMin(7, failures - FreeAttempts - 1));
            m_settings->setValue(QLatin1String(KeyVaultRetryAt), nowSeconds() + wait);
            emit notice(tr("Wrong. Try again in %1 s.").arg(wait));
        } else {
            emit notice(secretKind() == QLatin1String("pin") ? tr("Wrong PIN.") : tr("Wrong passphrase."));
        }
        m_settings->sync();
        return false;
    }
    QByteArray plain;
    const QByteArray sealed = QByteArray::fromBase64(m_settings->value(QLatin1String(KeyVaultData)).toByteArray());
    if (!sealed.isEmpty() && !Vault::open(keys, sealed, &plain)) {
        keys.clear();
        emit notice(tr("The stored accounts are damaged and cannot be read."));
        return false;
    }
    const QList<OtpAccount> list = accountsOf(plain);
    plain.fill('\0');
    m_settings->remove(QLatin1String(KeyVaultFailures));
    m_settings->remove(QLatin1String(KeyVaultRetryAt));
    m_settings->sync();
    m_keys.clear();
    m_keys = keys;
    m_accounts->setAccounts(list);
    setState(QLatin1String("unlocked"));
    return true;
}

void AppController::lock()
{
    if (m_state != QLatin1String("unlocked")) return;
    m_keys.clear();
    m_accounts->clearAccounts();
    setState(QLatin1String("locked"));
}

bool AppController::changeSecret(const QString &current, const QString &secret, const QString &confirm, const QString &kind)
{
    if (m_state != QLatin1String("unlocked") || !checkNewSecret(secret, confirm, kind)) return false;
    VaultKeys keys;
    if (!verify(current, &keys)) {
        emit notice(secretKind() == QLatin1String("pin") ? tr("The current PIN is wrong.") : tr("The current passphrase is wrong."));
        return false;
    }
    keys.clear();
    if (!storeNewSecret(secret, kind)) return false;
    saveAccounts();
    emit stateChanged();   // secretKind may have changed
    emit notice(kind == QLatin1String("pin") ? tr("PIN changed.") : tr("Passphrase changed."));
    return true;
}

void AppController::setForeground(bool foreground)
{
    if (!foreground) {
        m_backgroundSince = QDateTime::currentMSecsSinceEpoch();
        if (autoLock() == 0) lock();
        return;
    }
    if (m_backgroundSince && QDateTime::currentMSecsSinceEpoch() - m_backgroundSince >= qint64(autoLock()) * 1000)
        lock();
    m_backgroundSince = 0;
}

void AppController::saveAccounts()
{
    if (m_keys.isNull()) return;
    QByteArray plain = linksOf(m_accounts->accounts());
    const QByteArray sealed = Vault::seal(m_keys, plain);
    plain.fill('\0');
    if (sealed.isEmpty()) {
        emit notice(tr("The accounts could not be saved: the phone gave no random numbers."));
        return;
    }
    m_settings->setValue(QLatin1String(KeyVaultData), sealed.toBase64());
    m_settings->sync();
}

QByteArray AppController::linksOf(const QList<OtpAccount> &accounts)
{
    QStringList links;
    for (int i = 0; i < accounts.size(); ++i) links << Otp::toUri(accounts.at(i));
    return links.join(QLatin1String("\n")).toUtf8();
}

QList<OtpAccount> AppController::accountsOf(const QByteArray &links)
{
    QList<OtpAccount> list;
    const QStringList lines = QString::fromUtf8(links).split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        OtpAccount a;
        QString error;
        if (Otp::parseUri(lines.at(i), &a, &error)) list.append(a);
    }
    return list;
}

// -- backup -------------------------------------------------------------------------------------------

QString AppController::backupDir() const
{
#ifndef Q_OS_SYMBIAN
    // Desktop testing: keeps test backups out of the real folders.
    const QByteArray testDir = qgetenv("SA_BACKUP_DIR");
    if (!testDir.isEmpty()) return QDir::fromNativeSeparators(QString::fromLocal8Bit(testDir));
#endif
    const QString saved = m_settings->value(QLatin1String(KeyBackupDir)).toString();
    if (!saved.isEmpty()) return saved;
    // The memory card (E:) if there is one, else the phone memory.
    const QStringList choices = backupDirChoices();
    for (int i = 0; i < choices.size(); ++i)
        if (choices.at(i).startsWith(QLatin1String("E:"), Qt::CaseInsensitive)) return choices.at(i);
    return choices.isEmpty() ? QDir::homePath() + QLatin1String("/SimpleAuth") : choices.first();
}

void AppController::setBackupDir(const QString &dir)
{
    if (dir == backupDir()) return;
    m_settings->setValue(QLatin1String(KeyBackupDir), dir);
    m_settings->sync();
    emit backupDirChanged();
}

QStringList AppController::backupDirChoices() const
{
    QStringList out;
    const QFileInfoList drives = QDir::drives();
    for (int i = 0; i < drives.size(); ++i) {
        const QString root = drives.at(i).absolutePath();
#ifdef Q_OS_SYMBIAN
        // Z: is the ROM, D: the RAM disk that empties at every restart.
        const QChar letter = root.at(0).toUpper();
        if (letter == QLatin1Char('Z') || letter == QLatin1Char('D')) continue;
#endif
        if (!drives.at(i).isWritable()) continue;
        out << QDir::cleanPath(root + QLatin1String("/SimpleAuth"));
    }
    return out;
}

QString AppController::backup()
{
    if (m_state != QLatin1String("unlocked") || m_keys.isNull()) return QString();
    Vault::Backup b;
    b.passphrase = secretKind() == QLatin1String("passphrase");
    b.iterations = m_settings->value(QLatin1String(KeyVaultIterations)).toInt();
    b.salt = QByteArray::fromBase64(m_settings->value(QLatin1String(KeyVaultSalt)).toByteArray());
    QByteArray plain = linksOf(m_accounts->accounts());
    b.sealed = Vault::seal(m_keys, plain);
    plain.fill('\0');
    if (b.sealed.isEmpty() || b.salt.size() != Vault::SaltSize) {
        emit notice(tr("The backup could not be made: the phone gave no random numbers."));
        return QString();
    }

    const QString dir = backupDir();
    if (!QDir().mkpath(dir)) {
        emit notice(tr("Cannot create the folder %1.").arg(QDir::toNativeSeparators(dir)));
        return QString();
    }
    const QString name = QLatin1String("simpleauth-")
                         + QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd-hh-mm"))
                         + QLatin1String(".sauth");
    QFile file(dir + QLatin1Char('/') + name);
    const QByteArray data = Vault::packBackup(b);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(data) != data.size() || !file.flush()) {
        emit notice(tr("Cannot write %1.").arg(QDir::toNativeSeparators(file.fileName())));
        return QString();
    }
    file.close();
    emit notice(tr("%n account(s) backed up to %1.", "", m_accounts->accounts().size())
                    .arg(QDir::toNativeSeparators(file.fileName())));
    return name;
}

QStringList AppController::backupFiles() const
{
    QDir dir(backupDir());
    // The names hold the date, so reverse name order is newest first.
    return dir.entryList(QStringList() << QLatin1String("*.sauth"), QDir::Files, QDir::Name | QDir::Reversed);
}

QString AppController::backupKind(const QString &fileName) const
{
    QFile file(backupDir() + QLatin1Char('/') + fileName);
    Vault::Backup b;
    if (!file.open(QIODevice::ReadOnly) || !Vault::unpackBackup(file.readAll(), &b)) return QString();
    return b.passphrase ? QLatin1String("passphrase") : QLatin1String("pin");
}

bool AppController::restore(const QString &fileName, const QString &secret)
{
    if (m_state != QLatin1String("unlocked")) return false;
    QFile file(backupDir() + QLatin1Char('/') + fileName);
    Vault::Backup b;
    if (!file.open(QIODevice::ReadOnly) || !Vault::unpackBackup(file.readAll(), &b)) {
        emit notice(tr("%1 is not a SimpleAuth backup.").arg(fileName));
        return false;
    }
    VaultKeys keys = Vault::derive(secret, b.salt, b.iterations);
    QByteArray plain;
    const bool opened = Vault::open(keys, b.sealed, &plain);
    keys.clear();
    if (!opened) {
        emit notice(b.passphrase ? tr("Wrong passphrase for this backup (or the file is damaged).")
                                 : tr("Wrong PIN for this backup (or the file is damaged)."));
        return false;
    }
    const QList<OtpAccount> incoming = accountsOf(plain);
    plain.fill('\0');

    // The same key and kind is the same account, whatever it is called here.
    QList<OtpAccount> merged = m_accounts->accounts();
    int added = 0, known = 0;
    for (int i = 0; i < incoming.size(); ++i) {
        const OtpAccount &in = incoming.at(i);
        int found = -1;
        for (int j = 0; j < merged.size() && found < 0; ++j)
            if (merged.at(j).secret == in.secret && merged.at(j).hotp == in.hotp) found = j;
        if (found < 0) {
            merged.append(in);
            ++added;
        } else {
            ++known;
            // A counter never goes back: a code already used must not come round again.
            if (in.hotp && in.counter > merged.at(found).counter) merged[found].counter = in.counter;
        }
    }
    m_accounts->setAccounts(merged);
    saveAccounts();
    emit notice(tr("Restored: %1 added, %2 already here.").arg(added).arg(known));
    return true;
}

QList<OtpAccount> AppController::loadUnprotected() const
{
    QList<OtpAccount> list;
    const int n = m_settings->beginReadArray(QLatin1String(KeyUnprotected));
    for (int i = 0; i < n; ++i) {
        m_settings->setArrayIndex(i);
        OtpAccount a;
        QString error;
        if (Otp::parseUri(m_settings->value(QLatin1String("link")).toString(), &a, &error)) list.append(a);
    }
    m_settings->endArray();
    return list;
}

// -- settings ---------------------------------------------------------------------------------------

QString AppController::version() const
{
    return QString::fromLatin1(SA_STRINGIFY(APP_VERSION));
}

void AppController::setTimeOffset(int seconds)
{
    if (seconds == m_timeOffset) return;
    m_timeOffset = seconds;
    m_settings->setValue(QLatin1String(KeyTimeOffset), seconds);
    m_settings->sync();
    m_accounts->setTimeOffset(seconds);
    emit timeOffsetChanged();
}

int AppController::cameraRotation() const
{
    const QVariant saved = m_settings->value(QLatin1String(KeyCameraRotation));
    if (saved.isValid()) return saved.toInt();
    // Checked on phones: Belle FP2 (Qt 4.8) upright, Belle Refresh (Qt 4.7.4) sideways.
    return QString::fromLatin1(qVersion()).startsWith(QLatin1String("4.7")) ? 90 : 0;
}

void AppController::setCameraRotation(int degrees)
{
    degrees = ((degrees % 360) + 360) % 360 / 90 * 90;
    if (degrees == cameraRotation()) return;
    m_settings->setValue(QLatin1String(KeyCameraRotation), degrees);
    m_settings->sync();
    emit cameraRotationChanged();
}

QString AppController::cameraFocusMode() const
{
    return m_settings->value(QLatin1String(KeyCameraFocus)).toString();
}

void AppController::setCameraFocusMode(const QString &mode)
{
    if (mode == cameraFocusMode()) return;
    m_settings->setValue(QLatin1String(KeyCameraFocus), mode);
    m_settings->sync();
    emit cameraSettingsChanged();
}

qreal AppController::cameraZoom() const
{
    return qMax(qreal(1), m_settings->value(QLatin1String(KeyCameraZoom), 1.0).toReal());
}

void AppController::setCameraZoom(qreal zoom)
{
    zoom = qMax(qreal(1), zoom);
    if (qAbs(zoom - cameraZoom()) < 0.01) return;
    m_settings->setValue(QLatin1String(KeyCameraZoom), zoom);
    emit cameraSettingsChanged();
}

bool AppController::addAccount(const QString &issuer, const QString &name, const QString &secret,
                               const QString &type, const QString &algorithm, int digits, int period,
                               const QString &counter)
{
    if (m_state != QLatin1String("unlocked")) return false;
    OtpAccount a;
    a.issuer = issuer.trimmed();
    a.name = name.trimmed();
    bool ok = false;
    a.secret = Otp::base32Decode(secret, &ok);
    a.hotp = type == QLatin1String("hotp");
    Hash::fromName(algorithm, &a.algorithm);
    a.digits = digits;
    a.period = period;
    if (a.hotp) {
        const QString c = counter.trimmed();
        a.counter = c.isEmpty() ? 0 : c.toULongLong(&ok);
        if (!ok) {
            emit notice(tr("The counter must be a whole number."));
            return false;
        }
    }
    const QString problem = Otp::validate(a);
    if (!problem.isEmpty()) {
        emit notice(problem);
        return false;
    }
    m_accounts->add(a);
    emit notice(tr("Added %1.").arg(a.issuer.isEmpty() ? a.name : a.issuer));
    return true;
}

bool AppController::addFromLink(const QString &link)
{
    if (m_state != QLatin1String("unlocked")) return false;
    OtpAccount a;
    QString error;
    if (!Otp::parseUri(link, &a, &error)) {
        emit notice(error);
        return false;
    }
    m_accounts->add(a);
    emit notice(tr("Added %1.").arg(a.issuer.isEmpty() ? a.name : a.issuer));
    return true;
}

void AppController::copyCode(int row)
{
    const QString code = m_accounts->code(row);
    if (code.isEmpty()) return;
    QApplication::clipboard()->setText(code);
    emit notice(tr("Code for %1 copied.").arg(m_accounts->title(row)));
}

void AppController::copyText(const QString &text)
{
    QApplication::clipboard()->setText(text);
}

QString AppController::utcTime() const
{
    return QDateTime::fromTime_t(uint(m_accounts->now())).toUTC().toString(QLatin1String("HH:mm:ss"));
}

QString AppController::zoneOffset() const
{
    const QDateTime local = QDateTime::currentDateTime();
    QDateTime asUtc = local.toUTC();
    asUtc.setTimeSpec(Qt::LocalTime);
    const int minutes = asUtc.secsTo(local) / 60;
    const int a = qAbs(minutes);
    return QString::fromLatin1("%1%2:%3").arg(QLatin1Char(minutes < 0 ? '-' : '+'))
        .arg(a / 60, 2, 10, QLatin1Char('0')).arg(a % 60, 2, 10, QLatin1Char('0'));
}

void AppController::screenshot(const QString &name)
{
#ifndef Q_OS_SYMBIAN
    const QByteArray dir = qgetenv("SA_SHOT_DIR");
    if (dir.isEmpty() || !m_view) return;
    QPixmap::grabWidget(m_view).save(QString::fromLocal8Bit(dir) + QLatin1Char('/') + name + QLatin1String(".png"));
#else
    Q_UNUSED(name);
#endif
}
