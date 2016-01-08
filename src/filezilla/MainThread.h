#pragma once

#include "FtpControlSocket.h"
#include "structures.h"
#include "FileZillaApi.h"
#include "ApiLog.h"

#define FZAPI_THREADMSG_PROCESSREPLY 0
#define FZAPI_THREADMSG_COMMAND 1
#define FZAPI_THREADMSG_TRANSFEREND 2
#define FZAPI_THREADMSG_CANCEL 3
#define FZAPI_THREADMSG_DISCONNECT 4
#define FZAPI_THREADMSG_ASYNCREQUESTREPLY 5
#define FZAPI_THREADMSG_POSTKEEPALIVE 6

class CMainThread : public CApiLog
{
protected:
  CMainThread();

public:
  DWORD m_dwThreadId;
  HANDLE m_hThread;
  static CMainThread *Create(int nPriority = THREAD_PRIORITY_NORMAL, DWORD dwCreateFlags = 0);
  bool GetWorkingDirPath(CServerPath &path);
  void SetWorkingDir(t_directory *pWorkingDir);
  BOOL GetWorkingDir(t_directory* pWorkingDir);
  void SendDirectoryListing(t_directory * pDirectoryToSend);
#ifndef MPEXT
  void SetOption(int nOption, int nValue);
  int GetOption(int nOption);
#else
  bool UsingMlsd();
  bool UsingUtf8();
  std::string GetTlsVersionStr();
  std::string GetCipherName();
#endif
  t_command m_LastCommand;
#ifndef MPEXT_NO_CACHE
  CDirectoryCache *m_pDirectoryCache;
#endif
  void SetCurrentPath(CServerPath path);
  void Quit();
  BOOL GetCurrentServer(t_server &server);
  bool GetCurrentPath(CServerPath &dir);
  CServerPath GetCurrentPath();
  void SetConnected(BOOL bConnected = TRUE);
  BOOL m_bConnected;
  void SetBusy(BOOL bBusy);
  BOOL LastOperationSuccessful();
  void Command(const t_command& command);
  BOOL IsBusy();
  HWND m_hOwnerWnd;
  CFileZillaTools * m_pTools;
  BOOL m_bBusy;
  unsigned int m_nReplyMessageID;
  unsigned int m_nInternalMessageID;
  BOOL IsConnected();
  __int64 GetAsyncRequestID() const;
  __int64 GetNextAsyncRequestID();
  virtual int OnThreadMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
  DWORD SuspendThread();
  DWORD ResumeThread();
  BOOL PostThreadMessage( UINT message , WPARAM wParam, LPARAM lParam);

  BOOL IsValid() const;

protected:
  BOOL InitInstance();
  DWORD ExitInstance();
  DWORD Run();
  static DWORD WINAPI ThreadProc(LPVOID lpParameter);

  CCriticalSection m_CriticalSection;
  void ShowStatus(CString status,int type);
  void ShowStatus(UINT nID, int type);

  CControlSocket *m_pControlSocket;
  CFtpControlSocket *m_pFtpControlSocket;
#ifndef MPEXT_NO_SFTP
  CSFtpControlSocket *m_pSFtpControlSocket;
#endif
  __int64 m_nAsyncRequestID;
  void OnTimer(WPARAM wParam,LPARAM lParam);

protected:
#ifndef MPEXT_NO_IDENT
  CIdentServerControl *m_pIdentServer;
#endif
  t_directory *m_pWorkingDir;
  rde::map<int, int> m_Options;
  BOOL m_bQuit;
  t_command *m_pPostKeepAliveCommand;
  CServerPath m_CurrentPath;
  UINT m_nTimerID;
  virtual ~CMainThread();
  CEvent m_EventStarted;
};


