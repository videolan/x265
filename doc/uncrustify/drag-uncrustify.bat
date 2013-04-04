@echo on
::
::  Drag a CPP or H file onto this batch file to apply the
::  project's coding style to the file.  This will likely overwrite the
::  original file, so use with care

%~dp0\uncrustify.exe -f "%1" -c %~dp0\codingstyle.cfg -o indentoutput.tmp
move /Y indentoutput.tmp "%1"

pause
