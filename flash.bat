@echo off
echo --- Flashing ---

set PRJ=BUILD\bootloader\bootloader.bin
set FlashTool=tools\stm32-programmer\bin\STM32_Programmer_CLI.exe
set PRJ_ADDRESS=0x08000000
"%FlashTool%" -c port=SWD ^
    -d "%PRJ%"  "%PRJ_ADDRESS%" ^
    -rst

echo --- Done ---

@REM cmake --build BUILD --target flash_all  
