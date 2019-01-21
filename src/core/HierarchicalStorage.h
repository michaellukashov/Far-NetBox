
#pragma once

#include <registry.hpp>
#include <memory>
//---------------------------------------------------------------------------
enum TStorage
{
  stDetect,
  stRegistry,
  stIniFile,
  stNul,
  stXmlFile,
};

enum TStorageAccessMode
{
  smRead,
  smReadWrite,
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT THierarchicalStorage : public TObject
{
  NB_DISABLE_COPY(THierarchicalStorage)

public:
  THierarchicalStorage() = delete;
  explicit THierarchicalStorage(const UnicodeString AStorage) noexcept;
  virtual ~THierarchicalStorage() noexcept;
  virtual void Init() {}
  virtual bool OpenRootKey(bool CanCreate);
  virtual bool OpenSubKey(const UnicodeString ASubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  void CloseAll();
  virtual bool DeleteSubKey(const UnicodeString SubKey) = 0;
  virtual void GetSubKeyNames(TStrings *Strings) = 0;
  virtual void GetValueNames(TStrings *Strings) const = 0;
  bool HasSubKeys();
  bool HasSubKey(const UnicodeString SubKey);
  bool KeyExists(const UnicodeString SubKey);
  virtual bool ValueExists(const UnicodeString Value) const = 0;
  virtual void RecursiveDeleteSubKey(const UnicodeString Key);
  virtual void ClearSubKeys();
  virtual void ReadValues(TStrings *Strings, bool MaintainKeys = false);
  virtual void WriteValues(TStrings *Strings, bool MaintainKeys = false);
  virtual void ClearValues();
  virtual bool DeleteValue(const UnicodeString Name) = 0;

  virtual size_t BinaryDataSize(const UnicodeString Name) const = 0;

  virtual bool ReadBool(const UnicodeString Name, bool Default) const = 0;
  virtual intptr_t ReadIntPtr(const UnicodeString Name, intptr_t Default) const = 0;
  virtual int ReadInteger(const UnicodeString Name, int Default) const = 0;
  virtual int64_t ReadInt64(const UnicodeString Name, int64_t Default) const = 0;
  virtual TDateTime ReadDateTime(const UnicodeString Name, const TDateTime &Default) const = 0;
  virtual double ReadFloat(const UnicodeString Name, double Default) const = 0;
  virtual UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default) const = 0;
  virtual size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size) const = 0;

  virtual UnicodeString ReadString(const UnicodeString Name, const UnicodeString Default) const;
  RawByteString ReadBinaryData(const UnicodeString Name) const;
  virtual RawByteString ReadStringAsBinaryData(const UnicodeString Name, const RawByteString Default) const;

  virtual void WriteBool(const UnicodeString Name, bool Value) = 0;
  virtual void WriteStringRaw(const UnicodeString Name, const UnicodeString Value) = 0;
  virtual void WriteIntPtr(const UnicodeString Name, intptr_t Value) = 0;
  virtual void WriteInteger(const UnicodeString Name, int Value) = 0;
  virtual void WriteInt64(const UnicodeString Name, int64_t Value) = 0;
  virtual void WriteDateTime(const UnicodeString Name, const TDateTime &Value) = 0;
  virtual void WriteFloat(const UnicodeString Name, double Value) = 0;
  virtual void WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size) = 0;

  virtual void WriteString(const UnicodeString Name, const UnicodeString Value);
  void WriteBinaryData(const UnicodeString Name, const RawByteString Value);
  virtual void WriteBinaryDataAsString(const UnicodeString Name, const RawByteString Value);

  virtual void Flush();

  __property UnicodeString Storage  = { read = FStorage };
  ROProperty<UnicodeString> Storage{nb::bind(&THierarchicalStorage::GetStorage, this)};
  __property UnicodeString CurrentSubKey  = { read = GetCurrentSubKey };
  ROProperty<UnicodeString> CurrentSubKey{nb::bind(&THierarchicalStorage::GetCurrentSubKey, this)};
  __property TStorageAccessMode AccessMode  = { read = FAccessMode, write = SetAccessMode };
  RWProperty<TStorageAccessMode> AccessMode{nb::bind(&THierarchicalStorage::GetAccessMode, this), nb::bind(&THierarchicalStorage::SetAccessMode, this) };
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

  UnicodeString GetSourceConst() const { return GetSourceProtected(); }
  UnicodeString GetSource() { return GetSourceProtected(); }
  void SetAccessMode(TStorageAccessMode Value) { SetAccessModeProtected(Value); }
  bool GetTemporary() const { return GetTemporaryProtected(); }
