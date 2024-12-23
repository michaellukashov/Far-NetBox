#pragma once
enum MsgIDs {
    NB_PLUGIN_NAME,
    NB_PLUGIN_TITLE,

    NB_SESSION_NAME_COL_TITLE,
    NB_STORED_SESSION_TITLE,

    NB_STATUS_CLOSED,
    NB_STATUS_INITWINSOCK,
    NB_STATUS_LOOKUPHOST,
    NB_STATUS_CONNECT,
    NB_STATUS_AUTHENTICATE,
    NB_STATUS_AUTHENTICATED,
    NB_STATUS_STARTUP,
    NB_STATUS_OPEN_DIRECTORY,
    NB_STATUS_READY,

    MSG_TITLE_CONFIRMATION,
    MSG_TITLE_INFORMATION,
    MSG_TITLE_ERROR,
    MSG_TITLE_WARNING,

    MSG_BUTTON_Yes,
    MSG_BUTTON_No,
    MSG_BUTTON_OK,
    MSG_BUTTON_Cancel,
    MSG_BUTTON_Abort,
    MSG_BUTTON_Retry,
    MSG_BUTTON_Ignore,
    MSG_BUTTON_All,
    MSG_BUTTON_NoToAll,
    MSG_BUTTON_YesToAll,
    MSG_BUTTON_Help,
    MSG_BUTTON_Skip,
    MSG_BUTTON_Prev,
    MSG_BUTTON_Next,
    MSG_BUTTON_Append,

    MSG_BUTTON_CLOSE,
    MSG_CHECK_NEVER_ASK_AGAIN,
    MSG_CHECK_NEVER_SHOW_AGAIN,
    MSG_BUTTON_TIMEOUT,

    NB_GROUP_COL_TITLE,
    NB_RIGHTS_COL_TITLE,
    NB_RIGHTS_OCTAL_COL_TITLE,
    NB_LINK_TO_COL_TITLE,

    NB_NEW_SESSION_KEYBAR,
    NB_NEW_SESSION_HINT,
    NB_EXPORT_SESSION_KEYBAR,
    NB_COPY_SESSION_KEYBAR,
    NB_RENAME_SESSION_KEYBAR,
    NB_EDIT_HISTORY_KEYBAR,
    NB_OPEN_DIRECTORY_KEYBAR,
    NB_COPY_TO_FILE_KEYBAR,
    NB_MOVE_TO_FILE_KEYBAR,
    NB_RENAME_FILE_KEYBAR,

    NB_WARN_FATAL_ERROR,
    NB_CREATE_LOCAL_DIR_ERROR,
    NB_CREATE_LOCAL_DIRECTORY,
    NB_CANCEL_OPERATION2,
    NB_EDIT_MASK_ERROR,
    NB_VIEW_FROM_FIND_NOT_SUPPORTED,
    NB_PENDING_QUEUE_ITEMS,
    NB_GSSAPI_NOT_INSTALLED,
    NB_WATCH_ERROR_DIRECTORY,
    NB_WATCH_ERROR_GENERAL,
    NB_PERFORM_ON_COMMAND_SESSION,
    NB_NO_FILES_SELECTED,
    NB_CREATE_TEMP_DIR_ERROR,
    NB_SAVE_PASSWORD,
    NB_OLD_FAR,
    NB_SAVE_SYNCHRONIZE_MODE,
    NB_SYNCHRONISE_BEFORE_KEEPUPTODATE,
    NB_SYNCHRONIZE_BROWSING_ON,
    NB_SYNCHRONIZE_BROWSING_OFF,
    NB_SYNCHRONIZE_BROWSING_LOCAL_PATH_REQUIRED,
    NB_SYNC_DIR_BROWSE_ERROR,
    NB_SYNC_DIR_BROWSE_CREATE,
    NB_DELETE_LOCAL_FILE_ERROR,
    NB_TOO_MANY_WATCH_DIRECTORIES,
    NB_DIRECTORY_READING_CANCELLED,
    NB_FTP_PASV_MODE_REQUIRED,
    NB_EDITOR_ALREADY_LOADED,

    NB_COPY_PARAM_GROUP,
    NB_TRANSFER_SETTINGS_BUTTON,

    NB_CREATE_FOLDER_TITLE,
    NB_CREATE_FOLDER_PROMPT,
    NB_CREATE_FOLDER_ATTRIBUTES,
    NB_CREATE_FOLDER_SET_RIGHTS,
    NB_CREATE_FOLDER_REUSE_SETTINGS,

    NB_COPY_TITLE,
    NB_COPY_FILE_PROMPT,
    NB_COPY_FILES_PROMPT,
    NB_MOVE_TITLE,
    NB_MOVE_FILE_PROMPT,
    NB_MOVE_FILES_PROMPT,

    NB_READING_DIRECTORY_TITLE,
    NB_CHANGING_DIRECTORY_TITLE,

    NB_DELETE_FILE_CONFIRM,
    NB_DELETE_FILES_CONFIRM,
    NB_RECYCLE_FILE_CONFIRM,
    NB_RECYCLE_FILES_CONFIRM,

    NB_PROGRESS_COPY,
    NB_PROGRESS_MOVE,
    NB_PROGRESS_DELETE,
    NB_PROGRESS_SETPROPERTIES,
    NB_PROGRESS_CALCULATE_SIZE,
    NB_PROGRESS_REMOTE_MOVE,
    NB_PROGRESS_REMOTE_COPY,
    NB_PROGRESS_GETPROPERTIES,
    NB_PROGRESS_CALCULATE_CHECKSUM,

    NB_PROGRESS_FILE_LABEL,
    NB_TARGET_DIR_LABEL,
    NB_START_TIME_LABEL,
    NB_TIME_ELAPSED_LABEL,
    NB_BYTES_TRANSFERRED_LABEL,
    NB_CPS_LABEL,
    NB_TIME_LEFT_LABEL,

    NB_CANCEL_OPERATION_FATAL2,
    NB_CANCEL_OPERATION,

    NB_NOT_SUPPORTED,
    NB_OPERATION_NOT_SUPPORTED,
    NB_CANNOT_INIT_SESSION,

    NB_SESSION_ALREADY_EXISTS_ERROR,
    NB_NEW_SESSION_NAME_TITLE,
    NB_NEW_SESSION_NAME_PROMPT,
    NB_DELETE_SESSIONS_CONFIRM,
    NB_EXPORT_SESSION_TITLE,
    NB_EXPORT_SESSION_PROMPT,
    NB_EXPORT_SESSIONS_PROMPT,
    NB_IMPORT_SESSIONS_PROMPT,
    NB_IMPORT_SESSIONS_EMPTY,
    NB_DUPLICATE_SESSION_TITLE,
    NB_DUPLICATE_SESSION_PROMPT,
    NB_RENAME_SESSION_TITLE,
    NB_RENAME_SESSION_PROMPT,

    NB_CONFIG_INTERFACE,
    NB_CONFIG_CONFIRMATIONS,
    NB_CONFIG_TRANSFER,
    NB_CONFIG_BACKGROUND,
    NB_CONFIG_ENDURANCE,
    NB_CONFIG_TRANSFER_EDITOR,
    NB_CONFIG_LOGGING,
    NB_CONFIG_ABOUT,
    NB_CONFIG_INTEGRATION,
    NB_CONFIG_PANEL,

    NB_CONFIG_DISKS_MENU,
    NB_CONFIG_PLUGINS_MENU,
    NB_CONFIG_PLUGINS_MENU_COMMANDS,
    NB_CONFIG_SESSION_NAME_IN_TITLE,
    NB_CONFIG_COMMAND_PREFIXES,
    NB_CONFIG_PANEL_MODE_GROUP,
    NB_CONFIG_PANEL_MODE_CHECK,
    NB_CONFIG_PANEL_MODE_TYPES,
    NB_CONFIG_PANEL_MODE_WIDTHS,
    NB_CONFIG_PANEL_MODE_STATUS_TYPES,
    NB_CONFIG_PANEL_MODE_STATUS_WIDTHS,
    NB_CONFIG_PANEL_MODE_FULL_SCREEN,
    NB_CONFIG_PANEL_MODE_HINT,
    NB_CONFIG_PANEL_MODE_HINT2,
    NB_CONFIG_AUTO_READ_DIRECTORY_AFTER_OP,

    NB_ABOUT_VERSION,
    NB_ABOUT_PRODUCT_VERSION,
    NB_ABOUT_HOMEPAGE,
    NB_ABOUT_FORUM,
    NB_ABOUT_URL,

