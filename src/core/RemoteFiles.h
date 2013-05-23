//---------------------------------------------------------------------------
#ifndef RemoteFilesH
#define RemoteFilesH
//---------------------------------------------------------------------------
#include <map.h>
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
class TRemoteToken : public TObject
{
public:
  TRemoteToken();
  explicit TRemoteToken(const UnicodeString & Name);

  void Clear();

  bool operator ==(const TRemoteToken & rht) const;
  bool operator !=(const TRemoteToken & rht) const;
  TRemoteToken & operator =(const TRemoteToken & rht);

  intptr_t Compare(const TRemoteToken & rht) const;

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value) { FName = Value; }
  bool GetNameValid() const;
  intptr_t GetID() const { return FID; }
  void SetID(intptr_t Value);
  bool GetIDValid() const { return FIDValid; }
  bool GetIsSet() const;
  UnicodeString GetLogText() const;
  UnicodeString GetDisplayText() const;

private:
  UnicodeString FName;
  intptr_t FID;
  bool FIDValid;
};
//---------------------------------------------------------------------------
class TRemoteTokenList : public TObject
{
public:
  TRemoteTokenList * Duplicate() const;
  void Clear();
  void Add(const TRemoteToken & Token);
  void AddUnique(const TRemoteToken & Token);
  bool Exists(const UnicodeString & Name) const;
  const TRemoteToken * Find(uintptr_t ID) const;
  const TRemoteToken * Find(const UnicodeString & Name) const;
  void Log(TTerminal * Terminal, const wchar_t * Title);

  intptr_t GetCount() const;
  const TRemoteToken * Token(intptr_t Index) const;

private:
  typedef rde::vector<TRemoteToken> TTokens;
  typedef rde::map<UnicodeString, size_t> TNameMap;
  typedef rde::map<intptr_t, size_t> TIDMap;
  TTokens FTokens;
  mutable TNameMap FNameMap;
  mutable TIDMap FIDMap;
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
  intptr_t FIconIndex;
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

public:
  intptr_t GetAttr() const;
  bool GetBrokenLink() const;
  bool GetIsDirectory() const;
  TRemoteFile * GetLinkedFile() const;
  void SetLinkedFile(TRemoteFile * Value);
  UnicodeString GetModificationStr() const;
  void SetModification(const TDateTime & Value);
  void SetListingStr(const UnicodeString & Value);
  UnicodeString GetListingStr() const;
  UnicodeString GetRightsStr() const;
  wchar_t GetType() const;
  void SetType(wchar_t AType);
  void SetTerminal(TTerminal * Value);
  void SetRights(TRights * Value);
  UnicodeString GetFullFileName() const;
  bool GetHaveFullFileName() const;
  intptr_t GetIconIndex() const;
  UnicodeString GetTypeName() const;
  bool GetIsHidden() const;
  void SetIsHidden(bool Value);
  bool GetIsParentDirectory() const;
  bool GetIsThisDirectory() const;
  bool GetIsInaccesibleDirectory() const;
  UnicodeString GetExtension();
  UnicodeString GetUserModificationStr();

private:
  void LoadTypeInfo() const;

protected:
  void FindLinkedFile();

public:
  explicit TRemoteFile(TRemoteFile * ALinkedByFile = NULL);
  virtual ~TRemoteFile();
  TRemoteFile * Duplicate(bool Standalone = true) const;

  void ShiftTime(const TDateTime & Difference);
  void Complete();

