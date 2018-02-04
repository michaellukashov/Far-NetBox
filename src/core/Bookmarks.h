#pragma once

#include <Classes.hpp>

class THierarchicalStorage;
class TBookmarkList;
class TShortCuts;

class NB_CORE_EXPORT TBookmarks : public TObject
{
  NB_DISABLE_COPY(TBookmarks)
public:
  TBookmarks();
  virtual ~TBookmarks();

  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage, bool All);
  void ModifyAll(bool Modify);
  void Clear();

  __property TBookmarkList * Bookmarks[UnicodeString Index] = { read = GetBookmarks, write = SetBookmarks };
  __property TBookmarkList * SharedBookmarks = { read = GetSharedBookmarks, write = SetSharedBookmarks };

private:
  TStringList *FBookmarkLists;
  UnicodeString FSharedKey;
  static UnicodeString Keys[];

public:
  TBookmarkList *GetBookmarks(const UnicodeString AIndex);
  void SetBookmarks(const UnicodeString AIndex, TBookmarkList *Value);
  TBookmarkList *GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList *Value);

private:
  void LoadLevel(THierarchicalStorage *Storage, UnicodeString Key,
    intptr_t AIndex, TBookmarkList *BookmarkList);
};

class TBookmark;
class NB_CORE_EXPORT TBookmarkList : public TPersistent
{
  friend class TBookmarks;
  friend class TBookmark;
  NB_DISABLE_COPY(TBookmarkList)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TBookmarkList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TBookmarkList) || TPersistent::is(Kind); }
public:
  explicit TBookmarkList();
  virtual ~TBookmarkList();

  void Clear();
  void Add(TBookmark *Bookmark);
  void Insert(intptr_t Index, TBookmark *Bookmark);
  void InsertBefore(TBookmark *BeforeBookmark, TBookmark *Bookmark);
  void MoveTo(TBookmark *ToBookmark, TBookmark *Bookmark, bool Before);
  void Delete(TBookmark *&Bookmark);
  TBookmark *FindByName(const UnicodeString Node, UnicodeString Name) const;
  TBookmark *FindByShortCut(const TShortCut &ShortCut);
  virtual void Assign(const TPersistent *Source) override;
  void LoadOptions(THierarchicalStorage *Storage);
  void SaveOptions(THierarchicalStorage *Storage) const;
  void ShortCuts(TShortCuts &ShortCuts);

  __property int Count = { read = GetCount };
  __property TBookmark * Bookmarks[int Index] = { read = GetBookmarks };
  __property bool NodeOpened[UnicodeString Index] = { read = GetNodeOpened, write = SetNodeOpened };

protected:
  intptr_t IndexOf(TBookmark *Bookmark) const;
  void KeyChanged(intptr_t Index);

  __property bool Modified = { read = FModified, write = FModified };
  bool GetModified() const { return FModified; }
  void SetModified(bool Value) { FModified = Value; }

private:
  TStringList *FBookmarks;
  TStringList *FOpenedNodes;
  bool FModified;

public:
  intptr_t GetCount() const;
  TBookmark *GetBookmarks(intptr_t AIndex);
  bool GetNodeOpened(const UnicodeString AIndex) const;
  void SetNodeOpened(const UnicodeString AIndex, bool Value);
};

class NB_CORE_EXPORT TBookmark : public TPersistent
{
  friend class TBookmarkList;
  NB_DISABLE_COPY(TBookmark)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TBookmark); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TBookmark) || TPersistent::is(Kind); }
public:
  TBookmark();

  virtual void Assign(const TPersistent *Source) override;

  __property UnicodeString Name = { read = FName, write = SetName };
  __property UnicodeString Local = { read = FLocal, write = SetLocal };
  __property UnicodeString Remote = { read = FRemote, write = SetRemote };
  __property UnicodeString Node = { read = FNode, write = SetNode };
  __property TShortCut ShortCut = { read = FShortCut, write = SetShortCut };

protected:
  TBookmarkList *FOwner;

  static UnicodeString BookmarkKey(const UnicodeString Node, const UnicodeString Name);
  __property UnicodeString Key = { read = GetKey };

private:
  UnicodeString FName;
  UnicodeString FLocal;
  UnicodeString FRemote;
  UnicodeString FNode;
  TShortCut FShortCut;

public:
  UnicodeString GetName() const { return FName; }
  UnicodeString GetLocal() const { return FLocal; }
  UnicodeString GetRemote() const { return FRemote; }
  UnicodeString GetNode() const { return FNode; }
  TShortCut GetShortCut() const { return FShortCut; }

  void SetName(const UnicodeString Value);
  void SetLocal(const UnicodeString Value);
  void SetRemote(const UnicodeString Value);
  void SetNode(const UnicodeString Value);
  void SetShortCut(const TShortCut &Value);
  UnicodeString GetKey() const;

private:
  void Modify(intptr_t OldIndex);
};