protected:
  UnicodeString FStorage;
  std::unique_ptr<TStrings> FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit{false};
  bool FForceSave{false};
  bool FMungeStringValues{false};
  bool FForceAnsi{false};

  UnicodeString GetCurrentSubKey() const;
  UnicodeString GetCurrentSubKeyMunged() const;
  virtual void SetAccessModeProtected(TStorageAccessMode Value);
  virtual bool DoKeyExists(const UnicodeString SubKey, bool ForceAnsi) = 0;
  static UnicodeString IncludeTrailingBackslash(const UnicodeString S);
  static UnicodeString ExcludeTrailingBackslash(const UnicodeString S);
  virtual bool DoOpenSubKey(const UnicodeString SubKey, bool CanCreate) = 0;
  UnicodeString MungeKeyName(const UnicodeString Key);

  virtual UnicodeString GetSourceProtected() const = 0;
  virtual UnicodeString GetSourceProtected() = 0;
  virtual bool GetTemporaryProtected() const;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TRegistryStorage : public THierarchicalStorage
{
  NB_DISABLE_COPY(TRegistryStorage)
public:
  TRegistryStorage() = delete;
  explicit TRegistryStorage(const UnicodeString AStorage, HKEY ARootKey, REGSAM WowMode = 0) noexcept;
  explicit TRegistryStorage(const UnicodeString AStorage) noexcept;
  void Init() override;
  virtual ~TRegistryStorage() noexcept;

  bool Copy(TRegistryStorage *Storage);

  void CloseSubKey() override;
  bool DeleteSubKey(const UnicodeString SubKey) override;
  bool DeleteValue(const UnicodeString Name) override;
  void GetSubKeyNames(TStrings *Strings) override;
  bool ValueExists(const UnicodeString Value) const override;

  size_t BinaryDataSize(const UnicodeString Name) const override;

  bool ReadBool(const UnicodeString Name, bool Default) const override;
  intptr_t ReadIntPtr(const UnicodeString Name, intptr_t Default) const override;
  int ReadInteger(const UnicodeString Name, int Default) const override;
  int64_t ReadInt64(const UnicodeString Name, int64_t Default) const override;
  TDateTime ReadDateTime(const UnicodeString Name, const TDateTime &Default) const override;
  double ReadFloat(const UnicodeString Name, double Default) const override;
  UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default) const override;
  size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size) const override;

  void WriteBool(const UnicodeString Name, bool Value) override;
  void WriteIntPtr(const UnicodeString Name, intptr_t Value) override;
  void WriteInteger(const UnicodeString Name, int Value) override;
  void WriteInt64(const UnicodeString Name, int64_t Value) override;
  void WriteDateTime(const UnicodeString Name, const TDateTime &Value) override;
  void WriteFloat(const UnicodeString Name, double Value) override;
  void WriteStringRaw(const UnicodeString Name, const UnicodeString Value) override;
  void WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size) override;

  void GetValueNames(TStrings *Strings) const override;

protected:
  intptr_t GetFailedProtected() const;
  void SetAccessModeProtected(TStorageAccessMode Value) override;
  bool DoKeyExists(const UnicodeString SubKey, bool AForceAnsi) override;
  bool DoOpenSubKey(const UnicodeString SubKey, bool CanCreate) override;
  UnicodeString GetSourceProtected() const override;
  UnicodeString GetSourceProtected() override;

  __property int Failed  = { read = GetFailed, write = FFailed };

