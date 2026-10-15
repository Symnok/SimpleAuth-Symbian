@echo off
rem Builds the app for the PC (desktop Qt 4.7.4 MinGW, which ships the Symbian Qt Quick
rem Components) into build-desktop\, to try the pages without a phone:
rem
rem   tools\build-desktop.cmd              build
rem   tools\build-desktop.cmd test         build, then walk every page with tools\autotest.qml
rem                                        against a throwaway settings file; screenshots go to
rem                                        build-desktop\shots
setlocal
set PATH=D:\QtSDK\Desktop\Qt\4.7.4\mingw\bin;D:\QtSDK\mingw\bin;%PATH%
set ROOT=%~dp0..
if not exist "%ROOT%\build-desktop" mkdir "%ROOT%\build-desktop"
cd /d "%ROOT%\build-desktop"
rem The desktop Makefile does not track QML inside the .qrc: regenerate it every time.
del /q release\qrc_qml.cpp 2>nul
qmake CONFIG+=console CONFIG+=release -o Makefile ..\SimpleAuth.pro
if errorlevel 1 exit /b 1
mingw32-make -s release
if errorlevel 1 exit /b 1
if not "%1"=="test" goto :eof

if exist shots rmdir /s /q shots
mkdir shots
rem A settings file as a version without the lock left it: one account in plain text, which
rem the setup step must encrypt and then remove.
> autotest.ini echo [accounts]
>> autotest.ini echo 1\link=otpauth://totp/Legacy:old?secret=MZXW6YTBOI^&issuer=Legacy
>> autotest.ini echo size=1
set SA_SETTINGS=%CD%\autotest.ini
if exist backups rmdir /s /q backups
set SA_BACKUP_DIR=%CD%\backups
set SA_SHOT_DIR=%CD%\shots
for %%f in ("%ROOT%\tools\autotest.qml") do set SA_AUTOTEST=%%~ff
rem The fake camera's frames: six without a code (time for the Focus dialog), then the
rem first test link.
if not exist "%ROOT%\tools\qr-test\samples\0-frame.png" python "%ROOT%\tools\make-qr-samples.py"
for %%f in ("%ROOT%\tools\qr-test\samples") do set S=%%~ff
set N=%S%\none-scene.png
set SA_SCAN_IMAGES=%N%;%N%;%N%;%N%;%N%;%N%;%S%\0-frame.png
release\SimpleAuth.exe
rem The code "Show QR code" drew must read back as the account's link (needs Python + OpenCV).
python -c "import cv2,sys; t=cv2.QRCodeDetector().detectAndDecode(cv2.imread(r'shots\09-show-qr.png'))[0]; print('autotest: show-qr decodes to: '+t); sys.exit(0 if t.startswith('otpauth://totp/Google:') else 1)"
rem No key may be left readable in the settings file (all the test accounts' secrets).
findstr /i /c:"MZXW6YTB" /c:"GEZDGNBV" /c:"JBSWY3DP" /c:"APE4PNPM" /c:"otpauth" autotest.ini >nul && (echo autotest: FAIL plain secrets in autotest.ini) || (echo autotest: ok   no plain secrets in autotest.ini)
rem Nor in the backup file (binary: Python looks for the Base32 secrets and "otpauth").
python -c "import glob,sys; d=b''.join(open(f,'rb').read() for f in glob.glob(r'backups\*.sauth')); bad=[w for w in (b'GEZDGNBV',b'APE4PNPM',b'otpauth') if w in d]; print('autotest: '+('FAIL plain secrets in backup' if bad or not d else 'ok   no plain secrets in backup')); sys.exit(1 if bad else 0)"
