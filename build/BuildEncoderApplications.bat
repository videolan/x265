@echo off

   cd "%builddir%"
   mkdir 8bit_prem
   cd 8bit_prem
   set solution="x265.sln"
   

	echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE ^) >> cacheoutput.txt
	echo SET(HIGH_BIT_DEPTH OFF CACHE BOOL "Use 8bit pixels internally" FORCE ^) >> cacheoutput.txt
	
	cmake -G %makefile%  -C cacheoutput.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt
		
		if %errorlevel% equ 1 (
			echo Build with 8bit_prem unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with 8bit_prem successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with 8bit_prem successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with 8bit_prem unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo 8bit_prem solution is not created for %makefile% >> "%LOG%"
        )


   cd "%builddir%"
   mkdir 8bit_HM
   cd 8bit_HM

   	 
	echo SET(ENABLE_TESTS OFF CACHE BOOL "Enable Unit Tests" FORCE^) >> cacheoutput.txt
	echo SET(HIGH_BIT_DEPTH OFF CACHE BOOL "Use 8bit pixels internally" FORCE ^) >> cacheoutput.txt

	cmake -G %makefile% -C cacheoutput.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt

		if %errorlevel% equ 1 (
			echo Build with 8bit_HM unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with 8bit_HM successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with 8bit_HM successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with 8bit_HM unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo 8bit_HM solution is not created for %makefile% >> "%LOG%"
        )


cd "%builddir%"
   mkdir 16bit_prem
   cd 16bit_prem
   set solution="x265.sln"
   
	
	echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE ^) >> cacheoutput.txt
	echo SET(HIGH_BIT_DEPTH ON CACHE BOOL "Use 16bit pixels internally" FORCE ^) >> cacheoutput.txt
	
	cmake -G %makefile%  -C cacheoutput.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt
		
		if %errorlevel% equ 1 (
			echo Build with 16bit_prem unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with 16bit_prem successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with 16bit_prem successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with 16bit_prem unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo 16bit_prem solution is not created for %makefile% >> "%LOG%"
        )


   cd "%builddir%"
   mkdir 16bit_HM
   cd 16bit_HM

   	
	echo SET(ENABLE_TESTS OFF CACHE BOOL "Enable Unit Tests" FORCE^) >> cacheoutput.txt
	echo SET(HIGH_BIT_DEPTH ON CACHE BOOL "Use 16bit pixels internally" FORCE ^) >> cacheoutput.txt
	
	cmake -G %makefile% -C cacheoutput.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt

		if %errorlevel% equ 1 (
			echo Build with 16bit_HM unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with 16bit_HM successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with 16bit_HM successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with 16bit_HM unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo 16bit_HM solution is not created for %makefile% >> "%LOG%"
        )