    NB_LOGIN_EDIT,
    NB_LOGIN_ADD,
    NB_LOGIN_CONNECT,
    NB_LOGIN_CONNECT_BUTTON,
    NB_LOGIN_TAB_SESSION,
    NB_LOGIN_TAB_ENVIRONMENT,
    NB_LOGIN_TAB_DIRECTORIES,
    NB_LOGIN_TAB_SCP,
    NB_LOGIN_TAB_SFTP,
    NB_LOGIN_TAB_FTP,
    NB_LOGIN_TAB_FTPS,
    NB_LOGIN_TAB_SSH,
    NB_LOGIN_TAB_S3,
    NB_LOGIN_TAB_CONNECTION,
    NB_LOGIN_TAB_TUNNEL,
    NB_LOGIN_TAB_PROXY,
    NB_LOGIN_TAB_BUGS,
    NB_LOGIN_TAB_AUTH,
    NB_LOGIN_TAB_KEX,
    NB_LOGIN_TAB_WEBDAV,
    NB_LOGIN_GROUP_SESSION,
    NB_LOGIN_HOST_NAME,
    NB_LOGIN_PORT_NUMBER,
    NB_LOGIN_LOGIN_TYPE,
    NB_LOGIN_LOGIN_TYPE_ANONYMOUS,
    NB_LOGIN_LOGIN_TYPE_NORMAL,
    NB_LOGIN_USER_NAME,
    NB_LOGIN_PASSWORD,
    NB_LOGIN_S3_ACCESS_KEY,
    NB_LOGIN_S3_SECRET_ACCESS_KEY,
    NB_LOGIN_PRIVATE_KEY,
    NB_LOGIN_GROUP_PROTOCOL,
    NB_LOGIN_TRANSFER_PROTOCOL,
    NB_LOGIN_SCP,
    NB_LOGIN_SFTP,
    NB_LOGIN_FTP,
    NB_LOGIN_WEBDAV,
    NB_LOGIN_S3,
    NB_LOGIN_ALLOW_SCP_FALLBACK,
    NB_LOGIN_INSECURE,
    NB_LOGIN_TAB_HINT1,
    NB_LOGIN_TAB_HINT2,
    NB_LOGIN_DIRECTORIES_GROUP,
    NB_LOGIN_UPDATE_DIRECTORIES,
    NB_LOGIN_DIRECTORY_OPTIONS_GROUP,
    NB_LOGIN_CACHE_DIRECTORIES,
    NB_LOGIN_CACHE_DIRECTORY_CHANGES,
    NB_LOGIN_PRESERVE_DIRECTORY_CHANGES,
    NB_LOGIN_RESOLVE_SYMLINKS,
    NB_LOGIN_REMOTE_DIRECTORY,
    NB_LOGIN_ENVIRONMENT_GROUP,
    NB_LOGIN_EOL_TYPE,
    NB_LOGIN_DST_MODE_GROUP,
    NB_LOGIN_DST_MODE_UNIX,
    NB_LOGIN_DST_MODE_WIN,
    NB_LOGIN_DST_MODE_KEEP,
    NB_LOGIN_SSH_GROUP,
    NB_LOGIN_COMPRESSION,
    NB_LOGIN_ENCRYPTION_GROUP,
    NB_LOGIN_CIPHER,
    NB_LOGIN_UP,
    NB_LOGIN_DOWN,
    NB_LOGIN_SSH2DES,
    NB_CIPHER_NAME_WARN,
    NB_CIPHER_NAME_3DES,
    NB_CIPHER_NAME_BLOWFISH,
    NB_CIPHER_NAME_AES,
    NB_CIPHER_NAME_DES,
    NB_CIPHER_NAME_ARCFOUR,
    NB_CIPHER_NAME_CHACHA20,
    NB_CIPHER_NAME_AESGCM, 
    NB_LOGIN_SHELL_GROUP,
    NB_LOGIN_SHELL_SHELL,
    NB_LOGIN_SHELL_SHELL_DEFAULT,
    NB_LOGIN_SHELL_RETURN_VAR,
    NB_LOGIN_SHELL_RETURN_VAR_AUTODETECT,
    NB_LOGIN_SCP_LS_OPTIONS_GROUP,
    NB_LOGIN_IGNORE_LS_WARNINGS,
    NB_LOGIN_LISTING_COMMAND,
    NB_LOGIN_SCP_LS_FULL_TIME_AUTO,
    NB_LOGIN_SCP_OPTIONS,
    NB_LOGIN_LOOKUP_USER_GROUPS,
    NB_LOGIN_CLEAR_NATIONAL_VARS,
    NB_LOGIN_CLEAR_ALIASES,
    NB_LOGIN_SCP1_COMPATIBILITY,
    NB_LOGIN_TIME_DIFFERENCE,
    NB_LOGIN_TIME_DIFFERENCE_HOURS,
    NB_LOGIN_TIME_DIFFERENCE_MINUTES,
    NB_LOGIN_TIMEOUTS_GROUP,
    NB_LOGIN_TIMEOUT,
    NB_LOGIN_TIMEOUT_SECONDS,
    NB_LOGIN_PING_GROUP,
    NB_LOGIN_PING_OFF,
    NB_LOGIN_PING_NULL_PACKET,
    NB_LOGIN_PING_DUMMY_COMMAND,
    NB_LOGIN_PING_INTERVAL,
    NB_LOGIN_IP_GROUP,
    NB_LOGIN_IP_AUTO,
    NB_LOGIN_IP_V4,
    NB_LOGIN_IP_V6,
    NB_LOGIN_CODE_PAGE,
    NB_LOGIN_PROXY_GROUP,
    NB_LOGIN_PROXY_METHOD,
    NB_LOGIN_PROXY_NONE,
    NB_LOGIN_PROXY_SOCKS4,
    NB_LOGIN_PROXY_SOCKS5,
    NB_LOGIN_PROXY_HTTP,
    NB_LOGIN_PROXY_TELNET,
    NB_LOGIN_PROXY_LOCAL,
    NB_LOGIN_PROXY_SYSTEM,

    NB_LOGIN_PROXY_FTP_SITE,
    NB_LOGIN_PROXY_FTP_PROXYUSER_USERHOST,
    NB_LOGIN_PROXY_FTP_OPEN_HOST,
    NB_LOGIN_PROXY_FTP_PROXYUSER_USERUSER,
    NB_LOGIN_PROXY_FTP_USER_USERHOST,
    NB_LOGIN_PROXY_FTP_PROXYUSER_HOST,
    NB_LOGIN_PROXY_FTP_USERHOST_PROXYUSER,
    NB_LOGIN_PROXY_FTP_USER_USERPROXYUSERHOST,

