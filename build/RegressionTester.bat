@echo off

for /f "tokens=1,2,3,4,5 delims==" %%a in (config.txt) do (
if %%a==workingdir set workingdir=%%b
if %%a==testdir set testdir=%%b
if %%a==repository set repository=%%b
if %%a==password set password=%%b
if %%a==logpath set logpath=%%b
)

set HG=hg
set LOG="%logpath%"

echo %workingdir%
echo %testdir%
echo %repository%
echo %password%

if exist "%workingdir%" rd /s /q %workingdir%

mkdir "%workingdir%"
cd "%workingdir%"

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


if  not "%VS110COMNTOOLS%" == "" ( 
cd build\vc11-x86_64

mkdir Enabled
cd Enabled

if not exist x265.sln (

echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE ^) >> enablecache.txt
echo SET(HIGH_BIT_DEPTH ON CACHE BOOL "Use 16bit pixels internally" FORCE ^) >> enablecache.txt
echo SET(ENABLE_PRIMITIVES_VEC ON CACHE BOOL "Enable use of optimized encoder primitives" FORCE ^) >> enablecache.txt

cmake -G "Visual Studio 11 Win64" -C enablecache.txt ..\..\..\source

if exist x265.sln (
  call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat"
  MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt


if %errorlevel% equ 1 (echo Build for primitives enabled unsuccessfull >> "%LOG%"
exit 1
)
echo Build for primitives enabled successfull >> "%LOG%"

if exist Release\x265-cli.exe (
  echo Encoder client build successfully >> "%LOG%"
) else ( echo Encoder client not build successfully >> "%LOG%" )
)
)

cd ../
mkdir Disabled
cd Disabled
if not exist x265.sln (

echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE^) >> disablecache.txt
echo SET(HIGH_BIT_DEPTH OFF CACHE BOOL "Use 16bit pixels internally" FORCE^) >> disablecache.txt
echo SET(ENABLE_PRIMITIVES_VEC OFF CACHE BOOL "Enable use of optimized encoder primitives" FORCE^) >> disablecache.txt

cmake -G "Visual Studio 11 Win64" -C disablecache.txt ..\..\..\source
)

if exist x265.sln (
  call "%VS110COMNTOOLS%\..\..\VC\vcvarsall.bat"
  MSBuild /property:Configuration="Release" x265.sln >> BuilLog.txt

if %errorlevel% equ 1 (echo Build for primitives disabled unsuccessfull >> "%LOG%"
exit 1
)
echo Build for primitives disabled successfull >> "%LOG%"

if exist Release\x265-cli.exe (
	echo Encoder client build successfully >> "%LOG%"
	del \q BuildLog.txt
 ) else (
           echo Encoder client not build successfully >> "%LOG%" )
)
)
cd
pause

