#pragma once

class CApiLog : public TObject
{
NB_DECLARE_CLASS(CApiLog)
public:
  CApiLog();
  virtual ~CApiLog();

  BOOL InitLog(HWND hTargerWnd, int nLogMessage);
  BOOL InitLog(CApiLog *pParent);

  void LogMessage(int nMessageType, LPCTSTR pMsgFormat, ...) const;
  void LogMessageRaw(int nMessageType, LPCTSTR pMsg) const;
  void LogMessage(int nMessageType, UINT nFormatID, ...) const;
  void LogMessage(CString SourceFile, int nSourceLine, void *pInstance, int nMessageType, LPCTSTR pMsgFormat, ...) const;
  void LogMessageRaw(CString SourceFile, int nSourceLine, void *pInstance, int nMessageType, LPCTSTR pMsg) const;

  BOOL SetDebugLevel(int nLogLevel);
  int GetDebugLevel();
protected:
#ifdef MPEXT
  virtual BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) const;
#endif
  void SendLogMessage(int nMessageType, LPCTSTR pMsg) const;
  int m_nDebugLevel;
  CApiLog *m_pApiLogParent; //Pointer to topmost parent
  int m_nLogMessage;
  HWND m_hTargetWnd;
};
