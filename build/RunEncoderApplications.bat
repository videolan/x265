@echo off

   cd "%builddir%\16bit"
	if exist Release\x265-cli.exe (
		cd Release
		x265-cli.exe -i "%video%" -c "%currentworkingdir%"\cfg\encoder_I_15P.cfg -f 30 >> ExecLog.txt
		FC  str.bin D:\\Reference_files\Basketball_16bit_3F.bin >> DiffBin.txt
		FC  rec.yuv D:\\Reference_files\Basketball_16bit_3F.rec >> DiffYuv.txt
	)

   cd "%builddir%\16bit"
	if exist test\Release\TestBench.exe (
		cd test\Release
		TestBench.exe >> TestLog.txt
	) 

   cd "%builddir%\8bit\Release"
 	if exist x265-cli.exe (
		
		x265-cli.exe  -i "%video%" -c "%currentworkingdir%"\cfg\encoder_I_15P.cfg -f 30  >> ExecLog.txt
		FC  str.bin D:\\Reference_files\Basketball_8bit_3F.bin >> DiffBin.txt
		FC  rec.yuv D:\\Reference_files\Basketball_8bit_3F.rec >> DiffYuv.txt
	) 
   cd "%builddir%\8bit"
	if exist test\Release\TestBench.exe (
		cd test\Release
		TestBench.exe >> TestLog.txt
	) 
