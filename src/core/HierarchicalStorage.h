
#pragma once

#include <registry.hpp>
#include <memory>

enum TStorage
{
  stDetect,
  stRegistry,
  stIniFile,
  stNul,
  stXmlFile,
  stFar3Storage
};

enum TStorageAccessMode
{
  smRead,
  smReadWrite,
};

class NB_CORE_EXPORT THierarchicalStorage : public TObject
{
  NB_DISABLE_COPY(THierarchicalStorage)

public:
  explicit THierarchicalStorage(UnicodeString AStorage);
  virtual ~THierarchicalStorage();
  virtual void Init() {}
  virtual bool OpenRootKey(bool CanCreate);
  virtual bool OpenSubKey(UnicodeString ASubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  void CloseAll();
  virtual bool DeleteSubKey(UnicodeString SubKey) = 0;
  virtual void GetSubKeyNames(TStrings *Strings) = 0;
  virtual void GetValueNames(TStrings *Strings) const = 0;
  bool HasSubKeys();
  bool HasSubKey(UnicodeString SubKey);
  bool KeyExists(UnicodeString SubKey);
  virtual bool ValueExists(UnicodeString Value) const = 0;
  virtual void RecursiveDeleteSubKey(UnicodeString Key);
  virtual void ClearSubKeys();
  virtual void ReadValues(TStrings *Strings, bool MaintainKeys = false);
  virtual void WriteValues(TStrings *Strings, bool MaintainKeys = false);
  virtual void ClearValues();
  virtual bool DeleteValue(UnicodeString Name) = 0;
  virtual size_t BinaryDataSize(UnicodeString Name) const = 0;

  virtual bool ReadBool(UnicodeString Name, bool Default) const = 0;
  virtual intptr_t ReadInteger(UnicodeString Name, intptr_t Default) const = 0;
  virtual int64_t ReadInt64(UnicodeString Name, int64_t Default) const = 0;
  virtual TDateTime ReadDateTime(UnicodeString Name, const TDateTime &Default) const = 0;
  virtual double ReadFloat(UnicodeString Name, double Default) const = 0;
  virtual UnicodeString ReadStringRaw(UnicodeString Name, UnicodeString Default) const = 0;
  virtual size_t ReadBinaryData(UnicodeString Name, void *Buffer, size_t Size) const = 0;

  virtual UnicodeString ReadString(UnicodeString Name, UnicodeString Default) const;
  RawByteString ReadBinaryData(UnicodeString Name) const;
  virtual RawByteString ReadStringAsBinaryData(UnicodeString Name, RawByteString Default) const;

  virtual void WriteBool(UnicodeString Name, bool Value) = 0;
  virtual void WriteStringRaw(UnicodeString Name, UnicodeString Value) = 0;
  virtual void WriteInteger(UnicodeString Name, intptr_t Value) = 0;
  virtual void WriteInt64(UnicodeString Name, int64_t Value) = 0;
  virtual void WriteDateTime(UnicodeString Name, const TDateTime &Value) = 0;
  virtual void WriteFloat(UnicodeString Name, double Value) = 0;
  virtual void WriteBinaryData(UnicodeString Name, const void *Buffer, size_t Size) = 0;

  virtual void WriteString(UnicodeString Name, UnicodeString Value);
  void WriteBinaryData(UnicodeString Name, RawByteString Value);
  virtual void WriteBinaryDataAsString(UnicodeString Name, RawByteString Value);

  virtual void Flush();

#if 0
  __property UnicodeString Storage  = { read=FStorage };
  __property UnicodeString CurrentSubKey  = { read=GetCurrentSubKey };
  __property TStorageAccessMode AccessMode  = { read=FAccessMode, write=SetAccessMode };
  __property bool Explicit = { read = FExplicit, write = FExplicit };
  __property bool ForceAnsi = { read = FForceAnsi, write = FForceAnsi };
  __property bool MungeStringValues = { read = FMungeStringValues, write = FMungeStringValues };
  __property UnicodeString Source = { read = GetSource };
  __property bool Temporary = { read = GetTemporary };
#endif // #if 0

  UnicodeString GetStorage() const { return FStorage; }
  TStorageAccessMode GetAccessMode() const { return FAccessMode; }
  bool GetExplicit() const { return FExplicit; }
  void SetExplicit(bool Value) { FExplicit = Value; }
  bool GetForceAnsi() const { return FForceAnsi; }
  void SetForceAnsi(bool Value) { FForceAnsi = Value; }
  bool GetMungeStringValues() const { return FMungeStringValues; }
  void SetMungeStringValues(bool Value) { FMungeStringValues = Value; }

  virtual void SetAccessMode(TStorageAccessMode Value);

protected:
  UnicodeString FStorage;
  TStrings *FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit;
  bool FMungeStringValues;
  bool FForceAnsi;

public:
  UnicodeString GetCurrentSubKey() const;
  UnicodeString GetCurrentSubKeyMunged() const;
  virtual bool DoKeyExists(UnicodeString SubKey, bool ForceAnsi) = 0;
  static UnicodeString IncludeTrailingBackslash(UnicodeString S);
  static UnicodeString ExcludeTrailingBackslash(UnicodeString S);
  virtual bool DoOpenSubKey(UnicodeString SubKey, bool CanCreate) = 0;
  UnicodeString MungeKeyName(UnicodeString Key);
  virtual UnicodeString GetSource() const = 0;
  virtual UnicodeString GetSource() = 0;
  virtual bool GetTemporary() const;
};

class NB_CORE_EXPORT TRegistryStorage : public THierarchicalStorage
{
  NB_DISABLE_COPY(TRegistryStorage)
public:
  explicit TRegistryStorage(UnicodeString AStorage, HKEY ARootKey);
  explicit TRegistryStorage(UnicodeString AStorage);
  virtual void Init();
  virtual ~TRegistryStorage();

