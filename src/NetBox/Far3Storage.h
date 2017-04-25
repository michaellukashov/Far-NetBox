#pragma once

#include <vector.h>

#include <Classes.hpp>
#include "HierarchicalStorage.h"
#include "PluginSettings.hpp"


class TFar3Storage : public THierarchicalStorage
{
public:
  explicit TFar3Storage(const UnicodeString & AStorage,
    const GUID & Guid, FARAPISETTINGSCONTROL SettingsControl);
  virtual ~TFar3Storage();

  bool Copy(TFar3Storage * Storage);

  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const UnicodeString & SubKey);
  virtual bool DeleteValue(const UnicodeString & Name);
  virtual size_t BinaryDataSize(const UnicodeString & Name) const;
  virtual void GetSubKeyNames(TStrings * Strings);
  virtual bool ValueExists(const UnicodeString & Value) const;

  virtual bool ReadBool(const UnicodeString & Name, bool Default) const;
  virtual intptr_t ReadInteger(const UnicodeString & Name, intptr_t Default) const;
  virtual __int64 ReadInt64(const UnicodeString & Name, __int64 Default) const;
  virtual TDateTime ReadDateTime(const UnicodeString & Name, const TDateTime & Default) const;
  virtual double ReadFloat(const UnicodeString & Name, double Default) const;
  virtual UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) const;
  virtual size_t ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) const;

  virtual void WriteBool(const UnicodeString & Name, bool Value);
  virtual void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void WriteInteger(const UnicodeString & Name, intptr_t Value);
  virtual void WriteInt64(const UnicodeString & Name, __int64 Value);
  virtual void WriteDateTime(const UnicodeString & Name, const TDateTime & AValue);
  virtual void WriteFloat(const UnicodeString & Name, double AValue);
  virtual void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size);

  virtual void GetValueNames(TStrings * Strings) const;
  virtual void SetAccessMode(TStorageAccessMode Value);
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate);
  virtual UnicodeString GetSource() const;
  virtual UnicodeString GetSource();

private:
  UnicodeString GetFullCurrentSubKey() { return /* GetStorage() + */ GetCurrentSubKey(); }
  int OpenSubKeyInternal(int Root, const UnicodeString & SubKey, bool CanCreate);

private:
  int FRoot;
  mutable PluginSettings FPluginSettings;
  rde::vector<int> FSubKeyIds;

  void Init();
};

