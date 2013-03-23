@echo off

set buildmode=Release
if "%4" == "--debug" (
	SET buildmode=Debug )


if "%1" == "" goto usage
if "%1" == "ALL" Goto ALL
if "%1" == "-c" Goto Config

	
:ALL

:VisualStudion11
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
echo BUILDING SOLUTION For Visual Studio 11
mkdir VisualStudio11
cd VisualStudio11
cmake -G "Visual Studio 11" ../../source

if %errorlevel% equ 0 (
if "%3" == "--build" (
MSBuild /property:Configuration="%buildmode%" x265.sln
)
)
if %errorlevel% equ 1 (
echo *********** SOLUTION NOT BUILT *******
)
cd ../
if "%1" == "-c" goto eof


echo BUILDING SOLUTION For Visual Studio 11 Win64
:VisualStudio11win64
call "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat"
mkdir VisualStudio11win64
cd VisualStudio11win64
cmake -G "Visual Studio 11 Win64" ../../source

if %errorlevel% equ 0 (
if "%3" == "--build" (
MSBuild /property:Configuration="%buildmode%" x265.sln
)
)

if %errorlevel% equ 1 (
echo ******* SOLUTION NOT BUILT ***********
)
cd ../
if "%1" == "-c" goto eof


echo BUILDING SOLUTION For Visual Studio 10
:VisualStudio10
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
mkdir VisualStudio10
cd VisualStudio10
cmake -G "Visual Studio 10" ../../source

if %errorlevel% equ 0 (
if "%3" == "--build" (
MSBuild /property:Configuration="%buildmode%" x265.sln
)
)

if %errorlevel% equ 1 (
echo *********** SOLUTION NOT BUILT *******
)
cd ../
if "%1" == "-c" goto eof


echo BUILDING SOLUTION For Visual Studio 10 Win64
:VisualStudio10win64
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
mkdir VisualStudio10win64
cd VisualStudio10win64
cmake -G "Visual Studio 10 Win64" ../../source

if %errorlevel% equ 0 (
if "%3" == "--build" (
MSBuild /property:Configuration="%buildmode%" x265.sln
)
)

if %errorlevel% equ 1 (
echo ******* SOLUTION NOT BUILT ***********
)
cd ../
if "%1" == "-c" goto eof


echo BUILDING SOLUTION For Visual Studio 9 2008
:VisualStudio92008
call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
mkdir VisualStudio92008
cd VisualStudio92008
cmake -G "Visual Studio 9 2008" ../../source

if %errorlevel% equ 0 (
if "%3" == "--build" (
MSBuild /property:Configuration="%buildmode%" x265.sln
)
)

if %errorlevel% equ 1 (
echo ******* SOLUTION NOT BUILT ***********
)
cd ../
if "%1" == "-c" goto eof


echo BUILDING SOLUTION For Visual Studio 9 2008 Win64
:VisualStudio92008win64
call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
mkdir VisualStudio92008win64
cd VisualStudio92008win64
cmake -G "Visual Studio 9 2008 Win64" ../../source

if %errorlevel% equ 0 (
if "%3" == "--build" (
MSBuild /property:Configuration="%buildmode%" x265.sln
)
)

if %errorlevel% equ 1 (
echo ******* SOLUTION NOT BUILT ***********
)
cd ../
goto eof


:Config

if "%2" =="1" goto :VisualStudio11
if "%2" =="2" goto :VisualStudio11win64
if "%2" =="3" goto :VisualStudio10
if "%2" =="4" goto :VisualStudio10win64
if "%2" =="5" goto :VisualStudio92008
if "%2" =="6" goto :VisualStudio92008win64

:usage

  echo NOTE: Compilers Supported: VC9, VC10, VC11
  echo If you have all compilers installed, you may use ALL Option
  echo ConfigureSolution.bat ALL   -- Build the Solution for all supported compilers
  echo Example: ConfigureSolution.bat -c 1 --debug
  echo The following generators are available on this platform:
  
  echo 	1        = Generates Visual Studio 11 project files and solution.
  echo 	2      	 = Generates Visual Studio 11 Win64 project files and solution.
  echo 	3        = Generates Visual Studio 10 project files and solution.
  echo 	4      	 = Generates Visual Studio 10 Win64 project files and solution.
  echo 	5        = Generates Visual Studio 9 2008 project files and solution.
  echo 	6  	 = Generates Visual Studio 9 2008 Win64 project files and solution. 

goto eof
