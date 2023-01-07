
#pragma once

#include <rdestl/vector.h>
#include <rdestl/map.h>

#include <Sysutils.hpp>
#include <Common.h>
//---------------------------------------------------------------------------
#define SYMLINKSTR L" -> "
#define ROOTDIRECTORY L"/"
#define FILETYPE_DEFAULT L'-'
#define FILETYPE_SYMLINK L'L'
#define FILETYPE_DIRECTORY L'D'
#define PARTIAL_EXT L".filepart"
//---------------------------------------------------------------------------
class TTerminal;
class TRights;
class TRemoteFileList;
class THierarchicalStorage;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TRemoteToken : public TObject
{
public:
  TRemoteToken() noexcept;
  TRemoteToken(const TRemoteToken &rhs) noexcept;
  explicit TRemoteToken(UnicodeString Name) noexcept;

  void Clear();

  bool operator==(const TRemoteToken &rhs) const;
  bool operator!=(const TRemoteToken &rhs) const;
  TRemoteToken &operator=(const TRemoteToken &rhs);

  intptr_t Compare(const TRemoteToken &rhs) const;

  __property UnicodeString Name = { read = FName, write = FName };
  UnicodeString& Name{FName};
  __property bool NameValid = { read = GetNameValid };
  ROProperty<bool> NameValid{nb::bind(&TRemoteToken::GetNameValid, this)};
  __property uintptr_t ID = { read = FID, write = SetID };
  __property bool IDValid = { read = FIDValid };
  ROProperty<bool> IDValid{nb::bind(&TRemoteToken::GetIDValid, this)};
  __property bool IsSet  = { read = GetIsSet };
  ROProperty<bool> IsSet{nb::bind(&TRemoteToken::GetIsSet, this)};
  __property UnicodeString LogText = { read = GetLogText };
  ROProperty<UnicodeString> LogText{nb::bind(&TRemoteToken::GetLogText, this)};
  __property UnicodeString DisplayText = { read = GetDisplayText };
  ROProperty<UnicodeString> DisplayText{nb::bind(&TRemoteToken::GetDisplayText, this)};

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString Value) { FName = Value; }
  intptr_t GetID() const { return FID; }
  bool GetIDValid() const { return FIDValid; }

private:
  UnicodeString FName;
  intptr_t FID{0};
  bool FIDValid{false};

public:
  void SetID(intptr_t Value);
  bool GetNameValid() const;
  bool GetIsSet() const;
  UnicodeString GetDisplayText() const;
  UnicodeString GetLogText() const;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TRemoteTokenList : public TObject
{
public:
  TRemoteTokenList *Duplicate() const;
  void Clear();
  void Add(const TRemoteToken &Token);
  void AddUnique(const TRemoteToken &Token);
  bool Exists(UnicodeString Name) const;
  const TRemoteToken *Find(uintptr_t ID) const;
  const TRemoteToken *Find(UnicodeString Name) const;
  void Log(TTerminal *Terminal, const wchar_t *Title);

  intptr_t GetCount() const;
  const TRemoteToken *Token(intptr_t Index) const;

private:
  using TTokens = nb::vector_t<TRemoteToken>;
  using TNameMap = nb::map_t<UnicodeString, size_t>;
  using TIDMap = nb::map_t<intptr_t, size_t>;
  TTokens FTokens;
  mutable TNameMap FNameMap;
  mutable TIDMap FIDMap;
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TRemoteFile);
class NB_CORE_EXPORT TRemoteFile : public TPersistent
{
  NB_DISABLE_COPY(TRemoteFile)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TRemoteFile); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteFile) || TPersistent::is(Kind); }
