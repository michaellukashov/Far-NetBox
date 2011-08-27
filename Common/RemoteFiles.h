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
  explicit TRemoteToken(const std::wstring & Name);

  void Clear();

  bool operator ==(const TRemoteToken & rht) const;
  bool operator !=(const TRemoteToken & rht) const;
  TRemoteToken & operator =(const TRemoteToken & rht);

  int Compare(const TRemoteToken & rht) const;

  // __property std::wstring Name = { read = FName, write = FName };
  std::wstring GetName() const { return FName; }
  void SetName(std::wstring value) { FName = value; }
  // __property bool NameValid = { read = GetNameValid };
  bool GetNameValid() const;
  // __property unsigned int ID = { read = FID, write = SetID };
  unsigned int GetID() const { return FID; }
  void SetID(unsigned int value);
  // __property bool IDValid = { read = FIDValid };
  bool GetIDValid() const { return FIDValid; }
  // __property bool IsSet  = { read = GetIsSet };
  bool GetIsSet() const;
  // __property std::wstring LogText = { read = GetLogText };
   std::wstring GetLogText() const;
  // __property std::wstring DisplayText = { read = GetDisplayText };
  std::wstring GetDisplayText() const;

private:
  std::wstring FName;
  unsigned int FID;
  bool FIDValid;
};
//---------------------------------------------------------------------------
class TRemoteTokenList
{
public:
  TRemoteTokenList * Duplicate() const;
  void Clear();
  void Add(const TRemoteToken & Token);
  void AddUnique(const TRemoteToken & Token);
  bool Exists(const std::wstring & Name) const;
  const TRemoteToken * Find(unsigned int ID) const;
  const TRemoteToken * Find(const std::wstring & Name) const;
  void Log(TTerminal * Terminal, const char * Title);

  int Count() const;
  const TRemoteToken * Token(int Index) const;

private:
  typedef std::vector<TRemoteToken> TTokens;
  typedef std::map<std::wstring, size_t> TNameMap;
  typedef std::map<unsigned int, size_t> TIDMap;
  TTokens FTokens;
  TNameMap FNameMap;
  TIDMap FIDMap;
};
//---------------------------------------------------------------------------
class TRemoteFile : public TPersistent
{
private:
  TRemoteFileList * FDirectory;
  TRemoteToken FOwner;
  TModificationFmt FModificationFmt;
  __int64 FSize;
  std::wstring FFileName;
  int FINodeBlocks;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  int FIconIndex;
  bool FIsSymLink;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  std::wstring FLinkTo;
  TRights *FRights;
  TTerminal *FTerminal;
  char FType;
  bool FSelected;
  bool FCyclicLink;
  std::wstring FFullFileName;
  int FIsHidden;
  std::wstring FTypeName;
  void LoadTypeInfo();

protected:
  void FindLinkedFile();

public:
  TRemoteFile(TRemoteFile * ALinkedByFile = NULL);
  virtual ~TRemoteFile();
  TRemoteFile * Duplicate(bool Standalone = true) const;

  void ShiftTime(const TDateTime & Difference);
  void Complete();

