
#pragma once

#include <rdestl/vector.h>
#include <rdestl/map.h>

#include <Sysutils.hpp>
#include <Common.h>

#if 0
//moved to base/Common.h
enum TModificationFmt
{
  mfNone,
  mfMDHM,
  mfMDY,
  mfFull,
};
#endif // #if 0

#define SYMLINKSTR L" -> "
#define PARENTDIRECTORY L".."
#define THISDIRECTORY L"."
#define ROOTDIRECTORY L"/"
#define FILETYPE_DEFAULT L'-'
#define FILETYPE_SYMLINK L'L'
#define FILETYPE_DIRECTORY L'D'
#define PARTIAL_EXT L".filepart"

class TTerminal;
class TRights;
class TRemoteFileList;
class THierarchicalStorage;

class NB_CORE_EXPORT TRemoteToken : public TObject
{
public:
  TRemoteToken();
  explicit TRemoteToken(UnicodeString Name);
  explicit TRemoteToken(const TRemoteToken & rhs);

  void Clear();

  bool operator==(const TRemoteToken & rhs) const;
  bool operator!=(const TRemoteToken & rhs) const;
  TRemoteToken & operator=(const TRemoteToken & rhs);

  intptr_t Compare(const TRemoteToken & rhs) const;

#if 0
  __property UnicodeString Name = { read = FName, write = FName };
  __property bool NameValid = { read = GetNameValid };
  __property unsigned int ID = { read = FID, write = SetID };
  __property bool IDValid = { read = FIDValid };
  __property bool IsSet  = { read = GetIsSet };
  __property UnicodeString LogText = { read = GetLogText };
  __property UnicodeString DisplayText = { read = GetDisplayText };
#endif // #if 0

  UnicodeString GetName() const { return FName; }
  void SetName(UnicodeString Value) { FName = Value; }
  intptr_t GetID() const { return FID; }
  bool GetIDValid() const { return FIDValid; }

private:
  UnicodeString FName;
  intptr_t FID;
  bool FIDValid;

public:
  void SetID(intptr_t Value);
  bool GetNameValid() const;
  bool GetIsSet() const;
  UnicodeString GetDisplayText() const;
  UnicodeString GetLogText() const;
};

class NB_CORE_EXPORT TRemoteTokenList : public TObject
{
public:
  TRemoteTokenList * Duplicate() const;
  void Clear();
  void Add(const TRemoteToken & Token);
  void AddUnique(const TRemoteToken & Token);
  bool Exists(UnicodeString Name) const;
  const TRemoteToken * Find(uintptr_t ID) const;
  const TRemoteToken * Find(UnicodeString Name) const;
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

class NB_CORE_EXPORT TRemoteFile : public TPersistent
{
NB_DISABLE_COPY(TRemoteFile)
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRemoteFile); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteFile) || TPersistent::is(Kind); }
private:
  TRemoteFileList * FDirectory;
  TRemoteToken FOwner;
  TModificationFmt FModificationFmt;
  UnicodeString FFileName;
  UnicodeString FDisplayName;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  TRights * FRights;
  TTerminal * FTerminal;
  UnicodeString FLinkTo;
  UnicodeString FHumanRights;
  UnicodeString FFullFileName;
  UnicodeString FTypeName;
  int64_t FSize;
  int64_t FINodeBlocks;
  intptr_t FIconIndex;
  intptr_t FIsHidden;
  wchar_t FType;
  bool FIsSymLink;
  bool FCyclicLink;

public:
  intptr_t GetAttr() const;
  bool GetBrokenLink() const;
  bool GetIsDirectory() const;
  TRemoteFile * GetLinkedFile() const;
  void SetLinkedFile(TRemoteFile * Value);
  UnicodeString GetModificationStr() const;
  void SetModification(const TDateTime & Value);
  void SetListingStr(UnicodeString Value);
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
  UnicodeString GetExtension() const;
  UnicodeString GetUserModificationStr() const;
  void LoadTypeInfo() const;
  int64_t GetSize() const;

