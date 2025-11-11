@echo off
echo --------start--------
rmdir /s /q build 2>nul
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=./arm-toolchain.cmake
cmake --build build

@REM cmake --build build --target flash_all  
cmake --build build --target flash_boot  
@REM cmake --build build --target flash_app


echo --------DONE--------


