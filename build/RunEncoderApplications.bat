@echo off

   cd "%builddir%\enabled"
	if exist Release\x265-cli.exe (
		cd Release
		x265-cli.exe -c "%currentworkingdir%"\cfg\encoder_all_I.cfg -c "%currentworkingdir%"\cfg\per-sequence\BasketballDrive.cfg -f 3 -i "%video%" >> ExecLog.txt
	)

   cd "%builddir%\enabled"
	if exist test\Release\TestBench.exe (
		cd test\Release
		TestBench.exe >> TestLog.txt
	) 

   cd "%builddir%\disable\Release"
 	if exist x265-cli.exe (
		
		x265-cli.exe -c "%currentworkingdir%"\cfg\encoder_all_I.cfg -c "%currentworkingdir%"\cfg\per-sequence\BasketballDrive.cfg -f 3 -i "%video%" >> ExecLog.txt
	) 
   cd "%builddir%\disable"
	if exist test\Release\TestBench.exe (
		cd test\Release
		TestBench.exe >> TestLog.txt
	) 
