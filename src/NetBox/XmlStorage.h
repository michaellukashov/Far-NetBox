#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(const UnicodeString AStorage, const UnicodeString StoredSessionsSubKey);
  virtual void Init() override;
  virtual ~TXmlStorage();

  bool Copy(TXmlStorage *Storage);

  virtual void CloseSubKey() override;
  virtual bool DeleteSubKey(const UnicodeString SubKey) override;
  virtual void GetSubKeyNames(TStrings *Strings) override;
  virtual bool ValueExists(const UnicodeString Value) const override;
  virtual bool DeleteValue(const UnicodeString Name) override;
  virtual size_t BinaryDataSize(const UnicodeString Name) const override;
  virtual UnicodeString GetSource() const override;
  virtual UnicodeString GetSource() override;

  virtual bool ReadBool(const UnicodeString Name, bool Default) const override;
  virtual intptr_t ReadInteger(const UnicodeString Name, intptr_t Default) const override;
  virtual int64_t ReadInt64(const UnicodeString Name, int64_t Default) const override;
  virtual TDateTime ReadDateTime(const UnicodeString Name, const TDateTime &Default) const override;
  virtual double ReadFloat(const UnicodeString Name, double Default) const override;
  virtual UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default) const override;
  virtual size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size) const override;

  virtual void WriteBool(const UnicodeString Name, bool Value) override;
  virtual void WriteInteger(const UnicodeString Name, intptr_t Value) override;
  virtual void WriteInt64(const UnicodeString Name, int64_t Value) override;
  virtual void WriteDateTime(const UnicodeString Name, const TDateTime &Value) override;
  virtual void WriteFloat(const UnicodeString Name, double Value) override;
  virtual void WriteStringRaw(const UnicodeString Name, const UnicodeString Value) override;
  virtual void WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size) override;

  virtual void GetValueNames(TStrings *Strings) const override;

  virtual void SetAccessMode(TStorageAccessMode Value) override;
  virtual bool DoKeyExists(const UnicodeString SubKey, bool ForceAnsi) override;
  virtual bool DoOpenSubKey(const UnicodeString MungedSubKey, bool CanCreate) override;

protected:
  intptr_t GetFailed();
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
  tinyxml2::XMLDocument *FXmlDoc;
  rde::vector<tinyxml2::XMLElement *> FSubElements;
  tinyxml2::XMLElement *FCurrentElement;
  UnicodeString FStoredSessionsSubKey;
  intptr_t FFailed;
  bool FStoredSessionsOpened;
};
