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
  NetBox/NetBoxRus.lng
  NetBox/NetBoxPol.lng
  NetBox/NetBoxSpa.lng
  NetBox/resource.h.template
  NetBox/NetBox.rc
  NetBox/NetBox.rc.template
  ${CMAKE_SOURCE_DIR}/cmake/copy_file.cmake
  ${CMAKE_SOURCE_DIR}/.github/workflows/release.yml
  ${CMAKE_SOURCE_DIR}/appveyor.yml
  ${CMAKE_SOURCE_DIR}/appveyor.bat
  ${CMAKE_SOURCE_DIR}/ChangeLog
  ${CMAKE_SOURCE_DIR}/README.md
  ${CMAKE_SOURCE_DIR}/AGENTS.md
  ${CMAKE_SOURCE_DIR}/PROJECT_RULES.md
  ${CMAKE_SOURCE_DIR}/REFACTORING_PLAN.md
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
