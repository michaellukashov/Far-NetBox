//---------------------------------------------------------------------------
#include "stdafx.h"

#include "NamedObjs.h"
#include "Common.h"
//---------------------------------------------------------------------------
int NamedObjectSortProc(void *Item1, void *Item2)
{
    bool HasPrefix1 = (static_cast<TNamedObject *>(Item1))->GetHidden();
    bool HasPrefix2 = (static_cast<TNamedObject *>(Item2))->GetHidden();
    if (HasPrefix1 && !HasPrefix2) { return -1; }
    else if (!HasPrefix1 && HasPrefix2) { return 1; }
    else
    {
        return ::AnsiCompareStr((static_cast<TNamedObject *>(Item1))->GetName(), (static_cast<TNamedObject *>(Item2))->GetName());
    }
}
//--- TNamedObject ----------------------------------------------------------
TNamedObject::TNamedObject(const UnicodeString AName) :
    TPersistent()
{
    SetName(AName);
}
//---------------------------------------------------------------------------
void TNamedObject::SetName(const UnicodeString value)
{
    FHidden = (value.SubString(0, TNamedObjectList::HiddenPrefix.Length()) == TNamedObjectList::HiddenPrefix);
    FName = value;
}

int TNamedObject::CompareName(const UnicodeString aName,
                              bool CaseSensitive)
{
    // DEBUG_PRINTF(L"CaseSensitive = %d, Name = %s, aName = %s", CaseSensitive, Name.c_str(), aName.c_str());
    if (CaseSensitive)
    {
        return ::AnsiCompare(GetName(), aName);
    }
    else
    {
        return ::AnsiCompareIC(GetName(), aName);
    }
}
//---------------------------------------------------------------------------
void TNamedObject::MakeUniqueIn(TNamedObjectList *List)
{
    // This object can't be item of list, it would create infinite loop
    if (List && (List->IndexOf(this) < 0))
        while (List->FindByName(GetName()))
        {
            size_t N = 0, P = 0;
            // If name already contains number parenthesis remove it (and remember it)
            UnicodeString Name = GetName();
            if ((Name[Name.Length() - 1] == L')') && ((P = ::LastDelimiter(Name, L"(")) >= 0))
                try
                {
                    N = StrToInt(Name.SubString(P + 1, Name.Length() - P - 1));
                    Name.Delete(P, Name.Length() - P + 1);
                    SetName(::TrimRight(Name));
                }
                catch (const std::exception &E)
                {
                    N = 0;
                };
            Name += L" (" + IntToStr(static_cast<int>(N+1)) + L")";
        }
}
//--- TNamedObjectList ------------------------------------------------------
const UnicodeString TNamedObjectList::HiddenPrefix = L"_!_";
//---------------------------------------------------------------------------
TNamedObjectList::TNamedObjectList() :
    TObjectList(),
    FHiddenCount(0),
    AutoSort(true)
{
    AutoSort = true;
}
//---------------------------------------------------------------------------
TNamedObject *TNamedObjectList::AtObject(size_t Index)
{
    // DEBUG_PRINTF(L"Index = %d, Count = %d, GetHiddenCount = %d", Index, GetCount(), GetHiddenCount());
    return static_cast<TNamedObject *>(GetItem(Index + GetHiddenCount()));
}
//---------------------------------------------------------------------------
void TNamedObjectList::Recount()
{
    size_t i = 0;
    while ((i < TObjectList::GetCount()) && (static_cast<TNamedObject *>(GetItem(i)))->GetHidden()) { i++; }
    FHiddenCount = i;
}
//---------------------------------------------------------------------------
void TNamedObjectList::AlphaSort()
{
    Sort(NamedObjectSortProc);
}
//---------------------------------------------------------------------------
void TNamedObjectList::Notify(void *Ptr, TListNotification Action)
{
    TObjectList::Notify(Ptr, Action);
    if (AutoSort && (Action == lnAdded)) { AlphaSort(); }
    Recount();
}
//---------------------------------------------------------------------------
TNamedObject *TNamedObjectList::FindByName(const UnicodeString Name,
        bool CaseSensitive)
{
    for (size_t Index = 0; Index < TObjectList::GetCount(); Index++)
        if (!(static_cast<TNamedObject *>(GetItem(Index)))->CompareName(Name, CaseSensitive))
        {
            return static_cast<TNamedObject *>(GetItem(Index));
        }
    return NULL;
}
//---------------------------------------------------------------------------
void TNamedObjectList::SetCount(size_t value)
{
    TObjectList::SetCount(value/*+HiddenCount*/);
}
//---------------------------------------------------------------------------
size_t TNamedObjectList::GetCount()
{
    return TObjectList::GetCount() - GetHiddenCount();
}
