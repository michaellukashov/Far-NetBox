#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(UnicodeString AStorage, UnicodeString StoredSessionsSubKey);
  virtual void Init();
  virtual ~TXmlStorage();

  bool Copy(TXmlStorage *Storage);

  virtual void CloseSubKey();
  virtual bool DeleteSubKey(UnicodeString SubKey);
  virtual void GetSubKeyNames(TStrings *Strings);
  virtual bool ValueExists(UnicodeString Value) const;
  virtual bool DeleteValue(UnicodeString Name);
  virtual size_t BinaryDataSize(UnicodeString Name) const;
  virtual UnicodeString GetSource() const;
  virtual UnicodeString GetSource();

  virtual bool ReadBool(UnicodeString Name, bool Default) const;
  virtual intptr_t ReadInteger(UnicodeString Name, intptr_t Default) const;
  virtual int64_t ReadInt64(UnicodeString Name, int64_t Default) const;
  virtual TDateTime ReadDateTime(UnicodeString Name, const TDateTime &Default) const;
  virtual double ReadFloat(UnicodeString Name, double Default) const;
  virtual UnicodeString ReadStringRaw(UnicodeString Name, UnicodeString Default) const;
  virtual size_t ReadBinaryData(UnicodeString Name, void *Buffer, size_t Size) const;

  virtual void WriteBool(UnicodeString Name, bool Value);
  virtual void WriteInteger(UnicodeString Name, intptr_t Value);
  virtual void WriteInt64(UnicodeString Name, int64_t Value);
  virtual void WriteDateTime(UnicodeString Name, const TDateTime &Value);
  virtual void WriteFloat(UnicodeString Name, double Value);
  virtual void WriteStringRaw(UnicodeString Name, UnicodeString Value);
  virtual void WriteBinaryData(UnicodeString Name, const void *Buffer, size_t Size);

  virtual void GetValueNames(TStrings *Strings) const;

  virtual void SetAccessMode(TStorageAccessMode Value);
  virtual bool DoKeyExists(UnicodeString SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(UnicodeString MungedSubKey, bool CanCreate);

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
