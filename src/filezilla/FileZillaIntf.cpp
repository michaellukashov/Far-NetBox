
#include "stdafx.h"

#include "FileZillaIntf.h"
#include "FileZillaIntern.h"

#include "FzApiStructures.h"
#include "FileZillaApi.h"
#include "structures.h"

static HMODULE winsock_module = nullptr;
#ifndef NO_IPV6
static HMODULE winsock2_module = nullptr;
static HMODULE wship6_module = nullptr;
#endif // NO_IPV6

__removed #pragma package(smart_init)

void TFileZillaIntf::Initialize()
{
#ifndef NO_IPV6
  winsock2_module =
#endif
    winsock_module = ::LoadLibrary(L"ws2_32.dll");
  if (!winsock_module)
  {
    winsock_module = ::LoadLibrary(L"wsock32.dll");
  }
  if (!winsock_module)
  {
//    fatalbox("Unable to load any WinSock library");
  }
#ifndef NO_IPV6
  else
  {
    /* Check if we have getaddrinfo in Winsock */
    if (::GetProcAddress(winsock_module, "getaddrinfo") != nullptr)
    {
      GET_WINDOWS_FUNCTION(winsock_module, getaddrinfo);
      GET_WINDOWS_FUNCTION(winsock_module, freeaddrinfo);
      GET_WINDOWS_FUNCTION(winsock_module, getnameinfo);
    }
    else
    {
    /* Fall back to wship6.dll for Windows 2000 */
      wship6_module = ::LoadLibrary(L"wship6.dll");
      if (wship6_module)
      {
        GET_WINDOWS_FUNCTION(wship6_module, getaddrinfo);
        GET_WINDOWS_FUNCTION(wship6_module, freeaddrinfo);
        GET_WINDOWS_FUNCTION(wship6_module, getnameinfo);
      }
    }
  }
#endif // NO_IPV6
}

void TFileZillaIntf::Finalize()
{
#ifndef NO_IPV6
  if (wship6_module)
    ::FreeLibrary(wship6_module);
  if (winsock2_module)
    ::FreeLibrary(winsock2_module);
#endif
  if (winsock_module)
    ::FreeLibrary(winsock_module);
}

void TFileZillaIntf::SetResourceModule(void * ResourceHandle)
{
  // set afx resource handles, taken from AfxWinInit (mfc/appinit.cpp)
  AFX_MODULE_STATE * ModuleState = AfxGetModuleState();
  ModuleState->m_hCurrentInstanceHandle = static_cast<HINSTANCE>(ResourceHandle);
  ModuleState->m_hCurrentResourceHandle = static_cast<HINSTANCE>(ResourceHandle);
}

TFileZillaIntf::TFileZillaIntf() noexcept :
  FFileZillaApi(nullptr),
  FServer(new t_server())
{
  FIntern = new TFileZillaIntern(this);
}

TFileZillaIntf::~TFileZillaIntf() noexcept
{
  DebugAssert(FFileZillaApi == nullptr);

  delete FIntern;
  FIntern = nullptr;
  delete FServer;
  FServer = nullptr;
}

bool TFileZillaIntf::Init()
{
  DebugAssert(FFileZillaApi == nullptr);

  FFileZillaApi = new CFileZillaApi();

  bool Result = Check(FFileZillaApi->Init(FIntern, this), L"init");

  if (!Result)
  {
    delete FFileZillaApi;
    FFileZillaApi = nullptr;
  }

  return Result;
}

void TFileZillaIntf::Destroying()
{
  // need to close FZAPI before calling destructor as it in turn post messages
  // back while being destroyed, what may result in calling virtual methods
  // of already destroyed descendants
  delete FFileZillaApi;
  FFileZillaApi = nullptr;
}

bool TFileZillaIntf::SetCurrentPath(const wchar_t * APath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->SetCurrentPath(Path), L"setcurrentpath");
}

