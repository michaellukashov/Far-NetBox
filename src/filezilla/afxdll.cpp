#include "stdafx.h"
#include "afxdll.h"

#ifdef _AFXDLL

static AFX_EXTENSION_MODULE MyExtDLL = { NULL, NULL } ;

void InitExtensionModule(HINSTANCE HInst)
{
    AfxInitExtensionModule(MyExtDLL, HInst);
    // Insert this DLL into the resource chain
    new CDynLinkLibrary(MyExtDLL);
}

void TermExtensionModule()
{
    AfxTermExtensionModule(MyExtDLL);
}

#endif
