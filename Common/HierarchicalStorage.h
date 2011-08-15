//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

// #include <registry.hpp>
#include "Classes.h"
//---------------------------------------------------------------------------
enum TStorage { stDetect, stRegistry, stIniFile };
enum TStorageAccessMode { smRead, smReadWrite };
//---------------------------------------------------------------------------
class THierarchicalStorage
{
public:
  THierarchicalStorage(const wstring AStorage);
  virtual ~THierarchicalStorage();
  bool OpenRootKey(bool CanCreate);
  virtual bool OpenSubKey(const wstring SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const wstring SubKey) = 0;
  virtual void GetSubKeyNames(TStrings* Strings) = 0;
  virtual void GetValueNames(TStrings* Strings) = 0;
  bool HasSubKeys();
  bool HasSubKey(const wstring SubKey);
  virtual bool KeyExists(const wstring SubKey) = 0;
  virtual bool ValueExists(const wstring Value) = 0;
  virtual void RecursiveDeleteSubKey(const wstring Key);
  virtual void ClearSubKeys();
  virtual void ReadValues(TStrings* Strings, bool MaintainKeys = false);
  virtual void WriteValues(TStrings* Strings, bool MaintainKeys = false);
  virtual void ClearValues();
  virtual bool DeleteValue(const wstring Name) = 0;

  virtual int BinaryDataSize(const wstring Name) = 0;

  virtual bool ReadBool(const wstring Name, bool Default) = 0;
  virtual int ReadInteger(const wstring Name, int Default) = 0;
  virtual __int64 ReadInt64(const wstring Name, __int64 Default) = 0;
  virtual TDateTime ReadDateTime(const wstring Name, TDateTime Default) = 0;
  virtual double ReadFloat(const wstring Name, double Default) = 0;
  virtual wstring ReadStringRaw(const wstring Name, const wstring Default) = 0;
  virtual int ReadBinaryData(const wstring Name, void * Buffer, int Size) = 0;

  virtual wstring ReadString(wstring Name, wstring Default);
  wstring ReadBinaryData(const wstring Name);

  virtual void WriteBool(const wstring Name, bool Value) = 0;
  virtual void WriteStringRaw(const wstring Name, const wstring Value) = 0;
  virtual void WriteInteger(const wstring Name, int Value) = 0;
  virtual void WriteInt64(wstring Name, __int64 Value) = 0;
  virtual void WriteDateTime(const wstring Name, TDateTime Value) = 0;
  virtual void WriteFloat(const wstring Name, double Value) = 0;
  virtual void WriteBinaryData(const wstring Name, const void * Buffer, int Size) = 0;

  virtual void WriteString(const wstring Name, const wstring Value);
  void WriteBinaryData(const wstring Name, const wstring Value);

  // __property wstring Storage  = { read=FStorage };
  wstring GetStorage() { return FStorage; }
  // __property wstring CurrentSubKey  = { read=GetCurrentSubKey };
  wstring GetCurrentSubKey();
  // __property TStorageAccessMode AccessMode  = { read=FAccessMode, write=SetAccessMode };
  TStorageAccessMode GetAccessMode() { return FAccessMode; }
  virtual void SetAccessMode(TStorageAccessMode value);
  // __property bool Explicit = { read = FExplicit, write = FExplicit };
  bool GetExplicit() { return FExplicit; }
  void SetExplicit(bool value) { FExplicit = value; }
  // __property bool MungeStringValues = { read = FMungeStringValues, write = FMungeStringValues };
  bool GetMungeStringValues() { return FMungeStringValues; }
  void SetMungeStringValues(bool value) { FMungeStringValues = value; }
  // __property wstring Source = { read = GetSource };
  virtual wstring GetSource() = 0;

protected:
  wstring FStorage;
  TStrings *FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit;
  bool FMungeStringValues;

