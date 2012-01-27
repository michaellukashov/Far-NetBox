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
TBookmarks::TBookmarks() : nb::TObject()
{
  FSharedKey = TNamedObjectList::HiddenPrefix + L"shared";
  FBookmarkLists = new nb::TStringList();
  FBookmarkLists->SetSorted(true);
  FBookmarkLists->SetCaseSensitive(false);
  FBookmarkLists->SetDuplicates(nb::dupError);
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
    delete FBookmarkLists->GetObject(i);
  }
  FBookmarkLists->Clear();
}
//---------------------------------------------------------------------------
std::wstring TBookmarks::Keys[] = { L"Local", L"Remote", L"ShortCuts", L"Options" };
//---------------------------------------------------------------------------
void TBookmarks::Load(THierarchicalStorage * Storage)
{
  for (int i = 0; i <= 3; i++)
  {
    if (Storage->OpenSubKey(Keys[i], false))
    {
      nb::TStrings * BookmarkKeys = new nb::TStringList();
      {
        BOOST_SCOPE_EXIT ( (&BookmarkKeys) )
        {
          delete BookmarkKeys;
        } BOOST_SCOPE_EXIT_END
        Storage->GetSubKeyNames(BookmarkKeys);
        for (size_t Index = 0; Index < BookmarkKeys->GetCount(); Index++)
        {
          std::wstring Key = BookmarkKeys->GetString(Index);
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
void TBookmarks::LoadLevel(THierarchicalStorage * Storage, const std::wstring Key,
  int Index, TBookmarkList * BookmarkList)
{
  nb::TStrings * Names = new nb::TStringList();
  {
      BOOST_SCOPE_EXIT ( (&Names) )
      {
        delete Names;
      } BOOST_SCOPE_EXIT_END
    Storage->GetValueNames(Names);
    std::wstring Name;
    std::wstring Directory;
    nb::TShortCut ShortCut(0);
    for (size_t i = 0; i < Names->GetCount(); i++)
    {
      Name = Names->GetString(i);
      bool IsDirectory = (Index == 0) || (Index == 1);
      if (IsDirectory)
      {
        Directory = Storage->ReadString(Name, L"");
      }
      else
      {
        Directory = L""; // use only in cased of malformed config
        ShortCut = nb::TShortCut(Storage->Readint(Name, 0));
      }
      TBookmark * Bookmark;
      if (IsNumber(Name))
      {
        assert(IsDirectory); // unless malformed
        Name = Directory;
      }
      if (!Name.empty())
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
      Name = Names->GetString(i);
      if (Storage->OpenSubKey(Name, false))
      {
        LoadLevel(Storage, Key + (Key.empty() ? L"" : L"/") + Name, Index, BookmarkList);
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
        TBookmarkList * BookmarkList = reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObject(Index));
        if (All || BookmarkList->GetModified())
        {
          std::wstring Key;
          Key = FBookmarkLists->GetString(Index);
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
                  bool HasNode = !Bookmark->GetNode().empty();
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
    BookmarkList = reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObject(i));
    assert(BookmarkList);
    BookmarkList->SetModified(Modify);
  }
}
//---------------------------------------------------------------------------
TBookmarkList * TBookmarks::GetBookmark(const std::wstring Index)
{
  size_t I = FBookmarkLists->IndexOf(Index.c_str());
  if (I != -1)
  {
    return reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObject(I));
  }
  else
  {
    return NULL;
  }
}
//---------------------------------------------------------------------------
void TBookmarks::SetBookmark(const std::wstring Index, TBookmarkList *value)
{
  size_t I = FBookmarkLists->IndexOf(Index.c_str());
  if (I != -1)
  {
    TBookmarkList * BookmarkList;
    BookmarkList = reinterpret_cast<TBookmarkList *>(FBookmarkLists->GetObject(I));
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
TBookmarkList::TBookmarkList(): nb::TPersistent()
{
  FModified = false;
  FBookmarks = new nb::TStringList();
  FBookmarks->SetCaseSensitive(false);
  FOpenedNodes = new nb::TStringList();
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
    delete FBookmarks->GetObject(i);
  }
  FBookmarks->Clear();
  FOpenedNodes->Clear();
}
//---------------------------------------------------------------------------
void TBookmarkList::Assign(nb::TPersistent * Source)
{
  TBookmarkList * SourceList;
  SourceList = reinterpret_cast<TBookmarkList *>(Source);
  if (SourceList)
  {
    Clear();
    for (size_t i = 0; i < SourceList->FBookmarks->GetCount(); i++)
    {
      TBookmark * Bookmark = new TBookmark();
      Bookmark->Assign(reinterpret_cast<TBookmark *>(SourceList->FBookmarks->GetObject(i)));
      Add(Bookmark);
    }
    FOpenedNodes->Assign(SourceList->FOpenedNodes);
    SetModified(SourceList->GetModified());
  }
  else
  {
    nb::TPersistent::Assign(Source);
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
  assert(I != -1);
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
  assert(!Bookmark->GetName().empty());

  FModified = true;
  Bookmark->FOwner = this;
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) != -1)
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
  assert(I != -1);
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
  TBookmark * Bookmark = reinterpret_cast<TBookmark *>(FBookmarks->GetObject(Index));
  assert(FBookmarks->GetString(Index) != Bookmark->GetKey());
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) != -1)
  {
    throw ExtException(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->PutString(Index, Bookmark->GetKey());
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::FindByName(const std::wstring Node, const std::wstring Name)
{
  size_t I = FBookmarks->IndexOf(TBookmark::BookmarkKey(Node, Name).c_str());
  TBookmark * Bookmark = ((I != -1) ? reinterpret_cast<TBookmark *>(FBookmarks->GetObject(I)) : NULL);
  assert(!Bookmark || (Bookmark->GetNode() == Node && Bookmark->GetName() == Name));
  return Bookmark;
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::FindByShortCut(nb::TShortCut ShortCut)
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
  TBookmark * Bookmark = reinterpret_cast<TBookmark *>(FBookmarks->GetObject(Index));
  assert(Bookmark);
  return Bookmark;
}
//---------------------------------------------------------------------------
bool TBookmarkList::GetNodeOpened(const std::wstring Index)
{
  return (FOpenedNodes->IndexOf(Index.c_str()) != -1);
}
//---------------------------------------------------------------------------
void TBookmarkList::SetNodeOpened(const std::wstring Index, bool value)
{
  size_t I = FOpenedNodes->IndexOf(Index.c_str());
  if ((I != -1) != value)
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
void TBookmark::Assign(nb::TPersistent * Source)
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
    nb::TPersistent::Assign(Source);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetName(const std::wstring value)
{
  if (GetName() != value)
  {
    size_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    std::wstring OldName = FName;
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
void TBookmark::SetLocal(const std::wstring value)
{
  if (GetLocal() != value)
  {
    FLocal = value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetRemote(const std::wstring value)
{
  if (GetRemote() != value)
  {
    FRemote = value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetNode(const std::wstring value)
{
  if (GetNode() != value)
  {
    size_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    FNode = value;
    Modify(OldIndex);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetShortCut(nb::TShortCut value)
{
  if (GetShortCut() != value)
  {
    FShortCut = value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void TBookmark::Modify(size_t OldIndex)
{
  if (FOwner)
  {
    FOwner->SetModified(true);
    if (OldIndex != -1)
    {
      FOwner->KeyChanged(OldIndex);
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TBookmark::BookmarkKey(const std::wstring Node, const std::wstring Name)
{
  return FORMAT(L"%s\1%s", Node.c_str(), Name.c_str());
}
//---------------------------------------------------------------------------
std::wstring TBookmark::GetKey()
{
  return BookmarkKey(GetNode(), GetName());
}
