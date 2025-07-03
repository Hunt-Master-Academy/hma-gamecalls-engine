@echo off
echo Setting up Huntmaster Engine development environment...

REM Activate conda environment
call C:\ProgramData\miniconda3\Scripts\activate.bat
call conda activate huntmaster_final_env

REM Check if environment exists
if %errorlevel% neq 0 (
    echo Creating huntmaster_final_env...
    conda create -n huntmaster_final_env python=3.11 -y
    call conda activate huntmaster_final_env
    
    REM Install build tools
    conda install -c conda-forge cmake ninja gcc gxx gdb -y
    
    REM Install Python packages
    pip install numpy scipy matplotlib
)

REM Set environment variables
set PATH=C:\ProgramData\miniconda3\envs\huntmaster_final_env\Library\mingw-w64\bin;%PATH%
set CC=gcc
set CXX=g++

REM Check for Emscripten
where emcc >nul 2>nul
if %errorlevel% neq 0 (
    echo WARNING: Emscripten not found. WASM builds will not be available.
    echo Install from: https://emscripten.org/docs/getting_started/downloads.html
) else (
    echo Emscripten found.
)

echo.
echo Environment ready! You can now:
echo   1. Configure: cmake -B build -G "MinGW Makefiles"
echo   2. Build: cmake --build build
echo   3. Test: cd build ^&^& ctest
echo.