  wstring GetCurrentSubKeyMunged();
  static wstring IncludeTrailingBackslash(const wstring & S);
  static wstring ExcludeTrailingBackslash(const wstring & S);
  wstring MungeSubKey(wstring Key, bool Path);
};
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
  TRegistryStorage(const wstring AStorage, HKEY ARootKey);
  TRegistryStorage(const wstring AStorage);
  virtual ~TRegistryStorage();

  bool Copy(TRegistryStorage * Storage);

  virtual bool OpenSubKey(const wstring SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const wstring SubKey);
  virtual bool DeleteValue(const wstring Name);
  virtual void GetSubKeyNames(TStrings* Strings);
  virtual bool KeyExists(const wstring SubKey);
  virtual bool ValueExists(const wstring Value);

  virtual int BinaryDataSize(const wstring Name);

  virtual bool ReadBool(const wstring Name, bool Default);
  virtual int ReadInteger(const wstring Name, int Default);
  virtual __int64 ReadInt64(const wstring Name, __int64 Default);
  virtual TDateTime ReadDateTime(const wstring Name, TDateTime Default);
  virtual double ReadFloat(const wstring Name, double Default);
  virtual wstring ReadStringRaw(const wstring Name, const wstring Default);
  virtual int ReadBinaryData(const wstring Name, void * Buffer, int Size);

  virtual void WriteBool(const wstring Name, bool Value);
  virtual void WriteInteger(const wstring Name, int Value);
  virtual void WriteInt64(const wstring Name, __int64 Value);
  virtual void WriteDateTime(const wstring Name, TDateTime Value);
  virtual void WriteFloat(const wstring Name, double Value);
  virtual void WriteStringRaw(const wstring Name, const wstring Value);
  virtual void WriteBinaryData(const wstring Name, const void * Buffer, int Size);

  virtual void GetValueNames(TStrings* Strings);

protected:
  int GetFailed();
  virtual void SetAccessMode(TStorageAccessMode value);
  virtual wstring GetSource();

  __property int Failed  = { read=GetFailed, write=FFailed };

private:
  TRegistry * FRegistry;
  int FFailed;

  void Init();
};
//---------------------------------------------------------------------------
class TIniFileStorage : public THierarchicalStorage
{
public:
  TIniFileStorage(const wstring FileName);
  virtual ~TIniFileStorage();

  virtual bool OpenSubKey(const wstring SubKey, bool CanCreate, bool Path = false);
  virtual bool DeleteSubKey(const wstring SubKey);
  virtual bool DeleteValue(const wstring Name);
  virtual void GetSubKeyNames(TStrings* Strings);
  virtual bool KeyExists(const wstring SubKey);
  virtual bool ValueExists(const wstring Value);

  virtual int BinaryDataSize(const wstring Name);

  virtual bool ReadBool(const wstring Name, bool Default);
  virtual int ReadInteger(const wstring Name, int Default);
  virtual __int64 ReadInt64(const wstring Name, __int64 Default);
  virtual TDateTime ReadDateTime(const wstring Name, TDateTime Default);
  virtual double ReadFloat(const wstring Name, double Default);
  virtual wstring ReadStringRaw(const wstring Name, const wstring Default);
  virtual int ReadBinaryData(const wstring Name, void * Buffer, int Size);

  virtual void WriteBool(const wstring Name, bool Value);
  virtual void WriteInteger(const wstring Name, int Value);
  virtual void WriteInt64(wstring Name, __int64 Value);
  virtual void WriteDateTime(const wstring Name, TDateTime Value);
  virtual void WriteFloat(const wstring Name, double Value);
  virtual void WriteStringRaw(const wstring Name, const wstring Value);
  virtual void WriteBinaryData(const wstring Name, const void * Buffer, int Size);

  virtual void GetValueNames(TStrings* Strings);

private:
  TMemIniFile * FIniFile;
  TStrings * FOriginal;
  wstring GetCurrentSection();
  void ApplyOverrides();
protected:
  __property wstring CurrentSection  = { read=GetCurrentSection };
  virtual wstring GetSource();
};
//---------------------------------------------------------------------------
wstring PuttyMungeStr(const wstring Str);
//---------------------------------------------------------------------------
#endif
