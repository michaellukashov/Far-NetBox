﻿
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
#include "FileMasks.h"
#include "CopyParam.h"
#include <System.ShlObj.hpp>
#include <System.IOUtils.hpp>
#include <System.StrUtils.hpp>
#include <System.DateUtils.hpp>

// #pragma package(smart_init)

// const wchar_t * AutoSwitchNames = L"On;Off;Auto";
// const wchar_t * NotAutoSwitchNames = L"Off;On;Auto";

// See https://www.iana.org/assignments/hash-function-text-names/hash-function-text-names.xhtml
//const UnicodeString Sha1ChecksumAlg("sha-1");
//const UnicodeString Sha224ChecksumAlg("sha-224");
//const UnicodeString Sha256ChecksumAlg("sha-256");
//const UnicodeString Sha384ChecksumAlg("sha-384");
//const UnicodeString Sha512ChecksumAlg("sha-512");
//const UnicodeString Md5ChecksumAlg("md5");
//// Not defined by IANA
//const UnicodeString Crc32ChecksumAlg("crc32");

//const UnicodeString SshFingerprintType("ssh");
//const UnicodeString TlsFingerprintType("tls");

//const UnicodeString FtpsCertificateStorageKey(L"FtpsCertificates");
//const UnicodeString HttpsCertificateStorageKey(L"HttpsCertificates");
//const UnicodeString LastFingerprintsStorageKey(L"LastFingerprints");
//const UnicodeString DirectoryStatisticsCacheKey(L"DirectoryStatisticsCache");
//const UnicodeString SshHostCAsKey(L"SshHostCAs");
//const UnicodeString CDCacheKey(L"CDCache");
//const UnicodeString BannersKey(L"Banners");

//const UnicodeString OpensshFolderName(L".ssh");
//const UnicodeString OpensshAuthorizedKeysFileName(L"authorized_keys");

//const int32_t BelowNormalLogLevels = 1;

TSshHostCA::TSshHostCA()
{
  PermitRsaSha1 = false;
  PermitRsaSha256 = true;
  PermitRsaSha512 = true;
}

bool TSshHostCA::Load(THierarchicalStorage * Storage)
{
  PublicKey = DecodeBase64ToStr(Storage->ReadString(L"PublicKey", PublicKey));
  ValidityExpression = Storage->ReadString(L"Validity", ValidityExpression);
  PermitRsaSha1 = Storage->ReadBool(L"PermitRSASHA1", PermitRsaSha1);
  PermitRsaSha256 = Storage->ReadBool(L"PermitRSASHA256", PermitRsaSha256);
  PermitRsaSha512 = Storage->ReadBool(L"PermitRSASHA512", PermitRsaSha512);
  return !PublicKey.IsEmpty() && !ValidityExpression.IsEmpty();
}

void TSshHostCA::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteString(L"PublicKey", EncodeStrToBase64(PublicKey));
  Storage->WriteString(L"Validity", ValidityExpression);
  Storage->WriteBool(L"PermitRSASHA1", PermitRsaSha1);
  Storage->WriteBool(L"PermitRSASHA256", PermitRsaSha256);
  Storage->WriteBool(L"PermitRSASHA512", PermitRsaSha512);
}


/*TSshHostCAList::TSshHostCAList()
{
}*/

TSshHostCAList::TSshHostCAList(const TSshHostCA::TList & List) :
  FList(List)
{

}

void TSshHostCAList::Default()
{
  FList.clear();
}

void TSshHostCAList::Save(THierarchicalStorage * Storage)
{
  Storage->ClearSubKeys();
  TSshHostCA::TList::const_iterator I = FList.begin();
  while (I != FList.end())
  {
    const TSshHostCA & SshHostCA = *I;
    if (Storage->OpenSubKey(SshHostCA.Name, true))
    {
      SshHostCA.Save(Storage);
      Storage->CloseSubKey();
    }
    ++I;
  }
}

void TSshHostCAList::Load(THierarchicalStorage * Storage)
{
  FList.clear();
  std::unique_ptr<TStrings> SubKeys(std::make_unique<TStringList>());
  Storage->GetSubKeyNames(SubKeys.get());

  for (int32_t Index = 0; Index < SubKeys->Count; Index++)
  {
    TSshHostCA SshHostCA;
    SshHostCA.Name = SubKeys->Strings[Index];
    if (Storage->OpenSubKey(SshHostCA.Name, false))
    {
      if (SshHostCA.Load(Storage))
      {
        FList.push_back(SshHostCA);
      }

      Storage->CloseSubKey();
    }
  }
}

int32_t TSshHostCAList::GetCount() const
{
  return nb::ToInt32(FList.size());
}

const TSshHostCA * TSshHostCAList::Get(int32_t Index) const
{
  return &FList[Index];
}

const TSshHostCA * TSshHostCAList::Find(const UnicodeString & Name) const
{
  TSshHostCA::TList::const_iterator I = FList.begin();
  while (I != FList.end())
  {
    const TSshHostCA & SshHostCA = *I;
    if (SameStr(SshHostCA.Name, Name))
    {
      return &SshHostCA;
    }
    ++I;
  }
  return nullptr;
}

const TSshHostCA::TList & TSshHostCAList::GetList() const
{
  return FList;
}

TSshHostCAList & TSshHostCAList::operator =(const TSshHostCAList & other)
{
  FList = other.FList;
  return *this;
}


TConfiguration::TConfiguration(TObjectClassId Kind) noexcept :
  TObject(Kind)
{
  // FCriticalSection = new TCriticalSection();
  FUpdating = 0;
  FStorage = stDetect;
  FDontSave = false;
  FForceSave = false;
  FApplicationInfo = nullptr;
  // FUsage = std::make_unique<TUsage>(this);
  FDefaultCollectUsage = false;
  FScripting = false;
  FSshHostCAList = std::make_unique<TSshHostCAList>();

  UnicodeString RandomSeedPath;
  if (!base::GetEnvVariable("APPDATA").IsEmpty())
  {
    RandomSeedPath = "%APPDATA%";
  }
  else
  {
#if defined(_MSC_VER)
    RandomSeedPath = ::GetShellFolderPath(CSIDL_LOCAL_APPDATA);
    if (RandomSeedPath.IsEmpty())
    {
      RandomSeedPath = ::GetShellFolderPath(CSIDL_APPDATA);
    }
#endif // if defined(_MSC_VER)
  }

  FDefaultRandomSeedFile = ::IncludeTrailingBackslash(RandomSeedPath) + "winscp.rnd";
}