    NB_LOGIN_PROXY_HOST,
    NB_LOGIN_PROXY_PORT,
    NB_LOGIN_PROXY_USERNAME,
    NB_LOGIN_PROXY_PASSWORD,
    NB_LOGIN_PROXY_SETTINGS_GROUP,
    NB_LOGIN_PROXY_TELNET_COMMAND,
    NB_LOGIN_PROXY_LOCAL_COMMAND,
    NB_LOGIN_PROXY_LOCALHOST,
    NB_LOGIN_PROXY_DNS,
    NB_LOGIN_PROXY_DNS_NO,
    NB_LOGIN_PROXY_DNS_AUTO,
    NB_LOGIN_PROXY_DNS_YES,
    NB_LOGIN_BUGS_GROUP,
    NB_LOGIN_BUGS_IGNORE1,
    NB_LOGIN_BUGS_PLAIN_PW1,
    NB_LOGIN_BUGS_RSA1,
    NB_LOGIN_BUGS_HMAC2,
    NB_LOGIN_BUGS_DERIVE_KEY2,
    NB_LOGIN_BUGS_RSA_PAD2,
    NB_LOGIN_BUGS_PKSESSID2,
    NB_LOGIN_BUGS_REKEY2,
    NB_LOGIN_BUGS_AUTO,
    NB_LOGIN_BUGS_OFF,
    NB_LOGIN_BUGS_ON,
    NB_LOGIN_AUTH_SSH_NO_USER_AUTH,
    NB_LOGIN_AUTH_GROUP,
    NB_LOGIN_AUTH_TRY_AGENT,
    NB_LOGIN_AUTH_KI,
    NB_LOGIN_AUTH_KI_PASSWORD,
    NB_LOGIN_AUTH_AGENT_FWD,
    NB_LOGIN_AUTH_GSSAPI_PARAMS_GROUP,
    NB_LOGIN_AUTH_ATTEMPT_GSSAPI_AUTHENTICATION,
    NB_LOGIN_AUTH_ALLOW_GSSAPI_CREDENTIAL_DELEGATION,
    NB_LOGIN_AUTH_GSSAPI,
    NB_LOGIN_AUTH_GSSAPI_SERVER_REALM,
    NB_LOGIN_AUTH_PARAMS_GROUP,
    NB_LOGIN_RECYCLE_BIN_GROUP,
    NB_LOGIN_RECYCLE_BIN_DELETE,
    NB_LOGIN_RECYCLE_BIN_OVERWRITE,
    NB_LOGIN_RECYCLE_BIN_LABEL,
    NB_LOGIN_SFTP_PROTOCOL_GROUP,
    NB_LOGIN_SFTP_SERVER,
    NB_LOGIN_SFTP_SERVER_DEFAULT,
    NB_LOGIN_SFTP_MAX_VERSION,
    NB_LOGIN_UTF,
    NB_LOGIN_SFTP_BUGS_GROUP,
    NB_LOGIN_SFTP_BUGS_SYMLINK,
    NB_LOGIN_SFTP_BUGS_SIGNED_TS,
    NB_LOGIN_SFTP_MIN_PACKET_SIZE,
    NB_LOGIN_SFTP_MAX_PACKET_SIZE,
    NB_LOGIN_KEX_OPTIONS_GROUP,
    NB_LOGIN_KEX_LIST,
    NB_LOGIN_KEX_REEXCHANGE_GROUP,
    NB_LOGIN_KEX_REKEY_TIME,
    NB_LOGIN_KEX_REKEY_DATA,
    NB_KEX_NAME_WARN,
    NB_KEX_NAME_DHGROUP1_BITS,
    NB_KEX_NAME_DHGROUP14_BITS,
    NB_KEX_NAME_DHGROUP15_BITS,
    NB_KEX_NAME_DHGROUP16_BITS,
    NB_KEX_NAME_DHGROUP17_BITS,
    NB_KEX_NAME_DHGROUP18_BITS,
    NB_KEX_NAME_DHGEX,
    NB_KEX_NAME_RSA,
    NB_KEX_NAME_ECDH,
    NB_KEX_NAME_NTRU_HYBRID,
    NB_LOGIN_KEY_WITH_CERTIFICATE,
    NB_CERTIFICATE_ADDED,
    NB_SSH_HOST_CA_EDIT,
    NB_SSH_HOST_CA_ADD,
    NB_SSH_HOST_CA_NAME,
    NB_SSH_HOST_CA_PUBLIC_KEY,
    NB_SSH_HOST_CA_PUBLIC_HOSTS,
    NB_SSH_HOST_CA_BROWSE,
    NB_SSH_HOST_CA_NO_KEY,
    NB_SSH_HOST_CA_BROWSE_TITLE,
    NB_SSH_HOST_CA_BROWSE_FILTER,
    NB_SSH_HOST_CA_LOAD_ERROR,
    NB_SSH_HOST_CA_SIGNATURE_TYPES,
    NB_SSH_HOST_CA_SIGNATURES,
    NB_SSH_HOST_CA_NO_HOSTS,
    NB_SSH_HOST_CA_HOSTS_INVALID,
    NB_LOGIN_NOT_SHOWING_AGAIN,

    NB_LOGIN_TUNNEL_GROUP,
    NB_LOGIN_TUNNEL_TUNNEL,
    NB_LOGIN_TUNNEL_SESSION_GROUP,
    NB_LOGIN_TUNNEL_OPTIONS_GROUP,
    NB_LOGIN_TUNNEL_LOCAL_PORT_NUMBER,
    NB_LOGIN_TUNNEL_LOCAL_PORT_NUMBER_AUTOASSIGN,
    NB_LOGIN_ENVIRONMENT_UNIX,
    NB_LOGIN_ENVIRONMENT_WINDOWS,
    NB_LOGIN_CONNECTION_GROUP,
    NB_LOGIN_FTP_PASV_MODE,
    NB_LOGIN_SSH_OPTIMIZE_BUFFER_SIZE,

    NB_LOGIN_FTP_ALLOW_EMPTY_PASSWORD,
    NB_LOGIN_FTP_USE_MLSD,
    NB_LOGIN_FTP_GROUP,
    NB_LOGIN_FTP_DUPFF,
    NB_LOGIN_FTP_UNDUPFF,
    NB_LOGIN_FTP_SSLSESSIONREUSE,
    NB_LOGIN_FTP_ENCRYPTION,
    NB_LOGIN_FTP_USE_PLAIN_FTP,
    NB_LOGIN_FTP_REQUIRE_IMPLICIT_FTP,
    NB_LOGIN_FTP_REQUIRE_EXPLICIT_FTP,
    NB_LOGIN_FTP_POST_LOGIN_COMMANDS,

    NB_LOGIN_WEBDAV_GROUP,

    NB_PROPERTIES_CAPTION,
    NB_PROPERTIES_PROMPT,
    NB_PROPERTIES_PROMPT_FILES,
    NB_PROPERTIES_OWNER_RIGHTS,
    NB_PROPERTIES_GROUP_RIGHTS,
    NB_PROPERTIES_OTHERS_RIGHTS,
    NB_PROPERTIES_READ_RIGHTS,
    NB_PROPERTIES_WRITE_RIGHTS,
    NB_PROPERTIES_EXECUTE_RIGHTS,
    NB_PROPERTIES_RIGHTS,
    NB_PROPERTIES_OCTAL,
    NB_PROPERTIES_DIRECTORIES_X,
    NB_PROPERTIES_OWNER,
    NB_PROPERTIES_GROUP,
    NB_PROPERTIES_RECURSIVE,
    NB_PROPERTIES_NONE_RIGHTS,
    NB_PROPERTIES_DEFAULT_RIGHTS,
    NB_PROPERTIES_ALL_RIGHTS,
    NB_PROPERTIES_SETUID_RIGHTS,
    NB_PROPERTIES_SETGID_RIGHTS,
    NB_PROPERTIES_STICKY_BIT_RIGHTS,
    NB_PROPERTIES_LINKTO,

    NB_TRANSFER_MODE_TEXT,
    NB_TRANSFER_MODE_BINARY,
    NB_TRANSFER_MODE_AUTOMATIC,
    NB_TRANSFER_MODE_MASK,
    NB_TRANSFER_MODE,
    NB_TRANSFER_FILENAME_MODIFICATION,
    NB_TRANSFER_FILENAME_NOCHANGE,
    NB_TRANSFER_FILENAME_UPPERCASE,
    NB_TRANSFER_FILENAME_LOWERCASE,
    NB_TRANSFER_FILENAME_FIRSTUPPERCASE,
    NB_TRANSFER_FILENAME_LOWERCASESHORT,
    NB_TRANSFER_FILENAME_REPLACE_INVALID,
    NB_TRANSFER_PRESERVE_RIGHTS,
    NB_TRANSFER_PRESERVE_TIMESTAMP,
    NB_TRANSFER_PRESERVE_READONLY,
    NB_TRANSFER_REUSE_SETTINGS,
    NB_TRANSFER_QUEUE,
    NB_TRANSFER_QUEUE_NO_CONFIRMATION,
    NB_TRANSFER_NEWER_ONLY,
    NB_TRANSFER_CLEAR_ARCHIVE,
    NB_TRANSFER_OTHER,
    NB_TRANSFER_FILE_MASK,
    NB_TRANSFER_EXCLUDE,
    NB_TRANSFER_INCLUDE,
    NB_TRANSFER_UPLOAD_OPTIONS,
    NB_TRANSFER_DOWNLOAD_OPTIONS,
    NB_TRANSFER_COMMON_OPTIONS,
    NB_TRANSFER_PRESERVE_PERM_ERRORS,
    NB_TRANSFER_CALCULATE_SIZE,
    NB_TRANSFER_SPEED,

    NB_STRING_LINK_EDIT_CAPTION,
    NB_STRING_LINK_ADD_CAPTION,
    NB_STRING_LINK_FILE,
    NB_STRING_LINK_POINT_TO,
    NB_STRING_LINK_SYMLINK,

    NB_REMOTE_MOVE_TITLE,
    NB_REMOTE_MOVE_FILE,
    NB_REMOTE_MOVE_FILES,

    NB_REMOTE_COPY_TITLE,
    NB_REMOTE_COPY_FILE,
    NB_REMOTE_COPY_FILES,

    NB_RENAME_FILE_TITLE,
    NB_RENAME_FILE,

