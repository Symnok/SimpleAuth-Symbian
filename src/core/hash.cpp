// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// SHA-256 and SHA-512 (FIPS 180-4) and HMAC. SHA-1 is QCryptographicHash. The SHA-2 code
// comes from Symbigram's core.
#include "hash.h"

#include <QCryptographicHash>
#include <QString>
#include <cstring>

QByteArray Hash::sha1(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha1);
}

// -- SHA-256 ----------------------------------------------------------------------------------------

namespace
{
    const quint32 K256[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    inline quint32 rotr32(quint32 x, int n) { return (x >> n) | (x << (32 - n)); }

    void sha256Block(quint32 h[8], const uchar *p)
    {
        quint32 w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (quint32(p[i * 4]) << 24) | (quint32(p[i * 4 + 1]) << 16) | (quint32(p[i * 4 + 2]) << 8) | quint32(p[i * 4 + 3]);
        for (int i = 16; i < 64; ++i) {
            quint32 s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            quint32 s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        quint32 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            quint32 S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            quint32 ch = (e & f) ^ (~e & g);
            quint32 t1 = hh + S1 + ch + K256[i] + w[i];
            quint32 S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            quint32 maj = (a & b) ^ (a & c) ^ (b & c);
            quint32 t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
}

QByteArray Hash::sha256(const QByteArray &data)
{
    quint32 h[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    const int len = data.size();
    int off = 0;
    for (; off + 64 <= len; off += 64) sha256Block(h, p + off);

    uchar tail[128];
    int rest = len - off;
    memcpy(tail, p + off, rest);
    tail[rest++] = 0x80;
    int padTo = rest <= 56 ? 64 : 128;
    memset(tail + rest, 0, padTo - rest);
    quint64 bits = quint64(len) * 8;
    for (int i = 0; i < 8; ++i) tail[padTo - 1 - i] = uchar(bits >> (8 * i));
    sha256Block(h, tail);
    if (padTo == 128) sha256Block(h, tail + 64);

    QByteArray out(32, '\0');
    for (int i = 0; i < 8; ++i) {
        out[i * 4] = char(h[i] >> 24);
        out[i * 4 + 1] = char(h[i] >> 16);
        out[i * 4 + 2] = char(h[i] >> 8);
        out[i * 4 + 3] = char(h[i]);
    }
    return out;
}

// -- SHA-512 ----------------------------------------------------------------------------------------

namespace
{
    const quint64 K512[80] = {
        Q_UINT64_C(0x428a2f98d728ae22), Q_UINT64_C(0x7137449123ef65cd), Q_UINT64_C(0xb5c0fbcfec4d3b2f), Q_UINT64_C(0xe9b5dba58189dbbc),
        Q_UINT64_C(0x3956c25bf348b538), Q_UINT64_C(0x59f111f1b605d019), Q_UINT64_C(0x923f82a4af194f9b), Q_UINT64_C(0xab1c5ed5da6d8118),
        Q_UINT64_C(0xd807aa98a3030242), Q_UINT64_C(0x12835b0145706fbe), Q_UINT64_C(0x243185be4ee4b28c), Q_UINT64_C(0x550c7dc3d5ffb4e2),
        Q_UINT64_C(0x72be5d74f27b896f), Q_UINT64_C(0x80deb1fe3b1696b1), Q_UINT64_C(0x9bdc06a725c71235), Q_UINT64_C(0xc19bf174cf692694),
        Q_UINT64_C(0xe49b69c19ef14ad2), Q_UINT64_C(0xefbe4786384f25e3), Q_UINT64_C(0x0fc19dc68b8cd5b5), Q_UINT64_C(0x240ca1cc77ac9c65),
        Q_UINT64_C(0x2de92c6f592b0275), Q_UINT64_C(0x4a7484aa6ea6e483), Q_UINT64_C(0x5cb0a9dcbd41fbd4), Q_UINT64_C(0x76f988da831153b5),
        Q_UINT64_C(0x983e5152ee66dfab), Q_UINT64_C(0xa831c66d2db43210), Q_UINT64_C(0xb00327c898fb213f), Q_UINT64_C(0xbf597fc7beef0ee4),
        Q_UINT64_C(0xc6e00bf33da88fc2), Q_UINT64_C(0xd5a79147930aa725), Q_UINT64_C(0x06ca6351e003826f), Q_UINT64_C(0x142929670a0e6e70),
        Q_UINT64_C(0x27b70a8546d22ffc), Q_UINT64_C(0x2e1b21385c26c926), Q_UINT64_C(0x4d2c6dfc5ac42aed), Q_UINT64_C(0x53380d139d95b3df),
        Q_UINT64_C(0x650a73548baf63de), Q_UINT64_C(0x766a0abb3c77b2a8), Q_UINT64_C(0x81c2c92e47edaee6), Q_UINT64_C(0x92722c851482353b),
        Q_UINT64_C(0xa2bfe8a14cf10364), Q_UINT64_C(0xa81a664bbc423001), Q_UINT64_C(0xc24b8b70d0f89791), Q_UINT64_C(0xc76c51a30654be30),
        Q_UINT64_C(0xd192e819d6ef5218), Q_UINT64_C(0xd69906245565a910), Q_UINT64_C(0xf40e35855771202a), Q_UINT64_C(0x106aa07032bbd1b8),
        Q_UINT64_C(0x19a4c116b8d2d0c8), Q_UINT64_C(0x1e376c085141ab53), Q_UINT64_C(0x2748774cdf8eeb99), Q_UINT64_C(0x34b0bcb5e19b48a8),
        Q_UINT64_C(0x391c0cb3c5c95a63), Q_UINT64_C(0x4ed8aa4ae3418acb), Q_UINT64_C(0x5b9cca4f7763e373), Q_UINT64_C(0x682e6ff3d6b2b8a3),
        Q_UINT64_C(0x748f82ee5defb2fc), Q_UINT64_C(0x78a5636f43172f60), Q_UINT64_C(0x84c87814a1f0ab72), Q_UINT64_C(0x8cc702081a6439ec),
        Q_UINT64_C(0x90befffa23631e28), Q_UINT64_C(0xa4506cebde82bde9), Q_UINT64_C(0xbef9a3f7b2c67915), Q_UINT64_C(0xc67178f2e372532b),
        Q_UINT64_C(0xca273eceea26619c), Q_UINT64_C(0xd186b8c721c0c207), Q_UINT64_C(0xeada7dd6cde0eb1e), Q_UINT64_C(0xf57d4f7fee6ed178),
        Q_UINT64_C(0x06f067aa72176fba), Q_UINT64_C(0x0a637dc5a2c898a6), Q_UINT64_C(0x113f9804bef90dae), Q_UINT64_C(0x1b710b35131c471b),
        Q_UINT64_C(0x28db77f523047d84), Q_UINT64_C(0x32caab7b40c72493), Q_UINT64_C(0x3c9ebe0a15c9bebc), Q_UINT64_C(0x431d67c49c100d4c),
        Q_UINT64_C(0x4cc5d4becb3e42b6), Q_UINT64_C(0x597f299cfc657e2a), Q_UINT64_C(0x5fcb6fab3ad6faec), Q_UINT64_C(0x6c44198c4a475817)
    };

    inline quint64 rotr64(quint64 x, int n) { return (x >> n) | (x << (64 - n)); }

    struct Sha512State
    {
        quint64 h[8];
        uchar buf[128];
        int bufLen;
        quint64 total;

        void init()
        {
            h[0] = Q_UINT64_C(0x6a09e667f3bcc908); h[1] = Q_UINT64_C(0xbb67ae8584caa73b);
            h[2] = Q_UINT64_C(0x3c6ef372fe94f82b); h[3] = Q_UINT64_C(0xa54ff53a5f1d36f1);
            h[4] = Q_UINT64_C(0x510e527fade682d1); h[5] = Q_UINT64_C(0x9b05688c2b3e6c1f);
            h[6] = Q_UINT64_C(0x1f83d9abfb41bd6b); h[7] = Q_UINT64_C(0x5be0cd19137e2179);
            bufLen = 0;
            total = 0;
        }

        void block(const uchar *p)
        {
            quint64 w[80];
            for (int i = 0; i < 16; ++i) {
                quint64 v = 0;
                for (int j = 0; j < 8; ++j) v = (v << 8) | p[i * 8 + j];
                w[i] = v;
            }
            for (int i = 16; i < 80; ++i) {
                quint64 s0 = rotr64(w[i - 15], 1) ^ rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
                quint64 s1 = rotr64(w[i - 2], 19) ^ rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            quint64 a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
            for (int i = 0; i < 80; ++i) {
                quint64 S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
                quint64 ch = (e & f) ^ (~e & g);
                quint64 t1 = hh + S1 + ch + K512[i] + w[i];
                quint64 S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
                quint64 maj = (a & b) ^ (a & c) ^ (b & c);
                quint64 t2 = S0 + maj;
                hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        void update(const uchar *p, int len)
        {
            total += quint64(len);
            if (bufLen) {
                int take = qMin(128 - bufLen, len);
                memcpy(buf + bufLen, p, take);
                bufLen += take; p += take; len -= take;
                if (bufLen < 128) return;
                block(buf);
                bufLen = 0;
            }
            while (len >= 128) { block(p); p += 128; len -= 128; }
            if (len) { memcpy(buf, p, len); bufLen = len; }
        }

        void finish(uchar out[64])
        {
            uchar tail[256];
            int rest = bufLen;
            memcpy(tail, buf, rest);
            tail[rest++] = 0x80;
            int padTo = rest <= 112 ? 128 : 256;
            memset(tail + rest, 0, padTo - rest);
            quint64 bits = total * 8;
            for (int i = 0; i < 8; ++i) tail[padTo - 1 - i] = uchar(bits >> (8 * i));
            block(tail);
            if (padTo == 256) block(tail + 128);
            for (int i = 0; i < 8; ++i)
                for (int j = 0; j < 8; ++j) out[i * 8 + j] = uchar(h[i] >> (56 - 8 * j));
        }
    };
}

QByteArray Hash::sha512(const QByteArray &data)
{
    Sha512State s;
    s.init();
    s.update(reinterpret_cast<const uchar *>(data.constData()), data.size());
    QByteArray out(64, '\0');
    s.finish(reinterpret_cast<uchar *>(out.data()));
    return out;
}

// -- PBKDF2 -----------------------------------------------------------------------------------------

namespace
{
    const quint32 Sha256Iv[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

    void storeState(const quint32 h[8], uchar *out)
    {
        for (int i = 0; i < 8; ++i) {
            out[i * 4] = uchar(h[i] >> 24);
            out[i * 4 + 1] = uchar(h[i] >> 16);
            out[i * 4 + 2] = uchar(h[i] >> 8);
            out[i * 4 + 3] = uchar(h[i]);
        }
    }
}

QByteArray Hash::pbkdf2Sha256(const QByteArray &password, const QByteArray &salt, int iterations, int length)
{
    // The HMAC key's two padded blocks are hashed once; every later HMAC of a 32-byte
    // message then starts from those states and needs one more block each.
    const QByteArray key = password.size() > 64 ? sha256(password) : password;
    uchar ipad[64], opad[64];
    memset(ipad, 0x36, 64);
    memset(opad, 0x5c, 64);
    for (int i = 0; i < key.size(); ++i) {
        ipad[i] ^= uchar(key.at(i));
        opad[i] ^= uchar(key.at(i));
    }
    quint32 inner[8], outer[8];
    memcpy(inner, Sha256Iv, sizeof inner);
    memcpy(outer, Sha256Iv, sizeof outer);
    sha256Block(inner, ipad);
    sha256Block(outer, opad);
    memset(ipad, 0, 64);
    memset(opad, 0, 64);

    // The block for a 32-byte message after a 64-byte key block: message, 0x80, zeros, and
    // the total length in bits (96 bytes = 768 = 0x300).
    uchar block[64];
    memset(block, 0, 64);
    block[32] = 0x80;
    block[62] = 0x03;

    QByteArray out;
    const int blocks = (length + 31) / 32;
    for (int b = 1; b <= blocks; ++b) {
        QByteArray seed = salt;
        seed.append(char(b >> 24)).append(char(b >> 16)).append(char(b >> 8)).append(char(b));
        const QByteArray first = hmac(Sha256, password, seed);
        uchar u[32], acc[32];
        memcpy(u, first.constData(), 32);
        memcpy(acc, u, 32);
        for (int i = 1; i < iterations; ++i) {
            quint32 h[8];
            memcpy(block, u, 32);
            memcpy(h, inner, sizeof h);
            sha256Block(h, block);
            storeState(h, block);
            memcpy(h, outer, sizeof h);
            sha256Block(h, block);
            storeState(h, u);
            for (int j = 0; j < 32; ++j) acc[j] ^= u[j];
        }
        out.append(reinterpret_cast<const char *>(acc), 32);
        memset(u, 0, 32);
        memset(acc, 0, 32);
    }
    memset(block, 0, 64);
    memset(inner, 0, sizeof inner);
    memset(outer, 0, sizeof outer);
    out.truncate(length);
    return out;
}

bool Hash::equalConstantTime(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size()) return false;
    int diff = 0;
    for (int i = 0; i < a.size(); ++i) diff |= uchar(a.at(i)) ^ uchar(b.at(i));
    return diff == 0;
}

// -- dispatch and HMAC ------------------------------------------------------------------------------

QByteArray Hash::hash(Algorithm algorithm, const QByteArray &data)
{
    if (algorithm == Sha256) return sha256(data);
    if (algorithm == Sha512) return sha512(data);
    return sha1(data);
}

QByteArray Hash::hmac(Algorithm algorithm, const QByteArray &key, const QByteArray &data)
{
    const int blockSize = algorithm == Sha512 ? 128 : 64;
    QByteArray k = key.size() > blockSize ? hash(algorithm, key) : key;
    k.append(QByteArray(blockSize - k.size(), '\0'));
    QByteArray ipad(blockSize, '\0'), opad(blockSize, '\0');
    for (int i = 0; i < blockSize; ++i) {
        ipad[i] = char(uchar(k.at(i)) ^ 0x36);
        opad[i] = char(uchar(k.at(i)) ^ 0x5c);
    }
    return hash(algorithm, opad + hash(algorithm, ipad + data));
}

QString Hash::name(Algorithm algorithm)
{
    if (algorithm == Sha256) return QString::fromLatin1("SHA256");
    if (algorithm == Sha512) return QString::fromLatin1("SHA512");
    return QString::fromLatin1("SHA1");
}

bool Hash::fromName(const QString &name, Algorithm *algorithm)
{
    const QString n = name.trimmed().toUpper().remove(QLatin1Char('-'));
    if (n == QLatin1String("SHA1")) *algorithm = Sha1;
    else if (n == QLatin1String("SHA256")) *algorithm = Sha256;
    else if (n == QLatin1String("SHA512")) *algorithm = Sha512;
    else return false;
    return true;
}
