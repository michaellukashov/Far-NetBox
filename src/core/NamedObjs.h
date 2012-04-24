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
    explicit TNamedObject(const std::wstring aName);
    bool __fastcall GetHidden() { return FHidden; }
    std::wstring __fastcall GetName() { return FName; }
    void __fastcall SetName(const std::wstring value);
    int __fastcall CompareName(const std::wstring aName, bool CaseSensitive = false);
    void __fastcall MakeUniqueIn(TNamedObjectList *List);
private:
    std::wstring FName;
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

    static const std::wstring HiddenPrefix;
    static bool IsHidden(TNamedObject *Object);

    bool AutoSort;

    void AlphaSort();
    virtual TNamedObject *AtObject(size_t Index);
    TNamedObject *FindByName(const std::wstring Name, bool CaseSensitive = false);
    size_t GetCount();
    void SetCount(size_t value);
    size_t GetHiddenCount() { return FHiddenCount; }
    void SetHiddenCount(size_t value) { FHiddenCount = value; }
};
//---------------------------------------------------------------------------
int NamedObjectSortProc(void *Item1, void *Item2);
//---------------------------------------------------------------------------
#endif
