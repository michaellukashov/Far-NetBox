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

  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage, bool All);
  void __fastcall ModifyAll(bool Modify);
  void __fastcall Clear();

  TBookmarkList * __fastcall GetBookmarks(const UnicodeString & Index);
  void __fastcall SetBookmarks(const UnicodeString & Index, TBookmarkList * Value);
  TBookmarkList * __fastcall GetSharedBookmarks();
  void __fastcall SetSharedBookmarks(TBookmarkList * Value);

private:
  TStringList * FBookmarkLists;
  UnicodeString FSharedKey;
  static UnicodeString Keys[];

private:
  void __fastcall LoadLevel(THierarchicalStorage * Storage, const UnicodeString & Key,
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

  void __fastcall Clear();
  void __fastcall Add(TBookmark * Bookmark);
  void __fastcall Insert(intptr_t Index, TBookmark * Bookmark);
  void __fastcall InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark);
  void __fastcall MoveTo(TBookmark * ToBookmark, TBookmark * Bookmark, bool Before);
  void __fastcall Delete(TBookmark * Bookmark);
  TBookmark * __fastcall FindByName(const UnicodeString & Node, const UnicodeString & Name);
  TBookmark * __fastcall FindByShortCut(TShortCut ShortCut);
  virtual void Assign(TPersistent * Source);
  void __fastcall LoadOptions(THierarchicalStorage * Storage);
  void __fastcall SaveOptions(THierarchicalStorage * Storage);
  void __fastcall ShortCuts(TShortCuts & ShortCuts);

  intptr_t __fastcall GetCount();
  TBookmark * __fastcall GetBookmarks(intptr_t Index);
  bool __fastcall GetNodeOpened(const UnicodeString & Index);
  void __fastcall SetNodeOpened(const UnicodeString & Index, bool Value);

protected:
  intptr_t __fastcall IndexOf(TBookmark * Bookmark);
  void __fastcall KeyChanged(intptr_t Index);

  bool __fastcall GetModified() { return FModified; }
  void __fastcall SetModified(bool Value) { FModified = Value; }

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

  UnicodeString __fastcall GetName() { return FName; }
  void __fastcall SetName(const UnicodeString & Value);
  UnicodeString __fastcall GetLocal() { return FLocal; }
  void __fastcall SetLocal(const UnicodeString & Value);
  UnicodeString __fastcall GetRemote() { return FRemote; }
  void __fastcall SetRemote(const UnicodeString & Value);
  UnicodeString __fastcall GetNode() { return FNode; }
  void __fastcall SetNode(const UnicodeString & Value);
  TShortCut __fastcall GetShortCut() { return FShortCut; }
  void __fastcall SetShortCut(TShortCut Value);

protected:
  TBookmarkList * FOwner;

  static UnicodeString __fastcall BookmarkKey(const UnicodeString & Node, const UnicodeString & Name);

private:
  UnicodeString FName;
  UnicodeString FLocal;
  UnicodeString FRemote;
  UnicodeString FNode;
  TShortCut FShortCut;

  UnicodeString __fastcall GetKey();
  void __fastcall Modify(intptr_t OldIndex);
};
//---------------------------------------------------------------------------
#endif
