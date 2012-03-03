//---------------------------------------------------------------------------
#ifndef RemoteFilesH
#define RemoteFilesH
//---------------------------------------------------------------------------

#include <vector>
#include <map>

#include "Classes.h"
//---------------------------------------------------------------------------
enum TModificationFmt { mfNone, mfMDHM, mfMDY, mfFull };
//---------------------------------------------------------------------------
#define SYMLINKSTR L" -> "
#define PARENTDIRECTORY L".."
#define THISDIRECTORY L"."
#define ROOTDIRECTORY L"/"
#define FILETYPE_SYMLINK L'L'
#define FILETYPE_DIRECTORY L'D'
#define PARTIAL_EXT L".filepart"
//---------------------------------------------------------------------------
class TTerminal;
class TRights;
class TRemoteFileList;
class THierarchicalStorage;

//---------------------------------------------------------------------------
class TRemoteToken
{
public:
    TRemoteToken();
    explicit TRemoteToken(const std::wstring Name);

    void Clear();

    bool operator ==(const TRemoteToken &rht) const;
    bool operator !=(const TRemoteToken &rht) const;
    TRemoteToken &operator =(const TRemoteToken &rht);

    int Compare(const TRemoteToken &rht) const;

    std::wstring GetName() const { return FName; }
    void SetName(const std::wstring value) { FName = value; }
    bool GetNameValid() const;
    size_t GetID() const { return FID; }
    void SetID(size_t value);
    bool GetIDValid() const { return FIDValid; }
    bool GetIsSet() const;
    std::wstring GetLogText() const;
    std::wstring GetDisplayText() const;

private:
    std::wstring FName;
    size_t FID;
    bool FIDValid;
};
//---------------------------------------------------------------------------
class TRemoteTokenList
{
public:
    TRemoteTokenList *Duplicate() const;
    void Clear();
    void Add(const TRemoteToken &Token);
    void AddUnique(const TRemoteToken &Token);
    bool Exists(const std::wstring Name) const;
    const TRemoteToken *Find(unsigned int ID) const;
    const TRemoteToken *Find(const std::wstring Name) const;
    void Log(TTerminal *Terminal, const wchar_t *Title);

    size_t GetCount() const;
    const TRemoteToken *GetToken(size_t Index) const;

private:
    typedef std::vector<TRemoteToken> TTokens;
    typedef std::map<std::wstring, size_t> TNameMap;
    typedef std::map<size_t, size_t> TIDMap;
    TTokens FTokens;
    TNameMap FNameMap;
    TIDMap FIDMap;
};
//---------------------------------------------------------------------------
class TRemoteFile : public nb::TPersistent
{
private:
    TRemoteFileList *FDirectory;
    TRemoteToken FOwner;
    TModificationFmt FModificationFmt;
    __int64 FSize;
    std::wstring FFileName;
    int FINodeBlocks;
    nb::TDateTime FModification;
    nb::TDateTime FLastAccess;
    TRemoteToken FGroup;
    int FIconIndex;
    bool FIsSymLink;
    TRemoteFile *FLinkedFile;
    TRemoteFile *FLinkedByFile;
    std::wstring FLinkTo;
    TRights *FRights;
    TTerminal *FTerminal;
    char FType;
    bool FSelected;
    bool FCyclicLink;
    std::wstring FFullFileName;
    int FIsHidden;
    std::wstring FTypeName;
    TRemoteFile *Self;

    void LoadTypeInfo();

protected:
    void FindLinkedFile();

public:
    explicit TRemoteFile(TRemoteFile *ALinkedByFile = NULL);
    virtual ~TRemoteFile();
    TRemoteFile *Duplicate(bool Standalone = true) const;

    void ShiftTime(const nb::TDateTime &Difference);
    void Complete();