  // __property int Attr = { read = GetAttr };
  int GetAttr();
  // __property bool BrokenLink = { read = GetBrokenLink };
  bool GetBrokenLink();
  // __property TRemoteFileList * Directory = { read = FDirectory, write = FDirectory };
  TRemoteFileList *GetDirectory() const { return FDirectory; }
  void SetDirectory(TRemoteFileList *value) { FDirectory = value; }
  // __property std::wstring RightsStr = { read = GetRightsStr };
  std::wstring GetRightsStr();
  // __property __int64 Size = { read = FSize, write = FSize };
  __int64 GetSize() const { return FSize; }
  void SetSize(__int64 value) { FSize = value; }
  // __property TRemoteToken Owner = { read = FOwner, write = FOwner };
  TRemoteToken GetOwner() const { return FOwner; }
  void SetOwner(TRemoteToken value) { FOwner = value; }
  // __property TRemoteToken Group = { read = FGroup, write = FGroup };
  TRemoteToken GetGroup() const { return FGroup; }
  void SetGroup(TRemoteToken value) { FGroup = value; }
  // __property std::wstring FileName = { read = FFileName, write = FFileName };
  std::wstring GetFileName() const { return FFileName; }
  void SetFileName(std::wstring value) { FFileName = value; }
  // __property int INodeBlocks = { read = FINodeBlocks };
  int GetINodeBlocks() { return FINodeBlocks; };
  // __property TDateTime Modification = { read = FModification, write = SetModification };
  TDateTime GetModification() const { return FModification; }
  void SetModification(const TDateTime & value);
  // __property std::wstring ModificationStr = { read = GetModificationStr };
  std::wstring GetModificationStr();
  // __property std::wstring UserModificationStr = { read = GetUserModificationStr };
  std::wstring GetUserModificationStr();
  // __property TModificationFmt ModificationFmt = { read = FModificationFmt, write = FModificationFmt };
  TModificationFmt GetModificationFmt() const { return FModificationFmt; }
  void SetModificationFmt(TModificationFmt value) { FModificationFmt = value; }
  // __property TDateTime LastAccess = { read = FLastAccess, write = FLastAccess };
  TDateTime GetLastAccess() { return FLastAccess; }
  void SetLastAccess(TDateTime value) { FLastAccess = value; }
  // __property bool IsSymLink = { read = FIsSymLink };
  bool GetIsSymLink() const { return FIsSymLink; }
  // __property bool IsDirectory = { read = GetIsDirectory };
  bool GetIsDirectory() const;
  // __property TRemoteFile * LinkedFile = { read = GetLinkedFile, write = SetLinkedFile };
  TRemoteFile * GetLinkedFile();
  void SetLinkedFile(TRemoteFile * value);
  // __property std::wstring LinkTo = { read = FLinkTo, write = FLinkTo };
  std::wstring GetLinkTo() const { return FLinkTo; }
  void SetLinkTo(std::wstring value) { FLinkTo = value; }
  // __property std::wstring ListingStr = { read = GetListingStr, write = SetListingStr };
  std::wstring GetListingStr();
  void SetListingStr(std::wstring value);
  // __property TRights * Rights = { read = FRights, write = SetRights };
  TRights *GetRights() const { return FRights; }
  void SetRights(TRights * value);
  // __property TTerminal * Terminal = { read = FTerminal, write = SetTerminal };
  TTerminal * GetTerminal() const { return FTerminal; }
  void SetTerminal(TTerminal * value);
  // __property char Type = { read = GetType, write = SetType };
  char GetType() const;
  void SetType(char AType);
  // __property bool Selected  = { read=FSelected, write=FSelected };
  bool GetSelected() { return FSelected; }
  void SetSelected(bool value) { FSelected = value; }
  // __property std::wstring FullFileName  = { read = GetFullFileName, write = FFullFileName };
  std::wstring GetFullFileName() const;
  void SetFullFileName(std::wstring value) { FFullFileName = value; }
  // __property bool HaveFullFileName  = { read = GetHaveFullFileName };
  bool GetHaveFullFileName() const;
  // __property int IconIndex = { read = GetIconIndex };
  int GetIconIndex() const;
  // __property std::wstring TypeName = { read = GetTypeName };
  std::wstring GetTypeName();
  // __property bool IsHidden = { read = GetIsHidden, write = SetIsHidden };
  bool GetIsHidden();
  void SetIsHidden(bool value);
  // __property bool IsParentDirectory = { read = GetIsParentDirectory };
  bool GetIsParentDirectory() const;
  // __property bool IsThisDirectory = { read = GetIsThisDirectory };
  bool GetIsThisDirectory() const;
  // __property bool IsInaccesibleDirectory  = { read=GetIsInaccesibleDirectory };
  bool GetIsInaccesibleDirectory() const;
  // __property std::wstring Extension  = { read=GetExtension };
  std::wstring GetExtension();
};
//---------------------------------------------------------------------------
class TRemoteDirectoryFile : public TRemoteFile
{
public:
  TRemoteDirectoryFile();
};
//---------------------------------------------------------------------------
class TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  TRemoteParentDirectory(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TRemoteFileList : public TObjectList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
protected:
  std::wstring FDirectory;
  TDateTime FTimestamp;
  TRemoteFile * GetParentDirectory();

  virtual void Clear();
public:
  TRemoteFileList();
  TRemoteFile * FindFile(const std::wstring &FileName);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  virtual void AddFile(TRemoteFile * File);
  // __property std::wstring Directory = { read = FDirectory, write = SetDirectory };
  std::wstring GetDirectory() { return FDirectory; }
  virtual void SetDirectory(std::wstring value);
  // __property TRemoteFile * Files[int Index] = { read = GetFiles };
  TRemoteFile *GetFile(int Index);
  // __property std::wstring FullDirectory  = { read=GetFullDirectory };
  std::wstring GetFullDirectory();
  // __property bool IsRoot = { read = GetIsRoot };
  bool GetIsRoot();
  // __property std::wstring ParentPath = { read = GetParentPath };
  std::wstring GetParentPath();
  // __property __int64 TotalSize = { read = GetTotalSize };
  __int64 GetTotalSize();
  // __property TDateTime Timestamp = { read = FTimestamp };
  TDateTime GetTimestamp() { return FTimestamp; }
};
//---------------------------------------------------------------------------
class TRemoteDirectory : public TRemoteFileList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
private:
  bool FIncludeParentDirectory;
  bool FIncludeThisDirectory;
  TTerminal * FTerminal;
  TStrings * FSelectedFiles;
  TRemoteFile * FParentDirectory;
  TRemoteFile * FThisDirectory;
