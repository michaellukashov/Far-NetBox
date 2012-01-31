//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

#include "Classes.h"
//---------------------------------------------------------------------------
class nb::TRegistry;
//---------------------------------------------------------------------------
enum TStorage { stRegistry, stXmlFile };
enum TStorageAccessMode { smRead, smReadWrite };
//---------------------------------------------------------------------------
class THierarchicalStorage
{
public:
    explicit THierarchicalStorage(const std::wstring AStorage);
    virtual ~THierarchicalStorage();

    virtual void Init() {}
    bool OpenRootKey(bool CanCreate);
    virtual bool OpenSubKey(const std::wstring SubKey, bool CanCreate, bool Path = false);
    virtual void CloseSubKey();
    virtual bool DeleteSubKey(const std::wstring SubKey) = 0;
    virtual void GetSubKeyNames(nb::TStrings *Strings) = 0;
    virtual void GetValueNames(nb::TStrings *Strings) = 0;
    bool HasSubKeys();
    bool HasSubKey(const std::wstring SubKey);
    virtual bool KeyExists(const std::wstring SubKey) = 0;
    virtual bool ValueExists(const std::wstring Value) = 0;
    virtual void RecursiveDeleteSubKey(const std::wstring Key);
    virtual void ClearSubKeys();
    virtual void ReadValues(nb::TStrings *Strings, bool MaintainKeys = false);
    virtual void WriteValues(nb::TStrings *Strings, bool MaintainKeys = false);
    virtual void ClearValues();
    virtual bool DeleteValue(const std::wstring Name) = 0;

    virtual int BinaryDataSize(const std::wstring Name) = 0;

    virtual bool Readbool(const std::wstring Name, bool Default) = 0;
    virtual int Readint(const std::wstring Name, int Default) = 0;
    virtual __int64 ReadInt64(const std::wstring Name, __int64 Default) = 0;
    virtual nb::TDateTime ReadDateTime(const std::wstring Name, nb::TDateTime Default) = 0;
    virtual double ReadFloat(const std::wstring Name, double Default) = 0;
    virtual std::wstring ReadStringRaw(const std::wstring Name, const std::wstring Default) = 0;
    virtual size_t ReadBinaryData(const std::wstring Name, void *Buffer, size_t Size) = 0;

    virtual std::wstring ReadString(const std::wstring Name, const std::wstring Default);
    std::wstring ReadBinaryData(const std::wstring Name);

    virtual void Writebool(const std::wstring Name, bool Value) = 0;
    virtual void WriteStringRaw(const std::wstring Name, const std::wstring Value) = 0;
    virtual void Writeint(const std::wstring Name, int Value) = 0;
    virtual void WriteInt64(const std::wstring Name, __int64 Value) = 0;
    virtual void WriteDateTime(const std::wstring Name, nb::TDateTime Value) = 0;
    virtual void WriteFloat(const std::wstring Name, double Value) = 0;
    virtual void WriteBinaryData(const std::wstring Name, const void *Buffer, size_t Size) = 0;

    virtual void WriteString(const std::wstring Name, const std::wstring Value);
    void WriteBinaryData(const std::wstring Name, const std::wstring Value);

    // __property std::wstring Storage  = { read=FStorage };
    std::wstring GetStorage() { return FStorage; }
    // __property std::wstring CurrentSubKey  = { read=GetCurrentSubKey };
    std::wstring GetCurrentSubKey();
    // __property TStorageAccessMode AccessMode  = { read=FAccessMode, write=SetAccessMode };
    TStorageAccessMode GetAccessMode() { return FAccessMode; }
    virtual void SetAccessMode(TStorageAccessMode value);
    // __property bool Explicit = { read = FExplicit, write = FExplicit };
    bool GetExplicit() { return FExplicit; }
    void SetExplicit(bool value) { FExplicit = value; }
    // __property bool MungeStringValues = { read = FMungeStringValues, write = FMungeStringValues };
    bool GetMungeStringValues() { return FMungeStringValues; }
    void SetMungeStringValues(bool value) { FMungeStringValues = value; }
    // __property std::wstring Source = { read = GetSource };
    virtual std::wstring GetSource() = 0;

protected:
    std::wstring FStorage;
    nb::TStrings *FKeyHistory;
    TStorageAccessMode FAccessMode;
    bool FExplicit;
    bool FMungeStringValues;

    std::wstring GetCurrentSubKeyMunged();
    static std::wstring IncludeTrailingBackslash(const std::wstring S);
    static std::wstring ExcludeTrailingBackslash(const std::wstring S);
    std::wstring MungeSubKey(const std::wstring Key, bool Path);
};
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
    explicit TRegistryStorage(const std::wstring AStorage, HKEY ARootKey);
    explicit TRegistryStorage(const std::wstring AStorage);
    virtual ~TRegistryStorage();

    bool Copy(TRegistryStorage *Storage);

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
    virtual size_t ReadBinaryData(const std::wstring Name, void *Buffer, size_t Size);

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

    // __property int Failed  = { read=GetFailed, write=FFailed };
    int GetFailed();
    void SetFailed(int value) { FFailed = value; }

private:
    nb::TRegistry *FRegistry;
    int FFailed;

    void Init();
};
//---------------------------------------------------------------------------
std::wstring PuttyMungeStr(const std::wstring Str);
std::wstring PuttyUnMungeStr(const std::wstring Str);
//---------------------------------------------------------------------------
#endif
