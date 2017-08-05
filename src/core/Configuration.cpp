
#include <vcl.h>
#pragma hdrstop

#include <FileInfo.h>

#include <Common.h>
#include <Exceptions.h>
#include "Configuration.h"
#include "PuttyIntf.h"
#include "TextsCore.h"
#include "Interface.h"
#include "CoreMain.h"
#include "WinSCPSecurity.h"
#if defined(_MSC_VER) && !defined(__clang__)
#include <shlobj.h>
#endif // if defined(_MSC_VER) && !defined(__clang__)
#include <System.IOUtils.hpp>
#include <System.StrUtils.hpp>

const wchar_t * AutoSwitchNames = L"On;Off;Auto";
const wchar_t * NotAutoSwitchNames = L"Off;On;Auto";

// See https://www.iana.org/assignments/hash-function-text-names/hash-function-text-names.xhtml
const UnicodeString Sha1ChecksumAlg(L"sha-1");
const UnicodeString Sha224ChecksumAlg(L"sha-224");
const UnicodeString Sha256ChecksumAlg(L"sha-256");
const UnicodeString Sha384ChecksumAlg(L"sha-384");
const UnicodeString Sha512ChecksumAlg(L"sha-512");
const UnicodeString Md5ChecksumAlg(L"md5");
// Not defined by IANA
const UnicodeString Crc32ChecksumAlg(L"crc32");

const UnicodeString SshFingerprintType(L"ssh");
const UnicodeString TlsFingerprintType(L"tls");

TConfiguration::TConfiguration(TObjectClassId Kind) :
  TObject(Kind),
  FDontSave(false),
  FChanged(false),
  FUpdating(0),
  FApplicationInfo(nullptr),
  FLogging(false),
  FPermanentLogging(false),
  FLogWindowLines(0),
  FLogFileAppend(false),
  FLogSensitive(false),
  FPermanentLogSensitive(false),
  FLogMaxSize(0),
  FPermanentLogMaxSize(0),
  FLogMaxCount(0),
  FPermanentLogMaxCount(0),
  FLogProtocol(0),
  FPermanentLogProtocol(0),
  FActualLogProtocol(0),
  FLogActions(false),
  FPermanentLogActions(false),
  FLogActionsRequired(false),
  FConfirmOverwriting(false),
  FConfirmResume(false),
  FAutoReadDirectoryAfterOp(false),
  FSessionReopenAuto(0),
  FSessionReopenBackground(0),
  FSessionReopenTimeout(0),
  FSessionReopenAutoStall(0),
  FProgramIniPathWritable(0),
  FTunnelLocalPortNumberLow(0),
  FTunnelLocalPortNumberHigh(0),
  FCacheDirectoryChangesMaxSize(0),
  FShowFtpWelcomeMessage(false),
  FTryFtpWhenSshFails(false),
  FParallelDurationThreshold(0),
  FScripting(false),
  FSessionReopenAutoMaximumNumberOfRetries(0),
  FDisablePasswordStoring(false),
  FForceBanners(false),
  FDisableAcceptingHostKeys(false),
  FDefaultCollectUsage(false)
{
  FUpdating = 0;
  FStorage = stDetect;
  FDontSave = false;
  FApplicationInfo = nullptr;
#if 0
  FUsage = new TUsage(this);
#endif // #if 0
  FDefaultCollectUsage = false;
  FScripting = false;

  UnicodeString RandomSeedPath;
  if (!base::GetEnvVariable("APPDATA").IsEmpty())
  {
    RandomSeedPath = "%APPDATA%";
  }
  else
  {
#if defined(_MSC_VER) && !defined(__clang__)
    RandomSeedPath = ::GetShellFolderPath(CSIDL_LOCAL_APPDATA);
    if (RandomSeedPath.IsEmpty())
    {
      RandomSeedPath = ::GetShellFolderPath(CSIDL_APPDATA);
    }
#endif // if defined(_MSC_VER) && !defined(__clang__)
  }

  FDefaultRandomSeedFile = ::IncludeTrailingBackslash(RandomSeedPath) + "winscp.rnd";
}

void TConfiguration::Default()
{
  TGuard Guard(FCriticalSection);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  std::unique_ptr<TRegistryStorage> AdminStorage(new TRegistryStorage(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE));
  try__finally
  {
    if (AdminStorage->OpenRootKey(false))
    {
      LoadAdmin(AdminStorage.get());
      AdminStorage->CloseSubKey();
    }
  }
  __finally
  {
#if 0
    delete AdminStorage;
#endif // #if 0
  };

  SetRandomSeedFile(FDefaultRandomSeedFile);
  SetPuttyRegistryStorageKey(OriginalPuttyRegistryStorageKey);
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
  FExternalIpAddress.Clear();
  FTryFtpWhenSshFails = true;
  FParallelDurationThreshold = 10;
  SetCollectUsage(FDefaultCollectUsage);
  FSessionReopenAutoMaximumNumberOfRetries = CONST_DEFAULT_NUMBER_OF_RETRIES;

  FLogging = false;
  FPermanentLogging = false;
  FLogFileName = GetDefaultLogFileName();
  FPermanentLogFileName = FLogFileName;
  FLogFileAppend = true;
  FLogSensitive = false;
  FPermanentLogSensitive = FLogSensitive;
  FLogMaxSize = 0;
  FPermanentLogMaxSize = FLogMaxSize;
  FLogMaxCount = 0;
  FPermanentLogMaxCount = FLogMaxCount;
  FLogProtocol = 0;
  FPermanentLogProtocol = FLogProtocol;
  UpdateActualLogProtocol();
  FLogActions = false;
  FPermanentLogActions = false;
  FLogActionsRequired = false;
  FActionsLogFileName = "%TEMP%\\&S.xml";
  FPermanentActionsLogFileName = FActionsLogFileName;
  FProgramIniPathWritable = -1;
  FCustomIniFileStorageName = LoadCustomIniFileStorageName();

  Changed();
}

