@echo off
gcc main.c -o counter.exe -mwindows -O2 -s
if %ERRORLEVEL% EQU 0 (
    echo Build succeeded: counter.exe
) else (
    echo Build failed.
    exit /b 1
)
