//---------------------------------------------------------------------------
#ifndef RemoteFilesH
#define RemoteFilesH
//---------------------------------------------------------------------------
#include <vector>
#include <map>
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
  /* __fastcall */ TRemoteToken();
  explicit /* __fastcall */ TRemoteToken(const UnicodeString & Name);

  void __fastcall Clear();

  bool __fastcall operator ==(const TRemoteToken & rht) const;
  bool __fastcall operator !=(const TRemoteToken & rht) const;
  TRemoteToken & __fastcall operator =(const TRemoteToken & rht);

  int __fastcall Compare(const TRemoteToken & rht) const;

  UnicodeString __fastcall GetName() const;
  void __fastcall SetName(const UnicodeString value);
  bool __fastcall GetNameValid() const;
  unsigned int GetID() const { return FID; }
  void __fastcall SetID(unsigned int value);
  bool __fastcall GetIDValid() const { return FIDValid; }
  bool __fastcall GetIsSet() const;
  UnicodeString __fastcall GetLogText() const;
  UnicodeString __fastcall GetDisplayText() const;

private:
  UnicodeString FName;
  unsigned int FID;
  bool FIDValid;

public:
};
//---------------------------------------------------------------------------
class TRemoteTokenList
{
public:
  TRemoteTokenList * __fastcall Duplicate() const;
  void __fastcall Clear();
  void __fastcall Add(const TRemoteToken & Token);
  void __fastcall AddUnique(const TRemoteToken & Token);
  bool __fastcall Exists(const UnicodeString & Name) const;
  const TRemoteToken * Find(unsigned int ID) const;
  const TRemoteToken * Find(const UnicodeString & Name) const;
  void __fastcall Log(TTerminal * Terminal, const wchar_t * Title);

  int __fastcall Count() const;
  const TRemoteToken * __fastcall Token(int Index) const;

private:
  typedef std::vector<TRemoteToken> TTokens;
  typedef std::map<UnicodeString, size_t> TNameMap;
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
  UnicodeString FFileName;
  Integer FINodeBlocks;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  Integer FIconIndex;
  Boolean FIsSymLink;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  UnicodeString FLinkTo;
  TRights *FRights;
  TTerminal *FTerminal;
  wchar_t FType;
  bool FSelected;
  bool FCyclicLink;
  UnicodeString FFullFileName;
  int FIsHidden;
  UnicodeString FTypeName;
  TRemoteFile * Self;

public:
  int __fastcall GetAttr();
  bool __fastcall GetBrokenLink();
  bool __fastcall GetIsDirectory() const;
  TRemoteFile * __fastcall GetLinkedFile();
  void __fastcall SetLinkedFile(TRemoteFile * value);
  UnicodeString __fastcall GetModificationStr();
  void __fastcall SetModification(const TDateTime & value);
  void __fastcall SetListingStr(UnicodeString value);
  UnicodeString __fastcall GetListingStr();
  UnicodeString __fastcall GetRightsStr();
  wchar_t __fastcall GetType() const;
  void __fastcall SetType(wchar_t AType);
  void __fastcall SetTerminal(TTerminal * value);
  void __fastcall SetRights(TRights * value);
  UnicodeString __fastcall GetFullFileName() const;
  bool __fastcall GetHaveFullFileName() const;
  int __fastcall GetIconIndex() const;
  UnicodeString __fastcall GetTypeName();
  bool __fastcall GetIsHidden();
  void __fastcall SetIsHidden(bool value);
  bool __fastcall GetIsParentDirectory() const;
  bool __fastcall GetIsThisDirectory() const;
  bool __fastcall GetIsInaccesibleDirectory() const;
  UnicodeString __fastcall GetExtension();
  UnicodeString __fastcall GetUserModificationStr();

private:
  void __fastcall LoadTypeInfo();

protected:
  void __fastcall FindLinkedFile();

public:
  explicit /* __fastcall */ TRemoteFile(TRemoteFile * ALinkedByFile = NULL);
  virtual /* __fastcall */ ~TRemoteFile();
  TRemoteFile * __fastcall Duplicate(bool Standalone = true) const;

  void __fastcall ShiftTime(const TDateTime & Difference);
  void __fastcall Complete();

