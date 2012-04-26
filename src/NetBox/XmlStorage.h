#pragma once

#include "Classes.h"
#include "HierarchicalStorage.h"
#include "tinyxml.h"

//---------------------------------------------------------------------------
class TXmlStorage : public THierarchicalStorage
{
public:
    explicit TXmlStorage(const UnicodeString AStorage, const UnicodeString StoredSessionsSubKey);
    virtual ~TXmlStorage();

    virtual void Init();
    bool Copy(TXmlStorage *Storage);

    virtual bool OpenSubKey(const UnicodeString SubKey, bool CanCreate, bool Path = false);
    virtual void CloseSubKey();
    virtual bool DeleteSubKey(const UnicodeString SubKey);
    virtual bool DeleteValue(const UnicodeString Name);
    virtual void GetSubKeyNames(System::TStrings *Strings);
    virtual bool KeyExists(const UnicodeString SubKey);
    virtual bool ValueExists(const UnicodeString Value);

    virtual int BinaryDataSize(const UnicodeString Name);

    virtual bool Readbool(const UnicodeString Name, bool Default);
    virtual int Readint(const UnicodeString Name, int Default);
    virtual __int64 ReadInt64(const UnicodeString Name, __int64 Default);
    virtual System::TDateTime ReadDateTime(const UnicodeString Name, System::TDateTime Default);
    virtual double ReadFloat(const UnicodeString Name, double Default);
    virtual UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default);
    virtual size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size);

    virtual void Writebool(const UnicodeString Name, bool Value);
    virtual void Writeint(const UnicodeString Name, int Value);
    virtual void WriteInt64(const UnicodeString Name, __int64 Value);
    virtual void WriteDateTime(const UnicodeString Name, System::TDateTime Value);
    virtual void WriteFloat(const UnicodeString Name, double Value);
    virtual void WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
    virtual void WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size);

    virtual void GetValueNames(System::TStrings *Strings);

    virtual void SetAccessMode(TStorageAccessMode value);

protected:
    virtual UnicodeString GetSource();

    int GetFailed();
    void SetFailed(int value) { FFailed = value; }

private:
    UnicodeString GetSubKeyText(const UnicodeString Name);
    TiXmlElement *FindElement(const UnicodeString Value);
    std::string ToStdString(const UnicodeString String) { return System::W2MB(String.c_str()); }
    UnicodeString ToStdWString(const std::string &String) { return System::MB2W(String.c_str()); }
    void RemoveIfExists(const UnicodeString Name);
    void AddNewElement(const UnicodeString Name, const UnicodeString Value);
    TiXmlElement *FindChildElement(const std::string &subKey);
    UnicodeString GetValue(TiXmlElement *Element);

    bool LoadXml();
    bool WriteXml();

private:
    TiXmlDocument *FXmlDoc;
    std::vector<TiXmlElement *> FSubElements;
    TiXmlElement *FCurrentElement;
    UnicodeString FStoredSessionsSubKey;
    int FFailed;
    bool FStoredSessionsOpened;
};
