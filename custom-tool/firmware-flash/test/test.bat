@echo off
setlocal enabledelayedexpansion

echo [1] Compile test_recv.c ...
gcc -o test-recv.exe -O3 test_recv.c ..\src\serial_com.c -I..\inc
if ERRORLEVEL 1 (
    echo !! ERROR: Failed to build test-recv.exe
    pause
    exit /b 1
)

echo [2] Compile test_send.c ...
gcc -o test-send.exe -O3 test_send.c ..\src\serial_com.c -I..\inc
if ERRORLEVEL 1 (
    echo !! ERROR: Failed to build test-send.exe
    pause
    exit /b 1
)

echo [3] Starting receiver on COM12...
start "" /B test-recv.exe COM12 firmware_copy.bin

timeout /t 1 >nul

echo [4] Starting sender on COM11...
start /WAIT "" test-send.exe COM11 G:\VScode\ws\secure-bootloader\build\application\application.bin

echo [5] Transfer finished. Checking firmware integrity...
if not exist firmware_copy.bin (
    echo !! ERROR: firmware_copy.bin NOT FOUND! Receiver probably failed.
    pause
    exit /b 1
)


fc /b firmware_copy.bin "G:\VScode\ws\secure-bootloader\build\application\application.bin" >nul 2>&1

if ERRORLEVEL 1 (
    echo ==============================
    echo  FAIL!!!!
    echo ==============================
    echo Files are different!
) else (
    echo ==============================
    echo  OK ^<333333 
    echo ==============================
    echo Files are identical!
)

pause