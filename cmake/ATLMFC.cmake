#-------------------------------------------------------------------------------
# ATLMFC Library Configuration
#
# This module provides functions and variables for building minimal MFC subset
# as part of NetBox project.
#
# NOTE: Paths are relative to libs/atlmfc/ directory where this file is included.
#-------------------------------------------------------------------------------

# Set ATLMFC compile flags
set(ATLMFC_COMPILE_FLAGS
    -D_ATL_NO_DEBUG_CRT
    -D_ATL_NO_UUIDOF
    -D_ATL_NO_CONNECTION_POINTS
    -D_ATL_NO_DATETIME_RESOURCES_
    -D_ATL_NO_DEFAULT_LIBS
    -D_ATL_NO_PERF_SUPPORT
    -D_AFX_PORTABLE
    -D_ATL_NO_COMMODULE
    -D_AFX_NO_SOCKET_SUPPORT
    -DNOMINMAX
    -D_MFC_BLD
    -DAFX_DATA=
    -DAFX_DATA_IMPORT=
    -DAFX_CORE_DATA=
    -DAFX_CORE_DATADEF=
    -DAFX_CLASS_IMPORT=
    -DAFX_API_IMPORT=
    -D_ATL_STATIC_REGISTRY
    # -DAFXAPI=  # Removed: causes calling convention mismatch - AFXAPI already defined as __stdcall in afxver_.h
    # -DAFXOLEAPI=  # Removed: causes calling convention mismatch - AFXOLEAPI already defined as __stdcall in afxver_.h
)

if(CMAKE_COMPILER_IS_MINGW)
    set(ATLMFC_COMPILE_FLAGS ${ATLMFC_COMPILE_FLAGS}
        -fpermissive
        -Wno-int-to-pointer-cast
        -Wno-pragmas
        -Wno-non-virtual-dtor
        -Wno-unused-parameter
        -Wno-conversion-null
        -Wno-sign-conversion
        -Wno-deprecated
        -Wno-overloaded-virtual
        -Wno-sign-conversion
        -Wno-conversion
        -Wno-unused-result
    )
endif()

#-------------------------------------------------------------------------------
# Function: atlmfc_get_sources
#
# Returns the list of ATLMFC source files
#
# Output:
#   ATLMFC_SOURCES - List of source files (relative to libs/atlmfc/)
#-------------------------------------------------------------------------------
function(atlmfc_get_sources ATLMFC_SOURCES)
    set(${ATLMFC_SOURCES}
        mfc1.cpp
        mfc2.cpp
        mfc4.cpp
        mfc5.cpp
        src/mfc/thrdcore.cpp
        PARENT_SCOPE
    )
endfunction()

#-------------------------------------------------------------------------------
# Function: atlmfc_apply_compile_options
#
# Applies ATLMFC compile options to a target
#
# Arguments:
#   TARGET - The CMake target to apply options to
#-------------------------------------------------------------------------------
function(atlmfc_apply_compile_options TARGET)
    target_compile_options(${TARGET} PRIVATE
        ${ATLMFC_COMPILE_FLAGS}
        ${NETBOX_CXX_FLAGS}
        ${NETBOX_UNICODE_FLAGS}
    )

    if(MSVC)
        target_compile_options(${TARGET} PRIVATE
            -wd4995
            -wd4996
            -wd4101
            -wd4290
        )
    endif()
endfunction()

#-------------------------------------------------------------------------------
# Function: atlmfc_get_include_dirs
#
# Returns ATLMFC include directories
#
# Output:
#   ATLMFC_INCLUDE_DIRS - List of include directories
#-------------------------------------------------------------------------------
function(atlmfc_get_include_dirs ATLMFC_INCLUDE_DIRS)
    set(${ATLMFC_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/libs
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libs/GSL/include
        ${CMAKE_SOURCE_DIR}/src/include
        ${CMAKE_SOURCE_DIR}/src/base
        PARENT_SCOPE
    )
endfunction()
