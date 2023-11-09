
#pragma once

#include "FileMasks.h"
#include "FileBuffer.h"
#include "RemoteFiles.h"
#include "Exceptions.h"

// When adding new options, mind TCopyParamType::GetLogStr()
enum TOperationSide { osLocal, osRemote, osCurrent, };
enum TFileNameCase { ncNoChange, ncUpperCase, ncLowerCase, ncFirstUpperCase, ncLowerCaseShort, };
// TScript::OptionProc depend on the order
enum TTransferMode { tmBinary, tmAscii, tmAutomatic, };
enum TResumeSupport { rsOn, rsSmart, rsOff, };

class THierarchicalStorage;
constexpr int32_t cpaIncludeMaskOnly = 0x01;
constexpr int32_t cpaNoTransferMode  = 0x02;
constexpr int32_t cpaNoIncludeMask   = 0x04;
constexpr int32_t cpaNoClearArchive  = 0x08;
constexpr int32_t cpaNoPreserveTime  = 0x10;
constexpr int32_t cpaNoRights        = 0x20;
constexpr int32_t cpaNoPreserveReadOnly = 0x40;
constexpr int32_t cpaNoIgnorePermErrors = 0x80;
constexpr int32_t cpaNoNewerOnly        = 0x100;
constexpr int32_t cpaNoRemoveCtrlZ      = 0x200;
constexpr int32_t cpaNoRemoveBOM        = 0x400;
constexpr int32_t cpaNoPreserveTimeDirs = 0x800;
constexpr int32_t cpaNoResumeSupport    = 0x1000;
constexpr int32_t cpaNoEncryptNewFiles  = 0x2000;
constexpr int32_t cpaNoCalculateSize    = 0x4000;

struct TUsableCopyParamAttrs
{
  int General{0};
  int Upload{0};
  int Download{0};
};

class TTerminal;
class TFTPFileSystem;
class TSFTPFileSystem;

NB_DEFINE_CLASS_ID(TCopyParamType);
class NB_CORE_EXPORT TCopyParamType : public TObject
{
friend class TTerminal;
friend class TFTPFileSystem;
friend class TSFTPFileSystem;
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TCopyParamType); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCopyParamType) || TObject::is(Kind); }
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
  uint32_t FCPSLimit{0};
  bool FNewerOnly{false};
  bool FEncryptNewFiles{false};
  bool FExcludeHiddenFiles{false};
  bool FExcludeEmptyDirectories{false};
  int64_t FSize{0};
  int64_t FPartOffset{0};
  int64_t FPartSize{0};
  TOnceDoneOperation FOnceDoneOperation;
  TTransferOutEvent FOnTransferOut;
  TTransferInEvent FOnTransferIn;

public:
  void SetLocalInvalidChars(const UnicodeString & Value);
  bool GetReplaceInvalidChars() const;
  void SetReplaceInvalidChars(bool Value);
  UnicodeString RestoreChars(const UnicodeString AFileName) const;
  void DoGetInfoStr(const UnicodeString & Separator, int32_t Options,
    UnicodeString & Result, bool & SomeAttrIncluded, const UnicodeString & ALink, UnicodeString & ScriptArgs
    /*TAssemblyLanguage Language, UnicodeString & AssemblyCode*/) const;
  const TStrings * GetTransferSkipList() const;
  void SetTransferSkipList(const TStrings *Value);

