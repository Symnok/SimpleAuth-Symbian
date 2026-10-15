// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#include "qrcodeitem.h"
#include "qrcode.h"

#include <QPainter>

QrCodeItem::QrCodeItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent), m_size(0)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
}

void QrCodeItem::setText(const QString &text)
{
    if (text == m_text) return;
    m_text = text;
    m_modules.clear();
    m_size = 0;
    if (!text.isEmpty() && !QrCode::encode(text, m_modules, m_size)) m_size = 0;
    update();
    emit textChanged();
}

void QrCodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_size <= 0) return;
    const int quiet = 4;
    const int side = int(qMin(width(), height()));
    const int module = side / (m_size + 2 * quiet);
    if (module < 1) return;
    const int total = module * (m_size + 2 * quiet);
    const int x0 = (int(width()) - total) / 2, y0 = (int(height()) - total) / 2;

    painter->fillRect(x0, y0, total, total, Qt::white);
    const int left = x0 + quiet * module, top = y0 + quiet * module;
    for (int r = 0; r < m_size; ++r)
        for (int c = 0; c < m_size; ++c)
            if (m_modules.at(r * m_size + c))
                painter->fillRect(left + c * module, top + r * module, module, module, Qt::black);
}
