@echo off

 cd "%builddir%"
   mkdir 8bit
   cd 8bit
   set solution="x265.sln"

	echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE ^) >> enablecache.txt
	echo SET(HIGH_BIT_DEPTH OFF CACHE BOOL "Use 8bit pixels internally" FORCE ^) >> enablecache.txt

	cmake -G %makefile%  -C enablecache.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt
		
		if %errorlevel% equ 1 (
			echo Build with 8bit unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with 8bit successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with 8bit successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with 8bit unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo 8bit solution is not created for %makefile% >> "%LOG%"
        )



 cd "%builddir%"
   mkdir 16bit
   cd 16bit
   set solution="x265.sln"

	echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE ^) >> enablecache.txt
	echo SET(HIGH_BIT_DEPTH ON CACHE BOOL "Use 16bit pixels internally" FORCE ^) >> enablecache.txt
	
	cmake -G %makefile%  -C enablecache.txt ..\..\..\source

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt
		
		if %errorlevel% equ 1 (
			echo Build with 16bit unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with 16bit successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with 16bit successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with 16bit unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo 16bit solution is not created for %makefile% >> "%LOG%"
        )

  