    int GetAttr();
    bool GetBrokenLink();
    TRemoteFileList *GetDirectory() const { return FDirectory; }
    void SetDirectory(TRemoteFileList *value) { FDirectory = value; }
    std::wstring GetRightsStr();
    __int64 GetSize() const { return FSize; }
    void SetSize(__int64 value) { FSize = value; }
    TRemoteToken GetOwner() const { return FOwner; }
    void SetOwner(TRemoteToken value) { FOwner = value; }
    TRemoteToken GetGroup() const { return FGroup; }
    void SetGroup(TRemoteToken value) { FGroup = value; }
    std::wstring GetFileName() const { return FFileName; }
    void SetFileName(const std::wstring value) { FFileName = value; }
    int GetINodeBlocks() { return FINodeBlocks; };
    nb::TDateTime GetModification() const { return FModification; }
    void SetModification(const nb::TDateTime &value);
    std::wstring GetModificationStr();
    std::wstring GetUserModificationStr();
    TModificationFmt GetModificationFmt() const { return FModificationFmt; }
    void SetModificationFmt(TModificationFmt value) { FModificationFmt = value; }
    nb::TDateTime GetLastAccess() const { return FLastAccess; }
    void SetLastAccess(nb::TDateTime value) { FLastAccess = value; }
    bool GetIsSymLink() const { return FIsSymLink; }
    bool GetIsDirectory() const;
    TRemoteFile *GetLinkedFile();
    void SetLinkedFile(TRemoteFile *value);
    std::wstring GetLinkTo() const { return FLinkTo; }
    void SetLinkTo(const std::wstring value) { FLinkTo = value; }
    std::wstring GetListingStr();
    void SetListingStr(const std::wstring value, bool Utf);
    TRights *GetRights() const { return FRights; }
    void SetRights(TRights *value);
    TTerminal *GetTerminal() const { return FTerminal; }
    void SetTerminal(TTerminal *value);
    char GetType() const;
    void SetType(char AType);
    bool GetSelected() { return FSelected; }
    void SetSelected(bool value) { FSelected = value; }
    std::wstring GetFullFileName() const;
    void SetFullFileName(const std::wstring value) { FFullFileName = value; }
    bool GetHaveFullFileName() const;
    int GetIconIndex() const;
    std::wstring GetTypeName();
    bool GetIsHidden();
    void SetIsHidden(bool value);
    bool GetIsParentDirectory() const;
    bool GetIsThisDirectory() const;
    bool GetIsInaccesibleDirectory() const;
    std::wstring GetExtension();
};
//---------------------------------------------------------------------------
class TRemoteDirectoryFile : public TRemoteFile
{
public:
    TRemoteDirectoryFile();
    virtual ~TRemoteDirectoryFile()
    {}
};
//---------------------------------------------------------------------------
class TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
    explicit TRemoteParentDirectory(TTerminal *Terminal);
    virtual ~TRemoteParentDirectory()
    {}
};
//---------------------------------------------------------------------------
class TRemoteFileList : public nb::TObjectList
{
    friend class TSCPFileSystem;
    friend class TSFTPFileSystem;
    friend class TFTPFileSystem;
    friend class TWebDAVFileSystem;
protected:
    std::wstring FDirectory;
    nb::TDateTime FTimestamp;
    TRemoteFile *GetParentDirectory();

    virtual void Clear();
public:
    TRemoteFileList();
    virtual ~TRemoteFileList()
    {
        Clear();
    }
    TRemoteFile *FindFile(const std::wstring FileName);
    virtual void DuplicateTo(TRemoteFileList *Copy);
    virtual void AddFile(TRemoteFile *File);
    std::wstring GetDirectory() { return FDirectory; }
    virtual void SetDirectory(const std::wstring value);
    TRemoteFile *GetFile(size_t Index);
    std::wstring GetFullDirectory();
    bool GetIsRoot();
    std::wstring GetParentPath();
    __int64 GetTotalSize();
    nb::TDateTime GetTimestamp() { return FTimestamp; }
};
//---------------------------------------------------------------------------
class TRemoteDirectory : public TRemoteFileList
{
    friend class TSCPFileSystem;
    friend class TSFTPFileSystem;
    friend class TWebDAVFileSystem;
private:
    bool FIncludeParentDirectory;
    bool FIncludeThisDirectory;
    TTerminal *FTerminal;
    nb::TStrings *FSelectedFiles;
    TRemoteFile *FParentDirectory;
    TRemoteFile *FThisDirectory;
protected:
    virtual void Clear();
public:
    explicit TRemoteDirectory(TTerminal *aTerminal, TRemoteDirectory *Template = NULL);
    virtual ~TRemoteDirectory()
    {
        Clear();
    }
    virtual void AddFile(TRemoteFile *File);
    virtual void SetDirectory(const std::wstring value);
    virtual void DuplicateTo(TRemoteFileList *Copy);
    TTerminal *GetTerminal() { return FTerminal; }
    void SetTerminal(TTerminal *value) { FTerminal = value; }
    nb::TStrings *GetSelectedFiles();
    bool GetIncludeParentDirectory() { return FIncludeParentDirectory; }
    void SetIncludeParentDirectory(bool value);
    bool GetIncludeThisDirectory() { return FIncludeThisDirectory; }
    void SetIncludeThisDirectory(bool value);
    bool GetLoaded();
    TRemoteFile *GetParentDirectory() { return FParentDirectory; }
    TRemoteFile *GetThisDirectory() { return FThisDirectory; }
};
//---------------------------------------------------------------------------
class TCriticalSection;
class TRemoteDirectoryCache : private nb::TStringList
{
public:
    TRemoteDirectoryCache();
    virtual ~TRemoteDirectoryCache();
    bool HasFileList(const std::wstring Directory);
    bool HasNewerFileList(const std::wstring Directory, nb::TDateTime Timestamp);
    bool GetFileList(const std::wstring Directory,
                     TRemoteFileList *FileList);
    void AddFileList(TRemoteFileList *FileList);
    void ClearFileList(const std::wstring Directory, bool SubDirs);
    void Clear();

