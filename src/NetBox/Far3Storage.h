#pragma once

#include <vector.h>

#include <Classes.hpp>
#include "HierarchicalStorage.h"
#include "PluginSettings.hpp"


class TFar3Storage : public THierarchicalStorage
{
public:
  explicit TFar3Storage(UnicodeString AStorage,
    const GUID & Guid, FARAPISETTINGSCONTROL SettingsControl);
  virtual ~TFar3Storage();

  bool Copy(TFar3Storage * Storage);

  virtual void CloseSubKey() override;
  virtual bool DeleteSubKey(UnicodeString SubKey) override;
  virtual bool DeleteValue(UnicodeString Name) override;
  virtual size_t BinaryDataSize(UnicodeString Name) const override;
  virtual void GetSubKeyNames(TStrings * Strings) override;
  virtual bool ValueExists(UnicodeString Value) const override;

  virtual bool ReadBool(UnicodeString Name, bool Default) const override;
  virtual intptr_t ReadInteger(UnicodeString Name, intptr_t Default) const override;
  virtual __int64 ReadInt64(UnicodeString Name, __int64 Default) const override;
  virtual TDateTime ReadDateTime(UnicodeString Name, const TDateTime & Default) const override;
  virtual double ReadFloat(UnicodeString Name, double Default) const override;
  virtual UnicodeString ReadStringRaw(UnicodeString Name, UnicodeString Default) const override;
  virtual size_t ReadBinaryData(UnicodeString Name, void * Buffer, size_t Size) const override;

  virtual void WriteBool(UnicodeString Name, bool Value) override;
  virtual void WriteStringRaw(UnicodeString Name, UnicodeString Value) override;
  virtual void WriteInteger(UnicodeString Name, intptr_t Value) override;
  virtual void WriteInt64(UnicodeString Name, __int64 Value) override;
  virtual void WriteDateTime(UnicodeString Name, const TDateTime & AValue) override;
  virtual void WriteFloat(UnicodeString Name, double AValue) override;
  virtual void WriteBinaryData(UnicodeString Name, const void * Buffer, size_t Size) override;

  virtual void GetValueNames(TStrings * Strings) const override;
  virtual void SetAccessMode(TStorageAccessMode Value) override;
  virtual bool DoKeyExists(UnicodeString SubKey, bool ForceAnsi) override;
  virtual bool DoOpenSubKey(UnicodeString MungedSubKey, bool CanCreate) override;
  virtual UnicodeString GetSource() const override;
  virtual UnicodeString GetSource() override;

private:
  UnicodeString GetFullCurrentSubKey() { return /* GetStorage() + */ GetCurrentSubKey(); }
  int OpenSubKeyInternal(int Root, UnicodeString SubKey, bool CanCreate);

private:
  int FRoot;
  mutable PluginSettings FPluginSettings;
  rde::vector<int> FSubKeyIds;

  void Init();
};

