//---------------------------------------------------------------------------
#ifndef BookmarksH
#define BookmarksH
//---------------------------------------------------------------------------
class THierarchicalStorage;
class TBookmarkList;
class TShortCuts;
//---------------------------------------------------------------------------
class TBookmarks : public TObject
{
public:
  TBookmarks();
  virtual ~TBookmarks();

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage, bool All);
  void ModifyAll(bool Modify);
  void Clear();

  // __property TBookmarkList * Bookmarks[wstring Index] = { read = GetBookmarks, write = SetBookmarks };
  TBookmarkList * GetBookmarks(wstring Index);
  void SetBookmarks(wstring Index, TBookmarkList * value);
  // __property TBookmarkList * SharedBookmarks = { read = GetSharedBookmarks, write = SetSharedBookmarks };
  TBookmarkList * GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList * value);

private:
  TStringList * FBookmarkLists;
  wstring FSharedKey;
  static wstring Keys[];

  void LoadLevel(THierarchicalStorage * Storage, const wstring Key,
    int Index, TBookmarkList * BookmarkList);
};
//---------------------------------------------------------------------------
class TBookmarkList : public TPersistent
{
friend class TBookmarks;
friend class TBookmark;
public:
  TBookmarkList();
  virtual ~TBookmarkList();

  void Clear();
  void Add(TBookmark * Bookmark);
  void Insert(int Index, TBookmark * Bookmark);
  void InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark);
  void MoveTo(TBookmark * ToBookmark, TBookmark * Bookmark, bool Before);
  void Delete(TBookmark * Bookmark);
  TBookmark * FindByName(const wstring Node, const wstring Name);
  TBookmark * FindByShortCut(TShortCut ShortCut);
  virtual void Assign(TPersistent * Source);
  void LoadOptions(THierarchicalStorage * Storage);
  void SaveOptions(THierarchicalStorage * Storage);
  void ShortCuts(TShortCuts & ShortCuts);

  // __property int Count = { read = GetCount };
  int GetCount();
  // __property TBookmark * Bookmarks[int Index] = { read = GetBookmarks };
  TBookmark * GetBookmarks(int Index);
  // __property bool NodeOpened[wstring Index] = { read = GetNodeOpened, write = SetNodeOpened };
  bool GetNodeOpened(wstring Index);
  void SetNodeOpened(wstring Index, bool value);

protected:
  int IndexOf(TBookmark * Bookmark);
  void KeyChanged(int Index);

  // __property bool Modified = { read = FModified, write = FModified };
  bool GetModified() { return FModified; }
  void SetModified(bool value) { FModified = value; }

private:
  TStringList * FBookmarks;
  TStringList * FOpenedNodes;
  bool FModified;
};
//---------------------------------------------------------------------------
class TBookmark : public TPersistent
{
friend class TBookmarkList;
public:
  TBookmark();

  virtual void Assign(TPersistent * Source);

  // __property wstring Name = { read = FName, write = SetName };
  wstring GetName() { return FName; }
  void SetName(const wstring value);
  // __property wstring Local = { read = FLocal, write = SetLocal };
  wstring GetLocal() { return FLocal; }
  void SetLocal(const wstring value);
  // __property wstring Remote = { read = FRemote, write = SetRemote };
  wstring GetRemote() { return FRemote; }
  void SetRemote(const wstring value);
  // __property wstring Node = { read = FNode, write = SetNode };
  wstring GetNode() { return FNode; }
  void SetNode(const wstring value);
  // __property TShortCut ShortCut = { read = FShortCut, write = SetShortCut };
  TShortCut GetShortCut() { return FShortCut; }
  void SetShortCut(TShortCut value);

protected:
  TBookmarkList * FOwner;

  static wstring BookmarkKey(const wstring Node, const wstring Name);
  // __property wstring Key = { read = GetKey };
  wstring GetKey();

private:
  wstring FName;
  wstring FLocal;
  wstring FRemote;
  wstring FNode;
  TShortCut FShortCut;

  void Modify(int OldIndex);
};
//---------------------------------------------------------------------------
#endif