bool TFileZillaIntf::GetCurrentPath(wchar_t * APath, size_t MaxLen)
{
  CServerPath ServerPath;
  bool Result = Check(FFileZillaApi->GetCurrentPath(ServerPath), L"getcurrentpath");
  if (Result)
  {
    wcsncpy(APath, ServerPath.GetPath(), MaxLen);
    APath[MaxLen - 1] = L'\0';
  }
  return Result;
}

bool TFileZillaIntf::Cancel()
{
  DebugAssert(FFileZillaApi != nullptr);
  // tolerate even "idle" state, quite possible in MT environment
  return Check(FFileZillaApi->Cancel(), L"cancel", FZ_REPLY_WOULDBLOCK | FZ_REPLY_IDLE);
}

bool TFileZillaIntf::Connect(const wchar_t * Host, int Port, const wchar_t * User,
  const wchar_t * Pass, const wchar_t * Account,
  const wchar_t * Path, int ServerType, int Pasv, int TimeZoneOffset, int UTF8,
  int iForcePasvIp, int iUseMlsd,
  X509 * Certificate, EVP_PKEY * PrivateKey,
  int CodePage, int iDupFF, int iUndupFF)
{
  DebugAssert(FFileZillaApi != nullptr);
  DebugAssert((ServerType & FZ_SERVERTYPE_HIGHMASK) == FZ_SERVERTYPE_FTP);

  t_server Server;

  Server.host = Host;
  Server.port = Port;
  Server.user = User;
  Server.pass = Pass;
  Server.account = Account;
  Server.path = Path;
  Server.nServerType = ServerType;
  Server.nPasv = Pasv;
  Server.nTimeZoneOffset = TimeZoneOffset;
  Server.nUTF8 = UTF8;
  Server.nCodePage = CodePage;
  Server.iForcePasvIp = iForcePasvIp;
  Server.iUseMlsd = iUseMlsd;
  Server.iDupFF = iDupFF;
  Server.iUndupFF = iUndupFF;
  Server.Certificate = Certificate;
  Server.PrivateKey = PrivateKey;

  *FServer = Server;

  return Check(FFileZillaApi->Connect(Server), L"connect");
}

bool TFileZillaIntf::Close(bool AllowBusy)
{
  bool Result;
  int ReturnCode = FFileZillaApi->Disconnect();

  switch (ReturnCode)
  {
    // If the connection terminated itself meanwhile,
    // do not try to wait for close response.
    case FZ_REPLY_NOTCONNECTED:
      // We might check AllowBusy here, as it's actually similar scenario,
      // as we expect this to happen during authentication only
      Result = false;
      break;

    // waiting for disconnect
    case FZ_REPLY_WOULDBLOCK:
      Result = true;
      break;

    // allowing busy while opening, not sure if it safe,
    // but we need it, when cancelling password prompt
    case FZ_REPLY_BUSY:
      if (AllowBusy)
      {
        Result = false;
        break;
      }

    case FZ_REPLY_NOTINITIALIZED:
    default:
      Result = Check(ReturnCode, L"disconnect");
      break;
  }
  return Result;
}

bool TFileZillaIntf::CustomCommand(const wchar_t * Command)
{
  DebugAssert(FFileZillaApi != nullptr);
  return Check(FFileZillaApi->CustomCommand(Command), L"customcommand");
}

bool TFileZillaIntf::MakeDir(const wchar_t* APath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->MakeDir(Path), L"makedir");
}

bool TFileZillaIntf::Chmod(int Value, const wchar_t* FileName,
  const wchar_t* APath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->Chmod(Value, FileName, Path), L"chmod");
}

bool TFileZillaIntf::Delete(const wchar_t* FileName, const wchar_t* APath, bool FileNameOnly)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->Delete(FileName, Path, FileNameOnly), L"delete");
}

bool TFileZillaIntf::RemoveDir(const wchar_t* FileName, const wchar_t* APath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->RemoveDir(FileName, Path), L"removedir");
}

bool TFileZillaIntf::Rename(const wchar_t* OldName,
  const wchar_t* NewName, const wchar_t* APath, const wchar_t* ANewPath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  CServerPath NewPath(ANewPath, false);
  return Check(FFileZillaApi->Rename(OldName, NewName, Path, NewPath), L"rename");
}

