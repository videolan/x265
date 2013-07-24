if not exist vc11-x86_64\8bpp\Release\x265.exe (
  echo Perf test expects VC11 x64 8bpp build of x265, not found. Skipping perf tests
  exit /b 1
)
if not exist commandlines.txt (
  echo commandlines.txt does not exist.
  echo Copy commandlines-example to commandlines.txt and edit as necessary
  exit /b 1
)

set CSV=perflog.csv
set LOG=perflog.txt
if exist %LOG% del %LOG%

FOR /F "delims=EOF" %%i IN (commandlines.txt) do (
  echo Running videos with options: %%i
  call:encoder %video1% "%%i"
  call:encoder %video2% "%%i"
  call:encoder %video3% "%%i"
)
EXIT /B

:encoder
@echo x265.exe %1 %~2 -f %perfframes% -o hevc.out --hash 1 >> %LOG%
vc11-x86_64\8bpp\Release\x265.exe %1 %~2 -f %perfframes% -o hevc.out --csv %CSV% --hash 1 >> %LOG% 2>&1
%decoder% -b hevc.out >> %LOG%