TConfiguration::~TConfiguration()
{
  DebugAssert(!FUpdating);
  if (FApplicationInfo)
  {
    FreeFileInfo(FApplicationInfo);
  }
#if 0
  delete FCriticalSection;
  delete FUsage;
#endif // #if 0
}

void TConfiguration::UpdateStaticUsage()
{
#if 0
  Usage->Set(L"ConfigurationIniFile", (Storage == stIniFile));
  Usage->Set(L"ConfigurationIniFileCustom", !CustomIniFileStorageName.IsEmpty());
  Usage->Set("Unofficial", IsUnofficial);
#endif // #if 0

  // this is called from here, because we are guarded from calling into
  // master password handler here, see TWinConfiguration::UpdateStaticUsage
  StoredSessions->UpdateStaticUsage();
}

THierarchicalStorage * TConfiguration::CreateConfigStorage()
{
  bool SessionList = false;
  return CreateStorage(SessionList);
}

THierarchicalStorage * TConfiguration::CreateStorage(bool & SessionList)
{
  TGuard Guard(FCriticalSection);
  THierarchicalStorage * Result = nullptr;
  if (GetStorage() == stRegistry)
  {
    Result = new TRegistryStorage(GetRegistryStorageKey());
  }
  else if (GetStorage() == stNul)
  {
#if 0
    Result = TIniFileStorage::CreateFromPath(INI_NUL);
#endif // #if 0
    ThrowNotImplemented(3005);
    DebugAssert(false);
  }
  else
  {
#if 0
    UnicodeString StorageName = IniFileStorageName;
    Result = TIniFileStorage::CreateFromPath(StorageName);
#endif // #if 0
    ThrowNotImplemented(3005);
    DebugAssert(false);
  }

  if ((FOptionsStorage.get() != nullptr) && (FOptionsStorage->GetCount() > 0))
  {
    if (!SessionList)
    {
//      Result = new TOptionsStorage(FOptionsStorage.get(), ConfigurationSubKey, Result);
    }
    else
    {
      // cannot reuse session list storage for configuration as for it we need
      // the option-override storage above
    }
  }
  else
  {
    // All the above stores can be reused for configuration,
    // if no options-overrides are set
    SessionList = false;
  }

  return Result;
}

UnicodeString TConfiguration::PropertyToKey(UnicodeString Property)
{
  // no longer useful
  intptr_t P = Property.LastDelimiter(L".>");
  return Property.SubString(P + 1, Property.Length() - P);
}

#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>") + 1, ELEM.Length() - ELEM.LastDelimiter(L".>"))

#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKey(KEY, CANCREATE, true)) \
    { SCOPE_EXIT { Storage->CloseSubKey(); }; { BLOCK } }
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, VAR)
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
    KEY(Integer,  ParallelDurationThreshold); \
    KEY(Bool,     CollectUsage); \
    KEY(Integer,  SessionReopenAutoMaximumNumberOfRetries); \
  ); \
  BLOCK(L"Logging", CANCREATE, \
    KEYEX(Bool,  PermanentLogging, Logging); \
    KEYEX(String,PermanentLogFileName, LogFileName); \
    KEY(Bool,    LogFileAppend); \
    KEYEX(Bool,  PermanentLogSensitive, LogSensitive); \
    KEYEX(Int64, PermanentLogMaxSize, LogMaxSize); \
    KEYEX(Integer, PermanentLogMaxCount, LogMaxCount); \
    KEYEX(Integer,PermanentLogProtocol, LogProtocol); \
    KEYEX(Bool,  PermanentLogActions, LogActions); \
    KEYEX(String,PermanentActionsLogFileName, ActionsLogFileName); \
  )

void TConfiguration::SaveData(THierarchicalStorage * Storage, bool /*All*/)
{
#define KEYEX(TYPE, NAME, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR())
  REGCONFIG(true);
#undef KEYEX

  if (Storage->OpenSubKey("Usage", true))
  {
    // FUsage->Save(Storage);
    Storage->CloseSubKey();
  }
}

void TConfiguration::Save()
{
  // only modified, implicit
  DoSave(false, false);
}

void TConfiguration::SaveExplicit()
{
  // only modified, explicit
  DoSave(false, true);
}

void TConfiguration::DoSave(bool All, bool Explicit)
{
  if (FDontSave)
  {
    return;
  }

  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);
    Storage->SetExplicit(Explicit);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true))
    {
      // if saving to TOptionsStorage, make sure we save everything so that
      // all configuration is properly transferred to the master storage
      bool ConfigAll = All || Storage->GetTemporary();
      SaveData(Storage.get(), ConfigAll);
    }
  }
  __finally
  {
#if 0
    delete AStorage;
#endif // #if 0
  };

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

  SaveCustomIniFileStorageName();
}

void TConfiguration::SaveCustomIniFileStorageName()
{
#if 0
  // Particularly, not to create an empty "Override" key, unless the custom INI file is ever set
  if (CustomIniFileStorageName != LoadCustomIniFileStorageName())
  {
    std::unique_ptr<TRegistryStorage> RegistryStorage(new TRegistryStorage(GetRegistryStorageOverrideKey()));
    RegistryStorage->AccessMode = smReadWrite;
    RegistryStorage->Explicit = true;
    if (RegistryStorage->OpenRootKey(true))
    {
      RegistryStorage->WriteString(L"IniFile", CustomIniFileStorageName);
      RegistryStorage->CloseSubKey();
    }
  }
#endif // #if 0
}

void TConfiguration::Export(UnicodeString /*AFileName*/)
{
  ThrowNotImplemented(3004);
#if 0
  // not to "append" the export to an existing file
  if (FileExists(FileName))
  {
    DeleteFileChecked(FileName);
  }

  THierarchicalStorage * Storage = nullptr;
  THierarchicalStorage * ExportStorage = nullptr;
  try
  {
    ExportStorage = TIniFileStorage::CreateFromPath(FileName);
    ExportStorage->AccessMode = smReadWrite;
    ExportStorage->Explicit = true;

    Storage = CreateConfigStorage();
    Storage->AccessMode = smRead;

    CopyData(Storage, ExportStorage);

    if (ExportStorage->OpenSubKey(ConfigurationSubKey, true))
    {
      SaveData(ExportStorage, true);
    }
  }
  __finally
  {
    delete ExportStorage;
    delete Storage;
  }

  StoredSessions->Export(FileName);
#endif // #if 0
}

