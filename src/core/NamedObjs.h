//---------------------------------------------------------------------------
#ifndef NamedObjsH
#define NamedObjsH

#include "Classes.h"

//---------------------------------------------------------------------------
class TNamedObjectList;
class TNamedObject : public System::TPersistent
{
public:
    TNamedObject() : System::TPersistent() {};
    explicit TNamedObject(const UnicodeString aName);
    bool __fastcall GetHidden() { return FHidden; }
    UnicodeString __fastcall GetName() { return FName; }
    void __fastcall SetName(const UnicodeString value);
    int __fastcall CompareName(const UnicodeString aName, bool CaseSensitive = false);
    void __fastcall MakeUniqueIn(TNamedObjectList *List);
private:
    UnicodeString FName;
    bool FHidden;
};

//---------------------------------------------------------------------------
class TNamedObjectList : public System::TObjectList
{
private:
    size_t FHiddenCount;
    virtual void Notify(void *Ptr, System::TListNotification Action);
protected:
    void Recount();
public:
    TNamedObjectList();

    static const UnicodeString HiddenPrefix;
    static bool IsHidden(TNamedObject *Object);

    bool AutoSort;

    void AlphaSort();
    virtual TNamedObject *AtObject(size_t Index);
    TNamedObject *FindByName(const UnicodeString Name, bool CaseSensitive = false);
    size_t GetCount();
    void SetCount(size_t value);
    size_t GetHiddenCount() { return FHiddenCount; }
    void SetHiddenCount(size_t value) { FHiddenCount = value; }
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void *Item1, void *Item2);
//---------------------------------------------------------------------------
#endif
