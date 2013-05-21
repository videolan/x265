@echo off
@setlocal


@for /f "tokens=1* delims==" %%a in (config.txt) do (
if %%a==buildVersion set buildVersion=%%b
if %%a==inputFilePath set inputFilePath=%%b
if %%a==ffmpegPath set ffmpegPath=%%b
if %%a==frames set frames=%%b
if %%a==inputFileName1 set inputFileName1=%%b
if %%a==inputFileName2 set inputFileName2=%%b
if %%a==inputFileName3 set inputFileName3=%%b
)

echo Test Results > "..\..\..\build\%buildVersion%\Release\results.txt"

for /L %%G in (1,1,2) do (
if  %%G==1  set testFile=%inputFileName1%
if  %%G==2  set testFile=%inputFileName2%
if  %%G==3  set testFile=%inputFileName3%	


 CALL Profiler.bat
)

	