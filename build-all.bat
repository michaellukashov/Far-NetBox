@echo off
echo === Configuring and building NetBox (x64 RelWithDebugInfo) ===
echo.

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize VS environment
    exit /b 1
)
echo.

echo === Running CMake configure ===
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
if %errorlevel% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)
echo.

echo === Running CMake build ===
cmake --build build-RelWithDebugInfo -j
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)
echo.
echo === Build completed successfully ===
