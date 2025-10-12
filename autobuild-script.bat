@echo off
echo --------start--------
rmdir /s /q build 2>nul
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=./arm-toolchain.cmake
cmake --build build

cmake --build BUILD --target flash_all  

echo --------DONE--------