bool TFileZillaIntf::List(const wchar_t * APath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->List(Path), L"list");
}

bool TFileZillaIntf::ListFile(const wchar_t * FileName, const wchar_t * APath)
{
  DebugAssert(FFileZillaApi != nullptr);
  CServerPath Path(APath, false);
  return Check(FFileZillaApi->ListFile(FileName, Path), L"listfile");
}

bool TFileZillaIntf::FileTransfer(
  const wchar_t * LocalFile, const wchar_t * RemoteFile,
  const wchar_t * RemotePath, bool Get, __int64 Size, int Type, void * UserData,
  TTransferOutEvent OnTransferOut, TTransferInEvent OnTransferIn)
{
  t_transferfile Transfer;

  Transfer.localfile = LocalFile;
  Transfer.remotefile = RemoteFile;
  Transfer.remotepath = CServerPath(RemotePath, false);
  Transfer.get = Get;
  Transfer.size = Size;
  Transfer.server = *FServer;
  // 1 = ascii, 2 = binary
  Transfer.nType = Type;
  Transfer.nUserData = reinterpret_cast<void*>(UserData);
  Transfer.OnTransferOut = OnTransferOut;
  Transfer.OnTransferIn = OnTransferIn;

  return Check(FFileZillaApi->FileTransfer(Transfer), L"filetransfer");
}

void TFileZillaIntf::SetDebugLevel(TLogLevel Level)
{
  FIntern->SetDebugLevel(Level - LOG_APIERROR + 1);
}

bool TFileZillaIntf::FZPostMessage(WPARAM wParam, LPARAM lParam)
{
  unsigned int MessageID = FZ_MSG_ID(wParam);
  TMessageType Type;
  switch (MessageID)
  {
    case FZ_MSG_TRANSFERSTATUS:
      Type = MSG_TRANSFERSTATUS;
      break;

    default:
      Type = MSG_OTHER;
      break;
  }
  return DoPostMessage(Type, wParam, lParam);
}

void CopyContact(TFtpsCertificateData::TContact & Dest,
  const t_SslCertData::t_Contact& Source)
{
  Dest.Organization = Source.Organization;
  Dest.Unit = Source.Unit;
  Dest.CommonName = Source.CommonName;
  Dest.Mail = Source.Mail;
  Dest.Country = Source.Country;
  Dest.StateProvince = Source.StateProvince;
  Dest.Town = Source.Town;
  Dest.Other = Source.Other;
}

void CopyValidityTime(TFtpsCertificateData::TValidityTime & Dest,
  const t_SslCertData::t_validTime& Source)
{
  Dest.Year = Source.y;
  Dest.Month = Source.M;
  Dest.Day = Source.d;
  Dest.Hour = Source.h;
  Dest.Min = Source.m;
  Dest.Sec = Source.s;
}

void CopyFileTime(TRemoteFileTime & Dest, const t_directory::t_direntry::t_date & Source)
{
  Dest.Year = static_cast<WORD>(Source.year);
  Dest.Month = static_cast<WORD>(Source.month);
  Dest.Day = static_cast<WORD>(Source.day);
  Dest.Hour = static_cast<WORD>(Source.hour);
  Dest.Minute = static_cast<WORD>(Source.minute);
  Dest.Second = static_cast<WORD>(Source.second);
  Dest.HasTime = Source.hastime;
  Dest.HasDate = Source.hasdate;
  Dest.HasYear = Source.hasyear;
  Dest.HasSeconds = Source.hasseconds;
  Dest.Utc = Source.utc;
}

