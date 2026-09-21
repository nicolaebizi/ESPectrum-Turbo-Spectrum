@echo off
setlocal
cd /d "%~dp0"
py -m pip install -r requirements.txt
pyinstaller --noconfirm --clean --onefile --windowed --name ESPectrum-Turbo-Spectrum-Flasher --add-data "..\\..\\firmware\\Turbo Spectrum\\ESPectrum-Turbo-Spectrum-FULL.bin;firmware" esp32_turbo_flasher.py
echo.
echo Built: dist\\ESPectrum-Turbo-Spectrum-Flasher.exe
pause
