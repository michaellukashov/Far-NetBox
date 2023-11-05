
#pragma once

#include "structures.h"
#include "stdafx.h"
#include "FileZillaApi.h"
#include "FileZillaIntf.h"

class CTransferSocket;
class CMainThread;

class CAsyncProxySocketLayer;
class CFtpListResult;

#define CSMODE_NONE             0x0000
#define CSMODE_CONNECT          0x0001
#define CSMODE_COMMAND          0x0002
#define CSMODE_LIST             0x0004
#define CSMODE_TRANSFER         0x0008
#define CSMODE_DOWNLOAD         0x0010
#define CSMODE_UPLOAD           0x0020
#define CSMODE_TRANSFERERROR    0x0040
#define CSMODE_TRANSFERTIMEOUT  0x0080
#define CSMODE_DELETE           0x0100
#define CSMODE_RMDIR            0x0200
#define CSMODE_DISCONNECT       0x0400
#define CSMODE_MKDIR            0x0800
#define CSMODE_RENAME           0x1000
#define CSMODE_CHMOD            0x2000
#define CSMODE_LISTFILE         0x4000

struct t_transferdata
{
CUSTOM_MEM_ALLOCATION_IMPL
  t_transferdata() :
    transfersize(0), transferleft(0),
    localFileHandle(INVALID_HANDLE_VALUE),
    bResume(FALSE), bResumeAppend(FALSE), bType(FALSE)
  {
  }
  int64_t transfersize, transferleft;
  HANDLE localFileHandle;
  BOOL bResume, bResumeAppend, bType;
};

class CFtpControlSocket : public CAsyncSocketEx, public CApiLog
{
  friend class CTransferSocket;

public:
  CFtpControlSocket(CMainThread * pMainThread, CFileZillaTools * pTools);
  virtual ~CFtpControlSocket();

public:
  void Connect(t_server & server);
  virtual void OnTimer();
  BOOL IsReady();
  void List(BOOL bFinish, int nError = 0, CServerPath path = CServerPath(), CString subdir = L"");
  void ListFile(CString filename, const CServerPath & path);
  void FtpCommand(LPCTSTR pCommand);
  void Disconnect();
  void FileTransfer(t_transferfile * transferfile = 0, BOOL bFinish = FALSE, int nError = 0);
  void Delete(CString filename, const CServerPath & path, bool filenameOnly);
  void Rename(CString oldName, CString newName, const CServerPath & path, const CServerPath & newPath);
  void MakeDir(const CServerPath & path);
  void RemoveDir(CString dirname, const CServerPath & path);
  void Chmod(CString filename, const CServerPath & path, int nValue);

  void ProcessReply();
  void TransferEnd(int nMode);
  void Cancel(BOOL bQuit = FALSE);

  void SetAsyncRequestResult(int nAction, CAsyncRequestData * pData);

  BOOL Create();
  void TransfersocketListenFinished(unsigned int ip, unsigned short port);

  BOOL m_bKeepAliveActive;
  BOOL m_bDidRejectCertificate;

  // Some servers are broken. Instead of an empty listing, some MVS servers
  // for example they return something "550 no members found"
  // Other servers return "550 No files found."
  bool IsMisleadingListResponse();

  bool UsingMlsd() const;
  bool UsingUtf8() const;
  std::string GetTlsVersionStr() const;
  std::string GetCipherName() const;
  bool HandleSize(int code, int64_t & size);
  bool HandleMdtm(int code, t_directory::t_direntry::t_date & date);
  void TransferHandleListError();

  enum transferDirection
  {
    download = 0,
    upload = 1
  };

  BOOL RemoveActiveTransfer();
  BOOL SpeedLimitAddTransferredBytes(enum transferDirection direction, _int64 nBytesTransferred);

  _int64 GetSpeedLimit(enum transferDirection direction, CTime & time);

  _int64 GetAbleToTransferSize(enum transferDirection direction, bool &beenWaiting, int nBufSize = 0);

  t_server GetCurrentServer();
  CFtpListResult * CreateListResult(bool mlst);

public:
  virtual void OnReceive(int nErrorCode);
  virtual void OnConnect(int nErrorCode);
  virtual void OnClose(int nErrorCode);
  virtual void OnSend(int nErrorCode);

protected:
  class CFileTransferData;
  // Called by OnTimer()
  void ResumeTransfer();
  void CheckForTimeout();
  void SendKeepAliveCommand();

  virtual int OnLayerCallback(nb::list_t<t_callbackMsg> & callbacks);
  void SetFileExistsAction(int nAction, COverwriteRequestData * pData);
  void SetVerifyCertResult(int nResult, t_SslCertData * pData);
  void ResetOperation(int nSuccessful = -1);
  void ResetTransferSocket(int Error);
  int OpenTransferFile(CFileTransferData * pData);
  int ActivateTransferSocket(CFileTransferData * pData);
  void CancelTransferResume(CFileTransferData * pData);