protected:
  void FindLinkedFile();

public:
  explicit TRemoteFile(TRemoteFile * ALinkedByFile = nullptr);
  explicit TRemoteFile(TObjectClassId Kind, TRemoteFile * ALinkedByFile = nullptr);
  virtual ~TRemoteFile();
  TRemoteFile * Duplicate(bool Standalone = true) const;

  void ShiftTimeInSeconds(int64_t Seconds);
  bool GetIsTimeShiftingApplicable() const;
  void Complete();

  static bool GetIsTimeShiftingApplicable(TModificationFmt ModificationFmt);
  static void ShiftTimeInSeconds(TDateTime & DateTime, TModificationFmt ModificationFmt, int64_t Seconds);

#if 0
  __property int Attr = { read = GetAttr };
  __property bool BrokenLink = { read = GetBrokenLink };
  __property TRemoteFileList * Directory = { read = FDirectory, write = FDirectory };
  __property UnicodeString RightsStr = { read = GetRightsStr };
  __property __int64 Size = { read = GetSize, write = FSize };
  __property TRemoteToken Owner = { read = FOwner, write = FOwner };
  __property TRemoteToken Group = { read = FGroup, write = FGroup };
  __property UnicodeString FileName = { read = FFileName, write = FFileName };
  __property UnicodeString DisplayName = { read = FDisplayName, write = FDisplayName };
  __property int INodeBlocks = { read = FINodeBlocks };
  __property TDateTime Modification = { read = FModification, write = SetModification };
  __property UnicodeString ModificationStr = { read = GetModificationStr };
  __property UnicodeString UserModificationStr = { read = GetUserModificationStr };
  __property TModificationFmt ModificationFmt = { read = FModificationFmt, write = FModificationFmt };
  __property TDateTime LastAccess = { read = FLastAccess, write = FLastAccess };
  __property bool IsSymLink = { read = FIsSymLink };
  __property bool IsDirectory = { read = GetIsDirectory };
  __property TRemoteFile * LinkedFile = { read = GetLinkedFile, write = SetLinkedFile };
  __property UnicodeString LinkTo = { read = FLinkTo, write = FLinkTo };
  __property UnicodeString ListingStr = { read = GetListingStr, write = SetListingStr };
  __property TRights * Rights = { read = FRights, write = SetRights };
  __property UnicodeString HumanRights = { read = FHumanRights, write = FHumanRights };
  __property TTerminal * Terminal = { read = FTerminal, write = SetTerminal };
  __property wchar_t Type = { read = GetType, write = SetType };
  __property UnicodeString FullFileName  = { read = GetFullFileName, write = FFullFileName };
  __property bool HaveFullFileName  = { read = GetHaveFullFileName };
  __property int IconIndex = { read = GetIconIndex };
  __property UnicodeString TypeName = { read = GetTypeName };
  __property bool IsHidden = { read = GetIsHidden, write = SetIsHidden };
  __property bool IsParentDirectory = { read = GetIsParentDirectory };
  __property bool IsThisDirectory = { read = GetIsThisDirectory };
  __property bool IsInaccesibleDirectory  = { read=GetIsInaccesibleDirectory };
  __property UnicodeString Extension  = { read=GetExtension };
