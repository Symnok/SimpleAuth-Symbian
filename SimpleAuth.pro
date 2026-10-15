# SimpleAuth - a two-factor authenticator (TOTP/HOTP) for Symbian Anna/Belle.
# Copyright (C) 2026 - GPL-3.0-or-later, see LICENSE.
#
# Build with build-symbian.cmd (the "Qt 4.7.4 for Symbian Anna/Belle" kit) for the phone,
# or tools\build-desktop.cmd to run it on the PC. The OTP core (core.pri) is shared with
# the self test in tools/otp-test.

TEMPLATE = app
TARGET = SimpleAuth
VERSION = 1.0.2

QT += core gui declarative

include(core.pri)
include(3rdparty/zxing/zxing.pri)

INCLUDEPATH += src/app src/scan

HEADERS += \
    src/app/appcontroller.h \
    src/app/accountsmodel.h \
    src/app/qrcodeitem.h \
    src/scan/qrreader.h \
    src/scan/qrscanner.h

SOURCES += \
    src/main.cpp \
    src/app/appcontroller.cpp \
    src/app/accountsmodel.cpp \
    src/app/qrcodeitem.cpp \
    src/scan/qrreader.cpp \
    src/scan/qrscanner.cpp

RESOURCES += qml.qrc

# The version reaches the About page as an unquoted macro (stringified in code), which
# survives every generator's quoting rules.
DEFINES += APP_VERSION=$$VERSION

OTHER_FILES += \
    qml/*.qml \
    README.md

symbian {
    # Unprotected range: installs self-signed without Symbian Signed.
    TARGET.UID3 = 0xE5A71B2C
    # The camera (QR scanning) is the only capability; it is self-signable. The app never
    # uses the network and keeps its data in its private folder.
    TARGET.CAPABILITY = UserEnvironment

    # The camera through Qt Mobility (built into Belle; the Smart Installer package brings
    # it to Anna). The desktop build has no camera and plays test images instead.
    CONFIG += mobility
    MOBILITY += multimedia
    DEFINES += SA_CAMERA
    HEADERS += src/scan/framesurface.h
    TARGET.EPOCHEAPSIZE = 0x020000 0x2000000
    ICON = icon.svg

    # Qt Quick Components for Symbian (built into Belle; Anna gets them through the Smart
    # Installer package, SimpleAuth_installer.sis).
    CONFIG += qt-components
    DEPLOYMENT.installer_header = 0x2002CCCF

    vendorinfo = \
        "; Localised and unique vendor names" \
        "%{\"Symnok\"}" \
        ":\"Symnok\""
    # Symbian wants the version comma-separated (0,1,0); derive it from VERSION so the
    # installer's version follows the one build-symbian.cmd bumps.
    VERSION_COMMA = $$replace(VERSION, "\.", ",")
    packageheader = "$${LITERAL_HASH}{\"SimpleAuth\"},(0xE5A71B2C),$${VERSION_COMMA},TYPE=SA,RU"
    deployment.pkg_prerules += packageheader vendorinfo
    DEPLOYMENT += deployment
}

simulator {
    DEFINES += Q_WS_SIMULATOR
}

CODECFORTR = UTF-8
CODECFORSRC = UTF-8
