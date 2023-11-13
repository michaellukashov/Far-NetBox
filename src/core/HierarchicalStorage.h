
#pragma once

#include <registry.hpp>
#include <memory>
#include <rdestl/map.h>
#include <rdestl/vector.h>

enum TStorage { stDetect, stRegistry, stIniFile, stXmlFile, stFar3Storage, stNul };
enum TStorageAccessMode { smRead, smReadWrite };
using TIntMapping = rde::map<UnicodeString, int32_t>;

class NB_CORE_EXPORT THierarchicalStorage : public TObject
{
  NB_DISABLE_COPY(THierarchicalStorage)
friend class TCustomIniFileStorage;

public:
  THierarchicalStorage() = delete;
  explicit THierarchicalStorage(const UnicodeString & AStorage) noexcept;
  virtual ~THierarchicalStorage() noexcept;
  virtual void Init() {}
  void ConfigureForPutty();
  bool OpenRootKey(bool CanCreate);
  virtual bool OpenSubKey(const UnicodeString & ASubKey, bool CanCreate);
  virtual void CloseSubKey();
  void CloseAll();
  bool OpenSubKeyPath(const UnicodeString & KeyPath, bool CanCreate);
  void CloseSubKeyPath();
  void GetSubKeyNames(TStrings * Strings);
  void GetValueNames(TStrings * Strings);
  bool HasSubKeys();
  bool KeyExists(const UnicodeString & SubKey);
  bool ValueExists(const UnicodeString & Value);
  virtual void RecursiveDeleteSubKey(const UnicodeString & Key);
  virtual void ClearSubKeys();
  virtual void ReadValues(TStrings * Strings, bool MaintainKeys = false);
  virtual void WriteValues(TStrings * Strings, bool MaintainKeys = false);
  virtual void ClearValues();
  bool DeleteValue(const UnicodeString & Name);

  bool ReadBool(const UnicodeString & Name, bool Default);
  template<typename T>
  typename T ReadEnum(
    const UnicodeString & Name, const T Default, const TIntMapping & Mapping = TIntMapping());
  int32_t ReadInteger(const UnicodeString & Name, int32_t Default);
  int64_t ReadInt64(const UnicodeString & Name, int64_t Default);
  TDateTime ReadDateTime(const UnicodeString & Name, TDateTime Default);
  double ReadFloat(const UnicodeString & Name, double Default);
  UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  int32_t ReadBinaryData(const UnicodeString & Name, void * Buffer, int32_t Size);

  virtual UnicodeString ReadString(const UnicodeString & Name, const UnicodeString & Default);
  RawByteString ReadBinaryData(const UnicodeString & Name);
  RawByteString ReadStringAsBinaryData(const UnicodeString & Name, const RawByteString & Default);

  void WriteBool(const UnicodeString & Name, bool Value);
  void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  void WriteInteger(const UnicodeString & Name, int32_t Value);
  void WriteInt64(const UnicodeString & Name, int64_t Value);
  void WriteDateTime(const UnicodeString & Name, const TDateTime Value);
  void WriteFloat(const UnicodeString & Name, double Value);
  void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size);
  void WriteString(const UnicodeString & Name, const UnicodeString & Value);
  void WriteBinaryData(const UnicodeString & Name, const RawByteString & Value);
  void WriteBinaryDataAsString(const UnicodeString & Name, const RawByteString & Value);

  virtual void Flush();

  __property UnicodeString Storage  = { read = FStorage };
  ROProperty<UnicodeString> Storage{nb::bind(&THierarchicalStorage::GetStorage, this)};
  __property UnicodeString CurrentSubKey = { read = GetCurrentSubKey };
  ROProperty<UnicodeString> CurrentSubKey{nb::bind(&THierarchicalStorage::GetCurrentSubKey, this)};
  __property TStorageAccessMode AccessMode = { read = FAccessMode, write = SetAccessMode };
  RWProperty3<TStorageAccessMode> AccessMode{nb::bind(&THierarchicalStorage::GetAccessMode, this), nb::bind(&THierarchicalStorage::SetAccessMode, this) };
  __property bool Explicit = { read = FExplicit, write = FExplicit };
  bool& Explicit{FExplicit};
  __property bool ForceSave = { read = FForceSave, write = FForceSave };
  bool& ForceSave{FForceSave};
  __property bool ForceAnsi = { read = FForceAnsi, write = FForceAnsi };
  bool& ForceAnsi{FForceAnsi};
  __property bool MungeStringValues = { read = FMungeStringValues, write = FMungeStringValues };
  bool& MungeStringValues{FMungeStringValues};
  __property UnicodeString Source = { read = GetSource };
  ROProperty<UnicodeString> Source{nb::bind(&THierarchicalStorage::GetSourceConst, this)};
  __property bool Temporary = { read = GetTemporary };
  ROProperty<bool> Temporary{nb::bind(&THierarchicalStorage::GetTemporary, this)};

  UnicodeString GetStorage() const { return FStorage; }
  TStorageAccessMode GetAccessMode() const { return FAccessMode; }
  bool GetExplicit() const { return FExplicit; }
  void SetExplicit(bool Value) { FExplicit = Value; }
  bool GetForceAnsi() const { return FForceAnsi; }
  void SetForceAnsi(bool Value) { FForceAnsi = Value; }
  bool GetMungeStringValues() const { return FMungeStringValues; }
  void SetMungeStringValues(bool Value) { FMungeStringValues = Value; }

  UnicodeString GetSourceConst() const { return GetSource(); }
  void SetAccessMode(TStorageAccessMode Value) { SetAccessModeProtected(Value); }
