//---------------------------------------------------------------------------
#ifndef FileMasksH
#define FileMasksH
//---------------------------------------------------------------------------
#include <vector>
// #include <Masks.hpp>
//---------------------------------------------------------------------------
class EFileMasksException : public exception
{
public:
  EFileMasksException(wstring Message, int ErrorStart, int ErrorLen);
  int ErrorStart;
  int ErrorLen;
};
//---------------------------------------------------------------------------
class TMask
{
};

//---------------------------------------------------------------------------
class TFileMasks
{
public:
  struct TParams
  {
    TParams();
    __int64 Size;

    wstring ToString() const;
  };

  static bool IsMask(const wstring Mask);
  static wstring NormalizeMask(const wstring & Mask, const wstring & AnyMask = L"");

  TFileMasks();
  TFileMasks(const TFileMasks & Source);
  TFileMasks(const wstring & AMasks);
  ~TFileMasks();
  TFileMasks & operator =(const TFileMasks & rhm);
  TFileMasks & operator =(const wstring & rhs);
  bool operator ==(const TFileMasks & rhm) const;
  bool operator ==(const wstring & rhs) const;

  void SetMask(const wstring & Mask);
  void Negate();

  bool Matches(const wstring FileName, bool Directory = false,
    const wstring Path = L"", const TParams * Params = NULL) const;
  bool Matches(const wstring FileName, bool Local, bool Directory,
    const TParams * Params = NULL) const;

  // __property wstring Masks = { read = FStr, write = SetMasks };
  wstring GetMasks() { return FStr; }
  void SetMasks(const wstring value);
  
private:
  wstring FStr;

  struct TMaskMask
  {
    enum { Any, NoExt, Regular } Kind;
    TMask * Mask;
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
    wstring Str;
  };

  typedef std::vector<TMask> TMasks;
  TMasks FIncludeMasks;
  TMasks FExcludeMasks;

  void SetStr(const wstring value, bool SingleMask);
  void CreateMaskMask(const wstring & Mask, int Start, int End, bool Ex,
    TMaskMask & MaskMask);
  static inline void ReleaseMaskMask(TMaskMask & MaskMask);
  inline void Clear();
  static void Clear(TMasks & Masks);
  static void TrimEx(wstring & Str, int & Start, int & End);
  static bool MatchesMasks(const wstring FileName, bool Directory,
    const wstring Path, const TParams * Params, const TMasks & Masks);
  static inline bool MatchesMaskMask(const TMaskMask & MaskMask, const wstring & Str);
  static inline bool IsAnyMask(const wstring & Mask);
  void ThrowError(int Start, int End);
};
//---------------------------------------------------------------------------
wstring MaskFileName(wstring FileName, const wstring Mask);
bool IsFileNameMask(const wstring Mask);
wstring DelimitFileNameMask(wstring Mask);
//---------------------------------------------------------------------------
typedef void (TObject::*TCustomCommandPatternEvent)
  (int Index, const wstring Pattern, void * Arg, wstring & Replacement,
   bool & LastPass);
//---------------------------------------------------------------------------
class TCustomCommand
{
friend class TInteractiveCustomCommand;

public:
  TCustomCommand();

  wstring Complete(const wstring & Command, bool LastPass);
  virtual void Validate(const wstring & Command);

protected:
  static const char NoQuote;
  static const wstring Quotes;
  void GetToken(const wstring & Command,
    int Index, int & Len, char & PatternCmd);
  void CustomValidate(const wstring & Command, void * Arg);
  bool FindPattern(const wstring & Command, char PatternCmd);

  virtual void ValidatePattern(const wstring & Command,
    int Index, int Len, char PatternCmd, void * Arg);

  virtual int PatternLen(int Index, char PatternCmd) = 0;
  virtual bool PatternReplacement(int Index, const wstring & Pattern,
    wstring & Replacement, bool & Delimit) = 0;
  virtual void DelimitReplacement(wstring & Replacement, char Quote);
};
//---------------------------------------------------------------------------
class TInteractiveCustomCommand : public TCustomCommand
{
public:
  TInteractiveCustomCommand(TCustomCommand * ChildCustomCommand);

protected:
  virtual void Prompt(int Index, const wstring & Prompt,
    wstring & Value);
  virtual int PatternLen(int Index, char PatternCmd);
  virtual bool PatternReplacement(int Index, const wstring & Pattern,
    wstring & Replacement, bool & Delimit);

private:
  TCustomCommand * FChildCustomCommand;
};
//---------------------------------------------------------------------------
class TTerminal;
struct TCustomCommandData
{
  TCustomCommandData();
  TCustomCommandData(TTerminal * Terminal);

  wstring HostName;
  wstring UserName;
  wstring Password;
};
//---------------------------------------------------------------------------
class TFileCustomCommand : public TCustomCommand
{
public:
  TFileCustomCommand();
  TFileCustomCommand(const TCustomCommandData & Data, const wstring & Path);
  TFileCustomCommand(const TCustomCommandData & Data, const wstring & Path,
    const wstring & FileName, const wstring & FileList);

  virtual void Validate(const wstring & Command);
  virtual void ValidatePattern(const wstring & Command,
    int Index, int Len, char PatternCmd, void * Arg);

  bool IsFileListCommand(const wstring & Command);
  virtual bool IsFileCommand(const wstring & Command);

protected:
  virtual int PatternLen(int Index, char PatternCmd);
  virtual bool PatternReplacement(int Index, const wstring & Pattern,
    wstring & Replacement, bool & Delimit);

private:
  TCustomCommandData FData;
  wstring FPath;
  wstring FFileName;
  wstring FFileList;
};
//---------------------------------------------------------------------------
typedef TFileCustomCommand TRemoteCustomCommand;
//---------------------------------------------------------------------------
#endif
