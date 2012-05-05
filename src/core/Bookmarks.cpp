//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include <Common.h>
#include "NamedObjs.h"
#include "Bookmarks.h"
#include "Configuration.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
TBookmarks::TBookmarks() : TObject()
{
  FSharedKey = TNamedObjectList::HiddenPrefix + L"shared";
  FBookmarkLists = new TStringList();
  FBookmarkLists->SetSorted(true);
  FBookmarkLists->SetCaseSensitive(false);
  FBookmarkLists->SetDuplicates(dupError);
}
//---------------------------------------------------------------------------
TBookmarks::~TBookmarks()
{
  Clear();
  SAFE_DESTROY(FBookmarkLists);
}
//---------------------------------------------------------------------------
void TBookmarks::Clear()
{
  for (size_t i = 0; i < FBookmarkLists->GetCount(); i++)
  {
    delete FBookmarkLists->GetObjects(i);
  }
  FBookmarkLists->Clear();
}
//---------------------------------------------------------------------------
UnicodeString TBookmarks::Keys[] = { L"Local", L"Remote", L"ShortCuts", L"Options" };
//---------------------------------------------------------------------------
void TBookmarks::Load(THierarchicalStorage * Storage)
{
  for (int i = 0; i <= 3; i++)
  {
    if (Storage->OpenSubKey(Keys[i], false))
    {
      TStrings * BookmarkKeys = new TStringList();
      {
        BOOST_SCOPE_EXIT ( (&BookmarkKeys) )
        {
          delete BookmarkKeys;
        } BOOST_SCOPE_EXIT_END
        Storage->GetSubKeyNames(BookmarkKeys);
        for (size_t Index = 0; Index < BookmarkKeys->GetCount(); Index++)
        {
          UnicodeString Key = BookmarkKeys->GetStrings(Index);
          if (Storage->OpenSubKey(Key, false))
          {
            TBookmarkList * BookmarkList = GetBookmark(Key);
            if (!BookmarkList)
            {
              BookmarkList = new TBookmarkList();
              FBookmarkLists->AddObject(Key, BookmarkList);
            }
            if (i < 3)
            {
              LoadLevel(Storage, L"", i, BookmarkList);
            }
            else
            {
              BookmarkList->LoadOptions(Storage);
            }
            Storage->CloseSubKey();
          }
        }
      }
      Storage->CloseSubKey();
    }
  }

  ModifyAll(false);
}
//---------------------------------------------------------------------------
void TBookmarks::LoadLevel(THierarchicalStorage * Storage, const UnicodeString Key,
                           int Index, TBookmarkList * BookmarkList)
{
  TStrings * Names = new TStringList();
  {
    BOOST_SCOPE_EXIT ( (&Names) )
    {
      delete Names;
    } BOOST_SCOPE_EXIT_END
    Storage->GetValueNames(Names);
    UnicodeString Name;
    UnicodeString Directory;
    TShortCut ShortCut(0);
    for (size_t i = 0; i < Names->GetCount(); i++)
    {
      Name = Names->GetStrings(i);
      bool IsDirectory = (Index == 0) || (Index == 1);
      if (IsDirectory)
      {
        Directory = Storage->ReadString(Name, L"");
      }
      else
      {
        Directory = L""; // use only in cased of malformed config
        ShortCut = TShortCut(Storage->Readint(Name, 0));
      }
      TBookmark * Bookmark;
      if (IsNumber(Name))
      {
        assert(IsDirectory); // unless malformed
        Name = Directory;
      }
      if (!Name.IsEmpty())
      {
        Bookmark = BookmarkList->FindByName(Key, Name);
        bool New;
        New = (Bookmark == NULL);
        if (New)
        {
          Bookmark = new TBookmark();
          Bookmark->SetNode(Key);
          Bookmark->SetName(Name);
        }
        switch (Index)
        {
          case 0:
            Bookmark->SetLocal(Directory);
            break;

          case 1:
            Bookmark->SetRemote(Directory);
            break;

          case 2:
            Bookmark->SetShortCut(ShortCut);
            break;
        }
        if (New)
        {
          BookmarkList->Add(Bookmark);
        }
      }
    }

    Storage->GetSubKeyNames(Names);
    for (size_t i = 0; i < Names->GetCount(); i++)
    {
      Name = Names->GetStrings(i);
      if (Storage->OpenSubKey(Name, false))
      {
        LoadLevel(Storage, Key + (Key.IsEmpty() ? L"" : L"/") + Name, Index, BookmarkList);
        Storage->CloseSubKey();
      }
    }
  }
}
//---------------------------------------------------------------------------
void TBookmarks::Save(THierarchicalStorage * Storage, bool All)
{
  for (int i = 0; i <= 3; i++)
  {
    if (Storage->OpenSubKey(Keys[i], true))
    {
      for (size_t Index = 0; Index < FBookmarkLists->GetCount(); Index++)
      {
        TBookmarkList * BookmarkList = reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObjects(Index));
        if (All || BookmarkList->GetModified())
        {
          UnicodeString Key;
          Key = FBookmarkLists->GetStrings(Index);
          Storage->RecursiveDeleteSubKey(Key);
          if (Storage->OpenSubKey(Key, true))
          {
            if (i < 3)
            {
              for (size_t IndexB = 0; IndexB < BookmarkList->GetCount(); IndexB++)
              {
                TBookmark * Bookmark = BookmarkList->GetBookmark(IndexB);
                // avoid creating empty subfolder if there's no shortcut
                if ((i == 0) || (i == 1) ||
                    ((i == 2) && (Bookmark->GetShortCut() != 0)))
                {
                  bool HasNode = !Bookmark->GetNode().IsEmpty();
                  if (!HasNode || Storage->OpenSubKey(Bookmark->GetNode(), true))
                  {
                    switch (i)
                    {
                      case 0:
                        Storage->WriteString(Bookmark->GetName(), Bookmark->GetLocal());
                        break;

                      case 1:
                        Storage->WriteString(Bookmark->GetName(), Bookmark->GetRemote());
                        break;

                      case 2:
                        assert(Bookmark->GetShortCut() != 0);
                        Storage->Writeint(Bookmark->GetName(), Bookmark->GetShortCut());
                        break;
                    }

                    if (HasNode)
                    {
                      Storage->CloseSubKey();
                    }
                  }
                }
              }
            }
            else
            {
              BookmarkList->SaveOptions(Storage);
            }
            Storage->CloseSubKey();
          }
        }
      }
      Storage->CloseSubKey();
    }
  }

  if (!All)
  {
    ModifyAll(false);
  }
}
//---------------------------------------------------------------------------
void TBookmarks::ModifyAll(bool Modify)
{
  TBookmarkList * BookmarkList;
  for (size_t i = 0; i < FBookmarkLists->GetCount(); i++)
  {
    BookmarkList = reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObjects(i));
    assert(BookmarkList);
    BookmarkList->SetModified(Modify);
  }
}
//---------------------------------------------------------------------------
TBookmarkList * TBookmarks::GetBookmark(const UnicodeString Index)
{
  size_t I = FBookmarkLists->IndexOf(Index.c_str());
  if (I >= 0)
  {
    return reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObjects(I));
  }
  else
  {
    return NULL;
  }
}
//---------------------------------------------------------------------------
void TBookmarks::SetBookmark(const UnicodeString Index, TBookmarkList * value)
{
  size_t I = FBookmarkLists->IndexOf(Index.c_str());
  if (I >= 0)
  {
    TBookmarkList * BookmarkList;
    BookmarkList = reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObjects(I));
    BookmarkList->Assign(value);
  }
  else
  {
    TBookmarkList * BookmarkList = new TBookmarkList();
    BookmarkList->Assign(value);
    FBookmarkLists->AddObject(Index, BookmarkList);
  }
}
//---------------------------------------------------------------------------
TBookmarkList * TBookmarks::GetSharedBookmarks()
{
  return GetBookmark(FSharedKey);
}
//---------------------------------------------------------------------------
void TBookmarks::SetSharedBookmarks(TBookmarkList * value)
{
  SetBookmark(FSharedKey, value);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TBookmarkList::TBookmarkList(): TPersistent()
{
  FModified = false;
  FBookmarks = new TStringList();
  FBookmarks->SetCaseSensitive(false);
  FOpenedNodes = new TStringList();
  FOpenedNodes->SetCaseSensitive(false);
  FOpenedNodes->SetSorted(true);
}
//---------------------------------------------------------------------------
TBookmarkList::~TBookmarkList()
{
  Clear();
  SAFE_DESTROY(FBookmarks);
  SAFE_DESTROY(FOpenedNodes);
}
//---------------------------------------------------------------------------
void TBookmarkList::Clear()
{
  for (size_t i = 0; i < FBookmarks->GetCount(); i++)
  {
    delete FBookmarks->GetObjects(i);
  }
  FBookmarks->Clear();
  FOpenedNodes->Clear();
}
//---------------------------------------------------------------------------
void TBookmarkList::Assign(TPersistent * Source)
{
  TBookmarkList * SourceList;
  SourceList = reinterpret_cast<TBookmarkList *>(Source);
  if (SourceList)
  {
    Clear();
    for (size_t i = 0; i < SourceList->FBookmarks->GetCount(); i++)
    {
      TBookmark * Bookmark = new TBookmark();
      Bookmark->Assign(reinterpret_cast<TBookmark *>(SourceList->FBookmarks->GetObjects(i)));
      Add(Bookmark);
    }
    FOpenedNodes->Assign(SourceList->FOpenedNodes);
    SetModified(SourceList->GetModified());
  }
  else
  {
    TPersistent::Assign(Source);
  }
}
//---------------------------------------------------------------------------
void TBookmarkList::LoadOptions(THierarchicalStorage * Storage)
{
  FOpenedNodes->SetCommaText(Storage->ReadString(L"OpenedNodes", L""));
}
//---------------------------------------------------------------------------
void TBookmarkList::SaveOptions(THierarchicalStorage * Storage)
{
  Storage->WriteString(L"OpenedNodes", FOpenedNodes->GetCommaText());
}
//---------------------------------------------------------------------------
void TBookmarkList::Add(TBookmark * Bookmark)
{
  Insert(GetCount(), Bookmark);
}
//---------------------------------------------------------------------------
void TBookmarkList::InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark)
{
  assert(BeforeBookmark);
  size_t I = FBookmarks->IndexOf(BeforeBookmark->GetKey().c_str());
  assert(I >= 0);
  Insert(I, Bookmark);
}
//---------------------------------------------------------------------------
void TBookmarkList::MoveTo(TBookmark * ToBookmark,
                           TBookmark * Bookmark, bool Before)
{
  assert(ToBookmark != NULL);
  size_t NewIndex = FBookmarks->IndexOf(ToBookmark->GetKey().c_str());
  assert(Bookmark != NULL);
  size_t OldIndex = FBookmarks->IndexOf(Bookmark->GetKey().c_str());
  if (Before && (NewIndex > OldIndex))
  {
    // otherwise item is moved after the item in the target index
    NewIndex--;
  }
  else if (!Before && (NewIndex < OldIndex))
  {
    NewIndex++;
  }
  FModified = true;
  FBookmarks->Move(OldIndex, NewIndex);
}
//---------------------------------------------------------------------------
void TBookmarkList::Insert(size_t Index, TBookmark * Bookmark)
{
  assert(Bookmark);
  assert(!Bookmark->FOwner);
  assert(!Bookmark->GetName().IsEmpty());

  FModified = true;
  Bookmark->FOwner = this;
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) >= 0)
  {
    throw ExtException(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->InsertObject(Index, Bookmark->GetKey(), Bookmark);
}
//---------------------------------------------------------------------------
void TBookmarkList::Delete(TBookmark * Bookmark)
{
  assert(Bookmark);
  assert(Bookmark->FOwner == this);
  size_t I = IndexOf(Bookmark);
  assert(I >= 0);
  FModified = true;
  Bookmark->FOwner = NULL;
  FBookmarks->Delete(I);
  delete Bookmark;
}
//---------------------------------------------------------------------------
size_t TBookmarkList::IndexOf(TBookmark * Bookmark)
{
  return FBookmarks->IndexOf(Bookmark->GetKey().c_str());
}
//---------------------------------------------------------------------------
void TBookmarkList::KeyChanged(size_t Index)
{
  assert(Index < GetCount());
  TBookmark * Bookmark = reinterpret_cast<TBookmark *>(FBookmarks->GetObjects(Index));
  assert(FBookmarks->GetStrings(Index) != Bookmark->GetKey());
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) >= 0)
  {
    throw ExtException(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->PutString(Index, Bookmark->GetKey());
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::FindByName(const UnicodeString Node, const UnicodeString Name)
{
  size_t I = FBookmarks->IndexOf(TBookmark::BookmarkKey(Node, Name).c_str());
  TBookmark * Bookmark = ((I >= 0) ? reinterpret_cast<TBookmark *>(FBookmarks->GetObjects(I)) : NULL);
  assert(!Bookmark || (Bookmark->GetNode() == Node && Bookmark->GetName() == Name));
  return Bookmark;
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::FindByShortCut(TShortCut ShortCut)
{
  for (size_t Index = 0; Index < FBookmarks->GetCount(); Index++)
  {
    if (GetBookmark(Index)->GetShortCut() == ShortCut)
    {
      return GetBookmark(Index);
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
size_t TBookmarkList::GetCount()
{
  return FBookmarks->GetCount();
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::GetBookmark(size_t Index)
{
  TBookmark * Bookmark = reinterpret_cast<TBookmark *>(FBookmarks->GetObjects(Index));
  assert(Bookmark);
  return Bookmark;
}
//---------------------------------------------------------------------------
bool TBookmarkList::GetNodeOpened(const UnicodeString Index)
{
  return (FOpenedNodes->IndexOf(Index.c_str()) >= 0);
}
//---------------------------------------------------------------------------
void TBookmarkList::SetNodeOpened(const UnicodeString Index, bool value)
{
  size_t I = FOpenedNodes->IndexOf(Index.c_str());
  if ((I >= 0) != value)
  {
    if (value)
    {
      FOpenedNodes->Add(Index);
    }
    else
    {
      FOpenedNodes->Delete(I);
    }
    FModified = true;
  }
}
//---------------------------------------------------------------------------
void TBookmarkList::ShortCuts(TShortCuts & ShortCuts)
{
  for (size_t Index = 0; Index < GetCount(); Index++)
  {
    TBookmark * Bookmark = GetBookmark(Index);
    if (Bookmark->GetShortCut() != 0)
    {
      ShortCuts.Add(Bookmark->GetShortCut());
    }
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TBookmark::TBookmark()
{
  FOwner = NULL;
}
//---------------------------------------------------------------------------
void TBookmark::Assign(TPersistent * Source)
{
  TBookmark * SourceBookmark;
  SourceBookmark = reinterpret_cast<TBookmark *>(Source);
  if (SourceBookmark)
  {
    SetName(SourceBookmark->GetName());
    SetLocal(SourceBookmark->GetLocal());
    SetRemote(SourceBookmark->GetRemote());
    SetNode(SourceBookmark->GetNode());
    SetShortCut(SourceBookmark->GetShortCut());
  }
  else
  {
    TPersistent::Assign(Source);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetName(const UnicodeString value)
{
  if (GetName() != value)
  {
    size_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    UnicodeString OldName = FName;
    FName = value;
    try
    {
      Modify(OldIndex);
    }
    catch(...)
    {
      FName = OldName;
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetLocal(const UnicodeString value)
{
  if (GetLocal() != value)
  {
    FLocal = value;
    Modify(NPOS);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetRemote(const UnicodeString value)
{
  if (GetRemote() != value)
  {
    FRemote = value;
    Modify(NPOS);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetNode(const UnicodeString value)
{
  if (GetNode() != value)
  {
    size_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    FNode = value;
    Modify(OldIndex);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetShortCut(TShortCut value)
{
  if (GetShortCut() != value)
  {
    FShortCut = value;
    Modify(NPOS);
  }
}
//---------------------------------------------------------------------------
void TBookmark::Modify(size_t OldIndex)
{
  if (FOwner)
  {
    FOwner->SetModified(true);
    if (OldIndex >= 0)
    {
      FOwner->KeyChanged(OldIndex);
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TBookmark::BookmarkKey(const UnicodeString Node, const UnicodeString Name)
{
  return FORMAT(L"%s\1%s", Node.c_str(), Name.c_str());
}
//---------------------------------------------------------------------------
UnicodeString TBookmark::GetKey()
{
  return BookmarkKey(GetNode(), GetName());
}
