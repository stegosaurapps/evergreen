@echo off
set TARGET=%~dp0..\build

echo Clearing directory: %TARGET%

del /q "%TARGET%\*"
for /d %%x in ("%TARGET%\*") do rmdir /s /q "%%x"

echo Done.

endlocal