    NB_SERVER_PASSWORD_HIDE_TYPING,
    NB_SERVER_PASSWORD_NOTE1,
    NB_SERVER_PASSWORD_NOTE2,
    NB_PASSWORD_SHOW_PROMPT,
    NB_PASSWORD_SAVE,

    NB_LOGGING_ENABLE,
    NB_LOGGING_OPTIONS_GROUP,
    NB_LOGGING_LOG_PROTOCOL,
    NB_LOGGING_LOG_PROTOCOL_0,
    NB_LOGGING_LOG_PROTOCOL_1,
    NB_LOGGING_LOG_PROTOCOL_2,
    NB_LOGGING_LOG_TO_FILE,
    NB_LOGGING_LOG_FILE_APPEND,
    NB_LOGGING_LOG_FILE_OVERWRITE,
    NB_LOGGING_LOG_VIEW_GROUP,
    NB_LOGGING_LOG_VIEW_COMPLETE,
    NB_LOGGING_LOG_VIEW_LINES,
    NB_LOGGING_LOG_VIEW_LINES2,
    NB_LOGGING_LOG_FILE_HINT1,
    NB_LOGGING_LOG_FILE_HINT2,

    NB_CONFIRMATIONS_CONFIRM_OVERWRITING,
    NB_CONFIRMATIONS_CONTINUE_ON_ERROR,
    NB_CONFIRMATIONS_CONFIRM_RESUME,
    NB_CONFIRMATIONS_OPEN_COMMAND_SESSION,
    NB_CONFIRMATIONS_SYNCHRONIZED_BROWSING,

    NB_TRANSFER_RESUME,
    NB_TRANSFER_RESUME_ON,
    NB_TRANSFER_RESUME_SMART,
    NB_TRANSFER_RESUME_THRESHOLD_UNIT,
    NB_TRANSFER_RESUME_OFF,
    NB_TRANSFER_SESSION_REOPEN_GROUP,
    NB_TRANSFER_SESSION_REOPEN_AUTO_LABEL,
    NB_TRANSFER_SESSION_REOPEN_AUTO_LABEL2,
    NB_TRANSFER_SESSION_REOPEN_NUMBER_OF_RETRIES_LABEL,
    NB_TRANSFER_SESSION_REOPEN_NUMBER_OF_RETRIES_LABEL2,
    NB_TRANSFER_SESSION_TIMEOUTS_GROUP,
    NB_TRANSFER_SESSION_TIMEOUTS_WAIT_TIMEOUT_LABEL,
    NB_TRANSFER_SESSION_TIMEOUTS_WAIT_TIMEOUT_LABEL2,

    NB_TRANSFER_EDITOR_DOWNLOAD,
    NB_TRANSFER_EDITOR_DOWNLOAD_DEFAULT,
    NB_TRANSFER_EDITOR_DOWNLOAD_OPTIONS,

    NB_TRANSFER_EDITOR_UPLOAD,
    NB_TRANSFER_EDITOR_UPLOAD_SAME,
    NB_TRANSFER_EDITOR_UPLOAD_OPTIONS,
    NB_TRANSFER_EDITOR_UPLOAD_ON_SAVE,
    NB_TRANSFER_EDITOR_MULTIPLE,

    NB_TRANSFER_QUEUE_LIMIT,
    NB_TRANSFER_QUEUE_DEFAULT,
    NB_TRANSFER_AUTO_POPUP,
    NB_TRANSFER_QUEUE_BEEP,
    NB_TRANSFER_REMEMBER_PASSWORD,

    NB_MENU_COMMANDS,
    NB_MENU_COMMANDS_LOG,
    NB_MENU_COMMANDS_ATTRIBUTES,
    NB_MENU_COMMANDS_LINK,
    NB_MENU_COMMANDS_CONFIGURE,
    NB_MENU_COMMANDS_INFORMATION,
    NB_MENU_COMMANDS_PUTTY,
    NB_MENU_COMMANDS_PUTTYGEN,
    NB_MENU_COMMANDS_PAGEANT,
    NB_MENU_COMMANDS_OPEN_DIRECTORY,
    NB_MENU_COMMANDS_ADD_BOOKMARK,
    NB_MENU_COMMANDS_HOME_DIRECTORY,
    NB_MENU_COMMANDS_APPLY_COMMAND,
    NB_MENU_COMMANDS_CLEAR_CACHES,
    NB_MENU_COMMANDS_FULL_SYNCHRONIZE,
    NB_MENU_COMMANDS_QUEUE,
    NB_MENU_COMMANDS_SYNCHRONIZE,
    NB_MENU_COMMANDS_SYNCHRONIZE_BROWSING,
    NB_MENU_COMMANDS_EDIT_HISTORY,

    NB_SERVER_PROTOCOL_INFORMATION,
    NB_SERVER_INFORMATION_GROUP,
    NB_SERVER_SSH_IMPLEMENTATION,
    NB_SERVER_CIPHER,
    NB_SERVER_COMPRESSION,
    NB_SERVER_FS_PROTOCOL,
    NB_SERVER_HOST_KEY_MD5,
    NB_SERVER_HOST_KEY_SHA256,
    NB_SERVER_CERT_SHA1,
    NB_SERVER_CERT_SHA256,
    NB_PROTOCOL_INFORMATION_GROUP,
    NB_PROTOCOL_MODE_CHANGING,
    NB_PROTOCOL_OWNER_GROUP_CHANGING,
    NB_PROTOCOL_ANY_COMMAND,
    NB_PROTOCOL_SYMBOLIC_HARD_LINK,
    NB_PROTOCOL_USER_GROUP_LISTING,
    NB_PROTOCOL_REMOTE_COPY,
    NB_PROTOCOL_CHECKING_SPACE_AVAILABLE,
    NB_PROTOCOL_NATIVE_TEXT_MODE,
    NB_PROTOCOL_INFO_GROUP,
    NB_SPACE_AVAILABLE_GROUP,
    NB_SPACE_AVAILABLE_PATH,
    NB_SPACE_AVAILABLE_CHECK_SPACE,
    NB_SPACE_AVAILABLE_BYTES_UNKNOWN,
    NB_SPACE_AVAILABLE_BYTES_ON_DEVICE,
    NB_SPACE_AVAILABLE_UNUSED_BYTES_ON_DEVICE,
    NB_SPACE_AVAILABLE_BYTES_AVAILABLE_TO_USER,
    NB_SPACE_AVAILABLE_UNUSED_BYTES_AVAILABLE_TO_USER,
    NB_SPACE_AVAILABLE_BYTES_PER_ALLOCATION_UNIT,
    NB_SERVER_PROTOCOL_TAB_PROTOCOL,
    NB_SERVER_PROTOCOL_TAB_CAPABILITIES,
    NB_SERVER_PROTOCOL_TAB_SPACE_AVAILABLE,
    NB_SERVER_PROTOCOL_COPY_CLIPBOARD,
    NB_SERVER_REMOTE_SYSTEM,
    NB_SERVER_SESSION_PROTOCOL,
    NB_PROTOCOL_PROTOCOL_ANY_COMMAND,
    NB_PROTOCOL_CALCULATING_CHECKSUM,

    NB_INTEGRATION_PUTTY,
    NB_INTEGRATION_PUTTY_PASSWORD,
    NB_INTEGRATION_PAGEANT,
    NB_INTEGRATION_PUTTYGEN,
    NB_INTEGRATION_TELNET_FOR_FTP_IN_PUTTY,

    NB_OPEN_DIRECTORY_BROWSE_CAPTION,
    NB_OPEN_DIRECTORY_ADD_BOOKMARK_ACTION,
    NB_OPEN_DIRECTORY_REMOVE,
    NB_OPEN_DIRECTORY_UP,
    NB_OPEN_DIRECTORY_DOWN,
    NB_OPEN_DIRECTORY_HELP,

    NB_APPLY_COMMAND_TITLE,
    NB_APPLY_COMMAND_PROMPT,
    NB_APPLY_COMMAND_APPLY_TO_DIRECTORIES,
    NB_APPLY_COMMAND_RECURSIVE,
    NB_APPLY_COMMAND_HINT1,
    NB_APPLY_COMMAND_HINT2,
    NB_APPLY_COMMAND_HINT3,
    NB_APPLY_COMMAND_HINT4,
    NB_APPLY_COMMAND_HINT5,
    NB_APPLY_COMMAND_HINT_LOCAL,
    NB_APPLY_COMMAND_PARAM_TITLE,
    NB_APPLY_COMMAND_PARAM_PROMPT,
    NB_APPLY_COMMAND_REMOTE_COMMAND,
    NB_APPLY_COMMAND_LOCAL_COMMAND,
    NB_APPLY_COMMAND_SHOW_RESULTS,
    NB_APPLY_COMMAND_COPY_RESULTS,
    NB_CUSTOM_COMMAND_SELECTED_UNMATCH,
    NB_CUSTOM_COMMAND_SELECTED_UNMATCH1,
    NB_CUSTOM_COMMAND_PAIRS_DOWNLOAD_FAILED,
    NB_APPLY_COMMAND_LOCAL_PATH_REQUIRED,

