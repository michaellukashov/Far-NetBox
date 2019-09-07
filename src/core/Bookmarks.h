#pragma once

#include <string_view>
#include <Classes.hpp>
//---------------------------------------------------------------------------
#include <CopyParam.h>
//---------------------------------------------------------------------------
class THierarchicalStorage;
class TBookmarkList;
class TShortCuts;
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TBookmarks);
class NB_CORE_EXPORT TBookmarks : public TObject
{
  NB_DISABLE_COPY(TBookmarks)
public:
  TBookmarks() noexcept;
  virtual ~TBookmarks() noexcept;

  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage, bool All);
  void ModifyAll(bool Modify);
  void Clear();

  __property TBookmarkList * Bookmarks[UnicodeString Index] = { read = GetBookmarks, write = SetBookmarks };
  __property TBookmarkList * SharedBookmarks = { read = GetSharedBookmarks, write = SetSharedBookmarks };

private:
  TStringList *FBookmarkLists;
  UnicodeString FSharedKey;
  static std::string_view Keys[];

public:
  TBookmarkList *GetBookmarks(UnicodeString AIndex);
  void SetBookmarks(UnicodeString AIndex, TBookmarkList *Value);
  TBookmarkList *GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList *Value);

private:
  void LoadLevel(THierarchicalStorage *Storage, UnicodeString Key,
    intptr_t AIndex, TBookmarkList *BookmarkList);
};
//---------------------------------------------------------------------------
class TBookmark;
NB_DEFINE_CLASS_ID(TBookmarkList);
class NB_CORE_EXPORT TBookmarkList : public TPersistent
{
  friend class TBookmarks;
  friend class TBookmark;
  NB_DISABLE_COPY(TBookmarkList)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TBookmarkList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TBookmarkList) || TPersistent::is(Kind); }
public:
  explicit TBookmarkList() noexcept;
  virtual ~TBookmarkList() noexcept;

  void Clear();
  void Add(TBookmark *Bookmark);
  void Insert(intptr_t Index, TBookmark *Bookmark);
  void InsertBefore(TBookmark *BeforeBookmark, TBookmark *Bookmark);
  void MoveTo(TBookmark *ToBookmark, TBookmark *Bookmark, bool Before);
  void Delete(TBookmark *&Bookmark);
  TBookmark *FindByName(UnicodeString Node, UnicodeString Name) const;
  TBookmark *FindByShortCut(const TShortCut &ShortCut);
  void Assign(const TPersistent *Source) override;
  void LoadOptions(THierarchicalStorage *Storage);
  void SaveOptions(THierarchicalStorage *Storage) const;
  void ShortCuts(TShortCuts &ShortCuts);

  __property int Count = { read = GetCount };
  ROProperty<intptr_t> Count{nb::bind(&TBookmarkList::GetCount, this)};
  __property TBookmark * Bookmarks[int Index] = { read = GetBookmarks };
  __property bool NodeOpened[UnicodeString Index] = { read = GetNodeOpened, write = SetNodeOpened };

protected:
  intptr_t IndexOf(TBookmark *Bookmark) const;
  void KeyChanged(intptr_t Index);

  __property bool Modified = { read = FModified, write = FModified };
  bool& Modified{FModified};
  bool GetModified() const { return FModified; }
  void SetModified(bool Value) { FModified = Value; }

private:
  std::unique_ptr<TStringList> FBookmarks;
  std::unique_ptr<TStringList> FOpenedNodes;
  bool FModified{false};

public:
  intptr_t GetCount() const;
  TBookmark *GetBookmarks(intptr_t AIndex);
  bool GetNodeOpened(UnicodeString AIndex) const;
  void SetNodeOpened(UnicodeString AIndex, bool Value);
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TBookmark);
class NB_CORE_EXPORT TBookmark : public TPersistent
{
  friend class TBookmarkList;
  NB_DISABLE_COPY(TBookmark)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TBookmark); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TBookmark) || TPersistent::is(Kind); }
public:
  TBookmark() noexcept;

  virtual void Assign(const TPersistent *Source) override;

  UnicodeString GetSideDirectory(TOperationSide Side) const;

  __property UnicodeString Name = { read = FName, write = SetName };
  __property UnicodeString Local = { read = FLocal, write = SetLocal };
  __property UnicodeString Remote = { read = FRemote, write = SetRemote };
  __property UnicodeString Node = { read = FNode, write = SetNode };
  __property TShortCut ShortCut = { read = FShortCut, write = SetShortCut };

protected:
  TBookmarkList *FOwner{nullptr};

  static UnicodeString BookmarkKey(UnicodeString Node, UnicodeString Name);
  __property UnicodeString Key = { read = GetKey };
  ROProperty<UnicodeString> Key{nb::bind(&TBookmark::GetKey, this)};

private:
  UnicodeString FName;
  UnicodeString FLocal;
  UnicodeString FRemote;
  UnicodeString FNode;
  TShortCut FShortCut{};

public:
  UnicodeString GetName() const { return FName; }
  UnicodeString GetLocal() const { return FLocal; }
  UnicodeString GetRemote() const { return FRemote; }
  UnicodeString GetNode() const { return FNode; }
  TShortCut GetShortCut() const { return FShortCut; }

  void SetName(UnicodeString Value);
  void SetLocal(UnicodeString Value);
  void SetRemote(UnicodeString Value);
  void SetNode(UnicodeString Value);
  void SetShortCut(const TShortCut &Value);
  UnicodeString GetKey() const;

private:
  void Modify(intptr_t OldIndex);
};
//---------------------------------------------------------------------------
