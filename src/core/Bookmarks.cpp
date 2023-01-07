#include <vcl.h>
#pragma hdrstop
#include <Common.h>
#include "NamedObjs.h"
#include "Bookmarks.h"
#include "Configuration.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"

__removed #pragma package(smart_init)

TBookmarks::TBookmarks() noexcept : TObject(OBJECT_CLASS_TBookmarks),
  FSharedKey(UnicodeString(CONST_HIDDEN_PREFIX) + "shared")
{
  FBookmarkLists = CreateSortedStringList(false, dupError);
}

TBookmarks::~TBookmarks() noexcept
{
  Clear();
  SAFE_DESTROY(FBookmarkLists);
}

void TBookmarks::Clear()
{
  for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
  {
    TObject *Object = FBookmarkLists->GetObj(Index);
    SAFE_DESTROY(Object);
  }
  FBookmarkLists->Clear();
}

std::string_view TBookmarks::Keys[]{"Local", "Remote", "ShortCuts", "Options"};

void TBookmarks::Load(THierarchicalStorage *Storage)
{
  for (intptr_t Idx = 0; Idx <= 3; ++Idx)
  {
    if (Storage->OpenSubKey(Keys[Idx].data(), false))
    {
      std::unique_ptr<TStrings> BookmarkKeys(std::make_unique<TStringList>());
      try__finally
      {
        Storage->GetSubKeyNames(BookmarkKeys.get());
        for (intptr_t Index2 = 0; Index2 < BookmarkKeys->GetCount(); ++Index2)
        {
          UnicodeString Key = BookmarkKeys->GetString(Index2);
          if (Storage->OpenSubKey(Key, false))
          {
            TBookmarkList *BookmarkList = GetBookmarks(Key);
            if (BookmarkList == nullptr)
            {
              BookmarkList = new TBookmarkList();
              FBookmarkLists->AddObject(Key, BookmarkList);
            }
            if (Idx < 3)
            {
              LoadLevel(Storage, L"", Idx, BookmarkList);
            }
            else
            {
              BookmarkList->LoadOptions(Storage);
            }
            Storage->CloseSubKey();
          }
        }
      },
      __finally__removed
      ({
        delete BookmarkKeys;
      }) end_try__finally
      Storage->CloseSubKey();
    }
  }

  ModifyAll(false);
}

