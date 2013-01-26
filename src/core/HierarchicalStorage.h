//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

#include <registry.hpp>
//---------------------------------------------------------------------------
enum TStorage { stDetect, stRegistry, stIniFile, stNul, stXmlFile };
enum TStorageAccessMode { smRead, smReadWrite };
//---------------------------------------------------------------------------
class THierarchicalStorage
{
public:
  explicit THierarchicalStorage(const UnicodeString & AStorage);
  virtual void Init() {}
  virtual ~THierarchicalStorage();
  bool OpenRootKey(bool CanCreate);
  bool OpenSubKey(const UnicodeString & SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const UnicodeString & SubKey) = 0;
  virtual void GetSubKeyNames(Classes::TStrings * Strings) = 0;
  virtual void GetValueNames(Classes::TStrings * Strings) = 0;
  bool HasSubKeys();
  bool HasSubKey(const UnicodeString & SubKey);
  bool KeyExists(const UnicodeString & SubKey);
  virtual bool ValueExists(const UnicodeString & Value) = 0;
  virtual void RecursiveDeleteSubKey(const UnicodeString & Key);
  virtual void ClearSubKeys();
  virtual void ReadValues(Classes::TStrings * Strings, bool MaintainKeys = false);
  virtual void WriteValues(Classes::TStrings * Strings, bool MaintainKeys = false);
  virtual void ClearValues();
  virtual bool DeleteValue(const UnicodeString & Name) = 0;

  virtual size_t BinaryDataSize(const UnicodeString & Name) = 0;

  virtual bool ReadBool(const UnicodeString & Name, bool Default) = 0;
  virtual int ReadInteger(const UnicodeString & Name, int Default) = 0;
  virtual __int64 ReadInt64(const UnicodeString & Name, __int64 Default) = 0;
  virtual TDateTime ReadDateTime(const UnicodeString & Name, TDateTime Default) = 0;
  virtual double ReadFloat(const UnicodeString & Name, double Default) = 0;
  virtual UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) = 0;
  virtual size_t ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) = 0;

  virtual UnicodeString ReadString(const UnicodeString & Name, const UnicodeString & Default);
  RawByteString ReadBinaryData(const UnicodeString & Name);
  RawByteString ReadStringAsBinaryData(const UnicodeString & Name, const RawByteString & Default);

  virtual void WriteBool(const UnicodeString & Name, bool Value) = 0;
  virtual void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) = 0;
  virtual void WriteInteger(const UnicodeString & Name, int Value) = 0;
  virtual void WriteInt64(const UnicodeString & Name, __int64 Value) = 0;
  virtual void WriteDateTime(const UnicodeString & Name, TDateTime Value) = 0;
  virtual void WriteFloat(const UnicodeString & Name, double Value) = 0;
  virtual void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size) = 0;

  virtual void WriteString(const UnicodeString & Name, const UnicodeString & Value);
  void WriteBinaryData(const UnicodeString & Name, const RawByteString & Value);
  void WriteBinaryDataAsString(const UnicodeString & Name, const RawByteString & Value);

  virtual void Flush();

  UnicodeString GetStorage() const { return FStorage; }
  TStorageAccessMode GetAccessMode() const { return FAccessMode; }
  bool GetExplicit() const { return FExplicit; }
  void SetExplicit(bool value) { FExplicit = value; }
  bool GetForceAnsi() const { return FForceAnsi; }
  void SetForceAnsi(bool value) { FForceAnsi = value; }
  bool GetMungeStringValues() const { return FMungeStringValues; }
  void SetMungeStringValues(bool value) { FMungeStringValues = value; }

  virtual void SetAccessMode(TStorageAccessMode value);
  virtual UnicodeString GetSource() = 0;

protected:
  UnicodeString FStorage;
  TStrings * FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit;
  bool FMungeStringValues;
  bool FForceAnsi;

  UnicodeString GetCurrentSubKey() const;
  UnicodeString GetCurrentSubKeyMunged() const;
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi) = 0;
  static UnicodeString IncludeTrailingBackslash(const UnicodeString & S);
  static UnicodeString ExcludeTrailingBackslash(const UnicodeString & S);
  virtual bool DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate) = 0;
  UnicodeString MungeKeyName(const UnicodeString & Key);

