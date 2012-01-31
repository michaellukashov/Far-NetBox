#pragma once

#include <vector>

#include "Classes.h"
#include "HierarchicalStorage.h"
#include "PluginSettings.hpp"

//---------------------------------------------------------------------------
class TFar3Storage : public THierarchicalStorage
{
public:
  explicit TFar3Storage(const std::wstring AStorage,
    const GUID &guid, FARAPISETTINGSCONTROL SettingsControl);
  virtual ~TFar3Storage();

  bool Copy(TFar3Storage * Storage);

  virtual bool OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path = false);
  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const std::wstring SubKey);
  virtual bool DeleteValue(const std::wstring Name);
  virtual void GetSubKeyNames(nb::TStrings *Strings);
  virtual bool KeyExists(const std::wstring SubKey);
  virtual bool ValueExists(const std::wstring Value);

  virtual int BinaryDataSize(const std::wstring Name);

  virtual bool Readbool(const std::wstring Name, bool Default);
  virtual int Readint(const std::wstring Name, int Default);
  virtual __int64 ReadInt64(const std::wstring Name, __int64 Default);
  virtual nb::TDateTime ReadDateTime(const std::wstring Name, nb::TDateTime Default);
  virtual double ReadFloat(const std::wstring Name, double Default);
  virtual std::wstring ReadStringRaw(const std::wstring Name, const std::wstring Default);
  virtual size_t ReadBinaryData(const std::wstring Name, void * Buffer, size_t Size);

  virtual void Writebool(const std::wstring Name, bool Value);
  virtual void Writeint(const std::wstring Name, int Value);
  virtual void WriteInt64(const std::wstring Name, __int64 Value);
  virtual void WriteDateTime(const std::wstring Name, nb::TDateTime Value);
  virtual void WriteFloat(const std::wstring Name, double Value);
  virtual void WriteStringRaw(const std::wstring Name, const std::wstring Value);
  virtual void WriteBinaryData(const std::wstring Name, const void * Buffer, size_t Size);

  virtual void GetValueNames(nb::TStrings* Strings);
  virtual void SetAccessMode(TStorageAccessMode value);

protected:
  virtual std::wstring GetSource();

  int GetFailed();
  void SetFailed(int value) { FFailed = value; }

private:
  std::wstring GetFullCurrentSubKey() { return /* GetStorage() + */ GetCurrentSubKey(); }
  int OpenSubKeyInternal(int Root, const std::wstring SubKey, bool CanCreate, bool Path);

private:
  int FRoot;
  int FFailed;
  PluginSettings FPluginSettings;
  std::vector<int> FSubKeyIds;

  void Init();
};
//---------------------------------------------------------------------------
