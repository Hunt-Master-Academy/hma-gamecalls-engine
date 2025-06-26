@echo off
echo Building minimal WASM test...

REM Check if emcc is available
where emcc >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: emcc not found in PATH
    echo Trying to set up Emscripten environment...
    
    if exist "tools\emsdk\emsdk_env.bat" (
        call tools\emsdk\emsdk_env.bat
    ) else (
        echo Error: Emscripten not installed. Run install_emscripten.bat first.
        pause
        exit /b 1
    )
)

REM Create output directory
if not exist "web\dist" mkdir web\dist

REM Build minimal test
echo Compiling minimal test...
emcc test_wasm_minimal.cpp ^
    -O2 ^
    -s WASM=1 ^
    -s MODULARIZE=1 ^
    -s EXPORT_NAME="HuntmasterEngine" ^
    -s EXPORTED_FUNCTIONS="['_createEngine','_testFunction']" ^
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" ^
    -o web/dist/huntmaster_engine.js

if %errorlevel% equ 0 (
    echo Success! Created web/dist/huntmaster_engine.js
) else (
    echo Build failed!
)

pause