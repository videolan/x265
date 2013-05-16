echo Starting  %testFile%
::run the encoder
cd "%encoderApplicationPath%"
echo Begin
if exist x265-cli.exe (
	x265-cli.exe -c "%fileCfgPath%\%testFile%.cfg" -c "%fileCfgPath%\encoder_I_15P.cfg" -f %frames% -b "%encoderApplicationPath%\str_%testFile%.bin" > "%encoderApplicationPath%"\encodeLog_%testFile%.txt
)


:: run the HM decoder
if exist "%encoderApplicationPath%\str_%testFile%.bin" (

	cd "%DecoderApplicationPath%"
	if exist TAppDecoder.exe (
		TAppDecoder.exe -b "%encoderApplicationPath%\str_%testFile%.bin" -o "%DecoderApplicationPath%\decode_%testFile%.yuv" > "%DecoderApplicationPath%"\decodeLog_%testFile%.txt
	)
)




:: run dr_psnr
cd  "%psnrApplicationPath%"
if exist "%fileCfgPath%\%testFile%.yuv" (
if exist "%DecoderApplicationPath%\decode_%testFile%.yuv" (
if exist dr_psnr.exe (
dr_psnr.exe -r "%fileCfgPath%\%testFile%.yuv" -c "%DecoderApplicationPath%\decode_%testFile%.yuv" -w 1280 -h 720 -e 10 > "%psnrApplicationPath%"\dr_psnr_output_%testFile%.txt
)
)
)



echo Test Sequence File : %testFile%.yuv >> "%encoderApplicationPath%"\results.txt
if exist "%psnrApplicationPath%"\dr_psnr_output_%testFile%.txt (
findstr /l "Avg " "%psnrApplicationPath%"\dr_psnr_output_%testFile%.txt > temp.txt
for /f "tokens=2,3,4" %%a in (temp.txt) do (
echo PSNR Y  : %%a  >> "%encoderApplicationPath%"\results.txt
echo PSNR U  : %%b  >> "%encoderApplicationPath%"\results.txt
echo PSNR V  : %%c  >> "%encoderApplicationPath%"\results.txt
)
)


		
	
cd "%encoderApplicationPath%"
if exist "%encoderApplicationPath%"\encodeLog_%testFile%.txt (

	findstr /b "Bytes written to file: " "%encoderApplicationPath%"\encodeLog_%testFile%.txt >> "%encoderApplicationPath%"\results.txt
	
    findstr /l "sec"  "%encoderApplicationPath%"\encodeLog_%testFile%.txt >> "%encoderApplicationPath%"\results.txt
	
)