void TConfiguration::Default()
{
  TGuard Guard(FCriticalSection); nb::used(Guard);

  FDisablePasswordStoring = false;
  FForceBanners = false;
  FDisableAcceptingHostKeys = false;

  // TODO: use TFar3Storage
  std::unique_ptr<TRegistryStorage> AdminStorage(std::make_unique<TRegistryStorage>(GetRegistryStorageKey(), HKEY_LOCAL_MACHINE));
  AdminStorage->Init();
  try__finally
  {
    if (AdminStorage->OpenRootKey(false))
    {
      LoadAdmin(AdminStorage.get());
      AdminStorage->CloseSubKey();
    }
  }
  __finally__removed
  {
    // delete AdminStorage;
  } end_try__finally

  SetRandomSeedFile(FDefaultRandomSeedFile);
  SetPuttyRegistryStorageKey(OriginalPuttyRegistryStorageKey);
  FConfirmOverwriting = true;
  FConfirmResume = true;
  FAutoReadDirectoryAfterOp = true;
  FSessionReopenAuto = 5000;
  FSessionReopenBackground = 2000;
  FSessionReopenTimeout = 0;
  FSessionReopenAutoStall = 10 * MSecsPerSec;
  FTunnelLocalPortNumberLow = 50000;
  FTunnelLocalPortNumberHigh = 50099;
  FCacheDirectoryChangesMaxSize = 100;
  FShowFtpWelcomeMessage = false;
  FExternalIpAddress.Clear();
  FLocalPortNumberMin = 0;
  FLocalPortNumberMax = 0;
  FTryFtpWhenSshFails = true;
  FParallelDurationThreshold = 10;
  SetCollectUsage(FDefaultCollectUsage);
  FMimeTypes = UnicodeString();
  FCertificateStorage = EmptyStr;
  FAWSMetadataService = EmptyStr;
  FChecksumCommands = EmptyStr;
  FDontReloadMoreThanSessions = 1000;
  FScriptProgressFileNameLimit = 25;
  FQueueTransfersLimit = 2;
  FParallelTransferThreshold = -1; // default (currently off), 0 = explicitly off
  FKeyVersion = 0;
  FSshHostCAList->Default();
  RefreshPuttySshHostCAList();
  FSshHostCAsFromPuTTY = false;
  FHttpsCertificateValidation = 0;
  FSynchronizationChecksumAlgs = EmptyStr;
  CollectUsage = FDefaultCollectUsage;

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

TConfiguration::~TConfiguration() noexcept
{
  DebugAssert(!FUpdating);
  if (FApplicationInfo)
  {
    // FreeFileInfo(FApplicationInfo); TODO: check
    FApplicationInfo = nullptr;
  }
  // delete FCriticalSection;
  // delete FUsage;
}

void TConfiguration::ConfigurationInit()
{
  FUsage = std::make_unique<TUsage>(this);
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
  return CreateScpStorage(SessionList);
}

THierarchicalStorage * TConfiguration::CreateConfigRegistryStorage()
{
  return new TRegistryStorage(RegistryStorageKey);
}

THierarchicalStorage * TConfiguration::CreateScpStorage(bool & SessionList)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  THierarchicalStorage * Result = nullptr;
  if (GetStorage() == stRegistry)
  {
    Result = CreateConfigRegistryStorage();
  }
  else if (GetStorage() == stNul)
  {
    // Result = TIniFileStorage::CreateNul();
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

  if ((FOptionsStorage != nullptr) && (FOptionsStorage->GetCount() > 0))
  {
    if (!SessionList)
    {
      // Result = new TOptionsStorage(FOptionsStorage.get(), ConfigurationSubKey, Result);
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

UnicodeString TConfiguration::PropertyToKey(const UnicodeString & Property)
{
  // no longer useful
  const int32_t P = Property.LastDelimiter(L".>");
  UnicodeString Result = Property.SubString(P + 1, Property.Length() - P);
  if ((Result[1] == L'F') && (towupper(Result[2]) == Result[2]))
  {
    Result.Delete(1, 1);
  }
  return Result;
}
#undef LASTELEM
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>") + 1, ELEM.Length() - ELEM.LastDelimiter(L".>"))

#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (AStorage->OpenSubKeyPath(KEY, CANCREATE)) \
    { try__finally { BLOCK } __finally { AStorage->CloseSubKeyPath(); } end_try__finally }
#undef KEY4
#undef KEY3
#undef KEY2
#undef KEY
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, VAR)
#define KEY2(TYPE, VAR) KEYEX2(TYPE, VAR, VAR)
#define KEY3(TYPE, VAR) KEYEX3(TYPE, VAR, VAR)
#define KEY4(TYPE, VAR) KEYEX4(TYPE, VAR, VAR)
#define KEY5(TYPE, VAR) KEYEX5(TYPE, VAR, VAR)
#undef REGCONFIG
#define REGCONFIG(CANCREATE) \
  BLOCK("Interface", CANCREATE, \
    KEY(String,   RandomSeedFile); \
    KEY(String,   PuttyRegistryStorageKey); \
    KEY(Bool,     ConfirmOverwriting); \
    KEY(Bool,     ConfirmResume); \
    KEY(Bool,     AutoReadDirectoryAfterOp); \
    KEY3(Integer,  SessionReopenAuto); \
    KEY3(Integer,  SessionReopenBackground); \
    KEY3(Integer,  SessionReopenTimeout); \
    KEY3(Integer,  TunnelLocalPortNumberLow); \
    KEY3(Integer,  TunnelLocalPortNumberHigh); \
    KEY3(Integer,  CacheDirectoryChangesMaxSize); \
    KEY(Bool,     ShowFtpWelcomeMessage); \
    KEY(String,   ExternalIpAddress); \
    KEY4(Integer,  LocalPortNumberMin); \
    KEY4(Integer,  LocalPortNumberMax); \
    KEY(Bool,     TryFtpWhenSshFails); \
    KEY3(Integer,  ParallelDurationThreshold); \
    KEY(String,   MimeTypes); \
    KEY4(Integer,  DontReloadMoreThanSessions); \
    KEY4(Integer,  ScriptProgressFileNameLimit); \
    KEY4(Integer,  QueueTransfersLimit); \
    KEY4(Integer,  ParallelTransferThreshold); \
    KEY4(Integer,  KeyVersion); \
    KEY4(Bool,     SshHostCAsFromPuTTY); \
    KEY4(Integer,  HttpsCertificateValidation); \
    KEY5(String,   SynchronizationChecksumAlgs); \
    KEY(Bool,     CollectUsage); \
    KEY3(Integer,  SessionReopenAutoMaximumNumberOfRetries); \
    KEY5(String,   CertificateStorage); \
    KEY5(String,   AWSMetadataService); \
  ); \
  BLOCK("Logging", CANCREATE, \
    KEYEX(Bool,  PermanentLogging, Logging); \
    KEYEX(String,PermanentLogFileName, LogFileName); \
    KEY(Bool,    LogFileAppend); \
    KEYEX(Bool,  PermanentLogSensitive, LogSensitive); \
    KEYEX(Int64, PermanentLogMaxSize, LogMaxSize); \
    KEYEX3(Integer, PermanentLogMaxCount, LogMaxCount); \
    KEYEX3(Integer,PermanentLogProtocol, LogProtocol); \
    KEYEX(Bool,  PermanentLogActions, LogActions); \
    KEYEX(String,PermanentActionsLogFileName, ActionsLogFileName); \
  )

void TConfiguration::SaveData(THierarchicalStorage * AStorage, bool /*All*/)
{
#define KEYEX(TYPE, NAME, VAR) AStorage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR())
#define KEYEX2(TYPE, NAME, VAR) AStorage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), F ## VAR)
#define KEYEX3(TYPE, NAME, VAR) AStorage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), nb::ToInt32(Get ## VAR()))
#define KEYEX4(TYPE, NAME, VAR) AStorage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), nb::ToInt32(F ## VAR))
#define KEYEX5(TYPE, NAME, VAR) AStorage->Write ## TYPE(LASTELEM(UnicodeString(#NAME)), F ## VAR)
  REGCONFIG(true)
#undef KEYEX5
#undef KEYEX4
#undef KEYEX3
#undef KEYEX2
#undef KEYEX

  if (AStorage->OpenSubKey("Usage", true))
  {
    FUsage->Save(AStorage);
    AStorage->CloseSubKey();
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

void TConfiguration::DoSave(THierarchicalStorage * AStorage, bool All)
{
  if (AStorage->OpenSubKey(ConfigurationSubKey, true))
  {
    SaveData(AStorage, All);
    AStorage->CloseSubKey();
  }

  if (AStorage->OpenSubKey(SshHostCAsKey, true))
  {
    FSshHostCAList->Save(AStorage);
    AStorage->CloseSubKey();
  }
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
    Storage->ForceSave = FForceSave;
    // if saving to TOptionsStorage, make sure we save everything so that
    // all configuration is properly transferred to the master storage
    bool ConfigAll = All || Storage->Temporary;
    DoSave(Storage.get(), ConfigAll);
  }
  __finally__removed
  {
    // delete Storage;
  } end_try__finally

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
    std::unique_ptr<TRegistryStorage> RegistryStorage(std::make_unique<TRegistryStorage>(GetRegistryStorageOverrideKey()));
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

void TConfiguration::Export(const UnicodeString & /*AFileName*/)
{
  ThrowNotImplemented(3004);
#if 0
  // not to "append" the export to an existing file
  if (FileExists(AFileName))
  {
    DeleteFileChecked(AFileName);
  }

  THierarchicalStorage * Storage = nullptr;
  THierarchicalStorage * ExportStorage = nullptr;
  try
  {
    ExportStorage = TIniFileStorage::CreateFromPath(AFileName);
    ExportStorage->AccessMode = smReadWrite;
    ExportStorage->Explicit = true;

    Storage = CreateConfigStorage();
    Storage->AccessMode = smRead;

    CopyData(Storage, ExportStorage);

    DoSave(ExportStorage, true);
  }
  __finally
  {
    delete ExportStorage;
    delete Storage;
  }

  StoredSessions->Export(AFileName);
#endif // #if 0
}

void TConfiguration::Import(const UnicodeString & /*AFileName*/)
{
  ThrowNotImplemented(3005);
#if 0
  THierarchicalStorage * Storage = nullptr;
  THierarchicalStorage * ImportStorage = nullptr;
  try
  {
    ImportStorage = TIniFileStorage::CreateFromPath(AFileName);
    ImportStorage->AccessMode = smRead;

    Storage = CreateConfigStorage();
    Storage->AccessMode = smReadWrite;
    Storage->Explicit = true;

    CopyData(ImportStorage, Storage);

    Default();
    LoadFrom(ImportStorage);

    Storage->RecursiveDeleteSubKey(Configuration->StoredSessionsSubKey);

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
  FDontSave = true;
}

void TConfiguration::LoadData(THierarchicalStorage * AStorage)
{
#define KEYEX(TYPE, NAME, VAR) Set ## VAR(AStorage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), Get ## VAR()))
#define KEYEX2(TYPE, NAME, VAR) VAR = AStorage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), F ## VAR)
#define KEYEX3(TYPE, NAME, VAR) Set ## VAR(AStorage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), nb::ToInt32(Get ## VAR())))
#define KEYEX4(TYPE, NAME, VAR) F ## VAR = AStorage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), nb::ToInt32(F ## VAR))
#define KEYEX5(TYPE, NAME, VAR) F ## VAR = AStorage->Read ## TYPE(LASTELEM(UnicodeString(#NAME)), F ## VAR)
  REGCONFIG(false);
#undef KEYEX5
#undef KEYEX4
#undef KEYEX3
#undef KEYEX2
#undef KEYEX

  if (AStorage->OpenSubKey("Usage", false))
  {
    FUsage->Load(AStorage);
    AStorage->CloseSubKey();
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

void TConfiguration::LoadSshHostCAList(TSshHostCAList * SshHostCAList, THierarchicalStorage * AStorage)
{
  if (AStorage->OpenSubKey(SshHostCAsKey, false))
  {
    SshHostCAList->Load(AStorage);
    AStorage->CloseSubKey();
  }
}

void TConfiguration::LoadFrom(THierarchicalStorage * AStorage)
{
  if (AStorage->OpenSubKey(GetConfigurationSubKey(), false))
  {
    LoadData(AStorage);
    AStorage->CloseSubKey();
  }
  LoadSshHostCAList(FSshHostCAList.get(), AStorage);
}

UnicodeString TConfiguration::GetRegistryStorageOverrideKey() const
{
  return GetRegistryStorageKey() + " Override";
}

UnicodeString TConfiguration::LoadCustomIniFileStorageName()
{
  UnicodeString Result;
  std::unique_ptr<TRegistryStorage> RegistryStorage(std::make_unique<TRegistryStorage>(GetRegistryStorageOverrideKey()));
  RegistryStorage->Init();
  if (RegistryStorage->OpenRootKey(false))
  {
    Result = RegistryStorage->ReadString("IniFile", "");
    RegistryStorage->CloseSubKey();
  }
  RegistryStorage.reset(nullptr);
  return Result;
}

void TConfiguration::Load(THierarchicalStorage * Storage)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  const TStorageAccessMode StorageAccessMode = Storage->GetAccessMode();
  try__finally
  {
    Storage->SetAccessMode(smRead);
    LoadFrom(Storage);
  }
  __finally
  {
    Storage->SetAccessMode(StorageAccessMode);
  } end_try__finally
}

bool TConfiguration::CopySubKey(THierarchicalStorage * Source, THierarchicalStorage * Target, const UnicodeString & Name)
{
  bool Result = Source->OpenSubKey(Name, false);
  if (Result)
  {
    Result = Target->OpenSubKey(Name, true);
    if (!Result)
    {
      Source->CloseSubKey();
    }
  }
  return Result;
}

void TConfiguration::CopyAllStringsInSubKey(
  THierarchicalStorage * Source, THierarchicalStorage * Target, const UnicodeString & Name)
{
  if (CopySubKey(Source, Target, Name))
  {
    std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
    Source->GetValueNames(Names.get());

    for (int32_t Index = 0; Index < Names->Count; Index++)
    {
      UnicodeString Buf = Source->ReadStringRaw(Names->GetString(Index), UnicodeString());
      Target->WriteStringRaw(Names->GetString(Index), Buf);
    }

    Target->CloseSubKey();
    Source->CloseSubKey();
  }
}

void TConfiguration::CopyData(THierarchicalStorage * Source,
  THierarchicalStorage * Target)
{
  if (CopySubKey(Source, Target, ConfigurationSubKey))
  {
    if (CopySubKey(Source, Target, CDCacheKey))
    {
      std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
      Source->GetValueNames(Names.get());

      for (int32_t Index = 0; Index < Names->GetCount(); ++Index)
      {
        UnicodeString Name = Names->GetString(Index);
        Target->WriteBinaryData(Name, Source->ReadBinaryData(Name));
      }

      Target->CloseSubKey();
      Source->CloseSubKey();
    }

    CopyAllStringsInSubKey(Source, Target, BannersKey);
    CopyAllStringsInSubKey(Source, Target, LastFingerprintsStorageKey);

    Target->CloseSubKey();
    Source->CloseSubKey();
  }

  CopyAllStringsInSubKey(Source, Target, SshHostKeysSubKey);
  CopyAllStringsInSubKey(Source, Target, FtpsCertificateStorageKey);
  CopyAllStringsInSubKey(Source, Target, HttpsCertificateStorageKey);
}

void TConfiguration::LoadDirectoryChangesCache(const UnicodeString & SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smRead);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
        Storage->OpenSubKey(CDCacheKey, false) &&
        Storage->ValueExists(SessionKey))
    {
      DirectoryChangesCache->Deserialize(Storage->ReadBinaryData(SessionKey));
    }
  }
  __finally__removed
  {
    // delete Storage;
  } end_try__finally
}

void TConfiguration::SaveDirectoryChangesCache(const UnicodeString & SessionKey,
  TRemoteDirectoryChangesCache * DirectoryChangesCache)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  try__finally
  {
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
        Storage->OpenSubKey(CDCacheKey, true))
    {
      UnicodeString Data;
      DirectoryChangesCache->Serialize(Data);
      Storage->WriteBinaryData(SessionKey, Data);
    }
  }
  __finally__removed
  {
    // delete Storage;
  } end_try__finally
}

UnicodeString TConfiguration::BannerHash(const UnicodeString & ABanner) const
{
  RawByteString Result(16, 0);
  char * Buf = Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(ABanner.c_str()), nb::ToInt32(ABanner.Length() * sizeof(wchar_t)),
    nb::ToUInt8Ptr(Buf));
  return BytesToHex(Result);
}

void TConfiguration::GetBannerData(
  const UnicodeString & ASessionKey, UnicodeString & ABannerHash, uint32_t & AParams)
{
  ABannerHash = UnicodeString();
  AParams = 0;

  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smRead);
  if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
      Storage->OpenSubKey(BannersKey, false))
  {
    UnicodeString S = Storage->ReadString(ASessionKey, "");
    ABannerHash = CutToChar(S, L',', true);
    AParams = StrToIntDef(UnicodeString("$") + CutToChar(S, L',', true), 0);
  }
}

