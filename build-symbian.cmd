@echo off
rem Builds the phone package from the command line (Qt Creator does the same through its
rem Symbian kit). Qt for Symbian supports in-source builds only, so this runs in the
rem project directory; the generated files are listed in .gitignore.
rem
rem   build-symbian.cmd            release ARMv5 build, then SimpleAuth_<ver>.sis (self-signed;
rem                                installs on Belle and, as tried, on Anna too), after bumping
rem                                the patch version, so SimpleAuth.pro always names the newest
rem                                package
rem   build-symbian.cmd installer  the same, plus SimpleAuth_installer_<ver>.sis, the Smart
rem                                Installer wrapper that fetches Qt Quick Components / Qt
rem                                Mobility on a phone that lacks them
rem   build-symbian.cmd nobump     builds the version SimpleAuth.pro names as it is, for a
rem                                release number set by hand (e.g. 1.0.0)
rem   build-symbian.cmd clean      removes the build output
setlocal
call D:\QtSDK\Symbian\SDKs\SymbianSR1Qt474\env.bat
cd /d %~dp0

if "%1"=="clean" (
    call sbs -c arm.v5.urel.gcce4_4_1 clean
    goto :eof
)

rem Bump first: the version in SimpleAuth.pro is then the one being built, and each build
rem is higher than the last (Symbian refuses to replace an app with an equal version).
if not "%1"=="nobump" (
    python bump-version.py
    if errorlevel 1 exit /b 1
)
for /f "delims=" %%v in ('python bump-version.py --print') do set VER=%%v
echo === building SimpleAuth %VER%

qmake SimpleAuth.pro -spec symbian-sbsv2 CONFIG+=release
if errorlevel 1 exit /b 1
call sbs -c arm.v5.urel.gcce4_4_1
if errorlevel 1 exit /b 1

call createpackage.bat SimpleAuth_template.pkg release-armv5
if errorlevel 1 exit /b 1
if "%1"=="installer" (
    call createpackage.bat SimpleAuth_installer.pkg release-armv5
    if errorlevel 1 exit /b 1
)

rem The only packages left behind carry the version in their names; the unversioned and
rem unsigned intermediates go.
if exist SimpleAuth.sis ( move /y SimpleAuth.sis SimpleAuth_%VER%.sis >nul & echo built SimpleAuth_%VER%.sis )
if exist SimpleAuth_installer.sis ( move /y SimpleAuth_installer.sis SimpleAuth_installer_%VER%.sis >nul & echo built SimpleAuth_installer_%VER%.sis )
del /q SimpleAuth_unsigned.sis SimpleAuth_installer_unsigned.sis 2>nul
