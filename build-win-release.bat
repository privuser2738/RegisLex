@echo off
REM ============================================================================
REM RegisLex - Windows Release Build Script
REM ============================================================================
REM This script builds a clean Windows release and packages it in dist/
REM
REM Prerequisites:
REM   - CMake 3.16 or later
REM   - Visual Studio 2019/2022 with C/C++ workload OR MinGW-w64
REM   - Git (optional, for version info)
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

REM Colors (Windows 10+)
set "GREEN=[92m"
set "YELLOW=[93m"
set "RED=[91m"
set "CYAN=[96m"
set "RESET=[0m"

REM Parse command line argument
set ACTION=rebuild
if not "%1"=="" set ACTION=%1

echo.
echo %CYAN%============================================================================%RESET%
echo %CYAN%  %PROJECT_NAME% - Windows Release Build%RESET%
echo %CYAN%============================================================================%RESET%
echo.

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo %RED%ERROR: CMake not found in PATH%RESET%
    echo Please install CMake from https://cmake.org/download/
    exit /b 1
)

REM Get CMake version
for /f "tokens=3" %%i in ('cmake --version ^| findstr /r "[0-9]*\.[0-9]*\.[0-9]*"') do set CMAKE_VER=%%i
echo %GREEN%[INFO]%RESET% CMake version: %CMAKE_VER%

REM Detect compiler
set GENERATOR=
set COMPILER_NAME=Unknown

REM Check for Visual Studio 2022
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe" (
    set "GENERATOR=Visual Studio 17 2022"
    set COMPILER_NAME=Visual Studio 2022 Professional
    goto :compiler_found
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" (
    set "GENERATOR=Visual Studio 17 2022"
    set COMPILER_NAME=Visual Studio 2022 Community
    goto :compiler_found
)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe" (
    set "GENERATOR=Visual Studio 17 2022"
    set COMPILER_NAME=Visual Studio 2022 Enterprise
    goto :compiler_found
)

REM Check for Visual Studio 2019
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe" (
    set "GENERATOR=Visual Studio 16 2019"
    set COMPILER_NAME=Visual Studio 2019 Professional
    goto :compiler_found
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" (
    set "GENERATOR=Visual Studio 16 2019"
    set COMPILER_NAME=Visual Studio 2019 Community
    goto :compiler_found
)

REM Check for MinGW
where gcc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "GENERATOR=MinGW Makefiles"
    set COMPILER_NAME=MinGW GCC
    goto :compiler_found
)

REM Check for Ninja + cl
where ninja >nul 2>&1
if %ERRORLEVEL% equ 0 (
    where cl >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        set "GENERATOR=Ninja"
        set COMPILER_NAME=Ninja + MSVC
        goto :compiler_found
    )
)

echo %RED%ERROR: No supported compiler found%RESET%
echo Please install Visual Studio 2019/2022 or MinGW-w64
exit /b 1

:compiler_found
echo %GREEN%[INFO]%RESET% Compiler: %COMPILER_NAME%
echo %GREEN%[INFO]%RESET% Generator: %GENERATOR%
echo.

REM Handle actions
if /i "%ACTION%"=="clean" goto :do_clean
if /i "%ACTION%"=="build" goto :do_build
if /i "%ACTION%"=="rebuild" goto :do_rebuild

echo %RED%ERROR: Unknown action '%ACTION%'%RESET%
echo Usage: %~nx0 [clean^|rebuild^|build]
exit /b 1

:do_clean
echo %YELLOW%[CLEAN]%RESET% Removing build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%" 2>nul
    if exist "%BUILD_DIR%" (
        echo %RED%ERROR: Failed to remove build directory%RESET%
        exit /b 1
    )
)
echo %YELLOW%[CLEAN]%RESET% Removing dist directory...
if exist "%DIST_DIR%" (
    rmdir /s /q "%DIST_DIR%" 2>nul
)
echo %GREEN%[DONE]%RESET% Clean completed
if /i "%ACTION%"=="clean" exit /b 0
goto :do_configure

:do_rebuild
echo %YELLOW%[REBUILD]%RESET% Starting clean rebuild...
call :do_clean
goto :do_configure

:do_build
if not exist "%BUILD_DIR%" goto :do_configure
echo %GREEN%[INFO]%RESET% Build directory exists, skipping configure
goto :do_compile

:do_configure
echo.
echo %CYAN%[CONFIGURE]%RESET% Running CMake configuration...
echo.

mkdir "%BUILD_DIR%" 2>nul