  TRemoteFileList * GetDirectory() const { return FDirectory; }
  void SetDirectory(TRemoteFileList * Value) { FDirectory = Value; }
  __int64 GetSize() const { return FSize; }
  void SetSize(__int64 Value) { FSize = Value; }
  const TRemoteToken & GetFileOwner() const;
  TRemoteToken & GetFileOwner();
  void SetFileOwner(const TRemoteToken & Value);
  const TRemoteToken & GetFileGroup() const;
  TRemoteToken & GetFileGroup();
  void SetFileGroup(const TRemoteToken & Value);
  UnicodeString GetFileName() const { return FFileName; }
  void SetFileName(const UnicodeString & Value);
  TDateTime GetModification() const { return FModification; }
  TModificationFmt GetModificationFmt() const { return FModificationFmt; }
  void SetModificationFmt(TModificationFmt Value) { FModificationFmt = Value; }
  TDateTime GetLastAccess() const { return FLastAccess; }
  void SetLastAccess(TDateTime Value) { FLastAccess = Value; }
  bool GetIsSymLink() const { return FIsSymLink; }
  UnicodeString GetLinkTo() const;
  void SetLinkTo(const UnicodeString & Value);
  TRights * GetRights() const { return FRights; }
  TTerminal * GetTerminal() const { return FTerminal; }
  bool GetSelected() const { return FSelected; }
  void SetSelected(bool Value) { FSelected = Value; }
  void SetFullFileName(const UnicodeString & Value);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryFile : public TRemoteFile
{
public:
  TRemoteDirectoryFile();
  virtual ~TRemoteDirectoryFile() {}
};
//---------------------------------------------------------------------------
class TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  explicit TRemoteParentDirectory(TTerminal * Terminal);
  virtual ~TRemoteParentDirectory() {}
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
  TRemoteFile * GetParentDirectory();

