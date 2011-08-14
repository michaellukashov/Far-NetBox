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
  explicit TRemoteToken(const AnsiString & Name);

  void Clear();

  bool operator ==(const TRemoteToken & rht) const;
  bool operator !=(const TRemoteToken & rht) const;
  TRemoteToken & operator =(const TRemoteToken & rht);

  int Compare(const TRemoteToken & rht) const;

  __property AnsiString Name = { read = FName, write = FName };
  __property bool NameValid = { read = GetNameValid };
  __property unsigned int ID = { read = FID, write = SetID };
  __property bool IDValid = { read = FIDValid };
  __property bool IsSet  = { read = GetIsSet };
  __property AnsiString LogText = { read = GetLogText };
  __property AnsiString DisplayText = { read = GetDisplayText };

private:
  AnsiString FName;
  unsigned int FID;
  bool FIDValid;

  void SetID(unsigned int value);
  bool GetNameValid() const;
  bool GetIsSet() const;
  AnsiString GetDisplayText() const;
  AnsiString GetLogText() const;
};
//---------------------------------------------------------------------------
class TRemoteTokenList
{
public:
  TRemoteTokenList * Duplicate() const;
  void Clear();
  void Add(const TRemoteToken & Token);
  void AddUnique(const TRemoteToken & Token);
  bool Exists(const AnsiString & Name) const;
  const TRemoteToken * Find(unsigned int ID) const;
  const TRemoteToken * Find(const AnsiString & Name) const;
  void Log(TTerminal * Terminal, const char * Title);

  int Count() const;
  const TRemoteToken * Token(int Index) const;

private:
  typedef std::vector<TRemoteToken> TTokens;
  typedef std::map<AnsiString, size_t> TNameMap;
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
  AnsiString FFileName;
  Integer FINodeBlocks;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  Integer FIconIndex;
  Boolean FIsSymLink;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  AnsiString FLinkTo;
  TRights *FRights;
  TTerminal *FTerminal;
  Char FType;
  bool FSelected;
  bool FCyclicLink;
  AnsiString FFullFileName;
  int FIsHidden;
  AnsiString FTypeName;
  int GetAttr();
  bool GetBrokenLink();
  bool GetIsDirectory() const;
  TRemoteFile * GetLinkedFile();
  void SetLinkedFile(TRemoteFile * value);
  AnsiString GetModificationStr();
  void SetModification(const TDateTime & value);
  void SetListingStr(AnsiString value);
  AnsiString GetListingStr();
  AnsiString GetRightsStr();
  char GetType() const;
  void SetType(char AType);
  void SetTerminal(TTerminal * value);
  void SetRights(TRights * value);
  AnsiString GetFullFileName() const;
  bool GetHaveFullFileName() const;
  int GetIconIndex() const;
  AnsiString GetTypeName();
  bool GetIsHidden();
  void SetIsHidden(bool value);
  bool GetIsParentDirectory() const;
  bool GetIsThisDirectory() const;
  bool GetIsInaccesibleDirectory() const;
  AnsiString GetExtension();
  AnsiString GetUserModificationStr();
  void LoadTypeInfo();

protected:
  void FindLinkedFile();

public:
  TRemoteFile(TRemoteFile * ALinkedByFile = NULL);
  virtual ~TRemoteFile();
  TRemoteFile * Duplicate(bool Standalone = true) const;

  void ShiftTime(const TDateTime & Difference);
  void Complete();

