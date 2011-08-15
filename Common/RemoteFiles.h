//---------------------------------------------------------------------------
#ifndef RemoteFilesH
#define RemoteFilesH
//---------------------------------------------------------------------------
#include <vector>
#include <map>
//---------------------------------------------------------------------------
enum TModificationFmt { mfNone, mfMDHM, mfMDY, mfFull };
//---------------------------------------------------------------------------
#define SYMLINKSTR " -> "
#define PARENTDIRECTORY ".."
#define THISDIRECTORY "."
#define ROOTDIRECTORY "/"
#define FILETYPE_SYMLINK 'L'
#define FILETYPE_DIRECTORY 'D'
#define PARTIAL_EXT ".filepart"
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
  explicit TRemoteToken(const wstring & Name);

  void Clear();

  bool operator ==(const TRemoteToken & rht) const;
  bool operator !=(const TRemoteToken & rht) const;
  TRemoteToken & operator =(const TRemoteToken & rht);

  int Compare(const TRemoteToken & rht) const;

  // __property wstring Name = { read = FName, write = FName };
  wstring GetName() { return FName; }
  void SetName(wstring value) { FName = value; }
  // __property bool NameValid = { read = GetNameValid };
  bool GetNameValid() const;
  // __property unsigned int ID = { read = FID, write = SetID };
  unsigned int GetID() { return FID; }
  void SetID(unsigned int value);
  // __property bool IDValid = { read = FIDValid };
  bool GetIDValid() { return FIDValid; }
  // __property bool IsSet  = { read = GetIsSet };
  bool GetIsSet() const;
  // __property wstring LogText = { read = GetLogText };
   wstring GetLogText() const;
  // __property wstring DisplayText = { read = GetDisplayText };
  wstring GetDisplayText() const;

private:
  wstring FName;
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
  bool Exists(const wstring & Name) const;
  const TRemoteToken * Find(unsigned int ID) const;
  const TRemoteToken * Find(const wstring & Name) const;
  void Log(TTerminal * Terminal, const char * Title);

  int Count() const;
  const TRemoteToken * Token(int Index) const;

