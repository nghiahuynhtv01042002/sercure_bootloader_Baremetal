@echo off
echo delete build .....
rmdir /s /q build 2>nul

echo delete custom-tool\firmware-flash\firmware-flash-build ...
rmdir /s /q custom-tool\firmware-flash\firmware-flash-build 2>nul

echo clean file in custom-tool\gen-rsakeys
pushd custom-tool\gen-rsakeys
del *.pem *.txt *.hex *.sig
popd

echo Done