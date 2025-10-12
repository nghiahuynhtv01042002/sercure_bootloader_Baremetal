@echo off
echo --- Flashing Bootloader and Application ---

set BOOT=build\bootloader\bootloader.bin
set APP=build\application\application.bin
set FlashTool=tools\stm32-programmer\bin\STM32_Programmer_CLI.exe
set BOOT_ADDRESS=0x08000000
set APP_ADDRESS=0x08008000
"%FlashTool%" -c port=SWD ^
    -d "%BOOT%"  %BOOT_ADDRESS% ^
    -d "%APP%"  %APP_ADDRESS%   ^
    -rst

echo --- Done ---

@REM cmake --build BUILD --target flash_all  
