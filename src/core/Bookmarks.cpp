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
/* __fastcall */ TBookmarks::TBookmarks() : TObject()
{
  FSharedKey = TNamedObjectList::HiddenPrefix + L"shared";
  FBookmarkLists = new TStringList();
  FBookmarkLists->Sorted = true;
  FBookmarkLists->CaseSensitive = false;
  FBookmarkLists->Duplicates = dupError;
}
//---------------------------------------------------------------------------
/* __fastcall */ TBookmarks::~TBookmarks()
{
  Clear();
  SAFE_DESTROY(FBookmarkLists);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarks::Clear()
{
  for (int i = 0; i < FBookmarkLists->Count; i++)
  {
    delete FBookmarkLists->Objects[i];
  }
  FBookmarkLists->Clear();
}
//---------------------------------------------------------------------------
UnicodeString TBookmarks::Keys[] = { L"Local", L"Remote", L"ShortCuts", L"Options" };
//---------------------------------------------------------------------------
void __fastcall TBookmarks::Load(THierarchicalStorage * Storage)
{
  CALLSTACK;
  for (int i = 0; i <= 3; i++)
  {
    if (Storage->OpenSubKey(Keys[i], false))
    {
      TStrings * BookmarkKeys = new TStringList();
      TRY_FINALLY (
      {
        Storage->GetSubKeyNames(BookmarkKeys);
        for (int Index = 0; Index < BookmarkKeys->Count; Index++)
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
void __fastcall TBookmarks::LoadLevel(THierarchicalStorage * Storage, const UnicodeString Key,
  int Index, TBookmarkList * BookmarkList)
{
  CALLSTACK;
  TStrings * Names = new TStringList();
  TRY_FINALLY (
  {
    Storage->GetValueNames(Names);
    UnicodeString Name;
    UnicodeString Directory;
    TShortCut ShortCut(0);
    for (int i = 0; i < Names->Count; i++)
    {
      Name = Names->Strings[i];
      bool IsDirectory = (Index == 0) || (Index == 1);
      if (IsDirectory)
      {
        Directory = Storage->ReadString(Name, L"");
      }
      else
      {
        Directory = L""; // use only in cased of malformed config
        ShortCut = static_cast<TShortCut>(Storage->ReadInteger(Name, 0));
      }
      if (IsNumber(Name))
      {
        assert(IsDirectory); // unless malformed
        Name = Directory;
      }
      if (!Name.IsEmpty())
      {
        TBookmark * Bookmark = BookmarkList->FindByName(Key, Name);
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
    for (int i = 0; i < Names->Count; i++)
    {
      Name = Names->Strings[i];
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
void __fastcall TBookmarks::Save(THierarchicalStorage * Storage, bool All)
{
  for (int i = 0; i <= 3; i++)
  {
    if (Storage->OpenSubKey(Keys[i], true))
    {
      for (int Index = 0; Index < FBookmarkLists->Count; Index++)
      {
        TBookmarkList * BookmarkList = dynamic_cast<TBookmarkList *>(FBookmarkLists->Objects[Index]);
        if (All || BookmarkList->GetModified())
        {
          UnicodeString Key;
          Key = FBookmarkLists->Strings[Index];
          Storage->RecursiveDeleteSubKey(Key);
          if (Storage->OpenSubKey(Key, true))
          {
            if (i < 3)
            {
              for (int IndexB = 0; IndexB < BookmarkList->GetCount(); IndexB++)
              {
                TBookmark * Bookmark = BookmarkList->GetBookmarks(IndexB);
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
void __fastcall TBookmarks::ModifyAll(bool Modify)
{
  TBookmarkList * BookmarkList;
  for (int i = 0; i < FBookmarkLists->Count; i++)
  {
    BookmarkList = dynamic_cast<TBookmarkList *>(FBookmarkLists->Objects[i]);
    assert(BookmarkList);
    BookmarkList->SetModified(Modify);
  }
}
//---------------------------------------------------------------------------
TBookmarkList * __fastcall TBookmarks::GetBookmarks(UnicodeString Index)
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
void __fastcall TBookmarks::SetBookmarks(UnicodeString Index, TBookmarkList * Value)
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
TBookmarkList * __fastcall TBookmarks::GetSharedBookmarks()
{
  return GetBookmarks(FSharedKey);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarks::SetSharedBookmarks(TBookmarkList * Value)
{
  SetBookmarks(FSharedKey, Value);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TBookmarkList::TBookmarkList(): TPersistent()
{
  FModified = false;
  FBookmarks = new TStringList();
  FBookmarks->CaseSensitive = false;
  FOpenedNodes = new TStringList();
  FOpenedNodes->CaseSensitive = false;
  FOpenedNodes->Sorted = true;
}
//---------------------------------------------------------------------------
/* __fastcall */ TBookmarkList::~TBookmarkList()
{
  Clear();
  SAFE_DESTROY(FBookmarks);
  SAFE_DESTROY(FOpenedNodes);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::Clear()
{
  for (int i = 0; i < FBookmarks->Count; i++)
  {
    delete FBookmarks->Objects[i];
  }
  FBookmarks->Clear();
  FOpenedNodes->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::Assign(TPersistent * Source)
{
  TBookmarkList * SourceList;
  SourceList = dynamic_cast<TBookmarkList *>(Source);
  if (SourceList)
  {
    Clear();
    for (int i = 0; i < SourceList->FBookmarks->Count; i++)
    {
      TBookmark * Bookmark = new TBookmark();
      Bookmark->Assign(dynamic_cast<TBookmark *>(SourceList->FBookmarks->Objects[i]));
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
void __fastcall TBookmarkList::LoadOptions(THierarchicalStorage * Storage)
{
  CALLSTACK;
  FOpenedNodes->CommaText = Storage->ReadString(L"OpenedNodes", L"");
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::SaveOptions(THierarchicalStorage * Storage)
{
  Storage->WriteString(L"OpenedNodes", FOpenedNodes->CommaText);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::Add(TBookmark * Bookmark)
{
  Insert(GetCount(), Bookmark);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark)
{
  assert(BeforeBookmark);
  intptr_t I = FBookmarks->IndexOf(BeforeBookmark->GetKey().c_str());
  assert(I >= 0);
  Insert(I, Bookmark);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::MoveTo(TBookmark * ToBookmark,
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
void __fastcall TBookmarkList::Insert(intptr_t Index, TBookmark * Bookmark)
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
void __fastcall TBookmarkList::Delete(TBookmark * Bookmark)
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
intptr_t __fastcall TBookmarkList::IndexOf(TBookmark * Bookmark)
{
  return FBookmarks->IndexOf(Bookmark->GetKey().c_str());
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::KeyChanged(intptr_t Index)
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
TBookmark * __fastcall TBookmarkList::FindByName(const UnicodeString Node, const UnicodeString Name)
{
  intptr_t I = FBookmarks->IndexOf(TBookmark::BookmarkKey(Node, Name).c_str());
  TBookmark * Bookmark = ((I >= 0) ? dynamic_cast<TBookmark *>(FBookmarks->Objects[I]) : NULL);
  assert(!Bookmark || (Bookmark->GetNode() == Node && Bookmark->GetName() == Name));
  return Bookmark;
}
//---------------------------------------------------------------------------
TBookmark * __fastcall TBookmarkList::FindByShortCut(TShortCut ShortCut)
{
  for (int Index = 0; Index < FBookmarks->Count; Index++)
  {
    if (GetBookmarks(Index)->GetShortCut() == ShortCut)
    {
      return GetBookmarks(Index);
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
intptr_t __fastcall TBookmarkList::GetCount()
{
  return FBookmarks->Count;
}
//---------------------------------------------------------------------------
TBookmark * __fastcall TBookmarkList::GetBookmarks(intptr_t Index)
{
  TBookmark * Bookmark = dynamic_cast<TBookmark *>(FBookmarks->Objects[Index]);
  assert(Bookmark);
  return Bookmark;
}
//---------------------------------------------------------------------------
bool __fastcall TBookmarkList::GetNodeOpened(UnicodeString Index)
{
  return (FOpenedNodes->IndexOf(Index.c_str()) >= 0);
}
//---------------------------------------------------------------------------
void __fastcall TBookmarkList::SetNodeOpened(UnicodeString Index, bool Value)
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
void __fastcall TBookmarkList::ShortCuts(TShortCuts & ShortCuts)
{
  CALLSTACK;
  TRACE(">");
  for (int Index = 0; Index < GetCount(); Index++)
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
/* __fastcall */ TBookmark::TBookmark()
{
  FOwner = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TBookmark::Assign(TPersistent * Source)
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
void __fastcall TBookmark::SetName(const UnicodeString Value)
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
void __fastcall TBookmark::SetLocal(const UnicodeString Value)
{
  if (GetLocal() != Value)
  {
    FLocal = Value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void __fastcall TBookmark::SetRemote(const UnicodeString Value)
{
  if (GetRemote() != Value)
  {
    FRemote = Value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void __fastcall TBookmark::SetNode(const UnicodeString Value)
{
  if (GetNode() != Value)
  {
    intptr_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    FNode = Value;
    Modify(OldIndex);
  }
}
//---------------------------------------------------------------------------
void __fastcall TBookmark::SetShortCut(TShortCut Value)
{
  if (GetShortCut() != Value)
  {
    FShortCut = Value;
    Modify(-1);
  }
}
//---------------------------------------------------------------------------
void __fastcall TBookmark::Modify(intptr_t OldIndex)
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
UnicodeString __fastcall TBookmark::BookmarkKey(const UnicodeString Node, const UnicodeString Name)
{
  return FORMAT(L"%s\1%s", Node.c_str(), Name.c_str());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TBookmark::GetKey()
{
  return BookmarkKey(GetNode(), GetName());
}