bool TConfiguration::ShowBanner(
  const UnicodeString & ASessionKey, const UnicodeString & ABanner, uint32_t & AParams)
{
  UnicodeString StoredBannerHash;
  GetBannerData(ASessionKey, StoredBannerHash, AParams);
  const bool Result = (StoredBannerHash != BannerHash(ABanner));
  return Result;
}

void TConfiguration::SetBannerData(
  const UnicodeString & ASessionKey, const UnicodeString & ABannerHash, uint32_t Params)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
      Storage->OpenSubKey(BannersKey, true))
  {
    Storage->WriteString(ASessionKey, ABannerHash + "," + ::UIntToStr(Params));
  }
}

void TConfiguration::NeverShowBanner(const UnicodeString & ASessionKey, const UnicodeString & ABanner)
{
  UnicodeString DummyBannerHash;
  uint32_t Params;
  GetBannerData(ASessionKey, DummyBannerHash, Params);
  SetBannerData(ASessionKey, BannerHash(ABanner), Params);
}

void TConfiguration::SetBannerParams(const UnicodeString & ASessionKey, uint32_t AParams)
{
  UnicodeString BannerHash;
  uint32_t DummyParams;
  GetBannerData(ASessionKey, BannerHash, DummyParams);
  SetBannerData(ASessionKey, BannerHash, AParams);
}

