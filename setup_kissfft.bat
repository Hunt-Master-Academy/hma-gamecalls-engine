@echo off
echo Setting up Kiss FFT for Huntmaster Engine...

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: Please run this script from the huntmaster-engine root directory
    exit /b 1
)

REM Create libs directory if it doesn't exist
if not exist "libs" mkdir libs
cd libs

REM Clone Kiss FFT if not already present
if not exist "kissfft" (
    echo Cloning Kiss FFT...
    git clone https://github.com/mborgerding/kissfft.git
    cd kissfft
    git checkout v1.3.0
    cd ..
) else (
    echo Kiss FFT already exists in libs\kissfft
)

cd ..

echo.
echo Kiss FFT setup complete!
echo.
echo Now you can build the project:
echo   cd build
echo   rd /s /q .
echo   cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
echo   cmake --build .