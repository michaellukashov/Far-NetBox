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
const int cpaExcludeMaskOnly = 0x01;
const int cpaNoTransferMode =  0x02;
const int cpaNoExcludeMask =   0x04;
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
class TCopyParamType {
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
  wstring GetLogStr() const;
  char FInvalidCharsReplacement;
  wstring FLocalInvalidChars;
  wstring FTokenizibleChars;
  bool FCalculateSize;
  wstring FFileMask;
  TFileMasks FExcludeFileMask;
  bool FNegativeExclude;
  bool FClearArchive;
  unsigned long FCPSLimit;
  static const char TokenPrefix = '%';
  static const char NoReplacement = char(false);
  static const char TokenReplacement = char(true);

  static wstring Untokenize(wstring FileName);
  void SetLocalInvalidChars(wstring value);
  bool GetReplaceInvalidChars() const;
  void SetReplaceInvalidChars(bool value);
  char * ReplaceChar(wstring & FileName, char * InvalidChar) const;
  wstring RestoreChars(wstring FileName) const;

public:
  TCopyParamType();
  TCopyParamType(const TCopyParamType & Source);
  virtual ~TCopyParamType();
  TCopyParamType & operator =(const TCopyParamType & rhp);
  virtual void Assign(const TCopyParamType * Source);
  virtual void Default();
  wstring ChangeFileName(wstring FileName,
    TOperationSide Side, bool FirstLevel) const;
  int LocalFileAttrs(const TRights & Rights) const;
  TRights RemoteFileRights(int Attrs) const;
  bool UseAsciiTransfer(wstring FileName, TOperationSide Side,
    const TFileMasks::TParams & Params) const;
  bool AllowResume(__int64 Size) const;
  wstring ValidLocalFileName(wstring FileName) const;
  wstring ValidLocalPath(wstring Path) const;
  bool AllowAnyTransfer() const;
  bool AllowTransfer(wstring FileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params) const;

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  wstring GetInfoStr(wstring Separator, int Attrs) const;

  bool operator==(const TCopyParamType & rhp) const;

  __property TFileMasks AsciiFileMask = { read = FAsciiFileMask, write = FAsciiFileMask };
  __property TFileNameCase FileNameCase = { read = FFileNameCase, write = FFileNameCase };
  __property bool PreserveReadOnly = { read = FPreserveReadOnly, write = FPreserveReadOnly };
  __property bool PreserveTime = { read = FPreserveTime, write = FPreserveTime };
  __property TRights Rights = { read = FRights, write = FRights };
  __property TTransferMode TransferMode = { read = FTransferMode, write = FTransferMode };
  __property wstring LogStr  = { read=GetLogStr };
  __property bool AddXToDirectories  = { read=FAddXToDirectories, write=FAddXToDirectories };
  __property bool PreserveRights = { read = FPreserveRights, write = FPreserveRights };
  __property bool IgnorePermErrors = { read = FIgnorePermErrors, write = FIgnorePermErrors };
  __property TResumeSupport ResumeSupport = { read = FResumeSupport, write = FResumeSupport };
  __property __int64 ResumeThreshold = { read = FResumeThreshold, write = FResumeThreshold };
  __property char InvalidCharsReplacement = { read = FInvalidCharsReplacement, write = FInvalidCharsReplacement };
  __property bool ReplaceInvalidChars = { read = GetReplaceInvalidChars, write = SetReplaceInvalidChars };
  __property wstring LocalInvalidChars = { read = FLocalInvalidChars, write = SetLocalInvalidChars };
  __property bool CalculateSize = { read = FCalculateSize, write = FCalculateSize };
  __property wstring FileMask = { read = FFileMask, write = FFileMask };
  __property TFileMasks ExcludeFileMask = { read = FExcludeFileMask, write = FExcludeFileMask };
  __property bool NegativeExclude = { read = FNegativeExclude, write = FNegativeExclude };
  __property bool ClearArchive = { read = FClearArchive, write = FClearArchive };
  __property unsigned long CPSLimit = { read = FCPSLimit, write = FCPSLimit };
};
//---------------------------------------------------------------------------
unsigned long GetSpeedLimit(const wstring & Text);
wstring SetSpeedLimit(unsigned long Limit);
//---------------------------------------------------------------------------
#endif