    NB_SYNCHRONIZE_LOCAL_PATH_REQUIRED,
    NB_COMPARE_NO_DIFFERENCES,
    NB_FULL_SYNCHRONIZE_TITLE,
    NB_FULL_SYNCHRONIZE_LOCAL_LABEL,
    NB_FULL_SYNCHRONIZE_REMOTE_LABEL,
    NB_FULL_SYNCHRONIZE_DIRECTION_GROUP,
    NB_FULL_SYNCHRONIZE_LOCAL,
    NB_FULL_SYNCHRONIZE_REMOTE,
    NB_FULL_SYNCHRONIZE_BOTH,
    NB_FULL_SYNCHRONIZE_MODE_GROUP,
    NB_FULL_SYNCHRONIZE_GROUP,
    NB_FULL_SYNCHRONIZE_CRITERIONS_GROUP,
    NB_SYNCHRONIZE_SYNCHRONIZE_FILES,
    NB_SYNCHRONIZE_MIRROR_FILES,
    NB_SYNCHRONIZE_SYNCHRONIZE_TIMESTAMPS,
    NB_SYNCHRONIZE_DELETE,
    NB_SYNCHRONIZE_NO_CONFIRMATION,
    NB_SYNCHRONIZE_EXISTING_ONLY,
    NB_SYNCHRONIZE_REUSE_SETTINGS,
    NB_SYNCHRONIZE_PREVIEW_CHANGES,
    NB_SYNCHRONIZE_SYNCHRONIZE,
    NB_SYNCHRONIZE_BY_TIME,
    NB_SYNCHRONIZE_BY_SIZE,
    NB_SYNCHRONIZE_SELECTED_ONLY,
    NB_SYNCHRONIZE_SAME_SIZE,

    NB_SYNCHRONIZE_TITLE,
    NB_SYNCHRONIZE_SYCHRONIZING,
    NB_SYNCHRONIZE_LOCAL_LABEL,
    NB_SYNCHRONIZE_REMOTE_LABEL,
    NB_SYNCHRONIZE_RECURSIVE,
    NB_SYNCHRONIZE_GROUP,
    NB_SYNCHRONIZE_START_BUTTON,
    NB_SYNCHRONIZE_STOP_BUTTON,

    NB_SYNCHRONIZE_PROGRESS_TITLE,
    NB_SYNCHRONIZE_PROGRESS_COMPARE_TITLE,
    NB_SYNCHRONIZE_PROGRESS_LOCAL,
    NB_SYNCHRONIZE_PROGRESS_REMOTE,
    NB_SYNCHRONIZE_PROGRESS_START_TIME,
    NB_SYNCHRONIZE_PROGRESS_ELAPSED,

    NB_COPY_PARAM_CUSTOM_TITLE,

    NB_QUEUE_TITLE,
    NB_QUEUE_HEADER,
    NB_QUEUE_SHOW,
    NB_QUEUE_EXECUTE,
    NB_QUEUE_DELETE,
    NB_QUEUE_MOVE_UP,
    NB_QUEUE_MOVE_DOWN,
    NB_QUEUE_CLOSE,
    NB_QUEUE_PAUSE,
    NB_QUEUE_RESUME,
    NB_QUEUE_COPY,
    NB_QUEUE_MOVE,
    NB_QUEUE_DOWNLOAD,
    NB_QUEUE_UPLOAD,
    NB_QUEUE_CONNECTING,
    NB_QUEUE_QUERY,
    NB_QUEUE_ERROR,
    NB_QUEUE_PROMPT,
    NB_QUEUE_PENDING,
    NB_QUEUE_PENDING_ITEMS,
    NB_QUEUE_CALCULATING_SIZE,
    NB_QUEUE_PAUSED,

    NB_BANNER_TITLE,
    NB_BANNER_NEVER_SHOW_AGAIN,
    NB_BANNER_CONTINUE,

    NB_CHECKLIST_TITLE,
    NB_CHECKLIST_HEADER,
    NB_CHECKLIST_ACTIONS,
    NB_CHECKLIST_CHECKED,
    NB_CHECKLIST_CHECK_ALL,
    NB_CHECKLIST_UNCHECK_ALL,
    NB_CHECKLIST_MAXIMIZE,
    NB_CHECKLIST_RESTORE,

    NB_EDITOR_CURRENT,
    NB_EDITOR_NEW_INSTANCE,
    NB_EDITOR_NEW_INSTANCE_RO,

    NB_CREATING_FOLDER,

    NB_MENU_EDIT_HISTORY,

    NB_IMPORTED_SESSIONS_INFO,

    NB_StringTitle,

    NB_StringOK,
    NB_StringCancel,

    NB_StringSession,
    NB_StringProxy,

    //Edit link dialog
    NB_StringEdCrtTitle,
    NB_StringEdEdtTitle,
    NB_StringEdName,
    NB_StringEdURL,
    NB_StringEdCP,
    NB_StringEdAuth,
    NB_StringEdAuthUser,
    NB_StringEdAuthPsw,
    NB_StringEdAuthPromptPsw,
    NB_StringEdAuthShowPsw,
    NB_StringEdAuthCert,
    NB_StringEdErrURLEmpty,
    NB_StringEdErrURLInvalid,
    NB_StringEdErrNameEmpty,
    NB_StringEdErrNameInvalid,

    //Configure menu
    NB_StringSettingsMenuTitle,
    NB_StringMainSettingsMenuTitle,
    NB_StringProxySettingsMenuTitle,
    NB_StringLoggingSettingsMenuTitle,
    NB_StringAboutMenuTitle,

    //Main configure dialog
    NB_StringCfgAddToDM,
    NB_StringCfgAddToPM,
    NB_StringCfgUseOwnKey,
    NB_StringCfgPrefix,
    NB_StringCfgAltPrefixes,
    NB_StringCfgTimeout,
    NB_StringCfgSessionsPath,

    //Proxy configure dialog
    NB_StringProxySettingsDialogTitle,
    NB_StringProxySettingsProxyType,
    NB_proxyTypeItem1,
    NB_proxyTypeItem2,
    NB_proxyTypeItem3,
    NB_proxyTypeItem4,

    NB_StringProxySettingsProxyHost,
    NB_StringProxySettingsProxyPort,
    NB_StringProxySettingsProxyLogin,
    NB_StringProxySettingsProxyPassword,

    //Logging configure dialog
    NB_StringLoggingDialogTitle,
    NB_StringLoggingDialogEnableLogging,
    NB_StringLoggingOptionsSeparatorTitle,
    NB_StringLoggingOptionsLevel,
    NB_StringLoggingOptionsLevelItem1,
    NB_StringLoggingOptionsLevelItem2,
    NB_StringLoggingDialogLogToFile,
    NB_StringLogFileName,

    //About menu
    NB_StringAboutDialogTitle,
    NB_StringPluginDescriptionText,
    NB_StringPluginVersion,
    NB_StringPluginDescriptionClose,

    //Prompt to crypto key
    NB_StringSessionPwd,

    //Create directory dialog
    NB_StringMKDirTitle,
    NB_StringMKDirName,

    //Delete items dialog
    NB_StringDelTitle,
    NB_StringDelQuestion,
    NB_StringDelQuestSession,
    NB_StringDelQuestFolder,
    NB_StringDelQuestFile,
    NB_StringDelSelected,
    NB_StringDelBtnDelete,

    //Copy dialog
    NB_StringCopyTitle,
    NB_StringCopyPath,
    NB_StringCopySelected,
    NB_StringCopyBtnCopy,

    //Move dialog
    NB_StringMoveTitle,
    NB_StringMovePath,
    NB_StringMoveSelected,
    NB_StringMoveBtnCopy,

    //Progress titles
    NB_StringPrgConnect,
    NB_StringPrgChangeDir,
    NB_StringPrgGetList,
    NB_StringPrgRcvFile,
    NB_StringPrgSendFile,
    NB_StringPrgTo,
    NB_StringPrgDelete,

