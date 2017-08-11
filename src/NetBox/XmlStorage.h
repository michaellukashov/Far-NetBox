#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(UnicodeString AStorage, UnicodeString StoredSessionsSubKey);
  virtual void Init() override;
  virtual ~TXmlStorage();

  bool Copy(TXmlStorage *Storage);

  virtual void CloseSubKey() override;
  virtual bool DeleteSubKey(UnicodeString SubKey) override;
  virtual void GetSubKeyNames(TStrings *Strings) override;
  virtual bool ValueExists(UnicodeString Value) const override;
  virtual bool DeleteValue(UnicodeString Name) override;
  virtual size_t BinaryDataSize(UnicodeString Name) const override;
  virtual UnicodeString GetSource() const override;
  virtual UnicodeString GetSource() override;

  virtual bool ReadBool(UnicodeString Name, bool Default) const override;
  virtual intptr_t ReadInteger(UnicodeString Name, intptr_t Default) const override;
  virtual int64_t ReadInt64(UnicodeString Name, int64_t Default) const override;
  virtual TDateTime ReadDateTime(UnicodeString Name, const TDateTime &Default) const override;
  virtual double ReadFloat(UnicodeString Name, double Default) const override;
  virtual UnicodeString ReadStringRaw(UnicodeString Name, UnicodeString Default) const override;
  virtual size_t ReadBinaryData(UnicodeString Name, void *Buffer, size_t Size) const override;

  virtual void WriteBool(UnicodeString Name, bool Value) override;
  virtual void WriteInteger(UnicodeString Name, intptr_t Value) override;
  virtual void WriteInt64(UnicodeString Name, int64_t Value) override;
  virtual void WriteDateTime(UnicodeString Name, const TDateTime &Value) override;
  virtual void WriteFloat(UnicodeString Name, double Value) override;
  virtual void WriteStringRaw(UnicodeString Name, UnicodeString Value) override;
  virtual void WriteBinaryData(UnicodeString Name, const void *Buffer, size_t Size) override;

  virtual void GetValueNames(TStrings *Strings) const override;

  virtual void SetAccessMode(TStorageAccessMode Value) override;
  virtual bool DoKeyExists(UnicodeString SubKey, bool ForceAnsi) override;
  virtual bool DoOpenSubKey(UnicodeString MungedSubKey, bool CanCreate) override;

protected:
  intptr_t GetFailed();
  void SetFailed(intptr_t Value) { FFailed = Value; }

private:
  UnicodeString GetSubKeyText(UnicodeString Name) const;
  tinyxml2::XMLElement *FindElement(UnicodeString Name) const;
  //std::string ToStdString(UnicodeString String) const { return std::string(::W2MB(String.c_str()).c_str()); }
  UnicodeString ToUnicodeString(const char *String) const { return ::MB2W(String ? String : ""); }
  void RemoveIfExists(UnicodeString Name);
  void AddNewElement(UnicodeString Name, UnicodeString Value);
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
