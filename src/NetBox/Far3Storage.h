#pragma once

#include <rdestl/vector.h>

#include <Classes.hpp>
#include "HierarchicalStorage.h"
#include "PluginSettings.hpp"


class TFar3Storage : public THierarchicalStorage
{
public:
  explicit TFar3Storage(const UnicodeString & AStorage,
    const GUID & Guid, FARAPISETTINGSCONTROL SettingsControl);
  virtual ~TFar3Storage() noexcept override = default;

  bool Copy(TFar3Storage * Storage);

  virtual void DoCloseSubKey() override;
  virtual void DoDeleteSubKey(const UnicodeString & SubKey) override;
  virtual void DoGetSubKeyNames(TStrings * Strings) override;
  virtual bool DoValueExists(const UnicodeString & Value) override;
  virtual bool DoDeleteValue(const UnicodeString & Name) override;
//  virtual size_t DoBinaryDataSize(const UnicodeString & Name) const override;
  virtual int32_t DoBinaryDataSize(const UnicodeString & Name) override;

  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi) override;
  virtual bool DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate) override;
  virtual UnicodeString GetSource() const override;
//  virtual void DoGetSubKeyNames(TStrings *Strings) override;
  virtual void DoGetValueNames(TStrings *Strings) override;

//  virtual void DoSetAccessMode(TStorageAccessMode Value) override;

  virtual void DoWriteBool(const UnicodeString & Name, bool Value) override;
  virtual void DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) override;
  virtual void DoWriteInteger(const UnicodeString & Name, int32_t Value) override;
  virtual void DoWriteInt64(const UnicodeString & Name, int64_t Value) override;
  virtual void DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int32_t Size) override;

  virtual bool DoReadBool(const UnicodeString & Name, bool Default) override;
  virtual int32_t DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping) override;
  virtual int64_t DoReadInt64(const UnicodeString & Name, int64_t Default) override;
  virtual TDateTime DoReadDateTime(const UnicodeString & Name, TDateTime Default) override;
  virtual double DoReadFloat(const UnicodeString & Name, double Default) override;
  virtual UnicodeString DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) override;
  virtual int32_t DoReadBinaryData(const UnicodeString & Name, void * Buffer, int32_t Size) override;

private:
  UnicodeString GetFullCurrentSubKey() const { return /* GetStorage() + */ GetCurrentSubKey(); }
  size_t OpenSubKeyInternal(size_t Root, const UnicodeString & SubKey, bool CanCreate);

private:
  size_t FRoot{0};
  mutable PluginSettings FPluginSettings;
  nb::vector_t<size_t> FSubKeyIds;

  virtual void Init() override;
};