  virtual void DoClose(int nError = 0);
  int TryGetReplyCode();
  int GetReplyCode();
  CString GetReply();
  void LogOnToServer(BOOL bSkipReply = FALSE);
  BOOL Send(CString str);

  BOOL ParsePwdReply(CString & rawpwd);
  BOOL ParsePwdReply(CString & rawpwd, CServerPath & realPath);
  BOOL SendAuthSsl();

  void DiscardLine(CStringA line);
  int FileTransferListState(bool get);
  bool NeedModeCommand();
  bool NeedOptsCommand();
  CString GetListingCmd();

  bool InitConnect();
  int InitConnectState();

  bool IsRoutableAddress(const CString & host);
  bool CheckForcePasvIp(CString & host);
  void TransferFinished(bool preserveFileTimeForUploads);

  virtual void LogSocketMessageRaw(int nMessageType, LPCTSTR pMsg);
  virtual bool LoggingSocketMessage(int nMessageType);
  virtual int GetSocketOptionVal(int OptionID) const;

  void ShowStatus(UINT nID, int type) const;
  void ShowStatus(CString status,int type) const;
  void ShowTimeoutError(UINT nID) const;

  void Close();
  BOOL Connect(CString hostAddress, UINT nHostPort);
  CString ConvertDomainName(CString domain);
  bool ConnectTransferSocket(const CString & host, UINT port);

  struct t_ActiveList
  {
  CUSTOM_MEM_ALLOCATION_IMPL
    CFtpControlSocket * pOwner{nullptr};
    int64_t nBytesAvailable{0};
    int64_t nBytesTransferred{0};
  };
  static nb::list_t<t_ActiveList> m_InstanceList[2];
  static CTime m_CurrentTransferTime[2];
  static _int64 m_CurrentTransferLimit[2];
  static CCriticalSectionWrapper m_SpeedLimitSync;
  _int64 GetAbleToUDSize(bool & beenWaiting, CTime & curTime, _int64 & curLimit, nb::list_t<t_ActiveList>::iterator & iter, enum transferDirection direction, int nBufSize);
  _int64 GetSpeedLimit(CTime & time, int valType, int valValue);

  void SetDirectoryListing(t_directory * pDirectory, bool bSetWorkingDir = true);
  int CheckOverwriteFile();
  int CheckOverwriteFileAndCreateTarget();
  int FileTransferHandleDirectoryListing(t_directory * pDirectory);
  t_directory * m_pDirectoryListing{nullptr};

  CMainThread * m_pOwner{nullptr};
  CFileZillaTools * m_pTools{nullptr};

  CFile * m_pDataFile{nullptr};
  CTransferSocket * m_pTransferSocket{nullptr};
  CStringA m_MultiLine;
  CTime m_LastSendTime;

  CString m_ServerName;
  nb::list_t<CStringA> m_RecvBuffer;
  CTime m_LastRecvTime;
  class CLogonData;
  class CListData;
  class CListFileData;
  class CMakeDirData;

#ifndef MPEXT_NO_ZLIB
  bool m_useZlib;
  bool m_zlibSupported;
  int m_zlibLevel;
#endif

  bool m_bUTF8{false};
  int m_nCodePage{0};
  bool m_bAnnouncesUTF8{false};
  bool m_hasClntCmd{false};
  TFTPServerCapabilities m_serverCapabilities;
  CStringA m_ListFile;
  int64_t m_ListFileSize{0};
  bool m_isFileZilla{false};

  bool m_awaitsReply{false};
  bool m_skipReply{false};

  char * m_sendBuffer{nullptr};
  size_t m_sendBufferLen{0};

  bool m_bProtP{false};

  bool m_mayBeMvsFilesystem{false};
  bool m_mayBeBS2000Filesystem{false};

  struct t_operation
  {
  CUSTOM_MEM_ALLOCATION_IMPL
    int nOpMode{0};
    int nOpState{0};
    class COpData //: public TObject //Base class which will store operation specific parameters.
    {
    CUSTOM_MEM_ALLOCATION_IMPL
    public:
      COpData() {}
      virtual ~COpData() {}
    };
    COpData * pData{nullptr};
  public:
  };

  t_operation m_Operation;

  CAsyncProxySocketLayer * m_pProxyLayer{nullptr};
  CAsyncSslSocketLayer * m_pSslLayer{nullptr};
#ifndef MPEXT_NO_GSS
  CAsyncGssSocketLayer * m_pGssLayer;
#endif
  t_server m_CurrentServer;

private:
  BOOL m_bCheckForTimeout{FALSE};
};

