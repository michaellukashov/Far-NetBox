//---------------------------------------------------------------------------
#ifndef FileMasksH
#define FileMasksH
//---------------------------------------------------------------------------
#include <vector>
#ifndef  _MSC_VER
#include <Masks.hpp>
#endif
#include "Classes.h"
#include "Common.h"
//---------------------------------------------------------------------------
class EFileMasksException : public std::exception
{
public:
    explicit EFileMasksException(const std::wstring Message, size_t ErrorStart, size_t ErrorLen);
    size_t ErrorStart;
    size_t ErrorLen;
};
//---------------------------------------------------------------------------
extern const wchar_t IncludeExcludeFileMasksDelimiter;
#define MASK_INDEX(DIRECTORY, INCLUDE) ((DIRECTORY ? 2 : 0) + (INCLUDE ? 0 : 1))
//---------------------------------------------------------------------------
namespace Masks
{

class TMask
{
public:
    TMask(const std::wstring Mask) :
        FMask(Mask)
    {
    }
    bool GetMatches(const std::wstring Str);
private:
    std::wstring FMask;
};

} // namespace Masks

//---------------------------------------------------------------------------
class TFileMasks
{
public:
    struct TParams
    {
        TParams();
        // TParams(const TParams &rhs)
        // {
            // Size = rhs.Size;
            // Modification = rhs.Modification;
        // }
        __int64 Size;
        nb::TDateTime Modification;

        std::wstring ToString() const;
    };

    static bool __fastcall IsMask(const std::wstring Mask);
    static std::wstring __fastcall NormalizeMask(const std::wstring Mask, const std::wstring AnyMask = L"");
    static std::wstring __fastcall ComposeMaskStr(
    nb::TStrings * IncludeFileMasksStr, nb::TStrings * ExcludeFileMasksStr,
    nb::TStrings * IncludeDirectoryMasksStr, nb::TStrings * ExcludeDirectoryMasksStr);

    TFileMasks();
    TFileMasks(int ForceDirectoryMasks);
    TFileMasks(const TFileMasks &Source);
    explicit TFileMasks(const std::wstring AMasks);
    virtual ~TFileMasks();
    TFileMasks & __fastcall operator =(const TFileMasks &rhm);
    TFileMasks & __fastcall operator =(const std::wstring rhs);
    bool __fastcall operator ==(const TFileMasks &rhm) const;
    bool __fastcall operator ==(const std::wstring rhs) const;

    void __fastcall SetMask(const std::wstring Mask);

    bool __fastcall Matches(const std::wstring FileName, bool Directory = false,
                 const std::wstring Path = L"", const TParams *Params = NULL) const;
    bool __fastcall Matches(const std::wstring FileName, bool Local, bool Directory,
                 const TParams *Params = NULL) const;

    bool GetIsValid(size_t &Start, size_t &Length) const;

    std::wstring __fastcall GetMasks() const { return FStr; }
    void __fastcall SetMasks(const std::wstring value);

  // __property TStrings * IncludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, true) };
  nb::TStrings * __fastcall GetIncludeFileMasksStr() { return GetMasksStr(MASK_INDEX(false, true)); };
  // __property TStrings * ExcludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, false) };
  nb::TStrings * __fastcall GetExcludeFileMasksStr() { return GetMasksStr(MASK_INDEX(false, false)); };
  // __property TStrings * IncludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, true) };
  nb::TStrings * __fastcall GetIncludeDirectoryMasksStr() { return GetMasksStr(MASK_INDEX(true, true)); };
  // __property TStrings * ExcludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, false) };
  nb::TStrings * __fastcall GetExcludeDirectoryMasksStr() { return GetMasksStr(MASK_INDEX(true, false)); };

private:
  int FForceDirectoryMasks;
    std::wstring FStr;

    struct TMaskMask
    {
        enum { Any, NoExt, Regular } Kind;
        Masks::TMask *Mask;
    };

    struct TMask
    {
        TMaskMask FileNameMask;
        TMaskMask DirectoryMask;

        enum TMaskBoundary { None, Open, Close };

        TMaskBoundary HighSizeMask;
        __int64 HighSize;
        TMaskBoundary LowSizeMask;
        __int64 LowSize;

        TMaskBoundary HighModificationMask;
        nb::TDateTime HighModification;
        TMaskBoundary LowModificationMask;
        nb::TDateTime LowModification;

        std::wstring MaskStr;
        std::wstring UserStr;
    };

    typedef std::vector<TMask> TMasks;
  TMasks FMasks[4];
  mutable nb::TStrings * FMasksStr[4];

