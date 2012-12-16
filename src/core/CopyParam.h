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
  unsigned long FCPSLimit;
public:
  static const wchar_t TokenPrefix = L'%';
  static const wchar_t NoReplacement = wchar_t(false);
  static const wchar_t TokenReplacement = wchar_t(true);

public:
  void __fastcall SetLocalInvalidChars(const UnicodeString & Value);
  bool __fastcall GetReplaceInvalidChars() const;
  void __fastcall SetReplaceInvalidChars(bool value);
  UnicodeString __fastcall RestoreChars(const UnicodeString & FileName) const;

public:
  /* __fastcall */ TCopyParamType();
  /* __fastcall */ TCopyParamType(const TCopyParamType & Source);
  virtual/* __fastcall */  ~TCopyParamType();
  TCopyParamType & __fastcall operator =(const TCopyParamType & rhp);
  virtual void __fastcall Assign(const TCopyParamType * Source);
  virtual void __fastcall Default();
  UnicodeString __fastcall ChangeFileName(const UnicodeString & FileName,
    TOperationSide Side, bool FirstLevel) const;
  int __fastcall LocalFileAttrs(const TRights & Rights) const;
  TRights __fastcall RemoteFileRights(int Attrs) const;
  bool __fastcall UseAsciiTransfer(const UnicodeString & FileName, TOperationSide Side,
    const TFileMasks::TParams & Params) const;
  bool __fastcall AllowResume(__int64 Size) const;
  UnicodeString __fastcall ValidLocalFileName(const UnicodeString & FileName) const;
  UnicodeString __fastcall ValidLocalPath(const UnicodeString & Path) const;
  bool __fastcall AllowAnyTransfer() const;
  bool __fastcall AllowTransfer(const UnicodeString & FileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params) const;

  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage) const;
  UnicodeString __fastcall GetInfoStr(const UnicodeString & Separator, int Attrs) const;

  bool __fastcall operator==(const TCopyParamType & rhp) const;

  const TFileMasks & __fastcall GetAsciiFileMask() const;
  TFileMasks & __fastcall GetAsciiFileMask();
  void __fastcall SetAsciiFileMask(TFileMasks value);
  const TFileNameCase & __fastcall GetFileNameCase() const;
  void __fastcall SetFileNameCase(TFileNameCase value) { FFileNameCase = value; }
  bool __fastcall GetPreserveReadOnly() const { return FPreserveReadOnly; }
  void __fastcall SetPreserveReadOnly(bool value) { FPreserveReadOnly = value; }
  bool __fastcall GetPreserveTime() const { return FPreserveTime; }
  void __fastcall SetPreserveTime(bool value) { FPreserveTime = value; }
  const TRights & __fastcall GetRights() const { return FRights; }
  TRights & __fastcall GetRights() { return FRights; }
  void __fastcall SetRights(const TRights & value) { FRights.Assign(&value); }
  TTransferMode __fastcall GetTransferMode() const { return FTransferMode; }
  void __fastcall SetTransferMode(TTransferMode value) { FTransferMode = value; }
  UnicodeString __fastcall GetLogStr() const;
  bool __fastcall GetAddXToDirectories() const { return FAddXToDirectories; }
  void __fastcall SetAddXToDirectories(bool value) { FAddXToDirectories = value; }
  bool __fastcall GetPreserveRights() const { return FPreserveRights; }
  void __fastcall SetPreserveRights(bool value) { FPreserveRights = value; }
  bool __fastcall GetIgnorePermErrors() const { return FIgnorePermErrors; }
  void __fastcall SetIgnorePermErrors(bool value) { FIgnorePermErrors = value; }
  TResumeSupport __fastcall GetResumeSupport() const { return FResumeSupport; }
  void __fastcall SetResumeSupport(TResumeSupport value) { FResumeSupport = value; }
  __int64 GetResumeThreshold() const { return FResumeThreshold; }
  void __fastcall SetResumeThreshold(__int64 value) { FResumeThreshold = value; }
  wchar_t __fastcall GetInvalidCharsReplacement() const { return FInvalidCharsReplacement; }
  void __fastcall SetInvalidCharsReplacement(wchar_t value) { FInvalidCharsReplacement = value; }
  UnicodeString __fastcall GetLocalInvalidChars() const;
  bool __fastcall GetCalculateSize() const { return FCalculateSize; }
  void __fastcall SetCalculateSize(bool value) { FCalculateSize = value; }
  UnicodeString __fastcall GetFileMask() const;
  void __fastcall SetFileMask(const UnicodeString & Value);
  const TFileMasks & __fastcall GetIncludeFileMask() const;
  TFileMasks & __fastcall GetIncludeFileMask();
  void __fastcall SetIncludeFileMask(TFileMasks value);
  const TFileMasks & __fastcall GetExcludeFileMask() const { return FExcludeFileMask; }
  TFileMasks & __fastcall GetExcludeFileMask() { return FExcludeFileMask; }
  void SetExcludeFileMask(TFileMasks value) { FExcludeFileMask = value; }
  bool GetNegativeExclude() const { return FNegativeExclude; }
  void SetNegativeExclude(bool value) { FNegativeExclude = value; }
  bool __fastcall GetClearArchive() const { return FClearArchive; }
  void __fastcall SetClearArchive(bool value) { FClearArchive = value; }
  unsigned long __fastcall GetCPSLimit() const { return FCPSLimit; }
  void __fastcall SetCPSLimit(unsigned long value) { FCPSLimit = value; }
};
//---------------------------------------------------------------------------
unsigned long __fastcall GetSpeedLimit(const UnicodeString & Text);
UnicodeString __fastcall SetSpeedLimit(unsigned long Limit);
//---------------------------------------------------------------------------
#endif
