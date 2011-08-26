//---------------------------------------------------------------------------
#include "stdafx.h"
// #include <shfolder.h>
#include <shlobj.h>

#include <FileInfo.h>

#include "std::exceptions.h"
#include "Common.h"
#include "Configuration.h"                              c
#include "PuttyIntf.h"
#include "TextsCore.h"
#include "Interface.h"
#include "CoreMain.h"
#include "Security.h"
//---------------------------------------------------------------------------
TConfiguration::TConfiguration()
{
  FCriticalSection = new TCriticalSection();
  FUpdating = 0;
  FStorage = stDetect;
  FDontSave = false;
  FApplicationInfo = NULL;

  char Buf[10];
  std::wstring RandomSeedPath;
  if (GetEnvironmentVariable("APPDATA", Buf, sizeof(Buf)) > 0)
  {
    RandomSeedPath = "%APPDATA%";
  }
  else
  {
    RandomSeedPath = GetShellFolderPath(CSIDL_LOCAL_APPDATA);
    if (RandomSeedPath.empty())
    {
      RandomSeedPath = GetShellFolderPath(CSIDL_APPDATA);
    }
  }

  FDefaultRandomSeedFile = IncludeTrailingBackslash(RandomSeedPath) + "winscp.rnd";
}
//---------------------------------------------------------------------------
void TConfiguration::Default()
{
  TGuard Guard(FCriticalSection);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  TRegistryStorage * AdminStorage;
  AdminStorage = new TRegistryStorage(RegistryStorageKey, HKEY_LOCAL_MACHINE);
  try
  {
    if (AdminStorage->OpenRootKey(false))
    {
      LoadAdmin(AdminStorage);
      AdminStorage->CloseSubKey();
    }
  }
  catch(...)
  {
    delete AdminStorage;
  }

  RandomSeedFile = FDefaultRandomSeedFile;
  PuttyRegistryStorageKey = "Software\\SimonTatham\\PuTTY";
  FConfirmOverwriting = true;
  FConfirmResume = true;
  FAutoReadDirectoryAfterOp = true;
  FSessionReopenAuto = 5000;
  FSessionReopenBackground = 2000;
  FSessionReopenTimeout = 0;
  FTunnelLocalPortNumberLow = 50000;
  FTunnelLocalPortNumberHigh = 50099;
  FCacheDirectoryChangesMaxSize = 100;
  FShowFtpWelcomeMessage = false;

  FLogging = false;
  FPermanentLogging = false;
  FLogFileName = "";
  FPermanentLogFileName = "";
  FLogFileAppend = true;
  FLogWindowLines = 100;
  FLogProtocol = 0;
  FLogActions = false;
  FPermanentLogActions = false;
  UpdateActualLogProtocol();

  Changed();
}
//---------------------------------------------------------------------------
TConfiguration::~TConfiguration()
{
  assert(!FUpdating);
  if (FApplicationInfo) FreeFileInfo(FApplicationInfo);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
THierarchicalStorage * TConfiguration::CreateScpStorage(bool /*SessionList*/)
{
  if (Storage == stRegistry)
  {
    return new TRegistryStorage(RegistryStorageKey);
  }
  else
  {
    return new TIniFileStorage(IniFileStorageName);
  }
}
//---------------------------------------------------------------------------
#define LASTELEM(ELEM) \
  ELEM.substr(ELEM.LastDelimiter(".>")+1, ELEM.size() - ELEM.LastDelimiter(".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) try { BLOCK } catch(...) { Storage->CloseSubKey(); }
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, VAR)
#define REGCONFIG(CANCREATE) \
  BLOCK("Interface", CANCREATE, \
    KEY(String,   RandomSeedFile); \
    KEY(String,   PuttyRegistryStorageKey); \
    KEY(bool,     ConfirmOverwriting); \
    KEY(bool,     ConfirmResume); \
    KEY(bool,     AutoReadDirectoryAfterOp); \
    KEY(int,  SessionReopenAuto); \
    KEY(int,  SessionReopenBackground); \
    KEY(int,  SessionReopenTimeout); \
    KEY(int,  TunnelLocalPortNumberLow); \
    KEY(int,  TunnelLocalPortNumberHigh); \
    KEY(int,  CacheDirectoryChangesMaxSize); \
    KEY(bool,     ShowFtpWelcomeMessage); \
  ); \
  BLOCK("Logging", CANCREATE, \
    KEYEX(bool,  PermanentLogging, Logging); \
    KEYEX(String,PermanentLogFileName, LogFileName); \
    KEY(bool,    LogFileAppend); \
    KEY(int, LogWindowLines); \
    KEY(int, LogProtocol); \
    KEYEX(bool,  PermanentLogActions, LogActions); \
  );
//---------------------------------------------------------------------------
void TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
{
  #define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(LASTELEM(std::wstring(#NAME)), VAR)
  REGCONFIG(true);
  #undef KEYEX
}
//---------------------------------------------------------------------------
void TConfiguration::Save(bool All, bool Explicit)
{
  if (FDontSave) return;

  THierarchicalStorage * AStorage = CreateScpStorage(false);
  try
  {
    AStorage->AccessMode = smReadWrite;
    AStorage->Explicit = Explicit;
    if (AStorage->OpenSubKey(ConfigurationSubKey, true))
    {
      SaveData(AStorage, All);
    }
  }
  catch(...)
  {
    delete AStorage;
  }

  Saved();

  if (All)
  {
    StoredSessions->Save(true, Explicit);
  }

  // clean up as last, so that if it fails (read only INI), the saving can proceed
  if (Storage == stRegistry)
  {
    CleanupIniFile();
  }
}
//---------------------------------------------------------------------------
void TConfiguration::Export(const std::wstring FileName)
{
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  try
  {
    ExportStorage = new TIniFileStorage(FileName);
    ExportStorage->AccessMode = smReadWrite;
    ExportStorage->Explicit = true;

    Storage = CreateScpStorage(false);
    Storage->AccessMode = smRead;

    CopyData(Storage, ExportStorage);

    if (ExportStorage->OpenSubKey(ConfigurationSubKey, true))
    {
      SaveData(ExportStorage, true);
    }
  }
  catch(...)
  {
    delete ExportStorage;
    delete Storage;
  }

  StoredSessions->Export(FileName);
}
//---------------------------------------------------------------------------
void TConfiguration::LoadData(THierarchicalStorage * Storage)
{
  #define KEYEX(TYPE, VAR, NAME) VAR = Storage->Read ## TYPE(LASTELEM(std::wstring(#NAME)), VAR)
  #pragma warn -eas
  REGCONFIG(false);
  #pragma warn +eas
  #undef KEYEX
}
//---------------------------------------------------------------------------
void TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  FDisablePasswordStoring = Storage->ReadBool("DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->ReadBool("ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->ReadBool("DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
}
//---------------------------------------------------------------------------
void TConfiguration::Load()
{
  TGuard Guard(FCriticalSection);

  THierarchicalStorage * Storage = CreateScpStorage(false);
  try
  {
    Storage->AccessMode = smRead;
    if (Storage->OpenSubKey(ConfigurationSubKey, false))
    {
      LoadData(Storage);
    }
  }
  catch(...)
  {
    delete Storage;
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  TStrings * Names = new TStringList();
  try
  {
    if (Source->OpenSubKey(ConfigurationSubKey, false))
    {
      if (Target->OpenSubKey(ConfigurationSubKey, true))
      {
        if (Source->OpenSubKey("CDCache", false))
        {
          if (Target->OpenSubKey("CDCache", true))
          {
            Names->Clear();
            Source->GetValueNames(Names);

            for (int Index = 0; Index < Names->GetCount(); Index++)
            {
              Target->WriteBinaryData(Names->GetString(Index),
                Source->ReadBinaryData(Names->GetString(Index)));
            }

            Target->CloseSubKey();
          }
          Source->CloseSubKey();
        }

        if (Source->OpenSubKey("Banners", false))
        {
          if (Target->OpenSubKey("Banners", true))
          {
            Names->Clear();
            Source->GetValueNames(Names);

            for (int Index = 0; Index < Names->GetCount(); Index++)
            {
              Target->WriteString(Names->GetString(Index),
                Source->ReadString(Names->GetString(Index), ""));
            }

            Target->CloseSubKey();
          }
          Source->CloseSubKey();
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }

    if (Source->OpenSubKey(SshHostKeysSubKey, false))
    {
      if (Target->OpenSubKey(SshHostKeysSubKey, true))
      {
        Names->Clear();
        Source->GetValueNames(Names);

        for (int Index = 0; Index < Names->GetCount(); Index++)
        {
          Target->WriteStringRaw(Names->GetString(Index),
            Source->ReadStringRaw(Names->GetString(Index), ""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
  catch(...)
  {
    delete Names;
  }
}
//---------------------------------------------------------------------------
void TConfiguration::LoadDirectoryChangesCache(const std::wstring SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  try
  {
    Storage->AccessMode = smRead;
    if (Storage->OpenSubKey(ConfigurationSubKey, false) &&
        Storage->OpenSubKey("CDCache", false) &&
        Storage->ValueExists(SessionKey))
    {
      DirectoryChangesCache->Deserialize(Storage->ReadBinaryData(SessionKey));
    }
  }
  catch(...)
  {
    delete Storage;
  }
}
//---------------------------------------------------------------------------
void TConfiguration::SaveDirectoryChangesCache(const std::wstring SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  try
  {
    Storage->AccessMode = smReadWrite;
    if (Storage->OpenSubKey(ConfigurationSubKey, true) &&
        Storage->OpenSubKey("CDCache", true))
    {
      std::wstring Data;
      DirectoryChangesCache->Serialize(Data);
      Storage->WriteBinaryData(SessionKey, Data);
    }
  }
  catch(...)
  {
    delete Storage;
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::BannerHash(const std::wstring & Banner)
{
  std::wstring Result;
  Result.resize(16);
  md5checksum(Banner.c_str(), Banner.size(), (unsigned char*)Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
bool TConfiguration::ShowBanner(const std::wstring SessionKey,
  const std::wstring & Banner)
{
  bool Result;
  THierarchicalStorage * Storage = CreateScpStorage(false);
  try
  {
    Storage->AccessMode = smRead;
    Result =
      !Storage->OpenSubKey(ConfigurationSubKey, false) ||
      !Storage->OpenSubKey("Banners", false) ||
      !Storage->ValueExists(SessionKey) ||
      (Storage->ReadString(SessionKey, "") != StrToHex(BannerHash(Banner)));
  }
  catch(...)
  {
    delete Storage;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TConfiguration::NeverShowBanner(const std::wstring SessionKey,
  const std::wstring & Banner)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  try
  {
    Storage->AccessMode = smReadWrite;

    if (Storage->OpenSubKey(ConfigurationSubKey, true) &&
        Storage->OpenSubKey("Banners", true))
    {
      Storage->WriteString(SessionKey, StrToHex(BannerHash(Banner)));
    }
  }
  catch(...)
  {
    delete Storage;
  }
}
//---------------------------------------------------------------------------
void TConfiguration::Changed()
{
  if (FUpdating == 0)
  {
    if (OnChange)
    {
      OnChange(this);
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
  // Greater value would probably indicate some nesting problem in code
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
    CleanupRegistry(ConfigurationSubKey);
    if (Storage == stRegistry)
    {
      FDontSave = true;
    }
  }
  catch (std::exception &E)
  {
    throw Extstd::exception(&E, CLEANUP_CONFIG_ERROR);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupRegistry(std::wstring CleanupSubKey)
{
  TRegistryStorage *Registry = new TRegistryStorage(RegistryStorageKey);
  try
  {
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
  catch(...)
  {
    delete Registry;
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupHostKeys()
{
  try
  {
    CleanupRegistry(SshHostKeysSubKey);
  }
  catch (std::exception &E)
  {
    throw Extstd::exception(&E, CLEANUP_HOSTKEYS_ERROR);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupRandomSeedFile()
{
  try
  {
    DontSaveRandomSeed();
    if (FileExists(RandomSeedFileName))
    {
      if (!DeleteFile(RandomSeedFileName))
      {
        RaiseLastOSError();
      }
    }
  }
  catch (std::exception &E)
  {
    throw Extstd::exception(&E, CLEANUP_SEEDFILE_ERROR);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupIniFile()
{
  try
  {
    if (FileExists(IniFileStorageName))
    {
      if (!DeleteFile(IniFileStorageName))
      {
        RaiseLastOSError();
      }
    }
    if (Storage == stIniFile)
    {
      FDontSave = true;
    }
  }
  catch (std::exception &E)
  {
    throw Extstd::exception(&E, CLEANUP_INIFILE_ERROR);
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::EncryptPassword(std::wstring Password, std::wstring Key)
{
  if (Password.empty())
  {
    return std::wstring();
  }
  else
  {
    return ::EncryptPassword(Password, Key);
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::DecryptPassword(std::wstring Password, std::wstring Key)
{
  if (Password.empty())
  {
    return std::wstring();
  }
  else
  {
    return ::DecryptPassword(Password, Key);
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::StronglyRecryptPassword(std::wstring Password, std::wstring /*Key*/)
{
  return Password;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetOSVersionStr()
{
  std::wstring Result;
  OSVERSIONINFO OSVersionInfo;
  OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
  if (GetVersionEx(&OSVersionInfo) != 0)
  {
    Result = FORMAT("%d.%d.%d %s", (int(OSVersionInfo.dwMajorVersion),
      int(OSVersionInfo.dwMinorVersion), int(OSVersionInfo.dwBuildNumber),
      OSVersionInfo.szCSDVersion)).Trim();
  }
  return Result;
}
//---------------------------------------------------------------------------
// TVSFixedFileInfo *TConfiguration::GetFixedApplicationInfo()
// {
  // return GetFixedFileInfo(ApplicationInfo);
// }
//---------------------------------------------------------------------------
int TConfiguration::GetCompoundVersion()
{
  TVSFixedFileInfo * FileInfo = FixedApplicationInfo;
  return CalculateCompoundVersion(
    HIWORD(FileInfo->dwFileVersionMS), LOWORD(FileInfo->dwFileVersionMS),
    HIWORD(FileInfo->dwFileVersionLS), LOWORD(FileInfo->dwFileVersionLS));
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::ModuleFileName()
{
  return ParamStr(0);
}
//---------------------------------------------------------------------------
void * TConfiguration::GetFileApplicationInfo(const std::wstring FileName)
{
  void * Result;
  if (FileName.empty())
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
void * TConfiguration::GetApplicationInfo()
{
  return GetFileApplicationInfo("");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileProductName(const std::wstring FileName)
{
  return GetFileFileInfoString("ProductName", FileName);
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileCompanyName(const std::wstring FileName)
{
  return GetFileFileInfoString("CompanyName", FileName);
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetProductName()
{
  return GetFileProductName("");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetCompanyName()
{
  return GetFileCompanyName("");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileProductVersion(const std::wstring FileName)
{
  return TrimVersion(GetFileFileInfoString("ProductVersion", FileName));
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetProductVersion()
{
  return GetFileProductVersion("");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::TrimVersion(std::wstring Version)
{
  while ((Version.find_first_of(L".") != Version.LastDelimiter(".")) &&
    (Version.substr(Version.size() - 1, 2) == ".0"))
  {
    Version.resize(Version.size() - 2);
  }
  return Version;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetVersionStr()
{
  TGuard Guard(FCriticalSection);
  try
  {
    TVSFixedFileInfo * Info = FixedApplicationInfo;
    return FMTLOAD(VERSION, (
      HIWORD(Info->dwFileVersionMS),
      LOWORD(Info->dwFileVersionMS),
      HIWORD(Info->dwFileVersionLS),
      LOWORD(Info->dwFileVersionLS)));
  }
  catch (std::exception &E)
  {
    throw Extstd::exception(&E, "Can't get application version");
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetVersion()
{
  TGuard Guard(FCriticalSection);
  try
  {
    TVSFixedFileInfo * Info = FixedApplicationInfo;
    std::wstring Result;
    Result = TrimVersion(FORMAT("%d.%d.%d", (
      HIWORD(Info->dwFileVersionMS),
      LOWORD(Info->dwFileVersionMS),
      HIWORD(Info->dwFileVersionLS))));
    return Result;
  }
  catch (std::exception &E)
  {
    throw Extstd::exception(&E, "Can't get application version");
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileFileInfoString(const std::wstring Key,
  const std::wstring FileName)
{
  TGuard Guard(FCriticalSection);

  std::wstring Result;
  void * Info = GetFileApplicationInfo(FileName);
  try
  {
    if ((Info != NULL) && (GetTranslationCount(Info) > 0))
    {
      TTranslation Translation;
      Translation = GetTranslation(Info, 0);
      Result = ::GetFileInfoString(Info, Translation, Key);
    }
    else
    {
      assert(!FileName.empty());
    }
  }
  catch(...)
  {
    if (!FileName.empty())
    {
      FreeFileInfo(Info);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileInfoString(const std::wstring Key)
{
  return GetFileFileInfoString(Key, "");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetRegistryStorageKey()
{
  return GetRegistryKey();
}
//---------------------------------------------------------------------------
void TConfiguration::SetIniFileStorageName(std::wstring value)
{
  FIniFileStorageName = value;
  FStorage = stIniFile;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetIniFileStorageName()
{
  if (FIniFileStorageName.empty())
  {
    return ChangeFileExt(ParamStr(0), ".ini");
  }
  else
  {
    return FIniFileStorageName;
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetPuttySessionsKey()
{
  return PuttyRegistryStorageKey + "\\Sessions";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetStoredSessionsSubKey()
{
  return "Sessions";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetSshHostKeysSubKey()
{
  return "SshHostKeys";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetConfigurationSubKey()
{
  return "Configuration";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetRootKeyStr()
{
  return RootKeyToStr(HKEY_CURRENT_USER);
}
//---------------------------------------------------------------------------
bool TConfiguration::GetGSSAPIInstalled()
{
  return HasGSSAPI();
}
//---------------------------------------------------------------------------
void TConfiguration::SetStorage(TStorage value)
{
  if (FStorage != value)
  {
    THierarchicalStorage * SourceStorage = NULL;
    THierarchicalStorage * TargetStorage = NULL;

    try
    {
      SourceStorage = CreateScpStorage(false);
      SourceStorage->AccessMode = smRead;

      FStorage = value;

      TargetStorage = CreateScpStorage(false);
      TargetStorage->AccessMode = smReadWrite;
      TargetStorage->Explicit = true;

      // copy before save as it removes the ini file,
      // when switching from ini to registry
      CopyData(SourceStorage, TargetStorage);
    }
    catch(...)
    {
      delete SourceStorage;
      delete TargetStorage;
    }

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
    FStorage = FileExists(IniFileStorageName) ? stIniFile : stRegistry;
  }
  return FStorage;
}
//---------------------------------------------------------------------------
void TConfiguration::SetRandomSeedFile(std::wstring value)
{
  if (RandomSeedFile != value)
  {
    std::wstring PrevRandomSeedFileName = RandomSeedFileName;

    FRandomSeedFile = value;

    // never allow empty seed file to avoid Putty trying to reinitialize the path
    if (RandomSeedFileName.empty())
    {
      FRandomSeedFile = FDefaultRandomSeedFile;
    }

    if (!PrevRandomSeedFileName.empty() &&
        (PrevRandomSeedFileName != RandomSeedFileName) &&
        FileExists(PrevRandomSeedFileName))
    {
      // ignore any error
      DeleteFile(PrevRandomSeedFileName);
    }
  }
}
//---------------------------------------------------------------------
std::wstring TConfiguration::GetRandomSeedFileName()
{
  return StripPathQuotes(ExpandEnvironmentVariables(FRandomSeedFile)).Trim();
}
//---------------------------------------------------------------------
void TConfiguration::SetPuttyRegistryStorageKey(std::wstring value)
{
  SET_CONFIG_PROPERTY(PuttyRegistryStorageKey);
}
//---------------------------------------------------------------------------
TEOLType TConfiguration::GetLocalEOLType()
{
  return eolCRLF;
}
//---------------------------------------------------------------------
void TConfiguration::TemporaryLogging(const std::wstring ALogFileName)
{
  FLogging = true;
  FLogFileName = ALogFileName;
  FLogActions = SameText(ExtractFileExt(FLogFileName), ".xml");
  UpdateActualLogProtocol();
}
//---------------------------------------------------------------------
void TConfiguration::SetLogging(bool value)
{
  if (Logging != value)
  {
    FPermanentLogging = value;
    FLogging = value;
    UpdateActualLogProtocol();
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogFileName(std::wstring value)
{
  if (LogFileName != value)
  {
    FPermanentLogFileName = value;
    FLogFileName = value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogToFile(bool value)
{
  if (value != LogToFile)
  {
    LogFileName = value ? DefaultLogFileName : std::wstring("");
    Changed();
  }
}
//---------------------------------------------------------------------
bool TConfiguration::GetLogToFile()
{
  return !LogFileName.empty();
}
//---------------------------------------------------------------------
void TConfiguration::UpdateActualLogProtocol()
{
  FActualLogProtocol = FLogging ? FLogProtocol : 0;
}
//---------------------------------------------------------------------
void TConfiguration::SetLogProtocol(int value)
{
  SET_CONFIG_PROPERTY(LogProtocol);
  UpdateActualLogProtocol();
}
//---------------------------------------------------------------------
void TConfiguration::SetLogActions(bool value)
{
  if (LogActions != value)
  {
    FPermanentLogActions = value;
    FLogActions = value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogFileAppend(bool value)
{
  SET_CONFIG_PROPERTY(LogFileAppend);
}
//---------------------------------------------------------------------
void TConfiguration::SetLogWindowLines(int value)
{
  SET_CONFIG_PROPERTY(LogWindowLines);
}
//---------------------------------------------------------------------
void TConfiguration::SetLogWindowComplete(bool value)
{
  if (value != LogWindowComplete)
  {
    LogWindowLines = value ? 0 : 50;
    Changed();
  }
}
//---------------------------------------------------------------------
bool TConfiguration::GetLogWindowComplete()
{
  return (bool)(LogWindowLines == 0);
}
//---------------------------------------------------------------------
std::wstring TConfiguration::GetDefaultLogFileName()
{
  return IncludeTrailingBackslash(SystemTemporaryDirectory()) + "winscp.log";
}
//---------------------------------------------------------------------------
void TConfiguration::SetConfirmOverwriting(bool value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmOverwriting);
}
//---------------------------------------------------------------------------
bool TConfiguration::GetConfirmOverwriting()
{
  TGuard Guard(FCriticalSection);
  return FConfirmOverwriting;
}
//---------------------------------------------------------------------------
void TConfiguration::SetConfirmResume(bool value)
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
void TConfiguration::SetAutoReadDirectoryAfterOp(bool value)
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
std::wstring TConfiguration::GetTimeFormat()
{
  return "h:nn:ss";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetPartialExt() const
{
  return PARTIAL_EXT;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetDefaultKeyFile()
{
  return "";
}
//---------------------------------------------------------------------------
bool TConfiguration::GetRememberPassword()
{
  return false;
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenAuto(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenBackground(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenTimeout(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenTimeout);
}
//---------------------------------------------------------------------------
void TConfiguration::SetTunnelLocalPortNumberLow(int value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}
//---------------------------------------------------------------------------
void TConfiguration::SetTunnelLocalPortNumberHigh(int value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberHigh);
}
//---------------------------------------------------------------------------
void TConfiguration::SetCacheDirectoryChangesMaxSize(int value)
{
  SET_CONFIG_PROPERTY(CacheDirectoryChangesMaxSize);
}
//---------------------------------------------------------------------------
void TConfiguration::SetShowFtpWelcomeMessage(bool value)
{
  SET_CONFIG_PROPERTY(ShowFtpWelcomeMessage);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TShortCuts::Add(TShortCut ShortCut)
{
  FShortCuts.insert(ShortCut);
}
//---------------------------------------------------------------------------
bool TShortCuts::Has(TShortCut ShortCut) const
{
  return (FShortCuts.count(ShortCut) != 0);
}
