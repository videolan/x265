@echo off
if "%VS100COMNTOOLS%" == "" (
  msg "%username%" "Visual Studio 10 not detected"
  exit 1
)
if not exist xhevc.sln (
  call make-solutions.bat
)
if exist xhevc.sln (
  call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat"
  MSBuild /property:Configuration="Release" xhevc.sln
  MSBuild /property:Configuration="Debug" xhevc.sln
  MSBuild /property:Configuration="RelWithDebInfo" xhevc.sln
)
