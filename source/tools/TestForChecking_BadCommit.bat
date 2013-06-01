@ECHO OFF

cd C:\\Users\Mandar\Documents\RegressionTest_20may-clone

SET index=1
SET variable=1
SET NLM=^
SET NL=^^^%NLM%%NLM%^%NLM%%NLM%

set /p good=Enter Reference(good) Revision number:
set /p bad=Enter Current(bad) Revision number:

SET /a var = %bad% - %good%

:true
SET /a var = %var% / 2

hg bisect -r

hg bisect -b %bad%

if %index%==%variable% hg bisect -g %good% > result.txt
ECHO %NL% >> result.txt

if not %index%==%variable% hg bisect -g >> result.txt
ECHO %NL% >> result.txt

SET /a index=2

if %var%==%variable% goto false
if not %var%==%variable% goto true

:false
hg bisect -r
hg bisect -b %bad%
hg bisect -g >> result.txt
ECHO %NL% >> result.txt

hg bisect -r
hg bisect -b %bad%
hg bisect -g >> result.txt
ECHO %NL% >> result.txt
ECHO Now check the result.txt file for result
pause