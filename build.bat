@echo off
cd /d "%~dp0"
set GXX=g++
where %GXX% >nul 2>nul
if errorlevel 1 set GXX=C:\msys64\ucrt64\bin\g++.exe

"%GXX%" -std=c++17 -municode -mwindows -O2 -static -static-libgcc -static-libstdc++ ^
  -finput-charset=UTF-8 main.cpp -o TimurDubai.exe ^
  -lgdi32 -luser32 -lwinmm -lmsimg32

if errorlevel 1 (
  echo.
  echo BUILD FAILED
) else (
  echo.
  echo BUILD OK: TimurDubai.exe
)
pause
