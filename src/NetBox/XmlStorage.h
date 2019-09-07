#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(const UnicodeString AStorage, const UnicodeString StoredSessionsSubKey) noexcept;
  void Init() override;
  virtual ~TXmlStorage() noexcept;

  bool Copy(TXmlStorage *Storage);

  void CloseSubKey() override;
  bool DeleteSubKey(const UnicodeString SubKey) override;
  void GetSubKeyNames(TStrings *Strings) override;
  bool ValueExists(const UnicodeString Value) const override;
  bool DeleteValue(const UnicodeString Name) override;
  size_t BinaryDataSize(const UnicodeString Name) const override;

  bool ReadBool(const UnicodeString Name, bool Default) const override;
  intptr_t ReadIntPtr(const UnicodeString Name, intptr_t Default) const override;
  int ReadInteger(const UnicodeString Name, int Default) const override;
  int64_t ReadInt64(const UnicodeString Name, int64_t Default) const override;
  TDateTime ReadDateTime(const UnicodeString Name, const TDateTime &Default) const override;
  double ReadFloat(const UnicodeString Name, double Default) const override;
  UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default) const override;
  size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size) const override;

  void WriteBool(const UnicodeString Name, bool Value) override;
  void WriteIntPtr(const UnicodeString Name, intptr_t Value) override;
  void WriteInteger(const UnicodeString Name, int Value) override;
  void WriteInt64(const UnicodeString Name, int64_t Value) override;
  void WriteDateTime(const UnicodeString Name, const TDateTime &Value) override;
  void WriteFloat(const UnicodeString Name, double Value) override;
  void WriteStringRaw(const UnicodeString Name, const UnicodeString Value) override;
  void WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size) override;

  void GetValueNames(TStrings *Strings) const override;

protected:
  void SetAccessModeProtected(TStorageAccessMode Value) override;
  bool DoKeyExists(const UnicodeString SubKey, bool ForceAnsi) override;
  bool DoOpenSubKey(const UnicodeString MungedSubKey, bool CanCreate) override;

  UnicodeString GetSourceProtected() const override;
  UnicodeString GetSourceProtected() override;
protected:
  intptr_t GetFailed() const;
  void SetFailed(intptr_t Value) { FFailed = Value; }

private:
  UnicodeString GetSubKeyText(const UnicodeString Name) const;
  tinyxml2::XMLElement *FindElement(const UnicodeString Name) const;
  //std::string ToStdString(const UnicodeString String) const { return std::string(::W2MB(String.c_str()).c_str()); }
  UnicodeString ToUnicodeString(const char *String) const { return ::MB2W(String ? String : ""); }
  void RemoveIfExists(const UnicodeString Name);
  void AddNewElement(const UnicodeString Name, const UnicodeString Value);
  tinyxml2::XMLElement *FindChildElement(AnsiString SubKey) const;
  UnicodeString GetValue(tinyxml2::XMLElement *Element) const;

  bool ReadXml();
  bool WriteXml();

private:
  tinyxml2::XMLDocument *FXmlDoc{nullptr};
  nb::vector_t<tinyxml2::XMLElement *> FSubElements;
  tinyxml2::XMLElement *FCurrentElement{nullptr};
  UnicodeString FStoredSessionsSubKey;
  mutable intptr_t FFailed{0};
  bool FStoredSessionsOpened{false};
};
