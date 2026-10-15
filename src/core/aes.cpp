// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Table-driven AES-256 encryption, from Symbigram. The S-box and round tables are computed
// at first use rather than typed in, so there is no long constant to mistype.
#include "aes.h"

#include <cstring>

namespace
{
    uchar SBox[256];
    quint32 Te0[256], Te1[256], Te2[256], Te3[256];
    bool tablesReady = false;

    inline uchar xtime(uchar a) { return uchar((a << 1) ^ ((a & 0x80) ? 0x1B : 0)); }
    inline uchar gmul(uchar a, uchar b)
    {
        uchar r = 0;
        while (b) { if (b & 1) r ^= a; a = xtime(a); b >>= 1; }
        return r;
    }
    inline quint32 rotl8(quint32 x) { return (x << 8) | (x >> 24); }

    void buildTables()
    {
        if (tablesReady) return;
        uchar pow[256], log[256];
        uchar x = 1;
        for (int i = 0; i < 256; ++i) {
            pow[i] = x;
            log[x] = uchar(i);
            x = uchar(x ^ xtime(x));           // multiply by 3
        }
        SBox[0] = 0x63;
        for (int i = 1; i < 256; ++i) {
            uchar inv = pow[255 - log[i]];
            uchar s = inv, r = inv;
            for (int t = 0; t < 4; ++t) {
                r = uchar((r << 1) | (r >> 7));
                s ^= r;
            }
            SBox[i] = uchar(s ^ 0x63);
        }
        for (int i = 0; i < 256; ++i) {
            uchar s = SBox[i];
            quint32 t = (quint32(gmul(s, 2)) << 24) | (quint32(s) << 16) | (quint32(s) << 8) | quint32(gmul(s, 3));
            // Te0 = (2s, s, s, 3s) as a big-endian word. Rotating it left by a byte three
            // times yields the textbook Te1, so the tables below are stored in reverse
            // order: this Te1 is the textbook Te3 and this Te3 the textbook Te1. The
            // round function indexes them accordingly.
            Te0[i] = t; Te1[i] = rotl8(t); Te2[i] = rotl8(Te1[i]); Te3[i] = rotl8(Te2[i]);
        }
        tablesReady = true;
    }

    const quint32 Rcon[15] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D };

    inline quint32 subWord(quint32 w)
    {
        return (quint32(SBox[w >> 24]) << 24) | (quint32(SBox[(w >> 16) & 0xff]) << 16) | (quint32(SBox[(w >> 8) & 0xff]) << 8) | quint32(SBox[w & 0xff]);
    }

    inline quint32 load(const uchar *p) { return (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]); }
    inline void store(uchar *p, quint32 w) { p[0] = uchar(w >> 24); p[1] = uchar(w >> 16); p[2] = uchar(w >> 8); p[3] = uchar(w); }

    struct Aes256
    {
        quint32 ek[60];

        explicit Aes256(const QByteArray &key)
        {
            buildTables();
            const uchar *k = reinterpret_cast<const uchar *>(key.constData());
            for (int i = 0; i < 8; ++i) ek[i] = load(k + 4 * i);
            for (int i = 8; i < 60; ++i) {
                quint32 t = ek[i - 1];
                if (i % 8 == 0) t = subWord(rotl8(t)) ^ (Rcon[i / 8] << 24);
                else if (i % 8 == 4) t = subWord(t);
                ek[i] = ek[i - 8] ^ t;
            }
        }

        ~Aes256() { memset(ek, 0, sizeof ek); }

        void encrypt(const uchar *in, uchar *out) const
        {
            quint32 s0 = load(in) ^ ek[0], s1 = load(in + 4) ^ ek[1], s2 = load(in + 8) ^ ek[2], s3 = load(in + 12) ^ ek[3];
            quint32 t0, t1, t2, t3;
            for (int r = 1; r < 14; ++r) {
                const quint32 *rk = ek + 4 * r;
                t0 = Te0[s0 >> 24] ^ Te3[(s1 >> 16) & 0xff] ^ Te2[(s2 >> 8) & 0xff] ^ Te1[s3 & 0xff] ^ rk[0];
                t1 = Te0[s1 >> 24] ^ Te3[(s2 >> 16) & 0xff] ^ Te2[(s3 >> 8) & 0xff] ^ Te1[s0 & 0xff] ^ rk[1];
                t2 = Te0[s2 >> 24] ^ Te3[(s3 >> 16) & 0xff] ^ Te2[(s0 >> 8) & 0xff] ^ Te1[s1 & 0xff] ^ rk[2];
                t3 = Te0[s3 >> 24] ^ Te3[(s0 >> 16) & 0xff] ^ Te2[(s1 >> 8) & 0xff] ^ Te1[s2 & 0xff] ^ rk[3];
                s0 = t0; s1 = t1; s2 = t2; s3 = t3;
            }
            const quint32 *rk = ek + 56;
            t0 = (quint32(SBox[s0 >> 24]) << 24) ^ (quint32(SBox[(s1 >> 16) & 0xff]) << 16) ^ (quint32(SBox[(s2 >> 8) & 0xff]) << 8) ^ quint32(SBox[s3 & 0xff]) ^ rk[0];
            t1 = (quint32(SBox[s1 >> 24]) << 24) ^ (quint32(SBox[(s2 >> 16) & 0xff]) << 16) ^ (quint32(SBox[(s3 >> 8) & 0xff]) << 8) ^ quint32(SBox[s0 & 0xff]) ^ rk[1];
            t2 = (quint32(SBox[s2 >> 24]) << 24) ^ (quint32(SBox[(s3 >> 16) & 0xff]) << 16) ^ (quint32(SBox[(s0 >> 8) & 0xff]) << 8) ^ quint32(SBox[s1 & 0xff]) ^ rk[2];
            t3 = (quint32(SBox[s3 >> 24]) << 24) ^ (quint32(SBox[(s0 >> 16) & 0xff]) << 16) ^ (quint32(SBox[(s1 >> 8) & 0xff]) << 8) ^ quint32(SBox[s2 & 0xff]) ^ rk[3];
            store(out, t0); store(out + 4, t1); store(out + 8, t2); store(out + 12, t3);
        }
    };
}

QByteArray Aes::encryptBlock(const QByteArray &key32, const QByteArray &block16)
{
    if (key32.size() != 32 || block16.size() != 16) return QByteArray();
    Aes256 aes(key32);
    QByteArray out(16, '\0');
    aes.encrypt(reinterpret_cast<const uchar *>(block16.constData()), reinterpret_cast<uchar *>(out.data()));
    return out;
}

QByteArray Aes::ctr(const QByteArray &key32, const QByteArray &counter16, const QByteArray &data)
{
    if (key32.size() != 32 || counter16.size() != 16) return QByteArray();
    Aes256 aes(key32);
    uchar counter[16], stream[16];
    memcpy(counter, counter16.constData(), 16);
    QByteArray out = data;
    uchar *p = reinterpret_cast<uchar *>(out.data());
    for (int off = 0; off < out.size(); off += 16) {
        aes.encrypt(counter, stream);
        const int n = qMin(16, out.size() - off);
        for (int i = 0; i < n; ++i) p[off + i] ^= stream[i];
        for (int i = 15; i >= 0 && ++counter[i] == 0; --i) {}
    }
    memset(stream, 0, sizeof stream);
    return out;
}
