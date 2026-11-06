@echo off
rem Двойной клик: полный релиз (установщик Windows + .deb для Linux через WSL)
chcp 65001 >nul
cd /d "%~dp0.."
set MSYSTEM=MINGW64
set CHERE_INVOKING=1
"C:\msys64\usr\bin\bash.exe" -lc "./deploy/make_release.sh"
pause