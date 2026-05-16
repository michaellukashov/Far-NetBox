#===============================================================================
# NetBox Libraries Configuration Module
#
# Purpose: Centralized library configuration and dependency management
#
# Usage:
#   include(${CMAKE_SOURCE_DIR}/cmake/Libraries.cmake)
#   netbox_configure_libraries()
#===============================================================================

#===============================================================================
# Library Categories
#===============================================================================

set(NETBOX_CORE_LIBRARIES
    openssl-3
    atlmfc
    putty
)

set(NETBOX_PROTOCOL_LIBRARIES
    neon
    libs3
)

set(NETBOX_UTILITY_LIBRARIES
    tinyxml2
    expat
    zlib-ng
    dlmalloc
    tinylog
    fmt
)

set(NETBOX_ALL_LIBRARIES
    ${NETBOX_CORE_LIBRARIES}
    ${NETBOX_PROTOCOL_LIBRARIES}
    ${NETBOX_UTILITY_LIBRARIES}
)

#===============================================================================
# Library Configuration Function
#===============================================================================

function(netbox_configure_libraries)
    foreach(LIB ${NETBOX_ALL_LIBRARIES})
        if(EXISTS ${CMAKE_SOURCE_DIR}/libs/${LIB})
            add_subdirectory(libs/${LIB})
            # message(STATUS "Added library: ${LIB}")
        else()
            message(WARNING "Library directory not found: libs/${LIB}")
        endif()
    endforeach()
endfunction()

#===============================================================================
# Library Information Functions
#===============================================================================

function(netbox_print_library_summary)
    # message(STATUS "")
    # message(STATUS "NetBox Library Configuration:")
    # message(STATUS "  Core libraries: ${NETBOX_CORE_LIBRARIES}")
    # message(STATUS "  Protocol libraries: ${NETBOX_PROTOCOL_LIBRARIES}")
    # message(STATUS "  Utility libraries: ${NETBOX_UTILITY_LIBRARIES}")
    # message(STATUS "")
endfunction()

function(netbox_get_libraries OUTPUT_VAR)
    set(${OUTPUT_VAR} ${NETBOX_ALL_LIBRARIES} PARENT_SCOPE)
endfunction()
