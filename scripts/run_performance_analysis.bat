@echo off
:: Performance Profiling and Bottleneck Resolution Test Script (Windows)
:: This script builds and runs comprehensive performance analysis

setlocal enabledelayedexpansion

echo =========================================================================
echo Huntmaster Engine Performance Profiling and Bottleneck Resolution
echo =========================================================================
echo.

:: Configuration
set BUILD_DIR=build
set CMAKE_FLAGS=-DHUNTMASTER_BUILD_TOOLS=ON -DHUNTMASTER_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
set PROFILING_DEMO=performance_profiling_demo.exe

:: Check prerequisites
echo [INFO] Checking prerequisites...

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Please install CMake 3.16 or later.
    exit /b 1
)

where cl >nul 2>&1 || where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] No C++ compiler found. Please install Visual Studio Build Tools or MinGW.
    exit /b 1
)

echo [SUCCESS] Prerequisites check passed

:: Build the project
echo [INFO] Building Huntmaster Engine with Performance Profiling tools...

:: Create build directory
if exist "%BUILD_DIR%" (
    echo [INFO] Cleaning existing build directory...
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"

:: Configure with CMake
echo [INFO] Configuring build with CMake...
cd "%BUILD_DIR%"
cmake .. %CMAKE_FLAGS%

if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

:: Build the project
echo [INFO] Building project ^(this may take a few minutes^)...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

cd ..
echo [SUCCESS] Build completed successfully

:: Check if profiling demo was built
set DEMO_PATH=%BUILD_DIR%\tools\Release\%PROFILING_DEMO%
if not exist "%DEMO_PATH%" (
    set DEMO_PATH=%BUILD_DIR%\tools\%PROFILING_DEMO%
)

if exist "%DEMO_PATH%" (
    echo [SUCCESS] Performance profiling demo found: %DEMO_PATH%
) else (
    echo [ERROR] Performance profiling demo not found
    echo [INFO] Looking for available tools...
    dir "%BUILD_DIR%\tools\" /b 2>nul
    dir "%BUILD_DIR%\tools\Release\" /b 2>nul
    exit /b 1
)

:: Run performance profiling demo
echo [INFO] Running performance profiling demonstration...

:: Create output directory for results
set OUTPUT_DIR=performance_results_%date:~-4%%date:~4,2%%date:~7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
set OUTPUT_DIR=%OUTPUT_DIR: =0%
mkdir "%OUTPUT_DIR%"
cd "%OUTPUT_DIR%"

echo [INFO] Running comprehensive performance analysis...
echo [INFO] Output will be saved to: %cd%
echo.

:: Run the demo
"..\%DEMO_PATH%" 2>&1 | tee performance_analysis.log

if errorlevel 1 (
    echo [ERROR] Performance profiling demo failed
    exit /b 1
)

echo [SUCCESS] Performance profiling demo completed successfully
echo [INFO] Results saved in: %cd%

:: List generated files
echo [INFO] Generated files:
dir *.json *.log /b 2>nul

cd ..

:: Run basic performance tests if available
echo [INFO] Running basic performance tests...

set PERF_TEST=%BUILD_DIR%\tests\unit\Release\test_performance.exe
if not exist "%PERF_TEST%" (
    set PERF_TEST=%BUILD_DIR%\tests\unit\test_performance.exe
)

if exist "%PERF_TEST%" (
    echo [INFO] Running test_performance...
    "%PERF_TEST%"

    if errorlevel 1 (
        echo [WARNING] Some performance tests failed ^(this may be expected on slower systems^)
    ) else (
        echo [SUCCESS] Basic performance tests passed
    )
) else (
    echo [WARNING] Basic performance tests not found
)

:: Generate performance report summary
echo [INFO] Generating performance analysis summary...

:: Find latest results directory
for /f "delims=" %%d in ('dir performance_results_* /b /ad /o-d 2^>nul') do (
    set LATEST_RESULTS=%%d
    goto :found_results
)
goto :no_results

:found_results
echo.
echo =========================================================================
echo PERFORMANCE ANALYSIS SUMMARY
echo =========================================================================
echo Results Directory: %LATEST_RESULTS%
echo.

if exist "%LATEST_RESULTS%\performance_report.json" (
    echo [SUCCESS] ✓ Session performance report generated
)

if exist "%LATEST_RESULTS%\comprehensive_benchmark_results.json" (
    echo [SUCCESS] ✓ Comprehensive benchmark results generated
)

if exist "%LATEST_RESULTS%\performance_analysis.log" (
    echo [SUCCESS] ✓ Detailed analysis log generated

    :: Try to extract key metrics
    findstr /c:"Real-time ratio:" /c:"Average Processing Time" /c:"Peak Memory" /c:"Performance Category" "%LATEST_RESULTS%\performance_analysis.log" >nul 2>&1
    if not errorlevel 1 (
        echo.
        echo [INFO] Key Performance Metrics:
        findstr /c:"Real-time ratio:" /c:"Average Processing Time" /c:"Peak Memory" /c:"Performance Category" "%LATEST_RESULTS%\performance_analysis.log" | findstr /n "^" | findstr "^[1-9]:"
    )
)

echo.
echo [INFO] Next Steps:
echo   1. Review detailed results in: %LATEST_RESULTS%\
echo   2. Import JSON files into analysis tools
echo   3. Apply suggested optimizations
echo   4. Re-run benchmarks to validate improvements

goto :end

:no_results
echo [WARNING] No performance results found

:end
echo.
echo [SUCCESS] Performance profiling and bottleneck resolution analysis complete!
pause
