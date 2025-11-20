@echo off
setlocal enabledelayedexpansion
set "fw_path=..\..\build\application"
for %%F in (%fw_path%) do set "fw_name=%%~nF"

echo Generating RSA private key...
openssl genrsa -out private_key.pem 2048

echo Extracting public key...
openssl rsa -in private_key.pem -pubout -out public_key.pem

echo Signing firmware.bin...
openssl dgst -sha256 -sign private_key.pem -out %fw_name%.sig %fw_path%

echo Verifying signature...
openssl dgst -sha256 -verify public_key.pem -signature %fw_name%.sig %fw_path%

@REM echo Extracting key info...
@REM openssl rsa -in public_key.pem -pubin -text -noout > key_info.txt

echo Extracting modulus...
for /f "tokens=1* delims==" %%a in ('openssl rsa -in public_key.pem -pubin -modulus -noout') do (
    set MOD=%%b
)

REM Remove ":" characters manually
set CLEAN_MOD=
for %%c in (!MOD!) do (
    set TMP=%%c
)

REM replace colon
set CLEAN_MOD=%MOD::=%

echo %CLEAN_MOD%> modulus.hex

echo Extracting exponent...
for /f "tokens=2 delims= " %%a in ('openssl rsa -in public_key.pem -pubin -text -noout ^| findstr Exponent') do (
    set EXP=%%a
)

echo %EXP%> exponent.txt

echo Done!
echo - private_key.pem
echo - public_key.pem
echo - firmware.sig
echo - modulus.hex
echo - exponent.txt

endlocal
