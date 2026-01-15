message(STATUS "CMake version: ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.15)

project(NetBox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Remove for compiler-specific features
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type")
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Zi")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /Zi")
  # /MP Multi-processor compilation
  # /Zi Generate debug info

  # set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  # set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
# message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

if(NOT DEFINED PROJECT_PLATFORM OR PROJECT_PLATFORM STREQUAL "")
if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
  set(PROJECT_PLATFORM "ARM64" CACHE STRING "Platform")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(PROJECT_PLATFORM "x64" CACHE STRING "Platform")
else()
  set(PROJECT_PLATFORM "x86" CACHE STRING "Platform")
endif()
endif()

if(PROJECT_PLATFORM STREQUAL "x86" AND CMAKE_BUILD_TYPE STREQUAL "Release")
  set(OPT_USE_UNITY_BUILD ON CACHE STRING "Default use unity build")
else()
  set(OPT_USE_UNITY_BUILD OFF CACHE STRING "Default use unity build")
endif()
set(OPT_CREATE_PLUGIN_DIR OFF CACHE STRING "Create plugin dir")

#-------------------------------------------------------------------------------

include(CMakeParseArguments)
include(${CMAKE_ROOT}/Modules/CMakeDetermineSystem.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
# include(CMakeDetermineCXXCompiler)
message("System: ${CMAKE_SYSTEM_NAME}, PROJECT_PLATFORM: ${PROJECT_PLATFORM}")
# message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
# message("MSVC_VERSION: ${MSVC_VERSION}")
# message("CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
# message("CMAKE_COMPILER_IS_MSVC: ${CMAKE_COMPILER_IS_MSVC}")
# message("CMAKE_COMPILER_IS_MINGW: ${CMAKE_COMPILER_IS_MINGW}")
# message("CMAKE_COMPILER_IS_CLANG: ${CMAKE_COMPILER_IS_CLANG}")
# message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
# message("CMAKE_CXX_COMPILER_ARCHITECTURE_ID: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")

#-------------------------------------------------------------------------------

set(LIBNEON_DEFS -DNE_LFS -DNOCRYPT -DNE_HAVE_SSL -DHAVE_OPENSSL -DNE_HAVE_DAV -DNE_HAVE_ZLIB -DZLIB_COMPAT -DHAVE_EXPAT -DHAVE_EXPAT_H) #" -DUSE_GETADDRINFO")
set(LIBEXPAT_DEFS -DCOMPILED_FROM_DSP -DXML_STATIC)
set(NETBOX_DEFS -DNOMINMAX -DMPEXT -DWINSCP -DFARPLUGIN)
set(NETBOX_DEFS ${NETBOX_DEFS} -DFASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX)
set(NETBOX_DEFS ${NETBOX_DEFS} -DUSE_DLMALLOC -DUSE_DL_PREFIX -DNO_MALLINFO -DUSE_LOCKS=1)
set(NETBOX_DEFS ${NETBOX_DEFS} -DHIDE_TODO)
set(NETBOX_DEFS ${NETBOX_DEFS} -DNEED_LIBS3)
set(NETBOX_DEFS ${NETBOX_DEFS} -DNO_GZIP -DSTRICT -DNOCRYPT)
set(NETBOX_DEFS ${NETBOX_DEFS} -D_WIN32_WINNT=0x0501 -D_LIB -D_WINDOWS -DWIN32 -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
set(NETBOX_DEFS ${NETBOX_DEFS} -D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS -D_DISABLE_VECTOR_ANNOTATION)
set(NETBOX_DEFS ${NETBOX_DEFS} -DCOM_NO_WINDOWS_H)
#-D_WINDLL -D_USRDLL
if (MSVC_VERSION GREATER 1899)
set(NETBOX_DEFS ${NETBOX_DEFS} /Zc:threadSafeInit- /bigobj)
endif()

set(NETBOX_C_FLAGS )

if(PROJECT_PLATFORM STREQUAL "x64")
set(NETBOX_PLATFORM_FLAGS ${NETBOX_PLATFORM_FLAGS} -DWIN64 -D_WIN64 -D_AMD64_)
endif()

if(PROJECT_PLATFORM STREQUAL "x86")
# set(NETBOX_PLATFORM_FLAGS ${NETBOX_PLATFORM_FLAGS} -DWIN32 -D_WIN32)
endif()

if(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} -x c --std=c99)
endif()

set(NETBOX_FLAGS_RELEASE -U_DEBUG -DNDEBUG)
set(NETBOX_FLAGS_DEBUG -D_DEBUG -DDEBUG)

set(NETBOX_UNICODE_FLAGS -D_UNICODE -DUNICODE)

if(MSVC)

set(NETBOX_C_WARNING_FLAGS "")
set(NETBOX_CXX_WARNING_FLAGS "")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

elseif(CMAKE_COMPILER_IS_MINGW)

set(NETBOX_C_WARNING_FLAGS -Wno-unknown-pragmas)
set(NETBOX_CXX_WARNING_FLAGS ${NETBOX_C_WARNING_FLAGS} -Wno-attributes -Wno-write-strings -Wno-old-style-cast -Wno-effc++ -Wno-unknown-pragmas -Wno-unused-value)
set(NETBOX_CXX_FLAGS ${NETBOX_CXX_FLAGS} -Woverloaded-virtual -Wnon-virtual-dtor)
set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS} -static -s -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code)
set(NETBOX_FLAGS_RELEASE ${NETBOX_FLAGS_RELEASE})
set(NETBOX_FLAGS_DEBUG ${NETBOX_FLAGS_DEBUG})

endif()