    void __fastcall SetStr(const std::wstring value, bool SingleMask);
    void __fastcall CreateMaskMask(const std::wstring Mask, size_t Start, size_t End, bool Ex,
                        TMaskMask &MaskMask);
    void __fastcall CreateMask(const std::wstring & MaskStr, int MaskStart,
        int MaskEnd, bool Include);
    nb::TStrings * __fastcall GetMasksStr(int Index) const;
    static std::wstring __fastcall MakeDirectoryMask(std::wstring Str);
    static inline void __fastcall ReleaseMaskMask(TMaskMask &MaskMask);
    inline void __fastcall Init();
    void __fastcall DoInit(bool Delete);
    inline void __fastcall Clear();
    static void __fastcall Clear(TMasks &Masks);
    static void __fastcall TrimEx(std::wstring &Str, size_t &Start, size_t &End);
    static bool __fastcall MatchesMasks(const std::wstring FileName, bool Directory,
                             const std::wstring Path, const TParams *Params, const TMasks &Masks, bool Recurse);
    static inline bool __fastcall MatchesMaskMask(const TMaskMask &MaskMask, const std::wstring Str);
    static inline bool __fastcall IsAnyMask(const std::wstring Mask);
    static std::wstring __fastcall ComposeMaskStr(nb::TStrings * MasksStr, bool Directory);
    void __fastcall ThrowError(size_t Start, size_t End);
};
//---------------------------------------------------------------------------
std::wstring __fastcall MaskFileName(const std::wstring FileName, const std::wstring Mask);
bool __fastcall IsFileNameMask(const std::wstring Mask);
std::wstring __fastcall DelimitFileNameMask(const std::wstring Mask);
//---------------------------------------------------------------------------
typedef void (nb::TObject::*TCustomCommandPatternEvent)
(int Index, const std::wstring Pattern, void *Arg, std::wstring &Replacement,
 bool &LastPass);
//---------------------------------------------------------------------------
class TCustomCommand
{
    friend class TInteractiveCustomCommand;

public:
    TCustomCommand();
    virtual ~TCustomCommand()
    {
    }

    std::wstring __fastcall Complete(const std::wstring Command, bool LastPass);
    virtual void __fastcall Validate(const std::wstring Command);

protected:
    static const wchar_t NoQuote;
    static const std::wstring Quotes;
    void __fastcall GetToken(const std::wstring Command,
                  size_t Index, size_t &Len, wchar_t &PatternCmd);
    void __fastcall CustomValidate(const std::wstring Command, void *Arg);
    virtual bool __fastcall FindPattern(const std::wstring Command, wchar_t PatternCmd);

    virtual void __fastcall ValidatePattern(const std::wstring Command,
                                 size_t Index, size_t Len, wchar_t PatternCmd, void *Arg);

    virtual size_t __fastcall PatternLen(size_t Index, wchar_t PatternCmd) = 0;
    virtual bool __fastcall PatternReplacement(size_t Index, const std::wstring Pattern,
                                    std::wstring &Replacement, bool &Delimit) = 0;
    virtual void __fastcall DelimitReplacement(std::wstring &Replacement, wchar_t Quote);
};
//---------------------------------------------------------------------------
class TInteractiveCustomCommand : public TCustomCommand
{
public:
    explicit TInteractiveCustomCommand(TCustomCommand *ChildCustomCommand);

protected:
    virtual void __fastcall Prompt(size_t Index, const std::wstring Prompt,
                        std::wstring &Value);
    virtual size_t __fastcall PatternLen(size_t Index, wchar_t PatternCmd);
    virtual bool __fastcall PatternReplacement(size_t Index, const std::wstring Pattern,
                                    std::wstring &Replacement, bool &Delimit);

private:
    TCustomCommand *FChildCustomCommand;
};
//---------------------------------------------------------------------------
class TTerminal;
struct TCustomCommandData
{
    TCustomCommandData();
    explicit TCustomCommandData(TTerminal *Terminal);

    std::wstring HostName;
    std::wstring UserName;
    std::wstring Password;
};
//---------------------------------------------------------------------------
class TFileCustomCommand : public TCustomCommand
{
public:
    TFileCustomCommand();
    explicit TFileCustomCommand(const TCustomCommandData &Data, const std::wstring Path);
    explicit TFileCustomCommand(const TCustomCommandData &Data, const std::wstring Path,
                                const std::wstring FileName, const std::wstring FileList);

    virtual void __fastcall Validate(const std::wstring Command);
    virtual void __fastcall ValidatePattern(const std::wstring Command,
                                 size_t Index, size_t Len, wchar_t PatternCmd, void *Arg);

    bool __fastcall IsFileListCommand(const std::wstring Command);
    virtual bool __fastcall IsFileCommand(const std::wstring Command);

protected:
    virtual size_t __fastcall PatternLen(size_t Index, wchar_t PatternCmd);
    virtual bool __fastcall PatternReplacement(size_t Index, const std::wstring Pattern,
                                    std::wstring &Replacement, bool &Delimit);

private:
    TCustomCommandData FData;
    std::wstring FPath;
    std::wstring FFileName;
    std::wstring FFileList;
};
//---------------------------------------------------------------------------
typedef TFileCustomCommand TRemoteCustomCommand;
//---------------------------------------------------------------------------
#endif
