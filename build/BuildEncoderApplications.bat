@echo off

   cd "%builddir%"
   mkdir enabled
   cd enabled
   set solution="x265.sln"

	echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE ^) >> enablecache.txt
	echo SET(HIGH_BIT_DEPTH ON CACHE BOOL "Use 16bit pixels internally" FORCE ^) >> enablecache.txt
	echo SET(ENABLE_PRIMITIVES ON CACHE BOOL "Enable use of optimized encoder primitives" FORCE ^) >> enablecache.txt

	cmake -G %makefile%  -C enablecache.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt
		
		if %errorlevel% equ 1 (
			echo Build with primitive enabled unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with primitives enabled successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with primitive enable successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with primitive enable unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo Primitive Enable solution is not created for %makefile% >> "%LOG%"
        )


   cd "%builddir%"
   mkdir disable
   cd disable
 
	echo SET(ENABLE_TESTS ON CACHE BOOL "Enable Unit Tests" FORCE^) >> disablecache.txt
	echo SET(ENABLE_PRIMITIVES OFF CACHE BOOL "Enable use of optimized encoder primitives" FORCE^) >> disablecache.txt
	echo SET(ENABLE_PRIMITIVES_VEC OFF CACHE BOOL "Enable use of optimized encoder primitives" FORCE^) >> disablecache.txt

	cmake -G %makefile% -C disablecache.txt ..\..\..\source 

	if exist %solution% (
  		call %vcvars%
		MSBuild /property:Configuration="Release" x265.sln >> BuildLog.txt

		if %errorlevel% equ 1 (
			echo Build with primitive disable unsuccessfull for %makefile% refer the build log  >> "%LOG%"
			exit 1
		)
		echo Build with primitives disable successfull for %makefile% >> "%LOG%"

		if exist Release\x265-cli.exe (
  			echo build Application with primitive disable successfull for %makefile% >> "%LOG%"			
		) else (
			echo build Application with primitive disable unsuccessfull for %makefile%  refer the Build log >> "%LOG%"
		)     
        ) else ( echo Primitive Disable solution is not created for %makefile% >> "%LOG%"
        )

