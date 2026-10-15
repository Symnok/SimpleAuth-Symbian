# The OTP core: plain C++ and QtCore, shared by the app and the self tests (tools/).
INCLUDEPATH += $$PWD/src/core

HEADERS += \
    $$PWD/src/core/aes.h \
    $$PWD/src/core/hash.h \
    $$PWD/src/core/otp.h \
    $$PWD/src/core/qrcode.h \
    $$PWD/src/core/securerandom.h \
    $$PWD/src/core/vault.h

SOURCES += \
    $$PWD/src/core/aes.cpp \
    $$PWD/src/core/hash.cpp \
    $$PWD/src/core/otp.cpp \
    $$PWD/src/core/qrcode.cpp \
    $$PWD/src/core/securerandom.cpp \
    $$PWD/src/core/vault.cpp

# SecureRandom::bytes: CryptGenRandom on Windows (Symbian's Math::Random is in euser).
win32:LIBS += -ladvapi32
