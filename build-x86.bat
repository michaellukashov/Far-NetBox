@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build-RelWithDebugInfo-x86 -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON -DPROJECT_PLATFORM=x86
cmake --build build-RelWithDebugInfo-x86 -j
