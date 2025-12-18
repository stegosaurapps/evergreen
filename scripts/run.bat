@echo off
setlocal

set BUILD_DIR=vs2019-x64
set CONFIG=Debug

set EXE=%~dp0..\build\%BUILD_DIR%\%CONFIG%\evergreen.exe

if not exist "%EXE%" (
  echo ERROR: executable not found:
  echo %EXE%
  exit /b 1
)

"%EXE%"

endlocal
