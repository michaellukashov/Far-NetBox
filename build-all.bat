@echo off
echo === Configuring and building NetBox (x64 RelWithDebugInfo) ===
echo.

rem === Calling vcvarsall.bat to set up MSVC environment ===
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize VS environment
    exit /b 1
)

rem === Debug: Show environment variables ===
echo INCLUDE=%INCLUDE%
echo LIB=%LIB%
echo PATH=%PATH%
echo.

rem === Running CMake configure ===
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
if %errorlevel% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)
echo.

rem === Running CMake build ===
cmake --build build-RelWithDebugInfo -j
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)
echo.
echo === Build completed successfully ===
