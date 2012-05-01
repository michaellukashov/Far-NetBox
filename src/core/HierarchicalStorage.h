//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

#ifndef _MSC_VER
#include <registry.hpp>
#else
#include "Classes.h"
#endif
//---------------------------------------------------------------------------
#ifndef _MSC_VER
enum TStorage { stDetect, stRegistry, stIniFile, stNul };
#else
enum TStorage { stRegistry, stXmlFile, stDetect, stIniFile, stNul };
#endif
enum TStorageAccessMode { smRead, smReadWrite };
//---------------------------------------------------------------------------
class THierarchicalStorage
{
public:
  explicit THierarchicalStorage(const UnicodeString AStorage);
  virtual ~THierarchicalStorage();
  virtual void Init() {}
  bool __fastcall OpenRootKey(bool CanCreate);
  virtual bool __fastcall OpenSubKey(const UnicodeString SubKey, bool CanCreate, bool Path = false);
  virtual void __fastcall CloseSubKey();
  virtual bool __fastcall DeleteSubKey(const UnicodeString SubKey) = 0;
  virtual void __fastcall GetSubKeyNames(TStrings* Strings) = 0;
  virtual void __fastcall GetValueNames(TStrings* Strings) = 0;
  bool __fastcall HasSubKeys();
  bool __fastcall HasSubKey(const UnicodeString SubKey);
  virtual bool __fastcall KeyExists(const UnicodeString SubKey) = 0;
  virtual bool __fastcall ValueExists(const UnicodeString Value) = 0;
  virtual void __fastcall RecursiveDeleteSubKey(const UnicodeString Key);
  virtual void __fastcall ClearSubKeys();
  virtual void __fastcall ReadValues(TStrings * Strings, bool MaintainKeys = false);
  virtual void __fastcall WriteValues(TStrings * Strings, bool MaintainKeys = false);
  virtual void __fastcall ClearValues();
  virtual bool __fastcall DeleteValue(const UnicodeString Name) = 0;

  virtual int __fastcall BinaryDataSize(const UnicodeString Name) = 0;

  virtual bool __fastcall Readbool(const UnicodeString Name, bool Default) = 0;
  virtual int __fastcall Readint(const UnicodeString Name, int Default) = 0;
  virtual __int64 __fastcall ReadInt64(const UnicodeString Name, __int64 Default) = 0;
  virtual TDateTime __fastcall ReadDateTime(const UnicodeString Name, TDateTime Default) = 0;
  virtual double __fastcall ReadFloat(const UnicodeString Name, double Default) = 0;
  virtual UnicodeString __fastcall ReadStringRaw(const UnicodeString Name, const UnicodeString Default) = 0;
  virtual size_t __fastcall ReadBinaryData(const UnicodeString Name, void * Buffer, size_t Size) = 0;

  virtual UnicodeString __fastcall ReadString(const UnicodeString Name, const UnicodeString Default);
  UnicodeString __fastcall ReadBinaryData(const UnicodeString Name);

  virtual void __fastcall Writebool(const UnicodeString Name, bool Value) = 0;
  virtual void __fastcall WriteStringRaw(const UnicodeString Name, const UnicodeString Value) = 0;
  virtual void __fastcall Writeint(const UnicodeString Name, int Value) = 0;
  virtual void __fastcall WriteInt64(const UnicodeString Name, __int64 Value) = 0;
  virtual void __fastcall WriteDateTime(const UnicodeString Name, TDateTime Value) = 0;
  virtual void __fastcall WriteFloat(const UnicodeString Name, double Value) = 0;
  virtual void __fastcall WriteBinaryData(const UnicodeString Name, const void * Buffer, size_t Size) = 0;

  virtual void __fastcall WriteString(const UnicodeString Name, const UnicodeString Value);
  void __fastcall WriteBinaryData(const UnicodeString Name, const UnicodeString Value);

  UnicodeString __fastcall GetStorage() { return FStorage; }
  UnicodeString __fastcall GetCurrentSubKey();
  TStorageAccessMode __fastcall GetAccessMode() { return FAccessMode; }
  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
  bool __fastcall GetExplicit() { return FExplicit; }
  void __fastcall SetExplicit(bool value) { FExplicit = value; }
  bool __fastcall GetMungeStringValues() { return FMungeStringValues; }
  void __fastcall SetMungeStringValues(bool value) { FMungeStringValues = value; }
  virtual UnicodeString __fastcall GetSource() = 0;

protected:
  UnicodeString FStorage;
  TStrings * FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit;
  bool FMungeStringValues;

  UnicodeString __fastcall GetCurrentSubKeyMunged();
  static UnicodeString IncludeTrailingBackslash(const UnicodeString S);
  static UnicodeString ExcludeTrailingBackslash(const UnicodeString S);
  UnicodeString MungeSubKey(const UnicodeString Key, bool Path);
};
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
  explicit TRegistryStorage(const UnicodeString AStorage, HKEY ARootKey);
  explicit TRegistryStorage(const UnicodeString AStorage);
  virtual ~TRegistryStorage();

  bool Copy(TRegistryStorage * Storage);

  virtual bool __fastcall OpenSubKey(const UnicodeString SubKey, bool CanCreate, bool Path = false);
  virtual void __fastcall CloseSubKey();
  virtual bool __fastcall DeleteSubKey(const UnicodeString SubKey);
  virtual bool __fastcall DeleteValue(const UnicodeString Name);
  virtual void __fastcall GetSubKeyNames(TStrings * Strings);
  virtual bool __fastcall KeyExists(const UnicodeString SubKey);
  virtual bool __fastcall ValueExists(const UnicodeString Value);

  virtual int __fastcall BinaryDataSize(const UnicodeString Name);

  virtual bool __fastcall Readbool(const UnicodeString Name, bool Default);
  virtual int __fastcall Readint(const UnicodeString Name, int Default);
  virtual __int64 __fastcall ReadInt64(const UnicodeString Name, __int64 Default);
  virtual TDateTime __fastcall ReadDateTime(const UnicodeString Name, TDateTime Default);
  virtual double __fastcall ReadFloat(const UnicodeString Name, double Default);
  virtual UnicodeString __fastcall ReadStringRaw(const UnicodeString Name, const UnicodeString Default);
  virtual size_t __fastcall ReadBinaryData(const UnicodeString Name, void * Buffer, size_t Size);

  virtual void __fastcall Writebool(const UnicodeString Name, bool Value);
  virtual void __fastcall Writeint(const UnicodeString Name, int Value);
  virtual void __fastcall WriteInt64(const UnicodeString Name, __int64 Value);
  virtual void __fastcall WriteDateTime(const UnicodeString Name, TDateTime Value);
  virtual void __fastcall WriteFloat(const UnicodeString Name, double Value);
  virtual void __fastcall WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
  virtual void __fastcall WriteBinaryData(const UnicodeString Name, const void * Buffer, size_t Size);

  virtual void __fastcall GetValueNames(TStrings * Strings);

  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
protected:
  virtual UnicodeString __fastcall GetSource();

  int GetFailed();
  void SetFailed(int value) { FFailed = value; }

private:
  TRegistry * FRegistry;
  int FFailed;

  void Init();
};
//---------------------------------------------------------------------------
class TOptionsStorage : public TRegistryStorage
{
public:
  explicit TOptionsStorage(TStrings * Options);
};
//---------------------------------------------------------------------------
UnicodeString PuttyMungeStr(const UnicodeString Str);
UnicodeString PuttyUnMungeStr(const UnicodeString Str);
//---------------------------------------------------------------------------
#endif