  TRemoteFileList * __fastcall GetDirectory() const { return FDirectory; }
  void __fastcall SetDirectory(TRemoteFileList * value) { FDirectory = value; }
  __int64 __fastcall GetSize() const { return FSize; }
  void __fastcall SetSize(__int64 value) { FSize = value; }
  const TRemoteToken & __fastcall GetFileOwner() const;
  TRemoteToken & __fastcall GetFileOwner();
  void __fastcall SetFileOwner(TRemoteToken value);
  const TRemoteToken & __fastcall GetFileGroup() const;
  TRemoteToken & __fastcall GetFileGroup();
  void __fastcall SetFileGroup(TRemoteToken value);
  UnicodeString __fastcall GetFileName() const;
  void __fastcall SetFileName(const UnicodeString value);
  int __fastcall GetINodeBlocks();
  TDateTime __fastcall GetModification() const { return FModification; }
  TModificationFmt __fastcall GetModificationFmt() const { return FModificationFmt; }
  void __fastcall SetModificationFmt(TModificationFmt value) { FModificationFmt = value; }
  TDateTime __fastcall GetLastAccess() const { return FLastAccess; }
  void __fastcall SetLastAccess(TDateTime value) { FLastAccess = value; }
  bool __fastcall GetIsSymLink() const { return FIsSymLink; }
  UnicodeString __fastcall GetLinkTo() const;
  void __fastcall SetLinkTo(const UnicodeString value);
  TRights * __fastcall GetRights() const { return FRights; }
  TTerminal * __fastcall GetTerminal() const { return FTerminal; }
  bool __fastcall GetSelected() { return FSelected; }
  void __fastcall SetSelected(bool value) { FSelected = value; }
  void __fastcall SetFullFileName(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryFile : public TRemoteFile
{
public:
  /* __fastcall */ TRemoteDirectoryFile();
  virtual /* __fastcall */ ~TRemoteDirectoryFile() {}
};
//---------------------------------------------------------------------------
class TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  explicit /* __fastcall */ TRemoteParentDirectory(TTerminal * Terminal);
  virtual /* __fastcall */ ~TRemoteParentDirectory() {}
};
//---------------------------------------------------------------------------
class TRemoteFileList : public TObjectList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;
protected:
  UnicodeString FDirectory;
  TDateTime FTimestamp;
  TRemoteFile * __fastcall GetParentDirectory();

  virtual void __fastcall Clear();
public:
  /* __fastcall */ TRemoteFileList();
  virtual ~TRemoteFileList() { Clear(); }
  TRemoteFile * __fastcall FindFile(const UnicodeString &FileName);
  virtual void __fastcall DuplicateTo(TRemoteFileList * Copy);
  virtual void __fastcall AddFile(TRemoteFile * File);
  UnicodeString GetDirectory() const { return FDirectory; }
  virtual void __fastcall SetDirectory(UnicodeString value);
  TRemoteFile * __fastcall GetFiles(Integer Index);
  UnicodeString __fastcall GetFullDirectory();
  Boolean __fastcall GetIsRoot();
  UnicodeString __fastcall GetParentPath();
  __int64 __fastcall GetTotalSize();
  TDateTime GetTimestamp() const { return FTimestamp; }
};
//---------------------------------------------------------------------------
class TRemoteDirectory : public TRemoteFileList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TWebDAVFileSystem;
private:
  Boolean FIncludeParentDirectory;
  Boolean FIncludeThisDirectory;
  TTerminal * FTerminal;
  TStrings * FSelectedFiles;
  TRemoteFile * FParentDirectory;
  TRemoteFile * FThisDirectory;
protected:
  virtual void __fastcall Clear();
