
#pragma once

#include <nbsystem.h>

#include "FzApiStructures.h"
#include "structures.h"
#include "AsyncSslSocketLayer.h"
#include "FileZillaIntern.h"
#include "FileZillaOpt.h"
#include "ApiLog.h"

#define DECL_WINDOWS_FUNCTION(linkage, rettype, name, params) \
  typedef rettype (WINAPI * t_##name) params; \
  extern t_##name p_##name
#define STR1(x) #x
#define STR(x) STR1(x)
#define GET_WINDOWS_FUNCTION_PP(module, name) \
  (p_##name = module ? (t_##name) GetProcAddress(module, STR(name)) : nullptr)
#define GET_WINDOWS_FUNCTION(module, name) \
  (p_##name = module ? (t_##name) GetProcAddress(module, #name) : nullptr)

#ifndef NO_IPV6
DECL_WINDOWS_FUNCTION(static, int, getaddrinfo,
  (const char * nodename, const char * servname,
   const struct addrinfo * hints, struct addrinfo ** res));
DECL_WINDOWS_FUNCTION(static, void, freeaddrinfo, (struct addrinfo * res));
DECL_WINDOWS_FUNCTION(static, int, getnameinfo,
  (const struct sockaddr FAR * sa, socklen_t salen,
   char FAR * host, size_t hostlen, char FAR * serv,
   size_t servlen, int flags));
#endif

// This structure holds the commands which will be processed by the api.
// You don't have to fill this struct, you may use the command specific
// functions which is easier.
// See below for a list of supported commands and their parameters.

struct t_command
{
CUSTOM_MEM_ALLOCATION_IMPL
  t_command() : id(0), param4(0) {}
  int id{0}; // Type of command, see below
  CString param1; // Parameters for this command
  CString param2;
  int  param4{0};
  CServerPath path;
  CServerPath newPath; // Used for rename
  t_transferfile transferfile{};
  t_server server;
};

//Description of all api commands

#define FZ_COMMAND_CONNECT 0x0001
// Connects to the server passed in t_command::server
// Possible return values:
// FZ_REPLY_BUSY, FZ_REPLY_ERROR, FZ_REPLY_INVALIDPARAM,
// FZ_REPLY_NOTINITIALIZED, FZ_REPLY_OK, FZ_REPLY_WOULDBLOCK

#define FZ_COMMAND_LIST 0x0002
// Lists the contents of a directory. If no parameter is given, the current
// directory will be listed, else t_command::path specifies the directory
// which contents will be listed. t_command::param1 may specify the name
// of a direct child or parent directory (Use only the last path segment or
// ".."). When the directory listing is successful, it will be sent to the
// owner window (see FZ_DATA_LIST)
// t_command::param4 controls the list mode. (See list modes section)
// Possible return values:
// FZ_REPLY_BUSY, FZ_REPLY_ERROR, FZ_REPLY_INVALIDPARAM,
// FZ_REPLY_NOTCONNECTED, FZ_REPLY_NOTINITIALIZED, FZ_REPLY_OK,
// FZ_REPLY_WOULDBLOCK

#define FZ_COMMAND_FILETRANSFER 0x0004
// Transfers the file specified with t_command::transferfile, see
// t_transferfile for detailed information
// Possible return values:
// FZ_REPLY_BUSY, FZ_REPLY_ERROR, FZ_REPLY_INVALIDPARAM,
// FZ_REPLY_NOTCONNECTED, FZ_REPLY_NOTINITIALIZED, FZ_REPLY_OK,
// FZ_REPLY_WOULDBLOCK

#define FZ_COMMAND_DISCONNECT 0x0008
#define FZ_COMMAND_CUSTOMCOMMAND 0x0010
#define FZ_COMMAND_DELETE    0x0020
#define FZ_COMMAND_REMOVEDIR  0x0040
#define FZ_COMMAND_RENAME    0x0080
#define FZ_COMMAND_MAKEDIR    0x0100
#define FZ_COMMAND_CHMOD    0x0200
#define FZ_COMMAND_LISTFILE    0x0400

#define FZ_MSG_OFFSET 16
#define FZ_MSG_OFFSETMASK 0xFFFF
#define FZ_MSG_ID(x) ((x >> FZ_MSG_OFFSET) & FZ_MSG_OFFSETMASK)
#define FZ_MSG_PARAM(x) ( x & FZ_MSG_OFFSETMASK)
#define FZ_MSG_MAKEMSG(id, param) ((((DWORD)(id & FZ_MSG_OFFSETMASK)) << FZ_MSG_OFFSET) + (param & FZ_MSG_OFFSETMASK) )

#define FZ_MSG_REPLY      0
#define FZ_MSG_LISTDATA      1
#define FZ_MSG_ASYNCREQUEST    5
#define FZ_MSG_STATUS      6
#define FZ_MSG_TRANSFERSTATUS  7
#define FZ_MSG_CAPABILITIES    9

#define FZ_ASYNCREQUEST_OVERWRITE 1
#define FZ_ASYNCREQUEST_VERIFYCERT 2
#ifndef MPEXT_NO_GSS
#define FZ_ASYNCREQUEST_GSS_AUTHFAILED 3
#define FZ_ASYNCREQUEST_GSS_NEEDPASS 4
#endif
#ifndef MPEXT_NO_GSS
#define FZ_ASYNCREQUEST_GSS_NEEDUSER 8
#endif
#define FZ_ASYNCREQUEST_NEEDPASS 10

class CAsyncRequestData
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  CAsyncRequestData();
  virtual ~CAsyncRequestData();
  int nRequestType{0};
  int64_t nRequestID{0}; //Unique for every request sent
  int nRequestResult;
};

class COverwriteRequestData : public CAsyncRequestData
{
public:
  COverwriteRequestData();
  virtual ~COverwriteRequestData();
  CString FileName1;
  CString FileName2;
  CString path1,path2;
  int64_t size1{0};
  int64_t size2{0};
  CTime * localtime{nullptr};
  HANDLE localFileHandle{};
  t_directory::t_direntry::t_date remotetime;
  const t_transferfile * pTransferFile{nullptr};
};

class CVerifyCertRequestData : public CAsyncRequestData
{
public:
  CVerifyCertRequestData();
  virtual ~CVerifyCertRequestData();
  t_SslCertData * pCertData{nullptr};
};

class CNeedPassRequestData : public CAsyncRequestData
{
public:
  CNeedPassRequestData();
  virtual ~CNeedPassRequestData();
  CString Password;
  int nOldOpState{0};
};

#ifndef MPEXT_NO_GSS
class CGssNeedPassRequestData : public CAsyncRequestData
{
public:
  CGssNeedPassRequestData();
  virtual ~CGssNeedPassRequestData();
  CString pass;
  int nOldOpState{0};
};

class CGssNeedUserRequestData : public CAsyncRequestData
{
public:
  CGssNeedUserRequestData();
  virtual ~CGssNeedUserRequestData();
  CString user;
  int nOldOpState{0};
};
#endif

#define FZAPI_OPTION_SHOWHIDDEN 1

#define FTP_CONNECT 0 // SERVER USER PASS PORT
#define FTP_COMMAND 1 // COMMAND - - -
#define FTP_LIST 2 // - - - -
#define FTP_FILETRANSFER 3 // TRANSFERFILE
#define FTP_DISCONNECT 4 // - - - -
#define FTP_RECONNECT 5 // - - - -
#define FTP_DELETE 7 // FILENAME
#define FTP_REMOVEDIR 8 // DIRNAME

#define FZ_REPLY_OK                 0x0001
#define FZ_REPLY_WOULDBLOCK         0x0002
#define FZ_REPLY_ERROR              0x0004
#define FZ_REPLY_OWNERNOTSET        0x0008
#define FZ_REPLY_INVALIDPARAM       0x0010
#define FZ_REPLY_NOTCONNECTED       0x0020
#define FZ_REPLY_ALREADYCONNECTED   0x0040
#define FZ_REPLY_BUSY               0x0080
#define FZ_REPLY_IDLE               0x0100
#define FZ_REPLY_NOTINITIALIZED     0x0200
#define FZ_REPLY_ALREADYINIZIALIZED 0x0400
#define FZ_REPLY_CANCEL             0x0800
#define FZ_REPLY_DISCONNECTED       0x1000 // Always sent when disconnected from server
#define FZ_REPLY_CRITICALERROR      0x2000 // Used for FileTransfers only
#define FZ_REPLY_ABORTED            0x4000 // Used for FileTransfers only
#define FZ_REPLY_NOTSUPPORTED       0x8000 // Command is not supported for the current server

// Additional replies
#define FZ_REPLY_NOTBUSY FZ_REPLY_IDLE

// Servertypes
// General types
#define FZ_SERVERTYPE_HIGHMASK  0xF000
#define FZ_SERVERTYPE_SUBMASK  0x00FF
#define FZ_SERVERTYPE_LAYERMASK 0x0FF0

#define FZ_SERVERTYPE_FTP    0x1000

#define FZ_SERVERTYPE_LAYER_SSL_IMPLICIT 0x0100
#define FZ_SERVERTYPE_LAYER_SSL_EXPLICIT 0x0200
#define FZ_SERVERTYPE_LAYER_TLS_EXPLICIT 0x0400

#define FZ_SERVERTYPE_SUB_FTP_VMS    0x0001
#define FZ_SERVERTYPE_SUB_FTP_SFTP    0x0002
#define FZ_SERVERTYPE_SUB_FTP_WINDOWS  0x0004
#define FZ_SERVERTYPE_SUB_FTP_MVS    0x0010
#define FZ_SERVERTYPE_SUB_FTP_BS2000  0x0020

// Log messages
#define FZ_LOG_STATUS 0
#define FZ_LOG_ERROR 1
#define FZ_LOG_COMMAND 2
#define FZ_LOG_REPLY 3
// By calling CFileZillaApi::SetDebugLevel, the application can enable logging of the following messages:
#define FZ_LOG_APIERROR 5
#define FZ_LOG_WARNING 6
#define FZ_LOG_PROGRESS 7
#define FZ_LOG_INFO 8
#define FZ_LOG_DEBUG 9

class CMainThread;
class CFileZillaTools;

class CFileZillaApi
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  CFileZillaApi();
  virtual ~CFileZillaApi();

  void SetDebugLevel(int32_t nDebugLevel);

  int CustomCommand(CString ACommand);
  int Delete(CString FileName, const CServerPath & path, bool filenameOnly);
  int RemoveDir(CString DirName, const CServerPath & path = CServerPath());
  int Rename(CString oldName, CString newName, const CServerPath & path = CServerPath(), const CServerPath & newPath = CServerPath());
  int MakeDir(const CServerPath & path);

  // General async request reply function
  int SetAsyncRequestResult(int nAction, CAsyncRequestData * pData);

  int Disconnect();
  int Cancel();
  int Chmod(int nValue, CString FileName, const CServerPath & path = CServerPath());

  //Initialization
  int Init(TFileZillaIntern * Intern, CFileZillaTools * pTools);

  // Operations
  int Connect(const t_server & server);

  int List(const CServerPath & path);

  int ListFile(CString FileName, const CServerPath & path); //Get info about specified file

  int FileTransfer(const t_transferfile & TransferFile);
  int GetCurrentServer(t_server & server);

  int SetCurrentPath(CServerPath path);
  int GetCurrentPath(CServerPath & path);
  bool UsingMlsd() const;
  bool UsingUtf8() const;
  std::string GetTlsVersionStr() const;
  std::string GetCipherName() const;

protected:
  CMainThread * m_pMainThread{nullptr};
  uint32_t m_nInternalMessageID{0};
  BOOL m_bInitialized{FALSE};

  void Destroy();
  int IsBusy() const;
  int IsConnected() const;
};

