//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define TRACE_FILE_APPL_INFO NOTRACING

#include <FileInfo.h>

#include "Exceptions.h"
#include "Common.h"
#include "Configuration.h"
#include "PuttyIntf.h"
#include "TextsCore.h"
#include "Interface.h"
#include "CoreMain.h"
#include "WinSCPSecurity.h"
#include <shlobj.h>
#ifndef _MSC_VER
#include <System.IOUtils.hpp>
#endif
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
/* __fastcall */ TConfiguration::TConfiguration() :
  FDontSave(false),
  FChanged(false),
  FUpdating(0),
  FApplicationInfo(NULL),
  FLogging(false),
  FPermanentLogging(false),
  FLogWindowLines(0),
  FLogFileAppend(false),
  FLogProtocol(0),
  FActualLogProtocol(0),
  FLogActions(false),
  FPermanentLogActions(false),
  FConfirmOverwriting(false),
  FConfirmResume(false),
  FAutoReadDirectoryAfterOp(false),
  FSessionReopenAuto(0),
  FSessionReopenBackground(0),
  FSessionReopenTimeout(0),
  FSessionReopenAutoStall(0),
  FTunnelLocalPortNumberLow(0),
  FTunnelLocalPortNumberHigh(0),
  FCacheDirectoryChangesMaxSize(0),
  FShowFtpWelcomeMessage(false),
  FDisablePasswordStoring(false),
  FForceBanners(false),
  FDisableAcceptingHostKeys(false),
  FDefaultCollectUsage(false),
  FSessionReopenAutoMaximumNumberOfRetries(0),
  FCriticalSection(NULL)
{
  CALLSTACK;
  FCriticalSection = new TCriticalSection();
  FUpdating = 0;
  FStorage = stRegistry;
  FDontSave = false;
  FApplicationInfo = NULL;
  // FUsage = new TUsage(this);
  // FDefaultCollectUsage = false;

  wchar_t Buf[10];
  UnicodeString RandomSeedPath;
  if (GetEnvironmentVariable(L"APPDATA", Buf, LENOF(Buf)) > 0)
  {
    RandomSeedPath = L"%APPDATA%";
  }
  else
  {
    RandomSeedPath = GetShellFolderPath(CSIDL_LOCAL_APPDATA);
    if (RandomSeedPath.IsEmpty())
    {
      RandomSeedPath = GetShellFolderPath(CSIDL_APPDATA);
    }
  }

  FDefaultRandomSeedFile = IncludeTrailingBackslash(RandomSeedPath) + L"winscp.rnd";
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Default()
{
  CALLSTACK;
  TGuard Guard(FCriticalSection);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  TRegistryStorage * AdminStorage = NULL;
  AdminStorage = new TRegistryStorage(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE);
  std::auto_ptr<TRegistryStorage> AdminStoragePtr(AdminStorage);
  {
    if (AdminStorage->OpenRootKey(false))
    {
      LoadAdmin(AdminStorage);
      AdminStorage->CloseSubKey();
    }
  }

  SetRandomSeedFile(FDefaultRandomSeedFile);
  SetPuttyRegistryStorageKey(L"Software\\SimonTatham\\PuTTY");
  FConfirmOverwriting = true;
  FConfirmResume = true;
  FAutoReadDirectoryAfterOp = true;
  FSessionReopenAuto = 5000;
  FSessionReopenBackground = 2000;
  FSessionReopenTimeout = 0;
  FSessionReopenAutoStall = 60000;
  FTunnelLocalPortNumberLow = 50000;
  FTunnelLocalPortNumberHigh = 50099;
  FCacheDirectoryChangesMaxSize = 100;
  FShowFtpWelcomeMessage = false;
  FExternalIpAddress = L"";
  SetCollectUsage(FDefaultCollectUsage);
  FSessionReopenAutoMaximumNumberOfRetries = CONST_DEFAULT_NUMBER_OF_RETRIES;
  FDefaultCollectUsage = false;

  FLogging = false;
  FPermanentLogging = false;
  FLogFileName = GetDefaultLogFileName();
  FPermanentLogFileName = FLogFileName;
  FLogFileAppend = true;
  FLogWindowLines = 100;
  FLogProtocol = 0;
  UpdateActualLogProtocol();
  FLogActions = false;
  FPermanentLogActions = false;
  FActionsLogFileName = L"%TEMP%\\!S.xml";
  FPermanentActionsLogFileName = FActionsLogFileName;

  Changed();
}
//---------------------------------------------------------------------------
/* __fastcall */ TConfiguration::~TConfiguration()
{
  assert(!FUpdating);
  if (FApplicationInfo) { FreeFileInfo(FApplicationInfo); }
  delete FCriticalSection;
  // delete FUsage;
}
//---------------------------------------------------------------------------
THierarchicalStorage * TConfiguration::CreateScpStorage(bool /*SessionList*/)
{
  CALLSTACK;
  if (GetStorage() == stRegistry)
  {
    return new TRegistryStorage(GetRegistryStorageKey());
  }
#ifndef _MSC_VER
  else if (GetStorage() == stNul)
  {
    return new TIniFileStorage(L"nul");
  }
#endif
  else
  {
    Classes::Error(SNotImplemented, 3005);
    return NULL; // new TIniFileStorage(GetIniFileStorageName());
  }
}
//---------------------------------------------------------------------------
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>") + 1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) TRY_FINALLY1 (Storage, { BLOCK }, { Storage->CloseSubKey(); } );
#define KEY(TYPE, NAME) KEYEX(TYPE, NAME, NAME)
#undef REGCONFIG
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(String,   RandomSeedFile); \
    KEY(String,   PuttyRegistryStorageKey); \
    KEY(Bool,     ConfirmOverwriting); \
    KEY(Bool,     ConfirmResume); \
    KEY(Bool,     AutoReadDirectoryAfterOp); \
    KEY(Integer,  SessionReopenAuto); \
    KEY(Integer,  SessionReopenBackground); \
    KEY(Integer,  SessionReopenTimeout); \
    KEY(Integer,  SessionReopenAutoStall); \
    KEY(Integer,  TunnelLocalPortNumberLow); \
    KEY(Integer,  TunnelLocalPortNumberHigh); \
    KEY(Integer,  CacheDirectoryChangesMaxSize); \
    KEY(Bool,     ShowFtpWelcomeMessage); \
    KEY(String,   ExternalIpAddress); \
    KEY(Bool,     CollectUsage); \
    KEY(Integer,  SessionReopenAutoMaximumNumberOfRetries); \
  ); \
  BLOCK(L"Logging", CANCREATE, \
    KEYEX(Bool,  Logging, Logging); \
    KEYEX(String,LogFileName, LogFileName); \
    KEY(Bool,    LogFileAppend); \
    KEY(Integer, LogWindowLines); \
    KEY(Integer, LogProtocol); \
    KEYEX(Bool,  LogActions, LogActions); \
    KEYEX(String,PermanentActionsLogFileName, ActionsLogFileName); \
  );
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
{
  #define KEYEX(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(TEXT(#NAME))), Get ## VAR())
  REGCONFIG(true);
  #undef KEYEX

  if (Storage->OpenSubKey(L"Usage", true))
  {
    // FUsage->Save(Storage);
    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Save(bool All, bool Explicit)
{
  if (FDontSave) { return; }

  THierarchicalStorage * AStorage = CreateScpStorage(false);
  std::auto_ptr<THierarchicalStorage> AStoragePtr(AStorage);
  {
    if (AStorage)
    {
      AStorage->SetAccessMode(smReadWrite);
      AStorage->SetExplicit(Explicit);
      if (AStorage->OpenSubKey(GetConfigurationSubKey(), true))
      {
        SaveData(AStorage, All);
      }
    }
  }

  Saved();

  if (All)
  {
    StoredSessions->Save(true, Explicit);
  }

  // clean up as last, so that if it fails (read only INI), the saving can proceed
  if (GetStorage() == stRegistry)
  {
    CleanupIniFile();
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Export(const UnicodeString FileName)
{
  Classes::Error(SNotImplemented, 3004);
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  std::auto_ptr<THierarchicalStorage> StoragePtr(NULL);
  std::auto_ptr<THierarchicalStorage> ExportStoragePtr(NULL);
  {
    ExportStorage = NULL; // new TIniFileStorage(FileName);
    ExportStorage->SetAccessMode(smReadWrite);
    ExportStorage->SetExplicit(true);
    ExportStoragePtr.reset(ExportStorage);

    Storage = CreateScpStorage(false);
    Storage->SetAccessMode(smRead);
    StoragePtr.reset(Storage);

    CopyData(Storage, ExportStorage);

    if (ExportStorage->OpenSubKey(GetConfigurationSubKey(), true))
    {
      SaveData(ExportStorage, true);
    }
  }

  StoredSessions->Export(FileName);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::LoadData(THierarchicalStorage * Storage)
{
  CALLSTACK;
  #define KEYEX(TYPE, NAME, VAR) Set ## VAR(Storage->Read ## TYPE(LASTELEM(UnicodeString(TEXT(#NAME))), Get ## VAR()))
  #pragma warn -eas
  REGCONFIG(false);
  #pragma warn +eas
  #undef KEYEX

  if (Storage->OpenSubKey(L"Usage", false))
  {
    // FUsage->Load(Storage);
    Storage->CloseSubKey();
  }

  if (FPermanentLogActions && FPermanentActionsLogFileName.IsEmpty() &&
      FPermanentLogging && !FPermanentLogFileName.IsEmpty())
  {
     FPermanentActionsLogFileName = FPermanentLogFileName;
     FPermanentLogging = false;
     FPermanentLogFileName = L"";
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  CALLSTACK;
  FDisablePasswordStoring = Storage->ReadBool(L"DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->ReadBool(L"ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->ReadBool(L"DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
  FDefaultCollectUsage = Storage->ReadBool(L"DefaultCollectUsage", FDefaultCollectUsage);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Load()
{
  CALLSTACK;
  TGuard Guard(FCriticalSection);

  THierarchicalStorage * Storage = CreateScpStorage(false);
  std::auto_ptr<THierarchicalStorage> StoragePtr(Storage);
  {
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false))
    {
      TRACE("1");
      LoadData(Storage);
    }
    TRACE("2");
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  TStrings * Names = new TStringList();
  std::auto_ptr<TStrings> NamesPtr(Names);
  {
    if (Source->OpenSubKey(GetConfigurationSubKey(), false))
    {
      if (Target->OpenSubKey(GetConfigurationSubKey(), true))
      {
        if (Source->OpenSubKey(L"CDCache", false))
        {
          if (Target->OpenSubKey(L"CDCache", true))
          {
            Names->Clear();
            Source->GetValueNames(Names);

            for (int Index = 0; Index < Names->Count; Index++)
            {
              Target->WriteBinaryData(Names->Strings[Index],
                Source->ReadBinaryData(Names->Strings[Index]));
            }

            Target->CloseSubKey();
          }
          Source->CloseSubKey();
        }

        if (Source->OpenSubKey(L"Banners", false))
        {
          if (Target->OpenSubKey(L"Banners", true))
          {
            Names->Clear();
            Source->GetValueNames(Names);

            for (int Index = 0; Index < Names->Count; Index++)
            {
              Target->WriteString(Names->Strings[Index],
                Source->ReadString(Names->Strings[Index], L""));
            }

            Target->CloseSubKey();
          }
          Source->CloseSubKey();
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }

    if (Source->OpenSubKey(GetSshHostKeysSubKey(), false))
    {
      if (Target->OpenSubKey(GetSshHostKeysSubKey(), true))
      {
        Names->Clear();
        Source->GetValueNames(Names);

        for (int Index = 0; Index < Names->Count; Index++)
        {
          Target->WriteStringRaw(Names->Strings[Index],
            Source->ReadStringRaw(Names->Strings[Index], L""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::LoadDirectoryChangesCache(const UnicodeString SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  CALLSTACK;
  THierarchicalStorage * Storage = CreateScpStorage(false);
  std::auto_ptr<THierarchicalStorage> StoragePtr(Storage);
  {
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
        Storage->OpenSubKey(L"CDCache", false) &&
        Storage->ValueExists(SessionKey))
    {
      DirectoryChangesCache->Deserialize(Storage->ReadBinaryData(SessionKey));
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SaveDirectoryChangesCache(const UnicodeString SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  std::auto_ptr<THierarchicalStorage> StoragePtr(Storage);
  {
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(L"CDCache", true))
    {
      UnicodeString Data;
      DirectoryChangesCache->Serialize(Data);
      Storage->WriteBinaryData(SessionKey, Data);
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::BannerHash(const UnicodeString & Banner)
{
  RawByteString Result;
  Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char*>(Banner.c_str()), Banner.Length() * sizeof(wchar_t),
    (unsigned char*)Result.c_str());
  return BytesToHex(Result);
}
//---------------------------------------------------------------------------
bool __fastcall TConfiguration::ShowBanner(const UnicodeString SessionKey,
  const UnicodeString & Banner)
{
  bool Result;
  THierarchicalStorage * Storage = CreateScpStorage(false);
  std::auto_ptr<THierarchicalStorage> StoragePtr(Storage);
  {
    Storage->SetAccessMode(smRead);
    Result =
      !Storage->OpenSubKey(GetConfigurationSubKey(), false) ||
      !Storage->OpenSubKey(L"Banners", false) ||
      !Storage->ValueExists(SessionKey) ||
      (Storage->ReadString(SessionKey, L"") != BannerHash(Banner));
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::NeverShowBanner(const UnicodeString SessionKey,
  const UnicodeString & Banner)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  std::auto_ptr<THierarchicalStorage> StoragePtr(Storage);
  {
    Storage->SetAccessMode(smReadWrite);

    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(L"Banners", true))
    {
      Storage->WriteString(SessionKey, BannerHash(Banner));
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Changed()
{
  if (FUpdating == 0)
  {
    if (GetOnChange())
    {
      GetOnChange()(this);
    }
  }
  else
  {
    FChanged = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::BeginUpdate()
{
  if (FUpdating == 0)
  {
    FChanged = false;
  }
  FUpdating++;
  // Greater value would probably indicate some nesting problem in code
  assert(FUpdating < 6);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::EndUpdate()
{
  assert(FUpdating > 0);
  FUpdating--;
  if ((FUpdating == 0) && FChanged)
  {
    FChanged = false;
    Changed();
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CleanupConfiguration()
{
  try
  {
    CleanupRegistry(GetConfigurationSubKey());
    if (GetStorage() == stRegistry)
    {
      FDontSave = true;
    }
  }
  catch (Exception &E)
  {
    throw ExtException(&E, CLEANUP_CONFIG_ERROR);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CleanupRegistry(UnicodeString CleanupSubKey)
{
  TRegistryStorage *Registry = new TRegistryStorage(GetRegistryStorageKey());
  std::auto_ptr<TRegistryStorage> RegistryPtr(Registry);
  {
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CleanupHostKeys()
{
  try
  {
    CleanupRegistry(GetSshHostKeysSubKey());
  }
  catch (Exception &E)
  {
    throw ExtException(&E, CLEANUP_HOSTKEYS_ERROR);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CleanupRandomSeedFile()
{
  try
  {
    DontSaveRandomSeed();
    if (FileExists(GetRandomSeedFileName()))
    {
      if (!DeleteFile(GetRandomSeedFileName()))
      {
        RaiseLastOSError();
      }
    }
  }
  catch (Exception &E)
  {
    throw ExtException(&E, CLEANUP_SEEDFILE_ERROR);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CleanupIniFile()
{
  try
  {
    if (FileExists(GetIniFileStorageName()))
    {
      if (!DeleteFile(GetIniFileStorageName()))
      {
        RaiseLastOSError();
      }
    }
#if 0
    if (GetStorage() == stIniFile)
    {
      FDontSave = true;
    }
#endif
  }
  catch (Exception &E)
  {
    throw ExtException(&E, CLEANUP_INIFILE_ERROR);
  }
}
//---------------------------------------------------------------------------
RawByteString __fastcall TConfiguration::EncryptPassword(UnicodeString Password, UnicodeString Key)
{
  if (Password.IsEmpty())
  {
    return RawByteString();
  }
  else
  {
    return ::EncryptPassword(Password, Key);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::DecryptPassword(RawByteString Password, UnicodeString Key)
{
  if (Password.IsEmpty())
  {
    return UnicodeString();
  }
  else
  {
    return ::DecryptPassword(Password, Key);
  }
}
//---------------------------------------------------------------------------
RawByteString __fastcall TConfiguration::StronglyRecryptPassword(RawByteString Password, UnicodeString /*Key*/)
{
  return Password;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetOSVersionStr()
{
  UnicodeString Result;
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (GetVersionEx(&OSVersionInfo) != 0)
  {
    Result = FORMAT(L"%d.%d.%d %s", int(OSVersionInfo.dwMajorVersion),
      int(OSVersionInfo.dwMinorVersion), int(OSVersionInfo.dwBuildNumber),
      OSVersionInfo.szCSDVersion).Trim();
  }
  return Result;
}
//---------------------------------------------------------------------------
TVSFixedFileInfo *__fastcall TConfiguration::GetFixedApplicationInfo()
{
  return GetFixedFileInfo(GetApplicationInfo());
}
//---------------------------------------------------------------------------
int __fastcall TConfiguration::GetCompoundVersion()
{
  TVSFixedFileInfo * FileInfo = GetFixedApplicationInfo();
  return CalculateCompoundVersion(
    HIWORD(FileInfo->dwFileVersionMS), LOWORD(FileInfo->dwFileVersionMS),
    HIWORD(FileInfo->dwFileVersionLS), LOWORD(FileInfo->dwFileVersionLS));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::ModuleFileName()
{
  CALLSTACK;
  // TRACEFMT("[%s]", (ParamStr(0)));
  Classes::Error(SNotImplemented, 204);
  return L""; // FIXME ParamStr(0);
}
//---------------------------------------------------------------------------
void * __fastcall TConfiguration::GetFileApplicationInfo(const UnicodeString FileName)
{
  CCALLSTACK(TRACE_FILE_APPL_INFO);
  void * Result;
  if (FileName.IsEmpty())
  {
    CTRACE(TRACE_FILE_APPL_INFO, "1");
    if (!FApplicationInfo)
    {
      CTRACE(TRACE_FILE_APPL_INFO, "2");
      FApplicationInfo = CreateFileInfo(ModuleFileName());
    }
    CTRACE(TRACE_FILE_APPL_INFO, "3");
    Result = FApplicationInfo;
  }
  else
  {
    CTRACE(TRACE_FILE_APPL_INFO, "4");
    Result = CreateFileInfo(FileName);
  }
  return Result;
}
//---------------------------------------------------------------------------
void * __fastcall TConfiguration::GetApplicationInfo()
{
  CCALLSTACK(TRACE_FILE_APPL_INFO);
  return GetFileApplicationInfo("");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetFileProductName(const UnicodeString FileName)
{
  return GetFileFileInfoString(L"ProductName", FileName);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetFileCompanyName(const UnicodeString FileName)
{
  return GetFileFileInfoString(L"CompanyName", FileName);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetProductName()
{
  return GetFileProductName(L"");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetCompanyName()
{
  return GetFileCompanyName(L"");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetFileProductVersion(const UnicodeString FileName)
{
  CALLSTACK;
  return TrimVersion(GetFileFileInfoString(L"ProductVersion", FileName));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetProductVersion()
{
  return GetFileProductVersion(L"");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::TrimVersion(UnicodeString Version)
{
  while ((Version.Pos(L".") != Version.LastDelimiter(L".")) &&
    (Version.SubString(Version.Length() - 1, 2) == L".0"))
  {
    Version.SetLength(Version.Length() - 2);
  }
  return Version;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetVersionStr()
{
  TGuard Guard(FCriticalSection);
  try
  {
    TVSFixedFileInfo * Info = GetFixedApplicationInfo();
    return FMTLOAD(VERSION,
      HIWORD(Info->dwFileVersionMS),
      LOWORD(Info->dwFileVersionMS),
      HIWORD(Info->dwFileVersionLS),
      LOWORD(Info->dwFileVersionLS));
  }
  catch (Exception &E)
  {
    throw ExtException(&E, L"Can't get application version");
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetVersion()
{
  TGuard Guard(FCriticalSection);
  try
  {
    TVSFixedFileInfo * Info = GetFixedApplicationInfo();
    UnicodeString Result;
    Result = FORMAT(L"%d.%d.%d",
      HIWORD(Info->dwFileVersionMS),
      LOWORD(Info->dwFileVersionMS),
      HIWORD(Info->dwFileVersionLS));
    return Result;
  }
  catch (Exception &E)
  {
    throw ExtException(&E, L"Can't get application version");
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetFileFileInfoString(const UnicodeString Key,
  const UnicodeString FileName)
{
  CALLSTACK;
  TGuard Guard(FCriticalSection);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(FileName);
  TRY_FINALLY2 (FileName, Info,
  {
    if ((Info != NULL) && (GetTranslationCount(Info) > 0))
    {
      TRACE("1");
      TTranslation Translation;
      Translation = GetTranslation(Info, 0);
      try
      {
        Result = ::GetFileInfoString(Info, Translation, Key);
      }
      catch (const std::exception & e)
      {
        DEBUG_PRINTF(L"Error: %s", MB2W(e.what()).c_str());
        Result = L"";
      }
    }
    else
    {
      assert(!FileName.IsEmpty());
    }
  }
  ,
  {
    TRACE("2");
    if (!FileName.IsEmpty())
    {
      FreeFileInfo(Info);
    }
  }
  );
  TRACEFMT("3 [%s] [%s] [%s]", (Key, FileName, Result));
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetFileInfoString(const UnicodeString Key)
{
  return GetFileFileInfoString(Key, L"");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetRegistryStorageKey()
{
  return GetRegistryKey();
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetNulStorage()
{
  CALLSTACK;
  FStorage = stNul;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetDefaultStorage()
{
  CALLSTACK;
  FStorage = stDetect;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetIniFileStorageName(UnicodeString value)
{
  CALLSTACK;
  Classes::Error(SNotImplemented, 3006);
  FIniFileStorageName = value;
  // FStorage = stIniFile;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetIniFileStorageName()
{
  if (FIniFileStorageName.IsEmpty())
  {
    UnicodeString IniPath = L""; // ChangeFileExt(ParamStr(0), L".ini");
    /*
    if (FVirtualIniFileStorageName.IsEmpty() &&
        TPath::IsDriveRooted(IniPath))
    {
      UnicodeString LocalAppDataPath = GetShellFolderPath(CSIDL_LOCAL_APPDATA);
      // virtual store for non-system drives have a different virtual store,
      // do not bother about them
      if (TPath::IsDriveRooted(LocalAppDataPath) &&
          SameText(ExtractFileDrive(IniPath), ExtractFileDrive(LocalAppDataPath)))
      {
        FVirtualIniFileStorageName =
          IncludeTrailingBackslash(LocalAppDataPath) +
          L"VirtualStore\\" +
          IniPath.SubString(4, IniPath.Length() - 3);
      }
    }

    if (!FVirtualIniFileStorageName.IsEmpty() &&
        FileExists(FVirtualIniFileStorageName))
    {
      return FVirtualIniFileStorageName;
    }
    else*/
    {
      return IniPath;
    }
  }
  else
  {
    return FIniFileStorageName;
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetPuttySessionsKey()
{
  return GetPuttyRegistryStorageKey() + L"\\Sessions";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetStoredSessionsSubKey()
{
  return L"Sessions";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetSshHostKeysSubKey()
{
  return L"SshHostKeys";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetConfigurationSubKey()
{
  return L"Configuration";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetRootKeyStr()
{
  return RootKeyToStr(HKEY_CURRENT_USER);
}
//---------------------------------------------------------------------------
bool __fastcall TConfiguration::GetGSSAPIInstalled()
{
  return HasGSSAPI();
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetStorage(TStorage value)
{
  if (FStorage != value)
  {
    THierarchicalStorage * SourceStorage = NULL;
    THierarchicalStorage * TargetStorage = NULL;

    std::auto_ptr<THierarchicalStorage> SourceStoragePtr(NULL);
    std::auto_ptr<THierarchicalStorage> TargetStoragePtr(NULL);
    {
      SourceStorage = CreateScpStorage(false);
      SourceStorage->SetAccessMode(smRead);
      SourceStoragePtr.reset(SourceStorage);

      FStorage = value;

      TargetStorage = CreateScpStorage(false);
      TargetStorage->SetAccessMode(smReadWrite);
      TargetStorage->SetExplicit(true);
      TargetStoragePtr.reset(TargetStorage);

      // copy before save as it removes the ini file,
      // when switching from ini to registry
      CopyData(SourceStorage, TargetStorage);
    }

    // save all and explicit
    Save(true, true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Saved()
{
  // nothing
}
//---------------------------------------------------------------------------
TStorage __fastcall TConfiguration::GetStorage()
{
  CALLSTACK;
  if (FStorage == stDetect)
  {
    TRACEFMT("1 [%s]", (GetIniFileStorageName()));
    /* if (FileExists(IniFileStorageName))
    {
      TRACE("2");
      FStorage = stIniFile;
    }
    else*/
    {
      TRACE("3");
      FStorage = stRegistry;
    }
  }
  return FStorage;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetRandomSeedFile(UnicodeString value)
{
  if (GetRandomSeedFile() != value)
  {
    UnicodeString PrevRandomSeedFileName = GetRandomSeedFileName();

    FRandomSeedFile = value;

    // never allow empty seed file to avoid Putty trying to reinitialize the path
    if (GetRandomSeedFileName().IsEmpty())
    {
      FRandomSeedFile = FDefaultRandomSeedFile;
    }

    if (!PrevRandomSeedFileName.IsEmpty() &&
        (PrevRandomSeedFileName != GetRandomSeedFileName()) &&
        FileExists(PrevRandomSeedFileName))
    {
      // ignore any error
      DeleteFile(PrevRandomSeedFileName);
    }
  }
}
//---------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetRandomSeedFileName()
{
  return StripPathQuotes(ExpandEnvironmentVariables(FRandomSeedFile)).Trim();
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetExternalIpAddress(UnicodeString value)
{
  SET_CONFIG_PROPERTY(ExternalIpAddress);
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetPuttyRegistryStorageKey(UnicodeString value)
{
  SET_CONFIG_PROPERTY(PuttyRegistryStorageKey);
}
//---------------------------------------------------------------------------
TEOLType __fastcall TConfiguration::GetLocalEOLType()
{
  return eolCRLF;
}
//---------------------------------------------------------------------
bool __fastcall TConfiguration::GetCollectUsage()
{
  return false; // FUsage->Collect;
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetCollectUsage(bool value)
{
  // FUsage->Collect = value;
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::TemporaryLogging(const UnicodeString ALogFileName)
{
  if (SameText(ExtractFileExt(ALogFileName), L".xml"))
  {
    TemporaryActionsLogging(ALogFileName);
  }
  else
  {
    FLogging = true;
    FLogFileName = ALogFileName;
    UpdateActualLogProtocol();
  }
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::TemporaryActionsLogging(const UnicodeString ALogFileName)
{
  FLogActions = true;
  FActionsLogFileName = ALogFileName;
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogging(bool value)
{
  if (GetLogging() != value)
  {
    FPermanentLogging = value;
    FLogging = value;
    UpdateActualLogProtocol();
    Changed();
  }
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogFileName(UnicodeString value)
{
  if (GetLogFileName() != value)
  {
    FPermanentLogFileName = value;
    FLogFileName = value;
    Changed();
  }
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetActionsLogFileName(UnicodeString value)
{
  if (GetActionsLogFileName() != value)
  {
    FPermanentActionsLogFileName = value;
    FActionsLogFileName = value;
    Changed();
  }
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogToFile(bool value)
{
  if (value != GetLogToFile())
  {
    SetLogFileName(value ? GetDefaultLogFileName() : UnicodeString(L""));
    Changed();
  }
}
//---------------------------------------------------------------------
bool __fastcall TConfiguration::GetLogToFile()
{
  return !GetLogFileName().IsEmpty();
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::UpdateActualLogProtocol()
{
  FActualLogProtocol = FLogging ? FLogProtocol : 0;
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogProtocol(int value)
{
  SET_CONFIG_PROPERTY(LogProtocol);
  UpdateActualLogProtocol();
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogActions(bool value)
{
  if (GetLogActions() != value)
  {
    FPermanentLogActions = value;
    FLogActions = value;
    Changed();
  }
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogFileAppend(bool value)
{
  SET_CONFIG_PROPERTY(LogFileAppend);
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogWindowLines(int value)
{
  SET_CONFIG_PROPERTY(LogWindowLines);
}
//---------------------------------------------------------------------
void __fastcall TConfiguration::SetLogWindowComplete(bool value)
{
  if (value != GetLogWindowComplete())
  {
    SetLogWindowLines(value ? 0 : 50);
    Changed();
  }
}
//---------------------------------------------------------------------
bool __fastcall TConfiguration::GetLogWindowComplete()
{
  return static_cast<bool>(GetLogWindowLines() == 0);
}
//---------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetDefaultLogFileName()
{
  // return IncludeTrailingBackslash(SystemTemporaryDirectory()) + L"winscp.log";
  return L"%TEMP%\\&S.log";
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetConfirmOverwriting(bool value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmOverwriting);
}
//---------------------------------------------------------------------------
bool __fastcall TConfiguration::GetConfirmOverwriting()
{
  TGuard Guard(FCriticalSection);
  return FConfirmOverwriting;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetConfirmResume(bool value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmResume);
}
//---------------------------------------------------------------------------
bool __fastcall TConfiguration::GetConfirmResume()
{
  TGuard Guard(FCriticalSection);
  return FConfirmResume;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetAutoReadDirectoryAfterOp(bool value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(AutoReadDirectoryAfterOp);
}
//---------------------------------------------------------------------------
bool __fastcall TConfiguration::GetAutoReadDirectoryAfterOp()
{
  TGuard Guard(FCriticalSection);
  return FAutoReadDirectoryAfterOp;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetTimeFormat()
{
  return L"h:nn:ss";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetPartialExt() const
{
  return PARTIAL_EXT;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetDefaultKeyFile()
{
  return L"";
}
//---------------------------------------------------------------------------
bool __fastcall TConfiguration::GetRememberPassword()
{
  return false;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetSessionReopenAuto(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetSessionReopenAutoMaximumNumberOfRetries(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoMaximumNumberOfRetries);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetSessionReopenBackground(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetSessionReopenTimeout(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenTimeout);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetSessionReopenAutoStall(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoStall);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetTunnelLocalPortNumberLow(int value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetTunnelLocalPortNumberHigh(int value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberHigh);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetCacheDirectoryChangesMaxSize(int value)
{
  SET_CONFIG_PROPERTY(CacheDirectoryChangesMaxSize);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetShowFtpWelcomeMessage(bool value)
{
  SET_CONFIG_PROPERTY(ShowFtpWelcomeMessage);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetPermanentLogFileName()
{
  return FPermanentLogFileName;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetPermanentLogFileName(const UnicodeString value)
{
  FPermanentLogFileName = value;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetPermanentActionsLogFileName()
{
  return FPermanentActionsLogFileName;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetPermanentActionsLogFileName(const UnicodeString value)
{
  FPermanentActionsLogFileName = value;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TShortCuts::Add(TShortCut ShortCut)
{
  FShortCuts.insert(ShortCut);
}
//---------------------------------------------------------------------------
bool __fastcall TShortCuts::Has(TShortCut ShortCut) const
{
  return (FShortCuts.count(ShortCut) != 0);
}

