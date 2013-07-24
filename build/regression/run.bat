@echo off

if not exist config.txt (
  echo config.txt does not exist.
  echo Copy config-example to config.txt and edit as necessary
  exit /b 1
)
if not exist commandlines.txt (
  echo commandlines.txt does not exist.
  echo Copy commandlines-example to commandlines.txt and edit as necessary
  exit /b 1
)

for /f "tokens=1,2,3,4,5,6,7,8 delims==" %%a in (config.txt) do (
  if %%a==repository set repository=%%b
  if %%a==testframes set testframes=%%b
  if %%a==perfframes set perfframes=%%b
  if %%a==msys set msys=%%b
  if %%a==decoder set decoder=%%b
  if %%a==video1 set video1=%%b
  if %%a==video2 set video2=%%b
  if %%a==video3 set video3=%%b
)

set CWD=%CD%
set LOG="%CWD%"\regression.log
if exist %LOG% del %LOG%

hg pull -u "%repository%"
if %errorlevel% equ 1 (
  echo "Pull failed" >> "%LOG%"
  exit /b 1
)
hg summary >> "%LOG%"
hg summary

call:buildconfigs "%msys%" msys "MSYS Makefiles"
call:buildconfigs "%VS110COMNTOOLS%" vc11-x86_64 "Visual Studio 11 Win64"
call:buildconfigs "%VS110COMNTOOLS%" vc11-x86 "Visual Studio 11"
call:buildconfigs "%VS100COMNTOOLS%" vc10-x86_64 "Visual Studio 10 Win64"
call:buildconfigs "%VS100COMNTOOLS%" vc10-x86 "Visual Studio 10"
call:buildconfigs "%VS90COMNTOOLS%" vc9-x86_64 "Visual Studio 9 2008 Win64"
call:buildconfigs "%VS90COMNTOOLS%" vc9-x86 "Visual Studio 9 2008 Win64"

call 02perftest.bat
EXIT /B

:buildconfigs
set compiler=%~1
set buildconfig=%2
set generator=%~3

if exist %compiler% (
  echo Detected %generator%, building >> "%LOG%"
  cmd /c 01build-and-smoke-test.bat
)
