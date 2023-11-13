#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(const UnicodeString & AStorage, const UnicodeString & StoredSessionsSubKey) noexcept;
  void Init() override;
  virtual ~TXmlStorage() noexcept;

  bool Copy(TXmlStorage *Storage);

protected:
  void SetAccessModeProtected(TStorageAccessMode Value) override;
  bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi) override;
  bool DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate) override;
  void DoCloseSubKey() override;
  void DoDeleteSubKey(const UnicodeString & SubKey) override;
  void DoGetSubKeyNames(TStrings * Strings) override;
  bool DoValueExists(const UnicodeString & Value) override;
  bool DoDeleteValue(const UnicodeString & Name) override;
  size_t DoBinaryDataSize(const UnicodeString & Name) override;

  void DoWriteBool(const UnicodeString & Name, bool Value) override;
  void DoWriteInteger(const UnicodeString & Name, int32_t Value) override;
  void DoWriteInt64(const UnicodeString & Name, int64_t Value) override;
//  void DoWriteDateTime(const UnicodeString & Name, TDateTime Value) override;
//  void DoWriteFloat(const UnicodeString & Name, double Value) override;
  void DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) override;
  void DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size) override;

  bool DoReadBool(const UnicodeString & Name, bool Default) override;
  int32_t DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping) override;
  int64_t DoReadInt64(const UnicodeString & Name, int64_t Default) override;
  TDateTime DoReadDateTime(const UnicodeString & Name, TDateTime Default) override;
  double DoReadFloat(const UnicodeString & Name, double Default) override;
  UnicodeString DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) override;
  size_t DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) override;

  void DoGetValueNames(TStrings *Strings) override;

  UnicodeString GetSource() const override;
  UnicodeString GetSource();

protected:
  int32_t GetFailed();
  void SetFailed(int32_t Value) { FFailed = Value; }

private:
  UnicodeString GetSubKeyText(const UnicodeString & Name) const;
  tinyxml2::XMLElement * FindElement(const UnicodeString & Name) const;
  //std::string ToStdString(const UnicodeString & String) const { return std::string(::W2MB(String.c_str()).c_str()); }
  UnicodeString ToUnicodeString(const char * String) const { return ::MB2W(String ? String : ""); }
  void RemoveIfExists(const UnicodeString & Name);
  void AddNewElement(const UnicodeString & Name, const UnicodeString & Value);
  tinyxml2::XMLElement * FindChildElement(const AnsiString & SubKey) const;
  UnicodeString GetValue(tinyxml2::XMLElement *Element) const;

  bool ReadXml();
  bool WriteXml();

private:
  tinyxml2::XMLDocument *FXmlDoc{nullptr};
  nb::vector_t<tinyxml2::XMLElement *> FSubElements;
  tinyxml2::XMLElement *FCurrentElement{nullptr};
  UnicodeString FStoredSessionsSubKey;
  mutable int32_t FFailed{0};
  bool FStoredSessionsOpened{false};
};
