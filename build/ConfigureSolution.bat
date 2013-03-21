@echo off

if "%1" == "" goto usage

if "%1" == "ALL" Goto ALL

if "%1" == "SS" Goto SS


:ALL
:VisualStudion11

echo BUILDING SOLUTION For Visual Studio 11
mkdir VisualStudio11
cd VisualStudio11
cmake -G "Visual Studio 11" ../../source

if %errorlevel% equ 1 (
echo *********** SOLUTION IS NOT BUILDED *******
)
cd ../
if "%1" == "SS" goto eof


echo BUILDING THE SOLUTION For Visual Studio 11 Win64
:VisualStudio11win64
mkdir VisualStudio11win64
cd VisualStudio11win64
cmake -G "Visual Studio 11 Win64" ../../source

if %errorlevel% equ 1 (
echo ******* SOLUTION IS NOT BUILDED ***********
)
cd ../
if "%1" == "SS" goto eof


echo BUILDING THE SOLUTION For Visual Studio 10
:VisualStudio10
mkdir VisualStudio10
cd VisualStudio10
cmake -G "Visual Studio 10" ../../source

if %errorlevel% equ 1 (
echo *********** SOLUTION IS NOT BUILDED *******
)
cd ../
if "%1" == "SS" goto eof


echo BUILDING THE SOLUTION For Visual Studio 10 Win64
:VisualStudio10win64
mkdir VisualStudio10win64
cd VisualStudio10win64
cmake -G "Visual Studio 10 Win64" ../../source

if %errorlevel% equ 1 (
echo ******* SOLUTION IS NOT BUILDED ***********
)
cd ../
if "%1" == "SS" goto eof


echo BUILDING THE SOLUTION For Visual Studio 9 2008
:VisualStudio92008
mkdir VisualStudio92008
cd VisualStudio92008
cmake -G "Visual Studio 9 2008" ../../source

if %errorlevel% equ 1 (
echo ******* SOLUTION IS NOT BUILDED ***********
)
cd ../
if "%1" == "SS" goto eof


echo BUILDING SOLUTION For Visual Studio 9 2008 Win64
:VisualStudio92008win64
mkdir VisualStudio92008win64
cd VisualStudio92008win64
cmake -G "Visual Studio 9 2008 Win64" ../../source

if %errorlevel% equ 1 (
echo ******* SOLUTION IS NOT BUILDED ***********
)
cd ../
goto eof


:SS
if "%2" =="1" goto :VisualStudion11
if "%2" =="2" goto :VisualStudio11win64
if "%2" =="3" goto :VisualStudio10
if "%2" =="4" goto :VisualStudio10win64
if "%2" =="5" goto :VisualStudio92008
if "%2" =="6" goto :VisualStudio92008win64

:usage

  echo NOTE: If you have All the Compiler installed in your machine use ALL Option

  echo ConfigureSolution.bat ALL   -- Build the Solution for All the below Make Files
  
  echo Example: ConfigureSolution.bat ALL
  
  echo ConfigureSolution.bat SS "Make Files"   --  Build any Specific Make Files
  
  echo Example: ConfigureSolution.bat SS 1 
  
  echo The following generators are available on this platform:
  
  echo 	1        = Generates Visual Studio 10 project files.
  echo 	2      	 = Generates Visual Studio 10 Win64 project files
  echo 	3        = Generates Visual Studio 11 project files.
  echo 	4      	 = Generates Visual Studio 11 Win64 project files
  echo 	5        = Generates Visual Studio 9 2008 project files.
  echo 	6  	 = Generates Visual Studio 9 2008 Win64 project

goto eof


