@echo off
::
::  Drag a CPP or H file onto this batch file to apply the
::  project's coding style to the file.  This will likely overwrite the
::  original file, so use with care
::
::  This batch file expects to be in the same folder as AStyle.exe and
::  astyle-config.txt
::

%~dp0\AStyle.exe --options=%~dp0\astyle-config.txt %*
