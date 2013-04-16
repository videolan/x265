@echo off

for /f "tokens=1,2,3 delims==" %%a in (config.txt) do (
if %%a==workingdir set workingdir=%%b
if %%a==testdir set testdir=%%b
if %%a==repository set repository=%%b
)

set HG=hg
set currentworkingdir="%workingdir%"RegressionTest
set Buildbat="BuildEncoderApplications.bat"

if exist "%currentworkingdir%" rd /s /q %currentworkingdir%

mkdir "%currentworkingdir%"
cd "%currentworkingdir%"

set LOG="%currentworkingdir%"\RegressionTester.log

if %errorlevel% equ 1 (echo Working directory not created >> "%LOG%"
	exit 1
)
echo Working directory created >> "%LOG%"

"%HG%" init
"%HG%" pull "%repository%"
"%HG%" update

if %errorlevel% equ 1 (echo Pull unsuccessfull >> "%LOG%"
exit 1
)
echo Pull successfull >> "%LOG%"

set builddir=""
set makefile=""
set vcvars=""

::Build the solution and applications for VS 11

if  not "%VS110COMNTOOLS%" == "" ( 
	
	echo Regression Test	-	VS 11 Compiler found >> "%LOG%"
	set builddir="%currentworkingdir%"\build\vc11-x86_64
	set makefile="Visual Studio 11 Win64"
	set vcvars="%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat"
	call "%workingdir%BuildEncoderApplications.bat"

	set builddir="%currentworkingdir%"\build\vc11-x86   
	set makefile="Visual Studio 11"
	call "%workingdir%BuildEncoderApplications.bat"     
)

::Build the solution and applications for VS 10

if  not "%VS100COMNTOOLS%" == "" ( 
	echo Regression Test	-	VS 10 Compiler found >> "%LOG%"
	set builddir="%currentworkingdir%"\build\vc10-x86_64
	set makefile="Visual Studio 10 Win64"
	set vcvars="%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat"
	call "%workingdir%BuildEncoderApplications.bat"

	set builddir="%currentworkingdir%"\build\vc10-x86        
	set makefile="Visual Studio 10"
	call "%workingdir%BuildEncoderApplications.bat"
)

::Build the solution and applications foe VS 09

if  not "%%VS90COMNTOOLS%" == "" ( 
	echo Regression Test	-	VS 09 Compiler found >> "%LOG%"
	set builddir="%currentworkingdir%"\build\vc9-x86_64
	set makefile="Visual Studio 9 2008"
	set vcvars="%%VS90COMNTOOLS%%\..\..\VC\vcvarsall.bat"
	call "%workingdir%BuildEncoderApplications.bat"

	set builddir="%currentworkingdir%"\build\vc9-x86        
	set makefile="Visual Studio 10"
	call "%workingdir%BuildEncoderApplications.bat"
)


:: To do list 
:: Running the test bench and Encoder Application for all the build version
