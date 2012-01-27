//---------------------------------------------------------------------------
#ifndef FileMasksH
#define FileMasksH
//---------------------------------------------------------------------------
#include <vector>
#include "Classes.h"
#include "Common.h"
//---------------------------------------------------------------------------
class EFileMasksException : public std::exception
{
public:
  EFileMasksException(const std::wstring Message, int ErrorStart, int ErrorLen);
  int ErrorStart;
  int ErrorLen;
};
//---------------------------------------------------------------------------
namespace Masks {

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
    __int64 Size;

    std::wstring ToString() const;
  };

  static bool IsMask(const std::wstring Mask);
  static std::wstring NormalizeMask(const std::wstring Mask, const std::wstring AnyMask = L"");

  TFileMasks();
  TFileMasks(const TFileMasks & Source);
  TFileMasks(const std::wstring AMasks);
  ~TFileMasks();
  TFileMasks & operator =(const TFileMasks & rhm);
  TFileMasks & operator =(const std::wstring rhs);
  bool operator ==(const TFileMasks & rhm) const;
  bool operator ==(const std::wstring rhs) const;

  void SetMask(const std::wstring Mask);
  void Negate();

  bool Matches(const std::wstring FileName, bool Directory = false,
    const std::wstring Path = L"", const TParams * Params = NULL) const;
  bool Matches(const std::wstring FileName, bool Local, bool Directory,
    const TParams * Params = NULL) const;
  bool GetIsValid(int &Start, int &Length) const;
  // __property std::wstring Masks = { read = FStr, write = SetMasks };
  std::wstring GetMasks() const { return FStr; }
  void SetMasks(const std::wstring value);
  
private:
  std::wstring FStr;

  struct TMaskMask
  {
    enum { Any, NoExt, Regular } Kind;
    Masks::TMask * Mask;
  };

  struct TMask
  {
    bool DirectoryOnly;
    TMaskMask FileNameMask;
    TMaskMask DirectoryMask;
    enum TSizeMask { None, Open, Close };
    TSizeMask HighSizeMask;
    __int64 HighSize;
    TSizeMask LowSizeMask;
    __int64 LowSize;
    std::wstring Str;
  };

  typedef std::vector<TMask> TMasks;
  TMasks FIncludeMasks;
  TMasks FExcludeMasks;

  void SetStr(const std::wstring value, bool SingleMask);
  void CreateMaskMask(const std::wstring Mask, int Start, int End, bool Ex,
    TMaskMask & MaskMask);
  static inline void ReleaseMaskMask(TMaskMask & MaskMask);
  inline void Clear();
  static void Clear(TMasks & Masks);
  static void TrimEx(std::wstring & Str, int & Start, int & End);
  static bool MatchesMasks(const std::wstring FileName, bool Directory,
    const std::wstring Path, const TParams * Params, const TMasks & Masks);
  static inline bool MatchesMaskMask(const TMaskMask & MaskMask, const std::wstring Str);
  static inline bool IsAnyMask(const std::wstring Mask);
  void ThrowError(int Start, int End);
};
//---------------------------------------------------------------------------
std::wstring MaskFileName(const std::wstring FileName, const std::wstring Mask);
bool IsFileNameMask(const std::wstring Mask);
std::wstring DelimitFileNameMask(const std::wstring Mask);
//---------------------------------------------------------------------------
typedef void (nb::TObject::*TCustomCommandPatternEvent)
  (int Index, const std::wstring Pattern, void * Arg, std::wstring & Replacement,
   bool & LastPass);
//---------------------------------------------------------------------------
class TCustomCommand
{
friend class TInteractiveCustomCommand;

public:
  TCustomCommand();
  virtual ~TCustomCommand()
  {
  }

  std::wstring Complete(const std::wstring Command, bool LastPass);
  virtual void Validate(const std::wstring Command);

protected:
  static const char NoQuote;
  static const std::wstring Quotes;
  void GetToken(const std::wstring Command,
    size_t Index, size_t &Len, char & PatternCmd);
  void CustomValidate(const std::wstring Command, void * Arg);
  bool FindPattern(const std::wstring Command, char PatternCmd);

  virtual void ValidatePattern(const std::wstring Command,
    size_t Index, size_t Len, char PatternCmd, void * Arg);

  virtual size_t PatternLen(size_t Index, char PatternCmd) = 0;
  virtual bool PatternReplacement(size_t Index, const std::wstring Pattern,
    std::wstring & Replacement, bool & Delimit) = 0;
  virtual void DelimitReplacement(std::wstring & Replacement, char Quote);
};
//---------------------------------------------------------------------------
class TInteractiveCustomCommand : public TCustomCommand
{
public:
  TInteractiveCustomCommand(TCustomCommand * ChildCustomCommand);

protected:
  virtual void Prompt(size_t Index, const std::wstring Prompt,
    std::wstring & Value);
  virtual size_t PatternLen(size_t Index, char PatternCmd);
  virtual bool PatternReplacement(size_t Index, const std::wstring Pattern,
    std::wstring & Replacement, bool & Delimit);

private:
  TCustomCommand * FChildCustomCommand;
};
//---------------------------------------------------------------------------
class TTerminal;
struct TCustomCommandData
{
  TCustomCommandData();
  TCustomCommandData(TTerminal * Terminal);

  std::wstring HostName;
  std::wstring UserName;
  std::wstring Password;
};
//---------------------------------------------------------------------------
class TFileCustomCommand : public TCustomCommand
{
public:
  TFileCustomCommand();
  explicit TFileCustomCommand(const TCustomCommandData & Data, const std::wstring Path);
  explicit TFileCustomCommand(const TCustomCommandData & Data, const std::wstring Path,
    const std::wstring FileName, const std::wstring FileList);

  virtual void Validate(const std::wstring Command);
  virtual void ValidatePattern(const std::wstring Command,
    size_t Index, size_t Len, char PatternCmd, void * Arg);

  bool IsFileListCommand(const std::wstring Command);
  virtual bool IsFileCommand(const std::wstring Command);

protected:
  virtual size_t PatternLen(size_t Index, char PatternCmd);
  virtual bool PatternReplacement(size_t Index, const std::wstring Pattern,
    std::wstring & Replacement, bool & Delimit);

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
