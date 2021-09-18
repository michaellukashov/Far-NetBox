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
//#define PLUGIN_VERSION_WTXT L"2.4.5"

#define PLUGIN_AUTHOR       L"Michael Lukashov"
#define PLUGIN_DESCR        L"SFTP/FTP(S)/SCP/WebDAV client for FAR 3.0 x86/x64"
#define PLUGIN_FILENAME     L"NetBox.dll"

#define WINSCP_PLUGIN_VERSION_WTXT  L"1.6.2.151"
#define PUTTY_VERSION_WTXT          L"0.70"
#define FILEZILLA_VERSION_WTXT      L"2.2.32"
#define ZLIB_VERSION_WTXT           L"1.2.11"
#define OPENSSL_VERSION_WTXT        L"1.1.0g"
#define WINSCP_VERSION_WTXT         L"5.11.2"
