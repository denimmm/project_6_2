@echo off
SET /A "index = 1"
SET /A "count = 200"

:while
if %index% leq %count% (
     START /MIN Project_6_2.exe 10.126.150.41 8080 %index%
     SET /A index = %index% + 1
     @echo %index%
     goto :while
)

