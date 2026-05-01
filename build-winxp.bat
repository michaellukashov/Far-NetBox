@echo off

rem === Change to script directory so relative paths work from any cwd ===
cd /d "%~dp0"
echo === Configuring and building NetBox (WinXP x86 RelWithDebugInfo) ===
echo.

rem === Locate and call vcvarsall.bat to set up MSVC environment ===
call "%~dp0find-vs.bat"
if %errorlevel% neq 0 (
    echo ERROR: Failed to locate Visual Studio
    exit /b 1
)
call "%VCVARSALL%" amd64_x86
if %errorlevel% neq 0 (
    echo ERROR: Failed to initialize VS environment
    exit /b 1
)

rem === Running CMake configure ===
cmake -S . -B build-RelWithDebugInfo-winxp -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON -DPROJECT_PLATFORM=x86 -DOPT_USE_UNITY_BUILD=OFF
if %errorlevel% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)
echo.

rem === Running CMake build ===
cmake --build build-RelWithDebugInfo-winxp -j
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)
echo.

rem === Verify Windows XP subsystem version ===
set "DLL_PATH=Far3_x86\Plugins\NetBox\NetBox.dll"
if exist "%DLL_PATH%" (
    echo === Verifying WinXP binary compatibility ===
    dumpbin /headers "%DLL_PATH%" | findstr /i "subsystem"
    if %errorlevel% neq 0 (
        echo WARNING: Could not verify subsystem version
    )
) else (
    echo WARNING: DLL not found at %DLL_PATH% — skipping verification
)

echo.
echo === Build completed successfully ===
echo.
echo To verify XP compatibility manually, run:
echo   dumpbin /headers %DLL_PATH% ^| findstr /i "subsystem"
echo Expected output: 5.01 (not 6.00)
