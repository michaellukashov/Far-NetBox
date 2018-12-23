//---------------------------------------------------------------------------
#pragma once

#include "FileMasks.h"
#include "RemoteFiles.h"
//---------------------------------------------------------------------------
// When adding new options, mind TCopyParamType::GetLogStr()
enum TOperationSide
{
  osLocal,
  osRemote,
  osCurrent,
};

enum TFileNameCase
{
  ncNoChange,
  ncUpperCase,
  ncLowerCase,
  ncFirstUpperCase,
  ncLowerCaseShort,
};

// TScript::OptionProc depend on the order
enum TTransferMode
{
  tmBinary,
  tmAscii,
  tmAutomatic,
};

enum TResumeSupport
{
  rsOn,
  rsSmart,
  rsOff,
};

class THierarchicalStorage;
constexpr const int cpaIncludeMaskOnly = 0x01;
constexpr const int cpaNoTransferMode  = 0x02;
constexpr const int cpaNoIncludeMask   = 0x04;
constexpr const int cpaNoClearArchive  = 0x08;
constexpr const int cpaNoPreserveTime  = 0x10;
constexpr const int cpaNoRights        = 0x20;
constexpr const int cpaNoPreserveReadOnly = 0x40;
constexpr const int cpaNoIgnorePermErrors = 0x80;
constexpr const int cpaNoNewerOnly        = 0x100;
constexpr const int cpaNoRemoveCtrlZ      = 0x200;
constexpr const int cpaNoRemoveBOM        = 0x400;
constexpr const int cpaNoPreserveTimeDirs = 0x800;
constexpr const int cpaNoResumeSupport    = 0x1000;
constexpr const int cpaNoEncryptNewFiles  = 0x2000;
constexpr const int cpaNoCalculateSize    = 0x4000;
//---------------------------------------------------------------------------
struct TUsableCopyParamAttrs
{
  int General{0};
  int Upload{0};
  int Download{0};
};
//---------------------------------------------------------------------------
NB_DEFINE_CLASS_ID(TCopyParamType);
class NB_CORE_EXPORT TCopyParamType : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCopyParamType); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCopyParamType) || TObject::is(Kind); }
private:
  TFileMasks FAsciiFileMask;
  TFileNameCase FFileNameCase{};
  bool FPreserveReadOnly{false};
  bool FPreserveTime{false};
  bool FPreserveTimeDirs{false};
  TRights FRights{};
  TTransferMode FTransferMode{};
  bool FAddXToDirectories{false};
  bool FPreserveRights{false};
  bool FIgnorePermErrors{false};
  TResumeSupport FResumeSupport;
  int64_t FResumeThreshold{0};
  __removed UnicodeString GetLogStr() const;
  wchar_t FInvalidCharsReplacement{};
  UnicodeString FLocalInvalidChars;
  UnicodeString FTokenizibleChars;
  bool FCalculateSize{false};
  UnicodeString FFileMask;
  TFileMasks FIncludeFileMask;
  std::unique_ptr<TStringList> FTransferSkipList;
  UnicodeString FTransferResumeFile;
  bool FClearArchive{false};
  bool FRemoveCtrlZ{false};
  bool FRemoveBOM{false};
  uintptr_t FCPSLimit{0};
  bool FNewerOnly{false};
  bool FEncryptNewFiles{false};
  bool FExcludeHiddenFiles{false};
  bool FExcludeEmptyDirectories{false};
  int64_t FSize{0};
  static const wchar_t TokenPrefix = L'%';
  static const wchar_t NoReplacement = wchar_t(0);
  static const wchar_t TokenReplacement = wchar_t(1);

public:
  void SetLocalInvalidChars(const UnicodeString Value);
  bool GetReplaceInvalidChars() const;
  void SetReplaceInvalidChars(bool Value);
  UnicodeString RestoreChars(const UnicodeString AFileName) const;
  void DoGetInfoStr(UnicodeString Separator, intptr_t AAttrs,
    UnicodeString & Result, bool & SomeAttrIncluded, const UnicodeString ALink, UnicodeString & ScriptArgs
    /*TAssemblyLanguage Language, UnicodeString & AssemblyCode*/) const;
  TStrings *GetTransferSkipList() const;
  void SetTransferSkipList(TStrings *Value);