private:
  THierarchicalStorage(const THierarchicalStorage &);
  THierarchicalStorage & operator=(const THierarchicalStorage &);
};
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
  explicit TRegistryStorage(const UnicodeString & AStorage, HKEY ARootKey);
  explicit TRegistryStorage(const UnicodeString & AStorage);
  virtual void Init();
  virtual ~TRegistryStorage();

  bool Copy(TRegistryStorage * Storage);

  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const UnicodeString & SubKey);
  virtual bool DeleteValue(const UnicodeString & Name);
  virtual void GetSubKeyNames(Classes::TStrings * Strings);
  virtual bool ValueExists(const UnicodeString & Value);

  virtual size_t BinaryDataSize(const UnicodeString & Name);

  virtual bool ReadBool(const UnicodeString & Name, bool Default);
  virtual int ReadInteger(const UnicodeString & Name, int Default);
  virtual __int64 ReadInt64(const UnicodeString & Name, __int64 Default);
  virtual TDateTime ReadDateTime(const UnicodeString & Name, TDateTime Default);
  virtual double ReadFloat(const UnicodeString & Name, double Default);
  virtual UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  virtual size_t ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

  virtual void WriteBool(const UnicodeString & Name, bool Value);
  virtual void WriteInteger(const UnicodeString & Name, int Value);
  virtual void WriteInt64(const UnicodeString & Name, __int64 Value);
  virtual void WriteDateTime(const UnicodeString & Name, TDateTime Value);
  virtual void WriteFloat(const UnicodeString & Name, double Value);
  virtual void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size);

  virtual void GetValueNames(Classes::TStrings * Strings);

  int GetFailed();
  void SetFailed(int value) { FFailed = value; }
  virtual void SetAccessMode(TStorageAccessMode value);

protected:
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual UnicodeString GetSource();

private:
  TRegistry * FRegistry;
  int FFailed;
};
#ifndef _MSC_VER
//---------------------------------------------------------------------------
class TCustomIniFileStorage : public THierarchicalStorage
{
public:
  explicit TCustomIniFileStorage(const UnicodeString & Storage, TCustomIniFile * IniFile);
  virtual void Init() {}
  virtual ~TCustomIniFileStorage();

  virtual bool DeleteSubKey(const UnicodeString & SubKey);
  virtual bool DeleteValue(const UnicodeString & Name);
  virtual void GetSubKeyNames(Classes::TStrings * Strings);
  virtual bool ValueExists(const UnicodeString & Value);

  virtual size_t BinaryDataSize(const UnicodeString & Name);

  virtual bool ReadBool(const UnicodeString & Name, bool Default);
  virtual int ReadInteger(const UnicodeString & Name, int Default);
  virtual __int64 ReadInt64(const UnicodeString & Name, __int64 Default);
  virtual TDateTime ReadDateTime(const UnicodeString & Name, TDateTime Default);
  virtual double ReadFloat(const UnicodeString & Name, double Default);
  virtual UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  virtual size_t ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

  virtual void WriteBool(const UnicodeString & Name, bool Value);
  virtual void WriteInteger(const UnicodeString & Name, int Value);
  virtual void WriteInt64(const UnicodeString & Name, __int64 Value);
  virtual void WriteDateTime(const UnicodeString & Name, TDateTime Value);
  virtual void WriteFloat(const UnicodeString & Name, double Value);
  virtual void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size);

  virtual void GetValueNames(Classes::TStrings * Strings);

private:
  UnicodeString GetCurrentSection() const;

protected:
  TCustomIniFile * FIniFile;

  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual UnicodeString GetSource();
};
//---------------------------------------------------------------------------
class TIniFileStorage : public TCustomIniFileStorage
{
public:
  explicit TIniFileStorage(const UnicodeString & FileName);
  virtual ~TIniFileStorage();

  virtual void Flush();

private:
  TStrings * FOriginal;
  void ApplyOverrides();
};
//---------------------------------------------------------------------------
class TOptionsStorage : public TCustomIniFileStorage
{
public:
  explicit TOptionsStorage(TStrings * Options);
};
#endif
//---------------------------------------------------------------------------
UnicodeString PuttyMungeStr(const UnicodeString & Str);
UnicodeString PuttyUnMungeStr(const UnicodeString & Str);
//---------------------------------------------------------------------------
#endif
