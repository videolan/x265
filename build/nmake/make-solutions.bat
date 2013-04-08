@echo off
::
:: run this batch file to create a Visual Studion solution file for this project.
:: See the cmake documentation for other generator targets
::
call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat"
cmake -G "NMake Makefiles" ..\..\source && cmake-gui ..\..\source
