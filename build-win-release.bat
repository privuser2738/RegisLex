@echo off
REM ============================================================================
REM RegisLex - Windows Release Build Script
REM ============================================================================
REM This script builds a clean Windows release and packages it in dist/
REM
REM Prerequisites:
REM   - CMake 3.16 or later
REM   - Visual Studio 2019/2022/2025 with C/C++ workload
REM
REM Usage:
REM   build-win-release.bat [clean|rebuild|build]
REM     clean   - Remove build and dist directories
REM     rebuild - Clean then build (default)
REM     build   - Build without cleaning
REM ============================================================================

setlocal EnableDelayedExpansion

REM Configuration
set PROJECT_NAME=RegisLex
set BUILD_DIR=build-release
set DIST_DIR=dist
set BUILD_TYPE=Release

REM Parse command line argument
set ACTION=rebuild
if not "%1"=="" set ACTION=%1

echo.
echo ============================================================================
echo   %PROJECT_NAME% - Windows Release Build
echo ============================================================================
echo.

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    exit /b 1
)

REM Get CMake version
for /f "tokens=3" %%i in ('cmake --version ^| findstr /r "[0-9]*\.[0-9]*\.[0-9]*"') do set CMAKE_VER=%%i
echo [INFO] CMake version: %CMAKE_VER%

REM ============================================================================
REM Find Visual Studio Installation using vswhere
REM ============================================================================
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

set VS_PATH=
set VS_VERSION=
set GENERATOR=

if exist %VSWHERE% (
    echo [INFO] Using vswhere to locate Visual Studio...

    REM Get the installation path
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set VS_PATH=%%i
    )

    REM Get the version
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do (
        set VS_VERSION=%%i
    )
)

if not defined VS_PATH (
    REM Fallback: check common locations
    if exist "%ProgramFiles%\Microsoft Visual Studio\2025\Community\Common7\IDE\devenv.exe" (
        set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2025\Community"
        set VS_VERSION=18.0
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.exe" (
        set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\18\Community"
        set VS_VERSION=18.0
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" (
        set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community"
        set VS_VERSION=17.0
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe" (
        set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Professional"
        set VS_VERSION=17.0
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" (
        set "VS_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community"
        set VS_VERSION=16.0
    )
)

if not defined VS_PATH (
    echo ERROR: Visual Studio with C++ tools not found
    echo Please install Visual Studio with "Desktop development with C++" workload
    exit /b 1
)

echo [INFO] Visual Studio found at: %VS_PATH%
echo [INFO] Visual Studio version: %VS_VERSION%

REM Determine CMake generator based on VS version
set VS_MAJOR=%VS_VERSION:~0,2%
if "%VS_MAJOR%"=="18" (
    REM Note: CMake 4.x uses "Visual Studio 18 2026" for VS 2025/18
    set "GENERATOR=Visual Studio 18 2026"
    set COMPILER_NAME=Visual Studio 2025
) else if "%VS_MAJOR%"=="17" (
    set "GENERATOR=Visual Studio 17 2022"
    set COMPILER_NAME=Visual Studio 2022
) else if "%VS_MAJOR%"=="16" (
    set "GENERATOR=Visual Studio 16 2019"
    set COMPILER_NAME=Visual Studio 2019
) else (
    REM Try to auto-detect
    set "GENERATOR=Visual Studio 17 2022"
    set COMPILER_NAME=Visual Studio
)

echo [INFO] Using generator: %GENERATOR%
echo.

REM ============================================================================
REM Setup Visual Studio environment
REM ============================================================================
set VCVARSALL="%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"

if exist %VCVARSALL% (
    echo [INFO] Setting up Visual Studio environment...
    call %VCVARSALL% x64 >nul 2>&1
    if %ERRORLEVEL% neq 0 (
        echo [WARN] Failed to setup VS environment, continuing anyway...
    )
) else (
    echo [WARN] vcvarsall.bat not found, CMake will try to find compiler
)

REM ============================================================================
REM Handle actions
REM ============================================================================
if /i "%ACTION%"=="clean" goto :do_clean
if /i "%ACTION%"=="build" goto :do_build
if /i "%ACTION%"=="rebuild" goto :do_rebuild

echo ERROR: Unknown action '%ACTION%'
echo Usage: %~nx0 [clean^|rebuild^|build]
exit /b 1