private:
  TRemoteFileList *FDirectory{nullptr};
  TRemoteToken FOwner;
  TModificationFmt FModificationFmt{mfFull};
  UnicodeString FFileName;
  UnicodeString FDisplayName;
  TDateTime FModification{};
  TDateTime FLastAccess{};
  TRemoteToken FGroup{};
  TRemoteFile *FLinkedFile{nullptr};
  TRemoteFile *FLinkedByFile{nullptr};
  TRights *FRights{nullptr};
  TTerminal *FTerminal{nullptr};
  UnicodeString FLinkTo;
  UnicodeString FHumanRights;
  UnicodeString FFullFileName;
  UnicodeString FTypeName;
  bool FIsEncrypted{false};
  int64_t FSize{0};
  int64_t FINodeBlocks{0};
  intptr_t FIconIndex{0};
  intptr_t FIsHidden{0};
  wchar_t FType{0};
  bool FIsSymLink{false};
  bool FCyclicLink{false};

public:
  intptr_t GetAttr() const;
  bool GetBrokenLink() const;
  bool GetIsDirectory() const;
  TRemoteFile *GetLinkedFile() const;
  void SetLinkedFile(TRemoteFile *Value);
  UnicodeString GetModificationStr() const;
  void SetModification(const TDateTime Value);
  void SetListingStr(UnicodeString Value);
  UnicodeString GetListingStr() const;
  UnicodeString GetRightsStr() const;
  bool GetIsHidden() const;
  wchar_t GetType() const;
  void SetType(wchar_t AType);
  void SetTerminal(TTerminal *Value);
  void SetRights(TRights *Value);
  UnicodeString GetFullFileName() const;
  bool GetHaveFullFileName() const;
  intptr_t GetIconIndex() const;
  UnicodeString GetTypeName() const;
  void SetIsHidden(bool Value);
  bool GetIsParentDirectory() const;
  bool GetIsThisDirectory() const;
  bool GetIsInaccesibleDirectory() const;
  UnicodeString GetExtension() const;
  bool GetIsEncrypted() const { return FIsEncrypted; }
  UnicodeString GetUserModificationStr() const;
  void LoadTypeInfo() const;
  int64_t GetSize() const;

protected:
  void FindLinkedFile();

