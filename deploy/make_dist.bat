@echo off
rem Двойной клик: собрать dist\MonitorIBP через MSYS2 MINGW64
chcp 65001 >nul
cd /d "%~dp0.."
set MSYSTEM=MINGW64
set CHERE_INVOKING=1
"C:\msys64\usr\bin\bash.exe" -lc "./deploy/make_dist.sh"
pause