REM Configure with CMake
if "%GENERATOR%"=="MinGW Makefiles" (
    cmake -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DREGISLEX_BUILD_TESTS=ON -B "%BUILD_DIR%" -S .
) else if "%GENERATOR%"=="Ninja" (
    cmake -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DREGISLEX_BUILD_TESTS=ON -B "%BUILD_DIR%" -S .
) else (
    cmake -G "%GENERATOR%" -A x64 -DREGISLEX_BUILD_TESTS=ON -B "%BUILD_DIR%" -S .
)

if %ERRORLEVEL% neq 0 (
    echo.
    echo %RED%ERROR: CMake configuration failed%RESET%
    exit /b 1
)

echo.
echo %GREEN%[DONE]%RESET% Configuration completed
goto :do_compile

:do_compile
echo.
echo %CYAN%[BUILD]%RESET% Compiling %BUILD_TYPE% build...
echo.

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo.
    echo %RED%ERROR: Build failed%RESET%
    exit /b 1
)

echo.
echo %GREEN%[DONE]%RESET% Build completed successfully
goto :do_package

:do_package
echo.
echo %CYAN%[PACKAGE]%RESET% Creating distribution package...
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
echo %GREEN%[COPY]%RESET% Copying executables...
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
echo %GREEN%[COPY]%RESET% Copying libraries...
if exist "%BUILD_DIR%\lib\%BUILD_TYPE%\regislex_core.lib" (
    copy /y "%BUILD_DIR%\lib\%BUILD_TYPE%\regislex_core.lib" "%DIST_DIR%\lib\" >nul
) else if exist "%BUILD_DIR%\lib\regislex_core.lib" (
    copy /y "%BUILD_DIR%\lib\regislex_core.lib" "%DIST_DIR%\lib\" >nul
) else if exist "%BUILD_DIR%\lib\libregislex_core.a" (
    copy /y "%BUILD_DIR%\lib\libregislex_core.a" "%DIST_DIR%\lib\" >nul
)

REM Copy headers
echo %GREEN%[COPY]%RESET% Copying headers...
if exist "include\regislex" (
    xcopy /s /y /q "include\regislex\*" "%DIST_DIR%\include\regislex\" >nul 2>&1
)
if exist "include\platform" (
    xcopy /s /y /q "include\platform\*" "%DIST_DIR%\include\platform\" >nul 2>&1
)
if exist "include\database" (
    xcopy /s /y /q "include\database\*" "%DIST_DIR%\include\database\" >nul 2>&1
)

REM Copy documentation
echo %GREEN%[COPY]%RESET% Copying documentation...
if exist "README.md" copy /y "README.md" "%DIST_DIR%\" >nul
if exist "LICENSE" copy /y "LICENSE" "%DIST_DIR%\" >nul
if exist "docs" xcopy /s /y /q "docs\*" "%DIST_DIR%\docs\" >nul 2>&1

REM Copy resources/configs
echo %GREEN%[COPY]%RESET% Copying resources...
if exist "resources" xcopy /s /y /q "resources\*" "%DIST_DIR%\resources\" >nul 2>&1

REM Create version info file
echo %GREEN%[INFO]%RESET% Creating version info...
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
echo %GREEN%[INFO]%RESET% Creating launcher scripts...
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

REM Calculate package size
echo %GREEN%[INFO]%RESET% Calculating package size...
set SIZE=0
for /f "tokens=3" %%a in ('dir /s "%DIST_DIR%" ^| findstr "File(s)"') do set SIZE=%%a
echo Package size: %SIZE% bytes

REM List distribution contents
echo.
echo %CYAN%Distribution Contents:%RESET%
echo ----------------------
dir /s /b "%DIST_DIR%\*.exe" "%DIST_DIR%\*.lib" "%DIST_DIR%\*.a" 2>nul
echo.

REM Run tests if available
if exist "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex_tests.exe" (
    echo %CYAN%[TEST]%RESET% Running unit tests...
    "%BUILD_DIR%\bin\%BUILD_TYPE%\regislex_tests.exe"
    if %ERRORLEVEL% neq 0 (
        echo %YELLOW%[WARN]%RESET% Some tests failed
    ) else (
        echo %GREEN%[DONE]%RESET% All tests passed
    )
) else if exist "%BUILD_DIR%\bin\regislex_tests.exe" (
    echo %CYAN%[TEST]%RESET% Running unit tests...
    "%BUILD_DIR%\bin\regislex_tests.exe"
)

echo.
echo %GREEN%============================================================================%RESET%
echo %GREEN%  BUILD SUCCESSFUL%RESET%
echo %GREEN%============================================================================%RESET%
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
