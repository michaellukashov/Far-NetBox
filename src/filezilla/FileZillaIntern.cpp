//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
#include "FileZillaIntern.h"
#include "FileZillaIntf.h"
//---------------------------------------------------------------------------
TFileZillaIntern::TFileZillaIntern(TFileZillaIntf * AOwner) :
  FOwner(AOwner)
{
  // not being initialized by CApiLog
  m_nLogMessage = 0;
}
//---------------------------------------------------------------------------
BOOL TFileZillaIntern::PostMessage(HWND hWnd, UINT Msg, WPARAM wParam,
  LPARAM lParam) const
{
  DebugAssert(hWnd == NULL);
  DebugAssert(Msg == 0);

  bool Result;
  unsigned int MessageID = FZ_MSG_ID(wParam);

  switch (MessageID)
  {
    case FZ_MSG_STATUS:
    case FZ_MSG_ASYNCREQUEST:
    case FZ_MSG_LISTDATA:
    case FZ_MSG_TRANSFERSTATUS:
    case FZ_MSG_REPLY:
    case FZ_MSG_CAPABILITIES:
      Result = FOwner->PostMessage(wParam, lParam);
      break;

    // ignored for performance
    case FZ_MSG_SOCKETSTATUS:
      Result = false;
      break;

    // ignore
    // not useful. although FTP allows switching between secure and unsecure
    // connection during session, filezilla does not support it,
    // so we are either secure or not for whole session
    case FZ_MSG_SECURESERVER:
      DebugAssert(lParam == 0);
      Result = false;
      break;

    // should never get here, call compiled out in filezilla code
    case FZ_MSG_QUITCOMPLETE:
    default:
      DebugAssert(FALSE);
      Result = false;
      break;
  }

  return (Result ? TRUE : FALSE);
}
//------------------------------------------------------------------------------
NB_IMPLEMENT_CLASS(TFileZillaIntern, NB_GET_CLASS_INFO(CApiLog), nullptr)
//---------------------------------------------------------------------------
