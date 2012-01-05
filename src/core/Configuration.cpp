//---------------------------------------------------------------------------
#include "stdafx.h"
// #include <shfolder.h>
#include <shlobj.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

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
TConfiguration::TConfiguration() :
  FApplicationInfo(NULL),
  FCriticalSection(NULL)
{
  FCriticalSection = new TCriticalSection();
  FUpdating = 0;
  FStorage = stDetect;
  FDontSave = false;
  FApplicationInfo = NULL;

  wchar_t Buf[20];
  std::wstring RandomSeedPath;
  if (GetEnvironmentVariable(L"APPDATA", Buf, sizeof(Buf)) > 0)
  {
    RandomSeedPath = L"%APPDATA%";
  }
  else
  {
    RandomSeedPath = GetShellFolderPath(CSIDL_LOCAL_APPDATA);
    if (RandomSeedPath.empty())
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

  TRegistryStorage * AdminStorage;
  AdminStorage = new TRegistryStorage(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE);
  {
      BOOST_SCOPE_EXIT ( (&AdminStorage) )
      {
        delete AdminStorage;
      } BOOST_SCOPE_EXIT_END
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
  FTunnelLocalPortNumberLow = 50000;
  FTunnelLocalPortNumberHigh = 50099;
  FCacheDirectoryChangesMaxSize = 100;
  FShowFtpWelcomeMessage = false;

  FLogging = false;
  FPermanentLogging = false;
  FLogFileName = L"";
  FPermanentLogFileName = L"";
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
  if (GetStorage() == stRegistry)
  {
    return new TRegistryStorage(GetRegistryStorageKey());
  }
  else
  {
    return new TIniFileStorage(GetIniFileStorageName());
  }
}
//---------------------------------------------------------------------------
#define LASTELEM(ELEM) \
  ELEM.substr(::LastDelimiter(ELEM, L".>") + 1, ELEM.size() - ::LastDelimiter(ELEM, L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
  { \
      BOOST_SCOPE_EXIT ( (&Storage) ) \
      { \
        Storage->CloseSubKey(); \
      } BOOST_SCOPE_EXIT_END \
      BLOCK \
  }
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, VAR)
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
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
    KEY(bool, ShowFtpWelcomeMessage); \
  ); \
  BLOCK(L"Logging", CANCREATE, \
    KEYEX(bool, Logging, Logging); \
    KEYEX(String, LogFileName, LogFileName); \
    KEY(bool, LogFileAppend); \
    KEY(int, LogWindowLines); \
    KEY(int, LogProtocol); \
    KEYEX(bool, LogActions, LogActions); \
  );
//---------------------------------------------------------------------------
void TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
{
  #define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(LASTELEM(::MB2W(#NAME)), Get##VAR())
  REGCONFIG(true);
  #undef KEYEX
}
//---------------------------------------------------------------------------
void TConfiguration::Save(bool All, bool Explicit)
{
  if (FDontSave) return;

  THierarchicalStorage * AStorage = CreateScpStorage(false);
  {
      BOOST_SCOPE_EXIT ( (&AStorage) )
      {
        delete AStorage;
      } BOOST_SCOPE_EXIT_END
    AStorage->SetAccessMode(smReadWrite);
    AStorage->SetExplicit(Explicit);
    if (AStorage->OpenSubKey(GetConfigurationSubKey(), true))
    {
      SaveData(AStorage, All);
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
void TConfiguration::Export(const std::wstring &FileName)
{
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  {
      BOOST_SCOPE_EXIT ( (&ExportStorage) (&Storage) )
      {
        delete ExportStorage;
        delete Storage;
      } BOOST_SCOPE_EXIT_END
    ExportStorage = new TIniFileStorage(FileName);
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

  StoredSessions->Export(FileName);
}
//---------------------------------------------------------------------------
void TConfiguration::LoadData(THierarchicalStorage * Storage)
{
  #define KEYEX(TYPE, VAR, NAME) Set##VAR(Storage->Read ## TYPE(LASTELEM(::MB2W(#NAME)), Get##VAR()))
  // #pragma warn -eas
  REGCONFIG(false);
  // #pragma warn +eas
  #undef KEYEX
}
//---------------------------------------------------------------------------
void TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  FDisablePasswordStoring = Storage->Readbool(L"DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->Readbool(L"ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->Readbool(L"DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
}
//---------------------------------------------------------------------------
void TConfiguration::Load()
{
  TGuard Guard(FCriticalSection);

  THierarchicalStorage * Storage = CreateScpStorage(false);
  {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false))
    {
      LoadData(Storage);
    }
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  TStrings * Names = new TStringList();
  {
      BOOST_SCOPE_EXIT ( (&Names) )
      {
        delete Names;
      } BOOST_SCOPE_EXIT_END
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

            for (size_t Index = 0; Index < Names->GetCount(); Index++)
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

            for (size_t Index = 0; Index < Names->GetCount(); Index++)
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

        for (size_t Index = 0; Index < Names->GetCount(); Index++)
        {
          Target->WriteStringRaw(Names->GetString(Index),
            Source->ReadStringRaw(Names->GetString(Index), L""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
}
//---------------------------------------------------------------------------
void TConfiguration::LoadDirectoryChangesCache(const std::wstring &SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
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
void TConfiguration::SaveDirectoryChangesCache(const std::wstring &SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(L"CDCache", true))
    {
      std::wstring Data;
      DirectoryChangesCache->Serialize(Data);
      Storage->WriteBinaryData(SessionKey, Data);
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::BannerHash(const std::wstring & Banner)
{
  std::wstring Result(16, 0);
  md5checksum(static_cast<const char *>(static_cast<const void *>(Banner.c_str())), Banner.size() * sizeof(wchar_t),
    reinterpret_cast<unsigned char *>(const_cast<wchar_t *>(Result.c_str())));
  return Result;
}
//---------------------------------------------------------------------------
bool TConfiguration::ShowBanner(const std::wstring &SessionKey,
  const std::wstring &Banner)
{
  bool Result;
  THierarchicalStorage * Storage = CreateScpStorage(false);
  {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smRead);
    Result =
      !Storage->OpenSubKey(GetConfigurationSubKey(), false) ||
      !Storage->OpenSubKey(L"Banners", false) ||
      !Storage->ValueExists(SessionKey) ||
      (Storage->ReadString(SessionKey, L"") != StrToHex(BannerHash(Banner)));
  }

  return Result;
}
//---------------------------------------------------------------------------
void TConfiguration::NeverShowBanner(const std::wstring &SessionKey,
  const std::wstring &Banner)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  {
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smReadWrite);

    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(L"Banners", true))
    {
      Storage->WriteString(SessionKey, StrToHex(BannerHash(Banner)));
    }
  }
}
//---------------------------------------------------------------------------
void TConfiguration::Changed()
{
  if (FUpdating== 0)
  {
    if (!GetOnChange().empty())
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
    CleanupRegistry(GetConfigurationSubKey());
    if (GetStorage() == stRegistry)
    {
      FDontSave = true;
    }
  }
  catch (const std::exception &E)
  {
    throw ExtException(FMTLOAD(CLEANUP_CONFIG_ERROR), &E);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupRegistry(std::wstring CleanupSubKey)
{
  TRegistryStorage *Registry = new TRegistryStorage(GetRegistryStorageKey());
  {
      BOOST_SCOPE_EXIT ( (&Registry) )
      {
        delete Registry;
      } BOOST_SCOPE_EXIT_END
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupHostKeys()
{
  try
  {
    CleanupRegistry(GetSshHostKeysSubKey());
  }
  catch (const std::exception &E)
  {
    throw ExtException(FMTLOAD(CLEANUP_HOSTKEYS_ERROR), &E);
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
  catch (const std::exception &E)
  {
    throw ExtException(FMTLOAD(CLEANUP_SEEDFILE_ERROR), &E);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupIniFile()
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
    if (GetStorage() == stIniFile)
    {
      FDontSave = true;
    }
  }
  catch (const std::exception &E)
  {
    throw ExtException(FMTLOAD(CLEANUP_INIFILE_ERROR), &E);
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
    Result = ::Trim(FORMAT(L"%u.%u.%u %s", OSVersionInfo.dwMajorVersion,
      OSVersionInfo.dwMinorVersion, OSVersionInfo.dwBuildNumber,
      OSVersionInfo.szCSDVersion));
  }
  return Result;
}
//---------------------------------------------------------------------------
VS_FIXEDFILEINFO TConfiguration::GetFixedApplicationInfo()
{
  return GetFixedFileInfo(GetApplicationInfo());
}
//---------------------------------------------------------------------------
int TConfiguration::GetCompoundVersion()
{
  VS_FIXEDFILEINFO FileInfo = GetFixedApplicationInfo();
  return CalculateCompoundVersion(
    HIWORD(FileInfo.dwFileVersionMS), LOWORD(FileInfo.dwFileVersionMS),
    HIWORD(FileInfo.dwFileVersionLS), LOWORD(FileInfo.dwFileVersionLS));
 return 0;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::ModuleFileName()
{
  ::Error(SNotImplemented, 204); 
  return L""; // FIXME ParamStr(0);
}
//---------------------------------------------------------------------------
void * TConfiguration::GetFileApplicationInfo(const std::wstring &FileName)
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
  return GetFileApplicationInfo(L"");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileProductName(const std::wstring &FileName)
{
  return GetFileFileInfoString(L"ProductName", FileName);
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileCompanyName(const std::wstring &FileName)
{
  return GetFileFileInfoString(L"CompanyName", FileName);
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetProductName()
{
  return GetFileProductName(L"");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetCompanyName()
{
  return GetFileCompanyName(L"");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileProductVersion(const std::wstring &FileName)
{
  return TrimVersion(GetFileFileInfoString(L"ProductVersion", FileName));
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetProductVersion()
{
  return GetFileProductVersion(L"");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::TrimVersion(std::wstring Version)
{
  while ((Version.find_first_of(L".") != ::LastDelimiter(Version, L".")) &&
    (Version.substr(Version.size() - 1, 2) == L".0"))
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
    VS_FIXEDFILEINFO Info = GetFixedApplicationInfo();
    return FMTLOAD(VERSION,
      HIWORD(Info.dwFileVersionMS),
      LOWORD(Info.dwFileVersionMS),
      HIWORD(Info.dwFileVersionLS));
  }
  catch (const std::exception &E)
  {
    throw ExtException(L"Can't get application version", &E);
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetVersion()
{
  TGuard Guard(FCriticalSection);
  try
  {
    std::wstring Result;
    VS_FIXEDFILEINFO Info = GetFixedApplicationInfo();
    Result = TrimVersion(FORMAT(L"%d.%d.%d",
      HIWORD(Info.dwFileVersionMS),
      LOWORD(Info.dwFileVersionMS),
      HIWORD(Info.dwFileVersionLS)));
    return Result;
  }
  catch (const std::exception &E)
  {
    throw ExtException(L"Can't get application version", &E);
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileFileInfoString(const std::wstring &Key,
  const std::wstring &FileName)
{
  TGuard Guard(FCriticalSection);

  std::wstring Result;
  void * Info = GetFileApplicationInfo(FileName);
  {
      BOOST_SCOPE_EXIT ( (&FileName) (&Info) )
      {
        if (!FileName.empty())
        {
          FreeFileInfo(Info);
        }
      } BOOST_SCOPE_EXIT_END
    if ((Info != NULL) && (GetTranslationCount(Info) > 0))
    {
      TTranslation Translation;
      Translation = GetTranslation(Info, 0);
      try
      {
        Result = ::GetFileInfoString(Info, Translation, Key);
      }
      catch (const std::exception &e)
      {
        DEBUG_PRINTF(L"Error: %s", ::MB2W(e.what()).c_str());
        Result = L"";
      }
    }
    else
    {
      assert(!FileName.empty());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetFileInfoString(const std::wstring &Key)
{
  return GetFileFileInfoString(Key, L"");
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetRegistryStorageKey()
{
  return ::GetRegistryKey();
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
    return ChangeFileExt(L"" /*ParamStr(0)*/, L".ini");
  }
  else
  {
    return FIniFileStorageName;
  }
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetPuttySessionsKey()
{
  return GetPuttyRegistryStorageKey() + L"\\Sessions";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetStoredSessionsSubKey()
{
  return L"Sessions";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetSshHostKeysSubKey()
{
  return L"SshHostKeys";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetConfigurationSubKey()
{
  return L"Configuration";
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

    {
        BOOST_SCOPE_EXIT ( (&SourceStorage) (&TargetStorage) )
        {
          delete SourceStorage;
          delete TargetStorage;
        } BOOST_SCOPE_EXIT_END
      SourceStorage = CreateScpStorage(false);
      SourceStorage->SetAccessMode(smRead);

      FStorage = value;

      TargetStorage = CreateScpStorage(false);
      TargetStorage->SetAccessMode(smReadWrite);
      TargetStorage->SetExplicit(true);

      // copy before save as it removes the ini file,
      // when switching from ini to registry
      CopyData(SourceStorage, TargetStorage);
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
    FStorage = FileExists(GetIniFileStorageName()) ? stIniFile : stRegistry;
  }
  return FStorage;
}
//---------------------------------------------------------------------------
void TConfiguration::SetRandomSeedFile(std::wstring value)
{
  if (GetRandomSeedFile() != value)
  {
    std::wstring PrevRandomSeedFileName = GetRandomSeedFileName();

    FRandomSeedFile = value;

    // never allow empty seed file to avoid Putty trying to reinitialize the path
    if (GetRandomSeedFileName().empty())
    {
      FRandomSeedFile = FDefaultRandomSeedFile;
    }

    if (!PrevRandomSeedFileName.empty() &&
        (PrevRandomSeedFileName != GetRandomSeedFileName()) &&
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
  return StripPathQuotes(::Trim(ExpandEnvironmentVariables(FRandomSeedFile)));
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
void TConfiguration::TemporaryLogging(const std::wstring &ALogFileName)
{
  FLogging = true;
  FLogFileName = ALogFileName;
  FLogActions = AnsiSameText(ExtractFileExt(FLogFileName), L".xml");
  UpdateActualLogProtocol();
}
//---------------------------------------------------------------------
void TConfiguration::SetLogging(bool value)
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
void TConfiguration::SetLogFileName(std::wstring value)
{
  if (GetLogFileName() != value)
  {
    FPermanentLogFileName = value;
    FLogFileName = value;
    Changed();
  }
}
//---------------------------------------------------------------------
void TConfiguration::SetLogToFile(bool value)
{
  if (value != GetLogToFile())
  {
    SetLogFileName(value ? GetDefaultLogFileName() : std::wstring(L""));
    Changed();
  }
}
//---------------------------------------------------------------------
bool TConfiguration::GetLogToFile()
{
  return !GetLogFileName().empty();
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
  if (GetLogActions() != value)
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
  if (value != GetLogWindowComplete())
  {
    SetLogWindowLines(value ? 0 : 50);
    Changed();
  }
}
//---------------------------------------------------------------------
bool TConfiguration::GetLogWindowComplete()
{
  return static_cast<bool>(GetLogWindowLines() == 0);
}
//---------------------------------------------------------------------
std::wstring TConfiguration::GetDefaultLogFileName()
{
  return IncludeTrailingBackslash(SystemTemporaryDirectory()) + L"winscp.log";
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
  return L"h:nn:ss";
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetPartialExt() const
{
  return PARTIAL_EXT;
}
//---------------------------------------------------------------------------
std::wstring TConfiguration::GetDefaultKeyFile()
{
  return L"";
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
void TConfiguration::SetTunnelLocalPortNumberLow(size_t value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}
//---------------------------------------------------------------------------
void TConfiguration::SetTunnelLocalPortNumberHigh(size_t value)
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
