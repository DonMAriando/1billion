@echo off
setlocal
cd /d "%~dp0"
if not exist build\windows\Release\one_billion.exe (
  call COMPILAR_WINDOWS.bat
  if errorlevel 1 exit /b 1
)
build\windows\Release\one_billion.exe %*
