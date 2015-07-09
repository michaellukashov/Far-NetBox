#include "puttyexp.h"

#pragma warning(push)
#pragma warning(disable: 4005)

#undef RegOpenKey
#undef RegCreateKey
#undef RegCreateKey
#undef RegQueryValueEx
#undef RegSetValueEx
#undef RegCloseKey

#define RegOpenKey reg_open_winscp_key
#define RegCreateKey reg_create_winscp_key
#define RegCreateKey reg_create_winscp_key
#define RegQueryValueEx reg_query_winscp_value_ex
#define RegSetValueEx reg_set_winscp_value_ex
#define RegCloseKey reg_close_winscp_key
#pragma warning(pop)

#include "winstore.c"