void TConfiguration::Import(UnicodeString /*AFileName*/)
{
  ThrowNotImplemented(3005);
#if 0
  THierarchicalStorage * Storage = NULL;
  THierarchicalStorage * ImportStorage = NULL;
  try
  {
    ImportStorage = TIniFileStorage::CreateFromPath(FileName);
    ImportStorage->AccessMode = smRead;

    Storage = CreateConfigStorage();
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
  DoSave(true, true);
#endif // #if 0
}

void TConfiguration::LoadData(THierarchicalStorage * Storage)
{
#define KEYEX(TYPE, NAME, VAR) Set ## VAR(Storage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR()))
  REGCONFIG(false);
#undef KEYEX

  if (Storage->OpenSubKey("Usage", false))
  {
    // FUsage->Load(Storage);
    Storage->CloseSubKey();
  }

  if (FPermanentLogActions && FPermanentActionsLogFileName.IsEmpty() &&
      FPermanentLogging && !FPermanentLogFileName.IsEmpty())
  {
    FPermanentActionsLogFileName = FPermanentLogFileName;
    FPermanentLogging = false;
    FPermanentLogFileName.Clear();
  }
}

void TConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  FDisablePasswordStoring = Storage->ReadBool("DisablePasswordStoring", FDisablePasswordStoring);
  FForceBanners = Storage->ReadBool("ForceBanners", FForceBanners);
  FDisableAcceptingHostKeys = Storage->ReadBool("DisableAcceptingHostKeys", FDisableAcceptingHostKeys);
  FDefaultCollectUsage = Storage->ReadBool("DefaultCollectUsage", FDefaultCollectUsage);
}

void TConfiguration::LoadFrom(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(GetConfigurationSubKey(), false))
  {
    LoadData(Storage);
    Storage->CloseSubKey();
  }
}

UnicodeString TConfiguration::GetRegistryStorageOverrideKey() const
{
  return GetRegistryStorageKey() + L" Override";
}

UnicodeString TConfiguration::LoadCustomIniFileStorageName()
{
  UnicodeString Result;
  std::unique_ptr<TRegistryStorage> RegistryStorage(new TRegistryStorage(GetRegistryStorageOverrideKey()));
  if (RegistryStorage->OpenRootKey(false))
  {
    Result = RegistryStorage->ReadString(L"IniFile", L"");
    RegistryStorage->CloseSubKey();
  }
  RegistryStorage.reset(nullptr);
  return Result;
}

void TConfiguration::Load(THierarchicalStorage * Storage)
{
  TGuard Guard(FCriticalSection);
  TStorageAccessMode StorageAccessMode = Storage->GetAccessMode();
  try__finally
  {
    SCOPE_EXIT
    {
      Storage->SetAccessMode(StorageAccessMode);
    };
    Storage->SetAccessMode(smRead);
    LoadFrom(Storage);
  }
  __finally
  {
#if 0
    Storage->AccessMode = StorageAccessMode;
#endif // #if 0
  };
}

void TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  std::unique_ptr<TStrings> Names(new TStringList());
  try__finally
  {
    if (Source->OpenSubKey(GetConfigurationSubKey(), false))
    {
      if (Target->OpenSubKey(GetConfigurationSubKey(), true))
      {
        if (Source->OpenSubKey("CDCache", false))
        {
          if (Target->OpenSubKey("CDCache", true))
          {
            Names->Clear();
            Source->GetValueNames(Names.get());

            for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
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
            Source->GetValueNames(Names.get());

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
        Source->GetValueNames(Names.get());

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
  __finally
  {
#if 0
    delete Names;
#endif // #if 0
  };
}

void TConfiguration::LoadDirectoryChangesCache(const UnicodeString SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
        Storage->OpenSubKey("CDCache", false) &&
        Storage->ValueExists(SessionKey))
    {
      DirectoryChangesCache->Deserialize(Storage->ReadBinaryData(SessionKey));
    }
  }
  __finally
  {
#if 0
    delete Storage;
#endif // #if 0
  };
}

void TConfiguration::SaveDirectoryChangesCache(const UnicodeString SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey("CDCache", true))
    {
      UnicodeString Data;
      DirectoryChangesCache->Serialize(Data);
      Storage->WriteBinaryData(SessionKey, Data);
    }
  }
  __finally
  {
#if 0
    delete Storage;
#endif // #if 0
  };
}

UnicodeString TConfiguration::BannerHash(UnicodeString Banner) const
{
  RawByteString Result;
  Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(Banner.c_str()), static_cast<int>(Banner.Length() * sizeof(wchar_t)),
    reinterpret_cast<uint8_t *>(const_cast<char *>(Result.c_str())));
  return BytesToHex(Result);
}

bool TConfiguration::ShowBanner(const UnicodeString SessionKey,
  UnicodeString Banner)
{
  bool Result;
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smRead);
    Result =
      !Storage->OpenSubKey(GetConfigurationSubKey(), false) ||
      !Storage->OpenSubKey("Banners", false) ||
      !Storage->ValueExists(SessionKey) ||
      (Storage->ReadString(SessionKey, L"") != BannerHash(Banner));
  }
  __finally
  {
#if 0
    delete Storage;
#endif // #if 0
  };

  return Result;
}

void TConfiguration::NeverShowBanner(const UnicodeString SessionKey,
  UnicodeString Banner)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);

    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey("Banners", true))
    {
      Storage->WriteString(SessionKey, BannerHash(Banner));
    }
  }
  __finally
  {
#if 0
    delete Storage;
#endif // #if 0
  };
}

