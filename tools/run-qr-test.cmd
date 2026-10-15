@echo off
rem Makes the QR samples (tools\make-qr-samples.py, needs Python with qrcode + Pillow), then
rem builds and runs tools\qr-test with the desktop Qt 4.7.4 kit.
setlocal
set PATH=D:\QtSDK\Desktop\Qt\4.7.4\mingw\bin;D:\QtSDK\mingw\bin;%PATH%
cd /d %~dp0
python make-qr-samples.py
if errorlevel 1 exit /b 1
cd qr-test
qmake qr-test.pro CONFIG+=release >nul
if errorlevel 1 exit /b 1
mingw32-make -s release
if errorlevel 1 exit /b 1
release\qr-test.exe samples
