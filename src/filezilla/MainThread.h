
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
  bool UsingMlsd() const;
  bool UsingUtf8();
  std::string GetTlsVersionStr() const;
  std::string GetCipherName() const;
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
  BOOL IsBusy() const;
  CFileZillaTools * m_pTools{nullptr};
  BOOL m_bBusy{FALSE};
  unsigned int m_nInternalMessageID{0};
  BOOL IsConnected() const;
  int64_t GetAsyncRequestID() const;
  int64_t GetNextAsyncRequestID();
  virtual int OnThreadMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
  DWORD ResumeThread();
  BOOL PostThreadMessage( UINT message , WPARAM wParam, LPARAM lParam);

protected:
  BOOL InitInstance();
  DWORD ExitInstance();
  DWORD Run();
  static DWORD WINAPI ThreadProc(LPVOID lpParameter);

  mutable CCriticalSectionWrapper m_CriticalSection;

  CFtpControlSocket * m_pControlSocket{nullptr};
  int64_t m_nAsyncRequestID{0};
  void OnTimer(WPARAM wParam, LPARAM lParam);

protected:
  t_directory * m_pWorkingDir{nullptr};
  rde::map<int, int> m_Options;
  BOOL m_bQuit{FALSE};
  t_command * m_pPostKeepAliveCommand{nullptr};
  CServerPath m_CurrentPath;
  UINT_PTR m_nTimerID{0};
  virtual ~CMainThread();
  bool m_Started{false};
};