private:
  typedef std::vector<TRemoteToken> TTokens;
  typedef std::map<wstring, size_t> TNameMap;
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
  wstring FFileName;
  int FINodeBlocks;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  int FIconIndex;
  bool FIsSymLink;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  wstring FLinkTo;
  TRights *FRights;
  TTerminal *FTerminal;
  char FType;
  bool FSelected;
  bool FCyclicLink;
  wstring FFullFileName;
  int FIsHidden;
  wstring FTypeName;
  void SetModification(const TDateTime & value);
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
  TRemoteFileList *GetDirectory() { return FDirectory; }
  void SetDirectory(TRemoteFileList *value) { FDirectory = value; }
  // __property wstring RightsStr = { read = GetRightsStr };
  wstring GetRightsStr();
  // __property __int64 Size = { read = FSize, write = FSize };
  __int64 GetSize() { return FSize; }
  void SetSize(__int64 value) { FSize = value; }
  // __property TRemoteToken Owner = { read = FOwner, write = FOwner };
  TRemoteToken GetOwner() { return FOwner; }
  void SetOwner(TRemoteToken value) { FOwner = value; }
  // __property TRemoteToken Group = { read = FGroup, write = FGroup };
  TRemoteToken GetGroup() { return FGroup; }
  void SetGroup(TRemoteToken value) { FGroup = value; }
  // __property wstring FileName = { read = FFileName, write = FFileName };
  wstring GetFileName() { return FFileName; }
  void SetFileName(wstring value) { FFileName = value; }
  // __property int INodeBlocks = { read = FINodeBlocks };
  int GetINodeBlocks() { return FINodeBlocks; };
  // __property TDateTime Modification = { read = FModification, write = SetModification };
  TDateTime GetModification() { return FModification; }
  void SetModification(TDateTime value) { FModification = value; }
  // __property wstring ModificationStr = { read = GetModificationStr };
  wstring GetModificationStr();
  // __property wstring UserModificationStr = { read = GetUserModificationStr };
  wstring GetUserModificationStr();
  // __property TModificationFmt ModificationFmt = { read = FModificationFmt, write = FModificationFmt };
  TModificationFmt GetModificationFmt() { return FModificationFmt; }
  void SetModificationFmt(TModificationFmt value) { FModificationFmt = value; }
  // __property TDateTime LastAccess = { read = FLastAccess, write = FLastAccess };
  TDateTime GetLastAccess() { return FLastAccess; }
  void SetLastAccess(TDateTime value) { FLastAccess = value; }
  // __property bool IsSymLink = { read = FIsSymLink };
  bool GetIsSymLink() { return FIsSymLink; }
  // __property bool IsDirectory = { read = GetIsDirectory };
  bool GetIsDirectory() const;
  // __property TRemoteFile * LinkedFile = { read = GetLinkedFile, write = SetLinkedFile };
  TRemoteFile * GetLinkedFile();
  void SetLinkedFile(TRemoteFile * value);
  // __property wstring LinkTo = { read = FLinkTo, write = FLinkTo };
  wstring GetLinkTo() { return FLinkTo; }
  void SetLinkTo(wstring value) { FLinkTo = value; }
  // __property wstring ListingStr = { read = GetListingStr, write = SetListingStr };
  wstring GetListingStr();
  void SetListingStr(wstring value);
  // __property TRights * Rights = { read = FRights, write = SetRights };
  TRights *GetRights() { return FRights; }
  void SetRights(TRights * value);
  // __property TTerminal * Terminal = { read = FTerminal, write = SetTerminal };
  TTerminal * GetTerminal() { return FTerminal; }
  void SetTerminal(TTerminal * value);
  // __property char Type = { read = GetType, write = SetType };
  char GetType() const;
  void SetType(char AType);
  // __property bool Selected  = { read=FSelected, write=FSelected };
  bool GetSelected() { return FSelected; }
  void SetSelected(bool value) { FSelected = value; }
  // __property wstring FullFileName  = { read = GetFullFileName, write = FFullFileName };
  wstring GetFullFileName() const;
  void SetFullFileName(wstring value) { FFullFileName = value; }
  // __property bool HaveFullFileName  = { read = GetHaveFullFileName };
  bool GetHaveFullFileName() const;
  // __property int IconIndex = { read = GetIconIndex };
  int GetIconIndex() const;
  // __property wstring TypeName = { read = GetTypeName };
  wstring GetTypeName();
  // __property bool IsHidden = { read = GetIsHidden, write = SetIsHidden };
  bool GetIsHidden();
  void SetIsHidden(bool value);
  // __property bool IsParentDirectory = { read = GetIsParentDirectory };
  bool GetIsParentDirectory() const;
  // __property bool IsThisDirectory = { read = GetIsThisDirectory };
  bool GetIsThisDirectory() const;
  // __property bool IsInaccesibleDirectory  = { read=GetIsInaccesibleDirectory };
  bool GetIsInaccesibleDirectory() const;
  // __property wstring Extension  = { read=GetExtension };
  wstring GetExtension();
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
  wstring FDirectory;
  TDateTime FTimestamp;
  TRemoteFile * GetFiles(int Index);
  virtual void SetDirectory(wstring value);
  wstring GetFullDirectory();
  bool GetIsRoot();
  TRemoteFile * GetParentDirectory();
  wstring GetParentPath();
  __int64 GetTotalSize();

  virtual void Clear();
public:
  TRemoteFileList();
  TRemoteFile * FindFile(const wstring &FileName);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  virtual void AddFile(TRemoteFile * File);
  __property wstring Directory = { read = FDirectory, write = SetDirectory };
  __property TRemoteFile * Files[int Index] = { read = GetFiles };
  __property wstring FullDirectory  = { read=GetFullDirectory };
  __property bool IsRoot = { read = GetIsRoot };
  __property wstring ParentPath = { read = GetParentPath };
  __property __int64 TotalSize = { read = GetTotalSize };
  __property TDateTime Timestamp = { read = FTimestamp };
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
  virtual void SetDirectory(wstring value);
  TStrings * GetSelectedFiles();
  bool GetLoaded();
  void SetIncludeParentDirectory(bool value);
  void SetIncludeThisDirectory(bool value);
protected:
  virtual void Clear();
public:
  TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = NULL);
  virtual void AddFile(TRemoteFile * File);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  __property TTerminal * Terminal = { read = FTerminal, write = FTerminal };
  __property TStrings * SelectedFiles  = { read=GetSelectedFiles };
  __property bool IncludeParentDirectory = { read = FIncludeParentDirectory, write = SetIncludeParentDirectory };
  __property bool IncludeThisDirectory = { read = FIncludeThisDirectory, write = SetIncludeThisDirectory };
  __property bool Loaded = { read = GetLoaded };
  __property TRemoteFile * ParentDirectory = { read = FParentDirectory };
  __property TRemoteFile * ThisDirectory = { read = FThisDirectory };
};
//---------------------------------------------------------------------------
class TCriticalSection;
class TRemoteDirectoryCache : private TStringList
{
public:
  TRemoteDirectoryCache();
  virtual ~TRemoteDirectoryCache();
  bool HasFileList(const wstring Directory);
  bool HasNewerFileList(const wstring Directory, TDateTime Timestamp);
  bool GetFileList(const wstring Directory,
    TRemoteFileList * FileList);
  void AddFileList(TRemoteFileList * FileList);
  void ClearFileList(wstring Directory, bool SubDirs);
  void Clear();

