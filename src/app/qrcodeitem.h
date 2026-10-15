// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// A QML item that draws text as a QR code, square and centred, with whole-pixel modules
// (so a camera reads it off the screen) and the white quiet zone around it.
#ifndef QRCODEITEM_H
#define QRCODEITEM_H

#include <QDeclarativeItem>
#include <QVector>

class QrCodeItem : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// False when the text is too long for a code (the item then stays blank).
    Q_PROPERTY(bool valid READ isValid NOTIFY textChanged)

public:
    explicit QrCodeItem(QDeclarativeItem *parent = 0);

    QString text() const { return m_text; }
    void setText(const QString &text);
    bool isValid() const { return m_size > 0; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

signals:
    void textChanged();

private:
    QString m_text;
    QVector<bool> m_modules;
    int m_size;
};

#endif // QRCODEITEM_H
