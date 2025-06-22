
@echo off
REM Quick build script for Huntmaster Engine

REM Activate environment
call C:\ProgramData\miniconda3\Scripts\activate.bat huntmaster_engine_env

REM Check if Kiss FFT is set up
if not exist "libs\kissfft\CMakeLists.txt" (
    echo Setting up Kiss FFT...
    call setup_kissfft.bat
)

REM Create build directory if it doesn't exist
if not exist "build" mkdir build

REM Configure if needed
if not exist "build\CMakeCache.txt" (
    echo Configuring CMake...
    cd build
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
    cd ..
)

REM Build
echo Building project...
cd build
cmake --build .
cd ..

echo.
echo Build complete!
pause