protected:
  enum THierarchicalStorageAccess { hsaRead = 0x01, hsaWrite = 0x02 };
  struct TKeyEntry
  {
    UnicodeString Key;
    int32_t Levels{0};
    uint32_t Access{0};
  };

  UnicodeString FStorage;
  nb::vector_t<TKeyEntry> FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit{false};
  bool FForceSave{false};
  bool FMungeStringValues{false};
  bool FForceAnsi{false};
  int32_t FFakeReadOnlyOpens{0};
  mutable int32_t FRootAccess{0};

  __property bool ForceAnsi = { read = FForceAnsi, write = FForceAnsi };

  UnicodeString GetCurrentSubKey() const;
  UnicodeString GetCurrentSubKeyMunged() const;
  virtual void SetAccessModeProtected(TStorageAccessMode Value);
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi) = 0;
  virtual bool DoValueExists(const UnicodeString & Value) = 0;
  static UnicodeString IncludeTrailingBackslash(const UnicodeString & S);
  static UnicodeString ExcludeTrailingBackslash(const UnicodeString & S);
  virtual bool DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate) = 0;
  virtual void DoCloseSubKey() = 0;
  UnicodeString MungeKeyName(const UnicodeString & Key);
  virtual UnicodeString GetSource() const = 0;
  virtual bool GetTemporary() const;
  virtual void DoDeleteSubKey(const UnicodeString & SubKey) = 0;
  virtual bool DoDeleteValue(const UnicodeString & Name) = 0;

  virtual void DoGetSubKeyNames(TStrings * Strings) = 0;
  virtual void DoGetValueNames(TStrings * Strings) = 0;

  virtual void DoWriteBool(const UnicodeString & Name, bool Value) = 0;
  virtual void DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) = 0;
  virtual void DoWriteInteger(const UnicodeString & Name, int32_t Value) = 0;
  virtual void DoWriteInt64(const UnicodeString & Name, int64_t Value) = 0;
  virtual void DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size) = 0;

  virtual bool DoReadBool(const UnicodeString & Name, bool Default) = 0;
  virtual UnicodeString DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) = 0;
  int32_t ReadIntegerWithMapping(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping);
  virtual int32_t DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping) = 0;
  virtual int64_t DoReadInt64(const UnicodeString & Name, int64_t Default) = 0;
  virtual TDateTime DoReadDateTime(const UnicodeString & Name, TDateTime Default) = 0;
  virtual double DoReadFloat(const UnicodeString & Name, double Default) = 0;
  virtual size_t DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) = 0;

  virtual size_t DoBinaryDataSize(const UnicodeString & Name) = 0;

  virtual UnicodeString DoReadRootAccessString();

  size_t BinaryDataSize(const UnicodeString & Name);
  UnicodeString ReadAccessString();
  uint32_t ReadAccess(uint32_t CurrentAccess);
  virtual bool HasAccess(uint32_t Access);
  bool CanRead();
  bool CanWrite();
  virtual uint32_t GetCurrentAccess();
};

extern TIntMapping AutoSwitchMapping;
extern TIntMapping AutoSwitchReversedMapping;

template<typename T>
T THierarchicalStorage::ReadEnum(const UnicodeString & Name, const T Default, const TIntMapping & Mapping)
{
  const TIntMapping * AMapping = (Mapping.empty()) ? nullptr : &Mapping;
  return T(ReadIntegerWithMapping(Name, nb::ToInt32(Default), AMapping));
}

class NB_CORE_EXPORT TRegistryStorage : public THierarchicalStorage
{
  NB_DISABLE_COPY(TRegistryStorage)
public:
  TRegistryStorage() = delete;
  explicit TRegistryStorage(const UnicodeString & AStorage, HKEY ARootKey, REGSAM WowMode = 0) noexcept;
  explicit TRegistryStorage(const UnicodeString & AStorage) noexcept;
  virtual ~TRegistryStorage() noexcept;

  bool Copy(TRegistryStorage * Storage);

public:
  int32_t GetFailed() const { return FFailed; }
  void SetFailed(int32_t Value) { FFailed = Value; }
  virtual void SetAccessMode(TStorageAccessMode value);

protected:
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoValueExists(const UnicodeString & Value) override;
  virtual bool DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void DoCloseSubKey() override;
  virtual UnicodeString GetSource() const override;
  virtual size_t DoBinaryDataSize(const UnicodeString & Name) override;
  virtual void DoDeleteSubKey(const UnicodeString & SubKey) override;
  virtual bool DoDeleteValue(const UnicodeString & Name) override;

