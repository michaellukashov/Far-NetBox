//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
#include "FileZillaIntf.h"
#include "FileZillaIntern.h"
#include "FzApiStructures.h"
#include "structures.h"
#include "Common.h"
//---------------------------------------------------------------------------
#ifndef _DEBUG
#pragma comment(lib, "uafxcw.lib")
#else
#pragma comment(lib, "uafxcwd.lib")
#endif
//---------------------------------------------------------------------------
#define LENOF(x) ( (sizeof((x))) / (sizeof(*(x))))
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
  TRACE("1");
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

  bool Result = Check(FFileZillaApi->Init(FIntern), L"init");

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
  TRACE("1");
  // need to close FZAPI before calling destructor as it in turn post messages
  // back while being destroyed, what may result in calling virtual methods
  // of already destroyed descendants
  delete FFileZillaApi;
  FFileZillaApi = NULL;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::SetCurrentPath(const wchar_t * APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->SetCurrentPath(Path), L"setcurrentpath");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::GetCurrentPath(wchar_t * Path, size_t MaxLen)
{
  CServerPath APath;
  bool Result = Check(FFileZillaApi->GetCurrentPath(APath), L"getcurrentpath");
  if (Result)
  {
    wcsncpy(Path, APath.GetPath(), MaxLen);
    Path[MaxLen - 1] = L'\0';
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Cancel()
{
  ASSERT(FFileZillaApi != NULL);
  // tolerate even "idle" state, quite possible in MT environment
  return Check(FFileZillaApi->Cancel(), L"cancel", FZ_REPLY_WOULDBLOCK | FZ_REPLY_IDLE);
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Connect(const wchar_t * Host, int Port, const wchar_t * User,
  const wchar_t * Pass, const wchar_t * Account, bool FwByPass,
  const wchar_t * Path, int ServerType, int Pasv, int TimeZoneOffset, int UTF8,
  int iForcePasvIp, int iUseMlsd)
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
  Server.iForcePasvIp = iForcePasvIp;
  Server.iUseMlsd = iUseMlsd;

  *FServer = Server;

  return Check(FFileZillaApi->Connect(Server), L"connect");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Close(bool AllowBusy)
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
//---------------------------------------------------------------------------
bool TFileZillaIntf::CustomCommand(const wchar_t * Command)
{
  ASSERT(FFileZillaApi != NULL);
  return Check(FFileZillaApi->CustomCommand(Command), L"customcommand");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::MakeDir(const wchar_t* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->MakeDir(Path), L"makedir");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Chmod(int Value, const wchar_t* FileName,
  const wchar_t* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->Chmod(Value, FileName, Path), L"chmod");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Delete(const wchar_t* FileName, const wchar_t* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->Delete(FileName, Path), L"delete");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::RemoveDir(const wchar_t* FileName, const wchar_t* APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->RemoveDir(FileName, Path), L"removedir");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::Rename(const wchar_t* OldName,
  const wchar_t* NewName, const wchar_t* APath, const wchar_t* ANewPath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  CServerPath NewPath(ANewPath, FServer->nServerType);
  return Check(FFileZillaApi->Rename(OldName, NewName, Path, NewPath), L"rename");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::List()
{
  ASSERT(FFileZillaApi != NULL);
  return Check(FFileZillaApi->List(), L"list");
}
//---------------------------------------------------------------------------
bool TFileZillaIntf::List(const wchar_t * APath)
{
  ASSERT(FFileZillaApi != NULL);
  CServerPath Path(APath, FServer->nServerType);
  return Check(FFileZillaApi->List(Path), L"list");
}
//---------------------------------------------------------------------------
#ifdef MPEXT
bool TFileZillaIntf::ListFile(const wchar_t * AFullFileName)
{
  ASSERT(FFileZillaApi != NULL);
  CString FileName(AFullFileName);
  CServerPath Path(FServer->nServerType);
  Path.SetPath(FileName, TRUE);
  return Check(FFileZillaApi->ListFile(Path, FileName), L"listfile");
}
#endif
//---------------------------------------------------------------------------
bool TFileZillaIntf::FileTransfer(const wchar_t * LocalFile,
  const wchar_t * RemoteFile, const wchar_t * RemotePath, bool Get, __int64 Size,
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

  return Check(FFileZillaApi->FileTransfer(Transfer), L"filetransfer");
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

  CString a;
  unsigned int MessageID = FZ_MSG_ID(wParam);
  TRACEFMT("1 [%x] (%d) [%x]", wParam, MessageID, lParam);

  switch (MessageID)
  {
    case FZ_MSG_STATUS:
      {
        ASSERT(FZ_MSG_PARAM(wParam) == 0);
        t_ffam_statusmessage * Status = (t_ffam_statusmessage *)lParam;
        ASSERT(Status->post);
        Result = HandleStatus(Status->status, Status->type);
        delete Status;
      }

      break;

    case FZ_MSG_ASYNCREQUEST:
      TRACE("AsyncRequest");
      if (FZ_MSG_PARAM(wParam) == FZ_ASYNCREQUEST_OVERWRITE)
      {
        TRACE("Overwrite");
        int RequestResult;
        wchar_t FileName1[MAX_PATH];
        COverwriteRequestData * Data = (COverwriteRequestData *)lParam;
        try
        {
          ASSERT(Data != NULL);
          wcsncpy(FileName1, Data->FileName1, LENOF(FileName1));
          FileName1[LENOF(FileName1) - 1] = L'\0';
          TRACE("Before HandleAsynchRequestOverwrite");
          TRACEFMT("FileName1 [%s]", FileName1);
          TRACEFMT("FileName2 [%s]", (const wchar_t*)Data->FileName2);
          TRACEFMT("path1 [%s]", (const wchar_t*)Data->path1);
          TRACEFMT("path2 [%s]", (const wchar_t*)Data->path2);
          TRACEFMT("size1 [%d]", (int)Data->size1);
          TRACEFMT("size2 [%d]", (int)Data->size2);
          TRACEFMT("time1 [%x]", Data->time1);
          TRACEFMT("time1 [%d]", ((Data->time1 != NULL) ? Data->time1->GetTime() : 0));
          TRACEFMT("time2 [%x]", Data->time2);
          TRACEFMT("time2 [%d]", ((Data->time2 != NULL) ? Data->time2->GetTime() : 0));
          TRACEFMT("HasTime1 [%d]", (Data->time1 != NULL) && ((Data->time1->GetHour() != 0) || (Data->time1->GetMinute() != 0)));
          TRACEFMT("HasTime2 [%d]", (Data->time2 != NULL) && ((Data->time2->GetHour() != 0) || (Data->time2->GetMinute() != 0)));
          TRACEFMT("nUserData [%x]", reinterpret_cast<void*>(Data->pTransferFile->UserData));
          Result = HandleAsynchRequestOverwrite(
            FileName1, LENOF(FileName1), Data->FileName2, Data->path1, Data->path2,
            Data->size1, Data->size2,
            (Data->time1 != NULL) ? Data->time1->GetTime() : 0,
            (Data->time2 != NULL) ? Data->time2->GetTime() : 0,
            (Data->time1 != NULL) && ((Data->time1->GetHour() != 0) || (Data->time1->GetMinute() != 0)),
            (Data->time2 != NULL) && ((Data->time2->GetHour() != 0) || (Data->time2->GetMinute() != 0)),
            reinterpret_cast<void*>(Data->pTransferFile->UserData), RequestResult);
        }
        catch(...)
        {
          TRACE("overwrite exception");
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
        CVerifyCertRequestData * AData = (CVerifyCertRequestData *)lParam;
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
          TRACE("cert exception");
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
        CNeedPassRequestData * AData = (CNeedPassRequestData *)lParam;
        try
        {
            TNeedPassRequestData Data;
            Data.Password = NULL;
            Data.Password = AData->Password.GetBuffer(AData->Password.GetLength());
            Result = HandleAsynchRequestNeedPass(Data, RequestResult);
            AData->Password.ReleaseBuffer(AData->Password.GetLength());
            if (Result)
            {
              AData->Password = Data.Password;
              free(Data.Password);
              Data.Password = NULL;
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
        ASSERT(FALSE);
        Result = false;
      }
      break;

    case FZ_MSG_LISTDATA:
      {
        ASSERT(FZ_MSG_PARAM(wParam) == 0);
        t_directory * Directory = (t_directory *)lParam;
        CString Path = Directory->path.GetPath();
        std::vector<TListDataEntry> Entries(Directory->num);

        for (intptr_t Index = 0; Index < Directory->num; ++Index)
        {
          t_directory::t_direntry & Source = Directory->direntry[Index];
          TListDataEntry & Dest = Entries[Index];

          Dest.Name = Source.name;
          Dest.Permissions = Source.permissionstr;
          Dest.OwnerGroup = Source.ownergroup;
          Dest.Size = Source.size;
          Dest.Dir = Source.dir;
          Dest.Link = Source.bLink;
          Dest.Year = Source.date.year;
          Dest.Month = Source.date.month;
          Dest.Day = Source.date.day;
          Dest.Hour = Source.date.hour;
          Dest.Minute = Source.date.minute;
          Dest.Second = Source.date.second;
          Dest.HasTime = Source.date.hastime;
          Dest.HasDate = Source.date.hasdate;
          Dest.HasSeconds = Source.date.hasseconds;
          Dest.Utc = Source.date.utc;
          Dest.LinkTarget = Source.linkTarget;
        }

        int Num = Directory->num;
        TListDataEntry * pEntries = Num > 0 ? &Entries[0] : NULL;
        Result = HandleListData(Path, pEntries, Num);

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
            Status->transferrate, Status->bFileTransfer != 0);
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
      Result = HandleCapabilities((TFTPServerCapabilities *)lParam);
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
bool TFileZillaIntf::CheckError(int /*ReturnCode*/, const wchar_t * /*Context*/)
{
  return false;
}
//---------------------------------------------------------------------------
inline bool TFileZillaIntf::Check(int ReturnCode,
  const wchar_t * Context, int Expected)
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
