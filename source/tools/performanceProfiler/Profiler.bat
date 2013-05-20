echo Starting  %testFile%

for /f "tokens=1,2 delims=." %%a in ("%testFile%") do (

set filename=%%a
set extension=%%b
)

::run the encoder 
echo  >> ..\..\..\build\%buildVersion%\Release\results.txt
echo Test Sequence File : %testFile% >> ..\..\..\build\%buildVersion%\Release\results.txt

if exist ..\..\..\build\"%buildVersion%"\Release\x265-cli.exe (
	if "%extension%"=="y4m" (
	..\..\..\build\%buildVersion%\Release\x265-cli.exe -i "%inputFilePath%\%testFile%" -c "..\..\..\cfg\encoder_I_15P.cfg" -f %frames% -b "..\..\..\build\%buildVersion%\Release\str_%filename%.bin" > ..\..\..\build\%buildVersion%\Release\encodeLog_%filename%.txt
	)
)
if not exist ..\..\..\build\"%buildVersion%"\Release\x265-cli.exe (
	echo x265-cli.exe not Found 
	echo x265-cli.exe Not Found >> ..\..\..\build\%buildVersion%\Release\results.txt  
	goto error1
	)


:: run the HM decoder
if exist "..\..\..\build\%buildVersion%\Release\str_%filename%.bin" (
	if exist "..\HM Decoder\TAppDecoder.exe"  (
		"..\HM Decoder\TAppDecoder.exe" -b "..\..\..\build\%buildVersion%\Release\str_%filename%.bin" -o "..\..\..\build\%buildVersion%\Release\decode_%filename%.yuv" > ..\..\..\build\%buildVersion%\Release\decodeLog_%filename%.txt )
	if not exist "..\HM Decoder\TAppDecoder.exe" (
		echo Decoder.exe not Found 
		echo Decoder.exe Not Found >> ..\..\..\build\%buildVersion%\Release\results.txt  
		goto error1 )
		
)
if not exist ..\..\..\build\%buildVersion%\Release\str_%filename%.bin (
	echo str_%filename%.bin Not Found 
	echo str_%filename%.bin Not Found  >> ..\..\..\build\%buildVersion%\Release\results.txt
	goto error1
	)
:: converting input y4m files to raw yuv files
if "%extension%"=="y4m" (
	if not exist "%ffmpegPath%\ffmpeg.exe" (
		echo ffmpeg.exe not Found 
		echo ffmpeg.exe Not Found >> ..\..\..\build\%buildVersion%\Release\results.txt  
		goto error1
	)
	if not exist "%inputFilePath%\raw_%filename%.yuv" (
	%ffmpegPath%\ffmpeg.exe -i "%inputFilePath%\%testFile%" -f rawvideo "%inputFilePath%\raw_%filename%.yuv"
	)
)


:: run dr_psnr

if exist ..\..\..\build\"%buildVersion%"\tools\dr_psnr\Release\dr_psnr.exe (
	if exist ..\..\..\build\"%buildVersion%"\Release\decode_%filename%.yuv (
		
		if "%extension%"=="y4m" (
			..\..\..\build\"%buildVersion%"\tools\dr_psnr\Release\dr_psnr.exe -r "%inputFilePath%\raw_%filename%.yuv" -c "..\..\..\build\%buildVersion%\Release\decode_%filename%.yuv" -w 1280 -h 720 -e %frames% > "..\..\..\build\%buildVersion%\Release\dr_psnr_output_%filename%.txt"
		)
	)
)
if not exist ..\..\..\build\"%buildVersion%"\tools\dr_psnr\Release\dr_psnr.exe (
		echo PSNR.exe not Found 
		echo PSNR.exe Not Found >> ..\..\..\build\%buildVersion%\Release\results.txt  
		goto error1
	)


if exist "..\..\..\build\%buildVersion%\Release\dr_psnr_output_%filename%.txt" (
	findstr /l "Avg " "..\..\..\build\%buildVersion%\Release\dr_psnr_output_%filename%.txt" > temp.txt
	for /f "tokens=2,3,4" %%a in (temp.txt) do (
		echo PSNR Y  : %%a  >> "..\..\..\build\%buildVersion%\Release\results.txt"
		echo PSNR U  : %%b  >> "..\..\..\build\%buildVersion%\Release\results.txt"
		echo PSNR V  : %%c  >> "..\..\..\build\%buildVersion%\Release\results.txt"
		)
	)


		
	

if exist ..\..\..\build\%buildVersion%\Release\encodeLog_%filename%.txt (

	findstr /b "Bytes written to file: " ..\..\..\build\%buildVersion%\Release\encodeLog_%filename%.txt >> ..\..\..\build\%buildVersion%\Release\results.txt
	
    findstr /l "sec"  ..\..\..\build\%buildVersion%\Release\encodeLog_%filename%.txt >> ..\..\..\build\%buildVersion%\Release\results.txt
	
goto normalExit
	
)

:error1
echo Aborting operation.. >> ..\..\..\build\%buildVersion%\Release\results.txt

:normalExit
echo Finished processing %testFile%