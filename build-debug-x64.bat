@echo off

rem === Change to script directory so relative paths work from any cwd ===
cd /d "%~dp0"
echo === Configuring and building NetBox (x64 Debug) ===
echo.

rem === Locate and call vcvarsall.bat to set up MSVC environment ===
call "%~dp0find-vs.bat"
if %errorlevel% neq 0 (
    echo ERROR: Failed to locate Visual Studio
    exit /b 1
)
call "%VCVARSALL%" x86_amd64
if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize VS environment
    exit /b 1
)

rem === Running CMake configure ===
cmake -S . -B build-Debug-x64 -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
if %errorlevel% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)
echo.

rem === Running CMake build ===
cmake --build build-Debug-x64 -j
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)
echo.
echo === Build completed successfully ===