void TBookmarks::LoadLevel(THierarchicalStorage *Storage, UnicodeString Key,
  intptr_t AIndex, TBookmarkList *BookmarkList)
{
  std::unique_ptr<TStrings> Names(std::make_unique<TStringList>());
  try__finally
  {
    Storage->GetValueNames(Names.get());
    UnicodeString Name;
    UnicodeString Directory;
    TShortCut ShortCut(0);
    for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
    {
      Name = Names->GetString(Index);
      bool IsDirectory = (AIndex == 0) || (AIndex == 1);
      if (IsDirectory)
      {
        Directory = Storage->ReadString(Name, L"");
      }
      else
      {
        Directory.Clear(); // use only in case of malformed config
        ShortCut = static_cast<TShortCut>(Storage->ReadInteger(Name, 0));
      }
      if (IsNumber(Name))
      {
        DebugAssert(IsDirectory); // unless malformed
        Name = Directory;
      }
      if (!Name.IsEmpty())
      {
        TBookmark *Bookmark = BookmarkList->FindByName(Key, Name);
        bool New = (Bookmark == nullptr);
        if (New)
        {
          Bookmark = new TBookmark();
          Bookmark->SetNode(Key);
          Bookmark->SetName(Name);
        }
        switch (AIndex)
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

    Storage->GetSubKeyNames(Names.get());
    for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
    {
      Name = Names->GetString(Index);
      if (Storage->OpenSubKey(Name, false))
      {
        LoadLevel(Storage, Key + (Key.IsEmpty() ? L"" : L"/") + Name, AIndex, BookmarkList);
        Storage->CloseSubKey();
      }
    }
  },
  __finally__removed
  ({
    delete Names;
  }) end_try__finally
}

void TBookmarks::Save(THierarchicalStorage *Storage, bool All)
{
  for (intptr_t Idx = 0; Idx <= 3; Idx++)
  {
    if (Storage->OpenSubKey(Keys[Idx].data(), true))
    {
      for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
      {
        TBookmarkList *BookmarkList = FBookmarkLists->GetAs<TBookmarkList>(Index);
        if (All || BookmarkList->GetModified())
        {
          UnicodeString Key = FBookmarkLists->GetString(Index);
          Storage->RecursiveDeleteSubKey(Key);
          if (Storage->OpenSubKey(Key, true))
          {
            if (Idx < 3)
            {
              for (intptr_t IndexB = 0; IndexB < BookmarkList->GetCount(); IndexB++)
              {
                TBookmark *Bookmark = BookmarkList->GetBookmarks(IndexB);
                // avoid creating empty subfolder if there's no shortcut
                if ((Idx == 0) || (Idx == 1) ||
                  ((Idx == 2) && (Bookmark->GetShortCut() != 0)))
                {
                  bool HasNode = !Bookmark->GetNode().IsEmpty();
                  if (!HasNode || Storage->OpenSubKey(Bookmark->GetNode(), true))
                  {
                    switch (Idx)
                    {
                    case 0:
                      Storage->WriteString(Bookmark->GetName(), Bookmark->GetLocal());
                      break;

                    case 1:
                      Storage->WriteString(Bookmark->GetName(), Bookmark->GetRemote());
                      break;

                    case 2:
                      DebugAssert(Bookmark->GetShortCut() != 0);
                      Storage->WriteInteger(Bookmark->GetName(), static_cast<int>(Bookmark->GetShortCut()));
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

void TBookmarks::ModifyAll(bool Modify)
{
  for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
  {
    TBookmarkList *BookmarkList = FBookmarkLists->GetAs<TBookmarkList>(Index);
    DebugAssert(BookmarkList);
    BookmarkList->SetModified(Modify);
  }
}

TBookmarkList *TBookmarks::GetBookmarks(UnicodeString AIndex)
{
  intptr_t Index = FBookmarkLists->IndexOf(AIndex);
  if (Index >= 0)
  {
    return FBookmarkLists->GetAs<TBookmarkList>(Index);
  }
  return nullptr;
}

void TBookmarks::SetBookmarks(UnicodeString AIndex, TBookmarkList *Value)
{
  intptr_t Index = FBookmarkLists->IndexOf(AIndex);
  if (Index >= 0)
  {
    TBookmarkList *BookmarkList = FBookmarkLists->GetAs<TBookmarkList>(Index);
    BookmarkList->Assign(Value);
  }
  else
  {
    TBookmarkList *BookmarkList = new TBookmarkList();
    BookmarkList->Assign(Value);
    FBookmarkLists->AddObject(AIndex, BookmarkList);
  }
}

TBookmarkList *TBookmarks::GetSharedBookmarks()
{
  return GetBookmarks(FSharedKey);
}

void TBookmarks::SetSharedBookmarks(TBookmarkList *Value)
{
  SetBookmarks(FSharedKey, Value);
}


TBookmarkList::TBookmarkList() noexcept :
  TPersistent(OBJECT_CLASS_TBookmarkList),
  FBookmarks(std::make_unique<TStringList>()),
  FOpenedNodes(CreateSortedStringList())
{
  FBookmarks->SetCaseSensitive(false);
}

TBookmarkList::~TBookmarkList() noexcept
{
  Clear();
  __removed SAFE_DESTROY(FBookmarks);
  __removed SAFE_DESTROY(FOpenedNodes);
}

void TBookmarkList::Clear()
{
  for (intptr_t Index = 0; Index < FBookmarks->GetCount(); ++Index)
  {
    TObject *Object = FBookmarks->GetObj(Index);
    SAFE_DESTROY(Object);
  }
  FBookmarks->Clear();
  FOpenedNodes->Clear();
}

void TBookmarkList::Assign(const TPersistent *Source)
{
  const TBookmarkList *SourceList = dyn_cast<TBookmarkList>(Source);
  if (SourceList)
  {
    Clear();
    for (intptr_t Index = 0; Index < SourceList->FBookmarks->GetCount(); ++Index)
    {
      TBookmark *Bookmark = new TBookmark();
      Bookmark->Assign(SourceList->FBookmarks->GetAs<TBookmark>(Index));
      Add(Bookmark);
    }
    FOpenedNodes->Assign(SourceList->FOpenedNodes.get());
    SetModified(SourceList->GetModified());
  }
  else
  {
    TPersistent::Assign(Source);
  }
}

void TBookmarkList::LoadOptions(THierarchicalStorage *Storage)
{
  FOpenedNodes->SetCommaText(Storage->ReadString("OpenedNodes", L""));
}

void TBookmarkList::SaveOptions(THierarchicalStorage *Storage) const
{
  Storage->WriteString("OpenedNodes", FOpenedNodes->GetCommaText());
}

void TBookmarkList::Add(TBookmark *Bookmark)
{
  Insert(GetCount(), Bookmark);
}

void TBookmarkList::InsertBefore(TBookmark *BeforeBookmark, TBookmark *Bookmark)
{
  DebugAssert(BeforeBookmark);
  intptr_t Index = FBookmarks->IndexOf(BeforeBookmark->GetKey());
  DebugAssert(Index >= 0);
  Insert(Index, Bookmark);
}

void TBookmarkList::MoveTo(TBookmark *ToBookmark,
  TBookmark *Bookmark, bool Before)
{
  DebugAssert(ToBookmark != nullptr);
  intptr_t NewIndex = ToBookmark ? FBookmarks->IndexOf(ToBookmark->GetKey()) : 0;
  DebugAssert(Bookmark != nullptr);
  intptr_t OldIndex = Bookmark ? FBookmarks->IndexOf(Bookmark->GetKey()) : 0;
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

void TBookmarkList::Insert(intptr_t Index, TBookmark *Bookmark)
{
  DebugAssert(Bookmark);
  DebugAssert(!Bookmark->FOwner);
  DebugAssert(!Bookmark->GetName().IsEmpty());

  FModified = true;
  Bookmark->FOwner = this;
  if (FBookmarks->IndexOf(Bookmark->GetKey()) >= 0)
  {
    throw Exception(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName()));
  }
  FBookmarks->InsertObject(Index, Bookmark->GetKey(), Bookmark);
}

void TBookmarkList::Delete(TBookmark *&Bookmark)
{
  DebugAssert(Bookmark);
  DebugAssert(Bookmark->FOwner == this);
  intptr_t Index = IndexOf(Bookmark);
  DebugAssert(Index >= 0);
  FModified = true;
  Bookmark->FOwner = nullptr;
  FBookmarks->Delete(Index);
  SAFE_DESTROY(Bookmark);
}

intptr_t TBookmarkList::IndexOf(TBookmark *Bookmark) const
{
  return FBookmarks->IndexOf(Bookmark->GetKey());
}

void TBookmarkList::KeyChanged(intptr_t Index)
{
  DebugAssert(Index < GetCount());
  TBookmark *Bookmark = FBookmarks->GetAs<TBookmark>(Index);
  DebugAssert(FBookmarks->GetString(Index) != Bookmark->GetKey());
  if (FBookmarks->IndexOf(Bookmark->GetKey()) >= 0)
  {
    throw Exception(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName()));
  }
  FBookmarks->SetString(Index, Bookmark->GetKey());
}

TBookmark *TBookmarkList::FindByName(UnicodeString Node, UnicodeString Name) const
{
  intptr_t Index = FBookmarks->IndexOf(TBookmark::BookmarkKey(Node, Name));
  TBookmark *Bookmark = ((Index >= 0) ? FBookmarks->GetAs<TBookmark>(Index) : nullptr);
  DebugAssert(!Bookmark || (Bookmark->GetNode() == Node && Bookmark->GetName() == Name));
  return Bookmark;
}

TBookmark *TBookmarkList::FindByShortCut(const TShortCut &ShortCut)
{
  for (intptr_t Index = 0; Index < FBookmarks->GetCount(); ++Index)
  {
    if (GetBookmarks(Index)->GetShortCut() == ShortCut)
    {
      return GetBookmarks(Index);
    }
  }
  return nullptr;
}

intptr_t TBookmarkList::GetCount() const
{
  return FBookmarks->GetCount();
}

TBookmark *TBookmarkList::GetBookmarks(intptr_t AIndex)
{
  TBookmark *Bookmark = FBookmarks->GetAs<TBookmark>(AIndex);
  DebugAssert(Bookmark);
  return Bookmark;
}

bool TBookmarkList::GetNodeOpened(UnicodeString AIndex) const
{
  return (FOpenedNodes->IndexOf(AIndex) >= 0);
}

void TBookmarkList::SetNodeOpened(UnicodeString AIndex, bool Value)
{
  intptr_t Index = FOpenedNodes->IndexOf(AIndex);
  if ((Index >= 0) != Value)
  {
    if (Value)
    {
      FOpenedNodes->Add(AIndex);
    }
    else
    {
      FOpenedNodes->Delete(Index);
    }
    FModified = true;
  }
}

void TBookmarkList::ShortCuts(TShortCuts &ShortCuts)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TBookmark *Bookmark = GetBookmarks(Index);
    if (Bookmark->GetShortCut() != 0)
    {
      ShortCuts.Add(Bookmark->GetShortCut());
    }
  }
}


TBookmark::TBookmark() noexcept :
  TPersistent(OBJECT_CLASS_TBookmark)
{
}

void TBookmark::Assign(const TPersistent *Source)
{
  const TBookmark *SourceBookmark = dyn_cast<TBookmark>(Source);
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

void TBookmark::SetName(UnicodeString Value)
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
    catch (...)
    {
      FName = OldName;
      throw;
    }
  }
}

void TBookmark::SetLocal(UnicodeString Value)
{
  if (GetLocal() != Value)
  {
    FLocal = Value;
    Modify(-1);
  }
}

void TBookmark::SetRemote(UnicodeString Value)
{
  if (GetRemote() != Value)
  {
    FRemote = Value;
    Modify(-1);
  }
}

void TBookmark::SetNode(UnicodeString Value)
{
  if (GetNode() != Value)
  {
    intptr_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    FNode = Value;
    Modify(OldIndex);
  }
}

void TBookmark::SetShortCut(const TShortCut &Value)
{
  if (GetShortCut() != Value)
  {
    FShortCut = Value;
    Modify(-1);
  }
}

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

UnicodeString TBookmark::BookmarkKey(UnicodeString Node, UnicodeString Name)
{
  return FORMAT("%s\1%s", Node, Name);
}

UnicodeString TBookmark::GetKey() const
{
  return BookmarkKey(GetNode(), GetName());
}

UnicodeString TBookmark::GetSideDirectory(TOperationSide Side) const
{
  return (Side == osLocal) ? FLocal : FRemote;
}