public:
  explicit /* __fastcall */ TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = NULL);
  virtual /* __fastcall */ ~TRemoteDirectory() { Clear(); }
  virtual void __fastcall AddFile(TRemoteFile * File);
  virtual void __fastcall DuplicateTo(TRemoteFileList * Copy);
  TTerminal * GetTerminal() { return FTerminal; }
  void SetTerminal(TTerminal * value) { FTerminal = value; }
  TStrings * __fastcall GetSelectedFiles();
  Boolean GetIncludeParentDirectory() { return FIncludeParentDirectory; }
  void __fastcall SetIncludeParentDirectory(Boolean value);
  Boolean GetIncludeThisDirectory() { return FIncludeThisDirectory; }
  void __fastcall SetIncludeThisDirectory(Boolean value);
  Boolean __fastcall GetLoaded();
  TRemoteFile * GetParentDirectory() { return FParentDirectory; }
  TRemoteFile * GetThisDirectory() { return FThisDirectory; }
  virtual void __fastcall SetDirectory(UnicodeString value);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryCache : private TStringList
{
public:
  /* __fastcall */ TRemoteDirectoryCache();
  virtual /* __fastcall */ ~TRemoteDirectoryCache();
  bool __fastcall HasFileList(const UnicodeString Directory);
  bool __fastcall HasNewerFileList(const UnicodeString Directory, TDateTime Timestamp);
  bool __fastcall GetFileList(const UnicodeString Directory,
    TRemoteFileList * FileList);
  void __fastcall AddFileList(TRemoteFileList * FileList);
  void __fastcall ClearFileList(UnicodeString Directory, bool SubDirs);
  void __fastcall Clear();

  bool __fastcall GetIsEmpty() const;
protected:
  virtual void __fastcall Delete(int Index);
private:
  TCriticalSection * FSection;
  TRemoteDirectoryCache * Self;
  void __fastcall DoClearFileList(UnicodeString Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private TStringList
{
public:
  explicit /* __fastcall */ TRemoteDirectoryChangesCache(int MaxSize);
  virtual /* __fastcall */ ~TRemoteDirectoryChangesCache(){}

  void __fastcall AddDirectoryChange(const UnicodeString SourceDir,
    const UnicodeString Change, const UnicodeString TargetDir);
  void __fastcall ClearDirectoryChange(UnicodeString SourceDir);
  void __fastcall ClearDirectoryChangeTarget(UnicodeString TargetDir);
  bool __fastcall GetDirectoryChange(const UnicodeString SourceDir,
    const UnicodeString Change, UnicodeString & TargetDir);
  void __fastcall Clear();

  void __fastcall Serialize(UnicodeString & Data);
  void __fastcall Deserialize(const UnicodeString Data);

  bool __fastcall GetIsEmpty() const;

private:
  static bool __fastcall DirectoryChangeKey(const UnicodeString SourceDir,
    const UnicodeString Change, UnicodeString & Key);
  void __fastcall SetValue(const UnicodeString & Name, const UnicodeString & Value);
  UnicodeString __fastcall GetValue(const UnicodeString & Name);

  int FMaxSize;
};
//---------------------------------------------------------------------------
class TRights
{
public:
  static const int TextLen = 9;
  static const wchar_t UndefSymbol = L'$';
  static const wchar_t UnsetSymbol = L'-';
  static const wchar_t BasicSymbols[];
  static const wchar_t CombinedSymbols[];
  static const wchar_t ExtendedSymbols[];
  static const wchar_t ModeGroups[];
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
  static TFlag __fastcall RightToFlag(TRight Right);

  /* __fastcall */ TRights();
  /* __fastcall */ TRights(const TRights & Source);
  explicit /* __fastcall */ TRights(unsigned short Number);

  void __fastcall Assign(const TRights * Source);
  void __fastcall AddExecute();
  void __fastcall AllUndef();

  bool __fastcall operator ==(const TRights & rhr) const;
  bool __fastcall operator ==(unsigned short rhr) const;
  bool __fastcall operator !=(const TRights & rhr) const;
  TRights & __fastcall operator =(const TRights & rhr);
  TRights & __fastcall operator =(unsigned short rhr);
  TRights __fastcall operator ~() const;
  TRights __fastcall operator &(unsigned short rhr) const;
  TRights __fastcall operator &(const TRights & rhr) const;
  TRights & __fastcall operator &=(unsigned short rhr);
  TRights & __fastcall operator &=(const TRights & rhr);
  TRights __fastcall operator |(unsigned short rhr) const;
  TRights __fastcall operator |(const TRights & rhr) const;
  TRights & __fastcall operator |=(unsigned short rhr);
  TRights & __fastcall operator |=(const TRights & rhr);
  __fastcall operator unsigned short() const;
  __fastcall operator unsigned long() const;

  bool __fastcall GetIsUndef() const;
  UnicodeString __fastcall GetModeStr() const;
  UnicodeString __fastcall GetSimplestStr() const;
  void __fastcall SetNumber(unsigned short value);
  UnicodeString __fastcall GetText() const;
  void __fastcall SetText(const UnicodeString & value);
  void __fastcall SetOctal(UnicodeString value);
  unsigned short __fastcall GetNumber() const;
  unsigned short __fastcall GetNumberSet() const { return FSet; }
  unsigned short __fastcall GetNumberUnset() const { return FUnset; }
  unsigned long __fastcall GetNumberDecadic() const;
  UnicodeString __fastcall GetOctal() const;
  bool __fastcall GetReadOnly() const;
  bool __fastcall GetRight(TRight Right) const;
  TState __fastcall GetRightUndef(TRight Right) const;
  void __fastcall SetAllowUndef(bool value);
  void __fastcall SetReadOnly(bool value);
  void __fastcall SetRight(TRight Right, bool value);
  void __fastcall SetRightUndef(TRight Right, TState value);
  bool __fastcall GetAllowUndef() const { return FAllowUndef; }
  bool __fastcall GetUnknown() const { return FUnknown; }

private:
  bool FAllowUndef;
  unsigned short FSet;
  unsigned short FUnset;
  UnicodeString FText;
  bool FUnknown;
};
//---------------------------------------------------------------------------
#ifndef _MSC_VER
enum TValidProperty { vpRights, vpGroup, vpOwner, vpModification, vpLastAccess };
typedef Set<TValidProperty, vpRights, vpLastAccess> TValidProperties;
#else
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
  bool operator == (const TValidProperties & rhs) const
  {
    return FValue == rhs.FValue;
  }
  bool operator != (const TValidProperties & rhs) const
  {
    return !(operator == (rhs));
  }
  TValidProperties & operator << (const TValidProperty value)
  {
    FValue |= value;
    return *this;
  }
  TValidProperties & operator >> (const TValidProperty value)
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
#endif

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

  /* __fastcall */ TRemoteProperties();
  /* __fastcall */ TRemoteProperties(const TRemoteProperties & rhp);
  bool __fastcall operator ==(const TRemoteProperties & rhp) const;
  bool __fastcall operator !=(const TRemoteProperties & rhp) const;
  void __fastcall Default();
  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage) const;

  static TRemoteProperties __fastcall CommonProperties(TStrings * FileList);
  static TRemoteProperties __fastcall ChangedProperties(
    const TRemoteProperties & OriginalProperties, TRemoteProperties NewProperties);
};
//---------------------------------------------------------------------------
UnicodeString __fastcall UnixIncludeTrailingBackslash(const UnicodeString Path);
UnicodeString __fastcall UnixExcludeTrailingBackslash(const UnicodeString Path);
UnicodeString __fastcall UnixExtractFileDir(const UnicodeString Path);
UnicodeString __fastcall UnixExtractFilePath(const UnicodeString Path);
UnicodeString __fastcall UnixExtractFileName(const UnicodeString Path);
UnicodeString __fastcall UnixExtractFileExt(const UnicodeString Path);
Boolean __fastcall UnixComparePaths(const UnicodeString Path1, const UnicodeString Path2);
bool __fastcall UnixIsChildPath(UnicodeString Parent, UnicodeString Child);
bool __fastcall ExtractCommonPath(TStrings * Files, UnicodeString & Path);
bool __fastcall UnixExtractCommonPath(TStrings * Files, UnicodeString & Path);
UnicodeString __fastcall ExtractFileName(const UnicodeString & Path, bool Unix);
bool __fastcall IsUnixRootPath(const UnicodeString Path);
bool __fastcall IsUnixHiddenFile(const UnicodeString Path);
UnicodeString __fastcall AbsolutePath(const UnicodeString & Base, const UnicodeString & Path);
UnicodeString __fastcall FromUnixPath(const UnicodeString Path);
UnicodeString __fastcall ToUnixPath(const UnicodeString Path);
UnicodeString __fastcall MinimizeName(const UnicodeString FileName, int MaxLen, bool Unix);
UnicodeString __fastcall MakeFileList(TStrings * FileList);
TDateTime __fastcall ReduceDateTimePrecision(TDateTime DateTime,
  TModificationFmt Precision);
TModificationFmt __fastcall LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
UnicodeString __fastcall UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision);
int __fastcall FakeFileImageIndex(UnicodeString FileName, unsigned long Attrs = 0,
  UnicodeString * TypeName = NULL);
//---------------------------------------------------------------------------
#endif