UnicodeString TConfiguration::FormatFingerprintKey(UnicodeString SiteKey, UnicodeString FingerprintType) const
{
  return FORMAT("%s:%s", SiteKey, FingerprintType);
}

void TConfiguration::RememberLastFingerprint(UnicodeString SiteKey, UnicodeString FingerprintType, UnicodeString Fingerprint)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
      Storage->OpenSubKey("LastFingerprints", true))
  {
    UnicodeString FingerprintKey = FormatFingerprintKey(SiteKey, FingerprintType);
    Storage->WriteString(FingerprintKey, Fingerprint);
  }
}

UnicodeString TConfiguration::GetLastFingerprint(UnicodeString SiteKey, UnicodeString FingerprintType)
{
  UnicodeString Result;

  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smRead);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
      Storage->OpenSubKey("LastFingerprints", false))
  {
    UnicodeString FingerprintKey = FormatFingerprintKey(SiteKey, FingerprintType);
    Result = Storage->ReadString(FingerprintKey, L"");
  }
  return Result;
}

void TConfiguration::Changed()
{
  TNotifyEvent AOnChange = nullptr;

  {
    TGuard Guard(FCriticalSection);
    if (FUpdating == 0)
    {
      AOnChange = GetOnChange();
    }
    else
    {
      FChanged = true;
    }
  }

  // No specific reason to call this outside of a guard, just that it is less of a change to a previous unguarded code
  if (AOnChange != nullptr)
  {
    AOnChange(this);
  }
}

void TConfiguration::BeginUpdate()
{
  FCriticalSection.Enter();
  if (FUpdating == 0)
  {
    FChanged = false;
  }
  FUpdating++;
  // Greater value would probably indicate some nesting problem in code
  DebugAssert(FUpdating < 6);
}

void TConfiguration::EndUpdate()
{
  DebugAssert(FUpdating > 0);
  --FUpdating;
  if ((FUpdating == 0) && FChanged)
  {
    FChanged = false;
    Changed();
  }
  FCriticalSection.Leave();
}

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
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_CONFIG_ERROR));
  }
}

void TConfiguration::CleanupRegistry(UnicodeString CleanupSubKey)
{
  std::unique_ptr<TRegistryStorage> Registry(new TRegistryStorage(GetRegistryStorageKey()));
  try__finally
  {
    Registry->RecursiveDeleteSubKey(CleanupSubKey);
  }
  __finally
  {
#if 0
    delete Registry;
#endif // #if 0
  };
}

void TConfiguration::CleanupHostKeys()
{
  try
  {
    CleanupRegistry(GetSshHostKeysSubKey());
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_HOSTKEYS_ERROR));
  }
}

void TConfiguration::CleanupRandomSeedFile()
{
  try
  {
    DontSaveRandomSeed();
    if (::FileExists(ApiPath(GetRandomSeedFileName())))
    {
      DeleteFileChecked(GetRandomSeedFileName());
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_SEEDFILE_ERROR));
  }
}

void TConfiguration::CleanupIniFile()
{
#if 0
  try
  {
    if (::FileExists(ApiPath(GetIniFileStorageNameForReading())))
    {
      DeleteFileChecked(GetIniFileStorageNameForReading());
    }
    if (GetStorage() == stIniFile)
    {
      FDontSave = true;
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_INIFILE_ERROR));
  }
#endif // #if 0
}

void TConfiguration::DontSave()
{
  FDontSave = true;
}

RawByteString TConfiguration::EncryptPassword(UnicodeString Password, UnicodeString Key)
{
  if (Password.IsEmpty())
  {
    return RawByteString();
  }
  return ::EncryptPassword(Password, Key);
}

UnicodeString TConfiguration::DecryptPassword(RawByteString Password, UnicodeString Key)
{
  if (Password.IsEmpty())
  {
    return UnicodeString();
  }
  return ::DecryptPassword(Password, Key);
}

RawByteString TConfiguration::StronglyRecryptPassword(RawByteString Password, UnicodeString /*Key*/)
{
  return Password;
}

TVSFixedFileInfo * TConfiguration::GetFixedApplicationInfo() const
{
  return GetFixedFileInfo(GetApplicationInfo());
}

intptr_t TConfiguration::GetCompoundVersion() const
{
  TVSFixedFileInfo * FileInfo = GetFixedApplicationInfo();
  if (FileInfo)
  {
    return CalculateCompoundVersion(
      HIWORD(FileInfo->dwFileVersionMS), LOWORD(FileInfo->dwFileVersionMS),
      HIWORD(FileInfo->dwFileVersionLS), LOWORD(FileInfo->dwFileVersionLS));
  }
  return 0;
}

UnicodeString TConfiguration::ModuleFileName() const
{
  ThrowNotImplemented(204);
  return L"";
}

void * TConfiguration::GetFileApplicationInfo(const UnicodeString AFileName) const
{
  void * Result;
  if (AFileName.IsEmpty())
  {
    if (!FApplicationInfo)
    {
      FApplicationInfo = CreateFileInfo(ModuleFileName());
    }
    Result = FApplicationInfo;
  }
  else
  {
    Result = CreateFileInfo(AFileName);
  }
  return Result;
}

void * TConfiguration::GetApplicationInfo() const
{
  return GetFileApplicationInfo("");
}

UnicodeString TConfiguration::GetFileProductName(const UnicodeString AFileName) const
{
  return GetFileFileInfoString(L"ProductName", AFileName);
}

UnicodeString TConfiguration::GetFileCompanyName(const UnicodeString AFileName) const
{
  // particularly in IDE build, company name is empty
  return GetFileFileInfoString(L"CompanyName", AFileName, true);
}

UnicodeString TConfiguration::GetProductName() const
{
  return GetFileProductName(L"");
}

UnicodeString TConfiguration::GetCompanyName() const
{
  return GetFileCompanyName(L"");
}

UnicodeString TConfiguration::GetFileProductVersion(const UnicodeString AFileName) const
{
  return TrimVersion(GetFileFileInfoString(L"ProductVersion", AFileName));
}

