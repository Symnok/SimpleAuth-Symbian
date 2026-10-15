# Self test of the OTP core against the RFC 4226 / RFC 6238 test vectors. Desktop only:
#   tools\run-otp-test.cmd
TEMPLATE = app
TARGET = otp-test
CONFIG += console
CONFIG -= app_bundle
QT -= gui
include(../../core.pri)
SOURCES += main.cpp