  virtual void DoGetSubKeyNames(TStrings * Strings);
  virtual void DoGetValueNames(TStrings * Strings);

  virtual void DoWriteBool(const UnicodeString & Name, bool Value) override;
  virtual void DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) override;
  virtual void DoWriteInteger(const UnicodeString & Name, int32_t Value) override;
  virtual void DoWriteInt64(const UnicodeString & Name, int64_t Value) override;
  virtual void DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size) override;

  virtual bool DoReadBool(const UnicodeString & Name, bool Default) override;
  virtual int32_t DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping) override;
  virtual int64_t DoReadInt64(const UnicodeString & Name, int64_t Default) override;
  virtual TDateTime DoReadDateTime(const UnicodeString & Name, TDateTime Default) override;
  virtual double DoReadFloat(const UnicodeString & Name, double Default) override;
  virtual UnicodeString DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) override;
  virtual size_t DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) override;

private:
  std::unique_ptr<TRegistry> FRegistry;
  int32_t FFailed{0};
  REGSAM FWowMode{0};

public:
  virtual void Init() override;
};

#if 0

class TCustomIniFileStorage : public THierarchicalStorage
{
public:
  TCustomIniFileStorage(const UnicodeString & Storage, TCustomIniFile * IniFile);
  virtual ~TCustomIniFileStorage();

  virtual bool OpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void CloseSubKey();

private:
  UnicodeString GetCurrentSection();
  inline bool HandleByMasterStorage();
  inline bool HandleReadByMasterStorage(const UnicodeString & Name);
  inline bool DoValueExistsInternal(const UnicodeString & Value);
  void DoWriteStringRawInternal(const UnicodeString & Name, const UnicodeString & Value);
  int32_t DoReadIntegerWithMapping(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping);

protected:
  TCustomIniFile * FIniFile;
  std::unique_ptr<TStringList> FSections;
  std::unique_ptr<THierarchicalStorage> FMasterStorage;
  int32_t FMasterStorageOpenFailures;
  bool FOpeningSubKey;

  __property UnicodeString CurrentSection = { read = GetCurrentSection };
  virtual void SetAccessMode(TStorageAccessMode value);
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoValueExists(const UnicodeString & Value);
  virtual bool DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void DoCloseSubKey();
  virtual UnicodeString GetSource();
  virtual size_t DoBinaryDataSize(const UnicodeString & Name);
  virtual void DoDeleteSubKey(const UnicodeString & SubKey);
  virtual bool DoDeleteValue(const UnicodeString & Name);

  virtual void DoGetSubKeyNames(TStrings * Strings);
  virtual void DoGetValueNames(TStrings * Strings);

  virtual void DoWriteBool(const UnicodeString & Name, bool Value);
  virtual void DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void DoWriteInteger(const UnicodeString & Name, int32_t Value);
  virtual void DoWriteInt64(const UnicodeString & Name, int32_t Value);
  virtual void DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int Size);

  virtual bool DoReadBool(const UnicodeString & Name, bool Default);
  virtual int32_t DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping);
  virtual int32_t DoReadInt64(const UnicodeString & Name, int32_t Default);
  virtual TDateTime DoReadDateTime(const UnicodeString & Name, TDateTime Default);
  virtual double DoReadFloat(const UnicodeString & Name, double Default);
  virtual UnicodeString DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  virtual size_t DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

  virtual UnicodeString DoReadRootAccessString();
  virtual uint32_t GetCurrentAccess() override;
  virtual bool HasAccess(uint32_t Access) override;

  void CacheSections();
  void ResetCache();
  bool DoKeyExistsInternal(const UnicodeString & SubKey);
};

class TIniFileStorage : public TCustomIniFileStorage
{
public:
  static TIniFileStorage * CreateFromPath(const UnicodeString & AStorage);
  static TIniFileStorage * CreateNul();
  virtual ~TIniFileStorage();

  virtual void Flush();

private:
  TIniFileStorage(const UnicodeString & FileName, TCustomIniFile * IniFile);
  TStrings * FOriginal;
  void ApplyOverrides();
};

class TOptionsStorage : public TCustomIniFileStorage
{
public:
  TOptionsStorage(TStrings * Options, bool AllowWrite);
  TOptionsStorage(TStrings * Options, const UnicodeString & RootKey, THierarchicalStorage * MasterStorage);

protected:
  virtual bool GetTemporary();
};

#endif // #if 0

NB_CORE_EXPORT UnicodeString PuttyMungeStr(const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString PuttyUnMungeStr(const UnicodeString & Str);
NB_CORE_EXPORT AnsiString PuttyStr(const UnicodeString & Str);
TIntMapping CreateIntMappingFromEnumNames(const UnicodeString & Names);