    //Error messages
    NB_StringErrKeyFile,
    NB_StringErrEstablish,
    NB_StringErrCreateDir,
    NB_StringErrChangeDir,
    NB_StringErrListDir,
    NB_StringErrCopyFile,
    NB_StringErrRenameMove,
    NB_StringErrDeleteFile,
    NB_StringErrDeleteDir,

    NB_StringOperationCanceledByUser,

    NB_StringCreateNewSessionItem,

    NB_StringSSLErrorContinue,

    // TextCore1.rc
    MSG_CORE_ERROR_STRINGS,
    MSG_KEY_NOT_VERIFIED,
    MSG_CONNECTION_FAILED,
    MSG_USER_TERMINATED,
    MSG_LOST_CONNECTION,
    MSG_CANT_DETECT_RETURN_CODE,
    MSG_COMMAND_FAILED,
    MSG_COMMAND_FAILED_CODEONLY,
    MSG_INVALID_OUTPUT_ERROR,
    MSG_READ_CURRENT_DIR_ERROR,
    MSG_SKIP_STARTUP_MESSAGE_ERROR,
    MSG_CHANGE_DIR_ERROR,
    MSG_LIST_DIR_ERROR,
    MSG_LIST_LINE_ERROR,
    MSG_RIGHTS_ERROR,
    MSG_CLEANUP_CONFIG_ERROR,
    MSG_CLEANUP_CACHES_ERROR,
    MSG_CLEANUP_SEEDFILE_ERROR,
    MSG_CLEANUP_SESSIONS_ERROR,
    MSG_DETECT_RETURNVAR_ERROR,
    MSG_LOOKUP_GROUPS_ERROR,
    MSG_FILE_NOT_EXISTS,
    MSG_CANT_GET_ATTRS,
    MSG_OPENFILE_ERROR,
    MSG_READ_ERROR,
    MSG_COPY_FATAL,
    MSG_TOREMOTE_COPY_ERROR,
    MSG_TOLOCAL_COPY_ERROR,
    MSG_SCP_EMPTY_LINE,
    MSG_SCP_ILLEGAL_TIME_FORMAT,
    MSG_SCP_INVALID_CONTROL_RECORD,
    MSG_COPY_ERROR,
    MSG_SCP_ILLEGAL_FILE_DESCRIPTOR,
    MSG_NOT_DIRECTORY_ERROR,
    MSG_CREATE_DIR_ERROR,
    MSG_CREATE_FILE_ERROR,
    MSG_WRITE_ERROR,
    MSG_CANT_SET_ATTRS,
    MSG_REMOTE_ERROR,
    MSG_DELETE_FILE_ERROR,
    MSG_LOG_GEN_ERROR,
    MSG_LOG_OPENERROR,
    MSG_RENAME_FILE_ERROR,
    MSG_RENAME_CREATE_FILE_EXISTS,
    MSG_RENAME_CREATE_DIR_EXISTS,
    MSG_CHANGE_HOMEDIR_ERROR,
    MSG_UNALIAS_ALL_ERROR,
    MSG_UNSET_NATIONAL_ERROR,
    MSG_FIRST_LINE_EXPECTED,
    MSG_CLEANUP_INIFILE_ERROR,
    MSG_AUTHENTICATION_LOG,
    MSG_AUTHENTICATION_FAILED,
    MSG_NOT_CONNECTED,
    MSG_SAVE_KEY_ERROR,
    MSG_SSH_EXITCODE,
    MSG_SFTP_INVALID_TYPE,
    MSG_SFTP_VERSION_NOT_SUPPORTED,
    MSG_SFTP_MESSAGE_NUMBER,
    MSG_SFTP_STATUS_OK,
    MSG_SFTP_STATUS_EOF,
    MSG_SFTP_STATUS_NO_SUCH_FILE,
    MSG_SFTP_STATUS_PERMISSION_DENIED,
    MSG_SFTP_STATUS_FAILURE,
    MSG_SFTP_STATUS_BAD_MESSAGE,
    MSG_SFTP_STATUS_NO_CONNECTION,
    MSG_SFTP_STATUS_CONNECTION_LOST,
    MSG_SFTP_STATUS_OP_UNSUPPORTED,
    MSG_SFTP_ERROR_FORMAT3,
    MSG_SFTP_STATUS_UNKNOWN,
    MSG_READ_SYMLINK_ERROR,
    MSG_EMPTY_DIRECTORY,
    MSG_SFTP_NON_ONE_FXP_NAME_PACKET,
    MSG_SFTP_REALPATH_ERROR,
    MSG_CHANGE_PROPERTIES_ERROR,
    MSG_SFTP_INITIALIZE_ERROR,
    MSG_TIMEZONE_ERROR,
    MSG_SFTP_CREATE_FILE_ERROR,
    MSG_SFTP_OPEN_FILE_ERROR,
    MSG_SFTP_CLOSE_FILE_ERROR,
    MSG_NOT_FILE_ERROR,
    MSG_RENAME_AFTER_RESUME_ERROR,
    MSG_CREATE_LINK_ERROR,
    MSG_INVALID_SHELL_COMMAND,
    MSG_SFTP_SERVER_MESSAGE_UNSUPPORTED,
    MSG_INVALID_OCTAL_PERMISSIONS,
    MSG_SFTP_INVALID_EOL,
    MSG_SFTP_UNKNOWN_FILE_TYPE,
    MSG_SFTP_STATUS_INVALID_HANDLE,
    MSG_SFTP_STATUS_NO_SUCH_PATH,
    MSG_SFTP_STATUS_FILE_ALREADY_EXISTS,
    MSG_SFTP_STATUS_WRITE_PROTECT,
    MSG_SFTP_STATUS_NO_MEDIA,
    MSG_DECODE_UTF_ERROR,
    MSG_CUSTOM_COMMAND_ERROR,
    MSG_LOCALE_LOAD_ERROR,
    MSG_SFTP_INCOMPLETE_BEFORE_EOF,
    MSG_CALCULATE_SIZE_ERROR,
    MSG_SFTP_PACKET_TOO_BIG,
    MSG_SCP_INIT_ERROR,
    MSG_DUPLICATE_BOOKMARK,
    MSG_MOVE_FILE_ERROR,
    MSG_SFTP_PACKET_TOO_BIG_INIT_EXPLAIN,
    MSG_PRESERVE_TIME_PERM_ERROR3,
    MSG_ACCESS_VIOLATION_ERROR3,
    MSG_SFTP_STATUS_NO_SPACE_ON_FILESYSTEM,
    MSG_SFTP_STATUS_QUOTA_EXCEEDED,
    MSG_SFTP_STATUS_UNKNOWN_PRINCIPAL,
    MSG_COPY_FILE_ERROR,
    MSG_CUSTOM_COMMAND_UNTERMINATED,
    MSG_CUSTOM_COMMAND_UNKNOWN,
    MSG_CUSTOM_COMMAND_FILELIST_ERROR,

    MSG_UNKNOWN_SOCKET_STATUS,
    MSG_DELETE_ON_RESUME_ERROR,
    MSG_SFTP_PACKET_ERROR,
    MSG_ITEM_NAME_INVALID,
    MSG_SFTP_STATUS_LOCK_CONFLICT,
    MSG_SFTP_STATUS_DIR_NOT_EMPTY,
    MSG_SFTP_STATUS_NOT_A_DIRECTORY,
    MSG_SFTP_STATUS_INVALID_FILENAME,
    MSG_SFTP_STATUS_LINK_LOOP,
    MSG_SFTP_STATUS_CANNOT_DELETE,
    MSG_SFTP_STATUS_INVALID_PARAMETER,
    MSG_SFTP_STATUS_FILE_IS_A_DIRECTORY,
    MSG_SFTP_STATUS_BYTE_RANGE_LOCK_CONFLICT,
    MSG_SFTP_STATUS_BYTE_RANGE_LOCK_REFUSED,
    MSG_SFTP_STATUS_DELETE_PENDING,
    MSG_SFTP_STATUS_FILE_CORRUPT,
    MSG_KEY_TYPE_UNKNOWN2,
    MSG_KEY_TYPE_UNSUPPORTED2,
    MSG_KEY_TYPE_SSH1,
    MSG_SFTP_OVERWRITE_FILE_ERROR2,
    MSG_SFTP_OVERWRITE_DELETE_BUTTON,
    MSG_SPACE_AVAILABLE_ERROR,
    MSG_TUNNEL_NO_FREE_PORT,
    MSG_EVENT_SELECT_ERROR,
    MSG_UNEXPECTED_CLOSE_ERROR,
    MSG_TUNNEL_ERROR,
    MSG_CHECKSUM_ERROR,
    MSG_INTERNAL_ERROR,
    MSG_NOTSUPPORTED,
    MSG_FTP_ACCESS_DENIED,
    MSG_FTP_CREDENTIAL_PROMPT,
    MSG_FTP_RESPONSE_ERROR,

