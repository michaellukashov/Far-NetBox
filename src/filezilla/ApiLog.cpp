#include "stdafx.h"
#include "ApiLog.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CApiLog::CApiLog()
{
  m_hTargetWnd=0;
  m_pApiLogParent=0;
  m_nDebugLevel=0;
  m_nLogMessage=0;
}

CApiLog::~CApiLog()
{

}

BOOL CApiLog::InitLog(CApiLog *pParent)
{
  if (!pParent)
    return FALSE;
  while (pParent->m_pApiLogParent)
    pParent=pParent->m_pApiLogParent;
  m_hTargetWnd=0;
  m_pApiLogParent=pParent;
  return TRUE;
}

BOOL CApiLog::InitLog(HWND hTargetWnd, int nLogMessage)
{
  if (!hTargetWnd)
    return FALSE;
  m_hTargetWnd=hTargetWnd;
  m_nLogMessage=nLogMessage;
  m_pApiLogParent=0;
  return TRUE;
}

void CApiLog::LogMessage(int nMessageType, LPCTSTR pMsgFormat, ...) const
{
  DebugAssert(nMessageType>=0 && nMessageType<=8);
  DebugAssert(m_hTargetWnd || m_pApiLogParent);
  if (nMessageType>=FZ_LOG_APIERROR && (nMessageType-FZ_LOG_APIERROR)>=m_pApiLogParent->m_nDebugLevel)
    return;

  va_list ap;
    
    va_start(ap, pMsgFormat);
    CString text;
  text.FormatV(pMsgFormat, ap);
  va_end(ap);
  
  if (nMessageType>=FZ_LOG_DEBUG)
    return;
  SendLogMessage(nMessageType, text);
}

void CApiLog::LogMessageRaw(int nMessageType, LPCTSTR pMsg) const
{
  DebugAssert(nMessageType>=0 && nMessageType<=8);
  DebugAssert(m_hTargetWnd || m_pApiLogParent);
  if (nMessageType>=FZ_LOG_APIERROR && (nMessageType-FZ_LOG_APIERROR)>=m_pApiLogParent->m_nDebugLevel)
    return;

  if (nMessageType>=FZ_LOG_DEBUG)
    return;
  SendLogMessage(nMessageType, pMsg);
}

void CApiLog::LogMessage(int nMessageType, UINT nFormatID, ...) const
{
  DebugAssert(nMessageType>=0 && nMessageType<=8);
  DebugAssert(m_hTargetWnd || m_pApiLogParent);
  if (nMessageType>=FZ_LOG_APIERROR && (nMessageType-FZ_LOG_APIERROR)>=m_pApiLogParent->m_nDebugLevel)
    return;

  CString str;
  str.LoadString(nFormatID);

  va_list ap;
    
    va_start(ap, nFormatID);
    CString text;
  text.FormatV(str, ap);
  va_end(ap);
  
  if (nMessageType>=FZ_LOG_DEBUG)
    return;
  SendLogMessage(nMessageType, text);
}

void CApiLog::LogMessage(CString SourceFile, int nSourceLine, void *pInstance, int nMessageType, LPCTSTR pMsgFormat, ...) const
{
  DebugAssert(nMessageType>=4 && nMessageType<=8);
  DebugAssert(m_hTargetWnd || m_pApiLogParent);
  DebugAssert(nSourceLine>0);


  int pos=SourceFile.ReverseFind(_MPT('\\'));
  if (pos!=-1)
    SourceFile=SourceFile.Mid(pos+1);

  va_list ap;
    
  va_start(ap, pMsgFormat);
  CString text;
  text.FormatV(pMsgFormat, ap);
  va_end(ap);

  if (nMessageType>=FZ_LOG_DEBUG)
    return;

  CString msg;
  msg.Format(_T("%s(%d): %s   caller=0x%08x"), (LPCTSTR)SourceFile, nSourceLine, (LPCTSTR)text, (int)this);
  
  SendLogMessage(nMessageType, msg);
}

BOOL CApiLog::PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) const
{
  return m_pApiLogParent->PostMessage(hWnd, Msg, wParam, lParam);
}

void CApiLog::LogMessageRaw(CString SourceFile, int nSourceLine, void *pInstance, int nMessageType, LPCTSTR pMsg) const
{
  DebugAssert(nMessageType>=4 && nMessageType<=8);
  DebugAssert(m_hTargetWnd || m_pApiLogParent);
  DebugAssert(nSourceLine>0);

  int pos=SourceFile.ReverseFind(_MPT('\\'));
  if (pos!=-1)
    SourceFile=SourceFile.Mid(pos+1);
  
  if (nMessageType>=FZ_LOG_DEBUG)
    return;

  CString msg;
  msg.Format(_T("%s(%d): %s   caller=0x%08x"), (LPCTSTR)SourceFile, nSourceLine, pMsg, (int)this);
  
  SendLogMessage(nMessageType, msg);
}

void CApiLog::SendLogMessage(int nMessageType, LPCTSTR pMsg) const
{
  DebugAssert(m_pApiLogParent);
  DebugAssert(m_pApiLogParent->m_hTargetWnd == 0);
  DebugAssert(m_pApiLogParent->m_nLogMessage == 0);
  if (nMessageType>=FZ_LOG_APIERROR && (nMessageType-FZ_LOG_APIERROR)>=m_pApiLogParent->m_nDebugLevel)
    return;
  //Displays a message in the message log  
  t_ffam_statusmessage *pStatus = new t_ffam_statusmessage;
  pStatus->post = TRUE;
  pStatus->status = pMsg;
  pStatus->type = nMessageType;
  if (!this->PostMessage(m_pApiLogParent->m_hTargetWnd, m_pApiLogParent->m_nLogMessage, FZ_MSG_MAKEMSG(FZ_MSG_STATUS, 0), (LPARAM)pStatus))
    delete pStatus;
}

BOOL CApiLog::SetDebugLevel(int nDebugLevel)
{
  if (m_pApiLogParent)
    return FALSE;
  if (nDebugLevel<0 || nDebugLevel>4)
    return FALSE;
  m_nDebugLevel=nDebugLevel;
  return TRUE;
}

int CApiLog::GetDebugLevel()
{
  if (m_pApiLogParent)
    return m_pApiLogParent->m_nDebugLevel;
  return m_nDebugLevel;
}

NB_IMPLEMENT_CLASS(CApiLog, NB_GET_CLASS_INFO(TObject), nullptr)

