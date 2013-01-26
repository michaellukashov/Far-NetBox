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

  TBookmarkList * GetBookmarks(const UnicodeString & Index);
  void SetBookmarks(const UnicodeString & Index, TBookmarkList * Value);
  TBookmarkList * GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList * Value);

private:
  TStringList * FBookmarkLists;
  UnicodeString FSharedKey;
  static UnicodeString Keys[];

private:
  void LoadLevel(THierarchicalStorage * Storage, const UnicodeString & Key,
    intptr_t Index, TBookmarkList * BookmarkList);
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
  void Insert(intptr_t Index, TBookmark * Bookmark);
  void InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark);
  void MoveTo(TBookmark * ToBookmark, TBookmark * Bookmark, bool Before);
  void Delete(TBookmark * Bookmark);
  TBookmark * FindByName(const UnicodeString & Node, const UnicodeString & Name);
  TBookmark * FindByShortCut(TShortCut ShortCut);
  virtual void Assign(TPersistent * Source);
  void LoadOptions(THierarchicalStorage * Storage);
  void SaveOptions(THierarchicalStorage * Storage);
  void ShortCuts(TShortCuts & ShortCuts);

  intptr_t GetCount();
  TBookmark * GetBookmarks(intptr_t Index);
  bool GetNodeOpened(const UnicodeString & Index);
  void SetNodeOpened(const UnicodeString & Index, bool Value);

protected:
  intptr_t IndexOf(TBookmark * Bookmark);
  void KeyChanged(intptr_t Index);

  bool GetModified() { return FModified; }
  void SetModified(bool Value) { FModified = Value; }

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

  UnicodeString GetName() { return FName; }
  void SetName(const UnicodeString & Value);
  UnicodeString GetLocal() { return FLocal; }
  void SetLocal(const UnicodeString & Value);
  UnicodeString GetRemote() { return FRemote; }
  void SetRemote(const UnicodeString & Value);
  UnicodeString GetNode() { return FNode; }
  void SetNode(const UnicodeString & Value);
  TShortCut GetShortCut() { return FShortCut; }
  void SetShortCut(TShortCut Value);

protected:
  TBookmarkList * FOwner;

  static UnicodeString BookmarkKey(const UnicodeString & Node, const UnicodeString & Name);

private:
  UnicodeString FName;
  UnicodeString FLocal;
  UnicodeString FRemote;
  UnicodeString FNode;
  TShortCut FShortCut;

  UnicodeString GetKey();
  void Modify(intptr_t OldIndex);
};
//---------------------------------------------------------------------------
#endif
