#include "fzafx.h"
#include "afxdll.h"

HINSTANCE HInst;

// static AFX_EXTENSION_MODULE MyExtDLL = { NULL, NULL } ;

void InitExtensionModule(HINSTANCE HInst)
{
    // AfxInitExtensionModule(MyExtDLL, HInst);
    // AFX_MANAGE_STATE(AfxGetModuleState());
    // afxCurrentResourceHandle = HInst;
    ::HInst = HInst;
    // Insert this DLL into the resource chain
    // new CDynLinkLibrary(MyExtDLL);
    AFX_MANAGE_STATE(AfxGetModuleState());
    afxCurrentResourceHandle = ::HInst;
}

void TermExtensionModule()
{
    // AfxTermExtensionModule(MyExtDLL);
}