set(NETBOX_C_FLAGS ${NETBOX_DEFS} ${NETBOX_C_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_C_WARNING_FLAGS})
set(NETBOX_CXX_FLAGS ${NETBOX_DEFS} ${NETBOX_CXX_FLAGS} ${NETBOX_PLATFORM_FLAGS} ${NETBOX_CXX_WARNING_FLAGS})

if(MSVC)

set(NETBOX_DLL_LINK_FLAGS
  /NODEFAULTLIB:MSVCURT.LIB
  /NODEFAULTLIB:MSVCURTD.LIB
  /NODEFAULTLIB:MSVCPRT.LIB
  /NODEFAULTLIB:MSVCPRTD.LIB
  /NODEFAULTLIB:LIBC.LIB
  /NODEFAULTLIB:LIBCMT.LIB
  /NODEFAULTLIB:LIBCMTD.LIB
  /NODEFAULTLIB:MFC100U.LIB
  /NODEFAULTLIB:MFC100UD.LIB
  /NODEFAULTLIB:ATL.LIB
  /NODEFAULTLIB:LIBCRT.LIB
  /NODEFAULTLIB:LIBCRTD.LIB
  /NODEFAULTLIB:MSVCRT.LIB
  /NODEFAULTLIB:MSVCRTD.LIB
  /NODEFAULTLIB:MFCS100U.LIB
  /NODEFAULTLIB:MFCS100UD.LIB
  /NODEFAULTLIB:LIBCPMT.LIB
  /NODEFAULTLIB:LIBCPMTD.LIB
  /NODEFAULTLIB:LIBCRYPTO.LIB
  /NODEFAULTLIB:LIBSSL.LIB
  /NODEFAULTLIB:PUTTY.LIB
  /NODEFAULTLIB:KERNEL32.LIB
  /NODEFAULTLIB:USER32.LIB
  /NODEFAULTLIB:VERSION.LIB
  /DELAYLOAD:ws2_32.dll
  /DELAYLOAD:oleaut32.dll
  /DELAYLOAD:shlwapi.dll
  /DELAYLOAD:crypt32.dll
  /MANIFEST:NO
  /SUBSYSTEM:WINDOWS
  /NOLOGO
  /ignore:4099
  /ignore:4197
  /ignore:4199
)
#/DELAYLOAD:gdi32.dll

set(NETBOX_DLL_LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS} /RELEASE /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG)
set(NETBOX_DLL_LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS} /DEBUG)

elseif(CMAKE_COMPILER_IS_MINGW)
set(NETBOX_DLL_LINK_FLAGS )
set(NETBOX_DLL_LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS})
set(NETBOX_DLL_LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS})
endif()

if(MSVC)

#find_program(NASM_EXECUTABLE nasm)
# message("nasm: ${NASM_EXECUTABLE}")
set(NASM_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/tools/nasm.exe)

macro(compile_asm_files)
  cmake_parse_arguments(ARG "" "RESULT" "ASM_FILES" ${ARGN})
  if(NOT ARG_ASM_FILES)
      set(ARG_ASM_FILES " ")
  endif()
#  message("ARG_ASM_FILES: ${ARG_ASM_FILES}")
  if(PROJECT_PLATFORM STREQUAL "x64")
    set(_nasm_args win64)
  else()
    set(_nasm_args win32)
  endif()
  set(${ARG_RESULT} )
  foreach(_asm_file ${ARG_ASM_FILES})
#    message("asm_file: ${_asm_file}")
    set(_asm_source ${CMAKE_CURRENT_SOURCE_DIR}/${_asm_file})
    get_filename_component(_asm_source_fn ${_asm_source} NAME)
#    message("_asm_source_fn: ${_asm_source_fn}")
    # file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${_asm_source_fn}" _asm_file_name)
    # message("_asm_file_name: ${_asm_file_name}")
    set(_asm_object ${CMAKE_CURRENT_BINARY_DIR}/${_asm_source_fn}.obj)
    add_custom_command(
      OUTPUT ${_asm_object}
      COMMAND ${NASM_EXECUTABLE}
      ARGS -f ${_nasm_args} -o ${_asm_object} ${_asm_source}
      DEPENDS ${_asm_source}
    )
    set(${ARG_RESULT} ${${ARG_RESULT}} ${_asm_object})
  endforeach(_asm_file)
endmacro()

#[[set(NETBOX_SOURCES ${NETBOX_SOURCES}
  vc_crt_fix_impl.cpp
  ${ASM_OBJECTS}
)]]

endif(MSVC)
#-------------------------------------------------------------------------------

# OpenSSL - now built from libs/openssl-3/CMakeLists.txt
# See cmake/OpenSSL.cmake for configuration
#-------------------------------------------------------------------------------

# Add OpenSSL subdirectory
add_subdirectory(libs/openssl-3)

# OpenSSL libraries are now built from libs/openssl-3/CMakeLists.txt
# See cmake/OpenSSL.cmake for configuration

#-------------------------------------------------------------------------------
# Third-party Libraries (modularized in Phase 3)
# Each library is now built from its own libs/[library]/CMakeLists.txt
# See cmake/[Library].cmake for configuration
#-------------------------------------------------------------------------------

add_subdirectory(libs/atlmfc)
add_subdirectory(libs/PuTTY)
add_subdirectory(libs/tinyxml2)
add_subdirectory(libs/neon)
add_subdirectory(libs/expat)
add_subdirectory(libs/zlib-ng)
add_subdirectory(libs/dlmalloc)
add_subdirectory(libs/tinylog)
add_subdirectory(libs/fmt)
add_subdirectory(libs/libs3)

#===============================================================================
# NetBox Main Plugin (src/CMakeLists.txt)
#===============================================================================

add_subdirectory(src)