public:
  explicit TRemoteFile(TRemoteFile *ALinkedByFile = nullptr) noexcept;
  explicit TRemoteFile(TObjectClassId Kind, TRemoteFile *ALinkedByFile = nullptr) noexcept;
  virtual ~TRemoteFile() noexcept;
  TRemoteFile *Duplicate(bool Standalone = true) const;

  void ShiftTimeInSeconds(int64_t Seconds);
  bool GetIsTimeShiftingApplicable() const;
  void Complete();
  void SetEncrypted();

  static bool GetIsTimeShiftingApplicable(TModificationFmt ModificationFmt);
  static void ShiftTimeInSeconds(TDateTime &DateTime, TModificationFmt ModificationFmt, int64_t Seconds);

  __property int Attr = { read = GetAttr };
  __property bool BrokenLink = { read = GetBrokenLink };
  __property TRemoteFileList * Directory = { read = FDirectory, write = FDirectory };
  __property UnicodeString RightsStr = { read = GetRightsStr };
  __property __int64 Size = { read = GetSize, write = FSize };
  RWProperty<int64_t> Size{nb::bind(&TRemoteFile::GetSize, this), nb::bind(&TRemoteFile::SetSize, this)};
  __property TRemoteToken Owner = { read = FOwner, write = FOwner };
  TRemoteToken& Owner{FOwner};
  __property TRemoteToken Group = { read = FGroup, write = FGroup };
  TRemoteToken& Group{FGroup};
  __property UnicodeString FileName = { read = FFileName, write = FFileName };
  RWProperty<UnicodeString> FileName{nb::bind(&TRemoteFile::GetFileName, this), nb::bind(&TRemoteFile::SetFileName, this)};
  __property UnicodeString DisplayName = { read = FDisplayName, write = FDisplayName };
  __property int INodeBlocks = { read = FINodeBlocks };
  __property TDateTime Modification = { read = FModification, write = SetModification };
  RWProperty<TDateTime> Modification{nb::bind(&TRemoteFile::GetModification, this), nb::bind(&TRemoteFile::SetModification, this)};
  __property UnicodeString ModificationStr = { read = GetModificationStr };
  __property UnicodeString UserModificationStr = { read = GetUserModificationStr };
  __property TModificationFmt ModificationFmt = { read = FModificationFmt, write = FModificationFmt };
  __property TDateTime LastAccess = { read = FLastAccess, write = FLastAccess };
  __property bool IsSymLink = { read = FIsSymLink };
  ROProperty<bool> IsSymLink{nb::bind(&TRemoteFile::GetIsSymLink, this)};
  __property bool IsDirectory = { read = GetIsDirectory };
  ROProperty<bool> IsDirectory{nb::bind(&TRemoteFile::GetIsDirectory, this)};
  __property TRemoteFile * LinkedFile = { read = GetLinkedFile, write = SetLinkedFile };
  RWProperty<TRemoteFile *> LinkedFile{nb::bind(&TRemoteFile::GetLinkedFile, this), nb::bind(&TRemoteFile::SetLinkedFile, this)};
  __property UnicodeString LinkTo = { read = FLinkTo, write = FLinkTo };
  UnicodeString& LinkTo{FLinkTo};
  __property UnicodeString ListingStr = { read = GetListingStr, write = SetListingStr };
  RWProperty<UnicodeString> ListingStr{nb::bind(&TRemoteFile::GetListingStr, this), nb::bind(&TRemoteFile::SetListingStr, this)};
  __property TRights *Rights = { read = FRights, write = SetRights };
  RWProperty<TRights *> Rights{nb::bind(&TRemoteFile::GetRights, this), nb::bind(&TRemoteFile::SetRights, this)};
  __property UnicodeString HumanRights = { read = FHumanRights, write = FHumanRights };
  __property TTerminal *Terminal = { read = FTerminal, write = SetTerminal };
  RWProperty<TTerminal *> Terminal{nb::bind(&TRemoteFile::GetTerminal, this), nb::bind(&TRemoteFile::SetTerminal, this)};
  __property wchar_t Type = { read = GetType, write = SetType };
  RWProperty<wchar_t> Type{nb::bind(&TRemoteFile::GetType, this), nb::bind(&TRemoteFile::SetType, this)};
  __property UnicodeString FullFileName  = { read = GetFullFileName, write = FFullFileName };
  RWProperty<UnicodeString> FullFileName{nb::bind(&TRemoteFile::GetFullFileName, this), nb::bind(&TRemoteFile::SetFullFileName, this)};
  __property bool HaveFullFileName  = { read = GetHaveFullFileName };
  __property int IconIndex = { read = GetIconIndex };
  __property UnicodeString TypeName = { read = GetTypeName };
  __property bool IsHidden = { read = GetIsHidden, write = SetIsHidden };
  ROProperty<bool> IsHidden{nb::bind(&TRemoteFile::GetIsHidden, this)};
  __property bool IsParentDirectory = { read = GetIsParentDirectory };
  ROProperty<bool> IsParentDirectory{nb::bind(&TRemoteFile::GetIsParentDirectory, this)};
  __property bool IsThisDirectory = { read = GetIsThisDirectory };
  ROProperty<bool> IsThisDirectory{nb::bind(&TRemoteFile::GetIsThisDirectory, this)};
  __property bool IsInaccesibleDirectory  = { read = GetIsInaccesibleDirectory };
  ROProperty<bool> IsInaccesibleDirectory{nb::bind(&TRemoteFile::GetIsInaccesibleDirectory, this)};
  __property UnicodeString Extension  = { read = GetExtension };
  ROProperty<UnicodeString> Extension{nb::bind(&TRemoteFile::GetExtension, this)};
  __property bool IsEncrypted  = { read = FIsEncrypted };
  ROProperty<bool> IsEncrypted{nb::bind(&TRemoteFile::GetIsEncrypted, this)};

  TRemoteFileList *GetDirectory() const { return FDirectory; }
  void SetDirectory(TRemoteFileList *Value) { FDirectory = Value; }
  void SetSize(int64_t Value) { FSize = Value; }
  const TRemoteToken &GetFileOwner() const { return FOwner; }
  TRemoteToken &GetFileOwner() { return FOwner; }
  void SetFileOwner(const TRemoteToken &Value) { FOwner = Value; }
  const TRemoteToken &GetFileGroup() const { return FGroup; }
  TRemoteToken &GetFileGroup() { return FGroup; }
  void SetFileGroup(const TRemoteToken &Value) { FGroup = Value; }
  UnicodeString GetFileName() const { return FFileName; }
  void SetFileName(const UnicodeString Value) { FFileName = Value; }
  UnicodeString GetDisplayName() const { return FDisplayName; }
  void SetDisplayName(const UnicodeString Value) { FDisplayName = Value; }
  TDateTime GetModification() const { return FModification; }
  TModificationFmt GetModificationFmt() const { return FModificationFmt; }
  void SetModificationFmt(TModificationFmt Value) { FModificationFmt = Value; }
  TDateTime GetLastAccess() const { return FLastAccess; }
  void SetLastAccess(const TDateTime &Value) { FLastAccess = Value; }
  bool GetIsSymLink() const { return FIsSymLink; }
  UnicodeString GetLinkTo() const { return FLinkTo; }
  void SetLinkTo(UnicodeString Value) { FLinkTo = Value; }
  TRights *GetRights() const { return FRights; }
  UnicodeString GetHumanRights() const { return FHumanRights; }
  void SetHumanRights(const UnicodeString Value) { FHumanRights = Value; }
  TTerminal *GetTerminal() const { return FTerminal; }
  void SetFullFileName(const UnicodeString Value) { FFullFileName = Value; }

