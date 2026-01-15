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
# Plugin Directory Installation
#===============================================================================

function(netbox_setup_plugin_installation TARGET)
  if(OPT_CREATE_PLUGIN_DIR)

    set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "\\" "/" PROJECT_ROOT ${PROJECT_ROOT})
    set(NETBOX_PLUGIN_DIR ${PROJECT_ROOT}/Far3_${PROJECT_PLATFORM}/Plugins/NetBox)

    file(GLOB DIST_FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/src/NetBox/*.lng
      ${CMAKE_CURRENT_SOURCE_DIR}/src/NetBox/cacert.pem
      ${CMAKE_CURRENT_SOURCE_DIR}/README*.md
      ${CMAKE_CURRENT_SOURCE_DIR}/ChangeLog
      ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt
    )

    set(NB_TARGET_FILES
      NetBox.dll
      NetBox.pdb
      NetBox.map
    )

    foreach(NB_TARGET ${NB_TARGET_FILES})
      add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        "-DFILE_NAME=${NB_TARGET}"
        "-DFULL_FILE_NAME=$<TARGET_FILE_DIR:${TARGET}>//${NB_TARGET}"
        "-DNETBOX_PLUGIN_DIR=${NETBOX_PLUGIN_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file.cmake"
        VERBATIM
      )
    endforeach()

    foreach(DIST_FILE ${DIST_FILES})
      get_filename_component(FILE_NAME ${DIST_FILE} NAME)
      add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        "-DFILE_NAME=${FILE_NAME}"
        "-DFULL_FILE_NAME=${DIST_FILE}"
        "-DNETBOX_PLUGIN_DIR=${NETBOX_PLUGIN_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_file.cmake"
      )
    endforeach()

  endif()
endfunction()

#===============================================================================
# Post-Build Customization Hooks
#===============================================================================

function(netbox_add_post_build_customizations TARGET)
  if(MSVC)
    ucm_set_runtime(STATIC)
    ucm_add_flags(CXX /EHs)
    ucm_remove_flags(CXX C /Ob1 /Ob2 CONFIG Release MinSizeRel RelWithDebInfo)
    ucm_add_flags(CXX C /fp:fast /Gw /GL CONFIG Release RelWithDebInfo MinSizeRel)
    ucm_add_flags(CXX C /RTC1 /DDEBUG CONFIG Debug)
    ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_RELEASE} CONFIG Release RelWithDebInfo MinSizeRel)
    ucm_add_linker_flags(MODULE SHARED ${NETBOX_DLL_LINK_FLAGS_DEBUG} CONFIG Debug)
    ucm_add_linker_flags(MODULE SHARED /DEBUG /MAP CONFIG RelWithDebInfo)
  endif()
endfunction()
