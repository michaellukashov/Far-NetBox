#pragma once

#ifdef NETBOX_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
// #define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
// #define new DEBUG_NEW
#endif