private:
  void Init();
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TRemoteDirectoryFile);
class NB_CORE_EXPORT TRemoteDirectoryFile : public TRemoteFile
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TRemoteDirectoryFile); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteDirectoryFile) || TRemoteFile::is(Kind); }
public:
  TRemoteDirectoryFile() noexcept;
  explicit TRemoteDirectoryFile(TObjectClassId Kind) noexcept;
  void Init();
  virtual ~TRemoteDirectoryFile() = default;
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TRemoteParentDirectory);
class NB_CORE_EXPORT TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TRemoteParentDirectory); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteParentDirectory) || TRemoteDirectoryFile::is(Kind); }
public:
  TRemoteParentDirectory() = delete;
  explicit TRemoteParentDirectory(TTerminal *ATerminal) noexcept;
  virtual ~TRemoteParentDirectory() = default;
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TRemoteFileList);
class NB_CORE_EXPORT TRemoteFileList : public TObjectList
{
  friend class TSCPFileSystem;
  friend class TSFTPFileSystem;
  friend class TFTPFileSystem;
  friend class TWebDAVFileSystem;
  friend class TS3FileSystem;
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TRemoteFileList); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteFileList) || TObjectList::is(Kind); }
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
  virtual void AddFiles(const TRemoteFileList *AFileList);

public:
  TRemoteFileList() noexcept;
  explicit TRemoteFileList(TObjectClassId Kind) noexcept;
  virtual ~TRemoteFileList() noexcept { TRemoteFileList::Reset(); }
  virtual void Reset();
  TRemoteFile *FindFile(UnicodeString AFileName) const;
  virtual void DuplicateTo(TRemoteFileList *Copy) const;
  virtual void AddFile(TRemoteFile *AFile);

  static TStrings * CloneStrings(TStrings *List);

  __property UnicodeString Directory = { read = FDirectory, write = SetDirectory };
  RWProperty<UnicodeString> Directory{nb::bind(&TRemoteFileList::GetDirectory, this), nb::bind(&TRemoteFileList::SetDirectory, this)};
  __property TRemoteFile * Files[Integer Index] = { read = GetFiles };
  __property UnicodeString FullDirectory  = { read=GetFullDirectory };
  ROProperty<UnicodeString> FullDirectory{nb::bind(&TRemoteFileList::GetFullDirectory, this)};
  __property Boolean IsRoot = { read = GetIsRoot };
  ROProperty<Boolean> IsRoot{nb::bind(&TRemoteFileList::GetIsRoot, this)};
  __property UnicodeString ParentPath = { read = GetParentPath };
  ROProperty<UnicodeString> ParentPath{nb::bind(&TRemoteFileList::GetParentPath, this)};
  __property __int64 TotalSize = { read = GetTotalSize };
  ROProperty<int64_t> TotalSize{nb::bind(&TRemoteFileList::GetTotalSize, this)};
  __property TDateTime Timestamp = { read = FTimestamp };
  const TDateTime& Timestamp{FTimestamp};

  UnicodeString GetDirectory() const { return FDirectory; }
  TDateTime GetTimestamp() const { return FTimestamp; }
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TRemoteDirectory);
class NB_CORE_EXPORT TRemoteDirectory : public TRemoteFileList
{
  friend class TSCPFileSystem;
  friend class TSFTPFileSystem;
  friend class TWebDAVFileSystem;
  NB_DISABLE_COPY(TRemoteDirectory)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TRemoteDirectory); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteDirectory) || TRemoteFileList::is(Kind); }
