@echo off
rem Helper script to locate Visual Studio 2022 vcvarsall.bat
rem Supports Community, Professional, Enterprise, and BuildTools editions
rem Uses vswhere.exe when available, falls back to common install paths

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARSALL="

rem Try vswhere.exe first (most reliable method)
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%a in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        if exist "%%a\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARSALL=%%a\VC\Auxiliary\Build\vcvarsall.bat"
            goto :found
        )
    )
)

rem Fallback: check common VS 2022 install paths
for %%E in (Enterprise Professional Community BuildTools) do (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat"
        goto :found
    )
)

rem Fallback: check non-standard paths (e.g., VS 18.x)
for /d %%D in ("C:\Program Files\Microsoft Visual Studio\*") do (
    for %%E in (Enterprise Professional Community BuildTools) do (
        if exist "%%D\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARSALL=%%D\%%E\VC\Auxiliary\Build\vcvarsall.bat"
            goto :found
        )
    )
)

echo ERROR: Could not find vcvarsall.bat
exit /b 1

:found
exit /b 0
