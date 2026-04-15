#===============================================================================
# NetBox Installation and Post-Build Configuration
#
# Purpose: Handle plugin directory installation and post-build copy commands
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/cmake/Install.cmake)
#   netbox_setup_plugin_installation(TARGET NetBox)
#===============================================================================

#===============================================================================
# Plugin Installation Function
#===============================================================================

function(netbox_setup_plugin_installation TARGET)
  if(NOT OPT_CREATE_PLUGIN_DIR)
    return()
  endif()

  # Setup plugin directory path
  netbox_configure_plugin_directory()

  # Define files to copy
  netbox_define_distribution_files()
  netbox_define_target_files()

  # Add post-build copy commands
  netbox_add_target_copy_commands(${TARGET})
  netbox_add_distribution_copy_commands(${TARGET})
endfunction()

#===============================================================================
# Helper Functions
#===============================================================================

function(netbox_configure_plugin_directory)
  set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
  # get_filename_component(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../ PATH)
  string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})
  set(NETBOX_PLUGIN_DIR ${PROJECT_ROOT}/Far3_${PROJECT_PLATFORM}/Plugins/NetBox PARENT_SCOPE)
endfunction()

function(netbox_define_distribution_files)
  set(DIST_FILES
    ${CMAKE_SOURCE_DIR}/src/NetBox/*.lng
    ${CMAKE_SOURCE_DIR}/README*.md
    ${CMAKE_SOURCE_DIR}/src/NetBox/*.hlf
    ${CMAKE_SOURCE_DIR}/ChangeLog
    ${CMAKE_SOURCE_DIR}/LICENSE.txt
  PARENT_SCOPE)
endfunction()

function(netbox_define_target_files)

  set(NB_TARGET_FILES
    NetBox.dll
    NetBox.pdb
    NetBox.map
  PARENT_SCOPE)
endfunction()

function(netbox_add_target_copy_commands TARGET)
  netbox_define_target_files()

  foreach(NB_TARGET ${NB_TARGET_FILES})
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      "-DFILE_NAME=${NB_TARGET}"
      "-DFULL_FILE_NAME=$<TARGET_FILE_DIR:${TARGET}>//${NB_TARGET}"
      "-DNETBOX_PLUGIN_DIR=${NETBOX_PLUGIN_DIR}"
      -P "${CMAKE_SOURCE_DIR}/cmake/copy_file.cmake"
      VERBATIM
      COMMENT "Copying ${NB_TARGET} to plugin directory"
    )
  endforeach()
endfunction()

function(netbox_add_distribution_copy_commands TARGET)
  netbox_define_distribution_files()

  foreach(DIST_FILE ${DIST_FILES})
    get_filename_component(FILE_NAME ${DIST_FILE} NAME)
    add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND}
      "-DFILE_NAME=${FILE_NAME}"
      "-DFULL_FILE_NAME=${DIST_FILE}"
      "-DNETBOX_PLUGIN_DIR=${NETBOX_PLUGIN_DIR}"
      -P "${CMAKE_SOURCE_DIR}/cmake/copy_file.cmake"
      COMMENT "Copying ${FILE_NAME} to plugin directory"
    )
  endforeach()
endfunction()
