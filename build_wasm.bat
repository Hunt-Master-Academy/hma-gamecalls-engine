@echo off
echo Building Huntmaster Engine WASM...

REM Set up Emscripten environment
if exist "tools\emsdk\emsdk_env.bat" (
    call tools\emsdk\emsdk_env.bat
) else (
    echo Error: Emscripten not found. Run install_emscripten.bat first.
    exit /b 1
)

REM Create build directory
if not exist "build_wasm" mkdir build_wasm
cd build_wasm

REM Configure with CMake
echo Configuring CMake for WASM...
emcmake cmake -DCMAKE_BUILD_TYPE=Release ..

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    exit /b 1
)

REM Build
echo Building WASM module...
emmake make -j4

if %errorlevel% equ 0 (
    echo Build successful!
    echo Output files in: web/dist/
    cd ..
) else (
    echo Build failed!
    cd ..
    exit /b 1
)