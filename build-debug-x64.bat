@echo off
echo === Configuring and building NetBox (x64 Debug) ===
echo.

rem === Calling vcvarsall.bat to set up MSVC environment ===
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
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
