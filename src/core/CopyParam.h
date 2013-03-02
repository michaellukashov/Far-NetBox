//---------------------------------------------------------------------------
#ifndef CopyParamH
#define CopyParamH

#include "FileMasks.h"
#include "RemoteFiles.h"
//---------------------------------------------------------------------------
// When adding new options, mind TCopyParamType::GetLogStr()
enum TOperationSide { osLocal, osRemote, osCurrent };
enum TFileNameCase { ncNoChange, ncUpperCase, ncLowerCase, ncFirstUpperCase, ncLowerCaseShort };
// TScript::OptionProc depend on the order
enum TTransferMode { tmBinary, tmAscii, tmAutomatic };
enum TResumeSupport { rsOn, rsSmart, rsOff };
class THierarchicalStorage;
const int cpaIncludeMaskOnly = 0x01;
const int cpaExcludeMaskOnly = 0x01;
const int cpaNoTransferMode =  0x02;
const int cpaNoExcludeMask =   0x04;
const int cpaNoIncludeMask =   0x04;
const int cpaNoClearArchive =  0x08;
const int cpaNoPreserveTime =  0x10;
const int cpaNoRights =        0x20;
const int cpaNoPreserveReadOnly = 0x40;
const int cpaNoIgnorePermErrors = 0x80;
//---------------------------------------------------------------------------
struct TUsableCopyParamAttrs
{
  int General;
  int Upload;
  int Download;
};
//---------------------------------------------------------------------------
class TCopyParamType
{
private:
  TFileMasks FAsciiFileMask;
  TFileNameCase FFileNameCase;
  bool FPreserveReadOnly;
  bool FPreserveTime;
  TRights FRights;
  TTransferMode FTransferMode;
  bool FAddXToDirectories;
  bool FPreserveRights;
  bool FIgnorePermErrors;
  TResumeSupport FResumeSupport;
  __int64 FResumeThreshold;
  wchar_t FInvalidCharsReplacement;
  UnicodeString FLocalInvalidChars;
  UnicodeString FTokenizibleChars;
  bool FCalculateSize;
  UnicodeString FFileMask;
  TFileMasks FExcludeFileMask;
  bool FNegativeExclude;
  TFileMasks FIncludeFileMask;
  bool FClearArchive;
  uintptr_t FCPSLimit;

public:
  static const wchar_t TokenPrefix = L'%';
  static const wchar_t NoReplacement = wchar_t(0);
  static const wchar_t TokenReplacement = wchar_t(1);

public:
  void SetLocalInvalidChars(const UnicodeString & Value);
  bool GetReplaceInvalidChars() const;
  void SetReplaceInvalidChars(bool value);
  UnicodeString RestoreChars(const UnicodeString & FileName) const;

public:
  TCopyParamType();
  TCopyParamType(const TCopyParamType & Source);
  virtual ~TCopyParamType();
  TCopyParamType & operator =(const TCopyParamType & rhp);
  virtual void Assign(const TCopyParamType * Source);
  virtual void Default();
  UnicodeString ChangeFileName(const UnicodeString & FileName,
    TOperationSide Side, bool FirstLevel) const;
  int LocalFileAttrs(const TRights & Rights) const;
  TRights RemoteFileRights(intptr_t Attrs) const;
  bool UseAsciiTransfer(const UnicodeString & FileName, TOperationSide Side,
    const TFileMasks::TParams & Params) const;
  bool AllowResume(__int64 Size) const;
  UnicodeString ValidLocalFileName(const UnicodeString & FileName) const;
  UnicodeString ValidLocalPath(const UnicodeString & Path) const;
  bool AllowAnyTransfer() const;
  bool AllowTransfer(const UnicodeString & FileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params) const;

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString GetInfoStr(const UnicodeString & Separator, int Attrs) const;

  bool operator==(const TCopyParamType & rhp) const;

  const TFileMasks & GetAsciiFileMask() const;
  TFileMasks & GetAsciiFileMask();
  void SetAsciiFileMask(TFileMasks value);
  const TFileNameCase & GetFileNameCase() const;
  void SetFileNameCase(TFileNameCase value) { FFileNameCase = value; }
  bool GetPreserveReadOnly() const { return FPreserveReadOnly; }
  void SetPreserveReadOnly(bool value) { FPreserveReadOnly = value; }
  bool GetPreserveTime() const { return FPreserveTime; }
  void SetPreserveTime(bool value) { FPreserveTime = value; }
  const TRights & GetRights() const { return FRights; }
  TRights & GetRights() { return FRights; }
  void SetRights(const TRights & value) { FRights.Assign(&value); }
  TTransferMode GetTransferMode() const { return FTransferMode; }
  void SetTransferMode(TTransferMode value) { FTransferMode = value; }
  UnicodeString GetLogStr() const;
  bool GetAddXToDirectories() const { return FAddXToDirectories; }
  void SetAddXToDirectories(bool value) { FAddXToDirectories = value; }
  bool GetPreserveRights() const { return FPreserveRights; }
  void SetPreserveRights(bool value) { FPreserveRights = value; }
  bool GetIgnorePermErrors() const { return FIgnorePermErrors; }
  void SetIgnorePermErrors(bool value) { FIgnorePermErrors = value; }
  TResumeSupport GetResumeSupport() const { return FResumeSupport; }
  void SetResumeSupport(TResumeSupport value) { FResumeSupport = value; }
  __int64 GetResumeThreshold() const { return FResumeThreshold; }
  void SetResumeThreshold(__int64 value) { FResumeThreshold = value; }
  wchar_t GetInvalidCharsReplacement() const { return FInvalidCharsReplacement; }
  void SetInvalidCharsReplacement(wchar_t value) { FInvalidCharsReplacement = value; }
  UnicodeString GetLocalInvalidChars() const;
  bool GetCalculateSize() const { return FCalculateSize; }
  void SetCalculateSize(bool Value) { FCalculateSize = Value; }
  UnicodeString GetFileMask() const;
  void SetFileMask(const UnicodeString & Value);
  const TFileMasks & GetIncludeFileMask() const;
  TFileMasks & GetIncludeFileMask();
  void SetIncludeFileMask(TFileMasks Value);
  const TFileMasks & GetExcludeFileMask() const { return FExcludeFileMask; }
  TFileMasks & GetExcludeFileMask() { return FExcludeFileMask; }
  void SetExcludeFileMask(TFileMasks Value) { FExcludeFileMask = Value; }
  bool GetNegativeExclude() const { return FNegativeExclude; }
  void SetNegativeExclude(bool Value) { FNegativeExclude = Value; }
  bool GetClearArchive() const { return FClearArchive; }
  void SetClearArchive(bool Value) { FClearArchive = Value; }
  uintptr_t GetCPSLimit() const { return FCPSLimit; }
  void SetCPSLimit(uintptr_t Value) { FCPSLimit = Value; }
};
//---------------------------------------------------------------------------
uintptr_t  GetSpeedLimit(const UnicodeString & Text);
UnicodeString SetSpeedLimit(uintptr_t Limit);
//---------------------------------------------------------------------------
#endif
