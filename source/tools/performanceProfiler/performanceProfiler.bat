@echo off
@setlocal


@for /f "tokens=1* delims==" %%a in (config.txt) do (
if %%a==DecoderApplicationPath set DecoderApplicationPath=%%b
if %%a==ScriptPath set ScriptPath=%%b
if %%a==encoderApplicationPath set encoderApplicationPath=%%b
if %%a==fileCfgPath set fileCfgPath=%%b
if %%a==psnrApplicationPath set psnrApplicationPath=%%b
if %%a==frames set frames=%%b
if %%a==testFile1 set testFile1=%%b
if %%a==testFile2 set testFile2=%%b
if %%a==testFile3 set testFile3=%%b
)

echo Test Results > "%encoderApplicationPath%"\results.txt

for /L %%G in (1,1,2) do (

if  %%G==1  set testFile=%testFile1%
if  %%G==2  set testFile=%testFile2%
if  %%G==3  set testFile=%testFile3%	

 cd %ScriptPath%
 CALL Profiler.bat
)

	