  __property bool IsEmpty = { read = GetIsEmpty };
protected:
  virtual void Delete(int Index);
private:
  TCriticalSection * FSection;
  bool GetIsEmpty() const;
  void DoClearFileList(wstring Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private TStringList
{
public:
  TRemoteDirectoryChangesCache(int MaxSize);

  void AddDirectoryChange(const wstring SourceDir,
    const wstring Change, const wstring TargetDir);
  void ClearDirectoryChange(wstring SourceDir);
  void ClearDirectoryChangeTarget(wstring TargetDir);
  bool GetDirectoryChange(const wstring SourceDir,
    const wstring Change, wstring & TargetDir);
  void Clear();

  void Serialize(wstring & Data);
  void Deserialize(const wstring Data);

  __property bool IsEmpty = { read = GetIsEmpty };

private:
  static bool DirectoryChangeKey(const wstring SourceDir,
    const wstring Change, wstring & Key);
  bool GetIsEmpty() const;
  void SetValue(const wstring & Name, const wstring & Value);
  wstring GetValue(const wstring & Name);

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

  __property bool AllowUndef = { read = FAllowUndef, write = SetAllowUndef };
  __property bool IsUndef = { read = GetIsUndef };
  __property wstring ModeStr = { read = GetModeStr };
  __property wstring SimplestStr = { read = GetSimplestStr };
  __property wstring Octal = { read = GetOctal, write = SetOctal };
  __property unsigned short Number = { read = GetNumber, write = SetNumber };
  __property unsigned short NumberSet = { read = FSet };
  __property unsigned short NumberUnset = { read = FUnset };
  __property unsigned long NumberDecadic = { read = GetNumberDecadic };
  __property bool ReadOnly = { read = GetReadOnly, write = SetReadOnly };
  __property bool Right[TRight Right] = { read = GetRight, write = SetRight };
  __property TState RightUndef[TRight Right] = { read = GetRightUndef, write = SetRightUndef };
  __property wstring Text = { read = GetText, write = SetText };
  __property bool Unknown = { read = FUnknown };

private:
  bool FAllowUndef;
  unsigned short FSet;
  unsigned short FUnset;
  wstring FText;
  bool FUnknown;

  bool GetIsUndef() const;
  wstring GetModeStr() const;
  wstring GetSimplestStr() const;
  void SetNumber(unsigned short value);
  wstring GetText() const;
  void SetText(const wstring & value);
  void SetOctal(wstring value);
  unsigned short GetNumber() const;
  unsigned short GetNumberSet() const;
  unsigned short GetNumberUnset() const;
  unsigned long GetNumberDecadic() const;
  wstring GetOctal() const;
  bool GetReadOnly();
  bool GetRight(TRight Right) const;
  TState GetRightUndef(TRight Right) const;
  void SetAllowUndef(bool value);
  void SetReadOnly(bool value);
  void SetRight(TRight Right, bool value);
  void SetRightUndef(TRight Right, TState value);
};
//---------------------------------------------------------------------------
enum TValidProperty { vpRights, vpGroup, vpOwner, vpModification, vpLastAccess };
typedef Set<TValidProperty, vpRights, vpLastAccess> TValidProperties;
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
wstring UnixIncludeTrailingBackslash(const wstring Path);
wstring UnixExcludeTrailingBackslash(const wstring Path);
wstring UnixExtractFileDir(const wstring Path);
wstring UnixExtractFilePath(const wstring Path);
wstring UnixExtractFileName(const wstring Path);
wstring UnixExtractFileExt(const wstring Path);
bool UnixComparePaths(const wstring Path1, const wstring Path2);
bool UnixIsChildPath(wstring Parent, wstring Child);
bool ExtractCommonPath(TStrings * Files, wstring & Path);
bool UnixExtractCommonPath(TStrings * Files, wstring & Path);
wstring ExtractFileName(const wstring & Path, bool Unix);
bool IsUnixRootPath(const wstring Path);
bool IsUnixHiddenFile(const wstring Path);
wstring AbsolutePath(const wstring & Base, const wstring & Path);
wstring FromUnixPath(const wstring Path);
wstring ToUnixPath(const wstring Path);
wstring MinimizeName(const wstring FileName, int MaxLen, bool Unix);
wstring MakeFileList(TStrings * FileList);
TDateTime ReduceDateTimePrecision(TDateTime DateTime,
  TModificationFmt Precision);
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
wstring UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision);
int FakeFileImageIndex(wstring FileName, unsigned long Attrs = 0,
  wstring * TypeName = NULL);
//---------------------------------------------------------------------------
#endif