    MSG_TRANSFER_ERROR,
    MSG_EXECUTE_APP_ERROR,
    MSG_FILE_NOT_FOUND,
    MSG_DOCUMENT_WAIT_ERROR,
    MSG_SPEED_INVALID,
    MSG_CERT_ERR_DEPTH_ZERO_SELF_SIGNED_CERT,
    MSG_CERT_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD,
    MSG_CERT_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD,
    MSG_CERT_ERR_INVALID_CA,
    MSG_CERT_ERR_INVALID_PURPOSE,
    MSG_CERT_ERR_KEYUSAGE_NO_CERTSIGN,
    MSG_CERT_ERR_PATH_LENGTH_EXCEEDED,
    MSG_CERT_ERR_SELF_SIGNED_CERT_IN_CHAIN,
    MSG_CERT_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY,
    MSG_CERT_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE,
    MSG_CERT_ERR_UNABLE_TO_GET_ISSUER_CERT,
    MSG_CERT_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY,
    MSG_CERT_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE,
    MSG_CERT_ERR_UNKNOWN,
    MSG_CERT_ERRDEPTH,
    MSG_MASK_ERROR,
    MSG_FTP_CANNOT_OPEN_ACTIVE_CONNECTION2,
    MSG_CORE_DELETE_LOCAL_FILE_ERROR,
    MSG_URL_OPTION_BOOL_VALUE_ERROR,

    MSG_NET_TRANSL_NO_ROUTE2,
    MSG_NET_TRANSL_CONN_ABORTED,
    MSG_NET_TRANSL_HOST_NOT_EXIST2,
    MSG_NET_TRANSL_PACKET_GARBLED,
    MSG_REPORT_ERROR,
    MSG_TLS_CERT_DECODE_ERROR,
    MSG_FIND_FILE_ERROR,
    MSG_CERT_NAME_MISMATCH,

    MSG_CERT_ERR_BAD_CHAIN,
    MSG_CERT_OK,
    MSG_CERT_ERR_CERT_CHAIN_TOO_LONG,
    MSG_CERT_ERR_CERT_HAS_EXPIRED,
    MSG_CERT_ERR_CERT_NOT_YET_VALID,
    MSG_CERT_ERR_CERT_REJECTED,
    MSG_CERT_ERR_CERT_SIGNATURE_FAILURE,
    MSG_CERT_ERR_CERT_UNTRUSTED,
    MSG_REQUEST_REDIRECTED,
    MSG_TOO_MANY_REDIRECTS,
    MSG_REDIRECT_LOOP,
    MSG_INVALID_URL,
    MSG_PROXY_AUTHENTICATION_FAILED,
    MSG_CONFIGURED_KEY_NOT_MATCH,
    MSG_SFTP_STATUS_OWNER_INVALID,
    MSG_SFTP_STATUS_GROUP_INVALID,
    MSG_SFTP_STATUS_NO_MATCHING_BYTE_RANGE_LOCK,
    MSG_KEY_TYPE_UNOPENABLE,
    MSG_UNKNOWN_CHECKSUM,
    MSG_ALG_NOT_VERIFIED,
    MSG_SFTP_STATUS_4,
    MSG_CERTIFICATE_OPEN_ERROR,
    MSG_CERTIFICATE_READ_ERROR,
    MSG_CERTIFICATE_DECODE_ERROR_INFO,
    MSG_CERTIFICATE_DECODE_ERROR,
    MSG_CERTIFICATE_PUBLIC_KEY_NOT_FOUND,
    MSG_LOCK_FILE_ERROR,
    MSG_UNLOCK_FILE_ERROR,
    MSG_NOT_LOCKED,
    MSG_KEY_SAVE_ERROR,
    MSG_NEON_INIT_FAILED2,
    MSG_SCRIPT_AMBIGUOUS_SLASH_IN_PATH,
    MSG_CERT_IP_CANNOT_VERIFY,
    MSG_HOSTKEY_NOT_CONFIGURED,
    MSG_UNENCRYPTED_REDIRECT,
    MSG_HTTP_ERROR2,
    MSG_FILEZILLA_SITE_MANAGER_NOT_FOUND,
    MSG_FILEZILLA_NO_SITES,
    MSG_FILEZILLA_SITE_NOT_EXIST,
    MSG_SFTP_AS_FTP_ERROR,
    MSG_LOG_FATAL_ERROR,
    MSG_UNREQUESTED_FILE,

    MSG_SIZE_INVALID,
    MSG_KNOWN_HOSTS_NOT_FOUND,
    MSG_KNOWN_HOSTS_NO_SITES,
    MSG_HOSTKEY_NOT_MATCH_CLIPBOARD,
    MSG_S3_ERROR_RESOURCE,
    MSG_S3_ERROR_FURTHER_DETAILS,
    MSG_S3_ERROR_EXTRA_DETAILS,
    MSG_S3_STATUS_ACCESS_DENIED,
    MSG_DUPLICATE_FOLDER_NOT_SUPPORTED,
    MSG_MISSING_TARGET_BUCKET,

    MSG_CORE_CONFIRMATION_STRINGS,
    MSG_CONFIRM_PROLONG_TIMEOUT3,
    MSG_PROMPT_KEY_PASSPHRASE,
    MSG_PROMPT_FILE_OVERWRITE,
    MSG_DIRECTORY_OVERWRITE,
    MSG_CIPHER_TYPE_BOTH2,
    MSG_CIPHER_TYPE_CS2,
    MSG_CIPHER_TYPE_SC2,
    MSG_RESUME_TRANSFER2,
    MSG_PARTIAL_BIGGER_THAN_SOURCE,
    MSG_APPEND_OR_RESUME2,
    MSG_FILE_OVERWRITE_DETAILS,
    MSG_READ_ONLY_OVERWRITE,
    MSG_LOCAL_FILE_OVERWRITE2,
    MSG_REMOTE_FILE_OVERWRITE2,
    MSG_TIMEOUT_STILL_WAITING3,
    MSG_RECONNECT_BUTTON,
    MSG_RENAME_BUTTON,
    MSG_TUNNEL_SESSION_NAME,
    MSG_PASSWORD_TITLE,
    MSG_PASSPHRASE_TITLE,
    MSG_SERVER_PROMPT_TITLE,
    MSG_USERNAME_TITLE,
    MSG_USERNAME_PROMPT2,
    MSG_SERVER_PROMPT_TITLE2,
    MSG_NEW_PASSWORD_TITLE,
    MSG_PROMPT_PROMPT,
    MSG_TIS_INSTRUCTION,
    MSG_CRYPTOCARD_INSTRUCTION,
    MSG_PASSWORD_PROMPT,
    MSG_KEYBINTER_INSTRUCTION,
    MSG_NEW_PASSWORD_CURRENT_PROMPT,
    MSG_NEW_PASSWORD_NEW_PROMPT,
    MSG_NEW_PASSWORD_CONFIRM_PROMPT,
    MSG_TUNNEL_INSTRUCTION,
    MSG_RENAME_TITLE,
    MSG_RENAME_PROMPT2,
    MSG_VERIFY_CERT_PROMPT3,
    MSG_VERIFY_CERT_CONTACT,
    MSG_VERIFY_CERT_CONTACT_LIST,
    MSG_CERT_TEXT2,
    MSG_CERTIFICATE_PASSPHRASE_PROMPT,
    MSG_CERTIFICATE_PASSPHRASE_TITLE,
    MSG_KEY_TYPE_CONVERT4,
    MSG_MULTI_FILES_TO_ONE,
    MSG_KEY_EXCHANGE_ALG,
    MSG_KEYKEY_TYPE,
    MSG_S3_ACCESS_KEY_ID_TITLE,
    MSG_S3_ACCESS_KEY_ID_PROMPT,
    MSG_S3_SECRET_ACCESS_KEY_TITLE,
    MSG_S3_SECRET_ACCESS_KEY_PROMPT,

