@echo off
echo Installing Emscripten SDK...

REM Check if git is installed
git --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Git is not installed. Please install Git first.
    pause
    exit /b 1
)

REM Create tools directory if it doesn't exist
if not exist "tools" mkdir tools
cd tools

REM Clone emsdk if it doesn't exist
if not exist "emsdk" (
    echo Cloning Emscripten SDK...
    git clone https://github.com/emscripten-core/emsdk.git
) else (
    echo Updating existing Emscripten SDK...
    cd emsdk
    git pull
    cd ..
)

cd emsdk

REM Install and activate latest Emscripten
echo Installing latest Emscripten version...
call emsdk install latest

echo Activating Emscripten...
call emsdk activate latest

echo Setting up environment...
call emsdk_env.bat

echo.
echo Emscripten installation complete!
echo.
echo To use Emscripten in new terminals, run:
echo   cd %CD%
echo   emsdk_env.bat
echo.

cd ..\..
pause