#pragma once

#include "structures.h"
#include "StdAfx.h"
#include "FileZillaApi.h"
#include "FileZillaIntf.h"
#include "ControlSocket.h"

class CTransferSocket;
class CMainThread;
/////////////////////////////////////////////////////////////////////////////
// Befehlsziel CFtpControlSocket 

class CAsyncProxySocketLayer;
class CMainThread;
class CFtpControlSocket : public CControlSocket
{
  friend CTransferSocket;
public:

public:
  CFtpControlSocket(CMainThread *pMainThread, CFileZillaTools * pTools);
  virtual ~CFtpControlSocket();

public:
  virtual void Connect(t_server &server);
  virtual void OnTimer();
  virtual BOOL IsReady();
  virtual void List(BOOL bFinish, int nError=0, CServerPath path=CServerPath(), CString subdir=_MPT(""), int nListMode = 0);
#ifdef MPEXT
  virtual void ListFile(const CString & filename, const CServerPath & path);
#endif
  virtual void FtpCommand(LPCTSTR pCommand);
  virtual void Disconnect();
  virtual void FileTransfer(t_transferfile *transferfile = 0, BOOL bFinish = FALSE, int nError = 0);
  virtual void Delete(CString filename, const CServerPath &path);
  virtual void Rename(CString oldName, CString newName, const CServerPath &path, const CServerPath &newPath);
  virtual void MakeDir(const CServerPath &path);
  virtual void RemoveDir(CString dirname, const CServerPath &path);
  virtual void Chmod(CString filename, const CServerPath &path, int nValue);
    
  virtual void ProcessReply();
  virtual void TransferEnd(int nMode);
  virtual void Cancel(BOOL bQuit=FALSE);

  virtual void SetAsyncRequestResult(int nAction, CAsyncRequestData *pData);
  
  
  int CheckOverwriteFile();
  virtual BOOL Create();
  void TransfersocketListenFinished(unsigned int ip,unsigned short port);
  
  BOOL m_bKeepAliveActive;
#ifndef MPEXT_NO_SSL
  BOOL m_bDidRejectCertificate;
#endif
  // Some servers are broken. Instead of an empty listing, some MVS servers
  // for example they return something "550 no members found"
  // Other servers return "550 No files found."
  bool IsMisleadingListResponse();

#ifdef MPEXT
  virtual bool UsingMlsd();
  virtual bool UsingUtf8();
  virtual std::string GetTlsVersionStr();
  virtual std::string GetCipherName();
  bool HandleSize(int code, __int64 & size);
  bool HandleMdtm(int code, t_directory::t_direntry::t_date & date);
  void TransferHandleListError();
#endif

  // Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
  //{{AFX_VIRTUAL(CFtpControlSocket)
  public:
  virtual void OnReceive(int nErrorCode);
  virtual void OnConnect(int nErrorCode);
  virtual void OnClose(int nErrorCode);
  virtual void OnSend(int nErrorCode);
  //}}AFX_VIRTUAL

  // Generierte Nachrichtenzuordnungsfunktionen
  //{{AFX_MSG(CFtpControlSocket)
    // HINWEIS - Der Klassen-Assistent fügt hier Member-Funktionen ein und entfernt diese.
  //}}AFX_MSG

// Implementierung
protected:
  //Called by OnTimer()
  void ResumeTransfer();
  void CheckForTimeout();
  void SendKeepAliveCommand();

  virtual int OnLayerCallback(rde::list<t_callbackMsg>& callbacks);
  void SetFileExistsAction(int nAction, COverwriteRequestData *pData);
#ifndef MPEXT_NO_SSL
  void SetVerifyCertResult( int nResult, t_SslCertData *pData );
#endif
  void ResetOperation(int nSuccessful = -1);

  virtual void DoClose(int nError = 0);
  int GetReplyCode();
  CString GetReply();
  void LogOnToServer(BOOL bSkipReply = FALSE);
  BOOL Send(CString str);
  
  BOOL ParsePwdReply(CString& rawpwd);
  BOOL ParsePwdReply(CString& rawpwd, CServerPath & realPath);
  BOOL SendAuthSsl();

  void DiscardLine(CStringA line);
  bool NeedModeCommand();
  bool NeedOptsCommand();
  CString GetListingCmd();

  bool InitConnect();
  int InitConnectState();

#ifdef MPEXT
  bool IsRoutableAddress(const CString & host);
  bool CheckForcePasvIp(CString & host);
  void TransferFinished(bool preserveFileTimeForUploads);
#endif

  CFile *m_pDataFile;
  CTransferSocket *m_pTransferSocket;
  CStringA m_MultiLine;
  CTime m_LastSendTime;
  
  CString m_ServerName;
  rde::list<CStringA> m_RecvBuffer;
  CTime m_LastRecvTime;
  class CLogonData;
  class CListData;
  class CListFileData;
  class CFileTransferData;
  class CMakeDirData;

#ifndef MPEXT_NO_ZLIB
  bool m_useZlib;
  bool m_zlibSupported;
  int m_zlibLevel;
#endif

  bool m_bUTF8;
  bool m_bAnnouncesUTF8;
  int m_nCodePage;
  bool m_hasClntCmd;
#ifdef MPEXT
  TFTPServerCapabilities m_serverCapabilities;
  CStringA m_ListFile;
  __int64 m_ListFileSize;
#endif
  bool m_isFileZilla;

  bool m_awaitsReply;
  bool m_skipReply;

  char* m_sendBuffer;
  int m_sendBufferLen;

  bool m_bProtP;

  bool m_mayBeMvsFilesystem;
  bool m_mayBeBS2000Filesystem;

private:
  BOOL m_bCheckForTimeout;
};

