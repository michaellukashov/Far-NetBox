
#pragma once

#include <rdestl/vector.h>
#include <Masks.hpp>
#include <Exceptions.h>
#include "SessionData.h"

class NB_CORE_EXPORT EFileMasksException : public Exception
{
public:
  explicit EFileMasksException(UnicodeString AMessage, intptr_t AErrorStart, intptr_t AErrorLen);
  intptr_t ErrorStart;
  intptr_t ErrorLen;
};

extern const wchar_t IncludeExcludeFileMasksDelimiter;
#define MASK_INDEX(DIRECTORY, INCLUDE) ((DIRECTORY ? 2 : 0) + (INCLUDE ? 0 : 1))

class NB_CORE_EXPORT TFileMasks : public TObject
{
public:
  struct NB_CORE_EXPORT TParams : public TObject
  {
    TParams();
    int64_t Size;
    TDateTime Modification;

    UnicodeString ToString() const;
  };

  static bool IsMask(const UnicodeString Mask);
  static UnicodeString NormalizeMask(UnicodeString Mask, UnicodeString AnyMask = L"");
  static UnicodeString ComposeMaskStr(
    TStrings *IncludeFileMasksStr, TStrings *ExcludeFileMasksStr,
    TStrings *IncludeDirectoryMasksStr, TStrings *ExcludeDirectoryMasksStr);
  static UnicodeString ComposeMaskStr(TStrings *MasksStr, bool Directory);

  TFileMasks();
  explicit TFileMasks(intptr_t ForceDirectoryMasks);
  TFileMasks(const TFileMasks &Source);
  explicit TFileMasks(UnicodeString AMasks);
  virtual ~TFileMasks();
  TFileMasks &operator=(const TFileMasks &rhm);
  TFileMasks &operator=(UnicodeString rhs);
  bool operator==(const TFileMasks &rhm) const;
  bool operator==(UnicodeString rhs) const;

  void SetMask(UnicodeString Mask);

  bool Matches(const UnicodeString AFileName, bool Directory = false,
    const UnicodeString APath = L"", const TParams *Params = nullptr) const;
  bool Matches(const UnicodeString AFileName, bool Directory,
    const UnicodeString APath, const TParams *Params,
    bool RecurseInclude, bool &ImplicitMatch) const;
  bool Matches(const UnicodeString AFileName, bool Local, bool Directory,
    const TParams *Params = nullptr) const;
  bool Matches(const UnicodeString AFileName, bool Local, bool Directory,
    const TParams *Params, bool RecurseInclude, bool &ImplicitMatch) const;

#if 0
  __property UnicodeString Masks = { read = FStr, write = SetMasks };

  __property TStrings * IncludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, true) };
  __property TStrings * ExcludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, false) };
  __property TStrings * IncludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, true) };
  __property TStrings * ExcludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, false) };*/
#endif // #if 0

  UnicodeString GetMasks() const { return FStr; }
  void SetMasks(const UnicodeString Value) { SetMasksPrivate(Value); }

  TStrings *GetIncludeFileMasksStr() const { return GetMasksStr(MASK_INDEX(false, true)); }
  TStrings *GetExcludeFileMasksStr() const { return GetMasksStr(MASK_INDEX(false, false)); }
  TStrings *GetIncludeDirectoryMasksStr() const { return GetMasksStr(MASK_INDEX(true, true)); }
  TStrings *GetExcludeDirectoryMasksStr() const { return GetMasksStr(MASK_INDEX(true, false)); }

private:
  intptr_t FForceDirectoryMasks;
  UnicodeString FStr;

  struct TMaskMask : public TObject
  {
    TMaskMask() :
      Kind(Any),
      Mask(nullptr)
    {
    }
    enum
    {
      Any,
      NoExt,
      Regular,
    } Kind;
    Masks::TMask *Mask;
  };

  struct TMask : public TObject
  {
    TMask() :
      HighSizeMask(None),
      HighSize(0),
      LowSizeMask(None),
      LowSize(0),
      HighModificationMask(None),
      LowModificationMask(None)
    {
    }
    TMaskMask FileNameMask;
    TMaskMask DirectoryMask;

    enum TMaskBoundary
    {
      None,
      Open,
      Close,
    };

    TMaskBoundary HighSizeMask;
    int64_t HighSize;
    TMaskBoundary LowSizeMask;
    int64_t LowSize;

    TMaskBoundary HighModificationMask;
    TDateTime HighModification;
    TMaskBoundary LowModificationMask;
    TDateTime LowModification;

    UnicodeString MaskStr;
    UnicodeString UserStr;
  };

  typedef rde::vector<TMask> TMasks;
  TMasks FMasks[4];
  mutable TStrings *FMasksStr[4];

private:
  void SetStr(const UnicodeString Str, bool SingleMask);
  void SetMasksPrivate(const UnicodeString Value);
  void CreateMaskMask(UnicodeString Mask, intptr_t Start, intptr_t End,
    bool Ex, TMaskMask &MaskMask) const;
  void CreateMask(UnicodeString MaskStr, intptr_t MaskStart,
    intptr_t MaskEnd, bool Include);
  TStrings *GetMasksStr(intptr_t Index) const;
  static UnicodeString MakeDirectoryMask(UnicodeString AStr);
  static inline void ReleaseMaskMask(TMaskMask &MaskMask);
  inline void Init();
  void DoInit(bool Delete);
  void Clear();
  static void Clear(TMasks &Masks);
  static void TrimEx(UnicodeString &Str, intptr_t &Start, intptr_t &End);
  static bool MatchesMasks(const UnicodeString AFileName, bool Directory,
    const UnicodeString APath, const TParams *Params, const TMasks &Masks, bool Recurse);
  static inline bool MatchesMaskMask(const TMaskMask &MaskMask, UnicodeString Str);
  void ThrowError(intptr_t Start, intptr_t End) const;
};