  __property int Attr = { read = GetAttr };
  __property bool BrokenLink = { read = GetBrokenLink };
  __property TRemoteFileList * Directory = { read = FDirectory, write = FDirectory };
  __property AnsiString RightsStr = { read = GetRightsStr };
  __property __int64 Size = { read = FSize, write = FSize };
  __property TRemoteToken Owner = { read = FOwner, write = FOwner };
  __property TRemoteToken Group = { read = FGroup, write = FGroup };
  __property AnsiString FileName = { read = FFileName, write = FFileName };
  __property int INodeBlocks = { read = FINodeBlocks };
  __property TDateTime Modification = { read = FModification, write = SetModification };
  __property AnsiString ModificationStr = { read = GetModificationStr };
  __property AnsiString UserModificationStr = { read = GetUserModificationStr };
  __property TModificationFmt ModificationFmt = { read = FModificationFmt, write = FModificationFmt };
  __property TDateTime LastAccess = { read = FLastAccess, write = FLastAccess };
  __property bool IsSymLink = { read = FIsSymLink };
  __property bool IsDirectory = { read = GetIsDirectory };
  __property TRemoteFile * LinkedFile = { read = GetLinkedFile, write = SetLinkedFile };
  __property AnsiString LinkTo = { read = FLinkTo, write = FLinkTo };
  __property AnsiString ListingStr = { read = GetListingStr, write = SetListingStr };
  __property TRights * Rights = { read = FRights, write = SetRights };
  __property TTerminal * Terminal = { read = FTerminal, write = SetTerminal };
  __property Char Type = { read = GetType, write = SetType };
  __property bool Selected  = { read=FSelected, write=FSelected };
  __property AnsiString FullFileName  = { read = GetFullFileName, write = FFullFileName };
  __property bool HaveFullFileName  = { read = GetHaveFullFileName };
  __property int IconIndex = { read = GetIconIndex };
  __property AnsiString TypeName = { read = GetTypeName };
  __property bool IsHidden = { read = GetIsHidden, write = SetIsHidden };
  __property bool IsParentDirectory = { read = GetIsParentDirectory };
  __property bool IsThisDirectory = { read = GetIsThisDirectory };
  __property bool IsInaccesibleDirectory  = { read=GetIsInaccesibleDirectory };
  __property AnsiString Extension  = { read=GetExtension };
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
  AnsiString FDirectory;
  TDateTime FTimestamp;
  TRemoteFile * GetFiles(Integer Index);
  virtual void SetDirectory(AnsiString value);
  AnsiString GetFullDirectory();
  Boolean GetIsRoot();
  TRemoteFile * GetParentDirectory();
  AnsiString GetParentPath();
  __int64 GetTotalSize();

  virtual void Clear();
public:
  TRemoteFileList();
  TRemoteFile * FindFile(const AnsiString &FileName);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  virtual void AddFile(TRemoteFile * File);
  __property AnsiString Directory = { read = FDirectory, write = SetDirectory };
  __property TRemoteFile * Files[Integer Index] = { read = GetFiles };
  __property AnsiString FullDirectory  = { read=GetFullDirectory };
  __property Boolean IsRoot = { read = GetIsRoot };
  __property AnsiString ParentPath = { read = GetParentPath };
  __property __int64 TotalSize = { read = GetTotalSize };
  __property TDateTime Timestamp = { read = FTimestamp };
};
//---------------------------------------------------------------------------
class TRemoteDirectory : public TRemoteFileList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
private:
  Boolean FIncludeParentDirectory;
  Boolean FIncludeThisDirectory;
  TTerminal * FTerminal;
  TStrings * FSelectedFiles;
  TRemoteFile * FParentDirectory;
  TRemoteFile * FThisDirectory;
  virtual void SetDirectory(AnsiString value);
  TStrings * GetSelectedFiles();
  Boolean GetLoaded();
  void SetIncludeParentDirectory(Boolean value);
  void SetIncludeThisDirectory(Boolean value);
protected:
  virtual void Clear();
public:
  TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = NULL);
  virtual void AddFile(TRemoteFile * File);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  __property TTerminal * Terminal = { read = FTerminal, write = FTerminal };
  __property TStrings * SelectedFiles  = { read=GetSelectedFiles };
  __property Boolean IncludeParentDirectory = { read = FIncludeParentDirectory, write = SetIncludeParentDirectory };
  __property Boolean IncludeThisDirectory = { read = FIncludeThisDirectory, write = SetIncludeThisDirectory };
  __property Boolean Loaded = { read = GetLoaded };
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
  bool HasFileList(const AnsiString Directory);
  bool HasNewerFileList(const AnsiString Directory, TDateTime Timestamp);
  bool GetFileList(const AnsiString Directory,
    TRemoteFileList * FileList);
  void AddFileList(TRemoteFileList * FileList);
  void ClearFileList(AnsiString Directory, bool SubDirs);
  void Clear();

  __property bool IsEmpty = { read = GetIsEmpty };
