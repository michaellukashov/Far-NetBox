//---------------------------------------------------------------------------
#ifndef BookmarksH
#define BookmarksH

#include "Classes.h"

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

    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage, bool All);
    void ModifyAll(bool Modify);
    void Clear();

    TBookmarkList *GetBookmark(const UnicodeString Index);
    void SetBookmark(const UnicodeString Index, TBookmarkList *value);
    TBookmarkList *GetSharedBookmarks();
    void SetSharedBookmarks(TBookmarkList *value);

private:
    TStringList *FBookmarkLists;
    UnicodeString FSharedKey;
    static UnicodeString Keys[];

    void LoadLevel(THierarchicalStorage *Storage, const UnicodeString Key,
                   int Index, TBookmarkList *BookmarkList);
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
    void Add(TBookmark *Bookmark);
    void Insert(size_t Index, TBookmark *Bookmark);
    void InsertBefore(TBookmark *BeforeBookmark, TBookmark *Bookmark);
    void MoveTo(TBookmark *ToBookmark, TBookmark *Bookmark, bool Before);
    void Delete(TBookmark *Bookmark);
    TBookmark *FindByName(const UnicodeString Node, const UnicodeString Name);
    TBookmark *FindByShortCut(TShortCut ShortCut);
    virtual void __fastcall Assign(TPersistent *Source);
    void LoadOptions(THierarchicalStorage *Storage);
    void SaveOptions(THierarchicalStorage *Storage);
    void ShortCuts(TShortCuts &ShortCuts);

    size_t GetCount();
    TBookmark *GetBookmark(size_t Index);
    bool GetNodeOpened(const UnicodeString Index);
    void SetNodeOpened(const UnicodeString Index, bool value);

protected:
    size_t IndexOf(TBookmark *Bookmark);
    void KeyChanged(size_t Index);

    bool GetModified() { return FModified; }
    void SetModified(bool value) { FModified = value; }

private:
    TStringList *FBookmarks;
    TStringList *FOpenedNodes;
    bool FModified;
};
//---------------------------------------------------------------------------
class TBookmark : public TPersistent
{
    friend class TBookmarkList;
public:
    TBookmark();

    virtual void __fastcall Assign(TPersistent *Source);

    UnicodeString GetName() { return FName; }
    void SetName(const UnicodeString value);
    UnicodeString GetLocal() { return FLocal; }
    void SetLocal(const UnicodeString value);
    UnicodeString GetRemote() { return FRemote; }
    void SetRemote(const UnicodeString value);
    UnicodeString GetNode() { return FNode; }
    void SetNode(const UnicodeString value);
    TShortCut GetShortCut() { return FShortCut; }
    void SetShortCut(TShortCut value);

protected:
    TBookmarkList *FOwner;

    static UnicodeString BookmarkKey(const UnicodeString Node, const UnicodeString Name);
    UnicodeString GetKey();

private:
    UnicodeString FName;
    UnicodeString FLocal;
    UnicodeString FRemote;
    UnicodeString FNode;
    TShortCut FShortCut;

    void Modify(size_t OldIndex);
};
//---------------------------------------------------------------------------
#endif
