#===============================================================================
# NetBox Compiler Flags and Options
#
# Purpose: Centralized compiler, linker, and platform-specific configuration
#
# Provides:
#   - NETBOX_DEFS: Main project defines
#   - NETBOX_C_FLAGS / NETBOX_CXX_FLAGS: C/C++ compiler flags
#   - NETBOX_PLATFORM_FLAGS: Platform-specific flags (x64, x86, ARM64)
#   - NETBOX_UNICODE_FLAGS: Unicode-related defines
#   - NETBOX_FLAGS_RELEASE / NETBOX_FLAGS_DEBUG: Build type flags
#   - NETBOX_C_WARNING_FLAGS / NETBOX_CXX_WARNING_FLAGS: Warning flags
#   - NETBOX_DLL_LINK_FLAGS: Linker flags for DLL builds
#===============================================================================

#-------------------------------------------------------------------------------
# Library-Specific Defines
#-------------------------------------------------------------------------------

set(LIBNEON_DEFS
  -DNE_LFS
  -DNOCRYPT
  -DNE_HAVE_SSL
  -DHAVE_OPENSSL
  -DNE_HAVE_DAV
  -DNE_HAVE_ZLIB
  -DZLIB_COMPAT
  -DHAVE_EXPAT
  -DHAVE_EXPAT_H
)

set(LIBEXPAT_DEFS
  -DCOMPILED_FROM_DSP
  -DXML_STATIC
)

#-------------------------------------------------------------------------------
# Main Project Defines
#-------------------------------------------------------------------------------

set(NETBOX_DEFS
  -DNOMINMAX
  -DMPEXT
  -DWINSCP
  -DFARPLUGIN
  -DFASTDELEGATE_ALLOW_FUNCTION_TYPE_SYNTAX
  -DUSE_DLMALLOC
  -DUSE_DL_PREFIX
  -DNO_MALLINFO
  -DUSE_LOCKS=1
  -DHIDE_TODO
  -DNEED_LIBS3
  -DNO_GZIP
  -DSTRICT
  -DNOCRYPT
  -D_WIN32_WINNT=0x0501
  -D_LIB
  -D_WINDOWS
  -DWIN32
  -D_SCL_SECURE_NO_WARNINGS
  -D_CRT_SECURE_NO_WARNINGS
  -D_WINSOCK_DEPRECATED_NO_WARNINGS
  -D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
  -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
  -D_DISABLE_VECTOR_ANNOTATION
  -DCOM_NO_WINDOWS_H
)

# MSVC-specific flags for newer versions
if(MSVC_VERSION GREATER 1899)
  set(NETBOX_DEFS ${NETBOX_DEFS}
    /Zc:threadSafeInit-
    /bigobj
  )
endif()

#-------------------------------------------------------------------------------
# Base C Flags
#-------------------------------------------------------------------------------

set(NETBOX_C_FLAGS )

#-------------------------------------------------------------------------------
# Platform-Specific Flags
#-------------------------------------------------------------------------------

if(PROJECT_PLATFORM STREQUAL "x64")
  set(NETBOX_PLATFORM_FLAGS
    -DWIN64
    -D_WIN64
    -D_AMD64_
  )
elseif(PROJECT_PLATFORM STREQUAL "x86")
  set(NETBOX_PLATFORM_FLAGS )
endif()

# MinGW-specific C flags
if(CMAKE_COMPILER_IS_MINGW)
  set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS}
    -x c
    --std=c99
  )
endif()

#-------------------------------------------------------------------------------
# Build Type Flags
#-------------------------------------------------------------------------------

set(NETBOX_FLAGS_RELEASE -U_DEBUG -DNDEBUG)
set(NETBOX_FLAGS_DEBUG -D_DEBUG -DDEBUG)

#-------------------------------------------------------------------------------
# Unicode Flags
#-------------------------------------------------------------------------------

set(NETBOX_UNICODE_FLAGS -D_UNICODE -DUNICODE)

#-------------------------------------------------------------------------------
# Warning Flags
#-------------------------------------------------------------------------------

if(MSVC)

  set(NETBOX_C_WARNING_FLAGS "")
  set(NETBOX_CXX_WARNING_FLAGS "")

elseif(CMAKE_COMPILER_IS_MINGW)

  set(NETBOX_C_WARNING_FLAGS
    -Wno-unknown-pragmas
  )

  set(NETBOX_CXX_WARNING_FLAGS
    ${NETBOX_C_WARNING_FLAGS}
    -Wno-attributes
    -Wno-write-strings
    -Wno-old-style-cast
    -Wno-effc++
    -Wno-unknown-pragmas
    -Wno-unused-value
  )

  set(NETBOX_CXX_FLAGS ${NETBOX_CXX_FLAGS}
    -Woverloaded-virtual
    -Wnon-virtual-dtor
  )

  set(NETBOX_C_FLAGS ${NETBOX_C_FLAGS}
    -static
    -s
    -Wall
    -Wextra
    -pedantic
    -Wconversion
    -Wsign-conversion
    -Winit-self
    -Wunreachable-code
  )

