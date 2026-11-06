@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo Instala CMake 3.24+ y Visual Studio 2022 con Desarrollo de escritorio C++.
  exit /b 1
)
cmake -S . -B build\windows -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build\windows --config Release --parallel
if errorlevel 1 exit /b 1
ctest --test-dir build\windows -C Release --output-on-failure
if errorlevel 1 exit /b 1
echo Listo. Ejecuta JUGAR_WINDOWS.bat
exit /b 0
