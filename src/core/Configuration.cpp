//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <shlobj.h>
#include <FileInfo.h>

#include "Exceptions.h"
#include "Common.h"
#include "Configuration.h"
#include "PuttyIntf.h"
#include "TextsCore.h"
#include "Interface.h"
#include "CoreMain.h"
#include "WinSCPSecurity.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TConfiguration::TConfiguration() :
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
  FProgramIniPathWrittable(0),
  FTunnelLocalPortNumberLow(0),
  FTunnelLocalPortNumberHigh(0),
  FCacheDirectoryChangesMaxSize(0),
  FShowFtpWelcomeMessage(false),
  FTryFtpWhenSshFails(false),
  FDisablePasswordStoring(false),
  FForceBanners(false),
  FDisableAcceptingHostKeys(false),
  FDefaultCollectUsage(false),
  FSessionReopenAutoMaximumNumberOfRetries(0),
  FCriticalSection(NULL)
{
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
void TConfiguration::Default()
{
  TGuard Guard(FCriticalSection);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  TRegistryStorage * AdminStorage = new TRegistryStorage(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE);
  TRY_FINALLY (
  {
    if (AdminStorage->OpenRootKey(false))
    {
      LoadAdmin(AdminStorage);
      AdminStorage->CloseSubKey();
    }
  }
  ,
  {
    delete AdminStorage;
  }
  );

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
  FTryFtpWhenSshFails = true;
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
  FActionsLogFileName = L"%TEMP%\\&S.xml";
  FPermanentActionsLogFileName = FActionsLogFileName;
  FProgramIniPathWrittable = -1;

  Changed();
}
//---------------------------------------------------------------------------
TConfiguration::~TConfiguration()
{
  assert(!FUpdating);
  if (FApplicationInfo)
  {
    FreeFileInfo(FApplicationInfo);
  }
  delete FCriticalSection;
  // delete FUsage;
}
//---------------------------------------------------------------------------
THierarchicalStorage * TConfiguration::CreateStorage(bool /*SessionList*/)
{
  if (GetStorage() == stRegistry)
  {
    return new TRegistryStorage(GetRegistryStorageKey());
  }
#if defined(__BORLANDC__)
  else if (GetStorage() == stNul)
  {
    return new TIniFileStorage(L"nul");
  }
#endif
  else
  {
    Classes::Error(SNotImplemented, 3005);
  }
  assert(false);
  return NULL;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::PropertyToKey(const UnicodeString & Property)
{
  // no longer useful
  int P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}
//---------------------------------------------------------------------------
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>") + 1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) TRY_FINALLY ( { BLOCK }, { Storage->CloseSubKey(); } );
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
    KEY(Bool,     TryFtpWhenSshFails); \
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
void TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
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
void TConfiguration::Save(bool All, bool Explicit)
{
  if (FDontSave)
  {
    return;
  }

  THierarchicalStorage * AStorage = CreateStorage(false);
  TRY_FINALLY (
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
  ,
  {
    delete AStorage;
  }
  );

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
void TConfiguration::Export(const UnicodeString & FileName)
{
  Classes::Error(SNotImplemented, 3004);
  /*
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  TRY_FINALLY (
  {
    ExportStorage = NULL; // new TIniFileStorage(FileName);
    ExportStorage->SetAccessMode(smReadWrite);
    ExportStorage->SetExplicit(true);

    Storage = CreateScpStorage(false);
    Storage->SetAccessMode(smRead);

    CopyData(Storage, ExportStorage);

    if (ExportStorage->OpenSubKey(GetConfigurationSubKey(), true))
    {
      SaveData(ExportStorage, true);
    }
  }
  ,
  {
    delete ExportStorage;
    delete Storage;
  }
  );

  StoredSessions->Export(FileName);
  */
}
//---------------------------------------------------------------------------
void TConfiguration::Import(const UnicodeString & FileName)
{
  Classes::Error(SNotImplemented, 3005);
/*
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ImportStorage = NULL;
  try
  {
    ImportStorage = new TIniFileStorage(FileName);
    ImportStorage->AccessMode = smRead;

    Storage = CreateScpStorage(false);
    Storage->AccessMode = smReadWrite;
    Storage->Explicit = true;

    CopyData(ImportStorage, Storage);

    Default();
    LoadFrom(ImportStorage);

    if (ImportStorage->OpenSubKey(Configuration->StoredSessionsSubKey, false))
    {
      StoredSessions->Clear();
      StoredSessions->DefaultSettings->Default();
      StoredSessions->Load(ImportStorage);
    }
  }
  __finally
  {
    delete ImportStorage;
    delete Storage;
  }

  // save all and explicit
  Save(true, true);
*/
}
//---------------------------------------------------------------------------
void TConfiguration::LoadData(THierarchicalStorage * Storage)
{
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
}
//---------------------------------------------------------------------------
void TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  FDisablePasswordStoring = Storage->ReadBool(L"DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->ReadBool(L"ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->ReadBool(L"DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
  FDefaultCollectUsage = Storage->ReadBool(L"DefaultCollectUsage", FDefaultCollectUsage);
}
//---------------------------------------------------------------------------
void TConfiguration::LoadFrom(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetConfigurationSubKey(), false))
  {
    LoadData(Storage);
    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void TConfiguration::Load()
{
  TGuard Guard(FCriticalSection);

  THierarchicalStorage * Storage = CreateStorage(false);
  TRY_FINALLY (
  {
    Storage->SetAccessMode(smRead);
    LoadFrom(Storage);
  }
  ,
  {
    delete Storage;
  }
  );
}
//---------------------------------------------------------------------------
void TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  TStrings * Names = new TStringList();
  TRY_FINALLY (
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

            for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
            {
              Target->WriteBinaryData(Names->GetString(Index),
                Source->ReadBinaryData(Names->GetString(Index)));
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

            for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
            {
              Target->WriteString(Names->GetString(Index),
                Source->ReadString(Names->GetString(Index), L""));
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

        for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
        {
          Target->WriteStringRaw(Names->GetString(Index),
            Source->ReadStringRaw(Names->GetString(Index), L""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
  ,
  {
    delete Names;
  }
  );
}
//---------------------------------------------------------------------------
void TConfiguration::LoadDirectoryChangesCache(const UnicodeString & SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateStorage(false);
  TRY_FINALLY (
  {
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
        Storage->OpenSubKey(L"CDCache", false) &&
        Storage->ValueExists(SessionKey))
    {
      DirectoryChangesCache->Deserialize(Storage->ReadBinaryData(SessionKey));
    }
  }
  ,
  {
    delete Storage;
  }
  );
}
//---------------------------------------------------------------------------
void TConfiguration::SaveDirectoryChangesCache(const UnicodeString & SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateStorage(false);
  TRY_FINALLY (
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
  ,
  {
    delete Storage;
  }
  );
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::BannerHash(const UnicodeString & Banner)
{
  RawByteString Result;
  Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(Banner.c_str()), static_cast<int>(Banner.Length() * sizeof(wchar_t)),
    reinterpret_cast<unsigned char *>(const_cast<char *>(Result.c_str())));
  return BytesToHex(Result);
}
//---------------------------------------------------------------------------
bool TConfiguration::ShowBanner(const UnicodeString & SessionKey,
  const UnicodeString & Banner)
{
  bool Result;
  THierarchicalStorage * Storage = CreateStorage(false);
  TRY_FINALLY (
  {
    Storage->SetAccessMode(smRead);
    Result =
      !Storage->OpenSubKey(GetConfigurationSubKey(), false) ||
      !Storage->OpenSubKey(L"Banners", false) ||
      !Storage->ValueExists(SessionKey) ||
      (Storage->ReadString(SessionKey, L"") != BannerHash(Banner));
  }
  ,
  {
    delete Storage;
  }
  );

  return Result;
}
//---------------------------------------------------------------------------
void TConfiguration::NeverShowBanner(const UnicodeString & SessionKey,
  const UnicodeString & Banner)
{
  THierarchicalStorage * Storage = CreateStorage(false);
  TRY_FINALLY (
  {
    Storage->SetAccessMode(smReadWrite);

    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(L"Banners", true))
    {
      Storage->WriteString(SessionKey, BannerHash(Banner));
    }
  }
  ,
  {
    delete Storage;
  }
  );
}
//---------------------------------------------------------------------------
void TConfiguration::Changed()
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
void TConfiguration::BeginUpdate()
{
  if (FUpdating == 0)
  {
    FChanged = false;
  }
  FUpdating++;
  // Greater Value would probably indicate some nesting problem in code
  assert(FUpdating < 6);
}
//---------------------------------------------------------------------------
void TConfiguration::EndUpdate()
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
void TConfiguration::CleanupConfiguration()
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
void TConfiguration::CleanupRegistry(const UnicodeString & CleanupSubKey)
{
  TRegistryStorage *Registry = new TRegistryStorage(GetRegistryStorageKey());
  TRY_FINALLY (
  {
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
  ,
  {
    delete Registry;
  }
  );
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupHostKeys()
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
void TConfiguration::CleanupRandomSeedFile()
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
void TConfiguration::CleanupIniFile()
{
  try
  {
#if 0
    if (FileExists(GetIniFileStorageNameForReading()))
    {
      if (!DeleteFile(GetIniFileStorageNameForReading()))
      {
        RaiseLastOSError();
      }
    }
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
RawByteString TConfiguration::EncryptPassword(const UnicodeString & Password, const UnicodeString & Key)
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
UnicodeString TConfiguration::DecryptPassword(const RawByteString & Password, const UnicodeString & Key)
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
RawByteString TConfiguration::StronglyRecryptPassword(const RawByteString & Password, const UnicodeString & /*Key*/)
{
  return Password;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetOSVersionStr() const
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
TVSFixedFileInfo * TConfiguration::GetFixedApplicationInfo() const
{
  return GetFixedFileInfo(GetApplicationInfo());
}
//---------------------------------------------------------------------------
intptr_t TConfiguration::GetCompoundVersion() const
{
  TVSFixedFileInfo * FileInfo = GetFixedApplicationInfo();
  if (FileInfo)
    return CalculateCompoundVersion(
      HIWORD(FileInfo->dwFileVersionMS), LOWORD(FileInfo->dwFileVersionMS),
      HIWORD(FileInfo->dwFileVersionLS), LOWORD(FileInfo->dwFileVersionLS));
  else
    return 0;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::ModuleFileName() const
{
#if defined(__BORLANDC__)
  return ParamStr(0);
#endif
  Classes::Error(SNotImplemented, 204);
  return L"";
}
//---------------------------------------------------------------------------
void * TConfiguration::GetFileApplicationInfo(const UnicodeString & FileName) const
{
  void * Result;
  if (FileName.IsEmpty())
  {
    if (!FApplicationInfo)
    {
      FApplicationInfo = CreateFileInfo(ModuleFileName());
    }
    Result = FApplicationInfo;
  }
  else
  {
    Result = CreateFileInfo(FileName);
  }
  return Result;
}
//---------------------------------------------------------------------------
void * TConfiguration::GetApplicationInfo() const
{
  return GetFileApplicationInfo("");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileProductName(const UnicodeString & FileName) const
{
  return GetFileInfoString(L"ProductName", FileName);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileCompanyName(const UnicodeString & FileName) const
{
  return GetFileInfoString(L"CompanyName", FileName);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetProductName() const
{
  return GetFileProductName(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetCompanyName() const
{
  return GetFileCompanyName(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileProductVersion(const UnicodeString & FileName) const
{
  return TrimVersion(GetFileInfoString(L"ProductVersion", FileName));
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileDescription(const UnicodeString & FileName)
{
  return GetFileInfoString(L"FileDescription", FileName);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetProductVersion() const
{
  return GetFileProductVersion(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::TrimVersion(const UnicodeString & Version) const
{
  UnicodeString Result = Version;
  while ((Result.Pos(L".") != Result.LastDelimiter(L".")) &&
    (Result.SubString(Result.Length() - 1, 2) == L".0"))
  {
    Result.SetLength(Result.Length() - 2);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetVersionStr() const
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
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetVersion() const
{
  TGuard Guard(FCriticalSection);
  UnicodeString Result;
  try
  {
    TVSFixedFileInfo * Info = GetFixedApplicationInfo();
    if (Info)
    {
      Result = FORMAT(L"%d.%d.%d",
        HIWORD(Info->dwFileVersionMS),
        LOWORD(Info->dwFileVersionMS),
        HIWORD(Info->dwFileVersionLS));
    }
  }
  catch (Exception &E)
  {
    throw ExtException(&E, L"Can't get application version");
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileInfoString(const UnicodeString & Key,
  const UnicodeString & FileName) const
{
  TGuard Guard(FCriticalSection);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(FileName);
  TRY_FINALLY (
  {
    if ((Info != NULL) && (GetTranslationCount(Info) > 0))
    {
      TTranslation Translation = GetTranslation(Info, 0);
      try
      {
        Result = ::GetFileInfoString(Info, Translation, Key);
      }
      catch (const std::exception & e)
      {
        (void)e;
        DEBUG_PRINTF(L"Error: %s", MB2W(e.what()).c_str());
        Result = L"";
      }
    }
  }
  ,
  {
    if (!FileName.IsEmpty() && Info)
    {
      FreeFileInfo(Info);
    }
  }
  );
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileInfoString(const UnicodeString & Key) const
{
  return GetFileInfoString(Key, L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetRegistryStorageKey() const
{
  return GetRegistryKey();
}
//---------------------------------------------------------------------------
void TConfiguration::SetNulStorage()
{
  FStorage = stNul;
}
//---------------------------------------------------------------------------
void TConfiguration::SetDefaultStorage()
{
  FStorage = stDetect;
}
//---------------------------------------------------------------------------
/*
void TConfiguration::SetIniFileStorageName(const UnicodeString & Value)
{
  FStorage = stIniFile;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetIniFileStorageNameForReading()
{
  return GetIniFileStorageName(true);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetIniFileStorageNameForReadingWritting()
{
  return GetIniFileStorageName(false);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetIniFileStorageName(bool ReadingOnly)
{
  if (FIniFileStorageName.IsEmpty())
  {
    UnicodeString ProgramPath = ParamStr(0);

    UnicodeString ProgramIniPath = ChangeFileExt(ProgramPath, L".ini");

    UnicodeString IniPath;
    if (FileExists(ProgramIniPath))
    {
      IniPath = ProgramIniPath;
    }
    else
    {
      UnicodeString AppDataIniPath =
        IncludeTrailingBackslash(GetShellFolderPath(CSIDL_APPDATA)) +
        ExtractFileName(ProgramIniPath);
      if (FileExists(AppDataIniPath))
      {
        IniPath = AppDataIniPath;
      }
      else
      {
        // avoid expensive test if we are interested in existing files only
        if (!ReadingOnly && (FProgramIniPathWrittable < 0))
        {
          UnicodeString ProgramDir = ExtractFilePath(ProgramPath);
          FProgramIniPathWrittable = IsDirectoryWriteable(ProgramDir) ? 1 : 0;
        }

        // does not really matter what we return when < 0
        IniPath = (FProgramIniPathWrittable == 0) ? AppDataIniPath : ProgramIniPath;
      }
    }

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
    else
    {
      return IniPath;
    }
  }
  else
  {
    return FIniFileStorageName;
  }
}
*/
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetPuttySessionsKey()
{
  return GetPuttyRegistryStorageKey() + L"\\Sessions";
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetStoredSessionsSubKey()
{
  return L"Sessions";
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetSshHostKeysSubKey()
{
  return L"SshHostKeys";
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetConfigurationSubKey()
{
  return L"Configuration";
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetRootKeyStr()
{
  return RootKeyToStr(HKEY_CURRENT_USER);
}
//---------------------------------------------------------------------------
bool TConfiguration::GetGSSAPIInstalled() const
{
  return HasGSSAPI();
}
//---------------------------------------------------------------------------
void TConfiguration::SetStorage(TStorage Value)
{
  if (FStorage != Value)
  {
    THierarchicalStorage * SourceStorage = NULL;
    THierarchicalStorage * TargetStorage = NULL;

    TRY_FINALLY (
    {
      SourceStorage = CreateStorage(false);
      SourceStorage->SetAccessMode(smRead);

      FStorage = Value;

      TargetStorage = CreateStorage(false);
      TargetStorage->SetAccessMode(smReadWrite);
      TargetStorage->SetExplicit(true);

      // copy before save as it removes the ini file,
      // when switching from ini to registry
      CopyData(SourceStorage, TargetStorage);
    }
    ,
    {
      delete SourceStorage;
      delete TargetStorage;
    }
    );

    // save all and explicit
    Save(true, true);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::Saved()
{
  // nothing
}
//---------------------------------------------------------------------------
TStorage TConfiguration::GetStorage()
{
  if (FStorage == stDetect)
  {
    /*if (FileExists(IniFileStorageNameForReading))
    {
      FStorage = stIniFile;
    }
    else*/
    {
      FStorage = stRegistry;
    }
  }
  return FStorage;
}
//---------------------------------------------------------------------------
void TConfiguration::SetRandomSeedFile(const UnicodeString & Value)
{
  if (GetRandomSeedFile() != Value)
  {
    UnicodeString PrevRandomSeedFileName = GetRandomSeedFileName();

    FRandomSeedFile = Value;

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
UnicodeString TConfiguration::GetRandomSeedFileName()
{
  return StripPathQuotes(ExpandEnvironmentVariables(FRandomSeedFile)).Trim();
}
//---------------------------------------------------------------------
void TConfiguration::SetExternalIpAddress(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(ExternalIpAddress);
}
//---------------------------------------------------------------------
void TConfiguration::SetTryFtpWhenSshFails(bool Value)
{
  SET_CONFIG_PROPERTY(TryFtpWhenSshFails);
}
//---------------------------------------------------------------------
void TConfiguration::SetPuttyRegistryStorageKey(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(PuttyRegistryStorageKey);
}
//---------------------------------------------------------------------------
TEOLType TConfiguration::GetLocalEOLType()
{
  return eolCRLF;
}
//---------------------------------------------------------------------
bool TConfiguration::GetCollectUsage() const
{
  return false; // FUsage->Collect;
}
//---------------------------------------------------------------------
void TConfiguration::SetCollectUsage(bool Value)
{
  // FUsage->Collect = Value;
}
//---------------------------------------------------------------------
void TConfiguration::TemporaryLogging(const UnicodeString & ALogFileName)
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
void TConfiguration::TemporaryActionsLogging(const UnicodeString & ALogFileName)
{
  FLogActions = true;
  FActionsLogFileName = ALogFileName;
}
//---------------------------------------------------------------------
void TConfiguration::SetLogging(bool Value)
{
  if (GetLogging() != Value)
  {
    FPermanentLogging = Value;
    FLogging = Value;
    UpdateActualLogProtocol();
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogFileName(const UnicodeString & Value)
{
  if (GetLogFileName() != Value)
  {
    FPermanentLogFileName = Value;
    FLogFileName = Value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetActionsLogFileName(const UnicodeString & Value)
{
  if (GetActionsLogFileName() != Value)
  {
    FPermanentActionsLogFileName = Value;
    FActionsLogFileName = Value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogToFile(bool Value)
{
  if (Value != GetLogToFile())
  {
    SetLogFileName(Value ? GetDefaultLogFileName() : UnicodeString(L""));
    Changed();
  }
}
//---------------------------------------------------------------------
bool TConfiguration::GetLogToFile()
{
  return !GetLogFileName().IsEmpty();
}
//---------------------------------------------------------------------
void TConfiguration::UpdateActualLogProtocol()
{
  FActualLogProtocol = FLogging ? FLogProtocol : 0;
}
//---------------------------------------------------------------------
void TConfiguration::SetLogProtocol(intptr_t Value)
{
  SET_CONFIG_PROPERTY(LogProtocol);
  UpdateActualLogProtocol();
}
//---------------------------------------------------------------------
void TConfiguration::SetLogActions(bool Value)
{
  if (GetLogActions() != Value)
  {
    FPermanentLogActions = Value;
    FLogActions = Value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogFileAppend(bool Value)
{
  SET_CONFIG_PROPERTY(LogFileAppend);
}
//---------------------------------------------------------------------
void TConfiguration::SetLogWindowLines(intptr_t Value)
{
  SET_CONFIG_PROPERTY(LogWindowLines);
}
//---------------------------------------------------------------------
void TConfiguration::SetLogWindowComplete(bool Value)
{
  if (Value != GetLogWindowComplete())
  {
    SetLogWindowLines(Value ? 0 : 50);
    Changed();
  }
}
//---------------------------------------------------------------------
bool TConfiguration::GetLogWindowComplete()
{
  return static_cast<bool>(GetLogWindowLines() == 0);
}
//---------------------------------------------------------------------
UnicodeString TConfiguration::GetDefaultLogFileName()
{
  // return IncludeTrailingBackslash(SystemTemporaryDirectory()) + L"winscp.log";
  return L"%TEMP%\\&S.log";
}
//---------------------------------------------------------------------------
void TConfiguration::SetConfirmOverwriting(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmOverwriting);
}
//---------------------------------------------------------------------------
bool TConfiguration::GetConfirmOverwriting() const
{
  TGuard Guard(FCriticalSection);
  return FConfirmOverwriting;
}
//---------------------------------------------------------------------------
void TConfiguration::SetConfirmResume(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmResume);
}
//---------------------------------------------------------------------------
bool TConfiguration::GetConfirmResume()
{
  TGuard Guard(FCriticalSection);
  return FConfirmResume;
}
//---------------------------------------------------------------------------
void TConfiguration::SetAutoReadDirectoryAfterOp(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(AutoReadDirectoryAfterOp);
}
//---------------------------------------------------------------------------
bool TConfiguration::GetAutoReadDirectoryAfterOp()
{
  TGuard Guard(FCriticalSection);
  return FAutoReadDirectoryAfterOp;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetTimeFormat()
{
  return L"h:nn:ss";
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetPartialExt() const
{
  return PARTIAL_EXT;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetDefaultKeyFile()
{
  return L"";
}
//---------------------------------------------------------------------------
bool TConfiguration::GetRememberPassword()
{
  return false;
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenAuto(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenAutoMaximumNumberOfRetries(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoMaximumNumberOfRetries);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenBackground(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenTimeout(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenTimeout);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenAutoStall(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoStall);
}
//---------------------------------------------------------------------------
void TConfiguration::SetTunnelLocalPortNumberLow(intptr_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}
//---------------------------------------------------------------------------
void TConfiguration::SetTunnelLocalPortNumberHigh(intptr_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberHigh);
}
//---------------------------------------------------------------------------
void TConfiguration::SetCacheDirectoryChangesMaxSize(intptr_t Value)
{
  SET_CONFIG_PROPERTY(CacheDirectoryChangesMaxSize);
}
//---------------------------------------------------------------------------
void TConfiguration::SetShowFtpWelcomeMessage(bool Value)
{
  SET_CONFIG_PROPERTY(ShowFtpWelcomeMessage);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetPermanentLogFileName()
{
  return FPermanentLogFileName;
}
//---------------------------------------------------------------------------
void TConfiguration::SetPermanentLogFileName(const UnicodeString & Value)
{
  FPermanentLogFileName = Value;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetPermanentActionsLogFileName()
{
  return FPermanentActionsLogFileName;
}
//---------------------------------------------------------------------------
void TConfiguration::SetPermanentActionsLogFileName(const UnicodeString & Value)
{
  FPermanentActionsLogFileName = Value;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TShortCuts::Add(const TShortCut & ShortCut)
{
  FShortCuts.push_back(ShortCut);
}
//---------------------------------------------------------------------------
bool TShortCuts::Has(const TShortCut & ShortCut) const
{
  rde::vector<TShortCut>::iterator it = const_cast<TShortCuts *>(this)->FShortCuts.find(ShortCut);
  return (it != FShortCuts.end());
}