:do_clean
echo [CLEAN] Removing build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%" 2>nul
    if exist "%BUILD_DIR%" (
        echo ERROR: Failed to remove build directory
        exit /b 1
    )
)
echo [CLEAN] Removing dist directory...
if exist "%DIST_DIR%" (
    rmdir /s /q "%DIST_DIR%" 2>nul
)
echo [DONE] Clean completed
if /i "%ACTION%"=="clean" exit /b 0
goto :do_configure

:do_rebuild
echo [REBUILD] Starting clean rebuild...
call :do_clean
goto :do_configure

:do_build
if not exist "%BUILD_DIR%" goto :do_configure
echo [INFO] Build directory exists, skipping configure
goto :do_compile

:do_configure
echo.
echo [CONFIGURE] Running CMake configuration...
echo.

mkdir "%BUILD_DIR%" 2>nul

REM Try with the detected generator first
cmake -G "%GENERATOR%" -A x64 -DREGISLEX_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -B "%BUILD_DIR%" -S .

if %ERRORLEVEL% neq 0 (
    echo.
    echo [WARN] Generator "%GENERATOR%" failed, trying Ninja...

    REM Try Ninja as fallback
    where ninja >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        rmdir /s /q "%BUILD_DIR%" 2>nul
        mkdir "%BUILD_DIR%" 2>nul
        cmake -G "Ninja" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DREGISLEX_BUILD_TESTS=OFF -B "%BUILD_DIR%" -S .

        if %ERRORLEVEL% neq 0 (
            echo.
            echo ERROR: CMake configuration failed
            echo.
            echo Troubleshooting:
            echo   1. Run this from "Developer Command Prompt for VS"
            echo   2. Or run: "%VS_PATH%\Common7\Tools\VsDevCmd.bat"
            echo   3. Then run this script again
            exit /b 1
        )
    ) else (
        echo.
        echo ERROR: CMake configuration failed
        echo.
        echo Troubleshooting:
        echo   1. Open "Developer Command Prompt for VS 2025" from Start Menu
        echo   2. Navigate to: %CD%
        echo   3. Run: %~nx0
        echo.
        echo Or manually run:
        echo   "%VS_PATH%\Common7\Tools\VsDevCmd.bat"
        echo   %~nx0
        exit /b 1
    )
)

echo.
echo [DONE] Configuration completed
goto :do_compile

:do_compile
echo.
echo [BUILD] Compiling %BUILD_TYPE% build...
echo.

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo [DONE] Build completed successfully
goto :do_package

:do_package
echo.
echo [PACKAGE] Creating distribution package...
echo.

REM Create dist directory structure
mkdir "%DIST_DIR%" 2>nul
mkdir "%DIST_DIR%\bin" 2>nul
mkdir "%DIST_DIR%\lib" 2>nul
mkdir "%DIST_DIR%\include" 2>nul
mkdir "%DIST_DIR%\include\regislex" 2>nul
mkdir "%DIST_DIR%\docs" 2>nul
mkdir "%DIST_DIR%\resources" 2>nul

REM Copy executables
echo [COPY] Copying executables...
if exist "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex.exe" (
    copy /y "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex.exe" "%DIST_DIR%\bin\" >nul
) else if exist "%BUILD_DIR%\bin\regislex.exe" (
    copy /y "%BUILD_DIR%\bin\regislex.exe" "%DIST_DIR%\bin\" >nul
)

if exist "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex-cli.exe" (
    copy /y "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex-cli.exe" "%DIST_DIR%\bin\" >nul
) else if exist "%BUILD_DIR%\bin\regislex-cli.exe" (
    copy /y "%BUILD_DIR%\bin\regislex-cli.exe" "%DIST_DIR%\bin\" >nul
)

REM Copy libraries
echo [COPY] Copying libraries...
if exist "%BUILD_DIR%\lib\%BUILD_TYPE%\regislex_core.lib" (
    copy /y "%BUILD_DIR%\lib\%BUILD_TYPE%\regislex_core.lib" "%DIST_DIR%\lib\" >nul
) else if exist "%BUILD_DIR%\lib\regislex_core.lib" (
    copy /y "%BUILD_DIR%\lib\regislex_core.lib" "%DIST_DIR%\lib\" >nul
) else if exist "%BUILD_DIR%\lib\libregislex_core.a" (
    copy /y "%BUILD_DIR%\lib\libregislex_core.a" "%DIST_DIR%\lib\" >nul
)

