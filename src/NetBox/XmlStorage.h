#pragma once

#include "Classes.h"
#include "HierarchicalStorage.h"
#include "tinyxml.h"

//---------------------------------------------------------------------------
class TXmlStorage : public THierarchicalStorage
{
public:
    explicit TXmlStorage(const std::wstring AStorage, const std::wstring StoredSessionsSubKey);
    virtual ~TXmlStorage();

    virtual void Init();
    bool Copy(TXmlStorage *Storage);

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
    virtual int ReadBinaryData(const std::wstring Name, void *Buffer, size_t Size);

    virtual void Writebool(const std::wstring Name, bool Value);
    virtual void Writeint(const std::wstring Name, int Value);
    virtual void WriteInt64(const std::wstring Name, __int64 Value);
    virtual void WriteDateTime(const std::wstring Name, nb::TDateTime Value);
    virtual void WriteFloat(const std::wstring Name, double Value);
    virtual void WriteStringRaw(const std::wstring Name, const std::wstring Value);
    virtual void WriteBinaryData(const std::wstring Name, const void *Buffer, size_t Size);

    virtual void GetValueNames(nb::TStrings *Strings);

    virtual void SetAccessMode(TStorageAccessMode value);

protected:
    virtual std::wstring GetSource();

    int GetFailed();
    void SetFailed(int value) { FFailed = value; }

private:
    std::wstring GetSubKeyText(const std::wstring Name);
    TiXmlElement *FindElement(const std::wstring Value);
    std::string ToStdString(const std::wstring String) { return nb::W2MB(String.c_str()); }
    std::wstring ToStdWString(const std::string &String) { return nb::MB2W(String.c_str()); }
    void RemoveIfExists(const std::wstring Name);
    void AddNewElement(const std::wstring Name, const std::wstring Value);
    TiXmlElement *FindChildElement(const std::string &subKey);
    std::wstring GetValue(TiXmlElement *Element);

    bool LoadXml();
    bool WriteXml();

private:
    TiXmlDocument *FXmlDoc;
    std::vector<TiXmlElement *> FSubElements;
    TiXmlElement *FCurrentElement;
    std::wstring FStoredSessionsSubKey;
    int FFailed;
    bool FStoredSessionsOpened;
};
