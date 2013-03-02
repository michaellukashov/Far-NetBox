//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include <Common.h>
#include "NamedObjs.h"
#include "Bookmarks.h"
#include "Configuration.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TBookmarks::TBookmarks() : TObject()
{
  FSharedKey = TNamedObjectList::HiddenPrefix + L"shared";
  FBookmarkLists = new TStringList();
  FBookmarkLists->Sorted = true;
  FBookmarkLists->CaseSensitive = false;
  FBookmarkLists->Duplicates = dupError;
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
  for (intptr_t I = 0; I < FBookmarkLists->GetCount(); I++)
  {
    delete FBookmarkLists->Objects[I];
  }
  FBookmarkLists->Clear();
}
//---------------------------------------------------------------------------
UnicodeString TBookmarks::Keys[] = { L"Local", L"Remote", L"ShortCuts", L"Options" };
//---------------------------------------------------------------------------
void TBookmarks::Load(THierarchicalStorage * Storage)
{
  CALLSTACK;
  for (intptr_t I = 0; I <= 3; I++)
  {
    if (Storage->OpenSubKey(Keys[I], false))
    {
      TStrings * BookmarkKeys = new TStringList();
      TRY_FINALLY (
      {
        Storage->GetSubKeyNames(BookmarkKeys);
        for (intptr_t Index = 0; Index < BookmarkKeys->GetCount(); ++Index)
        {
          UnicodeString Key = BookmarkKeys->Strings[Index];
          if (Storage->OpenSubKey(Key, false))
          {
            TBookmarkList * BookmarkList = GetBookmarks(Key);
            if (!BookmarkList)
            {
              BookmarkList = new TBookmarkList();
              FBookmarkLists->AddObject(Key, BookmarkList);
            }
            if (I < 3)
            {
              LoadLevel(Storage, L"", I, BookmarkList);
            }
            else
            {
              BookmarkList->LoadOptions(Storage);
            }
            Storage->CloseSubKey();
          }
        }
      }
      ,
      {
        delete BookmarkKeys;
      }
      );
      Storage->CloseSubKey();
    }
  }

  ModifyAll(false);
}
//---------------------------------------------------------------------------
void TBookmarks::LoadLevel(THierarchicalStorage * Storage, const UnicodeString & Key,
  intptr_t Index, TBookmarkList * BookmarkList)
{
  CALLSTACK;
  TStrings * Names = new TStringList();
  TRY_FINALLY (
  {
    Storage->GetValueNames(Names);
    UnicodeString Name;
    UnicodeString Directory;
    TShortCut ShortCut(0);
    for (intptr_t I = 0; I < Names->GetCount(); ++I)
    {
      Name = Names->Strings[I];
      bool IsDirectory = (Index == 0) || (Index == 1);
      if (IsDirectory)
      {
        Directory = Storage->ReadString(Name, L"");
      }
      else
      {
        Directory = L""; // use only in case of malformed config
        ShortCut = static_cast<TShortCut>(Storage->ReadInteger(Name, 0));
      }
      if (Name.ToInt() > 0)
      {
        assert(IsDirectory); // unless malformed
        Name = Directory;
      }
      if (!Name.IsEmpty())
      {
        TBookmark * Bookmark = BookmarkList->FindByName(Key, Name);
        bool New = (Bookmark == NULL);
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
    for (intptr_t I = 0; I < Names->GetCount(); ++I)
    {
      Name = Names->Strings[I];
      if (Storage->OpenSubKey(Name, false))
      {
        LoadLevel(Storage, Key + (Key.IsEmpty() ? L"" : L"/") + Name, Index, BookmarkList);
        Storage->CloseSubKey();
      }
    }
  }
  ,
  {
   delete Names;
  }
  );
}
//---------------------------------------------------------------------------
void TBookmarks::Save(THierarchicalStorage * Storage, bool All)
{
  for (intptr_t I = 0; I <= 3; I++)
  {
    if (Storage->OpenSubKey(Keys[I], true))
    {
      for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
      {
        TBookmarkList * BookmarkList = dynamic_cast<TBookmarkList *>(FBookmarkLists->Objects[Index]);
        if (All || BookmarkList->GetModified())
        {
          UnicodeString Key;
          Key = FBookmarkLists->Strings[Index];
          Storage->RecursiveDeleteSubKey(Key);
          if (Storage->OpenSubKey(Key, true))
          {
            if (I < 3)
            {
              for (intptr_t IndexB = 0; IndexB < BookmarkList->GetCount(); IndexB++)
              {
                TBookmark * Bookmark = BookmarkList->GetBookmarks(IndexB);
                // avoid creating empty subfolder if there's no shortcut
                if ((I == 0) || (I == 1) ||
                    ((I == 2) && (Bookmark->GetShortCut() != 0)))
                {
                  bool HasNode = !Bookmark->GetNode().IsEmpty();
                  if (!HasNode || Storage->OpenSubKey(Bookmark->GetNode(), true))
                  {
                    switch (I)
                    {
                      case 0:
                        Storage->WriteString(Bookmark->GetName(), Bookmark->GetLocal());
                        break;

                      case 1:
                        Storage->WriteString(Bookmark->GetName(), Bookmark->GetRemote());
                        break;

                      case 2:
                        assert(Bookmark->GetShortCut() != 0);
                        Storage->WriteInteger(Bookmark->GetName(), Bookmark->GetShortCut());
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
  for (intptr_t I = 0; I < FBookmarkLists->GetCount(); I++)
  {
    BookmarkList = dynamic_cast<TBookmarkList *>(FBookmarkLists->Objects[I]);
    assert(BookmarkList);
    BookmarkList->SetModified(Modify);
  }
}
//---------------------------------------------------------------------------
TBookmarkList * TBookmarks::GetBookmarks(const UnicodeString & Index)
{
  intptr_t I = FBookmarkLists->IndexOf(Index.c_str());
  if (I >= 0)
  {
    return dynamic_cast<TBookmarkList *>(FBookmarkLists->Objects[I]);
  }
  else
  {
    return NULL;
  }
}
//---------------------------------------------------------------------------
void TBookmarks::SetBookmarks(const UnicodeString & Index, TBookmarkList * Value)
{
  intptr_t I = FBookmarkLists->IndexOf(Index.c_str());
  if (I >= 0)
  {
    TBookmarkList * BookmarkList;
    BookmarkList = dynamic_cast<TBookmarkList *>(FBookmarkLists->Objects[I]);
    BookmarkList->Assign(Value);
  }
  else
  {
    TBookmarkList * BookmarkList = new TBookmarkList();
    BookmarkList->Assign(Value);
    FBookmarkLists->AddObject(Index, BookmarkList);
  }
}
//---------------------------------------------------------------------------
TBookmarkList * TBookmarks::GetSharedBookmarks()
{
  return GetBookmarks(FSharedKey);
}
//---------------------------------------------------------------------------
void TBookmarks::SetSharedBookmarks(TBookmarkList * Value)
{
  SetBookmarks(FSharedKey, Value);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TBookmarkList::TBookmarkList(): TPersistent()
{
  FModified = false;
  FBookmarks = new TStringList();
  FBookmarks->CaseSensitive = false;
  FOpenedNodes = new TStringList();
  FOpenedNodes->CaseSensitive = false;
  FOpenedNodes->Sorted = true;
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
  for (intptr_t I = 0; I < FBookmarks->GetCount(); I++)
  {
    delete FBookmarks->Objects[I];
  }
  FBookmarks->Clear();
  FOpenedNodes->Clear();
}
//---------------------------------------------------------------------------
void TBookmarkList::Assign(TPersistent * Source)
{
  TBookmarkList * SourceList;
  SourceList = dynamic_cast<TBookmarkList *>(Source);
  if (SourceList)
  {
    Clear();
    for (intptr_t I = 0; I < SourceList->FBookmarks->GetCount(); I++)
    {
      TBookmark * Bookmark = new TBookmark();
      Bookmark->Assign(dynamic_cast<TBookmark *>(SourceList->FBookmarks->Objects[I]));
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
  CALLSTACK;
  FOpenedNodes->CommaText = Storage->ReadString(L"OpenedNodes", L"");
}
//---------------------------------------------------------------------------
void TBookmarkList::SaveOptions(THierarchicalStorage * Storage)
{
  Storage->WriteString(L"OpenedNodes", FOpenedNodes->CommaText);
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
  intptr_t I = FBookmarks->IndexOf(BeforeBookmark->GetKey().c_str());
  assert(I >= 0);
  Insert(I, Bookmark);
}
//---------------------------------------------------------------------------
void TBookmarkList::MoveTo(TBookmark * ToBookmark,
  TBookmark * Bookmark, bool Before)
{
  assert(ToBookmark != NULL);
  intptr_t NewIndex = FBookmarks->IndexOf(ToBookmark->GetKey().c_str());
  assert(Bookmark != NULL);
  intptr_t OldIndex = FBookmarks->IndexOf(Bookmark->GetKey().c_str());
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
void TBookmarkList::Insert(intptr_t Index, TBookmark * Bookmark)
{
  assert(Bookmark);
  assert(!Bookmark->FOwner);
  assert(!Bookmark->GetName().IsEmpty());

  FModified = true;
  Bookmark->FOwner = this;
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) >= 0)
  {
    throw Exception(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->InsertObject(Index, Bookmark->GetKey(), Bookmark);
}
//---------------------------------------------------------------------------
void TBookmarkList::Delete(TBookmark * Bookmark)
{
  assert(Bookmark);
  assert(Bookmark->FOwner == this);
  intptr_t I = IndexOf(Bookmark);
  assert(I >= 0);
  FModified = true;
  Bookmark->FOwner = NULL;
  FBookmarks->Delete(I);
  delete Bookmark;
}
//---------------------------------------------------------------------------
intptr_t TBookmarkList::IndexOf(TBookmark * Bookmark)
{
  return FBookmarks->IndexOf(Bookmark->GetKey().c_str());
}
//---------------------------------------------------------------------------
void TBookmarkList::KeyChanged(intptr_t Index)
{
  assert(Index < GetCount());
  TBookmark * Bookmark = dynamic_cast<TBookmark *>(FBookmarks->Objects[Index]);
  assert(FBookmarks->Strings[Index] != Bookmark->GetKey());
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) >= 0)
  {
    throw Exception(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->Strings[Index] = Bookmark->GetKey();
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::FindByName(const UnicodeString & Node, const UnicodeString & Name)
{
  intptr_t I = FBookmarks->IndexOf(TBookmark::BookmarkKey(Node, Name).c_str());
  TBookmark * Bookmark = ((I >= 0) ? dynamic_cast<TBookmark *>(FBookmarks->Objects[I]) : NULL);
  assert(!Bookmark || (Bookmark->GetNode() == Node && Bookmark->GetName() == Name));
  return Bookmark;
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::FindByShortCut(TShortCut ShortCut)
{
  for (intptr_t Index = 0; Index < FBookmarks->GetCount(); ++Index)
  {
    if (GetBookmarks(Index)->GetShortCut() == ShortCut)
    {
      return GetBookmarks(Index);
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
intptr_t TBookmarkList::GetCount()
{
  return FBookmarks->GetCount();
}
//---------------------------------------------------------------------------
TBookmark * TBookmarkList::GetBookmarks(intptr_t Index)
{
  TBookmark * Bookmark = dynamic_cast<TBookmark *>(FBookmarks->Objects[Index]);
  assert(Bookmark);
  return Bookmark;
}
//---------------------------------------------------------------------------
bool TBookmarkList::GetNodeOpened(const UnicodeString & Index)
{
  return (FOpenedNodes->IndexOf(Index.c_str()) >= 0);
}
//---------------------------------------------------------------------------
void TBookmarkList::SetNodeOpened(const UnicodeString & Index, bool Value)
{
  intptr_t I = FOpenedNodes->IndexOf(Index.c_str());
  if ((I >= 0) != Value)
  {
    if (Value)
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
  CALLSTACK;
  TRACE(">");
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TBookmark * Bookmark = GetBookmarks(Index);
    if (Bookmark->GetShortCut() != 0)
    {
      ShortCuts.Add(Bookmark->GetShortCut());
    }
  }
  TRACE("/");
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
  SourceBookmark = dynamic_cast<TBookmark *>(Source);
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
void TBookmark::SetName(const UnicodeString & Value)
{
  if (GetName() != Value)
  {
    intptr_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    UnicodeString OldName = FName;
    FName = Value;
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
void TBookmark::SetLocal(const UnicodeString & Value)
{
  if (GetLocal() != Value)
  {
    FLocal = Value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetRemote(const UnicodeString & Value)
{
  if (GetRemote() != Value)
  {
    FRemote = Value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetNode(const UnicodeString & Value)
{
  if (GetNode() != Value)
  {
    intptr_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    FNode = Value;
    Modify(OldIndex);
  }
}
//---------------------------------------------------------------------------
void TBookmark::SetShortCut(TShortCut Value)
{
  if (GetShortCut() != Value)
  {
    FShortCut = Value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void TBookmark::Modify(intptr_t OldIndex)
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
UnicodeString TBookmark::BookmarkKey(const UnicodeString & Node, const UnicodeString & Name)
{
  return FORMAT(L"%s\1%s", Node.c_str(), Name.c_str());
}
//---------------------------------------------------------------------------
UnicodeString TBookmark::GetKey()
{
  return BookmarkKey(GetNode(), GetName());
}


