// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Decodes every sample listed in <dir>/expected.txt at the size the camera page uses and
// compares the text. Exit code = number of failures.
#include "qrcode.h"
#include "qrreader.h"

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QTime>
#include <cstdio>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv, false);
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString::fromLatin1("samples");
    QFile list(dir + QLatin1String("/expected.txt"));
    if (!list.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::printf("cannot open %s\n", qPrintable(list.fileName()));
        return 1;
    }
    int failures = 0, total = 0, slowest = 0;
    while (!list.atEnd()) {
        const QString line = QString::fromUtf8(list.readLine()).trimmed();
        if (line.isEmpty()) continue;
        const int space = line.indexOf(QLatin1Char(' '));
        const QString name = space < 0 ? line : line.left(space);
        const QString want = space < 0 ? QString() : line.mid(space + 1);
        const QImage image(dir + QLatin1Char('/') + name);
        QTime t;
        t.start();
        // 480: the longer side the camera page decodes at.
        const QString got = QrReader::decode(image, 480);
        const int ms = t.elapsed();
        slowest = qMax(slowest, ms);
        ++total;
        const bool ok = got == want;
        if (!ok) ++failures;
        std::printf("%s %-24s %4d ms  %s\n", ok ? "ok  " : "FAIL", qPrintable(name), ms,
                    ok ? "" : qPrintable(QString::fromLatin1("got \"%1\"").arg(got)));
    }
    // Round trip: the encoder behind "Show QR code" against the reader behind the camera,
    // with links up to the longest an account makes (a 64-byte SHA512 key).
    const char *links[] = {
        "otpauth://totp/Google:alice@gmail.com?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=Google",
        "otpauth://hotp/Example:carol?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=Example&counter=12345",
        "otpauth://totp/A%20Rather%20Long%20Service%20Name:someone.with.a.long.address@example.com?secret="
        "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"
        "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=A%20Rather%20Long%20Service%20Name&algorithm=SHA512&digits=8&period=60",
    };
    for (unsigned i = 0; i < sizeof links / sizeof links[0]; ++i) {
        const QString link = QString::fromLatin1(links[i]);
        QVector<bool> modules;
        int n = 0;
        QString got;
        if (QrCode::encode(link, modules, n)) {
            const int m = 4, quiet = 4, side = (n + 2 * quiet) * m;
            QImage img(side, side, QImage::Format_RGB32);
            img.fill(0xffffffff);
            for (int r = 0; r < n; ++r)
                for (int c = 0; c < n; ++c)
                    if (modules.at(r * n + c))
                        for (int y = 0; y < m; ++y)
                            for (int x = 0; x < m; ++x)
                                img.setPixel((c + quiet) * m + x, (r + quiet) * m + y, 0xff000000);
            got = QrReader::decode(img, 800);
        }
        ++total;
        const bool ok = got == link;
        if (!ok) ++failures;
        std::printf("%s round trip %u (%d chars, %dx%d modules)\n", ok ? "ok  " : "FAIL", i, link.size(), n, n);
    }

    std::printf("\n%s: %d of %d failed, slowest %d ms\n", failures ? "FAILED" : "PASSED", failures, total, slowest);
    return failures;
}