bool TFileZillaIntf::HandleMessage(WPARAM wParam, LPARAM lParam)
{
  bool Result = false;

  CString a;
  unsigned int MessageID = FZ_MSG_ID(wParam);

  switch (MessageID)
  {
    case FZ_MSG_STATUS:
      {
        DebugAssert(FZ_MSG_PARAM(wParam) == 0);
        t_ffam_statusmessage * Status = reinterpret_cast<t_ffam_statusmessage *>(lParam);
        DebugAssert(Status->post);
        Result = HandleStatus(Status->status, Status->type);
        delete Status;
      }

      break;

    case FZ_MSG_ASYNCREQUEST:
      if (FZ_MSG_PARAM(wParam) == FZ_ASYNCREQUEST_OVERWRITE)
      {
        int RequestResult = 0;
        wchar_t FileName1[MAX_PATH];
        COverwriteRequestData * Data = reinterpret_cast<COverwriteRequestData *>(lParam);
        DebugAssert(Data != nullptr);
        if (Data)
        try
        {
          DebugAssert(Data != nullptr);
          wcsncpy(FileName1, Data->FileName1, LENOF(FileName1));
          FileName1[LENOF(FileName1) - 1] = L'\0';
          TRemoteFileTime RemoteTime;
          CopyFileTime(RemoteTime, Data->remotetime);
          Result = HandleAsynchRequestOverwrite(
            FileName1, LENOF(FileName1), Data->FileName2, Data->path1, Data->path2,
            Data->size1, Data->size2,
            (Data->localtime != nullptr) ? Data->localtime->GetTime() : 0,
            (Data->localtime != nullptr) && ((Data->localtime->GetHour() != 0) || (Data->localtime->GetMinute() != 0)),
            RemoteTime,
            reinterpret_cast<void*>(Data->pTransferFile->nUserData), RequestResult);
        }
        catch(...)
        {
          FFileZillaApi->SetAsyncRequestResult(FILEEXISTS_SKIP, Data);
          throw;
        }

        if (Result)
        {
          Data->FileName1 = FileName1;
          Result = Check(FFileZillaApi->SetAsyncRequestResult(RequestResult, Data),
            L"setasyncrequestresult");
        }
      }
      else if (FZ_MSG_PARAM(wParam) == FZ_ASYNCREQUEST_VERIFYCERT)
      {
        int RequestResult;
        CVerifyCertRequestData * AData = reinterpret_cast<CVerifyCertRequestData *>(lParam);
        DebugAssert(AData != nullptr);
        if (AData)
        try
        {
          DebugAssert(AData != nullptr);
          TFtpsCertificateData Data;
          CopyContact(Data.Subject, AData->pCertData->subject);
          CopyContact(Data.Issuer, AData->pCertData->issuer);
          CopyValidityTime(Data.ValidFrom, AData->pCertData->validFrom);
          CopyValidityTime(Data.ValidUntil, AData->pCertData->validUntil);
          Data.SubjectAltName = AData->pCertData->subjectAltName;
          Data.HashSha1 = AData->pCertData->hashSha1;
          DebugAssert(Data.HashSha1Len == sizeof(AData->pCertData->hashSha1));
          Data.HashSha256 = AData->pCertData->hashSha256;
          DebugAssert(Data.HashSha256Len == sizeof(AData->pCertData->hashSha256));
          Data.Certificate = AData->pCertData->certificate;
          Data.CertificateLen = AData->pCertData->certificateLen;
          Data.VerificationResult = AData->pCertData->verificationResult;
          Data.VerificationDepth = AData->pCertData->verificationDepth;

          Result = HandleAsyncRequestVerifyCertificate(Data, RequestResult);
        }
        catch(...)
        {
          FFileZillaApi->SetAsyncRequestResult(0, AData);
          throw;
        }

        if (Result)
        {
          Result = Check(FFileZillaApi->SetAsyncRequestResult(RequestResult, AData),
            L"setasyncrequestresult");
        }
      }
      else if (FZ_MSG_PARAM(wParam) == FZ_ASYNCREQUEST_NEEDPASS)
      {
        int RequestResult = 0;
        CNeedPassRequestData * AData = reinterpret_cast<CNeedPassRequestData *>(lParam);
        try
        {
            TNeedPassRequestData Data;
            Data.Password = nullptr;
            Data.Password = AData->Password.GetBuffer(AData->Password.GetLength());
            Result = HandleAsyncRequestNeedPass(Data, RequestResult);
            AData->Password.ReleaseBuffer(AData->Password.GetLength());
            if (Result && (RequestResult == TFileZillaIntf::REPLY_OK))
            {
              AData->Password = Data.Password;
              free(Data.Password);
              Data.Password = nullptr;
            }
        }
        catch(...)
        {
          FFileZillaApi->SetAsyncRequestResult(0, AData);
          throw;
        }
        if (Result)
        {
          Result = Check(FFileZillaApi->SetAsyncRequestResult(RequestResult, AData),
            L"setasyncrequestresult");
        }
      }
      else
      {
        // FZ_ASYNCREQUEST_GSS_AUTHFAILED
        // FZ_ASYNCREQUEST_GSS_NEEDUSER
        // FZ_ASYNCREQUEST_GSS_NEEDPASS
        DebugFail();
        Result = false;
      }
      break;

    case FZ_MSG_LISTDATA:
      {
        DebugAssert(FZ_MSG_PARAM(wParam) == 0);
        t_directory * Directory = reinterpret_cast<t_directory *>(lParam);
        CString Path = Directory->path.GetPath();
        nb::vector_t<TListDataEntry> Entries(Directory->num);

        for (int32_t Index = 0; Index < Directory->num; ++Index)
        {
          t_directory::t_direntry & Source = Directory->direntry[Index];
          TListDataEntry & Dest = Entries[Index];

          Dest.Name = Source.name;
          Dest.Permissions = Source.permissionstr;
          Dest.HumanPerm = Source.humanpermstr;
          Dest.OwnerGroup = Source.ownergroup;
          Dest.Owner = Source.owner;
          Dest.Group = Source.group;
          Dest.Size = Source.size;
          Dest.Dir = Source.dir;
          Dest.Link = Source.bLink;
          CopyFileTime(Dest.Time, Source.date);
          Dest.LinkTarget = Source.linkTarget;
        }

        int Num = Directory->num;
        TListDataEntry * pEntries = Num > 0 ? &Entries[0] : nullptr;
        Result = HandleListData(Path, pEntries, Num);

        delete Directory;
      }
      break;

    case FZ_MSG_TRANSFERSTATUS:
      {
        DebugAssert(FZ_MSG_PARAM(wParam) == 0);
        t_ffam_transferstatus * Status = reinterpret_cast<t_ffam_transferstatus *>(lParam);
        if (Status != nullptr)
        {
          Result = HandleTransferStatus(
            true, Status->transfersize, Status->bytes, Status->bFileTransfer != FALSE);
          delete Status;
        }
        else
        {
          Result = HandleTransferStatus(false, -1, -1, false);
        }
      }
      break;

    case FZ_MSG_REPLY:
      Result = HandleReply(FZ_MSG_PARAM(wParam), lParam);
      break;

    case FZ_MSG_CAPABILITIES:
      Result = HandleCapabilities(reinterpret_cast<TFTPServerCapabilities *>(lParam));
      break;

    default:
      DebugFail();
      Result = false;
      break;
  }

  return Result;
}

bool TFileZillaIntf::CheckError(int32_t /*ReturnCode*/, const wchar_t * /*Context*/)
{
  return false;
}

inline bool TFileZillaIntf::Check(int32_t ReturnCode,
  const wchar_t * Context, int32_t Expected)
{
  if ((ReturnCode & (Expected == -1 ? FZ_REPLY_OK : Expected)) == ReturnCode)
  {
    return true;
  }
  else
  {
    return CheckError(ReturnCode, Context);
  }
}

bool TFileZillaIntf::UsingMlsd()
{
  return FFileZillaApi->UsingMlsd();
}

bool TFileZillaIntf::UsingUtf8()
{
  return FFileZillaApi->UsingUtf8();
}

std::string TFileZillaIntf::GetTlsVersionStr()
{
  return FFileZillaApi->GetTlsVersionStr();
}

std::string TFileZillaIntf::GetCipherName()
{
  return FFileZillaApi->GetCipherName();
}
