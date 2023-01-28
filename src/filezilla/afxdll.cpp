#include "stdafx.h"
#include "afxdll.h"

HINSTANCE HInst = nullptr;

void InitExtensionModule(HINSTANCE HInstance)
{
  ::HInst = HInstance;
  AFX_MANAGE_STATE(AfxGetModuleState());
  afxCurrentResourceHandle = ::HInst;
}

void TermExtensionModule()
{
}
