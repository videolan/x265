@echo off
if "%VS90COMNTOOLS%" == "" (
  msg "%username%" "Visual Studio 9 not detected"
  exit 1
)

@mkdir 10bpp
@mkdir 8bpp

@cd 10bpp
if not exist x265.sln (
  cmake  -G "Visual Studio 9 2008 Win64" ../../../source -DHIGH_BIT_DEPTH=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DWINXP_SUPPORT=ON
)
if exist x265.sln (
  call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat"
  MSBuild /property:Configuration="Release" x265.sln
  copy/y Release\x265-static.lib ..\8bpp\x265-static-10bpp.lib
)
@cd ..

@cd 8bpp
if not exist x265-static-10bpp.lib (
  msg "%username%" "10bpp build failured"
  exit 1
)
if not exist x265.sln (
  cmake  -G "Visual Studio 9 2008 Win64" ../../../source -DHIGH_BIT_DEPTH=OFF -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=ON -DWINXP_SUPPORT=ON -DEXTRA_LIB=x265-static-10bpp.lib -DEXTRA_LINK_FLAGS="/FORCE:MULTIPLE"
)
if exist x265.sln (
  call "%VS90COMNTOOLS%\..\..\VC\vcvarsall.bat"
  MSBuild /property:Configuration="Release" x265.sln
  copy/y Release\x265.exe ..
)
@cd ..
