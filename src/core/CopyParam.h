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
    char FInvalidCharsReplacement;
    std::wstring FLocalInvalidChars;
    std::wstring FTokenizibleChars;
    bool FCalculateSize;
    std::wstring FFileMask;
    TFileMasks FExcludeFileMask;
    bool FNegativeExclude;
    bool FClearArchive;
    size_t FCPSLimit;

    static std::wstring Untokenize(const std::wstring FileName);
    wchar_t *ReplaceChar(std::wstring &FileName, wchar_t *InvalidChar) const;
    std::wstring RestoreChars(const std::wstring FileName) const;

public:
    static const wchar_t TokenPrefix = L'%';
    static const wchar_t NoReplacement = static_cast<wchar_t>(false);
    static const wchar_t TokenReplacement = static_cast<wchar_t>(true);

    TCopyParamType();
    TCopyParamType(const TCopyParamType &Source);
    virtual ~TCopyParamType();
    TCopyParamType & __fastcall operator =(const TCopyParamType &rhp);
    virtual void __fastcall Assign(const TCopyParamType *Source);
    virtual void __fastcall Default();
    std::wstring ChangeFileName(const std::wstring FileName,
                                TOperationSide Side, bool FirstLevel) const;
    int LocalFileAttrs(const TRights &Rights) const;
    TRights RemoteFileRights(int Attrs) const;
    bool UseAsciiTransfer(const std::wstring FileName, TOperationSide Side,
                          const TFileMasks::TParams &Params) const;
    bool AllowResume(__int64 Size) const;
    std::wstring ValidLocalFileName(const std::wstring FileName) const;
    std::wstring ValidLocalPath(const std::wstring Path) const;
    bool AllowAnyTransfer() const;
    bool AllowTransfer(const std::wstring FileName, TOperationSide Side,
                       bool Directory, const TFileMasks::TParams &Params) const;

    void Load(THierarchicalStorage *Storage);
    void Save(THierarchicalStorage *Storage) const;
    std::wstring GetInfoStr(const std::wstring Separator, int Attrs) const;

    bool operator==(const TCopyParamType &rhp) const;

    TFileMasks GetAsciiFileMask() const { return FAsciiFileMask; }
    void SetAsciiFileMask(TFileMasks value) { FAsciiFileMask = value; }
    TFileNameCase GetFileNameCase() const { return FFileNameCase; }
    void SetFileNameCase(TFileNameCase value) { FFileNameCase = value; }
    bool GetPreserveReadOnly() const { return FPreserveReadOnly; }
    void SetPreserveReadOnly(bool value) { FPreserveReadOnly = value; }
    bool GetPreserveTime() const { return FPreserveTime; }
    void SetPreserveTime(bool value) { FPreserveTime = value; }
    const TRights &GetRights() const { return FRights; }
    void SetRights(const TRights &value) { FRights.Assign(&value); }
    TTransferMode GetTransferMode() const { return FTransferMode; }
    void SetTransferMode(TTransferMode value) { FTransferMode = value; }
    std::wstring GetLogStr() const;
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
    bool GetReplaceInvalidChars() const;
    void SetReplaceInvalidChars(bool value);
    std::wstring GetLocalInvalidChars() const { return FLocalInvalidChars; }
    void SetLocalInvalidChars(const std::wstring value);
    bool GetCalculateSize() const { return FCalculateSize; }
    void SetCalculateSize(bool value) { FCalculateSize = value; }
    std::wstring GetFileMask() const { return FFileMask; }
    void SetFileMask(const std::wstring value) { FFileMask = value; }
    TFileMasks GetExcludeFileMask() const { return FExcludeFileMask; }
    void SetExcludeFileMask(TFileMasks value) { FExcludeFileMask = value; }
    bool GetNegativeExclude() const { return FNegativeExclude; }
    void SetNegativeExclude(bool value) { FNegativeExclude = value; }
    bool GetClearArchive() const { return FClearArchive; }
    void SetClearArchive(bool value) { FClearArchive = value; }
    size_t GetCPSLimit() const { return FCPSLimit; }
    void SetCPSLimit(size_t value) { FCPSLimit = value; }
};
//---------------------------------------------------------------------------
unsigned long GetSpeedLimit(const std::wstring Text);
std::wstring SetSpeedLimit(size_t Limit);
//---------------------------------------------------------------------------
#endif