  bool Copy(TRegistryStorage *Storage);

  virtual void CloseSubKey();
  virtual bool DeleteSubKey(UnicodeString SubKey);
  virtual bool DeleteValue(UnicodeString Name);
  virtual void GetSubKeyNames(TStrings *Strings);
  virtual bool ValueExists(UnicodeString Value) const;

  virtual size_t BinaryDataSize(UnicodeString Name) const;

  virtual bool ReadBool(UnicodeString Name, bool Default) const;
  virtual intptr_t ReadInteger(UnicodeString Name, intptr_t Default) const;
  virtual int64_t ReadInt64(UnicodeString Name, int64_t Default) const;
  virtual TDateTime ReadDateTime(UnicodeString Name, const TDateTime &Default) const;
  virtual double ReadFloat(UnicodeString Name, double Default) const;
  virtual UnicodeString ReadStringRaw(UnicodeString Name, UnicodeString Default) const;
  size_t ReadBinaryData(UnicodeString Name, void *Buffer, size_t Size) const;

  virtual void WriteBool(UnicodeString Name, bool Value);
  virtual void WriteInteger(UnicodeString Name, intptr_t Value);
  virtual void WriteInt64(UnicodeString Name, int64_t Value);
  virtual void WriteDateTime(UnicodeString Name, const TDateTime &Value);
  virtual void WriteFloat(UnicodeString Name, double Value);
  virtual void WriteStringRaw(UnicodeString Name, UnicodeString Value);
  void WriteBinaryData(UnicodeString Name, const void *Buffer, size_t Size);

  virtual void GetValueNames(TStrings *Strings) const;

protected:
  virtual bool DoKeyExists(UnicodeString SubKey, bool AForceAnsi);
  virtual bool DoOpenSubKey(UnicodeString SubKey, bool CanCreate);
  virtual UnicodeString GetSource() const;
  virtual UnicodeString GetSource();

#if 0
  __property int Failed  = { read=GetFailed, write=FFailed };
#endif // #if 0
public:
  intptr_t GetFailed() const;
  void SetFailed(intptr_t Value) { FFailed = Value; }
  virtual void SetAccessMode(TStorageAccessMode Value);

private:
  TRegistry *FRegistry;
  mutable intptr_t FFailed;
};

#if 0
class TCustomIniFileStorage : public THierarchicalStorage
{
public:
  __fastcall TCustomIniFileStorage(const UnicodeString Storage, TCustomIniFile *IniFile);
  virtual __fastcall ~TCustomIniFileStorage();

