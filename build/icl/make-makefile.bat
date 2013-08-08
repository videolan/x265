@echo off
::
:: run this batch file to create an Intel C++ 2013 NMake makefile for this project.
:: See the cmake documentation for other generator targets
::
if "%ICPP_COMPILER13%" == "" (
  msg "%username%" "Intel C++ 2013 not detected"
  exit 1
)
call "%ICPP_COMPILER13%\bin\compilervars.bat" intel64
set CC=icl
set CXX=icl
cmake -G "NMake Makefiles" ..\..\source && cmake-gui ..\..\source
