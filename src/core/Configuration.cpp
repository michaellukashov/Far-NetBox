//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#endif

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
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
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
  TGuard Guard(FCriticalSection);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  TRegistryStorage * AdminStorage;
  AdminStorage = new TRegistryStorage(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE);
  // try
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
#ifndef _MSC_VER
  __finally
  {
    delete AdminStorage;
  }
#endif

  SetRandomSeedFile(FDefaultRandomSeedFile);
  SetPuttyRegistryStorageKey(L"Software\\SimonTatham\\PuTTY");
  FConfirmOverwriting = true;
  FConfirmResume = true;
  FAutoReadDirectoryAfterOp = true;
  FSessionReopenAuto = 5000;
  FSessionReopenAutoMaximumNumberOfRetries = CONST_DEFAULT_NUMBER_OF_RETRIES;
  FSessionReopenBackground = 2000;
  FSessionReopenTimeout = 0;
  FSessionReopenAutoStall = 60000;
  FTunnelLocalPortNumberLow = 50000;
  FTunnelLocalPortNumberHigh = 50099;
  FCacheDirectoryChangesMaxSize = 100;
  FShowFtpWelcomeMessage = false;
  FExternalIpAddress = L"";

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
TConfiguration::~TConfiguration()
{
  assert(!FUpdating);
  if (FApplicationInfo) { FreeFileInfo(FApplicationInfo); }
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
THierarchicalStorage * TConfiguration::CreateScpStorage(bool /*SessionList*/)
{
  if (GetStorage() == stRegistry)
  {
    return new TRegistryStorage(GetRegistryStorageKey());
  }
#ifndef _MSC_VER
  else if (Storage == stNul)
  {
    return new TIniFileStorage(L"nul");
  }
#endif
  else
  {
    Error(SNotImplemented, 3005);
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
    KEY(String,   PuttyRegistryStorageKey); \
    KEY(bool,     ConfirmOverwriting); \
    KEY(bool,     ConfirmResume); \
    KEY(bool,     AutoReadDirectoryAfterOp); \
    KEY(int,      SessionReopenAuto); \
    KEY(int,      SessionReopenBackground); \
    KEY(int,      SessionReopenTimeout); \
    KEY(int,      SessionReopenAutoStall); \
    KEY(int,      TunnelLocalPortNumberLow); \
    KEY(int,      TunnelLocalPortNumberHigh); \
    KEY(int,      CacheDirectoryChangesMaxSize); \
    KEY(bool,     ShowFtpWelcomeMessage); \
    KEY(int,      SessionReopenAutoMaximumNumberOfRetries); \
  ); \
  BLOCK(L"Logging", CANCREATE, \
    KEYEX(bool,  Logging, Logging); \
    KEYEX(String,LogFileName, LogFileName); \
    KEY(bool,    LogFileAppend); \
    KEY(int,     LogWindowLines); \
    KEY(int,     LogProtocol); \
    KEYEX(bool,  PermanentLogActions, LogActions); \
    KEYEX(bool,  LogActions, LogActions); \
  );
    // KEY(String,   RandomSeedFile); \
    // KEY(String,   ExternalIpAddress); \
    // KEYEX(String,PermanentActionsLogFileName, ActionsLogFileName); \
    
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
{
  // #define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(LASTELEM(UnicodeString(TEXT(#NAME))), VAR)
  #define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(LASTELEM(MB2W(#NAME)), Get##VAR())
  REGCONFIG(true);
  #undef KEYEX
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Save(bool All, bool Explicit)
{
  if (FDontSave) { return; }

  THierarchicalStorage * AStorage = CreateScpStorage(false);
  // try
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
#ifndef _MSC_VER
  __finally
  {
    delete AStorage;
  }
#endif

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
  System::Error(SNotImplemented, 3004);
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ExportStorage = NULL;
  // try
  {
    BOOST_SCOPE_EXIT ( (&ExportStorage) (&Storage) )
    {
      delete ExportStorage;
      delete Storage;
    } BOOST_SCOPE_EXIT_END
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
#ifndef _MSC_VER
  __finally
  {
    delete ExportStorage;
    delete Storage;
  }
#endif

  StoredSessions->Export(FileName);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::LoadData(THierarchicalStorage * Storage)
{
  #define KEYEX(TYPE, VAR, NAME) Set##VAR(Storage->Read ## TYPE(LASTELEM(System::MB2W(#NAME)), Get##VAR()))
  // #pragma warn -eas
  REGCONFIG(false);
  // #pragma warn +eas
  #undef KEYEX

  if (FPermanentLogActions && FPermanentActionsLogFileName.IsEmpty() &&
      FPermanentLogging && !FPermanentLogFileName.IsEmpty())
  {
     FPermanentActionsLogFileName = FPermanentLogFileName;
     FPermanentLogging = false;
     FPermanentLogFileName = L"";
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  FDisablePasswordStoring = Storage->Readbool(L"DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->Readbool(L"ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->Readbool(L"DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Load()
{
  TGuard Guard(FCriticalSection);

  THierarchicalStorage * Storage = CreateScpStorage(false);
  // try
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
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  TStrings * Names = new TStringList();
  // try
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

            for (int Index = 0; Index < Names->GetCount(); Index++)
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

            for (int Index = 0; Index < Names->GetCount(); Index++)
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

        for (int Index = 0; Index < Names->GetCount(); Index++)
        {
          Target->WriteStringRaw(Names->GetStrings(Index),
            Source->ReadStringRaw(Names->GetStrings(Index), L""));
        }

        Target->CloseSubKey();
      }
      Source->CloseSubKey();
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Names;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::LoadDirectoryChangesCache(const UnicodeString SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  // try
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
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SaveDirectoryChangesCache(const UnicodeString SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  // try
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
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif
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
  // try
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
      (Storage->ReadString(SessionKey, L"") != BannerHash(Banner));
  }
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::NeverShowBanner(const UnicodeString SessionKey,
  const UnicodeString & Banner)
{
  THierarchicalStorage * Storage = CreateScpStorage(false);
  // try
  {
    BOOST_SCOPE_EXIT ( (&Storage) )
    {
      delete Storage;
    } BOOST_SCOPE_EXIT_END
    Storage->SetAccessMode(smReadWrite);

    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(L"Banners", true))
    {
      Storage->WriteString(SessionKey, BannerHash(Banner));
    }
  }
#ifndef _MSC_VER
  __finally
  {
    delete Storage;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::Changed()
{
  if (FUpdating == 0)
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
    throw ExtException(FMTLOAD(CLEANUP_CONFIG_ERROR), &E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::CleanupRegistry(UnicodeString CleanupSubKey)
{
  TRegistryStorage *Registry = new TRegistryStorage(GetRegistryStorageKey());
  // try
  {
    BOOST_SCOPE_EXIT ( (&Registry) )
    {
      delete Registry;
    } BOOST_SCOPE_EXIT_END
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
#ifndef _MSC_VER
  __finally
  {
    delete Registry;
  }
#endif
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
    throw ExtException(FMTLOAD(CLEANUP_HOSTKEYS_ERROR), &E);
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
    throw ExtException(FMTLOAD(CLEANUP_SEEDFILE_ERROR), &E);
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
    throw ExtException(FMTLOAD(CLEANUP_INIFILE_ERROR), &E);
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
VS_FIXEDFILEINFO __fastcall TConfiguration::GetFixedApplicationInfo()
{
  return GetFixedFileInfo(GetApplicationInfo());
}
//---------------------------------------------------------------------------
int __fastcall TConfiguration::GetCompoundVersion()
{
  VS_FIXEDFILEINFO FileInfo = GetFixedApplicationInfo();
  return CalculateCompoundVersion(
    HIWORD(FileInfo.dwFileVersionMS), LOWORD(FileInfo.dwFileVersionMS),
    HIWORD(FileInfo.dwFileVersionLS), LOWORD(FileInfo.dwFileVersionLS));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::ModuleFileName()
{
  System::Error(SNotImplemented, 204);
  return L""; // FIXME ParamStr(0);
}
//---------------------------------------------------------------------------
void * __fastcall TConfiguration::GetFileApplicationInfo(const UnicodeString FileName)
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
void * __fastcall TConfiguration::GetApplicationInfo()
{
  return GetFileApplicationInfo(L"");
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
    VS_FIXEDFILEINFO Info = GetFixedApplicationInfo();
    return FMTLOAD(VERSION,
      HIWORD(Info.dwFileVersionMS),
      LOWORD(Info.dwFileVersionMS),
      HIWORD(Info.dwFileVersionLS),
      LOWORD(Info.dwFileVersionLS));
  }
  catch (Exception &E)
  {
    throw ExtException(L"Can't get application version", &E);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetVersion()
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
  catch (Exception &E)
  {
    throw ExtException(L"Can't get application version", &E);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetFileFileInfoString(const UnicodeString Key,
  const UnicodeString FileName)
{
  TGuard Guard(FCriticalSection);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(FileName);
  // try
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
        DEBUG_PRINTF(L"Error: %s", MB2W(e.what()).c_str());
        Result = L"";
      }
    }
    else
    {
      assert(!FileName.IsEmpty());
    }
  }
#ifndef _MSC_VER
  __finally
  {
    if (!FileName.IsEmpty())
    {
      FreeFileInfo(Info);
    }
  }
#endif
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
  FStorage = stNul;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetDefaultStorage()
{
  FStorage = stDetect;
}
//---------------------------------------------------------------------------
void __fastcall TConfiguration::SetIniFileStorageName(UnicodeString value)
{
  System::Error(SNotImplemented, 3006);
  FIniFileStorageName = value;
  // FStorage = stIniFile;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TConfiguration::GetIniFileStorageName()
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

    // try
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
#ifndef _MSC_VER
    __finally
    {
      delete SourceStorage;
      delete TargetStorage;
    }
#endif

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
  /*
  if (FStorage == stDetect)
  {
    if (FileExists(IniFileStorageName))
    {
      FStorage = stIniFile;
    }
    else
    {
      FStorage = stRegistry;
    }
  }
  */
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
void __fastcall TConfiguration::TemporaryLogging(const UnicodeString ALogFileName)
{
/*
  FLogging = true;
  FLogFileName = ALogFileName;
  FLogActions = AnsiSameText(ExtractFileExt(FLogFileName), L".xml");
  UpdateActualLogProtocol();
*/
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
/*
void __fastcall TConfiguration::SetLogToFile(bool value)
{
  if (value != GetLogToFile())
  {
    SetLogFileName(value ? GetDefaultLogFileName() : UnicodeString(L""));
    Changed();
  }
}
*/
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
  return L"%TEMP%\\!S.log";
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