#endif // #if 0

  TRemoteFileList * GetDirectory() const { return FDirectory; }
  void SetDirectory(TRemoteFileList * Value) { FDirectory = Value; }
  void SetSize(int64_t Value) { FSize = Value; }
  const TRemoteToken & GetFileOwner() const { return FOwner; }
  TRemoteToken & GetFileOwner() { return FOwner; }
  void SetFileOwner(const TRemoteToken & Value) { FOwner = Value; }
  const TRemoteToken & GetFileGroup() const { return FGroup; }
  TRemoteToken & GetFileGroup() { return FGroup; }
  void SetFileGroup(const TRemoteToken & Value) { FGroup = Value; }
  UnicodeString GetFileName() const { return FFileName; }
  void SetFileName(UnicodeString Value) { FFileName = Value; }
  UnicodeString GetDisplayName() const { return FDisplayName; }
  void SetDisplayName(UnicodeString Value) { FDisplayName = Value; }
  TDateTime GetModification() const { return FModification; }
  TModificationFmt GetModificationFmt() const { return FModificationFmt; }
  void SetModificationFmt(TModificationFmt Value) { FModificationFmt = Value; }
  TDateTime GetLastAccess() const { return FLastAccess; }
  void SetLastAccess(const TDateTime & Value) { FLastAccess = Value; }
  bool GetIsSymLink() const { return FIsSymLink; }
  UnicodeString GetLinkTo() const { return FLinkTo; }
  void SetLinkTo(UnicodeString Value) { FLinkTo = Value; }
  TRights * GetRights() const { return FRights; }
  UnicodeString GetHumanRights() const { return FHumanRights; }
  void SetHumanRights(UnicodeString Value) { FHumanRights = Value; }
  TTerminal * GetTerminal() const { return FTerminal; }
  void SetFullFileName(UnicodeString Value) { FFullFileName = Value; }

private:
  void Init();
};

class NB_CORE_EXPORT TRemoteDirectoryFile : public TRemoteFile
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRemoteDirectoryFile); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteDirectoryFile) || TRemoteFile::is(Kind); }
public:
  TRemoteDirectoryFile();
  explicit TRemoteDirectoryFile(TObjectClassId Kind);
  void Init();
  virtual ~TRemoteDirectoryFile() {}
};

class NB_CORE_EXPORT TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRemoteParentDirectory); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteParentDirectory) || TRemoteDirectoryFile::is(Kind); }
public:
  explicit TRemoteParentDirectory(TTerminal * ATerminal);
  virtual ~TRemoteParentDirectory() {}
};

class NB_CORE_EXPORT TRemoteFileList : public TObjectList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRemoteFileList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteFileList) || TObjectList::is(Kind); }
protected:
  UnicodeString FDirectory;
  TDateTime FTimestamp;
public:
  TRemoteFile * GetFile(Integer Index) const;
  virtual void SetDirectory(UnicodeString Value);
  UnicodeString GetFullDirectory() const;
  Boolean GetIsRoot() const;
  TRemoteFile * GetParentDirectory();
  UnicodeString GetParentPath() const;
  int64_t GetTotalSize() const;
  virtual void AddFiles(const TRemoteFileList * AFileList);

public:
  TRemoteFileList();
  explicit TRemoteFileList(TObjectClassId Kind);
  virtual ~TRemoteFileList() { Reset(); }
  virtual void Reset();
  TRemoteFile * FindFile(UnicodeString AFileName) const;
  virtual void DuplicateTo(TRemoteFileList * Copy) const;
  virtual void AddFile(TRemoteFile * AFile);

  static TStrings * CloneStrings(TStrings * List);

#if 0
  __property UnicodeString Directory = { read = FDirectory, write = SetDirectory };
  __property TRemoteFile * Files[Integer Index] = { read = GetFiles };
  __property UnicodeString FullDirectory  = { read=GetFullDirectory };
  __property Boolean IsRoot = { read = GetIsRoot };
  __property UnicodeString ParentPath = { read = GetParentPath };
  __property __int64 TotalSize = { read = GetTotalSize };
  __property TDateTime Timestamp = { read = FTimestamp };
#endif // #if 0

  UnicodeString GetDirectory() const { return FDirectory; }
  TDateTime GetTimestamp() const { return FTimestamp; }
};

class NB_CORE_EXPORT TRemoteDirectory : public TRemoteFileList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TWebDAVFileSystem;
NB_DISABLE_COPY(TRemoteDirectory)
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRemoteDirectory); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteDirectory) || TRemoteFileList::is(Kind); }
private:
  TTerminal * FTerminal;
  TRemoteFile * FParentDirectory;
  TRemoteFile * FThisDirectory;
  Boolean FIncludeParentDirectory;
  Boolean FIncludeThisDirectory;