UnicodeString TConfiguration::FormatFingerprintKey(const UnicodeString & ASiteKey, const UnicodeString & AFingerprintType) const
{
  return FORMAT("%s:%s", ASiteKey, AFingerprintType);
}

void TConfiguration::RememberLastFingerprint(const UnicodeString & ASiteKey, const UnicodeString & AFingerprintType, const UnicodeString & AFingerprint)
{
  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), true) &&
      Storage->OpenSubKey(LastFingerprintsStorageKey, true))
  {
    const UnicodeString FingerprintKey = FormatFingerprintKey(ASiteKey, AFingerprintType);
    Storage->WriteString(FingerprintKey, AFingerprint);
  }
}

UnicodeString TConfiguration::GetLastFingerprint(const UnicodeString & ASiteKey, const UnicodeString & AFingerprintType)
{
  UnicodeString Result;

  std::unique_ptr<THierarchicalStorage> Storage(CreateConfigStorage());
  Storage->SetAccessMode(smRead);

  if (Storage->OpenSubKey(GetConfigurationSubKey(), false) &&
      Storage->OpenSubKey(LastFingerprintsStorageKey, false))
  {
    const UnicodeString FingerprintKey = FormatFingerprintKey(ASiteKey, AFingerprintType);
    Result = Storage->ReadString(FingerprintKey, "");
  }
  return Result;
}

void TConfiguration::Changed()
{
  TNotifyEvent AOnChange = nullptr;

  {
    TGuard Guard(FCriticalSection); nb::used(Guard);
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
  catch(Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_CONFIG_ERROR));
  }
}

bool TConfiguration::RegistryPathExists(const UnicodeString & RegistryPath) const
{
  std::unique_ptr<TRegistryStorage> Registry(std::make_unique<TRegistryStorage>(GetRegistryStorageKey()));
  Registry->Init();
  const UnicodeString ParentKey = ExtractFileDir(RegistryPath);
  const UnicodeString SubKey = base::ExtractFileName(RegistryPath, false);
  return
    Registry->OpenRootKey(false) &&
    (ParentKey.IsEmpty() ||
     Registry->OpenSubKeyPath(ParentKey, false)) &&
    Registry->KeyExists(SubKey);
}

void TConfiguration::CleanupRegistry(const UnicodeString & RegistryPath)
{
  std::unique_ptr<TRegistryStorage> Registry(std::make_unique<TRegistryStorage>(RegistryStorageKey()));

  UnicodeString ParentKey = ExtractFileDir(RegistryPath);
  if (ParentKey.IsEmpty() ||
      Registry->OpenSubKeyPath(ParentKey, false))
  {
    const UnicodeString SubKey = base::ExtractFileName(RegistryPath, false);
    Registry->RecursiveDeleteSubKey(SubKey);
  }
}

TStrings * TConfiguration::GetCaches() const
{
  std::unique_ptr<TStrings> Result(std::make_unique<TStringList>());
  Result->Add(SshHostKeysSubKey);
  Result->Add(FtpsCertificateStorageKey);
  Result->Add(HttpsCertificateStorageKey);
  Result->Add(DirectoryStatisticsCacheKey);
  Result->Add(TPath::Combine(ConfigurationSubKey, CDCacheKey));
  Result->Add(TPath::Combine(ConfigurationSubKey, BannersKey));
  Result->Add(TPath::Combine(ConfigurationSubKey, LastFingerprintsStorageKey));
  return Result.release();
}

bool TConfiguration::HasAnyCache() const
{
  bool Result = false;
  std::unique_ptr<TStrings> Caches(GetCaches());
  for (int32_t Index = 0; Index < Caches->Count(); Index++)
  {
    if (RegistryPathExists(Caches->GetString(Index)))
    {
      Result = true;
      break;
    }
  }
  return Result;
}

void TConfiguration::CleanupCaches()
{
  try
  {
    std::unique_ptr<TStrings> Caches(GetCaches());
    for (int32_t Index = 0; Index < Caches->Count(); Index++)
    {
      CleanupRegistry(Caches->GetString(Index));
    }
  }
  catch(Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_CACHES_ERROR));
  }
}

void TConfiguration::CleanupRandomSeedFile()
{
  try
  {
    DontSaveRandomSeed();
    if (base::FileExists(ApiPath(GetRandomSeedFileName())))
    {
      DeleteFileChecked(GetRandomSeedFileName());
    }
  }
  catch(Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_SEEDFILE_ERROR));
  }
}

void TConfiguration::CleanupIniFile()
{
#if 0
  try
  {
    if (base::FileExists(ApiPath(IniFileStorageNameForReading)))
    {
      DeleteFileChecked(IniFileStorageNameForReading);
    }
    if (Storage == stIniFile)
    {
      FDontSave = true;
    }
  }
  catch(Exception & E)
  {
    throw ExtException(&E, LoadStr(CLEANUP_INIFILE_ERROR));
  }
#endif // #if 0
}

void TConfiguration::DontSave()
{
  FDontSave = true;
}

RawByteString TConfiguration::EncryptPassword(const UnicodeString & APassword, const UnicodeString & AKey)
{
  if (APassword.IsEmpty())
  {
    return RawByteString();
  }
  else
  {
    return ::EncryptPassword(APassword, AKey);
  }
}

UnicodeString TConfiguration::DecryptPassword(const RawByteString & APassword, const UnicodeString & AKey)
{
  if (APassword.IsEmpty())
  {
    return UnicodeString();
  }
  return ::DecryptPassword(APassword, AKey);
}

RawByteString TConfiguration::StronglyRecryptPassword(const RawByteString & APassword, const UnicodeString & /*Key*/)
{
  return APassword;
}

TVSFixedFileInfo * TConfiguration::GetFixedApplicationInfo() const
{
  TVSFixedFileInfo * Result{nullptr};
  try
  {
    Result = GetFixedFileInfo(GetApplicationInfo());
  }
  catch(const Exception &)
  {
    Result = nullptr;
  }
  return Result;
}

int32_t TConfiguration::GetCompoundVersion() const
{
  const TVSFixedFileInfo * FileInfo = GetFixedApplicationInfo();
  if (!FileInfo)
    return 0;
  return CalculateCompoundVersion(
    HIWORD(FileInfo->dwFileVersionMS), LOWORD(FileInfo->dwFileVersionMS),
    HIWORD(FileInfo->dwFileVersionLS));
}

UnicodeString TConfiguration::ModuleFileName() const
{
  ThrowNotImplemented(204);
  return "";
}

