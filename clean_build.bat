@echo off
echo Cleaning build directory...
if exist build\*.obj del /Q build\*.obj
if exist build\program.exe del /Q build\program.exe

echo.
echo Running full build...
call build.bat
