#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(const UnicodeString & AStorage, const UnicodeString & StoredSessionsSubKey) noexcept;
  virtual ~TXmlStorage() noexcept override;
  virtual void Init() override;

  bool Copy(TXmlStorage * Storage);

protected:
  virtual void SetAccessModeProtected(TStorageAccessMode Value) override;
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi) override;
  virtual bool DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate) override;
  virtual void DoCloseSubKey() override;
  virtual bool DoDeleteSubKey(const UnicodeString & SubKey) override;
  virtual void DoGetSubKeyNames(TStrings * Strings) override;
  virtual bool DoValueExists(const UnicodeString & Value) override;
  virtual bool DoDeleteValue(const UnicodeString & Name) override;
  virtual int32_t DoBinaryDataSize(const UnicodeString & Name) override;

  virtual void DoWriteBool(const UnicodeString & Name, bool Value) override;
  virtual void DoWriteInteger(const UnicodeString & Name, int32_t Value) override;
  virtual void DoWriteInt64(const UnicodeString & Name, int64_t Value) override;
  virtual void DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) override;
  virtual void DoWriteBinaryData(const UnicodeString & Name, const uint8_t * Buffer, int32_t Size) override;

  virtual bool DoReadBool(const UnicodeString & Name, bool Default) override;
  virtual int32_t DoReadInteger(const UnicodeString & Name, int32_t Default, const TIntMapping * Mapping) override;
  virtual int64_t DoReadInt64(const UnicodeString & Name, int64_t Default) override;
  virtual TDateTime DoReadDateTime(const UnicodeString & Name, const TDateTime & Default) override;
  virtual double DoReadFloat(const UnicodeString & Name, double Default) override;
  virtual UnicodeString DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) override;
  virtual int32_t DoReadBinaryData(const UnicodeString & Name, void * Buffer, int32_t Size) override;

  virtual void DoGetValueNames(TStrings * Strings) override;

  virtual UnicodeString GetSource() const override;
  UnicodeString GetSource();

protected:
  int32_t GetFailed() const;
  void SetFailed(int32_t Value) { FFailed = Value; }

private:
  UnicodeString GetSubKeyText(const UnicodeString & Name) const;
  tinyxml2::XMLElement * FindElement(const UnicodeString & Name) const;
  //std::string ToStdString(const UnicodeString & String) const { return std::string(::W2MB(String.c_str()).c_str()); }
  static UnicodeString ToUnicodeString(const char * String) { return ::MB2W(String ? String : ""); }
  void RemoveIfExists(const UnicodeString & Name);
  void AddNewElement(const UnicodeString & Name, const UnicodeString & Value);
  tinyxml2::XMLElement * FindChildElement(const AnsiString & SubKey) const;
  UnicodeString GetValue(const tinyxml2::XMLElement * Element) const;

  bool ReadXml();
  bool WriteXml() const;

private:
  std::unique_ptr<tinyxml2::XMLDocument> FXmlDoc;
  nb::vector_t<tinyxml2::XMLElement *> FSubElements;
  gsl::owner<tinyxml2::XMLElement *> FCurrentElement{nullptr};
  UnicodeString FStoredSessionsSubKey;
  mutable int32_t FFailed{0};
  bool FStoredSessionsOpened{false};
};
