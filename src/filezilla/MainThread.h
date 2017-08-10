
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
  static CMainThread * Create(int nPriority, DWORD dwCreateFlags);
  void SetWorkingDir(t_directory * pWorkingDir);
  BOOL GetWorkingDir(t_directory * pWorkingDir);
  void SendDirectoryListing(t_directory * pDirectoryToSend);
  bool UsingMlsd();
  bool UsingUtf8();
  std::string GetTlsVersionStr();
  std::string GetCipherName();
  t_command m_LastCommand;
  void SetCurrentPath(CServerPath path);
  void Quit();
  BOOL GetCurrentServer(t_server & server);
  bool GetCurrentPath(CServerPath & dir);
  CServerPath GetCurrentPath();
  void SetConnected(BOOL bConnected = TRUE);
  BOOL m_bConnected;
  void SetBusy(BOOL bBusy);
  BOOL LastOperationSuccessful();
  void Command(const t_command & command);
  BOOL IsBusy();
  CFileZillaTools * m_pTools;
  BOOL m_bBusy;
  unsigned int m_nInternalMessageID;
  BOOL IsConnected();
  int64_t GetAsyncRequestID() const;
  int64_t GetNextAsyncRequestID();
  virtual int OnThreadMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
  DWORD SuspendThread();
  DWORD ResumeThread();
  BOOL PostThreadMessage( UINT message , WPARAM wParam, LPARAM lParam);

protected:
  BOOL InitInstance();
  DWORD ExitInstance();
  DWORD Run();
  static DWORD WINAPI ThreadProc(LPVOID lpParameter);

  CCriticalSection m_CriticalSection;

  CFtpControlSocket * m_pControlSocket;
  int64_t m_nAsyncRequestID;
  void OnTimer(WPARAM wParam, LPARAM lParam);

protected:
  t_directory * m_pWorkingDir;
  rde::map<int, int> m_Options;
  BOOL m_bQuit;
  t_command * m_pPostKeepAliveCommand;
  CServerPath m_CurrentPath;
  UINT_PTR m_nTimerID;
  virtual ~CMainThread();
  CEvent m_EventStarted;
};


