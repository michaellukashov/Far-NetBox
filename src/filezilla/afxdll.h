#pragma once

#ifdef _AFXDLL

// #include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define SECURITY_WIN32
// #include <windows.h>

void InitExtensionModule(HINSTANCE HInst);
void TermExtensionModule();

#endif