public:
  virtual void SetDirectory(UnicodeString Value);
  Boolean GetLoaded() const;
  void SetIncludeParentDirectory(Boolean Value);
  void SetIncludeThisDirectory(Boolean Value);
  void ReleaseRelativeDirectories();
public:
  explicit TRemoteDirectory(TTerminal * ATerminal, TRemoteDirectory * Template = nullptr);
  virtual ~TRemoteDirectory();
  virtual void AddFile(TRemoteFile * AFile) override;
  virtual void DuplicateTo(TRemoteFileList * Copy) const override;
  virtual void Reset() override;
#if 0
  __property TTerminal * Terminal = { read = FTerminal, write = FTerminal };
  __property Boolean IncludeParentDirectory = { read = FIncludeParentDirectory, write = SetIncludeParentDirectory };
  __property Boolean IncludeThisDirectory = { read = FIncludeThisDirectory, write = SetIncludeThisDirectory };
  __property Boolean Loaded = { read = GetLoaded };
  __property TRemoteFile * ParentDirectory = { read = FParentDirectory };
  __property TRemoteFile * ThisDirectory = { read = FThisDirectory };
#endif // #if 0

  TTerminal * GetTerminal() const { return FTerminal; }
  void SetTerminal(TTerminal * Value) { FTerminal = Value; }
  Boolean GetIncludeParentDirectory() const { return FIncludeParentDirectory; }
  Boolean GetIncludeThisDirectory() const { return FIncludeThisDirectory; }
  TRemoteFile * GetParentDirectory() const { return FParentDirectory; }
  TRemoteFile * GetThisDirectory() const { return FThisDirectory; }
  TStrings * GetSelectedFiles() const;
};

class TRemoteDirectoryCache : private TStringList
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TRemoteDirectoryCache)
public:
  TRemoteDirectoryCache();
  virtual ~TRemoteDirectoryCache();
  bool HasFileList(UnicodeString Directory) const;
  bool HasNewerFileList(UnicodeString Directory, const TDateTime & Timestamp) const;
  bool GetFileList(UnicodeString Directory,
    TRemoteFileList * FileList) const;
  void AddFileList(TRemoteFileList * FileList);
  void ClearFileList(UnicodeString Directory, bool SubDirs);
  void Clear();

#if 0
  __property bool IsEmpty = { read = GetIsEmpty };
#endif // #if 0
  bool GetIsEmpty() const { return GetIsEmptyPrivate(); }

protected:
  virtual void Delete(intptr_t Index);

private:
  TCriticalSection FSection;
  bool GetIsEmptyPrivate() const;
  void DoClearFileList(UnicodeString Directory, bool SubDirs);
};

class TRemoteDirectoryChangesCache : private TStringList
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TRemoteDirectoryChangesCache(intptr_t MaxSize);
  virtual ~TRemoteDirectoryChangesCache(){}

  void AddDirectoryChange(UnicodeString SourceDir,
    UnicodeString Change, UnicodeString TargetDir);
  void ClearDirectoryChange(UnicodeString SourceDir);
  void ClearDirectoryChangeTarget(UnicodeString TargetDir);
  bool GetDirectoryChange(UnicodeString SourceDir,
    UnicodeString Change, UnicodeString & TargetDir) const;
  void Clear();

  void Serialize(UnicodeString & Data) const;
  void Deserialize(UnicodeString Data);

#if 0
  __property bool IsEmpty = { read = GetIsEmpty };
#endif // #if 0
  bool GetIsEmpty() const { return GetIsEmptyPrivate(); }

private:
  static bool DirectoryChangeKey(UnicodeString SourceDir,
    UnicodeString Change, UnicodeString & Key);
  bool GetIsEmptyPrivate() const;
  void SetValue(UnicodeString Name, UnicodeString Value);
  UnicodeString GetValue(UnicodeString Name) const { return TStringList::GetValue(Name); }
  UnicodeString GetValue(UnicodeString Name);

  intptr_t FMaxSize;
};