    MSG_CORE_INFORMATION_STRINGS,
    MSG_YES_STR,
    MSG_NO_STR,
    MSG_SESSION_INFO_TIP2,
    MSG_VERSION2,
    MSG_CLOSED_ON_COMPLETION,
    MSG_SFTP_PROTOCOL_NAME2,
    MSG_FS_RENAME_NOT_SUPPORTED,
    MSG_SFTP_NO_EXTENSION_INFO,
    MSG_SFTP_EXTENSION_INFO,
    MSG_APPEND_BUTTON,
    MSG_YES_TO_NEWER_BUTTON,

    MSG_SKIP_ALL_BUTTON,

    MSG_COPY_PARAM_PRESET_ASCII,
    MSG_COPY_PARAM_PRESET_BINARY,
    MSG_COPY_PARAM_PRESET_EXCLUDE,
    MSG_COPY_INFO_TRANSFER_TYPE2,
    MSG_COPY_INFO_FILENAME,
    MSG_COPY_INFO_PERMISSIONS,
    MSG_COPY_INFO_ADD_X_TO_DIRS,
    MSG_COPY_INFO_TIMESTAMP,
    MSG_COPY_INFO_FILE_MASK,
    MSG_COPY_INFO_CLEAR_ARCHIVE,
    MSG_COPY_INFO_DONT_REPLACE_INV_CHARS,
    MSG_COPY_INFO_DONT_PRESERVE_TIME,
    MSG_COPY_INFO_DONT_CALCULATE_SIZE,
    MSG_COPY_INFO_DEFAULT,
    MSG_COPY_RULE_HOSTNAME,
    MSG_COPY_RULE_USERNAME,
    MSG_COPY_RULE_REMOTE_DIR,
    MSG_COPY_RULE_LOCAL_DIR,
    MSG_SYNCHRONIZE_SCAN,
    MSG_SYNCHRONIZE_START,
    MSG_SYNCHRONIZE_CHANGE,
    MSG_SYNCHRONIZE_UPLOADED,
    MSG_SYNCHRONIZE_DELETED,
    MSG_COPY_INFO_NOT_USABLE,
    MSG_COPY_INFO_IGNORE_PERM_ERRORS,
    MSG_AUTH_TRANSL_USERNAME,
    MSG_AUTH_TRANSL_KEYB_INTER,
    MSG_AUTH_TRANSL_PUBLIC_KEY,
    MSG_AUTH_TRANSL_WRONG_PASSPHRASE,
    MSG_AUTH_TRANSL_ACCESS_DENIED,
    MSG_AUTH_TRANSL_PUBLIC_KEY_AGENT,
    MSG_AUTH_TRANSL_TRY_PUBLIC_KEY,
    MSG_AUTH_PASSWORD,
    MSG_OPEN_TUNNEL,
    MSG_STATUS_CLOSED,
    MSG_STATUS_LOOKUPHOST,
    MSG_STATUS_CONNECT,
    MSG_STATUS_AUTHENTICATE,
    MSG_STATUS_AUTHENTICATED,
    MSG_STATUS_STARTUP,
    MSG_STATUS_READY,
    MSG_STATUS_OPEN_DIRECTORY,
    MSG_USING_TUNNEL,
    MSG_AUTH_TRANSL_KEY_REFUSED,
    MSG_PFWD_TRANSL_ADMIN,
    MSG_PFWD_TRANSL_CONNECT,
    MSG_NET_TRANSL_REFUSED2,
    MSG_NET_TRANSL_RESET,
    MSG_NET_TRANSL_TIMEOUT2,
    MSG_SESSION_INFO_TIP_NO_SSH,
    MSG_RESUME_BUTTON,
    MSG_FTP_NO_FEATURE_INFO,
    MSG_FTP_FEATURE_INFO,
    MSG_COPY_INFO_CPS_LIMIT2,
    MSG_COPY_KEY_BUTTON,
    MSG_UPDATE_KEY_BUTTON,
    MSG_ADD_KEY_BUTTON,
    MSG_COPY_INFO_PRESERVE_READONLY,

    MSG_SPEED_UNLIMITED,
    MSG_FTPS_IMPLICIT,
    MSG_FTPS_EXPLICIT,

    MSG_HOSTKEY,

    MSG_COPY_PARAM_NEWER_ONLY,
    MSG_FTP_SUGGESTION,

    MSG_ANY_HOSTKEY,
    MSG_ANY_CERTIFICATE,

    MSG_COPY_INFO_REMOVE_CTRLZ,
    MSG_COPY_INFO_REMOVE_BOM,

    MSG_VERSION_BUILD,
    MSG_VERSION_DEV_BUILD,
    MSG_VERSION_DEBUG_BUILD,
    MSG_VERSION_DONT_DISTRIBUTE,
    MSG_WEBDAV_EXTENSION_INFO,
    MSG_COPY_PARAM_PRESET_EXCLUDE_ALL_DIR,
    MSG_SCRIPT_CHECKSUM_DESC,
    MSG_CLIENT_CERTIFICATE_LOADING,
    MSG_NEED_CLIENT_CERTIFICATE,
    MSG_LOCKED,
    MSG_EXECUTABLE,
    MSG_SCRIPT_CMDLINE_PARAMETERS,
    MSG_SCRIPTING_USE_HOSTKEY,
    MSG_SCRIPT_SITE_WARNING,
    MSG_CODE_SESSION_OPTIONS,
    MSG_CODE_CONNECT,
    MSG_CODE_PS_ADD_TYPE,
    MSG_COPY_INFO_PRESERVE_TIME_DIRS,
    MSG_TEXT_FILE_ENCODING,
    MSG_AND_STR,
    MSG_AUTH_CHANGING_PASSWORD,
    MSG_PASTE_KEY_BUTTON,
    MSG_SCRIPT_CP_DESC,
    MSG_TIME_UNKNOWN,
    MSG_KEY_DETAILS,
    MSG_COPY_KEY_ACTION,
    MSG_COPY_CERTIFICATE_ACTION,

    MSG_CORE_VARIABLE_STRINGS,
    MSG_PUTTY_BASED_ON,
    MSG_PUTTY_VERSION,
    MSG_PUTTY_COPYRIGHT,
    MSG_PUTTY_URL,
    MSG_FILEZILLA_BASED_ON2,
    MSG_FILEZILLA_VERSION,
    MSG_FILEZILLA_COPYRIGHT2,
    MSG_FILEZILLA_URL,
    MSG_OPENSSL_BASED_ON,
    MSG_OPENSSL_COPYRIGHT2,
    MSG_OPENSSL_VERSION2,
    MSG_OPENSSL_URL,
    MSG_NEON_BASED_ON,
    MSG_NEON_COPYRIGHT,
    MSG_NEON_URL,
    MSG_EXPAT_BASED_ON,
    MSG_EXPAT_URL,
    MSG_PUTTY_LICENSE_URL,
    MSG_MAIN_MSG_TAG,
    MSG_INTERACTIVE_MSG_TAG,

    MSG_WINSCPFAR_NAME,
    MSG_WINSCP_VERSION,
    MSG_WINSCPFAR_VERSION,
    MSG_WINSCPFAR_BASED_ON,
    MSG_WINSCPFAR_BASED_VERSION,
    MSG_WINSCPFAR_BASED_COPYRIGHT,

    // TextsCore2.rc
    MSG_UNKNOWN_KEY4,
    MSG_DIFFERENT_KEY5,
    MSG_OLD_KEY,

    // rtlconsts.rc
    MSG_SDuplicateString,

    MSG_SListCountError,
    MSG_SListIndexError,

    MSG_SMemoryStreamError,

    MSG_SReadError,

    MSG_SSortedListError,

    MSG_STimeEncodeError,

    MSG_SWriteError,

    MSG_SNotImplemented,
    MSG_SOSError,
    MSG_SUnkOSError,

    MSG_SDateEncodeError,
    MSG_SCannotOpenClipboard,

    MSG_IDS_ERRORMSG_TIMEOUT,
    MSG_IDS_STATUSMSG_DISCONNECTED,

    MSG_CONVERTKEY_SAVE_TITLE, // "Save converted private key"
    MSG_CONVERTKEY_SAVE_FILTER, // "PuTTY Private Key Files (*.ppk)|*.ppk|All files (*.*)|*.*"
    MSG_CONVERTKEY_SAVED, // "Private key was converted and saved to '%s'."

    MSG_STACK_TRACE, // "Stack trace:"

    MSG_NO_FILES_SELECTED,

    NB_S3_DEFAULTREGION,
    NB_S3_URLSTYLE,
    NB_S3_REQUESTERPAYS,
    NB_S3_AUTHENTICATION,
    NB_S3_SESSIONTOKEN,
};
