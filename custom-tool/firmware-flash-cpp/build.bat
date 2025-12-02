rmdir /s /q firmware-flash-build 2>nul

cmake -S . -B firmware-flash-build -G "MinGW Makefiles"
cmake --build firmware-flash-build