void * TConfiguration::GetFileApplicationInfo(const UnicodeString & AFileName) const
{
  void * Result{nullptr};
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

UnicodeString TConfiguration::GetFileProductName(const UnicodeString & AFileName) const
{
  return GetFileFileInfoString("ProductName", AFileName);
}

UnicodeString TConfiguration::GetFileCompanyName(const UnicodeString & AFileName) const
{
  // particularly in IDE build, company name is empty
  return GetFileFileInfoString("CompanyName", AFileName, true);
}

UnicodeString TConfiguration::GetProductName() const
{
  return GetFileProductName("");
}

UnicodeString TConfiguration::GetCompanyName() const
{
  return GetFileCompanyName("");
}

UnicodeString TConfiguration::GetFileProductVersion(const UnicodeString & AFileName) const
{
  return TrimVersion(GetFileFileInfoString("ProductVersion", AFileName));
}

UnicodeString TConfiguration::GetFileDescription(const UnicodeString & AFileName) const
{
  return GetFileFileInfoString("FileDescription", AFileName);
}

UnicodeString TConfiguration::GetFileProductVersion() const
{
  return GetFileProductVersion("");
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

static TDateTime GetBuildDate()
{
  UnicodeString BuildDateStr = __DATE__;
  const UnicodeString MonthStr = CutToChar(BuildDateStr, L' ', true);
  const int32_t Month = ParseShortEngMonthName(MonthStr);
  const int32_t Day = StrToIntDef(CutToChar(BuildDateStr, L' ', true), 0);
  const int32_t Year = StrToIntDef(Trim(BuildDateStr), 0);
  TDateTime Result = EncodeDateVerbose(static_cast<Word>(Year), static_cast<Word>(Month), static_cast<Word>(Day));
  return Result;
}

UnicodeString TConfiguration::GetFullVersion() const
{
  UnicodeString Result = GetVersion();

  const UnicodeString AReleaseType = GetReleaseType();
  if (DebugAlwaysTrue(!AReleaseType.IsEmpty()) &&
      !SameText(AReleaseType, L"stable") &&
      !SameText(AReleaseType, L"development"))
  {
    Result += L" " + AReleaseType;
  }
  return Result;
}

static UnicodeString GetUnofficialBuildTag()
{
  DebugAssert(GetConfiguration()->GetIsUnofficial());
  UnicodeString Result;
  #ifdef _DEBUG
  Result = LoadStr(VERSION_DEBUG_BUILD);
  #else
  Result = LoadStr(VERSION_DEV_BUILD);
  #endif
  return Result;
}

UnicodeString TConfiguration::GetVersionStrHuman()
{
  TGuard Guard(FCriticalSection);
  try
  {
    const TDateTime BuildDate = GetBuildDate();

    UnicodeString FullVersion = GetFullVersion();

    if (GetIsUnofficial())
    {
      const UnicodeString BuildStr = GetUnofficialBuildTag();
      FullVersion += L" " + BuildStr;
    }

    UnicodeString DateStr;
    const TDateTime ANow = Now();
    if (BuildDate < ANow)
    {
      DateStr = FormatRelativeTime(ANow, BuildDate, true);
    }
    else
    {
      DateStr = FormatDateTime(L"ddddd", BuildDate);
    }

    UnicodeString Result = FORMAT(L"%s (%s)", FullVersion, DateStr);

    return Result;
  }
  catch(Exception & E)
  {
    throw ExtException(&E, L"Can't get application version");
  }
}

UnicodeString TConfiguration::GetVersionStr() const
{
  return GetProductVersionStr();
}

UnicodeString TConfiguration::GetProductVersionStr() const
{
  UnicodeString Result;
  TGuard Guard(FCriticalSection); nb::used(Guard);
  try
  {
    const TVSFixedFileInfo * FixedApplicationInfo = GetFixedApplicationInfo();
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
      BuildStr = GetUnofficialBuildTag();
    }

    const UnicodeString FullVersion = GetFullVersion();

    const WORD Build = LOWORD(FixedApplicationInfo->dwFileVersionLS);
    if (Build > 0)
    {
      BuildStr += L" " + ::IntToStr(Build);
    }

#if 0
    UnicodeString BuildDate = __DATE__;
    UnicodeString MonthStr = CutToChar(BuildDate, L' ', true);
    int32_t Month = ParseShortEngMonthName(MonthStr);
    int32_t Day = StrToInt64(CutToChar(BuildDate, L' ', true));
    int32_t Year = StrToInt64(Trim(BuildDate));
    UnicodeString DateStr = FORMAT("%d-%2.2d-%2.2d", Year, Month, Day);
    AddToList(BuildStr, DateStr, L" ");
#endif

    UnicodeString Result = FMTLOAD(VERSION2, FullVersion, BuildStr);

    if (GetIsUnofficial())
    {
      Result += L" " + LoadStr(VERSION_DONT_DISTRIBUTE);
    }

    return Result;
  }
  catch(Exception & E)
  {
    throw ExtException(&E, "Can't get application version");
  }
  return Result;
}

UnicodeString TConfiguration::GetFileVersion(const UnicodeString & AFileName) const
{
  UnicodeString Result;
  void * FileInfo = CreateFileInfo(AFileName);
  try__finally
  {
    Result = GetFileVersion(GetFixedFileInfo(FileInfo));
  }
  __finally
  {
    FreeFileInfo(FileInfo);
  } end_try__finally
  return Result;
}

UnicodeString TConfiguration::GetFileVersion(TVSFixedFileInfo * Info) const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  UnicodeString Result;
  if (Info)
  try
  {
    Result =
      FormatVersion(
        HIWORD(Info->dwFileVersionMS),
        LOWORD(Info->dwFileVersionMS),
        HIWORD(Info->dwFileVersionLS));
  }
  catch(Exception & E)
  {
    throw ExtException(&E, "Can't get file version");
  }
  return Result;
}

UnicodeString TConfiguration::GetVersion() const
{
  return GetFileVersion(GetFixedApplicationInfo());
}

UnicodeString TConfiguration::GetFileFileInfoString(const UnicodeString & AKey,
  const UnicodeString & AFileName, bool AllowEmpty) const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);

  UnicodeString Result;
  void * Info = GetFileApplicationInfo(AFileName);
  try__finally
  {
    try
    {
      if ((Info != nullptr) && (GetTranslationCount(Info) > 0))
      {
        const TTranslation Translation = GetTranslation(Info, 0);
        Result = ::GetFileInfoString(Info, Translation, AKey, AllowEmpty);
      }
      else
      {
        // DebugAssert(!AFileName.IsEmpty());
      }
    }
    catch(const Exception & E)
    {
      (void)E;
      DEBUG_PRINTF("Error: %s", E.Message);
      Result.Clear();
    }
    catch(const std::exception & E)
    {
      (void)E;
      DEBUG_PRINTF("Error: %s", ::MB2W(E.what()));
      Result.Clear();
    }
    catch(...)
    {
      Result.Clear();
    }
  }
  __finally
  {
    if (!AFileName.IsEmpty() && Info)
    {
      FreeFileInfo(Info);
    }
  } end_try__finally
  return Result;
}

UnicodeString TConfiguration::GetFileInfoString(const UnicodeString & Key) const
{
  return GetFileFileInfoString(Key, "");
}

UnicodeString TConfiguration::GetFileMimeType(const UnicodeString & AFileName) const
{
  UnicodeString Result;
  bool Found = false;

  if (!GetMimeTypes().IsEmpty())
  {
    const UnicodeString FileNameOnly = base::ExtractFileName(AFileName, false);
    UnicodeString AMimeTypes = GetMimeTypes();
    while (!Found && !AMimeTypes.IsEmpty())
    {
      UnicodeString Token = CutToChar(AMimeTypes, L',', true);
      UnicodeString MaskStr = CutToChar(Token, L'=', true);
      TFileMasks Mask(MaskStr);
      if (Mask.MatchesFileName(FileNameOnly))
      {
        Result = Token.Trim();
        Found = true;
      }
    }
  }

  if (!Found) // allow an override to "no" Content-Type
  {
    Result = ::GetFileMimeType(AFileName);
  }

  return Result;
}

UnicodeString TConfiguration::GetRegistryStorageKey() const
{
  return GetRegistryKey();
}

void TConfiguration::SetNulStorage()
{
  FStorage = stNul;
}