  virtual bool __fastcall OpenRootKey(bool CanCreate);
  virtual bool __fastcall OpenSubKey(UnicodeString SubKey, bool CanCreate, bool Path = false);
  virtual void __fastcall CloseSubKey();
  virtual bool __fastcall DeleteSubKey(const UnicodeString SubKey);
  virtual bool __fastcall DeleteValue(const UnicodeString Name);
  virtual void __fastcall GetSubKeyNames(Classes::TStrings *Strings);
  virtual bool __fastcall ValueExists(const UnicodeString Value);

  virtual size_t __fastcall BinaryDataSize(const UnicodeString Name);

  virtual bool __fastcall ReadBool(const UnicodeString Name, bool Default);
  virtual int __fastcall ReadInteger(const UnicodeString Name, int Default);
  virtual __int64 __fastcall ReadInt64(const UnicodeString Name, __int64 Default);
  virtual TDateTime __fastcall ReadDateTime(const UnicodeString Name, TDateTime Default);
  virtual double __fastcall ReadFloat(const UnicodeString Name, double Default);
  virtual UnicodeString __fastcall ReadStringRaw(const UnicodeString Name, const UnicodeString Default);
  virtual size_t __fastcall ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size);

  virtual void __fastcall WriteBool(const UnicodeString Name, bool Value);
  virtual void __fastcall WriteInteger(const UnicodeString Name, int Value);
  virtual void __fastcall WriteInt64(const UnicodeString Name, __int64 Value);
  virtual void __fastcall WriteDateTime(const UnicodeString Name, TDateTime Value);
  virtual void __fastcall WriteFloat(const UnicodeString Name, double Value);
  virtual void __fastcall WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
  virtual void __fastcall WriteBinaryData(const UnicodeString Name, const void *Buffer, int Size);

  virtual void __fastcall GetValueNames(Classes::TStrings *Strings);

private:
  UnicodeString __fastcall GetCurrentSection();
  inline bool __fastcall HandleByMasterStorage();
  inline bool __fastcall HandleReadByMasterStorage(UnicodeString Name);
  inline bool __fastcall DoValueExists(UnicodeString Value);
  void __fastcall DoWriteStringRaw(UnicodeString Name, UnicodeString Value);
  void __fastcall DoWriteBinaryData(UnicodeString Name, const void *Buffer, int Size);

protected:
  TCustomIniFile *FIniFile;
  std::unique_ptr<TStringList> FSections;
  std::unique_ptr<THierarchicalStorage> FMasterStorage;
  int FMasterStorageOpenFailures;
  bool FOpeningSubKey;

  __property UnicodeString CurrentSection  = { read=GetCurrentSection };
  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
  virtual bool __fastcall DoKeyExists(const UnicodeString SubKey, bool ForceAnsi);
  virtual bool __fastcall DoOpenSubKey(const UnicodeString SubKey, bool CanCreate);
  virtual UnicodeString __fastcall GetSource();
  void __fastcall CacheSections();
  void __fastcall ResetCache();
};
//---------------------------------------------------------------------------
class TIniFileStorage : public TCustomIniFileStorage
{
public:
  static TIniFileStorage *__fastcall CreateFromPath(const UnicodeString AStorage);
  virtual __fastcall ~TIniFileStorage();

  virtual void __fastcall Flush();

private:
  __fastcall TIniFileStorage(const UnicodeString FileName, TCustomIniFile *IniFile);
  TStrings *FOriginal;
  void __fastcall ApplyOverrides();
};
//---------------------------------------------------------------------------
class TOptionsStorage : public TCustomIniFileStorage
{
public:
  __fastcall TOptionsStorage(TStrings *Options, bool AllowWrite);
  __fastcall TOptionsStorage(TStrings *Options, UnicodeString RootKey, THierarchicalStorage *MasterStorage);

protected:
  virtual bool __fastcall GetTemporary();
};
#endif // #if 0

NB_CORE_EXPORT UnicodeString PuttyMungeStr(const UnicodeString Str);
NB_CORE_EXPORT UnicodeString PuttyUnMungeStr(const UnicodeString Str);

