# Decodes the QR samples made by tools/make-qr-samples.py with the phone's decoder
# (src/scan/qrreader + 3rdparty/zxing) and checks each against expected.txt. Desktop only:
#   tools\run-qr-test.cmd
TEMPLATE = app
TARGET = qr-test
CONFIG += console
CONFIG -= app_bundle
include(../../core.pri)
include(../../3rdparty/zxing/zxing.pri)
INCLUDEPATH += ../../src/scan
HEADERS += ../../src/scan/qrreader.h
SOURCES += ../../src/scan/qrreader.cpp main.cpp