void TConfiguration::SetExplicitIniFileStorageName(const UnicodeString & FileName)
{
  FIniFileStorageName = FileName;
  FStorage = stIniFile;
}

UnicodeString TConfiguration::GetDefaultIniFileExportPath() const
{
  const UnicodeString PersonalDirectory = GetPersonalFolder();
  const UnicodeString FileName = IncludeTrailingBackslash(PersonalDirectory) +
    base::ExtractFileName(ExpandEnvironmentVariables(GetIniFileStorageNameForReadingWriting()), false);
  return FileName;
}

UnicodeString TConfiguration::GetIniFileStorageNameForReading()
{
  return GetIniFileStorageName(true);
}

UnicodeString TConfiguration::GetIniFileStorageNameForReadingWriting() const
{
  return GetIniFileStorageName(false);
}

UnicodeString TConfiguration::GetAutomaticIniFileStorageName(bool ReadingOnly) const
{
  UnicodeString ProgramPath = ""; // TODO: ParamStr(0);

  const UnicodeString ProgramIniPath = ChangeFileExt(ProgramPath, L".ini");

  UnicodeString IniPath;
  if (base::FileExists(ApiPath(ProgramIniPath)))
  {
    IniPath = ProgramIniPath;
  }
  else
  {
    const UnicodeString AppDataIniPath =
      IncludeTrailingBackslash(GetShellFolderPath(CSIDL_APPDATA)) +
      base::ExtractFileName(ProgramIniPath, false);
    if (base::FileExists(ApiPath(AppDataIniPath)))
    {
      IniPath = AppDataIniPath;
    }
    else
    {
      // avoid expensive test if we are interested in existing files only
      if (!ReadingOnly && (FProgramIniPathWritable < 0))
      {
        const UnicodeString ProgramDir = ::ExtractFilePath(ProgramPath);
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
    const UnicodeString LocalAppDataPath = GetShellFolderPath(CSIDL_LOCAL_APPDATA);
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
      base::FileExists(ApiPath(FVirtualIniFileStorageName)))
  {
    return FVirtualIniFileStorageName;
  }
  else
  {
    return IniPath;
  }
}

UnicodeString TConfiguration::GetIniFileParamValue() const
{
  UnicodeString Result;
  if (Storage == stNul)
  {
    Result = INI_NUL;
  }
  else if ((Storage == stIniFile) && !FIniFileStorageName.IsEmpty())
  {
    Result = FIniFileStorageName;
  }
  return Result;
}

UnicodeString TConfiguration::GetIniFileStorageName(bool ReadingOnly) const
{
  UnicodeString Result;
  if (!FIniFileStorageName.IsEmpty())
  {
    Result = FIniFileStorageName;
  }
  else if (!FCustomIniFileStorageName.IsEmpty())
  {
    Result = ExpandEnvironmentVariables(FCustomIniFileStorageName);
  }
  else
  {
    Result = GetAutomaticIniFileStorageName(ReadingOnly);
  }
  return Result;
}

void TConfiguration::SetOptionsStorage(const TStrings * Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  if (FOptionsStorage == nullptr)
  {
    FOptionsStorage = std::make_unique<TStringList>();
  }
  FOptionsStorage->AddStrings(Value);
}

const TStrings * TConfiguration::GetOptionsStorage() const
{
  return FOptionsStorage.get();
}

UnicodeString TConfiguration::GetPuttySessionsSubKey() const
{
  return StoredSessionsSubKey;
}

UnicodeString TConfiguration::GetPuttySessionsKey(const UnicodeString & RootKey) const
{
  return RootKey + L"\\" + GetPuttySessionsSubKey();
}

UnicodeString TConfiguration::DoGetPuttySessionsKey() const
{
  return GetPuttySessionsKey(PuttyRegistryStorageKey);
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

void TConfiguration::MoveStorage(TStorage AStorage, const UnicodeString & ACustomIniFileStorageName)
{
  if ((FStorage != AStorage) ||
      ((FStorage == stIniFile) && !FIniFileStorageName.IsEmpty()) ||
      // Not expanding, as we want to allow change from explicit path to path with variables and vice versa
      !IsPathToSameFile(FCustomIniFileStorageName, ACustomIniFileStorageName))
  {
    const TStorage StorageBak = FStorage;
    const UnicodeString CustomIniFileStorageNameBak = FCustomIniFileStorageName;
    const UnicodeString IniFileStorageNameBak = FIniFileStorageName;
    try
    {
      std::unique_ptr<THierarchicalStorage> SourceStorage(CreateConfigStorage());
      std::unique_ptr<THierarchicalStorage> TargetStorage(CreateConfigStorage());
      try__finally
      {
        SourceStorage->SetAccessMode(smRead);

        FStorage = AStorage;
        FCustomIniFileStorageName = ACustomIniFileStorageName;
        FIniFileStorageName = UnicodeString();

        TargetStorage->SetAccessMode(smReadWrite);
        TargetStorage->SetExplicit(true);

        // copy before save as it removes the ini file,
        // when switching from ini to registry
        CopyData(SourceStorage.get(), TargetStorage.get());
      }
      __finally__removed
      {
        // delete SourceStorage;
        // delete TargetStorage;
      } end_try__finally

      // save all and explicit,
      // this also removes an INI file, when switching to registry storage
      DoSave(true, true);
    }
    catch(...)
    {
      // If this fails, do not pretend that storage was switched.
      // For instance:
      // - When writing to an INI file fails (unlikely, as we fallback to user profile)
      // - When removing INI file fails, when switching to registry
      //   (possible, when the INI file is in Program Files folder)
      FStorage = StorageBak;
      FCustomIniFileStorageName = CustomIniFileStorageNameBak;
      FIniFileStorageName = IniFileStorageNameBak;
      throw;
    }
  }
}

void TConfiguration::ScheduleCustomIniFileStorageUse(const UnicodeString & ACustomIniFileStorageName)
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
  TGuard Guard(FCriticalSection); nb::used(Guard);
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

static TStoredSessionList * CreateSessionsForImport(TStoredSessionList * Sessions)
{
  std::unique_ptr<TStoredSessionList> Result(std::make_unique<TStoredSessionList>(true));
  Result->DefaultSettings = Sessions->DefaultSettings;
  return Result.release();
}

TStoredSessionList * TConfiguration::SelectFilezillaSessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(CreateSessionsForImport(Sessions));

  const UnicodeString AppDataPath = GetShellFolderPath(CSIDL_APPDATA);
  const UnicodeString FilezillaSiteManagerFile = TPath::Combine(AppDataPath, L"FileZilla\\sitemanager.xml");
  const UnicodeString FilezillaConfigurationFile = TPath::Combine(AppDataPath, L"FileZilla\\filezilla.xml");

  if (base::FileExists(ApiPath(FilezillaSiteManagerFile)))
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
    std::unique_ptr<TStoredSessionList> SessionsForImport(SelectFilezillaSessionsForImport(Sessions, Error));
    return (SessionsForImport->GetCount() > 0);
  }
  catch(...)
  {
    return false;
  }
}

UnicodeString GetOpensshFolder()
{
  const UnicodeString ProfilePath = GetShellFolderPath(CSIDL_PROFILE);
  const UnicodeString Result = TPath::Combine(ProfilePath, OpensshFolderName);
  return Result;
}

TStoredSessionList * TConfiguration::SelectKnownHostsSessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(CreateSessionsForImport(Sessions));
  const UnicodeString KnownHostsFile = TPath::Combine(GetOpensshFolder(), L"known_hosts");

  try
  {
    if (base::FileExists(ApiPath(KnownHostsFile)))
    {
      std::unique_ptr<TStrings> Lines(std::make_unique<TStringList>());
      // LoadScriptFromFile(KnownHostsFile, Lines.get(), true);
      ImportSessionList->ImportFromKnownHosts(Lines.get());
    }
    else
    {
      throw Exception(LoadStr(KNOWN_HOSTS_NOT_FOUND));
    }
  }
  catch(Exception & E)
  {
    Error = FORMAT("%s\n(%s)", E.Message, KnownHostsFile);
  }

  return ImportSessionList.release();
}

