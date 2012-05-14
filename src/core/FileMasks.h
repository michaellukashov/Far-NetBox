//---------------------------------------------------------------------------
#ifndef FileMasksH
#define FileMasksH
//---------------------------------------------------------------------------
#include <vector>
#ifndef  _MSC_VER
#include <Masks.hpp>
#else
#include "boostdefines.hpp"
#include <boost/signals/signal5.hpp>

#include "Classes.h"
#include "Common.h"
#endif
//---------------------------------------------------------------------------
class EFileMasksException : public Exception
{
public:
  explicit /* __fastcall */ EFileMasksException(UnicodeString Message, int ErrorStart, int ErrorLen);
  int ErrorStart;
  int ErrorLen;
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
  explicit TMask(const UnicodeString Mask) :
    FMask(Mask)
  {
  }
  bool GetMatches(const UnicodeString Str);
private:
  UnicodeString FMask;
};

} // namespace Masks

//---------------------------------------------------------------------------
class TFileMasks
{
public:
  struct TParams
  {
    TParams();
    __int64 Size;
    TDateTime Modification;

    UnicodeString ToString() const;
  };

  static bool __fastcall IsMask(const UnicodeString Mask);
  static UnicodeString __fastcall NormalizeMask(const UnicodeString & Mask, const UnicodeString & AnyMask = L"");
  static UnicodeString __fastcall ComposeMaskStr(
    TStrings * IncludeFileMasksStr, TStrings * ExcludeFileMasksStr,
    TStrings * IncludeDirectoryMasksStr, TStrings * ExcludeDirectoryMasksStr);

  /* __fastcall */ TFileMasks();
  explicit /* __fastcall */ TFileMasks(int ForceDirectoryMasks);
  /* __fastcall */ TFileMasks(const TFileMasks & Source);
  explicit /* __fastcall */ TFileMasks(const UnicodeString & AMasks);
  virtual /* __fastcall */ ~TFileMasks();
  TFileMasks & __fastcall operator =(const TFileMasks & rhm);
  TFileMasks & __fastcall operator =(const UnicodeString & rhs);
  bool __fastcall operator ==(const TFileMasks & rhm) const;
  bool __fastcall operator ==(const UnicodeString & rhs) const;

  void __fastcall SetMask(const UnicodeString & Mask);

  bool __fastcall Matches(const UnicodeString FileName, bool Directory = false,
    const UnicodeString Path = L"", const TParams * Params = NULL) const;
  bool __fastcall Matches(const UnicodeString FileName, bool Local, bool Directory,
    const TParams * Params = NULL) const;
  bool __fastcall GetIsValid();
  bool __fastcall GetIsValid(int & Start, int & Length) const;

#ifndef  _MSC_VER
  __property UnicodeString Masks = { read = FStr, write = SetMasks };

  __property TStrings * IncludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, true) };
  __property TStrings * ExcludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, false) };
  __property TStrings * IncludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, true) };
  __property TStrings * ExcludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, false) };
#else
  UnicodeString __fastcall GetMasks() const { return FStr; }
  void __fastcall SetMasks(const UnicodeString value);

  TStrings * __fastcall GetIncludeFileMasksStr() { return GetMasksStr(MASK_INDEX(false, true)); };
  TStrings * __fastcall GetExcludeFileMasksStr() { return GetMasksStr(MASK_INDEX(false, false)); };
  TStrings * __fastcall GetIncludeDirectoryMasksStr() { return GetMasksStr(MASK_INDEX(true, true)); };
  TStrings * __fastcall GetExcludeDirectoryMasksStr() { return GetMasksStr(MASK_INDEX(true, false)); };
#endif

