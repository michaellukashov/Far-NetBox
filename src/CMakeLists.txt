#===============================================================================
# NetBox Main Plugin CMakeLists.tx
#
# Purpose: Build the NetBox plugin DLL for Far Manager
#
# Components:
#   - Base library (src/base/, src/nbcore/)
#   - FileZilla FTP implementation (src/filezilla/)
#   - WinSCP core (src/core/, src/windows/)
#   - NetBox plugin (src/NetBox/)
#   - Resources (src/resource/)
#===============================================================================

#===============================================================================
# CMake Includes
#===============================================================================

include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/NetBox.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Install.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/SourceGroups.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/TargetConfiguration.cmake)

#===============================================================================
# Source Configuration
#===============================================================================

# Configure source groups based on build type
netbox_configure_source_groups()

#===============================================================================
# Resources Targe
#===============================================================================

add_custom_target(NetBoxResources SOURCES
  NetBox/NetBoxEng.lng
  NetBox/NetBoxFr.lng
  NetBox/NetBoxRus.lng
  NetBox/NetBoxPol.lng
  NetBox/NetBoxSpa.lng
  NetBox/resource.h.template
  NetBox/NetBox.rc
  NetBox/NetBox.rc.template
  resource/rtlconsts.rc
  resource/TextsCore1.rc
  resource/TextsFileZilla.rc
  resource/TextsWin1.rc
  ${CMAKE_SOURCE_DIR}/cmake/copy_file.cmake
  ${CMAKE_SOURCE_DIR}/.github/workflows/release.yml
  ${CMAKE_SOURCE_DIR}/appveyor.yml
  ${CMAKE_SOURCE_DIR}/appveyor.bat
  ${CMAKE_SOURCE_DIR}/ChangeLog
  ${CMAKE_SOURCE_DIR}/README.md
)

#===============================================================================
# NetBox Shared Library (DLL)
#===============================================================================

# Create the NetBox targe
add_library(NetBox SHARED ${NETBOX_SOURCES})

# Configure the target with includes, libraries, and compiler options
netbox_configure_target(NetBox)

#===============================================================================
# Plugin Directory Installation (Optional)
#===============================================================================

netbox_setup_plugin_installation(NetBox)

#===============================================================================
# tinylog Unit Tests
#===============================================================================

if(OPT_CREATE_TESTS)
  add_executable(test_tinylog
    ${CMAKE_SOURCE_DIR}/libs/tinylog/tests/test_tinylog.cpp
    ${CMAKE_SOURCE_DIR}/libs/tinylog/tests/unicode_stub.cpp
    ${CMAKE_SOURCE_DIR}/src/nbcore/nbmemory.cpp
    ${CMAKE_SOURCE_DIR}/src/nbcore/nbstring.cpp
    ${CMAKE_SOURCE_DIR}/libs/tinylog/src/TinyLog.cpp
    ${CMAKE_SOURCE_DIR}/libs/tinylog/src/LogStream.cpp
    ${CMAKE_SOURCE_DIR}/libs/tinylog/src/Buffer.cpp
    ${CMAKE_SOURCE_DIR}/libs/tinylog/src/Utils.cpp
    ${CMAKE_SOURCE_DIR}/libs/tinylog/src/LockFreeQueue.cpp
  )
  netbox_configure_target(test_tinylog)
  target_compile_definitions(test_tinylog PRIVATE -DTINYLOG_EXPOR)
  add_test(NAME tinylog COMMAND test_tinylog)
  add_executable(test_sessionhistory
    ${CMAKE_SOURCE_DIR}/tests/nbcore/SessionHistoryTest.cpp
    ${CMAKE_SOURCE_DIR}/src/nbcore/SessionHistory.cpp
    ${CMAKE_SOURCE_DIR}/src/nbcore/nbmemory.cpp
    ${CMAKE_SOURCE_DIR}/src/nbcore/nbstring.cpp
    ${CMAKE_SOURCE_DIR}/src/nbcore/nbutils.cpp
    ${CMAKE_SOURCE_DIR}/src/base/UnicodeString.cpp
    ${CMAKE_SOURCE_DIR}/src/base/Classes.cpp
    ${CMAKE_SOURCE_DIR}/src/base/Sysutils.cpp
    ${CMAKE_SOURCE_DIR}/src/base/StrUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/base/WideStrUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/base/Common.cpp
  )
  netbox_configure_target(test_sessionhistory)
  add_test(NAME sessionhistory COMMAND test_sessionhistory)
endif()