private:
  TTerminal *FTerminal{nullptr};
  TRemoteFile *FParentDirectory{nullptr};
  TRemoteFile *FThisDirectory{nullptr};
  Boolean FIncludeParentDirectory{false};
  Boolean FIncludeThisDirectory{false};
public:
  void SetDirectory(const UnicodeString Value) override;
  Boolean GetLoaded() const;
  void SetIncludeParentDirectory(Boolean Value);
  void SetIncludeThisDirectory(Boolean Value);
  void ReleaseRelativeDirectories();
public:
  TRemoteDirectory() = delete;
  explicit TRemoteDirectory(TTerminal *ATerminal, TRemoteDirectory *Template = nullptr) noexcept;
  virtual ~TRemoteDirectory() noexcept;
  void AddFile(TRemoteFile *AFile) override;
  void DuplicateTo(TRemoteFileList *Copy) const override;
  void Reset() override;

  __property TTerminal * Terminal = { read = FTerminal, write = FTerminal };
  __property Boolean IncludeParentDirectory = { read = FIncludeParentDirectory, write = SetIncludeParentDirectory };
  __property Boolean IncludeThisDirectory = { read = FIncludeThisDirectory, write = SetIncludeThisDirectory };
  __property Boolean Loaded = { read = GetLoaded };
  __property TRemoteFile * ParentDirectory = { read = FParentDirectory };
  __property TRemoteFile * ThisDirectory = { read = FThisDirectory };

  TTerminal *GetTerminal() const { return FTerminal; }
  void SetTerminal(TTerminal *Value) { FTerminal = Value; }
  Boolean GetIncludeParentDirectory() const { return FIncludeParentDirectory; }
  Boolean GetIncludeThisDirectory() const { return FIncludeThisDirectory; }
  TRemoteFile *GetParentDirectory() const { return FParentDirectory; }
  TRemoteFile *GetThisDirectory() const { return FThisDirectory; }
  TStrings *GetSelectedFiles() const;
};
//---------------------------------------------------------------------------
class TRemoteDirectoryCache : private TStringList
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TRemoteDirectoryCache)
public:
  TRemoteDirectoryCache() noexcept;
  virtual ~TRemoteDirectoryCache() noexcept;
  bool HasFileList(UnicodeString Directory) const;
  bool HasNewerFileList(UnicodeString Directory, const TDateTime &Timestamp) const;
  bool GetFileList(UnicodeString Directory,
    TRemoteFileList *FileList) const;
  void AddFileList(TRemoteFileList *FileList);
  void ClearFileList(UnicodeString Directory, bool SubDirs);
  void Clear();

  __property bool IsEmpty = { read = GetIsEmpty };
  bool GetIsEmpty() const { return GetIsEmptyPrivate(); }

protected:
  void Delete(intptr_t Index) override;

