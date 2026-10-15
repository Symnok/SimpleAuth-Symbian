@echo off
rem Builds and runs the OTP self test (tools\otp-test) with the desktop Qt 4.7.4 kit.
setlocal
set PATH=D:\QtSDK\Desktop\Qt\4.7.4\mingw\bin;D:\QtSDK\mingw\bin;%PATH%
cd /d %~dp0otp-test
qmake otp-test.pro CONFIG+=release >nul
if errorlevel 1 exit /b 1
mingw32-make -s release
if errorlevel 1 exit /b 1
release\otp-test.exe