endif()

#-------------------------------------------------------------------------------
# Combined C/CXX Flags
#-------------------------------------------------------------------------------

set(NETBOX_C_FLAGS
  ${NETBOX_DEFS}
  ${NETBOX_C_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
  ${NETBOX_C_WARNING_FLAGS}
)

set(NETBOX_CXX_FLAGS
  ${NETBOX_DEFS}
  ${NETBOX_CXX_FLAGS}
  ${NETBOX_PLATFORM_FLAGS}
  ${NETBOX_CXX_WARNING_FLAGS}
)

#-------------------------------------------------------------------------------
# Linker Flags for DLL Builds
#-------------------------------------------------------------------------------

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

  set(NETBOX_DLL_LINK_FLAGS_RELEASE
    ${NETBOX_DLL_LINK_FLAGS}
    /RELEASE
    /INCREMENTAL:NO
    /OPT:REF
    /OPT:ICF
    /LTCG
  )

  set(NETBOX_DLL_LINK_FLAGS_DEBUG
    ${NETBOX_DLL_LINK_FLAGS}
    /DEBUG
  )

elseif(CMAKE_COMPILER_IS_MINGW)

  set(NETBOX_DLL_LINK_FLAGS "")
  set(NETBOX_DLL_LINK_FLAGS_RELEASE ${NETBOX_DLL_LINK_FLAGS})
  set(NETBOX_DLL_LINK_FLAGS_DEBUG ${NETBOX_DLL_LINK_FLAGS})

endif()

#===============================================================================
# MSVC Compiler and Linker Flags
#
# Purpose: Centralized MSVC-specific configuration for NetBox
#
# Provides:
#   - MSVC runtime library configuration (static)
#   - Linker flags for DLL builds
#===============================================================================

if(MSVC)

  #-------------------------------------------------------------------------------
  # Static Runtime Library
  #-------------------------------------------------------------------------------
  # Static runtime for all configurations (avoids DLL dependency issues)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:>$<IF:$<CONFIG:Debug>,,>")

  #-------------------------------------------------------------------------------
  # Compiler Flags
  #-------------------------------------------------------------------------------

  # Multi-processor compilation and debug info
  # ucm_add_flags(CXX C /MP /Zi)

  # Enable C++ exception handling with unwind semantics (required for try/catch)
  ucm_add_flags(CXX /EHsc)

  # Optimization flags for non-Debug builds
  # ucm_add_flags(CXX C /fp:fast /Gw /GL)

  # Remove /Ob1, /Ob2 in non-Debug configs (already optimized via /GL)
  ucm_remove_flags(CXX C /Ob1 /Ob2 CONFIG Release MinSizeRel RelWithDebInfo)

  # Debug flags
  # ucm_add_flags(CXX C /RTC1 /DDEBUG CONFIG Debug)

  #-------------------------------------------------------------------------------
  # Linker Flags
  #-------------------------------------------------------------------------------

  ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_RELEASE} CONFIG Release RelWithDebInfo MinSizeRel)
  ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_DEBUG} CONFIG Debug)
  ucm_add_linker_flags(MODULE SHARED /DEBUG /MAP CONFIG RelWithDebInfo)

endif()

#===============================================================================
# Helper Functions
#===============================================================================

# Apply NetBox compile options to a target
function(netbox_apply_compile_options TARGET)
  target_compile_options(${TARGET} PRIVATE
    ${NETBOX_CXX_FLAGS}
    ${NETBOX_UNICODE_FLAGS}
    ${NETBOX_PLATFORM_FLAGS}
  )
endfunction()

# Apply NetBox linker options to a target
function(netbox_apply_link_options TARGET)
  target_link_options(${TARGET} PRIVATE
    ${NETBOX_DLL_LINK_FLAGS}
  )
endfunction()

# Get NetBox include directories
function(netbox_get_include_dirs RESULT_VAR)
  set(${RESULT_VAR}
    libs
    src/include
    src/base
    src/resource
    PARENT_SCOPE
  )
endfunction()

#===============================================================================
# NASM Assembly Support (MSVC only)
#===============================================================================

if(MSVC)

  # Find NASM executable
  set(NASM_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/buildtools/tools/nasm.exe)

  # Compile ASM files using NASM
  macro(netbox_compile_asm_files RESULT_VAR)
    cmake_parse_arguments(ARG "" "RESULT" "ASM_FILES" ${ARGN})
    if(NOT ARG_ASM_FILES)
      set(ARG_ASM_FILES " ")
    endif()

    if(PROJECT_PLATFORM STREQUAL "x64")
      set(_nasm_args win64)
    else()
      set(_nasm_args win32)
    endif()

    set(${ARG_RESULT} "")
    foreach(_asm_file ${ARG_ASM_FILES})
      set(_asm_source ${CMAKE_CURRENT_SOURCE_DIR}/${_asm_file})
      get_filename_component(_asm_source_fn ${_asm_source} NAME)
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

endif()