private:
  TCriticalSection FSection;
  bool GetIsEmptyPrivate() const;
  void DoClearFileList(UnicodeString Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : private TStringList
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TRemoteDirectoryChangesCache() = delete;
  explicit TRemoteDirectoryChangesCache(intptr_t MaxSize) noexcept;
  virtual ~TRemoteDirectoryChangesCache() = default;

  void AddDirectoryChange(UnicodeString SourceDir,
    UnicodeString Change, UnicodeString TargetDir);
  void ClearDirectoryChange(UnicodeString SourceDir);
  void ClearDirectoryChangeTarget(UnicodeString TargetDir);
  bool GetDirectoryChange(UnicodeString SourceDir,
    UnicodeString Change, UnicodeString &TargetDir) const;
  void Clear();

  void Serialize(UnicodeString &Data) const;
  void Deserialize(UnicodeString Data);

  __property bool IsEmpty = { read = GetIsEmpty };
  bool GetIsEmpty() const { return GetIsEmptyPrivate(); }

private:
  static bool DirectoryChangeKey(UnicodeString SourceDir,
    UnicodeString Change, UnicodeString &Key);
  bool GetIsEmptyPrivate() const;
  void SetValue(UnicodeString Name, UnicodeString Value);
  UnicodeString GetValue(UnicodeString Name) const { return TStringList::GetValue(Name); }
  UnicodeString GetValue(UnicodeString Name);

  intptr_t FMaxSize{0};
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TRights : public TObject
{
public:
  static const intptr_t TextLen = 9;
  static const wchar_t UndefSymbol = L'$';
  static const wchar_t UnsetSymbol = L'-';
  // Used by Win32-OpenSSH for permissions that are not applicable on Windows.
  // See strmode() in contrib\win32\win32compat\misc.c
  static const wchar_t UnsetSymbolWin = L'*';
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

  TRights() noexcept;
  TRights(const TRights &Source) noexcept;
  explicit TRights(uint16_t ANumber) noexcept;
  void Assign(const TRights *Source);
  void AddExecute();
  void AllUndef();

  bool operator==(const TRights &rhr) const;
  bool operator==(uint16_t rhr) const;
  bool operator==(TFlag rhr) const;
  bool operator!=(const TRights &rhr) const;
  bool operator!=(const TFlag rhr) const;
  TRights &operator=(const TRights &rhr);
  TRights &operator=(uint16_t rhr);
  TRights operator~() const;
  TRights operator&(uint16_t rhr) const;
  TRights operator&(const TRights &rhr) const;
  TRights operator&(TFlag rhr) const;
  TRights &operator&=(uint16_t rhr);
  TRights &operator&=(const TRights &rhr);
  TRights &operator&=(TFlag rhr);
  TRights operator|(uint16_t rhr) const;
  TRights operator|(const TRights &rhr) const;
  TRights &operator|=(uint16_t rhr);
  TRights &operator|=(const TRights &rhr);
  operator uint16_t() const;
  operator uint32_t() const;

  __property bool AllowUndef = { read = FAllowUndef, write = SetAllowUndef };
  __property bool IsUndef = { read = GetIsUndef };
  __property UnicodeString ModeStr = { read = GetModeStr };
  __property UnicodeString SimplestStr = { read = GetSimplestStr };
  __property UnicodeString Octal = { read = GetOctal, write = SetOctal };
  RWProperty<UnicodeString> Octal{nb::bind(&TRights::GetOctal, this), nb::bind(&TRights::SetOctal, this)};
  __property unsigned short Number = { read = GetNumber, write = SetNumber };
  __property unsigned short NumberSet = { read = FSet };
  __property unsigned short NumberUnset = { read = FUnset };
  __property unsigned long NumberDecadic = { read = GetNumberDecadic };
  __property bool ReadOnly = { read = GetReadOnly, write = SetReadOnly };
  __property bool Right[TRight Right] = { read = GetRight, write = SetRight };
  __property TState RightUndef[TRight Right] = { read = GetRightUndef, write = SetRightUndef };
  __property UnicodeString Text = { read = GetText, write = SetText };
  RWProperty<UnicodeString> Text{nb::bind(&TRights::GetText, this), nb::bind(&TRights::SetText, this)};
  __property bool Unknown = { read = FUnknown };
  const bool& Unknown{FUnknown};

private:
  UnicodeString FText;
  uint16_t FSet{0};
  uint16_t FUnset{0};
  bool FAllowUndef{false};
  bool FUnknown{true};

public:
  bool GetIsUndef() const;
  UnicodeString GetModeStr() const;
  UnicodeString GetSimplestStr() const;
  void SetNumber(uint16_t Value);
  UnicodeString GetText() const;
  void SetText(const UnicodeString Value);
  void SetOctal(const UnicodeString AValue);
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
//---------------------------------------------------------------------------
//enum TValidProperty { vpRights, vpGroup, vpOwner, vpModification, vpLastAccess, vpEncrypt };
//typedef Set<TValidProperty, vpRights, vpEncrypt> TValidProperties;
enum TValidProperty
{
  vpRights = 0x1,
  vpGroup = 0x2,
  vpOwner = 0x4,
  vpModification = 0x8,
  vpLastAccess = 0x10,
  vpEncrypt = 0x20,
};

// FIXME
class NB_CORE_EXPORT TValidProperties // : public TObject
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  TValidProperties() = default;
  void Clear()
  {
    FValue = 0;
  }
  bool Contains(TValidProperty Value) const
  {
    return (FValue & Value) != 0;
  }
  bool operator==(const TValidProperties &rhs) const
  {
    return FValue == rhs.FValue;
  }
  bool operator!=(const TValidProperties &rhs) const
  {
    return !(operator==(rhs));
  }
  TValidProperties &operator<<(const TValidProperty Value)
  {
    FValue |= Value;
    return *this;
  }
  TValidProperties &operator>>(const TValidProperty Value)
  {
    FValue &= ~(nb::ToInt64(Value));
    return *this;
  }
  bool Empty() const
  {
    return FValue == 0;
  }

private:
  int64_t FValue{0};
};
//---------------------------------------------------------------------------
#if 0
#endif // #if 0
NB_DEFINE_CLASS_ID(TRemoteProperties);
class TRemoteProperties : public TObject
{
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TRemoteProperties); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRemoteProperties) || TObject::is(Kind); }
public:
  TValidProperties Valid;
  TRights Rights;
  TRemoteToken Group;
  TRemoteToken Owner;
  int64_t Modification{0}; // unix time
  int64_t LastAccess{0}; // unix time
  bool Encrypt{false};
  bool Recursive{false};
  bool AddXToDirectories{false};

  TRemoteProperties();
  TRemoteProperties(const TRemoteProperties &rhp);
  bool operator==(const TRemoteProperties &rhp) const;
  bool operator!=(const TRemoteProperties &rhp) const;
  void Default();
  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage) const;

  static TRemoteProperties CommonProperties(TStrings *AFileList);
  static TRemoteProperties ChangedProperties(
    const TRemoteProperties &OriginalProperties, TRemoteProperties &NewProperties);