TStoredSessionList * TConfiguration::SelectKnownHostsSessionsForImport(
  TStrings * Lines, TStoredSessionList * Sessions, UnicodeString &Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(CreateSessionsForImport(Sessions));

  try
  {
    ImportSessionList->ImportFromKnownHosts(Lines);
  }
  catch(Exception & E)
  {
    Error = E.Message;
  }

  return ImportSessionList.release();
}

TStoredSessionList * TConfiguration::SelectOpensshSessionsForImport(
  TStoredSessionList * Sessions, UnicodeString & Error)
{
  std::unique_ptr<TStoredSessionList> ImportSessionList(CreateSessionsForImport(Sessions));
  const UnicodeString ConfigFile = TPath::Combine(GetOpensshFolder(), L"config");

  try
  {
    if (base::FileExists(ApiPath(ConfigFile)))
    {
      std::unique_ptr<TStrings> Lines(std::make_unique<TStringList>());
      // LoadScriptFromFile(ConfigFile, Lines.get(), true);

      const UnicodeString OpensshIncludeDirective(L"Include");
      for (int32_t Index = 0; Index < Lines->Count; Index++)
      {
        UnicodeString Line = Lines->Strings[Index];
        UnicodeString Directive, Args;
        if (ParseOpensshDirective(Line, Directive, Args))
        {
          if (SameText(Directive, OpensshIncludeDirective))
          {
            while (!Args.IsEmpty())
            {
              UnicodeString IncludePath = ConvertPathFromOpenssh(CutOpensshToken(Args));

              // If path does not exist, try if it works relatively to .ssh/
              if (!base::FileExists(ApiPath(IncludePath)))
              {
                IncludePath = TPath::Combine(GetOpensshFolder(), IncludePath);
              }

              if (base::FileExists(ApiPath(IncludePath)))
              {
                std::unique_ptr <TStrings> LinesToInclude(std::make_unique<TStringList>());
                // LoadScriptFromFile(IncludePath, LinesToInclude.get(), true);
                Lines->Delete(Index); // Not really needed
                for (int32_t Index2 = 0; Index2 < LinesToInclude->Count; Index2++)
                {
                  Lines->Insert(Index + Index2, LinesToInclude->Strings[Index2]);
                }
                Index--;
              }
            }
          }
        }
      }

      ImportSessionList->ImportFromOpenssh(Lines.get());

      if (ImportSessionList->Count > 0)
      {
        ImportSessionList->SelectSessionsToImport(Sessions, true);
      }
      else
      {
        throw Exception(LoadStr(OPENSSH_CONFIG_NO_SITES));
      }
    }
    else
    {
      throw Exception(LoadStr(OPENSSH_CONFIG_NOT_FOUND));
    }
  }
  catch(Exception & E)
  {
    Error = FORMAT(L"%s\n(%s)", E.Message, ConfigFile);
  }

  return ImportSessionList.release();
}

void TConfiguration::SetRandomSeedFile(const UnicodeString & Value)
{
  if (GetRandomSeedFile() != Value)
  {
    const UnicodeString PrevRandomSeedFileName = GetRandomSeedFileName();

    FRandomSeedFile = Value;

    // never allow empty seed file to avoid Putty trying to reinitialize the path
    if (GetRandomSeedFileName().IsEmpty())
    {
      FRandomSeedFile = FDefaultRandomSeedFile;
    }

    if (!PrevRandomSeedFileName.IsEmpty() &&
        (PrevRandomSeedFileName != GetRandomSeedFileName()) &&
        base::FileExists(ApiPath(PrevRandomSeedFileName)))
    {
      // ignore any error
      base::FileRemove(ApiPath(PrevRandomSeedFileName));
    }
  }
}

UnicodeString TConfiguration::GetDirectoryStatisticsCacheKey(
  const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam)
{
  std::unique_ptr<TStringList> RawOptions(std::make_unique<TStringList>());
  RawOptions->Add(SessionKey);
  RawOptions->Add(base::UnixExcludeTrailingBackslash(Path));

#if 0
  TCopyParamType Defaults;
  TCopyParamType FilterCopyParam;
  FilterCopyParam.IncludeFileMask = CopyParam.IncludeFileMask;
  FilterCopyParam.ExcludeHiddenFiles = CopyParam.ExcludeHiddenFiles;
  FilterCopyParam.ExcludeEmptyDirectories = CopyParam.ExcludeEmptyDirectories;

  std::unique_ptr<TOptionsStorage> OptionsStorage(std::make_unique<TOptionsStorage>(RawOptions.get(), true));
  FilterCopyParam.Save(OptionsStorage.get(), &Defaults);

#endif // if 0
  const UTF8String RawOptionsBuf(RawOptions->GetCommaText().LowerCase());
  const UnicodeString Result = Sha256(RawOptionsBuf.c_str(), RawOptionsBuf.Length());
  return Result;
}

THierarchicalStorage * TConfiguration::OpenDirectoryStatisticsCache(bool CanCreate)
{
  std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateConfigStorage());
  Storage->AccessMode = CanCreate ? smReadWrite : smRead;
  if (!Storage->OpenSubKey(DirectoryStatisticsCacheKey, CanCreate))
  {
    Storage.reset(nullptr);
  }
  return Storage.release();
}

TStrings * TConfiguration::LoadDirectoryStatisticsCache(
  const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam)
{
  std::unique_ptr<THierarchicalStorage> Storage(OpenDirectoryStatisticsCache(false));
  std::unique_ptr<TStringList> Result;
  if (Storage != nullptr)
  {
    const UnicodeString Key = GetDirectoryStatisticsCacheKey(SessionKey, Path, CopyParam);
    const UnicodeString Buf = Storage->ReadString(Key, UnicodeString());
    Result.reset(CommaTextToStringList(Buf));
  }
  else
  {
    Result = std::make_unique<TStringList>();
  }
  return Result.release();
}

void TConfiguration::SaveDirectoryStatisticsCache(
  const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam, TStrings * DataList)
{
  std::unique_ptr<THierarchicalStorage> Storage(OpenDirectoryStatisticsCache(true));
  if (Storage != nullptr)
  {
    const UnicodeString Key = GetDirectoryStatisticsCacheKey(SessionKey, Path, CopyParam);
    const UnicodeString Buf = DataList->GetCommaText();
    Storage->WriteString(Key, Buf);
  }
}

UnicodeString TConfiguration::GetRandomSeedFileName() const
{
  // StripPathQuotes should not be needed as we do not feed quotes anymore
  return StripPathQuotes(::ExpandEnvironmentVariables(FRandomSeedFile)).Trim();
}

void TConfiguration::SetExternalIpAddress(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(ExternalIpAddress);
}

bool TConfiguration::HasLocalPortNumberLimits() const
{
  return (FLocalPortNumberMin > 0) && (FLocalPortNumberMax >= FLocalPortNumberMin);
}

void TConfiguration::SetLocalPortNumberMin(int32_t Value)
{
  SET_CONFIG_PROPERTY2(LocalPortNumberMin);
}

void TConfiguration::SetLocalPortNumberMax(int32_t Value)
{
  SET_CONFIG_PROPERTY2(LocalPortNumberMax);
}

void TConfiguration::SetMimeTypes(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY(MimeTypes);
}

void TConfiguration::SetCertificateStorage(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY2(CertificateStorage);
}

UnicodeString TConfiguration::GetCertificateStorageExpanded() const
{
  UnicodeString Result = FCertificateStorage;
  if (Result.IsEmpty())
  {
    const UnicodeString DefaultCertificateStorage = TPath::Combine(ExtractFilePath(ModuleFileName()), L"cacert.pem");
    if (base::FileExists(DefaultCertificateStorage))
    {
      Result = DefaultCertificateStorage;
    }
  }
  return Result;
}

void TConfiguration::SetAWSMetadataService(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY2(AWSMetadataService);
}

void TConfiguration::SetTryFtpWhenSshFails(bool Value)
{
  SET_CONFIG_PROPERTY(TryFtpWhenSshFails);
}