public:
  explicit TCopyParamType(TObjectClassId Kind = OBJECT_CLASS_TCopyParamType);
  TCopyParamType(const TCopyParamType &Source);
  virtual ~TCopyParamType();
  TCopyParamType &operator=(const TCopyParamType &rhs);
  virtual void Assign(const TCopyParamType *Source);
  virtual void Default();
  UnicodeString ChangeFileName(const UnicodeString AFileName,
    TOperationSide Side, bool FirstLevel) const;
  DWORD LocalFileAttrs(const TRights &Rights) const;
  TRights RemoteFileRights(uintptr_t Attrs) const;
  bool UseAsciiTransfer(const UnicodeString AFileName, TOperationSide Side,
    const TFileMasks::TParams &Params) const;
  bool AllowResume(int64_t Size) const;
  bool ResumeTransfer(const UnicodeString AFileName) const;
  UnicodeString ValidLocalFileName(const UnicodeString AFileName) const;
  UnicodeString ValidLocalPath(const UnicodeString APath) const;
  bool AllowAnyTransfer() const;
  bool AllowTransfer(const UnicodeString AFileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params, bool Hidden) const;
  bool SkipTransfer(const UnicodeString AFileName, bool Directory) const;

  virtual void Load(THierarchicalStorage *Storage);
  virtual void Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults = nullptr) const;
  UnicodeString GetInfoStr(const UnicodeString Separator, intptr_t Attrs) const;
  bool AnyUsableCopyParam(intptr_t Attrs) const;
  UnicodeString GenerateTransferCommandArgs(int Attrs, const UnicodeString & Link) const;
  UnicodeString GenerateAssemblyCode(/*TAssemblyLanguage Language, */int Attrs) const;

  bool operator==(const TCopyParamType &rhp) const;

  __property TFileMasks AsciiFileMask = { read = FAsciiFileMask, write = FAsciiFileMask };
  __property TFileNameCase FileNameCase = { read = FFileNameCase, write = FFileNameCase };
  __property bool PreserveReadOnly = { read = FPreserveReadOnly, write = FPreserveReadOnly };
  __property bool PreserveTime = { read = FPreserveTime, write = FPreserveTime };
  RWProperty<bool> PreserveTime{nb::bind(&TCopyParamType::GetPreserveTime, this), nb::bind(&TCopyParamType::SetPreserveTime, this)};
  __property bool PreserveTimeDirs = { read = FPreserveTimeDirs, write = FPreserveTimeDirs };
  __property TRights Rights = { read = FRights, write = FRights };
  __property TTransferMode TransferMode = { read = FTransferMode, write = FTransferMode };
  __property UnicodeString LogStr  = { read = GetLogStr };
  ROProperty<UnicodeString> LogStr{nb::bind(&TCopyParamType::GetLogStr, this)};
  __property bool AddXToDirectories  = { read = FAddXToDirectories, write = FAddXToDirectories };
  __property bool PreserveRights = { read = FPreserveRights, write = FPreserveRights };
  __property bool IgnorePermErrors = { read = FIgnorePermErrors, write = FIgnorePermErrors };
  __property TResumeSupport ResumeSupport = { read = FResumeSupport, write = FResumeSupport };
  __property int64_t ResumeThreshold = { read = FResumeThreshold, write = FResumeThreshold };
  __property wchar_t InvalidCharsReplacement = { read = FInvalidCharsReplacement, write = FInvalidCharsReplacement };
  __property bool ReplaceInvalidChars = { read = GetReplaceInvalidChars, write = SetReplaceInvalidChars };
  __property UnicodeString LocalInvalidChars = { read = FLocalInvalidChars, write = SetLocalInvalidChars };
  RWProperty<UnicodeString> LocalInvalidChars{nb::bind(&TCopyParamType::GetLocalInvalidChars, this), nb::bind(&TCopyParamType::SetLocalInvalidChars, this)};
  __property bool CalculateSize = { read = FCalculateSize, write = FCalculateSize };
  bool& CalculateSize{FCalculateSize};
  __property UnicodeString FileMask = { read = FFileMask, write = FFileMask };
  UnicodeString& FileMask{FFileMask};
  __property TFileMasks IncludeFileMask = { read = FIncludeFileMask, write = FIncludeFileMask };
  TFileMasks& IncludeFileMask{FIncludeFileMask};
  __property TStrings *TransferSkipList = { read = GetTransferSkipList, write = SetTransferSkipList };
  RWProperty<TStrings *>TransferSkipList{nb::bind(&TCopyParamType::GetTransferSkipList, this), nb::bind(&TCopyParamType::SetTransferSkipList, this)};
  __property UnicodeString TransferResumeFile = { read = FTransferResumeFile, write = FTransferResumeFile };
  UnicodeString& TransferResumeFile{FTransferResumeFile};
  __property bool ClearArchive = { read = FClearArchive, write = FClearArchive };
  bool& ClearArchive{FClearArchive};
  __property bool RemoveCtrlZ = { read = FRemoveCtrlZ, write = FRemoveCtrlZ };
  bool& RemoveCtrlZ{FRemoveCtrlZ};
  __property bool RemoveBOM = { read = FRemoveBOM, write = FRemoveBOM };
  bool& RemoveBOM{FRemoveBOM};
  __property uintptr_t CPSLimit = { read = FCPSLimit, write = FCPSLimit };
  uintptr_t& CPSLimit{FCPSLimit};
  __property bool NewerOnly = { read = FNewerOnly, write = FNewerOnly };
  bool& NewerOnly{FNewerOnly};
  __property bool EncryptNewFiles = { read = FEncryptNewFiles, write = FEncryptNewFiles };
  bool& EncryptNewFiles{FEncryptNewFiles};
  __property bool ExcludeHiddenFiles = { read = FExcludeHiddenFiles, write = FExcludeHiddenFiles };
  bool& ExcludeHiddenFiles{FExcludeHiddenFiles};
  __property bool ExcludeEmptyDirectories = { read = FExcludeEmptyDirectories, write = FExcludeEmptyDirectories };
  bool& ExcludeEmptyDirectories{FExcludeEmptyDirectories};
  __property __int64 Size = { read = FSize, write = FSize };
  int64_t& Size{FSize};

  const TFileMasks &GetAsciiFileMask() const { return FAsciiFileMask; }
  TFileMasks &GetAsciiFileMask() { return FAsciiFileMask; }
  void SetAsciiFileMask(const TFileMasks &Value) { FAsciiFileMask = Value; }
  const TFileNameCase &GetFileNameCase() const { return FFileNameCase; }
  void SetFileNameCase(TFileNameCase Value) { FFileNameCase = Value; }
  bool GetPreserveReadOnly() const { return FPreserveReadOnly; }
  void SetPreserveReadOnly(bool Value) { FPreserveReadOnly = Value; }
  bool GetPreserveTime() const { return FPreserveTime; }
  void SetPreserveTime(bool Value) { FPreserveTime = Value; }
  bool GetPreserveTimeDirs() const { return FPreserveTimeDirs; }
  void SetPreserveTimeDirs(bool Value) { FPreserveTimeDirs = Value; }
  const TRights &GetRights() const { return FRights; }
  TRights &GetRights() { return FRights; }
  void SetRights(const TRights &Value) { FRights.Assign(&Value); }
  TTransferMode GetTransferMode() const { return FTransferMode; }
  void SetTransferMode(TTransferMode Value) { FTransferMode = Value; }
  UnicodeString GetLogStr() const;
  bool GetAddXToDirectories() const { return FAddXToDirectories; }
  void SetAddXToDirectories(bool Value) { FAddXToDirectories = Value; }
  bool GetPreserveRights() const { return FPreserveRights; }
  void SetPreserveRights(bool Value) { FPreserveRights = Value; }
  bool GetIgnorePermErrors() const { return FIgnorePermErrors; }
  void SetIgnorePermErrors(bool Value) { FIgnorePermErrors = Value; }
  TResumeSupport GetResumeSupport() const { return FResumeSupport; }
  void SetResumeSupport(TResumeSupport Value) { FResumeSupport = Value; }
  int64_t GetResumeThreshold() const { return FResumeThreshold; }
  void SetResumeThreshold(int64_t Value) { FResumeThreshold = Value; }
  wchar_t GetInvalidCharsReplacement() const { return FInvalidCharsReplacement; }
  void SetInvalidCharsReplacement(wchar_t Value) { FInvalidCharsReplacement = Value; }
  UnicodeString GetLocalInvalidChars() const { return FLocalInvalidChars; }
  bool GetCalculateSize() const { return FCalculateSize; }
  void SetCalculateSize(bool Value) { FCalculateSize = Value; }
  UnicodeString GetFileMask() const { return FFileMask; }
  void SetFileMask(const UnicodeString Value) { FFileMask = Value; }
  const TFileMasks &GetIncludeFileMask() const { return FIncludeFileMask; }
  TFileMasks &GetIncludeFileMask() { return FIncludeFileMask; }
  void SetIncludeFileMask(const TFileMasks &Value) { FIncludeFileMask = Value; }
  bool GetClearArchive() const { return FClearArchive; }
  void SetClearArchive(bool Value) { FClearArchive = Value; }
  UnicodeString GetTransferResumeFile() const { return FTransferResumeFile; }
  void SetTransferResumeFile(UnicodeString Value) { FTransferResumeFile = Value; }
  bool GetRemoveCtrlZ() const { return FRemoveCtrlZ; }
  void SetRemoveCtrlZ(bool Value) { FRemoveCtrlZ = Value; }
  bool GetRemoveBOM() const { return FRemoveBOM; }
  void SetRemoveBOM(bool Value) { FRemoveBOM = Value; }
  uintptr_t GetCPSLimit() const { return FCPSLimit; }
  void SetCPSLimit(uintptr_t Value) { FCPSLimit = Value; }
  bool GetNewerOnly() const { return FNewerOnly; }
  void SetNewerOnly(bool Value) { FNewerOnly = Value; }

};
//---------------------------------------------------------------------------
NB_CORE_EXPORT uintptr_t GetSpeedLimit(const UnicodeString Text);
NB_CORE_EXPORT UnicodeString SetSpeedLimit(uintptr_t Limit);
NB_CORE_EXPORT void CopySpeedLimits(TStrings *Source, TStrings *Dest);
NB_CORE_EXPORT TOperationSide ReverseOperationSide(TOperationSide Side);
//---------------------------------------------------------------------------
