rem autotest
@echo off
echo ###########################################
echo clean all
echo ###########################################
call auto_build.bat clean

echo ###########################################
echo build firmware/application
echo ###########################################
call auto_build.bat application

echo ###########################################
echo generate signature
echo ###########################################

pushd custom-tool\gen-rsakeys
call gen_rsa_key.bat ..\..\build\application\application.bin
popd

echo ###########################################
echo build Bootloader
echo ###########################################
call auto_build.bat flash_boot

echo ###########################################
echo build flash_fw_cli
echo ###########################################
pushd custom-tool\firmware-flash-cpp
call build.bat
popd

rem execute test
echo ###########################################
echo execute test
echo ###########################################

set app_path="build\application\application.bin"
set sig_path=".\custom-tool\gen-rsakeys\signature.sig"
set flash_fw_cli=".\custom-tool\firmware-flash-cpp\firmware-flash-build\firmware_sender.exe"
echo "press any key to start running cli tool..."
pause

echo %flash_fw_cli% -p COM3 -i %app_path% -s %sig_path%
%flash_fw_cli% -p COM3 -i %app_path% -s %sig_path%