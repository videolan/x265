@echo off

if exist %buildconfig% rd /s /q %buildconfig%
mkdir %buildconfig%
cd %buildconfig%

call:makesolution 8bpp "-D ENABLE_PPA:BOOL=ON"
call:makesolution 16bpp "-D HIGH_BIT_DEPTH:BOOL=ON"
exit /B

:makesolution
set depth=%~1
if exist %depth% rd /s /q %depth%
mkdir %depth%
cd %depth%
set name=%generator%-%depth%

echo Running cmake for %name%
if "%buildconfig%" == "msys" (

  echo cd "%CD%" > buildscript.sh
  echo cmake -D ENABLE_TESTS:BOOL=ON %~2 -G "%generator%" ../../../../source >> buildscript.sh
  echo make >> buildscript.sh

  %msys% -l "%CD%\buildscript.sh"
  if exist x265.exe (
    rem We cannot test MSYS x265 without running in MSYS environment
    cd ..
    exit /b 0
  ) else (
    echo %name% could not create an x265.exe >> "%LOG%"
    cd ..
    exit /b 1
  )

) else (

  call "%compiler%\..\..\VC\vcvarsall.bat"
  cmake -D ENABLE_TESTS:BOOL=ON %~2 -G "%generator%" ..\..\..\..\source >> ..\cmake%depth%.txt
  if not exist x265.sln (
    echo %name% solution was not created >> "%LOG%"
    cd ..
    exit /b 1
  )

  echo Compiling for release...
  MSBuild /property:Configuration="Release" x265.sln >> ..\build%depth%_release.txt
  if %errorlevel% equ 1 (
    echo Release %name% build failed, refer the build log >> "%LOG%"
    cd ..
    exit /b 1
  )
  
  echo Compiling for debug...
  MSBuild /property:Configuration="Debug" x265.sln >> ..\build%depth%_debug.txt
  if %errorlevel% equ 1 (
    echo Debug %name% build failed, refer the build log >> "%LOG%"
    cd ..
    exit /b 1
  )
)

echo Smoke tests...
if exist Release\x265.exe (
  Release\x265.exe %video1% -f %testframes% --wpp --hash 1 -o str1.out -r rec1.yuv --no-progress >> ..\encoder_%depth%.txt 2>&1
  Release\x265.exe %video2% -f %testframes% --wpp --hash 1 -o str2.out -r rec2.yuv --no-progress >> ..\encoder_%depth%.txt 2>&1
  Release\x265.exe %video3% -f %testframes% --wpp --hash 1 -o str3.out -r rec3.yuv --no-progress >> ..\encoder_%depth%.txt 2>&1

  %decoder% -b str1.out -o str1.yuv >> ..\decoder_%depth%.txt
  %decoder% -b str2.out -o str2.yuv >> ..\decoder_%depth%.txt
  %decoder% -b str3.out -o str3.yuv >> ..\decoder_%depth%.txt

  FC /b rec1.yuv str1.yuv > NUL || echo Reconstructed frame mismatch for %name% %video1% >> ..\..\DiffBin.txt
  FC /b rec2.yuv str2.yuv > NUL || echo Reconstructed frame mismatch for %name% %video2% >> ..\..\DiffBin.txt
  FC /b rec3.yuv str3.yuv > NUL || echo Reconstructed frame mismatch for %name% %video3% >> ..\..\DiffBin.txt
)

echo Leak test...
if exist Debug\x265.exe (
:: hopefully you have VLD installed so this will check for leaks
  Debug\x265.exe %video1% -f %testframes% --wpp -o str4.out -r rec4.yuv --no-progress >> ..\encoder_%depth%.txt 2>&1
  %decoder% -b str4.out -o str4.yuv >> ..\decoder_%depth%.txt
  FC /b rec4.yuv str4.yuv > NUL || echo Reconstructed frames mismatch for debug %name% >> ..\..\DiffBin.txt
)

echo Testbench...
if exist test\Release\TestBench.exe (
  test\Release\TestBench.exe >> ..\testbench.txt 2>&1 || echo %name% testbench failed >> "%LOG%"
)
if exist test\TestBench.exe (
  test\TestBench.exe >> ..\testbench.txt 2>&1 || echo %name% testbench failed >> "%LOG%"
)
cd ..