class NB_CORE_EXPORT TRights : public TObject
{
public:
  static const intptr_t TextLen = 9;
  static const wchar_t UndefSymbol = L'$';
  static const wchar_t UnsetSymbol = L'-';
  static const wchar_t BasicSymbols[];
  static const wchar_t CombinedSymbols[];
  static const wchar_t ExtendedSymbols[];
  static const wchar_t ModeGroups[];

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
    rfSpecials =  07000, rfAllSpecials = 07777,
  };

  enum TUnsupportedFlag
  {
    rfDirectory = 040000,
  };

  enum TState
  {
    rsNo,
    rsYes,
    rsUndef,
  };

public:
  static TFlag RightToFlag(TRight Right);

  TRights();
  TRights(const TRights & Source);
  explicit TRights(uint16_t ANumber);
  void Assign(const TRights * Source);
  void AddExecute();
  void AllUndef();

  bool operator==(const TRights & rhr) const;
  bool operator==(uint16_t rhr) const;
  bool operator!=(const TRights & rhr) const;
  TRights & operator=(const TRights & rhr);
  TRights & operator=(uint16_t rhr);
  TRights operator~() const;
  TRights operator&(uint16_t rhr) const;
  TRights operator&(const TRights & rhr) const;
  TRights & operator&=(uint16_t rhr);
  TRights & operator&=(const TRights & rhr);
  TRights operator|(uint16_t rhr) const;
  TRights operator|(const TRights & rhr) const;
  TRights & operator|=(uint16_t rhr);
  TRights & operator|=(const TRights & rhr);
  operator uint16_t() const;
  operator uint32_t() const;

#if 0
  __property bool AllowUndef = { read = FAllowUndef, write = SetAllowUndef };
  __property bool IsUndef = { read = GetIsUndef };
  __property UnicodeString ModeStr = { read = GetModeStr };
  __property UnicodeString SimplestStr = { read = GetSimplestStr };
  __property UnicodeString Octal = { read = GetOctal, write = SetOctal };
  __property unsigned short Number = { read = GetNumber, write = SetNumber };
  __property unsigned short NumberSet = { read = FSet };
  __property unsigned short NumberUnset = { read = FUnset };
  __property unsigned long NumberDecadic = { read = GetNumberDecadic };
  __property bool ReadOnly = { read = GetReadOnly, write = SetReadOnly };
  __property bool Right[TRight Right] = { read = GetRight, write = SetRight };
  __property TState RightUndef[TRight Right] = { read = GetRightUndef, write = SetRightUndef };
  __property UnicodeString Text = { read = GetText, write = SetText };
  __property bool Unknown = { read = FUnknown };
#endif // #if 0

private:
  UnicodeString FText;
  uint16_t FSet;
  uint16_t FUnset;
  bool FAllowUndef;
  bool FUnknown;

public:
  bool GetIsUndef() const;
  UnicodeString GetModeStr() const;
  UnicodeString GetSimplestStr() const;
  void SetNumber(uint16_t Value);
  UnicodeString GetText() const;
  void SetText(UnicodeString Value);
  void SetOctal(UnicodeString AValue);
  uint16_t GetNumber() const;
  uint16_t GetNumberSet() const { return FSet; }
  uint16_t GetNumberUnset() const { return FUnset; }
  uint32_t GetNumberDecadic() const;
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
};

enum TValidProperty
{
  vpRights = 0x1,
  vpGroup = 0x2,
  vpOwner = 0x4,
  vpModification = 0x8,
  vpLastAccess = 0x10,
};

// FIXME
class NB_CORE_EXPORT TValidProperties // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
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
  bool operator==(const TValidProperties & rhs) const
  {
    return FValue == rhs.FValue;
  }
  bool operator!=(const TValidProperties & rhs) const
  {
    return !(operator==(rhs));
  }
  TValidProperties & operator<<(const TValidProperty Value)
  {
    FValue |= Value;
    return *this;
  }
  TValidProperties & operator>>(const TValidProperty Value)
  {
    FValue &= ~(static_cast<int64_t>(Value));
    return *this;
  }
  bool Empty() const
  {
    return FValue == 0;
  }

