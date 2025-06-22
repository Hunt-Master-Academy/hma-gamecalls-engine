@echo off
echo ============================================
echo  Huntmaster Engine Development Environment
echo ============================================
echo.

REM Activate conda environment
call C:\ProgramData\miniconda3\Scripts\activate.bat huntmaster_engine_env

REM Show environment info
echo [Environment Info]
conda info --envs | findstr "*"
echo.

echo [Available Tools]
where g++ 2>nul && echo G++ compiler: OK || echo G++ compiler: NOT FOUND
where cmake 2>nul && echo CMake: OK || echo CMake: NOT FOUND
where git 2>nul && echo Git: OK || echo Git: NOT FOUND
echo.

echo [Project Location]
echo %CD%
echo.

echo Ready for development!
echo Commands:
echo   - Build: cd build ^&^& cmake --build .
echo   - Test: cd build ^&^& ctest -C Debug --verbose
echo   - Clean: cd build ^&^& rd /s /q . ^&^& cmake -G "MinGW Makefiles" ..
echo.

REM Keep window open
cmd /k