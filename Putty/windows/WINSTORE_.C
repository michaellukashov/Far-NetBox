#include "puttyexp.h"

#pragma warning(disable:4005)
#define RegOpenKey reg_open_winscp_key
#define RegCreateKey reg_create_winscp_key
#define RegCreateKey reg_create_winscp_key
#define RegQueryValueEx reg_query_winscp_value_ex
#define RegSetValueEx reg_set_winscp_value_ex
#define RegCloseKey reg_close_winscp_key
#pragma warning(pop)

#include "winstore.c"

void putty_mungestr(const char *in, char *out)
{
  mungestr(in, out);
}

void putty_unmungestr(const char *in, char *out, int outlen)
{
  unmungestr(in, out, outlen);
}