UnicodeString TConfiguration::GetFileDescription(UnicodeString AFileName) const
{
  return GetFileFileInfoString(L"FileDescription", AFileName);
}

UnicodeString TConfiguration::GetFileProductVersion() const
{
  return GetFileProductVersion(L"");
}

UnicodeString TConfiguration::GetReleaseType() const
{
  return GetFileInfoString("ReleaseType");
}

bool TConfiguration::GetIsUnofficial() const
{
#ifdef BUILD_OFFICIAL
  return false;
#else
  return true;
#endif
}

UnicodeString TConfiguration::GetProductVersionStr() const
{
  UnicodeString Result;
  TGuard Guard(FCriticalSection);
  try
  {
    TVSFixedFileInfo * FixedApplicationInfo = GetFixedApplicationInfo();
#if 0
    return FMTLOAD(VERSION,
      HIWORD(Info->dwFileVersionMS),
      LOWORD(Info->dwFileVersionMS),
      HIWORD(Info->dwFileVersionLS),
      LOWORD(Info->dwFileVersionLS));
#endif // #if 0
    UnicodeString BuildStr;
    if (!GetIsUnofficial())
    {
      BuildStr = LoadStr(VERSION_BUILD);
    }
    else
    {
#ifdef _DEBUG
      BuildStr = LoadStr(VERSION_DEBUG_BUILD);
#else
      BuildStr = LoadStr(VERSION_DEV_BUILD);
#endif
    }

    int Build = LOWORD(FixedApplicationInfo->dwFileVersionLS);
    if (Build > 0)
    {
      BuildStr += L" " + ::IntToStr(Build);
    }

#if 0
#ifndef BUILD_OFFICIAL
    UnicodeString BuildDate = __DATE__;
    UnicodeString MonthStr = CutToChar(BuildDate, L' ', true);
    int Month = ParseShortEngMonthName(MonthStr);
    int Day = StrToInt64(CutToChar(BuildDate, L' ', true));
    int Year = StrToInt64(Trim(BuildDate));
    UnicodeString DateStr = FORMAT("%d-%2.2d-%2.2d", Year, Month, Day);
    AddToList(BuildStr, DateStr, L" ");
#endif
#endif

    UnicodeString FullVersion = GetProductVersion();

    UnicodeString AReleaseType = GetReleaseType();
    if (DebugAlwaysTrue(!AReleaseType.IsEmpty()) &&
        !SameText(AReleaseType, L"stable") &&
        !SameText(AReleaseType, L"development"))
    {
      FullVersion += L" " + AReleaseType;
    }

    Result = FMTLOAD(VERSION2, GetProductVersion(), Build);

#if 0
#ifndef BUILD_OFFICIAL
    Result += L" " + LoadStr(VERSION_DONT_DISTRIBUTE);
#endif
#endif
  }
  catch (Exception & E)
  {
    throw ExtException(&E, "Can't get application version");
  }
  return Result;
}

UnicodeString TConfiguration::GetFileVersion(UnicodeString AFileName)
{
  UnicodeString Result;
  void * FileInfo = CreateFileInfo(AFileName);
  try__finally
  {
    SCOPE_EXIT
    {
      FreeFileInfo(FileInfo);
    };
    Result = GetFileVersion(GetFixedFileInfo(FileInfo));
  }
  __finally
  {
#if 0
    FreeFileInfo(FileInfo);
#endif // #if 0
  };
  return Result;
}

UnicodeString TConfiguration::GetFileVersion(TVSFixedFileInfo * Info)
{
  TGuard Guard(FCriticalSection);
  try
  {
    UnicodeString Result =
      FormatVersion(
        HIWORD(Info->dwFileVersionMS),
        LOWORD(Info->dwFileVersionMS),
        HIWORD(Info->dwFileVersionLS));
    return Result;
  }
  catch (Exception & E)
  {
    throw ExtException(&E, L"Can't get file version");
  }
}

UnicodeString TConfiguration::GetProductVersion() const
{
  TGuard Guard(FCriticalSection);
  UnicodeString Result;
  try
  {
    TVSFixedFileInfo * FixedApplicationInfo = GetFixedApplicationInfo();
    if (FixedApplicationInfo)
    {
      Result = FormatVersion(
        HIWORD(FixedApplicationInfo->dwFileVersionMS),
        LOWORD(FixedApplicationInfo->dwFileVersionMS),
        HIWORD(FixedApplicationInfo->dwFileVersionLS));
    }
  }
  catch (Exception & E)
  {
    throw ExtException(&E, "Can't get application version");
  }
  return Result;
}

UnicodeString TConfiguration::GetVersion()
{
  return GetFileVersion(GetFixedApplicationInfo());
}

UnicodeString TConfiguration::GetFileFileInfoString(const UnicodeString AKey,
  const UnicodeString AFileName, bool AllowEmpty) const
{
  TGuard Guard(FCriticalSection);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(AFileName);
  try__finally
  {
    SCOPE_EXIT
    {
      if (!AFileName.IsEmpty() && Info)
      {
        FreeFileInfo(Info);
      }
    };
    if ((Info != nullptr) && (GetTranslationCount(Info) > 0))
    {
      TTranslation Translation = GetTranslation(Info, 0);
      try
      {
        Result = ::GetFileInfoString(Info, Translation, AKey, AllowEmpty);
      }
      catch (const std::exception & e)
      {
        (void)e;
        DEBUG_PRINTF("Error: %s", ::MB2W(e.what()));
        Result.Clear();
      }
    }
    else
    {
      DebugAssert(!AFileName.IsEmpty());
    }
  }
  __finally
  {
#if 0
    if (!AFileName.IsEmpty())
    {
      FreeFileInfo(Info);
    }
#endif // #if 0
  };
  return Result;
}

UnicodeString TConfiguration::GetFileInfoString(UnicodeString Key) const
{
  return GetFileFileInfoString(Key, L"");
}

UnicodeString TConfiguration::GetRegistryStorageKey() const
{
  return GetRegistryKey();
}