protected:
  virtual void Clear();
public:
  TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = NULL);
  virtual void AddFile(TRemoteFile * File);
  virtual void SetDirectory(std::wstring value);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  // __property TTerminal * Terminal = { read = FTerminal, write = FTerminal };
  TTerminal *GetTerminal() { return FTerminal; }
  void SetTerminal(TTerminal *value) { FTerminal = value; }
  // __property TStrings * SelectedFiles  = { read=GetSelectedFiles };
  TStrings * GetSelectedFiles();
  // __property bool IncludeParentDirectory = { read = FIncludeParentDirectory, write = SetIncludeParentDirectory };
  bool GetIncludeParentDirectory() { return FIncludeParentDirectory; }
  void SetIncludeParentDirectory(bool value);
  // __property bool IncludeThisDirectory = { read = FIncludeThisDirectory, write = SetIncludeThisDirectory };
  bool GetIncludeThisDirectory() { return FIncludeThisDirectory; }
  void SetIncludeThisDirectory(bool value);
  // __property bool Loaded = { read = GetLoaded };
  bool GetLoaded();
  // __property TRemoteFile * ParentDirectory = { read = FParentDirectory };
  TRemoteFile *GetParentDirectory() { return FParentDirectory; }
  // __property TRemoteFile * ThisDirectory = { read = FThisDirectory };
  TRemoteFile *GetThisDirectory() { return FThisDirectory; }
};
//---------------------------------------------------------------------------
class TCriticalSection;
class TRemoteDirectoryCache : private TStringList
{
public:
  TRemoteDirectoryCache();
  virtual ~TRemoteDirectoryCache();
  bool HasFileList(const std::wstring Directory);
  bool HasNewerFileList(const std::wstring Directory, TDateTime Timestamp);
  bool GetFileList(const std::wstring Directory,
    TRemoteFileList * FileList);
  void AddFileList(TRemoteFileList * FileList);
  void ClearFileList(std::wstring Directory, bool SubDirs);
  void Clear();

