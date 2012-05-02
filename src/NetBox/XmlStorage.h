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

    virtual void __fastcall Init();
    bool Copy(TXmlStorage *Storage);

    virtual bool __fastcall OpenSubKey(const UnicodeString SubKey, bool CanCreate, bool Path = false);
    virtual void __fastcall CloseSubKey();
    virtual bool __fastcall DeleteSubKey(const UnicodeString SubKey);
    virtual bool __fastcall DeleteValue(const UnicodeString Name);
    virtual void __fastcall GetSubKeyNames(TStrings *Strings);
    virtual bool __fastcall KeyExists(const UnicodeString SubKey);
    virtual bool __fastcall ValueExists(const UnicodeString Value);

    virtual int __fastcall BinaryDataSize(const UnicodeString Name);

    virtual bool __fastcall Readbool(const UnicodeString Name, bool Default);
    virtual int __fastcall Readint(const UnicodeString Name, int Default);
    virtual __int64 __fastcall ReadInt64(const UnicodeString Name, __int64 Default);
    virtual TDateTime __fastcall ReadDateTime(const UnicodeString Name, TDateTime Default);
    virtual double __fastcall ReadFloat(const UnicodeString Name, double Default);
    virtual UnicodeString __fastcall ReadStringRaw(const UnicodeString Name, const UnicodeString Default);
    virtual size_t __fastcall ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size);

    virtual void __fastcall Writebool(const UnicodeString Name, bool Value);
    virtual void __fastcall Writeint(const UnicodeString Name, int Value);
    virtual void __fastcall WriteInt64(const UnicodeString Name, __int64 Value);
    virtual void __fastcall WriteDateTime(const UnicodeString Name, TDateTime Value);
    virtual void __fastcall WriteFloat(const UnicodeString Name, double Value);
    virtual void __fastcall WriteStringRaw(const UnicodeString Name, const UnicodeString Value);
    virtual void __fastcall WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size);

    virtual void __fastcall GetValueNames(TStrings *Strings);

    virtual void __fastcall SetAccessMode(TStorageAccessMode value);

protected:
    virtual UnicodeString __fastcall GetSource();

    int __fastcall GetFailed();
    void __fastcall SetFailed(int value) { FFailed = value; }

private:
    UnicodeString GetSubKeyText(const UnicodeString Name);
    TiXmlElement *FindElement(const UnicodeString Value);
    std::string ToStdString(const UnicodeString String) { return W2MB(String.c_str()); }
    UnicodeString ToStdWString(const std::string &String) { return MB2W(String.c_str()); }
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