void TConfiguration::SetNulStorage()
{
  FStorage = stNul;
}

void TConfiguration::SetDefaultStorage()
{
  FStorage = stDetect;
}

#if 0
void TConfiguration::SetIniFileStorageName(UnicodeString Value)
{
  FIniFileStorageName = Value;
  FStorage = stIniFile;
}

UnicodeString TConfiguration::GetDefaultIniFileExportPath()
{
  UnicodeString PersonalDirectory = GetPersonalFolder();
  UnicodeString FileName = ::IncludeTrailingBackslash(PersonalDirectory) +
    ExtractFileName(ExpandEnvironmentVariables(IniFileStorageName));
  return FileName;
}

UnicodeString TConfiguration::GetIniFileStorageNameForReading()
{
  return GetIniFileStorageName(true);
}

UnicodeString TConfiguration::GetIniFileStorageNameForReadingWriting()
{
  return GetIniFileStorageName(false);
}

UnicodeString TConfiguration::GetAutomaticIniFileStorageName(bool ReadingOnly)
{
  UnicodeString ProgramPath = ParamStr(0);

  UnicodeString ProgramIniPath = ChangeFileExt(ProgramPath, L".ini");

  UnicodeString IniPath;
  if (::FileExists(ApiPath(ProgramIniPath)))
  {
    IniPath = ProgramIniPath;
  }
  else
  {
    UnicodeString AppDataIniPath =
      IncludeTrailingBackslash(GetShellFolderPath(CSIDL_APPDATA)) +
      ::ExtractFileName(ProgramIniPath);
    if (::FileExists(ApiPath(AppDataIniPath)))
    {
      IniPath = AppDataIniPath;
    }
    else
    {
      // avoid expensive test if we are interested in existing files only
      if (!ReadingOnly && (FProgramIniPathWritable < 0))
      {
        UnicodeString ProgramDir = ::ExtractFilePath(ProgramPath);
        FProgramIniPathWritable = IsDirectoryWriteable(ProgramDir) ? 1 : 0;
      }

      // does not really matter what we return when < 0
      IniPath = (FProgramIniPathWritable == 0) ? AppDataIniPath : ProgramIniPath;
    }
  }

  // BACKWARD COMPATIBILITY with 4.x
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
      ::FileExists(ApiPath(FVirtualIniFileStorageName)))
  {
    return FVirtualIniFileStorageName;
  }
  else
  {
    return IniPath;
  }
}

UnicodeString TConfiguration::GetIniFileStorageName(bool ReadingOnly)
{
  UnicodeString Result;
  if (!FIniFileStorageName.IsEmpty())
  {
    Result = FIniFileStorageName;
  }
  else if (!FCustomIniFileStorageName.IsEmpty())
  {
    Result = FCustomIniFileStorageName;
  }
  else
  {
    Result = GetAutomaticIniFileStorageName(ReadingOnly);
  }
  return Result;
}
#endif // #if 0

void TConfiguration::SetOptionsStorage(TStrings * Value)
{
  if (FOptionsStorage.get() == nullptr)
  {
    FOptionsStorage.reset(new TStringList());
  }
  FOptionsStorage->AddStrings(Value);
}

TStrings * TConfiguration::GetOptionsStorage()
{
  return FOptionsStorage.get();
}

UnicodeString TConfiguration::GetPuttySessionsKey() const
{
  return GetPuttyRegistryStorageKey() + "\\Sessions";
}

UnicodeString TConfiguration::GetStoredSessionsSubKey() const
{
  return "Sessions";
}

UnicodeString TConfiguration::GetSshHostKeysSubKey() const
{
  return "SshHostKeys";
}

UnicodeString TConfiguration::GetConfigurationSubKey() const
{
  return "Configuration";
}

UnicodeString TConfiguration::GetRootKeyStr() const
{
  return RootKeyToStr(HKEY_CURRENT_USER);
}

void TConfiguration::MoveStorage(TStorage AStorage, UnicodeString ACustomIniFileStorageName)
{
  if ((FStorage != AStorage) ||
      !IsPathToSameFile(FCustomIniFileStorageName, ACustomIniFileStorageName))
  {
    TStorage StorageBak = FStorage;
    UnicodeString CustomIniFileStorageNameBak = FCustomIniFileStorageName;
    try
    {
      std::unique_ptr<THierarchicalStorage> SourceStorage(CreateConfigStorage());
      std::unique_ptr<THierarchicalStorage> TargetStorage(CreateConfigStorage());
      try__finally
      {
        SourceStorage->SetAccessMode(smRead);

        FStorage = AStorage;
        FCustomIniFileStorageName = ACustomIniFileStorageName;

        TargetStorage->SetAccessMode(smReadWrite);
        TargetStorage->SetExplicit(true);

        // copy before save as it removes the ini file,
        // when switching from ini to registry
        CopyData(SourceStorage.get(), TargetStorage.get());
      }
      __finally
      {
#if 0
        delete SourceStorage;
        delete TargetStorage;
#endif // #if 0
      };
      // save all and explicit,
      // this also removes an INI file, when switching to registry storage
      DoSave(true, true);
    }
    catch (...)
    {
      // If this fails, do not pretend that storage was switched.
      // For instance:
      // - When writing to an INI file fails (unlikely, as we fallback to user profile)
      // - When removing INI file fails, when switching to registry
      //   (possible, when the INI file is in Program Files folder)
      FStorage = StorageBak;
      FCustomIniFileStorageName = CustomIniFileStorageNameBak;
      throw;
    }
  }
}

void TConfiguration::ScheduleCustomIniFileStorageUse(UnicodeString ACustomIniFileStorageName)
{
  FStorage = stIniFile;
  FCustomIniFileStorageName = ACustomIniFileStorageName;
  SaveCustomIniFileStorageName();
}

void TConfiguration::Saved()
{
  // nothing
}