  virtual void Clear();
public:
  TRemoteFileList();
  virtual ~TRemoteFileList() { Clear(); }
  TRemoteFile * FindFile(const UnicodeString & FileName) const;
  virtual void DuplicateTo(TRemoteFileList * Copy) const;
  virtual void AddFile(TRemoteFile * File);
  UnicodeString GetDirectory() const { return FDirectory; }
  virtual void SetDirectory(const UnicodeString & Value);
  TRemoteFile * GetFiles(Integer Index) const;
  UnicodeString GetFullDirectory();
  Boolean GetIsRoot();
  UnicodeString GetParentPath();
  __int64 GetTotalSize();
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
  mutable TStrings * FSelectedFiles;
  TRemoteFile * FParentDirectory;
  TRemoteFile * FThisDirectory;
protected:
  virtual void Clear();
public:
  explicit TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = NULL);
  virtual ~TRemoteDirectory() { Clear(); }
  virtual void AddFile(TRemoteFile * File);
  virtual void DuplicateTo(TRemoteFileList * Copy);
  TTerminal * GetTerminal() const { return FTerminal; }
  void SetTerminal(TTerminal * Value) { FTerminal = Value; }
  TStrings * GetSelectedFiles() const;
  Boolean GetIncludeParentDirectory() const { return FIncludeParentDirectory; }
  void SetIncludeParentDirectory(Boolean Value);
  Boolean GetIncludeThisDirectory() const { return FIncludeThisDirectory; }
  void SetIncludeThisDirectory(Boolean Value);
  Boolean GetLoaded() const;
  TRemoteFile * GetParentDirectory() const { return FParentDirectory; }
  TRemoteFile * GetThisDirectory() const { return FThisDirectory; }
  virtual void SetDirectory(const UnicodeString & Value);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryCache : private TStringList
{
  CUSTOM_MEM_ALLOCATION_IMPL;
public:
  TRemoteDirectoryCache();
  virtual ~TRemoteDirectoryCache();
  bool HasFileList(const UnicodeString & Directory);
  bool HasNewerFileList(const UnicodeString & Directory, TDateTime Timestamp);
  bool GetFileList(const UnicodeString & Directory,
    TRemoteFileList * FileList);
  void AddFileList(TRemoteFileList * FileList);
  void ClearFileList(const UnicodeString & Directory, bool SubDirs);
  void Clear();

  bool GetIsEmpty() const;
protected:
  virtual void Delete(intptr_t Index);
private:
  TCriticalSection * FSection;
  void DoClearFileList(const UnicodeString & Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private TStringList
{
  CUSTOM_MEM_ALLOCATION_IMPL;
public:
  explicit TRemoteDirectoryChangesCache(intptr_t MaxSize);
  virtual ~TRemoteDirectoryChangesCache(){}

  void AddDirectoryChange(const UnicodeString & SourceDir,
    const UnicodeString & Change, const UnicodeString & TargetDir);
  void ClearDirectoryChange(const UnicodeString & SourceDir);
  void ClearDirectoryChangeTarget(const UnicodeString & TargetDir);
  bool GetDirectoryChange(const UnicodeString & SourceDir,
    const UnicodeString & Change, UnicodeString & TargetDir);
  void Clear();

  void Serialize(UnicodeString & Data);
  void Deserialize(const UnicodeString & Data);

  bool GetIsEmpty() const;

private:
  static bool DirectoryChangeKey(const UnicodeString & SourceDir,
    const UnicodeString & Change, UnicodeString & Key);
  void SetValue(const UnicodeString & Name, const UnicodeString & Value);
  UnicodeString GetValue(const UnicodeString & Name);

  intptr_t FMaxSize;
};
//---------------------------------------------------------------------------
class TRights : public TObject
{
public:
  static const intptr_t TextLen = 9;
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
  static TFlag RightToFlag(TRight Right);

  TRights();
  TRights(const TRights & Source);
  explicit TRights(unsigned short Number);

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

  bool GetIsUndef() const;
  UnicodeString GetModeStr() const;
  UnicodeString GetSimplestStr() const;
  void SetNumber(unsigned short Value);
  UnicodeString GetText() const;
  void SetText(const UnicodeString & Value);
  void SetOctal(const UnicodeString & Value);
  unsigned short GetNumber() const;
  unsigned short GetNumberSet() const { return FSet; }
  unsigned short GetNumberUnset() const { return FUnset; }
  unsigned long GetNumberDecadic() const;
  UnicodeString GetOctal() const;
  bool GetReadOnly() const;
  bool GetRight(TRight Right) const;
  TState GetRightUndef(TRight Right) const;
  void SetAllowUndef(bool Value);
  void SetReadOnly(bool Value);
  void SetRight(TRight Right, bool Value);
  void SetRightUndef(TRight Right, TState Value);
  bool GetAllowUndef() const { return FAllowUndef; }
  bool GetUnknown() const { return FUnknown; }

private:
  bool FAllowUndef;
  unsigned short FSet;
  unsigned short FUnset;
  UnicodeString FText;
  bool FUnknown;
};
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
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
class TValidProperties : public TObject
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
  bool Contains(TValidProperty Value) const
  {
    return (FValue & Value) != 0;
  }
  bool operator == (const TValidProperties & rhs) const
  {
    return FValue == rhs.FValue;
  }
  bool operator != (const TValidProperties & rhs) const
  {
    return !(operator == (rhs));
  }
  TValidProperties & operator << (const TValidProperty Value)
  {
    FValue |= Value;
    return *this;
  }
  TValidProperties & operator >> (const TValidProperty Value)
  {
    FValue &= ~(static_cast<__int64>(Value));
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

class TRemoteProperties : public TObject
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
UnicodeString UnixIncludeTrailingBackslash(const UnicodeString & Path);
UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & Path);
UnicodeString UnixExtractFileDir(const UnicodeString & Path);
UnicodeString UnixExtractFilePath(const UnicodeString & Path);
UnicodeString UnixExtractFileName(const UnicodeString & Path);
UnicodeString UnixExtractFileExt(const UnicodeString & Path);
Boolean UnixComparePaths(const UnicodeString & Path1, const UnicodeString & Path2);
bool UnixIsChildPath(const UnicodeString & Parent, const UnicodeString & Child);
bool ExtractCommonPath(TStrings * Files, UnicodeString & Path);
bool UnixExtractCommonPath(TStrings * Files, UnicodeString & Path);
UnicodeString ExtractFileName(const UnicodeString & Path, bool Unix);
bool IsUnixRootPath(const UnicodeString & Path);
bool IsUnixHiddenFile(const UnicodeString & Path);
UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & Path);
UnicodeString FromUnixPath(const UnicodeString & Path);
UnicodeString ToUnixPath(const UnicodeString & Path);
UnicodeString MinimizeName(const UnicodeString & FileName, intptr_t MaxLen, bool Unix);
UnicodeString MakeFileList(TStrings * FileList);
TDateTime ReduceDateTimePrecision(const TDateTime & DateTime,
  TModificationFmt Precision);
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
UnicodeString UserModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
int FakeFileImageIndex(const UnicodeString & FileName, unsigned long Attrs = 0,
  UnicodeString * TypeName = NULL);
//---------------------------------------------------------------------------
#endif
