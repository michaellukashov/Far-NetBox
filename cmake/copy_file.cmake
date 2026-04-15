# Check if the file exists before attempting to copy it
# This is to avoid cmake errors when the file does not exist
# (e.g. when building the source package)

# Handle glob patterns (e.g. *.lng, *.hlf)
if("${FULL_FILE_NAME}" MATCHES "[*?]")
  # Convert backslashes to forward slashes for glob
  string(REPLACE "\\" "/" FULL_FILE_NAME "${FULL_FILE_NAME}")
  file(GLOB COPY_FILES "${FULL_FILE_NAME}")
  if(NOT COPY_FILES)
    return()
  endif()
else()
  if(NOT EXISTS "${FULL_FILE_NAME}")
    return()
  endif()
  set(COPY_FILES "${FULL_FILE_NAME}")
endif()

# Create the plugin directory if it does not exist
file(MAKE_DIRECTORY ${NETBOX_PLUGIN_DIR})

# Copy each file
foreach(COPY_SRC ${COPY_FILES})
  get_filename_component(COPY_SRC_NAME ${COPY_SRC} NAME)
  file(COPY_FILE "${COPY_SRC}" "${NETBOX_PLUGIN_DIR}/${COPY_SRC_NAME}" ONLY_IF_DIFFERENT)
endforeach()