  // __property bool IsEmpty = { read = GetIsEmpty };
  bool GetIsEmpty() const;
protected:
  virtual void Delete(int Index);
private:
  TCriticalSection * FSection;
  void DoClearFileList(std::wstring Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private TStringList
{
public:
  TRemoteDirectoryChangesCache(int MaxSize);

  void AddDirectoryChange(const std::wstring SourceDir,
    const std::wstring Change, const std::wstring TargetDir);
  void ClearDirectoryChange(std::wstring SourceDir);
  void ClearDirectoryChangeTarget(std::wstring TargetDir);
  bool GetDirectoryChange(const std::wstring SourceDir,
    const std::wstring Change, std::wstring & TargetDir);
  void Clear();

  void Serialize(std::wstring & Data);
  void Deserialize(const std::wstring Data);

  // __property bool IsEmpty = { read = GetIsEmpty };
  bool GetIsEmpty() const;

private:
  static bool DirectoryChangeKey(const std::wstring SourceDir,
    const std::wstring Change, std::wstring & Key);
  void SetValue(const std::wstring & Name, const std::wstring & Value);
  std::wstring GetValue(const std::wstring & Name);

  int FMaxSize;
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
  enum TRight {
    rrUserIDExec, rrGroupIDExec, rrStickyBit,
    rrUserRead, rrUserWrite, rrUserExec,
    rrGroupRead, rrGroupWrite, rrGroupExec,
    rrOtherRead, rrOtherWrite, rrOtherExec,
    rrFirst = rrUserIDExec, rrLast = rrOtherExec };
  enum TFlag {
    rfSetUID =    04000, rfSetGID =      02000, rfStickyBit = 01000,
    rfUserRead =  00400, rfUserWrite =   00200, rfUserExec =  00100,
    rfGroupRead = 00040, rfGroupWrite =  00020, rfGroupExec = 00010,
    rfOtherRead = 00004, rfOtherWrite =  00002, rfOtherExec = 00001,
    rfRead =      00444, rfWrite =       00222, rfExec =      00111,
    rfNo =        00000, rfDefault =     00644, rfAll =       00777,
    rfSpecials =  07000, rfAllSpecials = 07777 };
  enum TUnsupportedFlag {
    rfDirectory  = 040000 };
  enum TState { rsNo, rsYes, rsUndef };

public:
  static TFlag RightToFlag(TRight Right);

  TRights();
  TRights(const TRights & Source);
  TRights(unsigned short Number);

  void Assign(const TRights * Source);
  void AddExecute();
  void AllUndef();

  bool operator ==(const TRights & rhr) const;
  bool operator ==(unsigned short rhr) const;
  bool operator !=(const TRights & rhr) const;
  TRights & operator =(const TRights & rhr);
  TRights & operator =(unsigned short rhr);
  TRights operator ~() const;
  TRights operator &(unsigned short rhr) const;
  TRights operator &(const TRights & rhr) const;
  TRights & operator &=(unsigned short rhr);
  TRights & operator &=(const TRights & rhr);
  TRights operator |(unsigned short rhr) const;
  TRights operator |(const TRights & rhr) const;
  TRights & operator |=(unsigned short rhr);
  TRights & operator |=(const TRights & rhr);
  operator unsigned short() const;
  operator unsigned long() const;

  // __property bool AllowUndef = { read = FAllowUndef, write = SetAllowUndef };
  bool GetAllowUndef() const { return FAllowUndef; }
  void SetAllowUndef(bool value);
  // __property bool IsUndef = { read = GetIsUndef };
  bool GetIsUndef() const;
  // __property std::wstring ModeStr = { read = GetModeStr };
  std::wstring GetModeStr() const;
  // __property std::wstring SimplestStr = { read = GetSimplestStr };
  std::wstring GetSimplestStr() const;
  // __property std::wstring Octal = { read = GetOctal, write = SetOctal };
  std::wstring GetOctal() const;
  void SetOctal(std::wstring value);
  // __property unsigned short Number = { read = GetNumber, write = SetNumber };
  unsigned short GetNumber() const;
  void SetNumber(unsigned short value);
  // __property unsigned short NumberSet = { read = FSet };
  unsigned short GetNumberSet() const { return FSet; }
  // __property unsigned short NumberUnset = { read = FUnset };
  unsigned short GetNumberUnset() const { return FUnset; }
  // __property unsigned long NumberDecadic = { read = GetNumberDecadic };
  unsigned long GetNumberDecadic() const;
  // __property bool ReadOnly = { read = GetReadOnly, write = SetReadOnly };
  bool GetReadOnly();
  void SetReadOnly(bool value);
  // __property bool Right[TRight Right] = { read = GetRight, write = SetRight };
  bool GetRight(TRight Right) const;
  void SetRight(TRight Right, bool value);
  // __property TState RightUndef[TRight Right] = { read = GetRightUndef, write = SetRightUndef };
  TState GetRightUndef(TRight Right) const;
  void SetRightUndef(TRight Right, TState value);
  // __property std::wstring Text = { read = GetText, write = SetText };
  std::wstring GetText() const;
  void SetText(const std::wstring & value);
  // __property bool Unknown = { read = FUnknown };
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
enum TValidProperty { vpRights, vpGroup, vpOwner, vpModification, vpLastAccess };
// FIXME
// typedef Set<TValidProperty, vpRights, vpLastAccess> TValidProperties;
struct TValidProperties
{
public:
    void Clear();
    bool Contains(TValidProperty value) const
    {
        return false;
    }
    bool operator == (const TValidProperties &rhs) const
    {
        return false;
    }
    bool operator != (const TValidProperties &rhs) const
    {
        return !(operator == (rhs));
    }
    TValidProperties & operator << (const TValidProperty value)
    {
        return *this;
    }
    TValidProperties & operator >> (const TValidProperty value)
    {
        return *this;
    }
    bool Empty() const
    {
        return true;
    }
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
  TRemoteProperties(const TRemoteProperties & rhp);
  bool operator ==(const TRemoteProperties & rhp) const;
  bool operator !=(const TRemoteProperties & rhp) const;
  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  static TRemoteProperties CommonProperties(TStrings * FileList);
  static TRemoteProperties ChangedProperties(
    const TRemoteProperties & OriginalProperties, TRemoteProperties NewProperties);
};
//---------------------------------------------------------------------------
std::wstring UnixIncludeTrailingBackslash(const std::wstring Path);
std::wstring UnixExcludeTrailingBackslash(const std::wstring Path);
std::wstring UnixExtractFileDir(const std::wstring Path);
std::wstring UnixExtractFilePath(const std::wstring Path);
std::wstring UnixExtractFileName(const std::wstring Path);
std::wstring UnixExtractFileExt(const std::wstring Path);
bool UnixComparePaths(const std::wstring Path1, const std::wstring Path2);
bool UnixIsChildPath(std::wstring Parent, std::wstring Child);
bool ExtractCommonPath(TStrings * Files, std::wstring & Path);
bool UnixExtractCommonPath(TStrings * Files, std::wstring & Path);
std::wstring ExtractFileName(const std::wstring & Path, bool Unix);
bool IsUnixRootPath(const std::wstring Path);
bool IsUnixHiddenFile(const std::wstring Path);
std::wstring AbsolutePath(const std::wstring & Base, const std::wstring & Path);
std::wstring FromUnixPath(const std::wstring Path);
std::wstring ToUnixPath(const std::wstring Path);
std::wstring MinimizeName(const std::wstring FileName, int MaxLen, bool Unix);
std::wstring MakeFileList(TStrings * FileList);
TDateTime ReduceDateTimePrecision(TDateTime DateTime,
  TModificationFmt Precision);
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
std::wstring UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision);
int FakeFileImageIndex(std::wstring FileName, unsigned long Attrs = 0,
  std::wstring * TypeName = NULL);
//---------------------------------------------------------------------------
#endif
