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

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage, bool All);
  void ModifyAll(bool Modify);
  void Clear();

#if 0
  __property TBookmarkList * Bookmarks[UnicodeString Index] = { read = GetBookmarks, write = SetBookmarks };
  __property TBookmarkList * SharedBookmarks = { read = GetSharedBookmarks, write = SetSharedBookmarks };
#endif // if 0

private:
  TStringList * FBookmarkLists;
  UnicodeString FSharedKey;
  static UnicodeString Keys[];

public:
  TBookmarkList * GetBookmarks(UnicodeString AIndex);
  void SetBookmarks(UnicodeString AIndex, TBookmarkList * Value);
  TBookmarkList * GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList * Value);

private:
  void LoadLevel(THierarchicalStorage * Storage, UnicodeString Key,
    intptr_t AIndex, TBookmarkList * BookmarkList);
};

class TBookmark;
class NB_CORE_EXPORT TBookmarkList : public TPersistent
{
friend class TBookmarks;
friend class TBookmark;
NB_DISABLE_COPY(TBookmarkList)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TBookmarkList;
  }
public:
  explicit TBookmarkList();
  virtual ~TBookmarkList();

  void Clear();
  void Add(TBookmark * Bookmark);
  void Insert(intptr_t Index, TBookmark * Bookmark);
  void InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark);
  void MoveTo(TBookmark * ToBookmark, TBookmark * Bookmark, bool Before);
  void Delete(TBookmark *& Bookmark);
  TBookmark * FindByName(UnicodeString Node, UnicodeString Name) const;
  TBookmark * FindByShortCut(const TShortCut & ShortCut);
  virtual void Assign(const TPersistent * Source);
  void LoadOptions(THierarchicalStorage * Storage);
  void SaveOptions(THierarchicalStorage * Storage) const;
  void ShortCuts(TShortCuts & ShortCuts);

#if 0
  __property int Count = { read = GetCount };
  __property TBookmark * Bookmarks[int Index] = { read = GetBookmarks };
  __property bool NodeOpened[UnicodeString Index] = { read = GetNodeOpened, write = SetNodeOpened };
#endif // if 0

protected:
  intptr_t IndexOf(TBookmark * Bookmark) const;
  void KeyChanged(intptr_t Index);

#if 0
  __property bool Modified = { read = FModified, write = FModified };
#endif // if 0
  bool GetModified() const { return FModified; }
  void SetModified(bool Value) { FModified = Value; }

private:
  TStringList * FBookmarks;
  TStringList * FOpenedNodes;
  bool FModified;

public:
  intptr_t GetCount() const;
  TBookmark * GetBookmarks(intptr_t AIndex);
  bool GetNodeOpened(UnicodeString AIndex) const;
  void SetNodeOpened(UnicodeString AIndex, bool Value);
};

class NB_CORE_EXPORT TBookmark : public TPersistent
{
friend class TBookmarkList;
NB_DISABLE_COPY(TBookmark)
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TBookmark;
  }
public:
  TBookmark();

  virtual void Assign(const TPersistent * Source);

#if 0
  __property UnicodeString Name = { read = FName, write = SetName };
  __property UnicodeString Local = { read = FLocal, write = SetLocal };
  __property UnicodeString Remote = { read = FRemote, write = SetRemote };
  __property UnicodeString Node = { read = FNode, write = SetNode };
  __property TShortCut ShortCut = { read = FShortCut, write = SetShortCut };
#endif // if 0

protected:
  TBookmarkList * FOwner;

  static UnicodeString BookmarkKey(UnicodeString Node, UnicodeString Name);
#if 0
  __property UnicodeString Key = { read = GetKey };
#endif // if 0

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

  void SetName(UnicodeString Value);
  void SetLocal(UnicodeString Value);
  void SetRemote(UnicodeString Value);
  void SetNode(UnicodeString Value);
  void SetShortCut(const TShortCut & Value);
  UnicodeString GetKey() const;

private:
  void Modify(intptr_t OldIndex);
};

