# Check if the file exists before attempting to copy it
# This is to avoid cmake errors when the file does not exist
# (e.g. when building the source package)
if(EXISTS "${FULL_FILE_NAME}")
  # Create the plugin directory if it does not exist
  file(MAKE_DIRECTORY ${NETBOX_PLUGIN_DIR})
  # Copy the file to the plugin directory, only if it is different from the existing file
  file(COPY_FILE "${FULL_FILE_NAME}" "${NETBOX_PLUGIN_DIR}/${FILE_NAME}" ONLY_IF_DIFFERENT)
endif()