UnicodeString MaskFileName(UnicodeString AFileName, const UnicodeString Mask);
bool IsFileNameMask(UnicodeString AMask);
bool IsEffectiveFileNameMask(UnicodeString AMask);
UnicodeString DelimitFileNameMask(UnicodeString AMask);

#if 0
typedef void (__closure *TCustomCommandPatternEvent)
(int Index, const UnicodeString Pattern, void *Arg, UnicodeString &Replacement,
  bool &LastPass);
#endif // #if 0
typedef nb::FastDelegate5<void,
        intptr_t /*Index*/, UnicodeString /*Pattern*/, void * /*Arg*/, UnicodeString & /*Replacement*/,
        bool & /*LastPass*/> TCustomCommandPatternEvent;

class NB_CORE_EXPORT TCustomCommand : public TObject
{
  friend class TInteractiveCustomCommand;

public:
  TCustomCommand();
  // Needs an explicit virtual destructor, as is has virtual methods
  virtual ~TCustomCommand() {}

  UnicodeString Complete(UnicodeString Command, bool LastPass);
  virtual void Validate(UnicodeString Command);
  bool HasAnyPatterns(UnicodeString Command) const;

  static UnicodeString Escape(UnicodeString S);

protected:
  static const wchar_t NoQuote;
  static const UnicodeString Quotes;
  void GetToken(UnicodeString Command,
    intptr_t Index, intptr_t &Len, wchar_t &PatternCmd) const;
  void CustomValidate(UnicodeString Command, void *Arg);
  bool FindPattern(UnicodeString Command, wchar_t PatternCmd) const;

  virtual void ValidatePattern(UnicodeString Command,
    intptr_t Index, intptr_t Len, wchar_t PatternCmd, void *Arg);

  virtual intptr_t PatternLen(UnicodeString Command, intptr_t Index) const = 0;
  virtual void PatternHint(intptr_t Index, UnicodeString Pattern);
  virtual bool PatternReplacement(intptr_t Index, UnicodeString Pattern,
    UnicodeString &Replacement, bool &Delimit) const = 0;
  virtual void DelimitReplacement(UnicodeString &Replacement, wchar_t Quote);
};

class NB_CORE_EXPORT TInteractiveCustomCommand : public TCustomCommand
{
  NB_DISABLE_COPY(TInteractiveCustomCommand)
public:
  explicit TInteractiveCustomCommand(TCustomCommand *ChildCustomCommand);

protected:
  virtual void Prompt(intptr_t Index, UnicodeString Prompt,
    UnicodeString &Value) const;
  virtual void Execute(UnicodeString Command,
    UnicodeString &Value) const;
  virtual intptr_t PatternLen(UnicodeString Command, intptr_t Index) const;
  virtual bool PatternReplacement(intptr_t Index, UnicodeString Pattern,
    UnicodeString &Replacement, bool &Delimit) const;
  void ParsePromptPattern(
    UnicodeString Pattern, UnicodeString &Prompt, UnicodeString &Default, bool &Delimit) const;
  bool IsPromptPattern(UnicodeString Pattern) const;

private:
  TCustomCommand *FChildCustomCommand;
};

class TTerminal;

struct NB_CORE_EXPORT TCustomCommandData : public TObject
{
//NB_DISABLE_COPY(TCustomCommandData)
public:
  TCustomCommandData();
  explicit TCustomCommandData(const TCustomCommandData &Data);
  explicit TCustomCommandData(TTerminal *Terminal);
  explicit TCustomCommandData(
    TSessionData *SessionData, UnicodeString AUserName,
    UnicodeString APassword);

#if 0
  __property TSessionData *SessionData = { read = GetSessionData };
#endif // #if 0
  TSessionData *GetSessionData() const { return GetSessionDataPrivate(); }

  TCustomCommandData &operator=(const TCustomCommandData &Data);

private:
  std::unique_ptr<TSessionData> FSessionData;
  void Init(
    TSessionData *ASessionData, UnicodeString AUserName,
    UnicodeString APassword, UnicodeString AHostKey);

  TSessionData *GetSessionDataPrivate() const;
};

class NB_CORE_EXPORT TFileCustomCommand : public TCustomCommand
{
public:
  TFileCustomCommand();
  explicit TFileCustomCommand(const TCustomCommandData &Data, UnicodeString APath);
  explicit TFileCustomCommand(const TCustomCommandData &Data, UnicodeString APath,
    UnicodeString AFileName, UnicodeString FileList);
  virtual ~TFileCustomCommand() {}

  virtual void Validate(UnicodeString Command);
  virtual void ValidatePattern(UnicodeString Command,
    intptr_t Index, intptr_t Len, wchar_t PatternCmd, void *Arg);

  bool IsFileListCommand(UnicodeString Command) const;
  virtual bool IsFileCommand(UnicodeString Command) const;
  bool IsRemoteFileCommand(UnicodeString Command) const;
  bool IsSiteCommand(UnicodeString Command) const;
  bool IsPasswordCommand(UnicodeString Command) const;

protected:
  virtual intptr_t PatternLen(UnicodeString Command, intptr_t Index) const;
  virtual bool PatternReplacement(intptr_t Index, UnicodeString Pattern,
    UnicodeString &Replacement, bool &Delimit) const;

private:
  TCustomCommandData FData;
  UnicodeString FPath;
  UnicodeString FFileName;
  UnicodeString FFileList;
};

typedef TFileCustomCommand TRemoteCustomCommand;
extern UnicodeString FileMasksDelimiters;
extern UnicodeString AnyMask;