public:
  explicit TCopyParamType(TObjectClassId Kind = OBJECT_CLASS_TCopyParamType) noexcept;
  TCopyParamType(const TCopyParamType &Source) noexcept;
  virtual ~TCopyParamType() noexcept;
  TCopyParamType &operator=(const TCopyParamType &rhs);
  virtual void Assign(const TCopyParamType *Source);
  virtual void Default();
  UnicodeString ChangeFileName(const UnicodeString AFileName,
    TOperationSide Side, bool FirstLevel) const;
  DWORD LocalFileAttrs(const TRights &Rights) const;
  TRights RemoteFileRights(uint32_t Attrs) const;
  bool UseAsciiTransfer(const UnicodeString AFileName, TOperationSide Side,
    const TFileMasks::TParams &Params) const;
  bool AllowResume(int64_t Size, const UnicodeString & FileName) const;
  bool ResumeTransfer(const UnicodeString AFileName) const;
  UnicodeString ValidLocalFileName(const UnicodeString & AFileName) const;
  UnicodeString ValidLocalPath(const UnicodeString & APath) const;
  bool AllowAnyTransfer() const;
  bool AllowTransfer(const UnicodeString & AFileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params, bool Hidden) const;
  bool SkipTransfer(const UnicodeString & AFileName, bool Directory) const;

  virtual void Load(THierarchicalStorage *Storage);
  virtual void Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults = nullptr) const;
  UnicodeString GetInfoStr(const UnicodeString & Separator, int32_t Attrs) const;
  bool AnyUsableCopyParam(int32_t Attrs) const;
  UnicodeString GenerateTransferCommandArgs(int Attrs, const UnicodeString & Link) const;
  UnicodeString GenerateAssemblyCode(/*TAssemblyLanguage Language, */int Attrs) const;

  bool operator==(const TCopyParamType &rhp) const;

  __property TFileMasks AsciiFileMask = { read = FAsciiFileMask, write = FAsciiFileMask };
  TFileMasks& AsciiFileMask{FAsciiFileMask};
  __property TFileNameCase FileNameCase = { read = FFileNameCase, write = FFileNameCase };
  TFileNameCase& FileNameCase{FFileNameCase};
  __property bool PreserveReadOnly = { read = FPreserveReadOnly, write = FPreserveReadOnly };
  bool& PreserveReadOnly{FPreserveReadOnly};
  __property bool PreserveTime = { read = FPreserveTime, write = FPreserveTime };
  RWProperty3<bool> PreserveTime{nb::bind(&TCopyParamType::GetPreserveTime, this), nb::bind(&TCopyParamType::SetPreserveTime, this)};
  __property bool PreserveTimeDirs = { read = FPreserveTimeDirs, write = FPreserveTimeDirs };
  bool& PreserveTimeDirs{FPreserveTimeDirs};
  __property TRights Rights = { read = FRights, write = FRights };
  TRights& Rights{FRights};
  __property TTransferMode TransferMode = { read = FTransferMode, write = FTransferMode };
  TTransferMode& TransferMode{FTransferMode};
  __property UnicodeString LogStr  = { read=GetLogStr };
  ROProperty<UnicodeString> LogStr{nb::bind(&TCopyParamType::GetLogStr, this)};
  __property bool AddXToDirectories  = { read=FAddXToDirectories, write=FAddXToDirectories };
  bool& AddXToDirectories{FAddXToDirectories};
  __property bool PreserveRights = { read = FPreserveRights, write = FPreserveRights };
  bool& PreserveRights{FPreserveRights};
  __property bool IgnorePermErrors = { read = FIgnorePermErrors, write = FIgnorePermErrors };
  bool& IgnorePermErrors{FIgnorePermErrors};
  __property TResumeSupport ResumeSupport = { read = FResumeSupport, write = FResumeSupport };
  TResumeSupport& ResumeSupport{FResumeSupport};
  __property __int64 ResumeThreshold = { read = FResumeThreshold, write = FResumeThreshold };
  int64_t& ResumeThreshold{FResumeThreshold};
  __property wchar_t InvalidCharsReplacement = { read = FInvalidCharsReplacement, write = FInvalidCharsReplacement };
  wchar_t& InvalidCharsReplacement{FInvalidCharsReplacement};
  __property bool ReplaceInvalidChars = { read = GetReplaceInvalidChars, write = SetReplaceInvalidChars };
  RWProperty3<bool> ReplaceInvalidChars{nb::bind(&TCopyParamType::GetReplaceInvalidChars, this), nb::bind(&TCopyParamType::SetReplaceInvalidChars, this)};
  __property UnicodeString LocalInvalidChars = { read = FLocalInvalidChars, write = SetLocalInvalidChars };
  RWProperty<UnicodeString> LocalInvalidChars{nb::bind(&TCopyParamType::GetLocalInvalidChars, this), nb::bind(&TCopyParamType::SetLocalInvalidChars, this)};
  __property bool CalculateSize = { read = FCalculateSize, write = FCalculateSize };
  bool& CalculateSize{FCalculateSize};
  __property UnicodeString FileMask = { read = FFileMask, write = FFileMask };
  UnicodeString& FileMask{FFileMask};
  __property TFileMasks IncludeFileMask = { read = FIncludeFileMask, write = FIncludeFileMask };
  TFileMasks& IncludeFileMask{FIncludeFileMask};
  __property TStrings * TransferSkipList = { read = GetTransferSkipList, write = SetTransferSkipList };
  RWProperty1<TStrings>TransferSkipList{nb::bind(&TCopyParamType::GetTransferSkipList, this), nb::bind(&TCopyParamType::SetTransferSkipList, this)};
  __property UnicodeString TransferResumeFile = { read = FTransferResumeFile, write = FTransferResumeFile };
  UnicodeString& TransferResumeFile{FTransferResumeFile};
  __property bool ClearArchive = { read = FClearArchive, write = FClearArchive };
  bool& ClearArchive{FClearArchive};
  __property bool RemoveCtrlZ = { read = FRemoveCtrlZ, write = FRemoveCtrlZ };
  bool& RemoveCtrlZ{FRemoveCtrlZ};
  __property bool RemoveBOM = { read = FRemoveBOM, write = FRemoveBOM };
  bool& RemoveBOM{FRemoveBOM};
  __property unsigned long CPSLimit = { read = FCPSLimit, write = FCPSLimit };
  uint32_t& CPSLimit{FCPSLimit};
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
  __property __int64 PartSize = { read = FPartSize, write = FPartSize };
  int64_t& PartSize{FPartSize};
  __property __int64 PartOffset = { read = FPartOffset, write = FPartOffset };
  int64_t& PartOffset{FPartOffset};
  __property TOnceDoneOperation OnceDoneOperation = { read = FOnceDoneOperation, write = FOnceDoneOperation };
  TOnceDoneOperation& OnceDoneOperation{FOnceDoneOperation};
  __property TTransferOutEvent OnTransferOut = { read = FOnTransferOut, write = FOnTransferOut };
  __property TTransferInEvent OnTransferIn = { read = FOnTransferIn, write = FOnTransferIn };

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
  void SetTransferResumeFile(const UnicodeString & Value) { FTransferResumeFile = Value; }
  bool GetRemoveCtrlZ() const { return FRemoveCtrlZ; }
  void SetRemoveCtrlZ(bool Value) { FRemoveCtrlZ = Value; }
  bool GetRemoveBOM() const { return FRemoveBOM; }
  void SetRemoveBOM(bool Value) { FRemoveBOM = Value; }
  uint32_t GetCPSLimit() const { return FCPSLimit; }
  void SetCPSLimit(uint32_t Value) { FCPSLimit = Value; }
  bool GetNewerOnly() const { return FNewerOnly; }
  void SetNewerOnly(bool Value) { FNewerOnly = Value; }

};

NB_CORE_EXPORT uint32_t GetSpeedLimit(const UnicodeString Text);
NB_CORE_EXPORT UnicodeString SetSpeedLimit(uint32_t Limit);
NB_CORE_EXPORT void CopySpeedLimits(TStrings *Source, TStrings *Dest);
NB_CORE_EXPORT TOperationSide ReverseOperationSide(TOperationSide Side);
extern const uint32_t MinSpeed;
extern const uint32_t MaxSpeed;