private:
  int64_t FValue;
};

#if 0
enum TValidProperty { vpRights, vpGroup, vpOwner, vpModification, vpLastAccess };
typedef Set<TValidProperty, vpRights, vpLastAccess> TValidProperties;
#endif // #if 0
class TRemoteProperties : public TObject
{
public:
  static inline bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRemoteProperties); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteProperties) || TObject::is(Kind); }
public:
  TValidProperties Valid;
  TRights Rights;
  TRemoteToken Group;
  TRemoteToken Owner;
  int64_t Modification; // unix time
  int64_t LastAccess; // unix time
  bool Recursive;
  bool AddXToDirectories;

  TRemoteProperties();
  TRemoteProperties(const TRemoteProperties & rhp);
  bool operator==(const TRemoteProperties & rhp) const;
  bool operator!=(const TRemoteProperties & rhp) const;
  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  static TRemoteProperties CommonProperties(TStrings * AFileList);
  static TRemoteProperties ChangedProperties(
    const TRemoteProperties & OriginalProperties, TRemoteProperties & NewProperties);

public:
  TRemoteProperties & operator=(const TRemoteProperties & other);
};

#if 0
// moved to base/Common.h
NB_CORE_EXPORT bool IsUnixStyleWindowsPath(UnicodeString APath);
NB_CORE_EXPORT bool UnixIsAbsolutePath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixIncludeTrailingBackslash(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExcludeTrailingBackslash(UnicodeString APath, bool Simple = false);
NB_CORE_EXPORT UnicodeString SimpleUnixExcludeTrailingBackslash(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixCombinePaths(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT UnicodeString UnixExtractFileDir(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFilePath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileName(UnicodeString APath);
NB_CORE_EXPORT UnicodeString UnixExtractFileExt(UnicodeString APath);
NB_CORE_EXPORT Boolean UnixSamePath(UnicodeString APath1, UnicodeString APath2);
NB_CORE_EXPORT bool UnixIsChildPath(UnicodeString AParent, UnicodeString AChild);
NB_CORE_EXPORT bool ExtractCommonPath(const TStrings * AFiles, OUT UnicodeString & APath);
NB_CORE_EXPORT bool UnixExtractCommonPath(const TStrings * AFiles, OUT UnicodeString & APath);
NB_CORE_EXPORT UnicodeString ExtractFileName(UnicodeString APath, bool Unix);
NB_CORE_EXPORT bool IsUnixRootPath(UnicodeString APath);
NB_CORE_EXPORT bool IsUnixHiddenFile(UnicodeString APath);
NB_CORE_EXPORT UnicodeString AbsolutePath(UnicodeString Base, UnicodeString APath);
NB_CORE_EXPORT UnicodeString FromUnixPath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString ToUnixPath(UnicodeString APath);
NB_CORE_EXPORT UnicodeString MinimizeName(UnicodeString AFileName, intptr_t MaxLen, bool Unix);
NB_CORE_EXPORT UnicodeString MakeFileList(const TStrings * AFileList);
NB_CORE_EXPORT TDateTime ReduceDateTimePrecision(const TDateTime & ADateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
NB_CORE_EXPORT UnicodeString UserModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT UnicodeString ModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
NB_CORE_EXPORT int FakeFileImageIndex(UnicodeString AFileName, uint32_t Attrs = INVALID_FILE_ATTRIBUTES,
  UnicodeString * TypeName = nullptr);
NB_CORE_EXPORT bool SameUserName(UnicodeString UserName1, UnicodeString UserName2);
NB_CORE_EXPORT UnicodeString FormatMultiFilesToOneConfirmation(UnicodeString ATarget, bool Unix);

#endif // #if 0
