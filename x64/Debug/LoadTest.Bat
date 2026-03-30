@echo off
setlocal enabledelayedexpansion

SET /A index=1
SET /A count=100

:while
if !index! LEQ !count! (
    START "" /MIN .\Project_6_2.exe 127.0.0.1 8080 !index!
    echo !index!
    SET /A index=!index!+1
    goto :while
)

endlocal