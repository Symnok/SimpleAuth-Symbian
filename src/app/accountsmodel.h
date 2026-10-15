// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// The account list the main page shows: each row's current code and the seconds left in
// its period, refreshed as the clock ticks. It holds the accounts only while the app is
// unlocked; the controller stores them, encrypted, whenever edited() is emitted.
#ifndef ACCOUNTSMODEL_H
#define ACCOUNTSMODEL_H

#include "otp.h"

#include <QAbstractListModel>
#include <QList>

class QTimer;

class AccountsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IssuerRole = Qt::UserRole + 1,
        NameRole,
        CodeRole,       ///< grouped for reading: "123 456"
        HotpRole,
        RemainingRole,  ///< TOTP: whole seconds left in the period
        PeriodRole,
        CounterRole
    };

    explicit AccountsModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    int count() const { return m_accounts.size(); }

    void add(const OtpAccount &account);
    /// Unlocking: the decrypted accounts. Locking: clearAccounts() wipes them.
    void setAccounts(const QList<OtpAccount> &accounts);
    void clearAccounts();
    const QList<OtpAccount> &accounts() const { return m_accounts; }
    /// Seconds added to the phone's clock before computing codes.
    void setTimeOffset(int seconds);

    /// The code without spaces, for the clipboard.
    Q_INVOKABLE QString code(int row) const;
    Q_INVOKABLE QString title(int row) const;
    /// The otpauth:// link (for "Show QR code"); HOTP links carry the current counter.
    Q_INVOKABLE QString link(int row) const;
    /// For the edit page.
    Q_INVOKABLE QString issuer(int row) const;
    Q_INVOKABLE QString name(int row) const;
    /// The key in Base32, as sites show it (no spaces).
    Q_INVOKABLE QString secret(int row) const;
    /// "Time-based, SHA1, 6 digits, every 30 s" and the like.
    Q_INVOKABLE QString details(int row) const;
    /// Renames an account; false (unchanged) if both names would be empty.
    Q_INVOKABLE bool setNames(int row, const QString &issuer, const QString &name);
    Q_INVOKABLE void remove(int row);
    /// HOTP: steps the counter, which makes the next code.
    Q_INVOKABLE void nextCode(int row);

    /// The corrected current Unix time the codes are made for.
    quint64 now() const;

signals:
    void countChanged();
    /// An account was added, removed or its counter stepped: time to store the list.
    void edited();

private slots:
    void tick();

private:
    void wipe();

    QList<OtpAccount> m_accounts;
    QTimer *m_timer;
    int m_timeOffset;
    quint64 m_lastSecond;
};

#endif // ACCOUNTSMODEL_H
