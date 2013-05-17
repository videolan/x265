echo Starting  %testFile%

for /f "tokens=1,2 delims=." %%a in ("%testFile%") do (

set filename=%%a
set extension=%%b
)
echo %filename%
echo %extension%



::run the encoder
cd "%encoderApplicationPath%"
echo Begin
if exist x265-cli.exe (
if "%extension%"=="y4m" (
	x265-cli.exe -i "%fileCfgPath%\%testFile%" -c "%fileCfgPath%\encoder_I_15P.cfg" -f %frames% -b "%encoderApplicationPath%\str_%filename%.bin" > "%encoderApplicationPath%"\encodeLog_%filename%.txt
)

if "%extension%"=="yuv" (
	x265-cli.exe -c "%fileCfgPath%\%filename%.cfg" -c "%fileCfgPath%\encoder_I_15P.cfg" -f %frames% -b "%encoderApplicationPath%\str_%filename%.bin" > "%encoderApplicationPath%"\encodeLog_%filename%.txt
)
)


:: run the HM decoder
if exist "%encoderApplicationPath%\str_%filename%.bin" (

	cd "%DecoderApplicationPath%"
	if exist TAppDecoder.exe (
		TAppDecoder.exe -b "%encoderApplicationPath%\str_%filename%.bin" -o "%DecoderApplicationPath%\decode_%filename%.yuv" > "%DecoderApplicationPath%"\decodeLog_%filename%.txt
	)
)

:: convert y4m to yuv for running dr_psnr
if "%extension%"=="y4m" (
	cd "%fileCfgPath%"
	if exist "raw_%filename%.yuv" (
		del "raw_%filename%.yuv"
	)

	cd "%ffmpegPath%"
	ffmpeg.exe -i "%fileCfgPath%\%testFile%" -f rawvideo "%fileCfgPath%\raw_%filename%.yuv"
)


:: run dr_psnr
cd  "%psnrApplicationPath%"
if exist dr_psnr.exe (
	if exist "%DecoderApplicationPath%\decode_%filename%.yuv" (
		if "%extension%"=="yuv" (
			dr_psnr.exe -r "%fileCfgPath%\%filename%.yuv" -c "%DecoderApplicationPath%\decode_%filename%.yuv" -w 1280 -h 720 -e 10 > "%psnrApplicationPath%"\dr_psnr_output_%filename%.txt
		)
		if "%extension%"=="y4m" (
			dr_psnr.exe -r "%fileCfgPath%\raw_%filename%.yuv" -c "%DecoderApplicationPath%\decode_%filename%.yuv" -w 1280 -h 720 -e 10 > "%psnrApplicationPath%"\dr_psnr_output_%filename%.txt
		)
	)
)


echo : >>"%encoderApplicationPath%"\results.txt
echo Test Sequence File : %testFile% >> "%encoderApplicationPath%"\results.txt
if exist "%psnrApplicationPath%"\dr_psnr_output_%filename%.txt (
	findstr /l "Avg " "%psnrApplicationPath%"\dr_psnr_output_%filename%.txt > temp.txt
	for /f "tokens=2,3,4" %%a in (temp.txt) do (
		echo PSNR Y  : %%a  >> "%encoderApplicationPath%"\results.txt
		echo PSNR U  : %%b  >> "%encoderApplicationPath%"\results.txt
		echo PSNR V  : %%c  >> "%encoderApplicationPath%"\results.txt
		)
	)


		
	
cd "%encoderApplicationPath%"
if exist "%encoderApplicationPath%"\encodeLog_%filename%.txt (

	findstr /b "Bytes written to file: " "%encoderApplicationPath%"\encodeLog_%filename%.txt >> "%encoderApplicationPath%"\results.txt
	
    findstr /l "sec"  "%encoderApplicationPath%"\encodeLog_%filename%.txt >> "%encoderApplicationPath%"\results.txt
	
)

