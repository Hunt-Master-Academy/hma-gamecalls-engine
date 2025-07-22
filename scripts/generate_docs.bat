@echo off
REM Documentation Generation Script for Huntmaster Audio Engine (Windows)
REM This script generates HTML documentation using Doxygen

setlocal enabledelayedexpansion

REM Configuration
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "DOCS_OUTPUT_DIR=%PROJECT_ROOT%\docs\api"
set "DOXYFILE=%PROJECT_ROOT%\Doxyfile"

echo Huntmaster Audio Engine - Documentation Generator
echo ==================================================

REM Check if Doxygen is installed
where doxygen >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: Doxygen is not installed or not in PATH
    echo Please install Doxygen from https://www.doxygen.nl/download.html
    echo Make sure to add Doxygen to your system PATH
    pause
    exit /b 1
)

REM Display Doxygen version
for /f "tokens=*" %%i in ('doxygen --version') do set "DOXYGEN_VERSION=%%i"
echo Found Doxygen version: !DOXYGEN_VERSION!

REM Check if Doxyfile exists
if not exist "%DOXYFILE%" (
    echo Error: Doxyfile not found at %DOXYFILE%
    pause
    exit /b 1
)

REM Create output directory if it doesn't exist
if not exist "%DOCS_OUTPUT_DIR%" mkdir "%DOCS_OUTPUT_DIR%"

REM Clean previous documentation
if exist "%DOCS_OUTPUT_DIR%\html" (
    echo Cleaning previous documentation...
    rmdir /s /q "%DOCS_OUTPUT_DIR%\html"
)

REM Change to project root directory
cd /d "%PROJECT_ROOT%"

REM Generate documentation
echo Generating documentation...
echo Working directory: %CD%
echo Output directory: %DOCS_OUTPUT_DIR%

doxygen "%DOXYFILE%"
if %ERRORLEVEL% EQU 0 (
    echo ✓ Documentation generated successfully!

    REM Check if HTML was generated
    if exist "%DOCS_OUTPUT_DIR%\html" (
        echo ✓ HTML documentation available at: %DOCS_OUTPUT_DIR%\html\index.html

        REM Count generated files
        for /f %%i in ('dir "%DOCS_OUTPUT_DIR%\html\*.html" /b /s ^| find /c /v ""') do set "HTML_FILES=%%i"
        echo Generated !HTML_FILES! HTML files

        REM Display main entry points
        echo.
        echo Main documentation entry points:
        echo   • Main page: %DOCS_OUTPUT_DIR%\html\index.html
        echo   • Class index: %DOCS_OUTPUT_DIR%\html\annotated.html
        echo   • File index: %DOCS_OUTPUT_DIR%\html\files.html
        echo   • Namespace index: %DOCS_OUTPUT_DIR%\html\namespaces.html

        REM Optional: Open documentation in browser (uncomment if desired)
        REM start "" "%DOCS_OUTPUT_DIR%\html\index.html"

    ) else (
        echo Warning: HTML directory not found after generation
    )

) else (
    echo ✗ Documentation generation failed!
    pause
    exit /b 1
)

echo.
echo Documentation generation complete!
pause
