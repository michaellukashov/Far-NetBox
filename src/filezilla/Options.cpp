
#include "stdafx.h"
#include <Options.h>
#include <FileZillaIntern.h>
#include <FileZillaIntf.h>
#include <Crypt.h>

CString COptions::GetInstanceOption(CApiLog * Instance, int OptionID)
{
  DebugAssert(Instance);
  DebugAssert(NB_STATIC_DOWNCAST(TFileZillaIntern, Instance) != NULL);

  TFileZillaIntern * Intern = NB_STATIC_DOWNCAST(TFileZillaIntern, Instance);

  const TFileZillaIntf * Intf = Intern->GetOwner();
  DebugAssert(Intf != NULL);
  CString Result = Intf->Option(OptionID);
  switch (OptionID)
  {
    case OPTION_PROXYPASS:
    case OPTION_FWPASS:
      Result = CCrypt::encrypt(Result);
      break;
  }
  return Result;
}

int COptions::GetInstanceOptionVal(CApiLog * Instance, int OptionID)
{
  DebugAssert(Instance);
  DebugAssert(NB_STATIC_DOWNCAST(TFileZillaIntern, Instance) != NULL);

  TFileZillaIntern * Intern = NB_STATIC_DOWNCAST(TFileZillaIntern, Instance);

  const TFileZillaIntf * Intf = Intern->GetOwner();
  DebugAssert(Intf != NULL);
  return (int)Intf->OptionVal(OptionID);
}
