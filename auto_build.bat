@echo off
echo --------start--------
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=./arm-toolchain.cmake
@REM cmake --build build //build all
set arg1=%1

if "%arg1%"=="" (
    cmake --build build
) else if "%arg1%"=="all" (
    cmake --build build
) else if "%arg1%"=="bootloader" (
    cmake --build build --target bootloader.elf
) else if "%arg1%"=="application" (
    cmake --build build --target application.elf
) else if "%arg1%"=="flash_boot" (
    cmake --build build --target bootloader.elf
    cmake --build build --target flash_boot
) else if "%arg1%"=="flash_app" (
    cmake --build build --target application.elf
    cmake --build build --target flash_app
) else if "%arg1%"=="flash_all" (
    cmake --build build 
    cmake --build build --target flash_all
) else if "%arg1%"=="clean" (
    call clean_all.bat
) else (
    echo Unknown target: %arg1%
)

echo --------DONE--------
