if(EXISTS "${FULL_FILE_NAME}")
  file(MAKE_DIRECTORY ${NETBOX_PLUGIN_DIR})
  file(COPY_FILE "${FULL_FILE_NAME}" "${NETBOX_PLUGIN_DIR}/${FILE_NAME}" ONLY_IF_DIFFERENT)
endif()