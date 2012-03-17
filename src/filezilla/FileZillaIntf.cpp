//---------------------------------------------------------------------------
#include "fzafx.h"
//---------------------------------------------------------------------------
#include "FtpControlSocket.h"
#include "FileZillaIntf.h"
#include "FileZillaIntern.h"
#include "FzApiStructures.h"
#include "structures.h"
#include "Classes.h"
#include "Common.h"
//---------------------------------------------------------------------------
#ifndef _DEBUG
#pragma comment(lib, "uafxcw.lib")
#else
#pragma comment(lib, "uafxcwd.lib")
#endif
//---------------------------------------------------------------------------
void TFileZillaIntf::Initialize()
{
  // noop
}
//---------------------------------------------------------------------------
void TFileZillaIntf::Finalize()
{
  // noop
}
//---------------------------------------------------------------------------
void TFileZillaIntf::SetResourceModule(void * ResourceHandle)
{
  // set afx resource handles, taken from AfxWinInit (mfc/appinit.cpp)
  AFX_MODULE_STATE * ModuleState = AfxGetModuleState();
  ModuleState->m_hCurrentInstanceHandle = static_cast<HINSTANCE>(ResourceHandle);
  ModuleState->m_hCurrentResourceHandle = static_cast<HINSTANCE>(ResourceHandle);
}
//---------------------------------------------------------------------------
TFileZillaIntf::TFileZillaIntf() :
  FFileZillaApi(NULL),
  FIntern(new TFileZillaIntern(this)),
  FServer(new t_server())
{
}
//---------------------------------------------------------------------------
TFileZillaIntf::~TFileZillaIntf()
{
  ASSERT(FFileZillaApi == NULL);

  delete FIntern;
  FIntern = NULL;
  delete FServer;
  FServer = NULL;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Init()
{
  ASSERT(FFileZillaApi == NULL);

  FFileZillaApi = new CFileZillaApi();

  bool Result = Check(FFileZillaApi->Init(FIntern), "init");

  if (!Result)
  {
    delete FFileZillaApi;
    FFileZillaApi = NULL;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TFileZillaIntf::Destroying()
{
  // need to close FZAPI before calling destructor as it in turn post messages
  // back while being destroyed, what may result in calling virtual methods
  // of already destroyed descendants
  delete FFileZillaApi;
  FFileZillaApi = NULL;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::SetCurrentPath(const char * APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->SetCurrentPath(Path), "setcurrentpath");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::GetCurrentPath(char * Path, size_t MaxLen)
{
  CServerPath APath;
  bool Result = Check(FFileZillaApi->GetCurrentPath(APath), "getcurrentpath");
  if (Result)
  {
    // char nstring[APath.Size()];
    // strcpy_s(nstring, APath);
    // strncpy(Path, APath.GetPath(), MaxLen);
	CString path = APath.GetPath();
    strcpy_s(Path, MaxLen, nb::W2MB(path.GetBuffer(MaxLen)).c_str());
    Path[MaxLen - 1] = '\0';
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Cancel()
{
  ASSERT(FFileZillaApi != NULL);
  // tolerate even "idle" state, quite possible in MT environment
  return Check(FFileZillaApi->Cancel(), "cancel", FZ_REPLY_WOULDBLOCK | FZ_REPLY_IDLE);
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Connect(const char * Host, int Port, const char * User,
  const char * Pass, const char * Account, bool FwByPass,
  const char * Path, int ServerType, int Pasv, int TimeZoneOffset, int UTF8,
  bool bForcePasvIp)
{
  ASSERT(FFileZillaApi != NULL);
  ASSERT((ServerType & FZ_SERVERTYPE_HIGHMASK) == FZ_SERVERTYPE_FTP);

  t_server Server;

  Server.host = Host;
  Server.port = Port;
  Server.user = User;
  Server.pass = Pass;
  Server.account = Account;
  Server.fwbypass = FwByPass;
  Server.path = Path;
  Server.nServerType = ServerType;
  Server.nPasv = Pasv;
  Server.nTimeZoneOffset = TimeZoneOffset;
  Server.nUTF8 = UTF8;
  Server.bForcePasvIp = bForcePasvIp;

  *FServer = Server;

  return Check(FFileZillaApi->Connect(Server), "connect");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Close()
{
  bool Result;
  int ReturnCode = FFileZillaApi->Disconnect();

  switch (ReturnCode)
  {
    // it the connection terminated itself meanwhile
    case FZ_REPLY_NOTCONNECTED:
      Result = true;
      break;

    // waiting for disconnect
    case FZ_REPLY_WOULDBLOCK:
      Result = true;
      break;

    case FZ_REPLY_NOTINITIALIZED:
    default:
      Result = Check(ReturnCode, "disconnect");
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::CustomCommand(const wchar_t * Command)
{
  ASSERT(FFileZillaApi != NULL);
  return Check(FFileZillaApi->CustomCommand(Command), "customcommand");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::MakeDir(const char* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->MakeDir(Path), "makedir");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Chmod(int Value, const char* FileName,
  const char* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->Chmod(Value, FileName, Path), "chmod");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Delete(const char* FileName, const char* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->Delete(FileName, Path), "delete");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::RemoveDir(const char* FileName, const char* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->RemoveDir(FileName, Path), "removedir");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Rename(const char* OldName,
  const char* NewName, const char* APath, const char* ANewPath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  CServerPath NewPath(ANewPath, FServer->nServerType);
  return Check(FFileZillaApi->Rename(OldName, NewName, Path, NewPath), "rename");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::List()
{
  ASSERT(FFileZillaApi != NULL);
  return Check(FFileZillaApi->List(), "list");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::List(const char * APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->List(Path), "list");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::FileTransfer(const char * LocalFile,
  const char * RemoteFile, const char * RemotePath, bool Get, __int64 Size,
  int Type, void * UserData)
{
  t_transferfile Transfer;

  Transfer.localfile = LocalFile;
  Transfer.remotefile = RemoteFile;
  Transfer.remotepath = CServerPath(RemotePath, FServer->nServerType);
  Transfer.get = Get;
  Transfer.size = Size;
  Transfer.server = *FServer;
  // 1 = ascii, 2 = binary
  Transfer.nType = Type;
  Transfer.UserData = UserData;

  return Check(FFileZillaApi->FileTransfer(Transfer), "filetransfer");
}
//---------------------------------------------------------------------------
void TFileZillaIntf::SetDebugLevel(TLogLevel Level)
{
  FIntern->SetDebugLevel(Level - LOG_APIERROR + 1);
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::PostMessage(WPARAM wParam, LPARAM lParam)
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
bool TFileZillaIntf::HandleMessage(WPARAM wParam, LPARAM lParam)
{
  bool Result;
  TFTPServerCapabilities serverCapabilities;
  unsigned int MessageID = FZ_MSG_ID(wParam);

  // DEBUG_PRINTF(L"MessageID = %u, lParam = %u", MessageID, lParam);
  switch (MessageID)
  {
    case FZ_MSG_STATUS:
      {
        ASSERT(FZ_MSG_PARAM(wParam) == 0);
        t_ffam_statusmessage * Status = reinterpret_cast<t_ffam_statusmessage *>(lParam);
        ASSERT(Status->post);
        Result = HandleStatus(nb::W2MB(Status->status.GetBuffer(Status->status.GetLength())).c_str(), Status->type);
        delete Status;
      }

      break;

    case FZ_MSG_ASYNCREQUEST:
      if (FZ_MSG_PARAM(wParam) == FZ_ASYNCREQUEST_OVERWRITE)
      {
        int RequestResult;
        char FileName1[MAX_PATH];
        COverwriteRequestData * Data = reinterpret_cast<COverwriteRequestData *>(lParam);
        try
        {
          ASSERT(Data != NULL);
          strncpy(FileName1, nb::W2MB(Data->FileName1.GetBuffer(Data->FileName1.GetLength())).c_str(), sizeof(FileName1));
          FileName1[sizeof(FileName1) - 1] = '\0';
          Result = HandleAsynchRequestOverwrite(
            FileName1, sizeof(FileName1),
			nb::W2MB(Data->FileName2.GetBuffer(Data->FileName2.GetLength())).c_str(),
			nb::W2MB(Data->path1.GetBuffer(Data->path1.GetLength())).c_str(),
			nb::W2MB(Data->path2.GetBuffer(Data->path2.GetLength())).c_str(),
            Data->size1, Data->size2,
            (Data->time1 != NULL) ? Data->time1->GetTime() : 0,
            (Data->time2 != NULL) ? Data->time2->GetTime() : 0,
            (Data->time1 != NULL) && ((Data->time1->GetHour() != 0) || (Data->time1->GetMinute() != 0)),
            (Data->time2 != NULL) && ((Data->time2->GetHour() != 0) || (Data->time2->GetMinute() != 0)),
            Data->pTransferFile->UserData, RequestResult);
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
            "setasyncrequestresult");
        }
      }
      else if (FZ_MSG_PARAM(wParam) == FZ_ASYNCREQUEST_VERIFYCERT)
      {
        int RequestResult;
        CVerifyCertRequestData * AData = reinterpret_cast<CVerifyCertRequestData *>(lParam);
        try
        {
          ASSERT(AData != NULL);
          TFtpsCertificateData Data;
          CopyContact(Data.Subject, AData->pCertData->subject);
          CopyContact(Data.Issuer, AData->pCertData->issuer);
          CopyValidityTime(Data.ValidFrom, AData->pCertData->validFrom);
          CopyValidityTime(Data.ValidUntil, AData->pCertData->validUntil);
          Data.Hash = AData->pCertData->hash;
          Data.VerificationResult = AData->pCertData->verificationResult;
          Data.VerificationDepth = AData->pCertData->verificationDepth;

          Result = HandleAsynchRequestVerifyCertificate(Data, RequestResult);
        }
        catch(...)
        {
          FFileZillaApi->SetAsyncRequestResult(0, AData);
          throw;
        }

        if (Result)
        {
          Result = Check(FFileZillaApi->SetAsyncRequestResult(RequestResult, AData),
            "setasyncrequestresult");
        }
      }
      else
      {
        // FZ_ASYNCREQUEST_GSS_AUTHFAILED
        // FZ_ASYNCREQUEST_GSS_NEEDUSER
        // FZ_ASYNCREQUEST_GSS_NEEDPASS
        ASSERT(FALSE);
        Result = false;
      }
      break;

    case FZ_MSG_LISTDATA:
      {
        ASSERT(FZ_MSG_PARAM(wParam) == 0);
        t_directory * Directory = reinterpret_cast<t_directory *>(lParam);
        CString Path = Directory->path.GetPath();
        std::vector<TListDataEntry> Entries(Directory->num);

        for (size_t Index = 0; Index < static_cast<size_t>(Directory->num); Index++)
        {
          t_directory::t_direntry & Source = Directory->direntry[Index];
          TListDataEntry & Dest = Entries[Index];

          Dest.Name = reinterpret_cast<const wchar_t *>(Source.name.GetBuffer(Source.name.GetLength()));
          Dest.Permissions = reinterpret_cast<const wchar_t *>(Source.permissionstr.GetBuffer(Source.permissionstr.GetLength()));
          Dest.OwnerGroup = reinterpret_cast<const wchar_t *>(Source.ownergroup.GetBuffer(Source.ownergroup.GetLength()));
          Dest.Size = Source.size;
          Dest.Dir = Source.dir;
          Dest.Link = Source.bLink;
          Dest.Year = Source.date.year;
          Dest.Month = Source.date.month;
          Dest.Day = Source.date.day;
          Dest.Hour = Source.date.hour;
          Dest.Minute = Source.date.minute;
          Dest.HasTime = Source.date.hastime;
          Dest.HasDate = Source.date.hasdate;
          Dest.LinkTarget = reinterpret_cast<const wchar_t *>(Source.linkTarget.GetBuffer(Source.linkTarget.GetLength()));
        }

        size_t Num = Directory->num;
        TListDataEntry * pEntries = Num > 0 ? &Entries[0] : NULL;
        Result = HandleListData(nb::W2MB(Path.GetBuffer(Path.GetLength())).c_str(), pEntries, Num);
        delete Directory;
      }
      break;

    case FZ_MSG_TRANSFERSTATUS:
      {
        ASSERT(FZ_MSG_PARAM(wParam) == 0);
        t_ffam_transferstatus * Status = reinterpret_cast<t_ffam_transferstatus *>(lParam);
        if (Status != NULL)
        {
          Result = HandleTransferStatus(true, Status->transfersize, Status->bytes,
            Status->percent, Status->timeelapsed, Status->timeleft,
            Status->transferrate, Status->bFileTransfer);
          delete Status;
        }
        else
        {
          Result = HandleTransferStatus(false, -1, -1, -1, -1, -1, -1, false);
        }
      }
      break;

    case FZ_MSG_REPLY:
      Result = HandleReply(FZ_MSG_PARAM(wParam), lParam);
      break;

    case FZ_MSG_CAPABILITIES:
      Result = HandleCapabilities(serverCapabilities);
      break;

    case FZ_MSG_SOCKETSTATUS:
    case FZ_MSG_SECURESERVER:
    case FZ_MSG_QUITCOMPLETE:
    default:
      ASSERT(false);
      Result = false;
      break;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::CheckError(int /*ReturnCode*/, const char * /*Context*/)
{
  return false;
}
//---------------------------------------------------------------------------
inline bool TFileZillaIntf::Check(int ReturnCode,
  const char * Context, int Expected)
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