public:
  TRemoteProperties &operator=(const TRemoteProperties &other);
};
//---------------------------------------------------------------------------
enum TChecklistAction
{
  saNone,
  saUploadNew,
  saDownloadNew,
  saUploadUpdate,
  saDownloadUpdate,
  saDeleteRemote,
  saDeleteLocal,
};
//---------------------------------------------------------------------------
class TSynchronizeChecklist;
NB_DEFINE_CLASS_ID(TChecklistItem);
class NB_CORE_EXPORT TChecklistItem : public TObject
{
  friend class TTerminal;
  friend class TSynchronizeChecklist;
  NB_DISABLE_COPY(TChecklistItem)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TChecklistItem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TChecklistItem) || TObject::is(Kind); }
public:

  struct NB_CORE_EXPORT TFileInfo : public TObject
  {
    UnicodeString FileName;
    UnicodeString Directory;
    TDateTime Modification;
    TModificationFmt ModificationFmt{mfNone};
    int64_t Size{0};
  };

  TChecklistAction Action{saNone};
  bool IsDirectory{false};
  TFileInfo Local;
  TFileInfo Remote;
  intptr_t ImageIndex{-1};
  bool Checked{true};
  TRemoteFile *RemoteFile{nullptr};

  const UnicodeString& GetFileName() const;
  bool IsRemoteOnly() const { return (Action == saDownloadNew) || (Action == saDeleteRemote); }
  bool IsLocalOnly() const { return (Action == saUploadNew) || (Action == saDeleteLocal); }
  bool HasSize() const { return !IsDirectory || FDirectoryHasSize; }
  int64_t GetSize() const;
  int64_t GetSize(TChecklistAction AAction) const;

  TChecklistItem() noexcept;
  ~TChecklistItem() noexcept;

