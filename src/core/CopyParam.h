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
  // UnicodeString __fastcall GetLogStr() const;
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

  static UnicodeString Untokenize(const UnicodeString FileName);
  wchar_t * ReplaceChar(UnicodeString & FileName, wchar_t * InvalidChar) const;
  // UnicodeString RestoreChars(const UnicodeString FileName) const;
  void __fastcall SetLocalInvalidChars(UnicodeString value);
  bool __fastcall GetReplaceInvalidChars() const;
  void __fastcall SetReplaceInvalidChars(bool value);
  UnicodeString __fastcall RestoreChars(UnicodeString FileName) const;

public:
  // static const wchar_t TokenPrefix = L'%';
  // static const wchar_t NoReplacement = static_cast<wchar_t>(false);
  // static const wchar_t TokenReplacement = static_cast<wchar_t>(true);

  /* __fastcall */ TCopyParamType();
  /* __fastcall */ TCopyParamType(const TCopyParamType & Source);
  virtual/* __fastcall */  ~TCopyParamType();
  TCopyParamType & __fastcall operator =(const TCopyParamType & rhp);
  virtual void __fastcall Assign(const TCopyParamType * Source);
  virtual void __fastcall Default();
  UnicodeString __fastcall ChangeFileName(UnicodeString FileName,
    TOperationSide Side, bool FirstLevel) const;
  int __fastcall LocalFileAttrs(const TRights & Rights) const;
  TRights __fastcall RemoteFileRights(int Attrs) const;
  bool __fastcall UseAsciiTransfer(UnicodeString FileName, TOperationSide Side,
    const TFileMasks::TParams & Params) const;
  bool __fastcall AllowResume(__int64 Size) const;
  UnicodeString __fastcall ValidLocalFileName(UnicodeString FileName) const;
  UnicodeString __fastcall ValidLocalPath(UnicodeString Path) const;
  bool __fastcall AllowAnyTransfer() const;
  bool __fastcall AllowTransfer(UnicodeString FileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params) const;

  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage) const;
  UnicodeString __fastcall GetInfoStr(UnicodeString Separator, int Attrs) const;

  bool __fastcall operator==(const TCopyParamType & rhp) const;

#ifndef _MSC_VER
  __property TFileMasks AsciiFileMask = { read = FAsciiFileMask, write = FAsciiFileMask };
  __property TFileNameCase FileNameCase = { read = FFileNameCase, write = FFileNameCase };
  __property bool PreserveReadOnly = { read = FPreserveReadOnly, write = FPreserveReadOnly };
  __property bool PreserveTime = { read = FPreserveTime, write = FPreserveTime };
  __property TRights Rights = { read = FRights, write = FRights };
  __property TTransferMode TransferMode = { read = FTransferMode, write = FTransferMode };
  __property UnicodeString LogStr  = { read=GetLogStr };
  __property bool AddXToDirectories  = { read=FAddXToDirectories, write=FAddXToDirectories };
  __property bool PreserveRights = { read = FPreserveRights, write = FPreserveRights };
  __property bool IgnorePermErrors = { read = FIgnorePermErrors, write = FIgnorePermErrors };
  __property TResumeSupport ResumeSupport = { read = FResumeSupport, write = FResumeSupport };
  __property __int64 ResumeThreshold = { read = FResumeThreshold, write = FResumeThreshold };
  __property wchar_t InvalidCharsReplacement = { read = FInvalidCharsReplacement, write = FInvalidCharsReplacement };
  __property bool ReplaceInvalidChars = { read = GetReplaceInvalidChars, write = SetReplaceInvalidChars };
  __property UnicodeString LocalInvalidChars = { read = FLocalInvalidChars, write = SetLocalInvalidChars };
  __property bool CalculateSize = { read = FCalculateSize, write = FCalculateSize };
  __property UnicodeString FileMask = { read = FFileMask, write = FFileMask };
  __property TFileMasks IncludeFileMask = { read = FIncludeFileMask, write = FIncludeFileMask };
  __property bool ClearArchive = { read = FClearArchive, write = FClearArchive };
  __property unsigned long CPSLimit = { read = FCPSLimit, write = FCPSLimit };
#else
  TFileMasks GetAsciiFileMask() const { return FAsciiFileMask; }
  void SetAsciiFileMask(TFileMasks value) { FAsciiFileMask = value; }
  TFileNameCase GetFileNameCase() const { return FFileNameCase; }
  void SetFileNameCase(TFileNameCase value) { FFileNameCase = value; }
  bool GetPreserveReadOnly() const { return FPreserveReadOnly; }
  void SetPreserveReadOnly(bool value) { FPreserveReadOnly = value; }
  bool GetPreserveTime() const { return FPreserveTime; }
  void SetPreserveTime(bool value) { FPreserveTime = value; }
  const TRights & GetRights() const { return FRights; }
  void SetRights(const TRights & value) { FRights.Assign(&value); }
  TTransferMode GetTransferMode() const { return FTransferMode; }
  void SetTransferMode(TTransferMode value) { FTransferMode = value; }
  UnicodeString __fastcall GetLogStr() const;
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
  char GetInvalidCharsReplacement() const { return FInvalidCharsReplacement; }
  void SetInvalidCharsReplacement(char value) { FInvalidCharsReplacement = value; }
  // bool GetReplaceInvalidChars() const;
  // void SetReplaceInvalidChars(bool value);
  UnicodeString GetLocalInvalidChars() const { return FLocalInvalidChars; }
  // void SetLocalInvalidChars(const UnicodeString value);
  bool GetCalculateSize() const { return FCalculateSize; }
  void SetCalculateSize(bool value) { FCalculateSize = value; }
  UnicodeString GetFileMask() const { return FFileMask; }
  void SetFileMask(const UnicodeString value) { FFileMask = value; }
  TFileMasks GetIncludeFileMask() const { return FIncludeFileMask; }
  void SetIncludeFileMask(TFileMasks value) { FIncludeFileMask = value; }
  TFileMasks GetExcludeFileMask() const { return FExcludeFileMask; }
  void SetExcludeFileMask(TFileMasks value) { FExcludeFileMask = value; }
  bool GetNegativeExclude() const { return FNegativeExclude; }
  void SetNegativeExclude(bool value) { FNegativeExclude = value; }
  bool GetClearArchive() const { return FClearArchive; }
  void SetClearArchive(bool value) { FClearArchive = value; }
  size_t GetCPSLimit() const { return FCPSLimit; }
  void SetCPSLimit(size_t value) { FCPSLimit = value; }
#endif
};
//---------------------------------------------------------------------------
unsigned long __fastcall GetSpeedLimit(const UnicodeString & Text);
UnicodeString __fastcall SetSpeedLimit(unsigned long Limit);
//---------------------------------------------------------------------------
#endif
