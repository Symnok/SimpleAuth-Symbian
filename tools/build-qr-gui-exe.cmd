@echo off
rem Builds tools\otp-qr-gui.exe: the QR test bench as one file that runs without Python.
rem Needs Python only here, on the build PC. PyInstaller, qrcode and Pillow go into a private
rem virtual environment (tools\.venv-exe), so the global Python is left alone.
setlocal
cd /d %~dp0
if not exist .venv-exe\Scripts\python.exe (
    python -m venv .venv-exe
    if errorlevel 1 exit /b 1
    .venv-exe\Scripts\python -m pip install -q pyinstaller qrcode pillow
    if errorlevel 1 exit /b 1
)
.venv-exe\Scripts\python make-gui-icon.py
if errorlevel 1 exit /b 1
.venv-exe\Scripts\python -m PyInstaller --noconfirm --clean --onefile --windowed ^
    --name otp-qr-gui --icon "%~dp0otp-qr-gui.ico" --add-data "%~dp0otp-qr-gui.ico;." ^
    --workpath build-exe --specpath build-exe --distpath . ^
    "%~dp0otp-qr-gui.py"
if errorlevel 1 exit /b 1
echo built %~dp0otp-qr-gui.exe
