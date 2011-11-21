//---------------------------------------------------------------------------
#include "fzafx.h"
#include <Options.h>
#include <FileZillaIntern.h>
#include <FileZillaIntf.h>
#include <Crypt.h>

#include <Common.h>

//---------------------------------------------------------------------------
CString COptions::GetInstanceOption(CApiLog * Instance, int OptionID)
{
  ASSERT(Instance);
  ASSERT(dynamic_cast<TFileZillaIntern *>(Instance) != NULL);

  TFileZillaIntern * Intern = (TFileZillaIntern *)Instance;

  const TFileZillaIntf * Intf = Intern->GetOwner();
  ASSERT(Intf != NULL);
  const wchar_t *res = Intf->Option(OptionID);
  DEBUG_PRINTF(L"res = %s", res);
  CString Result = res;
  switch (OptionID)
  {
    case OPTION_PROXYPASS:
    case OPTION_FWPASS:
      Result = CCrypt::encrypt(Result);
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
int COptions::GetInstanceOptionVal(CApiLog * Instance, int OptionID)
{
  ASSERT(Instance);
  ASSERT(dynamic_cast<TFileZillaIntern *>(Instance) != NULL);

  TFileZillaIntern * Intern = (TFileZillaIntern *)Instance;

  const TFileZillaIntf * Intf = Intern->GetOwner();
  ASSERT(Intf != NULL);
  return Intf->OptionVal(OptionID);
}
