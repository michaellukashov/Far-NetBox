#pragma once

#include <vector>

#include "Classes.h"
#include "HierarchicalStorage.h"
#include "PluginSettings.hpp"

//---------------------------------------------------------------------------
class TFar3Storage : public THierarchicalStorage
{
public:
  explicit /* __fastcall */ TFar3Storage(const UnicodeString AStorage,
                                         const GUID & guid, FARAPISETTINGSCONTROL SettingsControl);
  virtual /* __fastcall */ ~TFar3Storage();

  bool __fastcall Copy(TFar3Storage * Storage);

  virtual void __fastcall CloseSubKey();
  virtual bool __fastcall DeleteSubKey(const UnicodeString SubKey);
  virtual bool __fastcall DeleteValue(const UnicodeString Name);
  virtual void __fastcall GetSubKeyNames(TStrings * Strings);
  virtual bool __fastcall ValueExists(const UnicodeString Value);

  virtual size_t __fastcall BinaryDataSize(const UnicodeString Name);

  virtual bool __fastcall ReadBool(const UnicodeString Name, bool Default);
  virtual int __fastcall ReadInteger(const UnicodeString Name, int Default);
  virtual __int64 __fastcall ReadInt64(const UnicodeString Name, __int64 Default);
  virtual TDateTime __fastcall ReadDateTime(const UnicodeString Name, TDateTime Default);
  virtual double __fastcall ReadFloat(const UnicodeString Name, double Default);
  virtual UnicodeString __fastcall ReadStringRaw(const UnicodeString Name, const UnicodeString Default);
  virtual size_t __fastcall ReadBinaryData(const UnicodeString Name, void * Buffer, size_t Size);

  virtual void __fastcall WriteBool(const UnicodeString Name, bool Value);
  virtual void __fastcall WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
  virtual void __fastcall WriteInteger(const UnicodeString Name, int Value);
  virtual void __fastcall WriteInt64(const UnicodeString Name, __int64 Value);
  virtual void __fastcall WriteDateTime(const UnicodeString Name, TDateTime Value);
  virtual void __fastcall WriteFloat(const UnicodeString Name, double Value);
  virtual void __fastcall WriteBinaryData(const UnicodeString Name, const void * Buffer, int Size);

  virtual void __fastcall GetValueNames(TStrings * Strings);
  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
  virtual bool __fastcall DoKeyExists(const UnicodeString SubKey, bool ForceAnsi);
  virtual bool __fastcall DoOpenSubKey(const UnicodeString MungedSubKey, bool CanCreate);
  virtual UnicodeString __fastcall GetSource();

private:
  UnicodeString __fastcall GetFullCurrentSubKey() { return /* GetStorage() + */ GetCurrentSubKey(); }
  int __fastcall OpenSubKeyInternal(int Root, const UnicodeString SubKey, bool CanCreate, bool Path);

private:
  int FRoot;
  PluginSettings FPluginSettings;
  std::vector<int> FSubKeyIds;

  void __fastcall Init();
};
//---------------------------------------------------------------------------
