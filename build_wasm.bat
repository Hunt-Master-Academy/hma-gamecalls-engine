@echo off
echo Building Huntmaster Engine WASM (Simple Build)...

REM Set up Emscripten environment
if exist "tools\emsdk\emsdk_env.bat" (
    echo Setting up Emscripten environment...
    call tools\emsdk\emsdk_env.bat
) else (
    echo Error: Emscripten not found. Run install_emscripten.bat first.
    pause
    exit /b 1
)

REM Create output directory
if not exist "web\dist" mkdir web\dist

REM Compile WASM module with proper exports
echo Compiling WebAssembly module...
emcc ^
    src/WASMInterface.cpp ^
    src/MFCCProcessor.cpp ^
    src/DTWProcessor.cpp ^
    src/ThirdPartyLibs.cpp ^
    libs/kissfft/kiss_fft.c ^
    libs/kissfft/tools/kiss_fftr.c ^
    -I include ^
    -I libs ^
    -I libs/kissfft ^
    -O2 ^
    -s WASM=1 ^
    -s MODULARIZE=1 ^
    -s EXPORT_NAME="HuntmasterEngine" ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s INITIAL_MEMORY=16MB ^
    -s EXPORTED_FUNCTIONS="['_malloc','_free','_createEngine','_destroyEngine','_loadMasterCall','_processAudioChunk','_getSimilarityScore','_startSession','_endSession','_getSessionFeatureCount','_getSessionFeatures']" ^
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','getValue','setValue','HEAP8','HEAP16','HEAP32','HEAPU8','HEAPU16','HEAPU32','HEAPF32','HEAPF64']" ^
    -s SINGLE_FILE=1 ^
    -s ASSERTIONS=1 ^
    -s SAFE_HEAP=1 ^
    -o web/dist/huntmaster_engine.js

if %errorlevel% equ 0 (
    echo.
    echo Build successful!
    echo Output: web/dist/huntmaster_engine.js
    
    REM Get file size
    for %%I in (web\dist\huntmaster_engine.js) do echo File size: %%~zI bytes
    
    echo.
    echo To test the web demo:
    echo   1. Start a local web server in the 'web' directory
    echo   2. Open http://localhost:8000 in your browser
    echo.
    
    REM Launch local web server
    echo Launching local web server at http://localhost:8000 ...
    start powershell -NoExit -Command ^
        "& {`n" ^
        "    Set-Location 'E:\develop\Huntmaster\huntmaster-engine\web' `n" ^
        "    python -m http.server 8000 `n" ^
        "}"
    
    REM Open browser
    start http://localhost:8000
    
) else (
    echo.
    echo Build failed!
    pause
)

pause