protected:
  virtual void Delete(int Index);
private:
  TCriticalSection * FSection;
  bool GetIsEmpty() const;
  void DoClearFileList(AnsiString Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private TStringList
{
public:
  TRemoteDirectoryChangesCache(int MaxSize);

  void AddDirectoryChange(const AnsiString SourceDir,
    const AnsiString Change, const AnsiString TargetDir);
  void ClearDirectoryChange(AnsiString SourceDir);
  void ClearDirectoryChangeTarget(AnsiString TargetDir);
  bool GetDirectoryChange(const AnsiString SourceDir,
    const AnsiString Change, AnsiString & TargetDir);
  void Clear();

  void Serialize(AnsiString & Data);
  void Deserialize(const AnsiString Data);

  __property bool IsEmpty = { read = GetIsEmpty };

private:
  static bool DirectoryChangeKey(const AnsiString SourceDir,
    const AnsiString Change, AnsiString & Key);
  bool GetIsEmpty() const;
  void SetValue(const AnsiString & Name, const AnsiString & Value);
  AnsiString GetValue(const AnsiString & Name);

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
  __property AnsiString ModeStr = { read = GetModeStr };
  __property AnsiString SimplestStr = { read = GetSimplestStr };
  __property AnsiString Octal = { read = GetOctal, write = SetOctal };
  __property unsigned short Number = { read = GetNumber, write = SetNumber };
  __property unsigned short NumberSet = { read = FSet };
  __property unsigned short NumberUnset = { read = FUnset };
  __property unsigned long NumberDecadic = { read = GetNumberDecadic };
  __property bool ReadOnly = { read = GetReadOnly, write = SetReadOnly };
  __property bool Right[TRight Right] = { read = GetRight, write = SetRight };
  __property TState RightUndef[TRight Right] = { read = GetRightUndef, write = SetRightUndef };
  __property AnsiString Text = { read = GetText, write = SetText };
  __property bool Unknown = { read = FUnknown };

private:
  bool FAllowUndef;
  unsigned short FSet;
  unsigned short FUnset;
  AnsiString FText;
  bool FUnknown;

  bool GetIsUndef() const;
  AnsiString GetModeStr() const;
  AnsiString GetSimplestStr() const;
  void SetNumber(unsigned short value);
  AnsiString GetText() const;
  void SetText(const AnsiString & value);
  void SetOctal(AnsiString value);
  unsigned short GetNumber() const;
  unsigned short GetNumberSet() const;
  unsigned short GetNumberUnset() const;
  unsigned long GetNumberDecadic() const;
  AnsiString GetOctal() const;
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
AnsiString UnixIncludeTrailingBackslash(const AnsiString Path);
AnsiString UnixExcludeTrailingBackslash(const AnsiString Path);
AnsiString UnixExtractFileDir(const AnsiString Path);
AnsiString UnixExtractFilePath(const AnsiString Path);
AnsiString UnixExtractFileName(const AnsiString Path);
AnsiString UnixExtractFileExt(const AnsiString Path);
Boolean UnixComparePaths(const AnsiString Path1, const AnsiString Path2);
bool UnixIsChildPath(AnsiString Parent, AnsiString Child);
bool ExtractCommonPath(TStrings * Files, AnsiString & Path);
bool UnixExtractCommonPath(TStrings * Files, AnsiString & Path);
AnsiString ExtractFileName(const AnsiString & Path, bool Unix);
bool IsUnixRootPath(const AnsiString Path);
bool IsUnixHiddenFile(const AnsiString Path);
AnsiString AbsolutePath(const AnsiString & Base, const AnsiString & Path);
AnsiString FromUnixPath(const AnsiString Path);
AnsiString ToUnixPath(const AnsiString Path);
AnsiString MinimizeName(const AnsiString FileName, int MaxLen, bool Unix);
AnsiString MakeFileList(TStrings * FileList);
TDateTime ReduceDateTimePrecision(TDateTime DateTime,
  TModificationFmt Precision);
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
AnsiString UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision);
int FakeFileImageIndex(AnsiString FileName, unsigned long Attrs = 0,
  AnsiString * TypeName = NULL);
//---------------------------------------------------------------------------
#endif
