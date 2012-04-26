//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

#include "Classes.h"
//---------------------------------------------------------------------------
class System::TRegistry;
//---------------------------------------------------------------------------
enum TStorage { stRegistry, stXmlFile };
enum TStorageAccessMode { smRead, smReadWrite };
//---------------------------------------------------------------------------
class THierarchicalStorage
{
public:
    explicit THierarchicalStorage(const UnicodeString AStorage);
    virtual ~THierarchicalStorage();

    virtual void Init() {}
    bool OpenRootKey(bool CanCreate);
    virtual bool OpenSubKey(const UnicodeString SubKey, bool CanCreate, bool Path = false);
    virtual void CloseSubKey();
    virtual bool DeleteSubKey(const UnicodeString SubKey) = 0;
    virtual void GetSubKeyNames(System::TStrings *Strings) = 0;
    virtual void GetValueNames(System::TStrings *Strings) = 0;
    bool HasSubKeys();
    bool HasSubKey(const UnicodeString SubKey);
    virtual bool KeyExists(const UnicodeString SubKey) = 0;
    virtual bool ValueExists(const UnicodeString Value) = 0;
    virtual void RecursiveDeleteSubKey(const UnicodeString Key);
    virtual void ClearSubKeys();
    virtual void ReadValues(System::TStrings *Strings, bool MaintainKeys = false);
    virtual void WriteValues(System::TStrings *Strings, bool MaintainKeys = false);
    virtual void ClearValues();
    virtual bool DeleteValue(const UnicodeString Name) = 0;

    virtual int BinaryDataSize(const UnicodeString Name) = 0;

    virtual bool Readbool(const UnicodeString Name, bool Default) = 0;
    virtual int Readint(const UnicodeString Name, int Default) = 0;
    virtual __int64 ReadInt64(const UnicodeString Name, __int64 Default) = 0;
    virtual System::TDateTime ReadDateTime(const UnicodeString Name, System::TDateTime Default) = 0;
    virtual double ReadFloat(const UnicodeString Name, double Default) = 0;
    virtual UnicodeString ReadStringRaw(const UnicodeString Name, const UnicodeString Default) = 0;
    virtual size_t ReadBinaryData(const UnicodeString Name, void *Buffer, size_t Size) = 0;

    virtual UnicodeString ReadString(const UnicodeString Name, const UnicodeString Default);
    UnicodeString ReadBinaryData(const UnicodeString Name);

    virtual void Writebool(const UnicodeString Name, bool Value) = 0;
    virtual void WriteStringRaw(const UnicodeString Name, const UnicodeString Value) = 0;
    virtual void Writeint(const UnicodeString Name, int Value) = 0;
    virtual void WriteInt64(const UnicodeString Name, __int64 Value) = 0;
    virtual void WriteDateTime(const UnicodeString Name, System::TDateTime Value) = 0;
    virtual void WriteFloat(const UnicodeString Name, double Value) = 0;
    virtual void WriteBinaryData(const UnicodeString Name, const void *Buffer, size_t Size) = 0;

    virtual void WriteString(const UnicodeString Name, const UnicodeString Value);
    void WriteBinaryData(const UnicodeString Name, const UnicodeString Value);

    UnicodeString GetStorage() { return FStorage; }
    UnicodeString GetCurrentSubKey();
    TStorageAccessMode GetAccessMode() { return FAccessMode; }
    virtual void SetAccessMode(TStorageAccessMode value);
    bool GetExplicit() { return FExplicit; }
    void SetExplicit(bool value) { FExplicit = value; }
    bool GetMungeStringValues() { return FMungeStringValues; }
    void SetMungeStringValues(bool value) { FMungeStringValues = value; }
    virtual UnicodeString GetSource() = 0;

protected:
    UnicodeString FStorage;
    System::TStrings *FKeyHistory;
    TStorageAccessMode FAccessMode;
    bool FExplicit;
    bool FMungeStringValues;

    UnicodeString GetCurrentSubKeyMunged();
    static UnicodeString IncludeTrailingBackslash(const UnicodeString S);
    static UnicodeString ExcludeTrailingBackslash(const UnicodeString S);
    UnicodeString MungeSubKey(const UnicodeString Key, bool Path);
};
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
    explicit TRegistryStorage(const UnicodeString AStorage, HKEY ARootKey);
    explicit TRegistryStorage(const UnicodeString AStorage);
    virtual ~TRegistryStorage();

    bool Copy(TRegistryStorage *Storage);

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
    System::TRegistry *FRegistry;
    int FFailed;

    void Init();
};
//---------------------------------------------------------------------------
class TOptionsStorage : public TRegistryStorage
{
public:
  explicit TOptionsStorage(System::TStrings * Options);
};
//---------------------------------------------------------------------------
UnicodeString PuttyMungeStr(const UnicodeString Str);
UnicodeString PuttyUnMungeStr(const UnicodeString Str);
//---------------------------------------------------------------------------
#endif