public:
  intptr_t GetFailed() const { return GetFailedProtected(); }
  void SetFailed(intptr_t Value) { FFailed = Value; }

private:
  std::unique_ptr<TRegistry> FRegistry;
  mutable intptr_t FFailed{0};
  REGSAM FWowMode{0};
};
//---------------------------------------------------------------------------
#if 0
class TCustomIniFileStorage : public THierarchicalStorage
{
public:
  TCustomIniFileStorage(const UnicodeString Storage, TCustomIniFile *IniFile) noexcept;
  virtual ~TCustomIniFileStorage() noexcept;

  virtual bool OpenRootKey(bool CanCreate);
  virtual bool OpenSubKey(UnicodeString SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const UnicodeString SubKey);
  virtual bool DeleteValue(const UnicodeString Name);
  virtual void GetSubKeyNames(Classes::TStrings *Strings);
  virtual bool ValueExists(const UnicodeString Value);

  virtual size_t BinaryDataSize(const UnicodeString Name);

  virtual bool ReadBool(const UnicodeString Name, bool Default);
  virtual int ReadInteger(const UnicodeString Name, int Default);
  virtual int64_t ReadInt64(const UnicodeString Name, int64_t Default);
  virtual TDateTime ReadDateTime(const UnicodeString Name, TDateTime Default);
  virtual double ReadFloat(const UnicodeString Name, double Default);
  virtual UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default);
  virtual size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size);

  virtual void WriteBool(const UnicodeString Name, bool Value);
  virtual void WriteInteger(const UnicodeString Name, int Value);
  virtual void WriteInt64(const UnicodeString Name, int64_t Value);
  virtual void WriteDateTime(const UnicodeString Name, TDateTime Value);
  virtual void WriteFloat(const UnicodeString Name, double Value);
  virtual void WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
  virtual void WriteBinaryData(const UnicodeString Name, const void *Buffer, int Size);

  virtual void GetValueNames(Classes::TStrings *Strings);

private:
  UnicodeString GetCurrentSection();
  inline bool HandleByMasterStorage();
  inline bool HandleReadByMasterStorage(const UnicodeString &Name);
  inline bool DoValueExists(const UnicodeString &Value);
  void DoWriteStringRaw(const UnicodeString &Name, const UnicodeString &Value);
  void DoWriteBinaryData(const UnicodeString &Name, const void *Buffer, int Size);

protected:
  TCustomIniFile *FIniFile;
  std::unique_ptr<TStringList> FSections;
  std::unique_ptr<THierarchicalStorage> FMasterStorage;
  int FMasterStorageOpenFailures;
  bool FOpeningSubKey;

  __property UnicodeString CurrentSection  = { read = GetCurrentSection };
  virtual void SetAccessMode(TStorageAccessMode value);
  virtual bool DoKeyExists(const UnicodeString SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(const UnicodeString SubKey, bool CanCreate);
  virtual UnicodeString GetSource();
  void CacheSections();
  void ResetCache();
};
//---------------------------------------------------------------------------
class TIniFileStorage : public TCustomIniFileStorage
{
public:
  static TIniFileStorage *CreateFromPath(const UnicodeString AStorage);
  virtual ~TIniFileStorage() noexcept;

  virtual void Flush();

private:
  TIniFileStorage(const UnicodeString FileName, TCustomIniFile *IniFile);
  TStrings *FOriginal;
  void ApplyOverrides();
};
//---------------------------------------------------------------------------
class TOptionsStorage : public TCustomIniFileStorage
{
public:
  TOptionsStorage(TStrings *Options, bool AllowWrite);
  TOptionsStorage(TStrings *Options, const UnicodeString &RootKey, THierarchicalStorage *MasterStorage);

protected:
  virtual bool GetTemporary();
};
#endif // #if 0
//---------------------------------------------------------------------------
NB_CORE_EXPORT UnicodeString PuttyMungeStr(const UnicodeString Str);
NB_CORE_EXPORT UnicodeString PuttyUnMungeStr(const UnicodeString Str);
//---------------------------------------------------------------------------
