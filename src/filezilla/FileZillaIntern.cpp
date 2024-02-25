
#include "stdafx.h"

#include "FileZillaIntern.h"
#include "FileZillaIntf.h"
#include "FileZillaApi.h"

// #pragma package(smart_init)

TFileZillaIntern::TFileZillaIntern(TFileZillaIntf * AOwner) noexcept :
  // TObject(OBJECT_CLASS_TFileZillaIntern),
  FOwner(AOwner)
{
  FDebugLevel = 0;
}

bool TFileZillaIntern::PostMessage(WPARAM wParam, LPARAM lParam) const
{
  bool Result;
  uint32_t MessageID = FZ_MSG_ID(wParam);

  switch (MessageID)
  {
    case FZ_MSG_STATUS:
    case FZ_MSG_ASYNCREQUEST:
    case FZ_MSG_LISTDATA:
    case FZ_MSG_TRANSFERSTATUS:
    case FZ_MSG_REPLY:
    case FZ_MSG_CAPABILITIES:
      Result = FOwner->FZPostMessage(wParam, lParam);
      break;

    default:
      DebugFail();
      Result = false;
      break;
  }

  return Result;
}

CString TFileZillaIntern::GetOption(int32_t OptionID) const
{
  return FOwner->Option(OptionID);
}

int32_t TFileZillaIntern::GetOptionVal(int OptionID) const
{
  return nb::ToInt32(FOwner->OptionVal(OptionID));
}

int32_t TFileZillaIntern::GetDebugLevel() const
{
  return FDebugLevel;
}

void TFileZillaIntern::SetDebugLevel(int32_t DebugLevel)
{
  FDebugLevel = DebugLevel;
}

