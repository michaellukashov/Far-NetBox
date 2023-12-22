#pragma once

#include "plugin_version.hpp"

#define _MAJOR NETBOX_VERSION_MAJOR
#define _MINOR NETBOX_VERSION_MINOR
#define _PATCH NETBOX_VERSION_PATCH
#define _BUILD NETBOX_VERSION_BUILD

#define PLUGIN_VERSION_NUM   _MAJOR,_MINOR,_PATCH,_BUILD

#define _NTOA(n) #n
#define _NTOA1(v)         _NTOA(v)
#define _NTOA2(v,r)       _NTOA1(v)   "." _NTOA1(r)
#define _NTOA3(v,r,b)     _NTOA2(v,r) "." _NTOA1(b)
#define _NTOA4(v,s,r,b)   _NTOA2(v,s) "." _NTOA2(r,b)

#define PLUGIN_VERSION_STR _NTOA4(_MAJOR,_MINOR,_PATCH,_BUILD)
#define PLUGIN_VERSION_TXT _NTOA3(_MAJOR,_MINOR,_PATCH)

#define PLUGIN_AUTHOR       L"Mikhail Lukashov"
#define PLUGIN_DESCR        L"SFTP/FTP(S)/SCP/WebDAV/S3 client for FAR 3.0 x86/x64"
#define PLUGIN_FILENAME     L"NetBox.dll"

#define WINSCP_PLUGIN_VERSION_WTXT  L"1.6.2.151"
#define PUTTY_VERSION_WTXT          L"0.80"
#define FILEZILLA_VERSION_WTXT      L"2.2.32"
#define ZLIB_VERSION_WTXT           L"zlib-ng 2.1.5"
#define OPENSSL_VERSION_WTXT        L"3.2.0"
#define WINSCP_VERSION_WTXT         L"6.2.1"
