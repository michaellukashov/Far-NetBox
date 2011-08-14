//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

#include <registry.hpp>
//---------------------------------------------------------------------------
enum TStorage { stDetect, stRegistry, stIniFile };
enum TStorageAccessMode { smRead, smReadWrite };
//---------------------------------------------------------------------------
class THierarchicalStorage
{
public:
  THierarchicalStorage(const AnsiString AStorage);
  virtual ~THierarchicalStorage();
  bool OpenRootKey(bool CanCreate);
  virtual bool OpenSubKey(const AnsiString SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const AnsiString SubKey) = 0;
  virtual void GetSubKeyNames(Classes::TStrings* Strings) = 0;
  virtual void GetValueNames(Classes::TStrings* Strings) = 0;
  bool HasSubKeys();
  bool HasSubKey(const AnsiString SubKey);
  virtual bool KeyExists(const AnsiString SubKey) = 0;
  virtual bool ValueExists(const AnsiString Value) = 0;
  virtual void RecursiveDeleteSubKey(const AnsiString Key);
  virtual void ClearSubKeys();
  virtual void ReadValues(Classes::TStrings* Strings, bool MaintainKeys = false);
  virtual void WriteValues(Classes::TStrings* Strings, bool MaintainKeys = false);
  virtual void ClearValues();
  virtual bool DeleteValue(const AnsiString Name) = 0;

  virtual int BinaryDataSize(const AnsiString Name) = 0;

  virtual bool ReadBool(const AnsiString Name, bool Default) = 0;
  virtual int ReadInteger(const AnsiString Name, int Default) = 0;
  virtual __int64 ReadInt64(const AnsiString Name, __int64 Default) = 0;
  virtual TDateTime ReadDateTime(const AnsiString Name, TDateTime Default) = 0;
  virtual double ReadFloat(const AnsiString Name, double Default) = 0;
  virtual AnsiString ReadStringRaw(const AnsiString Name, const AnsiString Default) = 0;
  virtual int ReadBinaryData(const AnsiString Name, void * Buffer, int Size) = 0;

  virtual AnsiString ReadString(AnsiString Name, AnsiString Default);
  AnsiString ReadBinaryData(const AnsiString Name);

  virtual void WriteBool(const AnsiString Name, bool Value) = 0;
  virtual void WriteStringRaw(const AnsiString Name, const AnsiString Value) = 0;
  virtual void WriteInteger(const AnsiString Name, int Value) = 0;
  virtual void WriteInt64(AnsiString Name, __int64 Value) = 0;
  virtual void WriteDateTime(const AnsiString Name, TDateTime Value) = 0;
  virtual void WriteFloat(const AnsiString Name, double Value) = 0;
  virtual void WriteBinaryData(const AnsiString Name, const void * Buffer, int Size) = 0;

  virtual void WriteString(const AnsiString Name, const AnsiString Value);
  void WriteBinaryData(const AnsiString Name, const AnsiString Value);

  __property AnsiString Storage  = { read=FStorage };
  __property AnsiString CurrentSubKey  = { read=GetCurrentSubKey };
  __property TStorageAccessMode AccessMode  = { read=FAccessMode, write=SetAccessMode };
  __property bool Explicit = { read = FExplicit, write = FExplicit };
  __property bool MungeStringValues = { read = FMungeStringValues, write = FMungeStringValues };
  __property AnsiString Source = { read = GetSource };

protected:
  AnsiString FStorage;
  TStrings * FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit;
  bool FMungeStringValues;

