@echo off
::
:: run this batch file to create an Intel C++ 2013 NMake makefile for this project.
:: See the cmake documentation for other generator targets
::
if not "%ICPP_COMPILER13%" == "" ( set ICL="%ICPP_COMPILER13" )
if not "%ICPP_COMPILER14%" == "" ( set ICL="%ICPP_COMPILER14" )
if "%ICL%" == "" (
  msg "%username%" "Intel C++ 2013 not detected"
  exit 1
)
call "%ICL%\bin\compilervars.bat" ia32
set CC=icl
set CXX=icl
cmake -G "NMake Makefiles" ..\..\source && cmake-gui ..\..\source