TStorage TConfiguration::GetStorage() const
{
  TGuard Guard(FCriticalSection);
  if (FStorage == stDetect)
  {
#if 0
    if (::FileExists(ApiPath(IniFileStorageNameForReading)))
    {
      FStorage = stIniFile;
    }
    else
#endif // #if 0
    {
      FStorage = stRegistry;
    }
  }
  return FStorage;
}

TStoredSessionList * TConfiguration::SelectFilezillaSessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  UnicodeString AppDataPath;
#if defined(_MSC_VER) && !defined(__clang__)
  AppDataPath = GetShellFolderPath(CSIDL_APPDATA);
#endif // if defined(_MSC_VER) && !defined(__clang__)
  UnicodeString FilezillaSiteManagerFile =
    ::IncludeTrailingBackslash(AppDataPath) + L"FileZilla\\sitemanager.xml";
  UnicodeString FilezillaConfigurationFile =
    ::IncludeTrailingBackslash(AppDataPath) + L"FileZilla\\filezilla.xml";

  if (FileExists(ApiPath(FilezillaSiteManagerFile)))
  {
    ImportSessionList->ImportFromFilezilla(FilezillaSiteManagerFile, FilezillaConfigurationFile);

    if (ImportSessionList->GetCount() > 0)
    {
      ImportSessionList->SelectSessionsToImport(Sessions, true);
    }
    else
    {
      Error = FMTLOAD(FILEZILLA_NO_SITES, FilezillaSiteManagerFile);
    }
  }
  else
  {
    Error = FMTLOAD(FILEZILLA_SITE_MANAGER_NOT_FOUND, FilezillaSiteManagerFile);
  }

  return ImportSessionList.release();
}

bool TConfiguration::AnyFilezillaSessionForImport(TStoredSessionList * Sessions)
{
  try
  {
    UnicodeString Error;
    std::unique_ptr<TStoredSessionList> SessionsList(SelectFilezillaSessionsForImport(Sessions, Error));
    return (SessionsList->GetCount() > 0);
  }
  catch (...)
  {
    return false;
  }
}

TStoredSessionList * TConfiguration::SelectKnownHostsSessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  UnicodeString ProfilePath;
#if defined(_MSC_VER) && !defined(__clang__)
  ProfilePath = GetShellFolderPath(CSIDL_PROFILE);
#endif // if defined(_MSC_VER) && !defined(__clang__)
  UnicodeString KnownHostsFile = IncludeTrailingBackslash(ProfilePath) + L".ssh\\known_hosts";

  try
  {
    if (FileExists(ApiPath(KnownHostsFile)))
    {
      std::unique_ptr<TStrings> Lines(new TStringList());
#if 0
      LoadScriptFromFile(KnownHostsFile, Lines.get());
#endif // if 0
      ImportSessionList->ImportFromKnownHosts(Lines.get());
    }
    else
    {
      throw Exception(LoadStr(KNOWN_HOSTS_NOT_FOUND));
    }
  }
  catch (Exception & E)
  {
    Error = FORMAT(L"%s\n(%s)", E.Message, KnownHostsFile);
  }

  return ImportSessionList.release();
}

TStoredSessionList * TConfiguration::SelectKnownHostsSessionsForImport(
  TStrings * Lines, TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(new TStoredSessionList(true));
  ImportSessionList->SetDefaultSettings(Sessions->GetDefaultSettings());

  try
  {
    ImportSessionList->ImportFromKnownHosts(Lines);
  }
  catch (Exception & E)
  {
    Error = E.Message;
  }

  return ImportSessionList.release();
}

void TConfiguration::SetRandomSeedFile(UnicodeString Value)
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
        ::FileExists(ApiPath(PrevRandomSeedFileName)))
    {
      // ignore any error
      ::RemoveFile(ApiPath(PrevRandomSeedFileName));
    }
  }
}

UnicodeString TConfiguration::GetRandomSeedFileName() const
{
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  return StripPathQuotes(::ExpandEnvironmentVariables(FRandomSeedFile)).Trim();
}

void TConfiguration::SetExternalIpAddress(UnicodeString Value)
{
  SET_CONFIG_PROPERTY(ExternalIpAddress);
}

void TConfiguration::SetTryFtpWhenSshFails(bool Value)
{
  SET_CONFIG_PROPERTY(TryFtpWhenSshFails);
}

void TConfiguration::SetParallelDurationThreshold(intptr_t Value)
{
  SET_CONFIG_PROPERTY(ParallelDurationThreshold);
}

void TConfiguration::SetPuttyRegistryStorageKey(UnicodeString Value)
{
  SET_CONFIG_PROPERTY(PuttyRegistryStorageKey);
}

TEOLType TConfiguration::GetLocalEOLType() const
{
  return eolCRLF;
}

bool TConfiguration::GetCollectUsage() const
{
  return false; // FUsage->Collect;
}

void TConfiguration::SetCollectUsage(bool /*Value*/)
{
  // FUsage->Collect = Value;
}

void TConfiguration::TemporaryLogging(const UnicodeString ALogFileName)
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

void TConfiguration::TemporaryActionsLogging(const UnicodeString ALogFileName)
{
  FLogActions = true;
  FActionsLogFileName = ALogFileName;
}

void TConfiguration::TemporaryLogProtocol(intptr_t ALogProtocol)
{
  FLogProtocol = ALogProtocol;
  UpdateActualLogProtocol();
}

void TConfiguration::TemporaryLogSensitive(bool ALogSensitive)
{
  FLogSensitive = ALogSensitive;
}

void TConfiguration::TemporaryLogMaxSize(int64_t ALogMaxSize)
{
  FLogMaxSize = ALogMaxSize;
}

void TConfiguration::TemporaryLogMaxCount(intptr_t ALogMaxCount)
{
  FLogMaxCount = ALogMaxCount;
}

void TConfiguration::SetLogging(bool Value)
{
  TGuard Guard(FCriticalSection);
  if (GetLogging() != Value)
  {
    FPermanentLogging = Value;
    FLogging = Value;
    UpdateActualLogProtocol();
    Changed();
  }
}

