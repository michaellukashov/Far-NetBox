//---------------------------------------------------------------------------
#include "stdafx.h"

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
  FSessionReopenAutoMaximumNumberOfRetries(0),
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
  FCriticalSection(NULL)
{
  FCriticalSection = new TCriticalSection();
  FUpdating = 0;
  FStorage = stRegistry;
  FDontSave = false;
  FApplicationInfo = NULL;

  UnicodeString RandomSeedPath;
  if (GetEnvironmentVariable(L"APPDATA", NULL, 0) > 0)
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
  FSessionReopenAutoMaximumNumberOfRetries = CONST_DEFAULT_NUMBER_OF_RETRIES;
  FSessionReopenBackground = 10000;
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
  if (FApplicationInfo) { FreeFileInfo(FApplicationInfo); }
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
THierarchicalStorage * TConfiguration::CreateStorage()
{
  if (GetStorage() == stRegistry)
  {
    return new TRegistryStorage(GetRegistryStorageKey());
  }
  else
  {
    System::Error(SNotImplemented, 3005);
    return NULL; // new TIniFileStorage(GetIniFileStorageName());
  }
}
//---------------------------------------------------------------------------
#define LASTELEM(ELEM) \
  ELEM.SubString(::LastDelimiter(ELEM, L".>") + 1, ELEM.Length() - ::LastDelimiter(ELEM, L".>"))
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
#undef REGCONFIG
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(String,   RandomSeedFile); \
    KEY(String,   PuttyRegistryStorageKey); \
    KEY(bool,     ConfirmOverwriting); \
    KEY(bool,     ConfirmResume); \
    KEY(bool,     AutoReadDirectoryAfterOp); \
    KEY(int,  SessionReopenAuto); \
    KEY(int,  SessionReopenAutoMaximumNumberOfRetries); \
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
#define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(LASTELEM(System::MB2W(#NAME)), Get##VAR())
  REGCONFIG(true);
#undef KEYEX
}
//---------------------------------------------------------------------------
void TConfiguration::Save(bool All, bool Explicit)
{
  if (FDontSave) { return; }

  THierarchicalStorage * AStorage = CreateStorage();
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
void TConfiguration::Export(const UnicodeString FileName)
{
  System::Error(SNotImplemented, 3004);
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  {
    BOOST_SCOPE_EXIT ( (&ExportStorage) (&Storage) )
    {
      delete ExportStorage;
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    ExportStorage = NULL; // new TIniFileStorage(FileName);
    ExportStorage->SetAccessMode(smReadWrite);
    ExportStorage->SetExplicit(true);

    Storage = CreateStorage();
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
#define KEYEX(TYPE, VAR, NAME) Set##VAR(Storage->Read ## TYPE(LASTELEM(System::MB2W(#NAME)), Get##VAR()))
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

  THierarchicalStorage * Storage = CreateStorage();
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
  System::TStrings * Names = new System::TStringList();
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
              Target->WriteBinaryData(Names->GetStrings(Index),
                                      Source->ReadBinaryData(Names->GetStrings(Index)));
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
              Target->WriteString(Names->GetStrings(Index),
                                  Source->ReadString(Names->GetStrings(Index), L""));
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
          Target->WriteStringRaw(Names->GetStrings(Index),
                                 Source->ReadStringRaw(Names->GetStrings(Index), L""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
}
//---------------------------------------------------------------------------
void TConfiguration::LoadDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateStorage();
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
void TConfiguration::SaveDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateStorage();
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
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
UnicodeString TConfiguration::BannerHash(const UnicodeString Banner)
{
  UnicodeString Result(16, 0);
  md5checksum(static_cast<const char *>(static_cast<const void *>(Banner.c_str())), Banner.Length() * sizeof(wchar_t),
              reinterpret_cast<unsigned char *>(const_cast<wchar_t *>(Result.c_str())));
  return Result;
}
//---------------------------------------------------------------------------
bool TConfiguration::ShowBanner(const UnicodeString SessionKey,
                                const UnicodeString Banner)
{
  bool Result;
  THierarchicalStorage * Storage = CreateStorage();
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
void TConfiguration::NeverShowBanner(const UnicodeString SessionKey,
                                     const UnicodeString Banner)
{
  THierarchicalStorage * Storage = CreateStorage();
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
    if (!GetOnChange().IsEmpty())
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
  catch (const std::exception & E)
  {
    throw ExtException(FMTLOAD(CLEANUP_CONFIG_ERROR), &E);
  }
}
//---------------------------------------------------------------------------
void TConfiguration::CleanupRegistry(const UnicodeString CleanupSubKey)
{
  TRegistryStorage * Registry = new TRegistryStorage(GetRegistryStorageKey());
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
  catch (const std::exception & E)
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
  catch (const std::exception & E)
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
#if 0
    if (GetStorage() == stIniFile)
    {
      FDontSave = true;
    }
#endif
  }
  catch (const std::exception & E)
  {
    throw ExtException(FMTLOAD(CLEANUP_INIFILE_ERROR), &E);
  }
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::EncryptPassword(const UnicodeString Password, const UnicodeString Key)
{
  if (Password.IsEmpty())
  {
    return UnicodeString();
  }
  else
  {
    return ::EncryptPassword(Password, Key);
  }
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::DecryptPassword(const UnicodeString Password, const UnicodeString Key)
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
UnicodeString TConfiguration::StronglyRecryptPassword(const UnicodeString Password, const UnicodeString /*Key*/)
{
  return Password;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetOSVersionStr()
{
  UnicodeString Result;
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
UnicodeString TConfiguration::ModuleFileName()
{
  System::Error(SNotImplemented, 204);
  return L""; // FIXME ParamStr(0);
}
//---------------------------------------------------------------------------
void * TConfiguration::GetFileApplicationInfo(const UnicodeString FileName)
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
void * TConfiguration::GetApplicationInfo()
{
  return GetFileApplicationInfo(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileProductName(const UnicodeString FileName)
{
  return GetFileFileInfoString(L"ProductName", FileName);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileCompanyName(const UnicodeString FileName)
{
  return GetFileFileInfoString(L"CompanyName", FileName);
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetProductName()
{
  return GetFileProductName(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetCompanyName()
{
  return GetFileCompanyName(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileProductVersion(const UnicodeString FileName)
{
  return TrimVersion(GetFileFileInfoString(L"ProductVersion", FileName));
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetProductVersion()
{
  return GetFileProductVersion(L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::TrimVersion(const UnicodeString Version)
{
  UnicodeString version = Version;
  while ((version.find_first_of(L".") != ::LastDelimiter(version, L".")) &&
         (version.SubString(version.Length() - 1, 2) == L".0"))
  {
    version.SetLength(version.Length() - 2);
  }
  return version;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetVersionStr()
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
  catch (const std::exception & E)
  {
    throw ExtException(L"Can't get application version", &E);
  }
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetVersion()
{
  TGuard Guard(FCriticalSection);
  try
  {
    UnicodeString Result;
    VS_FIXEDFILEINFO Info = GetFixedApplicationInfo();
    Result = TrimVersion(FORMAT(L"%d.%d.%d",
                                HIWORD(Info.dwFileVersionMS),
                                LOWORD(Info.dwFileVersionMS),
                                HIWORD(Info.dwFileVersionLS)));
    return Result;
  }
  catch (const std::exception & E)
  {
    throw ExtException(L"Can't get application version", &E);
  }
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileFileInfoString(const UnicodeString Key,
    const UnicodeString FileName)
{
  TGuard Guard(FCriticalSection);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(FileName);
  {
    BOOST_SCOPE_EXIT ( (&FileName) (&Info) )
    {
      if (!FileName.IsEmpty())
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
      catch (const std::exception & e)
      {
        DEBUG_PRINTF(L"Error: %s", System::MB2W(e.what()).c_str());
        Result = L"";
      }
    }
    else
    {
      assert(!FileName.IsEmpty());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetFileInfoString(const UnicodeString Key)
{
  return GetFileFileInfoString(Key, L"");
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetRegistryStorageKey()
{
  return ::GetRegistryKey();
}
//---------------------------------------------------------------------------
void TConfiguration::SetIniFileStorageName(const UnicodeString value)
{
  System::Error(SNotImplemented, 3006);
  FIniFileStorageName = value;
  // FStorage = stIniFile;
}
//---------------------------------------------------------------------------
UnicodeString TConfiguration::GetIniFileStorageName()
{
  if (FIniFileStorageName.IsEmpty())
  {
    return ChangeFileExt(L"" /*ParamStr(0)*/, L".ini");
  }
  else
  {
    return FIniFileStorageName;
  }
}
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
      SourceStorage = CreateStorage();
      SourceStorage->SetAccessMode(smRead);

      FStorage = value;

      TargetStorage = CreateStorage();
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
  /*
  if (FStorage == stDetect)
  {
    FStorage = FileExists(GetIniFileStorageName()) ? stIniFile : stRegistry;
  }
  */
  return FStorage;
}
//---------------------------------------------------------------------------
void TConfiguration::SetRandomSeedFile(const UnicodeString value)
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
UnicodeString TConfiguration::GetRandomSeedFileName()
{
  return StripPathQuotes(::Trim(ExpandEnvironmentVariables(FRandomSeedFile)));
}
//---------------------------------------------------------------------
void TConfiguration::SetPuttyRegistryStorageKey(const UnicodeString value)
{
  SET_CONFIG_PROPERTY(PuttyRegistryStorageKey);
}
//---------------------------------------------------------------------------
TEOLType TConfiguration::GetLocalEOLType()
{
  return eolCRLF;
}
//---------------------------------------------------------------------
void TConfiguration::TemporaryLogging(const UnicodeString ALogFileName)
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
void TConfiguration::SetLogFileName(const UnicodeString value)
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
    SetLogFileName(value ? GetDefaultLogFileName() : UnicodeString(L""));
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
void TConfiguration::SetLogProtocol(size_t value)
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
UnicodeString TConfiguration::GetDefaultLogFileName()
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
void TConfiguration::SetSessionReopenAuto(size_t value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenAutoMaximumNumberOfRetries(int value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoMaximumNumberOfRetries);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenBackground(size_t value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}
//---------------------------------------------------------------------------
void TConfiguration::SetSessionReopenTimeout(size_t value)
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
void TShortCuts::Add(System::TShortCut ShortCut)
{
  FShortCuts.insert(ShortCut);
}
//---------------------------------------------------------------------------
bool TShortCuts::Has(System::TShortCut ShortCut) const
{
  return (FShortCuts.count(ShortCut) != 0);
}