    bool GetIsEmpty() const;
protected:
    virtual void Delete(size_t Index);
private:
    TCriticalSection *FSection;
    TRemoteDirectoryCache *Self;
    void DoClearFileList(const std::wstring Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private nb::TStringList
{
public:
    explicit TRemoteDirectoryChangesCache(int MaxSize);
    virtual ~TRemoteDirectoryChangesCache()
    {}

    void AddDirectoryChange(const std::wstring SourceDir,
                            const std::wstring Change, const std::wstring TargetDir);
    void ClearDirectoryChange(const std::wstring SourceDir);
    void ClearDirectoryChangeTarget(const std::wstring TargetDir);
    bool GetDirectoryChange(const std::wstring SourceDir,
                            const std::wstring Change, std::wstring &TargetDir);
    void Clear();

    void Serialize(std::wstring &Data);
    void Deserialize(const std::wstring Data);

    bool GetIsEmpty() const;

private:
    static bool DirectoryChangeKey(const std::wstring SourceDir,
                                   const std::wstring Change, std::wstring &Key);
    void SetValue(const std::wstring Name, const std::wstring Value);
    std::wstring GetValue(const std::wstring Name);

    size_t FMaxSize;
};
//---------------------------------------------------------------------------
class TRights
{
public:
    static const int TextLen = 9;
    static const char UndefSymbol = '$';
    static const char UnsetSymbol = '-';
    static const char BasicSymbols[];
    static const char CombinedSymbols[];
    static const char ExtendedSymbols[];
    static const char ModeGroups[];
    enum TRight
    {
        rrUserIDExec, rrGroupIDExec, rrStickyBit,
        rrUserRead, rrUserWrite, rrUserExec,
        rrGroupRead, rrGroupWrite, rrGroupExec,
        rrOtherRead, rrOtherWrite, rrOtherExec,
        rrFirst = rrUserIDExec, rrLast = rrOtherExec
    };
    enum TFlag
    {
        rfSetUID =    04000, rfSetGID =      02000, rfStickyBit = 01000,
        rfUserRead =  00400, rfUserWrite =   00200, rfUserExec =  00100,
        rfGroupRead = 00040, rfGroupWrite =  00020, rfGroupExec = 00010,
        rfOtherRead = 00004, rfOtherWrite =  00002, rfOtherExec = 00001,
        rfRead =      00444, rfWrite =       00222, rfExec =      00111,
        rfNo =        00000, rfDefault =     00644, rfAll =       00777,
        rfSpecials =  07000, rfAllSpecials = 07777
    };
    enum TUnsupportedFlag
    {
        rfDirectory  = 040000
    };
    enum TState { rsNo, rsYes, rsUndef };

public:
    static TFlag RightToFlag(TRight Right);

    TRights();
    TRights(const TRights &Source);
    explicit TRights(unsigned short Number);

    void Assign(const TRights *Source);
    void AddExecute();
    void AllUndef();

    bool operator ==(const TRights &rhr) const;
    bool operator ==(unsigned short rhr) const;
    bool operator !=(const TRights &rhr) const;
    TRights &operator =(const TRights &rhr);
    TRights &operator =(unsigned short rhr);
    TRights operator ~() const;
    TRights operator &(unsigned short rhr) const;
    TRights operator &(const TRights &rhr) const;
    TRights &operator &=(unsigned short rhr);
    TRights &operator &=(const TRights &rhr);
    TRights operator |(unsigned short rhr) const;
    TRights operator |(const TRights &rhr) const;
    TRights &operator |=(unsigned short rhr);
    TRights &operator |=(const TRights &rhr);
    operator unsigned short() const;
    operator unsigned long() const;

    bool GetAllowUndef() const { return FAllowUndef; }
    void SetAllowUndef(bool value);
    bool GetIsUndef() const;
    std::wstring GetModeStr() const;
    std::wstring GetSimplestStr() const;
    std::wstring GetOctal() const;
    void SetOctal(const std::wstring value);
    unsigned short GetNumber() const;
    void SetNumber(unsigned short value);
    unsigned short GetNumberSet() const { return FSet; }
    unsigned short GetNumberUnset() const { return FUnset; }
    unsigned long GetNumberDecadic() const;
    bool GetReadOnly();
    void SetReadOnly(bool value);
    bool GetRight(TRight Right) const;
    void SetRight(TRight Right, bool value);
    TState GetRightUndef(TRight Right) const;
    void SetRightUndef(TRight Right, TState value);
    std::wstring GetText() const;
    void SetText(const std::wstring value);
    bool GetUnknown() { return FUnknown; }

private:
    bool FAllowUndef;
    unsigned short FSet;
    unsigned short FUnset;
    std::wstring FText;
    bool FUnknown;

    // unsigned short GetNumberSet() const;
    // unsigned short GetNumberUnset() const;
};
//---------------------------------------------------------------------------
enum TValidProperty
{
    vpRights = 0x1,
    vpGroup = 0x2,
    vpOwner = 0x4,
    vpModification = 0x8,
    vpLastAccess = 0x10,
};
// FIXME
// typedef Set<TValidProperty, vpRights, vpLastAccess> TValidProperties;
struct TValidProperties
{
public:
    TValidProperties() :
        FValue(0)
    {
    }
    void Clear()
    {
        FValue = 0;
    }
    bool Contains(TValidProperty value) const
    {
        return (FValue & value) != 0;
    }
    bool operator == (const TValidProperties &rhs) const
    {
        return FValue == rhs.FValue;
    }
    bool operator != (const TValidProperties &rhs) const
    {
        return !(operator == (rhs));
    }
    TValidProperties &operator << (const TValidProperty value)
    {
        FValue |= value;
        return *this;
    }
    TValidProperties &operator >> (const TValidProperty value)
    {
        FValue &= ~(static_cast<__int64>(value));
        return *this;
    }
    bool Empty() const
    {
        return FValue == 0;
    }
private:
    __int64 FValue;
};


class TRemoteProperties
{
public:
    TValidProperties Valid;
    bool Recursive;
    TRights Rights;
    bool AddXToDirectories;
    TRemoteToken Group;
    TRemoteToken Owner;
    __int64 Modification; // unix time
    __int64 LastAccess; // unix time

    TRemoteProperties();
    TRemoteProperties(const TRemoteProperties &rhp);
    bool operator ==(const TRemoteProperties &rhp) const;
    bool operator !=(const TRemoteProperties &rhp) const;
    void Default();
    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage) const;

    static TRemoteProperties CommonProperties(nb::TStrings *FileList);
    static TRemoteProperties ChangedProperties(
        const TRemoteProperties &OriginalProperties, TRemoteProperties NewProperties);
};
//---------------------------------------------------------------------------
std::wstring UnixIncludeTrailingBackslash(const std::wstring Path);
std::wstring UnixExcludeTrailingBackslash(const std::wstring Path);
std::wstring UnixExtractFileDir(const std::wstring Path);
std::wstring UnixExtractFilePath(const std::wstring Path);
std::wstring UnixExtractFileName(const std::wstring Path);
std::wstring UnixExtractFileExt(const std::wstring Path);
bool UnixComparePaths(const std::wstring Path1, const std::wstring Path2);
bool UnixIsChildPath(const std::wstring Parent, const std::wstring Child);
bool ExtractCommonPath(nb::TStrings *Files, std::wstring &Path);
bool UnixExtractCommonPath(nb::TStrings *Files, std::wstring &Path);
std::wstring ExtractFileName(const std::wstring Path, bool Unix);
bool IsUnixRootPath(const std::wstring Path);
bool IsUnixHiddenFile(const std::wstring Path);
std::wstring AbsolutePath(const std::wstring Base, const std::wstring Path);
std::wstring FromUnixPath(const std::wstring Path);
std::wstring ToUnixPath(const std::wstring Path);
std::wstring MinimizeName(const std::wstring FileName, size_t MaxLen, bool Unix);
std::wstring MakeFileList(nb::TStrings *FileList);
nb::TDateTime ReduceDateTimePrecision(nb::TDateTime DateTime,
                                      TModificationFmt Precision);
TModificationFmt LessDateTimePrecision(
    TModificationFmt Precision1, TModificationFmt Precision2);
std::wstring UserModificationStr(nb::TDateTime DateTime,
                                 TModificationFmt Precision);
int FakeFileImageIndex(const std::wstring FileName, unsigned long Attrs = 0,
                       std::wstring *TypeName = NULL);
//---------------------------------------------------------------------------
#endif