bool TConfiguration::GetLogging() const
{
  TGuard Guard(FCriticalSection);
  return FPermanentLogging;
}

void TConfiguration::SetLogFileName(UnicodeString Value)
{
  TGuard Guard(FCriticalSection);
  if (GetLogFileName() != Value)
  {
    FPermanentLogFileName = Value;
    FLogFileName = Value;
    Changed();
  }
}

UnicodeString TConfiguration::GetLogFileName() const
{
  TGuard Guard(FCriticalSection);
  return FPermanentLogFileName;
}

void TConfiguration::SetActionsLogFileName(UnicodeString Value)
{
  TGuard Guard(FCriticalSection);
  if (GetActionsLogFileName() != Value)
  {
    FPermanentActionsLogFileName = Value;
    FActionsLogFileName = Value;
    Changed();
  }
}

UnicodeString TConfiguration::GetPermanentActionsLogFileName() const
{
  TGuard Guard(FCriticalSection);
  return FPermanentActionsLogFileName;
}

UnicodeString TConfiguration::GetActionsLogFileName() const
{
  TGuard Guard(FCriticalSection);
  return FActionsLogFileName;
}

bool TConfiguration::GetLogToFile() const
{
  // guarded within GetLogFileName
  return !GetLogFileName().IsEmpty();
}

void TConfiguration::UpdateActualLogProtocol()
{
  FActualLogProtocol = FLogging ? FLogProtocol : 0;
}

void TConfiguration::SetLogProtocol(intptr_t Value)
{
  TGuard Guard(FCriticalSection);
  if (GetLogProtocol() != Value)
  {
    FPermanentLogProtocol = Value;
    FLogProtocol = Value;
    Changed();
    UpdateActualLogProtocol();
  }
}

void TConfiguration::SetLogActions(bool Value)
{
  TGuard Guard(FCriticalSection);
  if (GetLogActions() != Value)
  {
    FPermanentLogActions = Value;
    FLogActions = Value;
    Changed();
  }
}

bool TConfiguration::GetLogActions() const
{
  TGuard Guard(FCriticalSection);
  return FPermanentLogActions;
}

void TConfiguration::SetLogFileAppend(bool Value)
{
  SET_CONFIG_PROPERTY(LogFileAppend);
}

void TConfiguration::SetLogSensitive(bool Value)
{
  if (GetLogSensitive() != Value)
  {
    FPermanentLogSensitive = Value;
    FLogSensitive = Value;
    Changed();
  }
}

void TConfiguration::SetLogMaxSize(int64_t Value)
{
  TGuard Guard(FCriticalSection);
  if (GetLogMaxSize() != Value)
  {
    FPermanentLogMaxSize = Value;
    FLogMaxSize = Value;
    Changed();
  }
}

int64_t TConfiguration::GetLogMaxSize() const
{
  TGuard Guard(FCriticalSection);
  return FPermanentLogMaxSize;
}

void TConfiguration::SetLogMaxCount(intptr_t Value)
{
  if (GetLogMaxCount() != Value)
  {
    FPermanentLogMaxCount = Value;
    FLogMaxCount = Value;
    Changed();
  }
}

intptr_t TConfiguration::GetLogMaxCount() const
{
  TGuard Guard(FCriticalSection);
  return FPermanentLogMaxCount;
}

UnicodeString TConfiguration::GetDefaultLogFileName() const
{
  return "%TEMP%\\&S.log";
}

void TConfiguration::SetConfirmOverwriting(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmOverwriting);
}

bool TConfiguration::GetConfirmOverwriting() const
{
  TGuard Guard(FCriticalSection);
  return FConfirmOverwriting;
}

void TConfiguration::SetConfirmResume(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(ConfirmResume);
}

bool TConfiguration::GetConfirmResume() const
{
  TGuard Guard(FCriticalSection);
  return FConfirmResume;
}

void TConfiguration::SetAutoReadDirectoryAfterOp(bool Value)
{
  TGuard Guard(FCriticalSection);
  SET_CONFIG_PROPERTY(AutoReadDirectoryAfterOp);
}

bool TConfiguration::GetAutoReadDirectoryAfterOp() const
{
  TGuard Guard(FCriticalSection);
  return FAutoReadDirectoryAfterOp;
}

UnicodeString TConfiguration::TimeFormat() const
{
  return "h:nn:ss";
}

UnicodeString TConfiguration::GetPartialExt() const
{
  return PARTIAL_EXT;
}

UnicodeString TConfiguration::GetDefaultKeyFile() const
{
  return L"";
}

bool TConfiguration::GetRememberPassword() const
{
  return false;
}

void TConfiguration::SetSessionReopenAuto(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}

void TConfiguration::SetSessionReopenBackground(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}

void TConfiguration::SetSessionReopenTimeout(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenTimeout);
}

void TConfiguration::SetSessionReopenAutoStall(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoStall);
}

void TConfiguration::SetTunnelLocalPortNumberLow(intptr_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}

void TConfiguration::SetTunnelLocalPortNumberHigh(intptr_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberHigh);
}

void TConfiguration::SetCacheDirectoryChangesMaxSize(intptr_t Value)
{
  SET_CONFIG_PROPERTY(CacheDirectoryChangesMaxSize);
}

void TConfiguration::SetShowFtpWelcomeMessage(bool Value)
{
  SET_CONFIG_PROPERTY(ShowFtpWelcomeMessage);
}

bool TConfiguration::GetPersistent() const
{
  return (GetStorage() != stNul);
}

void TConfiguration::SetSessionReopenAutoMaximumNumberOfRetries(intptr_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoMaximumNumberOfRetries);
}


void TShortCuts::Add(const TShortCut & ShortCut)
{
  FShortCuts.push_back(ShortCut);
}

bool TShortCuts::Has(const TShortCut & ShortCut) const
{
  rde::vector<TShortCut>::iterator it = const_cast<TShortCuts *>(this)->FShortCuts.find(ShortCut);
  return (it != FShortCuts.end());
}

