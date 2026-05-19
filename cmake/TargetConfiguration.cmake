#===============================================================================
# NetBox Target Configuration Module
#
# Purpose: Configure the NetBox target with proper includes, libraries, and flags
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/cmake/TargetConfiguration.cmake)
#   netbox_configure_target(NetBox)
#===============================================================================

#===============================================================================
# Target Configuration Function
#===============================================================================

function(netbox_configure_target TARGET)
  # Set up include directories
  netbox_configure_include_directories(${TARGET})

  # Set up library dependencies
  netbox_configure_library_dependencies(${TARGET})

  # Set up compiler options
  netbox_configure_compiler_options(${TARGET})

  # Set up platform-specific settings
  netbox_configure_platform_settings(${TARGET})
endfunction()

#===============================================================================
# Include Directory Configuration
#===============================================================================

function(netbox_configure_include_directories TARGET)
  target_include_directories(${TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
    ${CMAKE_SOURCE_DIR}/libs/openssl-3/include
    ${CMAKE_SOURCE_DIR}/libs/putty
    ${CMAKE_SOURCE_DIR}/libs/putty/windows
    ${CMAKE_SOURCE_DIR}/libs/neon/src
    ${CMAKE_SOURCE_DIR}/libs/tinyxml2
    ${CMAKE_SOURCE_DIR}/libs/expat/lib
    ${CMAKE_SOURCE_DIR}/libs/atlmfc/include
    ${CMAKE_SOURCE_DIR}/libs/fmt
    ${CMAKE_SOURCE_DIR}/libs/tinylog
    ${CMAKE_SOURCE_DIR}/libs/libs3/inc
    ${CMAKE_SOURCE_DIR}/libs/GSL/include
    include
    filezilla
    base
    core
    windows
    resource
    PluginSDK/Far3
  )
endfunction()

#===============================================================================
# Library Dependencies Configuration
#===============================================================================

function(netbox_configure_library_dependencies TARGET)
  # Core third-party libraries
  set(THIRD_PARTY_LIBS
    putty
    puttyvs
    atlmfc
    zlib
    tinyxml2
    ssleay32
    libeay32
    neon
    expat
    dlmalloc
    fmt
    s3
    tinylog
  )

  # Windows system libraries
  set(WINDOWS_LIBS
    Ws2_32.lib
    kernel32.lib
    user32.lib
    advapi32.lib
    Version.lib
    winhttp.lib
    winspool.lib
    Crypt32.lib
    secur32.lib
    shell32.lib
    shlwapi.lib
    comdlg32.lib
    delayimp.lib
  )

  # MSVC-specific libraries
  if(MSVC)
    set(MSVC_LIBS
      debug libcmtd.lib
      debug libcpmtd.lib
      optimized libcmt.lib
      optimized libcpmt.lib
    )
  endif()

  target_link_libraries(${TARGET} PRIVATE
    ${THIRD_PARTY_LIBS}
    ${WINDOWS_LIBS}
    ${MSVC_LIBS}
  )
endfunction()

#===============================================================================
# Compiler Options Configuration
#===============================================================================

function(netbox_configure_compiler_options TARGET)
  target_compile_options(${TARGET} PRIVATE
    -DBUILD_OFFICIAL
    -wd4251 -wd4275 -wd4661
    ${LIBNEON_DEFS}
    ${LIBEXPAT_DEFS}
    ${ATLMFC_COMPILE_FLAGS}
    ${PUTTY_COMPILE_FLAGS}
    ${NETBOX_DEFS}
    ${NETBOX_UNICODE_FLAGS}
    ${NETBOX_PLATFORM_FLAGS}
    -DGSL_THROW_ON_CONTRACT_VIOLATION
    -DNOMINMAX
    -D_CRTIMP=
  )
endfunction()

#===============================================================================
# Platform-Specific Configuration
#===============================================================================

function(netbox_configure_platform_settings TARGET)
  # Platform-specific settings can be added here
  # For example, different linker flags for different architectures
  if(PROJECT_PLATFORM STREQUAL "x64")
    # x64-specific settings
  elseif(PROJECT_PLATFORM STREQUAL "x86")
    # x86-specific settings
  elseif(PROJECT_PLATFORM STREQUAL "ARM64")
    # ARM64-specific settings
  endif()
endfunction()
