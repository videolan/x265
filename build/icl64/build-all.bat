@echo off
if "%ICPP_COMPILER13%" == "" (
  msg "%username%" "Intel C++ 2013 not detected"
  exit 1
)
if not exist Makefile (
  call make-makefile.bat
)
if exist Makefile (
  call "%ICPP_COMPILER13%\bin\compilervars.bat" intel64
  nmake
)
