// SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
// Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
//
// Checks the OTP core against the published test vectors. Exit code = number of failures.
//
//   otp-test --open-backup FILE SECRET   instead opens a .sauth backup (e.g. one made by the
//                                        Windows Phone version) and prints its accounts
#include "aes.h"
#include <QFile>
#include "otp.h"
#include "securerandom.h"
#include "vault.h"

#include <QCoreApplication>
#include <cstdio>

static int failures = 0;

static void check(const QString &what, const QString &got, const QString &want)
{
    const bool ok = got == want;
    if (!ok) ++failures;
    std::printf("%s %-40s %s%s\n", ok ? "ok  " : "FAIL", qPrintable(what), qPrintable(got),
                ok ? "" : qPrintable(QString::fromLatin1("  (want %1)").arg(want)));
}

static QString L(const char *s) { return QString::fromLatin1(s); }
static QString hex(const QByteArray &b) { return QString::fromLatin1(b.toHex()); }
static QString yes(bool b) { return L(b ? "ok" : "error"); }

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc == 4 && QString::fromLatin1(argv[1]) == L("--open-backup")) {
        QFile file(QString::fromLocal8Bit(argv[2]));
        Vault::Backup b;
        if (!file.open(QIODevice::ReadOnly) || !Vault::unpackBackup(file.readAll(), &b)) {
            std::printf("not a backup file\n");
            return 1;
        }
        VaultKeys keys = Vault::derive(QString::fromLocal8Bit(argv[3]), b.salt, b.iterations);
        QByteArray plain;
        if (!Vault::open(keys, b.sealed, &plain)) {
            std::printf("wrong secret (or damaged file)\n");
            return 1;
        }
        std::printf("%s, %d iterations\n%s\n", b.passphrase ? "passphrase" : "PIN", b.iterations, plain.constData());
        return 0;
    }

    // Hashes: FIPS 180 "abc"; RFC 2202 / RFC 4231 HMAC test case 2 (key "Jefe").
    check(L("sha256(abc)"), hex(Hash::sha256("abc")),
          L("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    check(L("sha512(abc)"), hex(Hash::sha512("abc")),
          L("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"));
    const QByteArray jefeMsg("what do ya want for nothing?");
    check(L("hmac-sha1 (RFC 2202 #2)"), hex(Hash::hmac(Hash::Sha1, "Jefe", jefeMsg)),
          L("effcdf6ae5eb2fa2d27416d5f184df9c259a7c79"));
    check(L("hmac-sha256 (RFC 4231 #2)"), hex(Hash::hmac(Hash::Sha256, "Jefe", jefeMsg)),
          L("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"));
    check(L("hmac-sha512 (RFC 4231 #2)"), hex(Hash::hmac(Hash::Sha512, "Jefe", jefeMsg)),
          L("164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737"));
    // RFC 4231 #6: a 131-byte key, longer than the block, is hashed first.
    check(L("hmac-sha256 long key (RFC 4231 #6)"),
          hex(Hash::hmac(Hash::Sha256, QByteArray(131, char(0xaa)), "Test Using Larger Than Block-Size Key - Hash Key First")),
          L("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"));

    // Base32 (RFC 4648 section 10), plus the forms sites print.
    const char *b32[][2] = { { "", "" }, { "f", "MY" }, { "fo", "MZXQ" }, { "foo", "MZXW6" },
                             { "foob", "MZXW6YQ" }, { "fooba", "MZXW6YTB" }, { "foobar", "MZXW6YTBOI" } };
    for (unsigned i = 0; i < sizeof b32 / sizeof b32[0]; ++i)
        check(L("base32 encode \"%1\"").arg(L(b32[i][0])), Otp::base32Encode(b32[i][0]), L(b32[i][1]));
    check(L("base32 decode padded"), QString::fromLatin1(Otp::base32Decode(L("MZXW6YTBOI======"))), L("foobar"));
    check(L("base32 decode spaced lowercase"), QString::fromLatin1(Otp::base32Decode(L("mzxw 6ytb oi"))), L("foobar"));
    bool ok = true;
    Otp::base32Decode(L("MZXW1"), &ok);
    check(L("base32 rejects '1'"), L(ok ? "accepted" : "rejected"), L("rejected"));

    // RFC 4226 appendix D: HOTP, secret "12345678901234567890", counters 0..9.
    const QByteArray seed20("12345678901234567890");
    const char *hotpCodes[] = { "755224", "287082", "359152", "969429", "338314",
                                "254676", "287922", "162583", "399871", "520489" };
    for (int c = 0; c < 10; ++c)
        check(L("hotp counter %1").arg(c), Otp::hotp(seed20, c, 6, Hash::Sha1), L(hotpCodes[c]));

    // RFC 6238 appendix B: TOTP, 8 digits, period 30, one seed per hash.
    const QByteArray seed32("12345678901234567890123456789012");
    const QByteArray seed64("1234567890123456789012345678901234567890123456789012345678901234");
    struct Case { quint64 t; const char *sha1, *sha256, *sha512; };
    const Case totpCases[] = {
        { Q_UINT64_C(59),          "94287082", "46119246", "90693936" },
        { Q_UINT64_C(1111111109),  "07081804", "68084774", "25091201" },
        { Q_UINT64_C(1111111111),  "14050471", "67062674", "99943326" },
        { Q_UINT64_C(1234567890),  "89005924", "91819424", "93441116" },
        { Q_UINT64_C(2000000000),  "69279037", "90698825", "38618901" },
        { Q_UINT64_C(20000000000), "65353130", "77737706", "47863826" },
    };
    for (unsigned i = 0; i < sizeof totpCases / sizeof totpCases[0]; ++i) {
        const Case &c = totpCases[i];
        const QString t = QString::number(c.t);
        check(L("totp sha1   t=%1").arg(t), Otp::totp(seed20, c.t, 30, 8, Hash::Sha1), L(c.sha1));
        check(L("totp sha256 t=%1").arg(t), Otp::totp(seed32, c.t, 30, 8, Hash::Sha256), L(c.sha256));
        check(L("totp sha512 t=%1").arg(t), Otp::totp(seed64, c.t, 30, 8, Hash::Sha512), L(c.sha512));
    }

    // otpauth:// links.
    OtpAccount a;
    QString error;
    const QString link = L("otpauth://totp/ACME%20Co:john.doe@email.com?secret=HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"
                           "&issuer=ACME%20Co&algorithm=SHA256&digits=7&period=60");
    check(L("parse link"), yes(Otp::parseUri(link, &a, &error)), L("ok"));
    check(L("  issuer"), a.issuer, L("ACME Co"));
    check(L("  name"), a.name, L("john.doe@email.com"));
    check(L("  secret"), Otp::base32Encode(a.secret), L("HXDMVJECJJWSRB3HWIZR4IFUGFTMXBOZ"));
    check(L("  algorithm/digits/period"), L("%1/%2/%3").arg(Hash::name(a.algorithm)).arg(a.digits).arg(a.period),
          L("SHA256/7/60"));
    OtpAccount b;
    Otp::parseUri(Otp::toUri(a), &b, &error);
    check(L("  round trip"), Otp::toUri(b), Otp::toUri(a));
    check(L("parse hotp link"),
          yes(Otp::parseUri(L("otpauth://hotp/alice?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&counter=5"), &a, &error)), L("ok"));
    check(L("  hotp code at counter 5"), Otp::code(a, 0), L("254676"));
    check(L("reject link without secret"), yes(Otp::parseUri(L("otpauth://totp/x?issuer=y"), &a, &error)), L("error"));
    check(L("reject http link"), yes(Otp::parseUri(L("https://example.com"), &a, &error)), L("error"));

    // Labels with a colon in the account name survive storage (the issuer split happens
    // on the raw label, so an encoded colon stays in the name).
    OtpAccount colon;
    colon.name = L("user:with:colons");
    colon.secret = seed20;
    OtpAccount colonBack;
    Otp::parseUri(Otp::toUri(colon), &colonBack, &error);
    check(L("colon in name, no issuer"), colonBack.issuer + L("|") + colonBack.name, L("|user:with:colons"));
    colon.issuer = L("Iss:uer");
    Otp::parseUri(Otp::toUri(colon), &colonBack, &error);
    check(L("colon in issuer and name"), colonBack.issuer + L("|") + colonBack.name, L("Iss:uer|user:with:colons"));

    // The encrypted store. AES-256: FIPS-197 C.3; CTR: SP 800-38A F.5.5 (first block and the
    // counter carry into the next); PBKDF2-HMAC-SHA256: RFC 7914 section 11.
    check(L("aes-256 FIPS-197 C.3"),
          hex(Aes::encryptBlock(QByteArray::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"),
                                QByteArray::fromHex("00112233445566778899aabbccddeeff"))),
          L("8ea2b7ca516745bfeafc49904b496089"));
    const QByteArray ctrKey = QByteArray::fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    check(L("aes-256-ctr block 1 (F.5.5)"),
          hex(Aes::ctr(ctrKey, QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"),
                       QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172a"))),
          L("601ec313775789a5b7a7f504bbf3d228"));
    check(L("aes-256-ctr block 2 (F.5.5)"),
          hex(Aes::ctr(ctrKey, QByteArray::fromHex("f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"),
                       QByteArray::fromHex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51")).mid(16)),
          L("f443e3ca4d62b59aca84e990cacaf5c5"));
    check(L("pbkdf2-sha256 passwd/salt c=1"), hex(Hash::pbkdf2Sha256("passwd", "salt", 1, 64)),
          L("55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc49ca9cccf179b645991664b39d77ef317c71b845b1e30bd509112041d3a19783"));
    check(L("pbkdf2-sha256 Password/NaCl c=80000"), hex(Hash::pbkdf2Sha256("Password", "NaCl", 80000, 64)),
          L("4ddcd8f60b98be21830cee5ef22701f9641a4418d04c0414aeff08876b34ab56a1d425a1225833549adb841b51c9b3176a272bdebba1d078478f62b397f33c8d"));

    const QByteArray salt = SecureRandom::bytes(Vault::SaltSize);
    check(L("random salt"), L(salt.size() == Vault::SaltSize && salt != QByteArray(Vault::SaltSize, '\0') ? "ok" : "bad"), L("ok"));
    VaultKeys keys = Vault::derive(L("1234"), salt, 2000);
    const QByteArray plain("otpauth://totp/A:b?secret=GEZDGNBVGY3TQOJQ\notpauth://hotp/C:d?secret=MZXW6YTB&counter=3");
    const QByteArray sealed = Vault::seal(keys, plain);
    QByteArray opened;
    check(L("vault round trip"), yes(Vault::open(keys, sealed, &opened) && opened == plain), L("ok"));
    check(L("vault hides the text"), L(sealed.contains("GEZDGNBV") ? "visible" : "hidden"), L("hidden"));
    check(L("vault fresh IV each seal"), L(Vault::seal(keys, plain) != sealed ? "differs" : "same"), L("differs"));
    VaultKeys wrong = Vault::derive(L("1235"), salt, 2000);
    check(L("vault wrong PIN refused"), yes(Vault::open(wrong, sealed, &opened)), L("error"));
    check(L("vault check value differs"), L(Vault::checkValue(wrong) != Vault::checkValue(keys) ? "differs" : "same"), L("differs"));
    QByteArray tampered = sealed;
    tampered[20] = char(tampered.at(20) ^ 1);
    check(L("vault tampering refused"), yes(Vault::open(keys, tampered, &opened)), L("error"));
    // Backup files: the header round-trips, and a file opens with its own salt and count.
    Vault::Backup backup;
    backup.passphrase = true;
    backup.iterations = 2000;
    backup.salt = salt;
    backup.sealed = sealed;
    Vault::Backup back;
    check(L("backup file unpacks"), yes(Vault::unpackBackup(Vault::packBackup(backup), &back)), L("ok"));
    check(L("  header fields"), L("%1/%2/%3").arg(back.passphrase).arg(back.iterations).arg(back.salt == salt),
          L("1/2000/1"));
    VaultKeys fromFile = Vault::derive(L("1234"), back.salt, back.iterations);
    check(L("  opens with its PIN"), yes(Vault::open(fromFile, back.sealed, &opened) && opened == plain), L("ok"));
    check(L("  other data refused"), yes(Vault::unpackBackup("SAUTHBK2 not a backup file at all, sorry", &back)), L("error"));
    check(L("  cut file refused"), yes(Vault::unpackBackup(Vault::packBackup(backup).left(40), &back)), L("error"));
    std::printf("     calibrated iterations for 500 ms on this PC: %d\n", Vault::calibrate(500));

    std::printf("\n%s: %d failure(s)\n", failures ? "FAILED" : "PASSED", failures);
    return failures;
}