private:
  FILETIME FLocalLastWriteTime{};
  bool FDirectoryHasSize{false};

//  TChecklistItem() noexcept;
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TSynchronizeChecklist : public TObject
{
  NB_DISABLE_COPY(TSynchronizeChecklist)
  friend class TTerminal;

public:
#if 0
  enum TAction {
    saNone, saUploadNew, saDownloadNew, saUploadUpdate, saDownloadUpdate, saDeleteRemote, saDeleteLocal };
#endif // #if 0
  static const intptr_t ActionCount = saDeleteLocal;

#if 0
  class TItem
  {
  friend class TTerminal;
  friend class TSynchronizeChecklist;

  public:
    struct TFileInfo
    {
      UnicodeString FileName;
      UnicodeString Directory;
      TDateTime Modification;
      TModificationFmt ModificationFmt;
      __int64 Size;
    };

    TAction Action;
    bool IsDirectory;
    TFileInfo Local;
    TFileInfo Remote;
    int ImageIndex;
    bool Checked;
    TRemoteFile * RemoteFile;

    const UnicodeString& GetFileName() const;
    bool IsRemoteOnly() const { return (Action == saDownloadNew) || (Action == saDeleteRemote); }
    bool IsLocalOnly() const { return (Action == saUploadNew) || (Action == saDeleteLocal); }
    bool HasSize() const { return !IsDirectory || FDirectoryHasSize; }
    __int64 GetSize() const;
    __int64 GetSize(TAction AAction) const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;
    bool FDirectoryHasSize;

    TItem();
  };
#endif // #if 0

  typedef nb::vector_t<const TChecklistItem*> TItemList;

  TSynchronizeChecklist() noexcept;
  ~TSynchronizeChecklist() noexcept;

  void Update(const TChecklistItem *Item, bool Check, TChecklistAction Action);
  void UpdateDirectorySize(const TChecklistItem * Item, int64_t Size);
  void Delete(const TChecklistItem* Item);

  static TChecklistAction Reverse(TChecklistAction Action);
  static bool IsItemSizeIrrelevant(TChecklistAction Action);

  __property int Count = { read = GetCount };
  ROProperty<intptr_t> Count{nb::bind(&TSynchronizeChecklist::GetCount, this)};
  __property int CheckedCount = { read = GetCheckedCount };
  ROProperty<intptr_t> CheckedCount{nb::bind(&TSynchronizeChecklist::GetCheckedCount, this)};
  __property const TItem * Item[int Index] = { read = GetItem };

protected:
  __removed TSynchronizeChecklist() noexcept;

  void Sort();
  void Add(TChecklistItem *Item);

public:
  void SetMasks(UnicodeString Value);

  intptr_t GetCount() const;
  intptr_t GetCheckedCount() const;
  const TChecklistItem *GetItem(intptr_t Index) const;

private:
  std::unique_ptr<TList> FList;

  static intptr_t Compare(const void *AItem1, const void *AItem2);
};
//---------------------------------------------------------------------------
class TFileOperationProgressType;
//---------------------------------------------------------------------------
class TSynchronizeProgress
{
public:
  explicit TSynchronizeProgress(const TSynchronizeChecklist * Checklist) noexcept;

  void ItemProcessed(const TChecklistItem* ChecklistItem);
  intptr_t Progress(const TFileOperationProgressType * CurrentItemOperationProgress) const;
  TDateTime TimeLeft(const TFileOperationProgressType * CurrentItemOperationProgress) const;

private:
  const TSynchronizeChecklist * FChecklist{nullptr};
  mutable int64_t FTotalSize{-1};
  int64_t FProcessedSize{0};

  int64_t ItemSize(const TChecklistItem* ChecklistItem) const;
  int64_t GetProcessed(const TFileOperationProgressType * CurrentItemOperationProgress) const;
};
//---------------------------------------------------------------------------

