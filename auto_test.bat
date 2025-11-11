rem autotest
@echo off
echo ###########################################
echo build Bootloader
echo ###########################################

call autobuild-script.bat
echo ###########################################
echo build flash_fw_cli
echo ###########################################

pushd custom-tool\firmware-flash
call build.bat
popd

rem execute test
echo ###########################################
echo execute test
echo ###########################################

set app_path="build\application\application.bin"
set flash_fw_cli=".\custom-tool\firmware-flash\firmware-flash-build\firmware_sender.exe"

echo %flash_fw_cli% COM3 %app_path%
%flash_fw_cli% COM3 %app_path%