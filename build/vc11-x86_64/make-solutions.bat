@echo off
::
:: run this batch file to create a Visual Studion solution file for this project.
:: See the cmake documentation for other generator targets
::
cmake -G "Visual Studio 11 Win64" ..\..\source && cmake-gui ..\..\source
