// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "accountsmodel.h"

#include <QDateTime>
#include <QTimer>

namespace
{
/// "123456" -> "123 456", "12345678" -> "1234 5678": easier to read off and type.
QString grouped(const QString &code)
{
    const int half = code.size() / 2;
    return code.left(half) + QLatin1Char(' ') + code.mid(half);
}
}

AccountsModel::AccountsModel(QObject *parent)
    : QAbstractListModel(parent), m_timer(new QTimer(this)), m_timeOffset(0), m_lastSecond(0)
{
    QHash<int, QByteArray> roles;
    roles[IssuerRole] = "issuer";
    roles[NameRole] = "name";
    roles[CodeRole] = "code";
    roles[HotpRole] = "hotp";
    roles[RemainingRole] = "remaining";
    roles[PeriodRole] = "period";
    roles[CounterRole] = "counter";
    setRoleNames(roles);

    // A quarter-second poll, acting only when the second changes: the countdown then steps
    // close to the real second boundary without a timer re-aligned to it.
    m_timer->setInterval(250);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    m_timer->start();
}

quint64 AccountsModel::now() const
{
    const qint64 t = QDateTime::currentMSecsSinceEpoch() / 1000 + m_timeOffset;
    return t > 0 ? quint64(t) : 0;
}

int AccountsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_accounts.size();
}

QVariant AccountsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_accounts.size()) return QVariant();
    const OtpAccount &a = m_accounts.at(index.row());
    if (role == IssuerRole) return a.issuer;
    if (role == NameRole) return a.name;
    if (role == CodeRole) return grouped(Otp::code(a, now()));
    if (role == HotpRole) return a.hotp;
    if (role == RemainingRole) return a.hotp ? 0 : int(a.period - now() % quint64(a.period));
    if (role == PeriodRole) return a.period;
    if (role == CounterRole) return QString::number(a.counter);
    return QVariant();
}

void AccountsModel::tick()
{
    const quint64 second = now();
    if (second == m_lastSecond || m_accounts.isEmpty()) return;
    m_lastSecond = second;
    // Every TOTP row's countdown moves each second; codes are cheap to recompute (one HMAC
    // per row), so the whole list is refreshed.
    emit dataChanged(index(0), index(m_accounts.size() - 1));
}

void AccountsModel::setTimeOffset(int seconds)
{
    m_timeOffset = seconds;
    m_lastSecond = 0;
    tick();
}

void AccountsModel::add(const OtpAccount &account)
{
    beginInsertRows(QModelIndex(), m_accounts.size(), m_accounts.size());
    m_accounts.append(account);
    endInsertRows();
    emit edited();
    emit countChanged();
}

QString AccountsModel::code(int row) const
{
    if (row < 0 || row >= m_accounts.size()) return QString();
    return Otp::code(m_accounts.at(row), now());
}

QString AccountsModel::title(int row) const
{
    if (row < 0 || row >= m_accounts.size()) return QString();
    const OtpAccount &a = m_accounts.at(row);
    return a.issuer.isEmpty() ? a.name : a.issuer;
}

QString AccountsModel::link(int row) const
{
    if (row < 0 || row >= m_accounts.size()) return QString();
    return Otp::toUri(m_accounts.at(row));
}

QString AccountsModel::issuer(int row) const
{
    return row >= 0 && row < m_accounts.size() ? m_accounts.at(row).issuer : QString();
}

QString AccountsModel::name(int row) const
{
    return row >= 0 && row < m_accounts.size() ? m_accounts.at(row).name : QString();
}

QString AccountsModel::secret(int row) const
{
    return row >= 0 && row < m_accounts.size() ? Otp::base32Encode(m_accounts.at(row).secret) : QString();
}

QString AccountsModel::details(int row) const
{
    if (row < 0 || row >= m_accounts.size()) return QString();
    const OtpAccount &a = m_accounts.at(row);
    if (a.hotp)
        return tr("Counter-based, %1, %2 digits, counter %3").arg(Hash::name(a.algorithm)).arg(a.digits).arg(a.counter);
    return tr("Time-based, %1, %2 digits, every %3 s").arg(Hash::name(a.algorithm)).arg(a.digits).arg(a.period);
}

bool AccountsModel::setNames(int row, const QString &issuer, const QString &name)
{
    if (row < 0 || row >= m_accounts.size()) return false;
    const QString i = issuer.trimmed(), n = name.trimmed();
    if (i.isEmpty() && n.isEmpty()) return false;
    OtpAccount &a = m_accounts[row];
    if (a.issuer == i && a.name == n) return true;
    a.issuer = i;
    a.name = n;
    emit dataChanged(index(row), index(row));
    emit edited();
    return true;
}

void AccountsModel::remove(int row)
{
    if (row < 0 || row >= m_accounts.size()) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_accounts.removeAt(row);
    endRemoveRows();
    emit edited();
    emit countChanged();
}

void AccountsModel::nextCode(int row)
{
    if (row < 0 || row >= m_accounts.size() || !m_accounts.at(row).hotp) return;
    ++m_accounts[row].counter;
    emit edited();
    emit dataChanged(index(row), index(row));
}

void AccountsModel::setAccounts(const QList<OtpAccount> &accounts)
{
    beginResetModel();
    wipe();
    m_accounts = accounts;
    endResetModel();
    emit countChanged();
}

void AccountsModel::clearAccounts()
{
    beginResetModel();
    wipe();
    endResetModel();
    emit countChanged();
}

void AccountsModel::wipe()
{
    // Locking: the keys should not linger in freed memory.
    for (int i = 0; i < m_accounts.size(); ++i) m_accounts[i].secret.fill(char(0));
    m_accounts.clear();
}