REM Copy headers
echo [COPY] Copying headers...
if exist "include\regislex" (
    xcopy /s /y /q "include\regislex\*" "%DIST_DIR%\include\regislex\" >nul 2>&1
)
if exist "include\platform" (
    mkdir "%DIST_DIR%\include\platform" 2>nul
    xcopy /s /y /q "include\platform\*" "%DIST_DIR%\include\platform\" >nul 2>&1
)
if exist "include\database" (
    mkdir "%DIST_DIR%\include\database" 2>nul
    xcopy /s /y /q "include\database\*" "%DIST_DIR%\include\database\" >nul 2>&1
)

REM Copy documentation
echo [COPY] Copying documentation...
if exist "README.md" copy /y "README.md" "%DIST_DIR%\" >nul
if exist "LICENSE" copy /y "LICENSE" "%DIST_DIR%\" >nul
if exist "docs" xcopy /s /y /q "docs\*" "%DIST_DIR%\docs\" >nul 2>&1

REM Copy resources/configs
echo [COPY] Copying resources...
if exist "resources" xcopy /s /y /q "resources\*" "%DIST_DIR%\resources\" >nul 2>&1

REM Create version info file
echo [INFO] Creating version info...
(
    echo RegisLex Distribution Package
    echo ==============================
    echo.
    echo Version: 1.0.0
    echo Build Type: %BUILD_TYPE%
    echo Platform: Windows x64
    echo Build Date: %DATE% %TIME%
    echo.
    echo Compiler: %COMPILER_NAME%
    echo CMake Version: %CMAKE_VER%
) > "%DIST_DIR%\VERSION.txt"

REM Get git info if available
where git >nul 2>&1
if %ERRORLEVEL% equ 0 (
    for /f "tokens=*" %%i in ('git rev-parse --short HEAD 2^>nul') do set GIT_HASH=%%i
    for /f "tokens=*" %%i in ('git describe --tags --always 2^>nul') do set GIT_TAG=%%i
    if defined GIT_HASH (
        echo Git Commit: !GIT_HASH! >> "%DIST_DIR%\VERSION.txt"
    )
    if defined GIT_TAG (
        echo Git Tag: !GIT_TAG! >> "%DIST_DIR%\VERSION.txt"
    )
)

REM Create quick start script
echo [INFO] Creating launcher scripts...
(
    echo @echo off
    echo REM RegisLex Server Launcher
    echo cd /d "%%~dp0"
    echo bin\regislex.exe %%*
) > "%DIST_DIR%\start-server.bat"

(
    echo @echo off
    echo REM RegisLex CLI
    echo cd /d "%%~dp0"
    echo bin\regislex-cli.exe %%*
) > "%DIST_DIR%\regislex.bat"

REM List distribution contents
echo.
echo Distribution Contents:
echo ----------------------
dir /b "%DIST_DIR%\bin\*.exe" 2>nul
dir /b "%DIST_DIR%\lib\*.lib" 2>nul
dir /b "%DIST_DIR%\lib\*.a" 2>nul
echo.

REM Run tests if available
set TEST_EXE=
if exist "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex_tests.exe" (
    set "TEST_EXE=%BUILD_DIR%\bin\%BUILD_TYPE%\regislex_tests.exe"
) else if exist "%BUILD_DIR%\bin\regislex_tests.exe" (
    set "TEST_EXE=%BUILD_DIR%\bin\regislex_tests.exe"
)

if defined TEST_EXE (
    echo [TEST] Running unit tests...
    "%TEST_EXE%"
    if %ERRORLEVEL% neq 0 (
        echo [WARN] Some tests failed
    ) else (
        echo [DONE] All tests passed
    )
)

echo.
echo ============================================================================
echo   BUILD SUCCESSFUL
echo ============================================================================
echo.
echo Distribution package created in: %CD%\%DIST_DIR%\
echo.
echo To start the server:
echo   cd %DIST_DIR%
echo   start-server.bat -p 8080
echo.
echo To use the CLI:
echo   cd %DIST_DIR%
echo   regislex.bat help
echo.

exit /b 0
