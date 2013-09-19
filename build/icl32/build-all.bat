@echo off
if not "%ICPP_COMPILER13%" == "" ( set ICL="%ICPP_COMPILER13" )
if not "%ICPP_COMPILER14%" == "" ( set ICL="%ICPP_COMPILER14" )
if "%ICL%" == "" (
  msg "%username%" "Intel C++ 2013 not detected"
  exit 1
)
if not exist Makefile (
  call make-makefile.bat
)
if exist Makefile (
  call "%ICL%\bin\compilervars.bat" ia32
  nmake
)
