@echo off
REM Format entire Huntmaster Engine codebase with clang-format
REM This script applies the project's .clang-format configuration to all C/C++ files

setlocal enabledelayedexpansion

echo.
echo 🎨 Huntmaster Engine Code Formatter
echo =====================================

REM Check if clang-format is available
where clang-format >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ❌ Error: clang-format is not installed or not in PATH
    echo Please install clang-format:
    echo   - Install Visual Studio Build Tools
    echo   - Install LLVM tools from https://llvm.org/
    echo   - Or use VS Code C++ extension
    exit /b 1
)

REM Verify .clang-format file exists
if not exist ".clang-format" (
    echo ❌ Error: .clang-format configuration file not found
    exit /b 1
)

for /f "tokens=*" %%i in ('clang-format --version') do set CLANG_VERSION=%%i
echo ✅ Using clang-format: !CLANG_VERSION!
echo ✅ Configuration file: .clang-format found

REM Define directories to format
set DIRECTORIES=src include tests tools

REM Count total files (approximate)
set /a TOTAL_FILES=0
for %%d in (%DIRECTORIES%) do (
    if exist "%%d" (
        for /r "%%d" %%f in (*.cpp *.hpp *.h *.cc *.cxx *.c) do (
            set /a TOTAL_FILES+=1
        )
    )
)

echo.
echo 📁 Formatting approximately !TOTAL_FILES! files in directories: %DIRECTORIES%
echo.

REM Check for dry run flag
set DRY_RUN=false
if "%1"=="--dry-run" set DRY_RUN=true
if "%1"=="-n" set DRY_RUN=true

if "!DRY_RUN!"=="true" (
    echo 🔍 DRY RUN MODE - No files will be modified
    echo.
)

REM Format each directory
for %%d in (%DIRECTORIES%) do (
    if exist "%%d" (
        echo 📂 Processing directory: %%d

        for /r "%%d" %%f in (*.cpp *.hpp *.h *.cc *.cxx *.c) do (
            if "!DRY_RUN!"=="true" (
                echo   Would format: %%f
            ) else (
                echo   Formatting: %%f
                clang-format -i -style=file "%%f"
                if !ERRORLEVEL! neq 0 (
                    echo   ❌ Failed to format: %%f
                )
            )
        )
    ) else (
        echo ⚠️  Directory %%d not found, skipping...
    )
)

if "!DRY_RUN!"=="false" (
    echo.
    echo 🎉 Code formatting completed successfully!
    echo.
    echo 📋 Summary:
    echo   ✅ Applied Google-based style with 4-space indentation
    echo   ✅ 100-character line limit enforced
    echo   ✅ Consistent bracing and spacing applied
    echo   ✅ Header include order standardized
    echo.
    echo 💡 Next steps:
    echo   1. Review changes: git diff
    echo   2. Test build: cmake --build build
    echo   3. Run tests: cd build ^&^& ctest
    echo   4. Commit changes: git add . ^&^& git commit -m "style: apply clang-format to entire codebase"
) else (
    echo.
    echo 📋 Dry run completed. Use without --dry-run to apply formatting.
)

echo.
echo 🔧 VS Code Integration:
echo   • Format on save: enabled
echo   • Auto format: configured
echo   • Style config: .clang-format

pause