void TConfiguration::SetParallelDurationThreshold(int32_t Value)
{
  SET_CONFIG_PROPERTY(ParallelDurationThreshold);
}

void TConfiguration::SetPuttyRegistryStorageKey(const UnicodeString & Value)
{
  SET_CONFIG_PROPERTY_EX(PuttyRegistryStorageKey,
    RefreshPuttySshHostCAList());
}

TEOLType TConfiguration::GetLocalEOLType() const
{
  return eolCRLF;
}

bool TConfiguration::GetCollectUsage() const
{
  return FUsage->GetCollect();
}

void TConfiguration::SetCollectUsage(bool Value)
{
  FUsage->SetCollect(Value);
}

void TConfiguration::TemporaryLogging(const UnicodeString & ALogFileName)
{
  if (SameText(ExtractFileExt(ALogFileName), ".xml"))
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

void TConfiguration::TemporaryActionsLogging(const UnicodeString & ALogFileName)
{
  FLogActions = true;
  FActionsLogFileName = ALogFileName;
}

void TConfiguration::TemporaryLogProtocol(int32_t ALogProtocol)
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

void TConfiguration::TemporaryLogMaxCount(int32_t ALogMaxCount)
{
  FLogMaxCount = ALogMaxCount;
}

void TConfiguration::SetLogging(bool Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
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
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FPermanentLogging;
}

void TConfiguration::SetLogFileName(const UnicodeString & Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  if (GetLogFileName() != Value)
  {
    FPermanentLogFileName = Value;
    FLogFileName = Value;
    Changed();
  }
}

void TConfiguration::SetActionsLogFileName(const UnicodeString & Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  if (GetActionsLogFileName() != Value)
  {
    FPermanentActionsLogFileName = Value;
    FActionsLogFileName = Value;
    Changed();
  }
}

UnicodeString TConfiguration::GetPermanentActionsLogFileName() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FPermanentActionsLogFileName;
}

UnicodeString TConfiguration::GetActionsLogFileName() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FActionsLogFileName;
}

bool TConfiguration::GetLogToFile() const
{
  // guarded within GetLogFileName
  return !GetLogFileName().IsEmpty();
}

void TConfiguration::UpdateActualLogProtocol()
{
  FActualLogProtocol = FLogging ? FLogProtocol : (-BelowNormalLogLevels - 1);
}

void TConfiguration::SetLogProtocol(int32_t Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
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
  TGuard Guard(FCriticalSection); nb::used(Guard);
  if (GetLogActions() != Value)
  {
    FPermanentLogActions = Value;
    FLogActions = Value;
    Changed();
  }
}

bool TConfiguration::GetLogActions() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
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
  TGuard Guard(FCriticalSection); nb::used(Guard);
  if (GetLogMaxSize() != Value)
  {
    FPermanentLogMaxSize = Value;
    FLogMaxSize = Value;
    Changed();
  }
}

int64_t TConfiguration::GetLogMaxSize() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FPermanentLogMaxSize;
}

void TConfiguration::SetLogMaxCount(int32_t Value)
{
  if (GetLogMaxCount() != Value)
  {
    FPermanentLogMaxCount = Value;
    FLogMaxCount = Value;
    Changed();
  }
}

int32_t TConfiguration::GetLogMaxCount() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FPermanentLogMaxCount;
}

UnicodeString TConfiguration::GetDefaultLogFileName() const
{
  return "%TEMP%\\&S.log";
}

void TConfiguration::SetConfirmOverwriting(bool Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  SET_CONFIG_PROPERTY(ConfirmOverwriting);
}

bool TConfiguration::GetConfirmOverwriting() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FConfirmOverwriting;
}

void TConfiguration::SetConfirmResume(bool Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  SET_CONFIG_PROPERTY(ConfirmResume);
}

bool TConfiguration::GetConfirmResume() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FConfirmResume;
}

void TConfiguration::SetAutoReadDirectoryAfterOp(bool Value)
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  SET_CONFIG_PROPERTY(AutoReadDirectoryAfterOp);
}

bool TConfiguration::GetAutoReadDirectoryAfterOp() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FAutoReadDirectoryAfterOp;
}

UnicodeString TConfiguration::GetConfigurationTimeFormat() const
{
  return "h:nn:ss";
}

UnicodeString TConfiguration::GetDefaultKeyFile() const
{
  return "";
}

bool TConfiguration::GetRememberPassword() const
{
  return false;
}

void TConfiguration::SetSessionReopenAuto(int32_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAuto);
}

void TConfiguration::SetSessionReopenBackground(int32_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenBackground);
}

void TConfiguration::SetSessionReopenTimeout(int32_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenTimeout);
}

void TConfiguration::SetSessionReopenAutoStall(int32_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoStall);
}

void TConfiguration::SetTunnelLocalPortNumberLow(int32_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberLow);
}

void TConfiguration::SetTunnelLocalPortNumberHigh(int32_t Value)
{
  SET_CONFIG_PROPERTY(TunnelLocalPortNumberHigh);
}

void TConfiguration::SetCacheDirectoryChangesMaxSize(int32_t Value)
{
  SET_CONFIG_PROPERTY(CacheDirectoryChangesMaxSize);
}

void TConfiguration::SetShowFtpWelcomeMessage(bool Value)
{
  SET_CONFIG_PROPERTY(ShowFtpWelcomeMessage);
}

void TConfiguration::SetQueueTransfersLimit(int32_t Value)
{
  SET_CONFIG_PROPERTY2(QueueTransfersLimit);
}

const TSshHostCAList * TConfiguration::GetSshHostCAList() const
{
  return FSshHostCAList.get();
}

void TConfiguration::SetSshHostCAList(const TSshHostCAList * value)
{
  *FSshHostCAList = *value;
}

const TSshHostCAList * TConfiguration::GetPuttySshHostCAList()
{
  if (FPuttySshHostCAList == nullptr)
  {
    std::unique_ptr<TRegistryStorage> Storage(std::make_unique<TRegistryStorage>(PuttyRegistryStorageKey));
    Storage->ConfigureForPutty();
    FPuttySshHostCAList = std::make_unique<TSshHostCAList>();
    LoadSshHostCAList(FPuttySshHostCAList.get(), Storage.get());
  }
  return FPuttySshHostCAList.get();
}

void TConfiguration::RefreshPuttySshHostCAList()
{
  FPuttySshHostCAList.reset();
}

const TSshHostCAList * TConfiguration::GetActiveSshHostCAList()
{
  return FSshHostCAsFromPuTTY ? GetPuttySshHostCAList() : GetSshHostCAList();
}

bool TConfiguration::GetPersistent() const
{
  return (FStorage != stNul) && !FDontSave;
}

void TShortCuts::Add(const TShortCut &ShortCut)
{
  FShortCuts.push_back(ShortCut);
}

bool TShortCuts::Has(const TShortCut &ShortCut) const
{
  const nb::vector_t<TShortCut>::const_iterator it = const_cast<TShortCuts *>(this)->FShortCuts.find(ShortCut);
  return (it != FShortCuts.end());
}

void TConfiguration::SetSessionReopenAutoMaximumNumberOfRetries(int32_t Value)
{
  SET_CONFIG_PROPERTY(SessionReopenAutoMaximumNumberOfRetries);
}

UnicodeString  TConfiguration::GetLogFileName() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  return FPermanentLogFileName;
}

UnicodeString TConfiguration::GetProductVersion() const
{
  TGuard Guard(FCriticalSection); nb::used(Guard);
  UnicodeString Result;
  try
  {
    const TVSFixedFileInfo * FixedApplicationInfo = GetFixedApplicationInfo();
    if (FixedApplicationInfo)
    {
      Result = FormatVersion(
        HIWORD(FixedApplicationInfo->dwFileVersionMS),
        LOWORD(FixedApplicationInfo->dwFileVersionMS),
        HIWORD(FixedApplicationInfo->dwFileVersionLS));
    }
  }
  catch(Exception & E)
  {
    throw ExtException(&E, "Can't get application version");
  }
  return Result;
}

