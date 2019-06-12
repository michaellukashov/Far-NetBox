#include "stdafx.h"
#include "afxdll.h"

HINSTANCE HInst = NULL;

void InitExtensionModule(HINSTANCE HInstance)
{
  ::HInst = HInstance;
  AFX_MANAGE_STATE(AfxGetModuleState());
  afxCurrentResourceHandle = ::HInst;
}

void TermExtensionModule()
{
}
