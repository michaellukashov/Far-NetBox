#pragma once

#include <vcl.h>
#include "HierarchicalStorage.h"
#include "tinyxml2.h"

//---------------------------------------------------------------------------
class TXmlStorage : public THierarchicalStorage
{
public:
  explicit TXmlStorage(const UnicodeString & AStorage, const UnicodeString & StoredSessionsSubKey);
  virtual void Init();
  virtual ~TXmlStorage();

  bool Copy(TXmlStorage * Storage);

  virtual void CloseSubKey();
  virtual bool DeleteSubKey(const UnicodeString & SubKey);
  virtual void GetSubKeyNames(TStrings * Strings);
  virtual bool ValueExists(const UnicodeString & Value);
  virtual bool DeleteValue(const UnicodeString & Name);

  virtual size_t BinaryDataSize(const UnicodeString & Name);

  virtual bool ReadBool(const UnicodeString & Name, bool Default);
  virtual intptr_t ReadInteger(const UnicodeString & Name, intptr_t Default);
  virtual __int64 ReadInt64(const UnicodeString & Name, __int64 Default);
  virtual TDateTime ReadDateTime(const UnicodeString & Name, TDateTime Default);
  virtual double ReadFloat(const UnicodeString & Name, double Default);
  virtual UnicodeString ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  virtual size_t ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

  virtual void WriteBool(const UnicodeString & Name, bool Value);
  virtual void WriteInteger(const UnicodeString & Name, intptr_t Value);
  virtual void WriteInt64(const UnicodeString & Name, __int64 Value);
  virtual void WriteDateTime(const UnicodeString & Name, TDateTime Value);
  virtual void WriteFloat(const UnicodeString & Name, double Value);
  virtual void WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void WriteBinaryData(const UnicodeString & Name, const void * Buffer, size_t Size);

  virtual void GetValueNames(TStrings * Strings);

  virtual void SetAccessMode(TStorageAccessMode Value);
  virtual bool DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool DoOpenSubKey(const UnicodeString & MungedSubKey, bool CanCreate);

protected:
  virtual UnicodeString GetSource();

  intptr_t GetFailed();
  void SetFailed(intptr_t Value) { FFailed = Value; }

private:
  UnicodeString GetSubKeyText(const UnicodeString & Name);
  tinyxml2::XMLElement * FindElement(const UnicodeString & Value);
  std::string ToStdString(const UnicodeString & String) { return std::string(W2MB(String.c_str()).c_str()); }
  UnicodeString ToUnicodeString(const char * String) { return MB2W(String ? String : ""); }
  void RemoveIfExists(const UnicodeString & Name);
  void AddNewElement(const UnicodeString & Name, const UnicodeString & Value);
  tinyxml2::XMLElement * FindChildElement(const std::string & subKey);
  UnicodeString GetValue(tinyxml2::XMLElement * Element);

  bool ReadXml();
  bool WriteXml();

private:
  tinyxml2::XMLDocument * FXmlDoc;
  rde::vector<tinyxml2::XMLElement *> FSubElements;
  tinyxml2::XMLElement * FCurrentElement;
  UnicodeString FStoredSessionsSubKey;
  intptr_t FFailed;
  bool FStoredSessionsOpened;
};
