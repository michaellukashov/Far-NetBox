@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_arm64
cmake -S . -B build-RelWithDebugInfo-ARM64 -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON -DPROJECT_PLATFORM=ARM64
cmake --build build-RelWithDebugInfo-ARM64 -j