private:
  int FForceDirectoryMasks;
  UnicodeString FStr;

  struct TMaskMask
  {
    TMaskMask() :
      Kind(Any),
      Mask(NULL)
    {}
    enum { Any, NoExt, Regular } Kind;
    Masks::TMask * Mask;
  };

  struct TMask
  {
    TMask() :
      // FileNameMask
      // DirectoryMask
      HighSizeMask(None),
      HighSize(0),
      LowSizeMask(None),
      LowSize(0),
      HighModificationMask(None),
      // HighModification
      LowModificationMask(None)
      // LowModification
      // MaskStr
      // UserStr
    {}
    TMaskMask FileNameMask;
    TMaskMask DirectoryMask;

    enum TMaskBoundary { None, Open, Close };

    TMaskBoundary HighSizeMask;
    __int64 HighSize;
    TMaskBoundary LowSizeMask;
    __int64 LowSize;

    TMaskBoundary HighModificationMask;
    TDateTime HighModification;
    TMaskBoundary LowModificationMask;
    TDateTime LowModification;

    UnicodeString MaskStr;
    UnicodeString UserStr;
  };

  typedef std::vector<TMask> TMasks;
  TMasks FMasks[4];
  mutable TStrings * FMasksStr[4];

  void __fastcall SetStr(const UnicodeString value, bool SingleMask);
  // void __fastcall SetMasks(const UnicodeString value);
  void __fastcall CreateMaskMask(const UnicodeString & Mask, int Start, int End,
    bool Ex, TMaskMask & MaskMask);
  void __fastcall CreateMask(const UnicodeString & MaskStr, int MaskStart,
    int MaskEnd, bool Include);
  TStrings * __fastcall GetMasksStr(int Index) const;
  static UnicodeString __fastcall MakeDirectoryMask(UnicodeString Str);
  static inline void __fastcall ReleaseMaskMask(TMaskMask & MaskMask);
  inline void __fastcall Init();
  void __fastcall DoInit(bool Delete);
  void __fastcall Clear();
  static void __fastcall Clear(TMasks & Masks);
  static void __fastcall TrimEx(UnicodeString & Str, int & Start, int & End);
  static bool __fastcall MatchesMasks(const UnicodeString FileName, bool Directory,
    const UnicodeString Path, const TParams * Params, const TMasks & Masks, bool Recurse);
  static inline bool __fastcall MatchesMaskMask(const TMaskMask & MaskMask, const UnicodeString & Str);
  static inline bool __fastcall IsAnyMask(const UnicodeString & Mask);
  static UnicodeString __fastcall ComposeMaskStr(TStrings * MasksStr, bool Directory);
  void __fastcall ThrowError(int Start, int End);
};
//---------------------------------------------------------------------------
UnicodeString __fastcall MaskFileName(UnicodeString FileName, const UnicodeString Mask);
bool __fastcall IsFileNameMask(const UnicodeString Mask);
UnicodeString __fastcall DelimitFileNameMask(UnicodeString Mask);
//---------------------------------------------------------------------------
#ifndef  _MSC_VER
typedef void __fastcall (__closure * TCustomCommandPatternEvent)
  (int Index, const UnicodeString Pattern, void * Arg, UnicodeString & Replacement,
   bool & LastPass);
#else
typedef boost::signal5<void, int /* Index */, const UnicodeString /* Pattern */, void * /* Arg */, UnicodeString & /* Replacement */,
   bool & /* LastPass */ > TCustomCommandPatternSignal;
typedef TCustomCommandPatternSignal::slot_type TCustomCommandPatternEvent;
#endif
//---------------------------------------------------------------------------
class TCustomCommand
{
friend class TInteractiveCustomCommand;

public:
  TCustomCommand();
  virtual ~TCustomCommand() {}

  UnicodeString __fastcall Complete(const UnicodeString & Command, bool LastPass);
  virtual void __fastcall Validate(const UnicodeString & Command);

protected:
  static const wchar_t NoQuote;
  static const UnicodeString Quotes;
  void __fastcall GetToken(const UnicodeString & Command,
    int Index, int & Len, wchar_t & PatternCmd);
  void __fastcall CustomValidate(const UnicodeString & Command, void * Arg);
  bool __fastcall FindPattern(const UnicodeString & Command, wchar_t PatternCmd);

  virtual void __fastcall ValidatePattern(const UnicodeString & Command,
    int Index, int Len, wchar_t PatternCmd, void * Arg);

  virtual int __fastcall PatternLen(int Index, wchar_t PatternCmd) = 0;
  virtual bool __fastcall PatternReplacement(int Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit) = 0;
  virtual void __fastcall DelimitReplacement(UnicodeString & Replacement, wchar_t Quote);
};
//---------------------------------------------------------------------------
class TInteractiveCustomCommand : public TCustomCommand
{
public:
  explicit /* __fastcall */ TInteractiveCustomCommand(TCustomCommand * ChildCustomCommand);

protected:
  virtual void __fastcall Prompt(int Index, const UnicodeString & Prompt,
    UnicodeString & Value);
  virtual int __fastcall PatternLen(int Index, wchar_t PatternCmd);
  virtual bool __fastcall PatternReplacement(int Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit);

private:
  TCustomCommand * FChildCustomCommand;
};
//---------------------------------------------------------------------------
class TTerminal;
struct TCustomCommandData
{
  /* __fastcall */ TCustomCommandData();
  explicit /* __fastcall */ TCustomCommandData(TTerminal * Terminal);

  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString Password;
};
//---------------------------------------------------------------------------
class TFileCustomCommand : public TCustomCommand
{
public:
  /* __fastcall */ TFileCustomCommand();
  explicit /* __fastcall */ TFileCustomCommand(const TCustomCommandData & Data, const UnicodeString & Path);
  explicit /* __fastcall */ TFileCustomCommand(const TCustomCommandData & Data, const UnicodeString & Path,
    const UnicodeString & FileName, const UnicodeString & FileList);
  virtual /* __fastcall */ ~TFileCustomCommand() {}

  virtual void __fastcall Validate(const UnicodeString & Command);
  virtual void __fastcall ValidatePattern(const UnicodeString & Command,
    int Index, int Len, wchar_t PatternCmd, void * Arg);

  bool __fastcall IsFileListCommand(const UnicodeString & Command);
  virtual bool __fastcall IsFileCommand(const UnicodeString & Command);

protected:
  virtual int __fastcall PatternLen(int Index, wchar_t PatternCmd);
  virtual bool __fastcall PatternReplacement(int Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit);

private:
  TCustomCommandData FData;
  UnicodeString FPath;
  UnicodeString FFileName;
  UnicodeString FFileList;
};
//---------------------------------------------------------------------------
typedef TFileCustomCommand TRemoteCustomCommand;
//---------------------------------------------------------------------------
#endif