  AnsiString GetCurrentSubKey();
  AnsiString GetCurrentSubKeyMunged();
  virtual void SetAccessMode(TStorageAccessMode value);
  static AnsiString IncludeTrailingBackslash(const AnsiString & S);
  static AnsiString ExcludeTrailingBackslash(const AnsiString & S);
  AnsiString MungeSubKey(AnsiString Key, bool Path);
  virtual AnsiString GetSource() = 0;
};
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
  TRegistryStorage(const AnsiString AStorage, HKEY ARootKey);
  TRegistryStorage(const AnsiString AStorage);
  virtual ~TRegistryStorage();

  bool Copy(TRegistryStorage * Storage);

  virtual bool OpenSubKey(const AnsiString SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const AnsiString SubKey);
  virtual bool DeleteValue(const AnsiString Name);
  virtual void GetSubKeyNames(Classes::TStrings* Strings);
  virtual bool KeyExists(const AnsiString SubKey);
  virtual bool ValueExists(const AnsiString Value);

  virtual int BinaryDataSize(const AnsiString Name);

  virtual bool ReadBool(const AnsiString Name, bool Default);
  virtual int ReadInteger(const AnsiString Name, int Default);
  virtual __int64 ReadInt64(const AnsiString Name, __int64 Default);
  virtual TDateTime ReadDateTime(const AnsiString Name, TDateTime Default);
  virtual double ReadFloat(const AnsiString Name, double Default);
  virtual AnsiString ReadStringRaw(const AnsiString Name, const AnsiString Default);
  virtual int ReadBinaryData(const AnsiString Name, void * Buffer, int Size);

  virtual void WriteBool(const AnsiString Name, bool Value);
  virtual void WriteInteger(const AnsiString Name, int Value);
  virtual void WriteInt64(const AnsiString Name, __int64 Value);
  virtual void WriteDateTime(const AnsiString Name, TDateTime Value);
  virtual void WriteFloat(const AnsiString Name, double Value);
  virtual void WriteStringRaw(const AnsiString Name, const AnsiString Value);
  virtual void WriteBinaryData(const AnsiString Name, const void * Buffer, int Size);

  virtual void GetValueNames(Classes::TStrings* Strings);

protected:
  int GetFailed();
  virtual void SetAccessMode(TStorageAccessMode value);
  virtual AnsiString GetSource();

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
  TIniFileStorage(const AnsiString FileName);
  virtual ~TIniFileStorage();

  virtual bool OpenSubKey(const AnsiString SubKey, bool CanCreate, bool Path = false);
  virtual bool DeleteSubKey(const AnsiString SubKey);
  virtual bool DeleteValue(const AnsiString Name);
  virtual void GetSubKeyNames(Classes::TStrings* Strings);
  virtual bool KeyExists(const AnsiString SubKey);
  virtual bool ValueExists(const AnsiString Value);

  virtual int BinaryDataSize(const AnsiString Name);

  virtual bool ReadBool(const AnsiString Name, bool Default);
  virtual int ReadInteger(const AnsiString Name, int Default);
  virtual __int64 ReadInt64(const AnsiString Name, __int64 Default);
  virtual TDateTime ReadDateTime(const AnsiString Name, TDateTime Default);
  virtual double ReadFloat(const AnsiString Name, double Default);
  virtual AnsiString ReadStringRaw(const AnsiString Name, const AnsiString Default);
  virtual int ReadBinaryData(const AnsiString Name, void * Buffer, int Size);

  virtual void WriteBool(const AnsiString Name, bool Value);
  virtual void WriteInteger(const AnsiString Name, int Value);
  virtual void WriteInt64(AnsiString Name, __int64 Value);
  virtual void WriteDateTime(const AnsiString Name, TDateTime Value);
  virtual void WriteFloat(const AnsiString Name, double Value);
  virtual void WriteStringRaw(const AnsiString Name, const AnsiString Value);
  virtual void WriteBinaryData(const AnsiString Name, const void * Buffer, int Size);

  virtual void GetValueNames(Classes::TStrings* Strings);

private:
  TMemIniFile * FIniFile;
  TStrings * FOriginal;
  AnsiString GetCurrentSection();
  void ApplyOverrides();
protected:
  __property AnsiString CurrentSection  = { read=GetCurrentSection };
  virtual AnsiString GetSource();
};
//---------------------------------------------------------------------------
AnsiString PuttyMungeStr(const AnsiString Str);
//---------------------------------------------------------------------------
#endif
