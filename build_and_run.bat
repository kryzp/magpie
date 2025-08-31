@echo off

call build.bat

if errorlevel 1 (
   exit /b 1
)

.\build\magpie_win32.exe
