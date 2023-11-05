
#pragma once

#include <rdestl/vector.h>
#include <Masks.hpp>
#include <Exceptions.h>
#include "SessionData.h"

class NB_CORE_EXPORT EFileMasksException : public Exception
{
public:
  explicit EFileMasksException(UnicodeString AMessage, int32_t AErrorStart, int32_t AErrorLen) noexcept;
  int32_t ErrorStart{0};
  int32_t ErrorLen{0};
};

extern const wchar_t IncludeExcludeFileMasksDelimiter;
#define MASK_INDEX(DIRECTORY, INCLUDE) ((DIRECTORY ? 2 : 0) + (INCLUDE ? 0 : 1))

class TTerminal;
class NB_CORE_EXPORT TFileMasks final : public TObject
{
friend class TTerminal;
public:
  struct NB_CORE_EXPORT TParams : public TObject
  {
    TParams() noexcept;
    int64_t Size{0};
    TDateTime Modification;

    UnicodeString ToString() const;
  };

  static bool IsMask(const UnicodeString Mask);
  static UnicodeString EscapeMask(const UnicodeString & S);
  static UnicodeString NormalizeMask(const UnicodeString Mask, const UnicodeString AnyMask = L"");
  static UnicodeString ComposeMaskStr(
    TStrings *IncludeFileMasksStr, TStrings *ExcludeFileMasksStr,
    TStrings *IncludeDirectoryMasksStr, TStrings *ExcludeDirectoryMasksStr);
  static UnicodeString ComposeMaskStr(TStrings *MasksStr, bool Directory);

  TFileMasks() noexcept;
  explicit TFileMasks(int32_t ForceDirectoryMasks) noexcept;
  TFileMasks(const TFileMasks &Source) noexcept;
  explicit TFileMasks(const UnicodeString AMasks) noexcept;
  virtual ~TFileMasks() noexcept;
  TFileMasks &operator=(const TFileMasks &rhm);
  TFileMasks &operator=(const UnicodeString & rhs);
  bool operator==(const TFileMasks &rhm) const;
  bool operator==(const UnicodeString & rhs) const;

  void SetMask(const UnicodeString & Mask);

  bool MatchesFileName(const UnicodeString & FileName, bool Directory = false, const TParams * Params = nullptr) const;
  bool Matches(const UnicodeString & FileName, bool Local, bool Directory,
    const TParams * Params = nullptr) const;
  bool Matches(const UnicodeString FileName, bool Local, bool Directory,
    const TParams * Params, bool RecurseInclude, bool & ImplicitMatch) const;

  void SetRoots(const UnicodeString & LocalRoot, const UnicodeString & RemoteRoot);
  void SetRoots(TStrings * LocalFileList, const UnicodeString & RemoteRoot);
  void SetRoots(const UnicodeString & LocalRoot, TStrings * RemoteFileList);

  __property UnicodeString Masks = { read = FStr, write = SetMasks };
  RWPropertySimple<UnicodeString> Masks{&FStr, nb::bind(&TFileMasks::SetMasks, this)};
  __property bool NoImplicitMatchWithDirExcludeMask = { read = FNoImplicitMatchWithDirExcludeMask, write = FNoImplicitMatchWithDirExcludeMask };
  __property bool AllDirsAreImplicitlyIncluded = { read = FAllDirsAreImplicitlyIncluded, write = FAllDirsAreImplicitlyIncluded };

  __property TStrings *IncludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, true) };
  __property TStrings *ExcludeFileMasksStr = { read = GetMasksStr, index = MASK_INDEX(false, false) };
  __property TStrings *IncludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, true) };
  __property TStrings *ExcludeDirectoryMasksStr = { read = GetMasksStr, index = MASK_INDEX(true, false) };

private:
  int32_t FForceDirectoryMasks{0};
  UnicodeString FStr;
  bool FNoImplicitMatchWithDirExcludeMask{false};
  bool FAllDirsAreImplicitlyIncluded{false};
  bool FAnyRelative{false};
  UnicodeString FLocalRoot;
  UnicodeString FRemoteRoot;

  struct TMask : public TObject
  {
    TMask() = default;
    enum TKind { Any, NoExt, Regular };

    TKind FileNameMaskKind;
    Masks::TMask * FileNameMask{nullptr};
    TKind DirectoryMaskKind;
    Masks::TMask * RemoteDirectoryMask{nullptr};
    Masks::TMask * LocalDirectoryMask{nullptr};

    enum TMaskBoundary { None, Open, Close };

    TMaskBoundary HighSizeMask;
  
    int64_t HighSize{0};
    TMaskBoundary LowSizeMask{None};
    int64_t LowSize{0};

    TMaskBoundary HighModificationMask;
    TDateTime HighModification;
    TMaskBoundary LowModificationMask;
    TDateTime LowModification;

    UnicodeString MaskStr;
    UnicodeString UserStr;
  };

  typedef nb::vector_t<TMask> TMasks;
  TMasks FMasks[4];
  mutable TStrings *FMasksStr[4]{};

private:
  void SetStr(const UnicodeString & Value, bool SingleMask);
  void SetMasks(const UnicodeString & Value);
  void CreateMaskMask(
    const UnicodeString Mask, int32_t Start, int32_t End, bool Ex, TMask::TKind & MaskKind, Masks::TMask *& MaskMask);
  void CreateMask(const UnicodeString MaskStr, int32_t MaskStart,
    int32_t MaskEnd, bool Include);
  TStrings *GetMasksStr(int32_t Index) const;
  static UnicodeString MakeDirectoryMask(UnicodeString AStr);
  inline void Init();
  void DoInit(bool Delete);
  void DoCopy(const TFileMasks & Source);
  void Clear();
  static void Clear(TMasks &Masks);
  static void TrimEx(UnicodeString &Str, int32_t &Start, int32_t &End);
  static bool MatchesMasks(
    const UnicodeString AFileName, bool Local, bool Directory,
    const UnicodeString APath, const TParams * Params, const TMasks & Masks, bool Recurse);
  static bool MatchesMaskMask(TMask::TKind MaskKind, Masks::TMask * MaskMask, const UnicodeString Str);
  static Masks::TMask * DoCreateMaskMask(const UnicodeString & Str);
  void ThrowError(int32_t Start, int32_t End) const;
  bool DoMatches(
    const UnicodeString & FileName, bool Local, bool Directory, const UnicodeString & Path, const TParams * Params,
    bool RecurseInclude, bool & ImplicitMatch) const;
};

UnicodeString MaskFileName(UnicodeString AFileName, const UnicodeString Mask);
bool IsFileNameMask(const UnicodeString AMask);
bool IsEffectiveFileNameMask(const UnicodeString AMask);
UnicodeString DelimitFileNameMask(UnicodeString AMask);

using TCustomCommandPatternEvent = nb::FastDelegate5<void,
  int32_t /*Index*/, UnicodeString /*Pattern*/, void * /*Arg*/, UnicodeString & /*Replacement*/,
   bool & /*LastPass*/>;

class NB_CORE_EXPORT TCustomCommand : public TObject
{
  friend class TInteractiveCustomCommand;

public:
  TCustomCommand() noexcept;
  // Needs an explicit virtual destructor, as is has virtual methods
  virtual ~TCustomCommand() = default;

  UnicodeString Complete(const UnicodeString & Command, bool LastPass);
  virtual void Validate(const UnicodeString & Command);
  bool HasAnyPatterns(const UnicodeString & Command) const;

  static UnicodeString Escape(const UnicodeString S);

protected:
  static const wchar_t NoQuote;
  static const UnicodeString Quotes;
  void GetToken(const UnicodeString & Command,
    int32_t Index, int32_t & Len, wchar_t & PatternCmd) const;
  void CustomValidate(const UnicodeString & Command, void * Arg);
  bool FindPattern(const UnicodeString & Command, wchar_t PatternCmd) const;

  virtual void ValidatePattern(const UnicodeString & Command,
    int32_t Index, int32_t Len, wchar_t PatternCmd, void *Arg);

  virtual int32_t PatternLen(const UnicodeString & Command, int32_t Index) const = 0;
  virtual void PatternHint(int32_t Index, const UnicodeString & Pattern);
  virtual bool PatternReplacement(int32_t Index, const UnicodeString & Pattern,
    UnicodeString &Replacement, bool & Delimit) const = 0;
  virtual void DelimitReplacement(UnicodeString & Replacement, wchar_t Quote);
};

class NB_CORE_EXPORT TInteractiveCustomCommand : public TCustomCommand
{
  NB_DISABLE_COPY(TInteractiveCustomCommand)
public:
  TInteractiveCustomCommand() = delete;
  explicit TInteractiveCustomCommand(TCustomCommand *ChildCustomCommand) noexcept;

protected:
  virtual void Prompt(int32_t Index, const UnicodeString Prompt,
    UnicodeString & Value) const;
  virtual void Execute(const UnicodeString & Command,
    UnicodeString & Value) const;
  virtual int32_t PatternLen(const UnicodeString & Command, int32_t Index) const override;
  virtual bool PatternReplacement(int32_t Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit) const override;
  void ParsePromptPattern(
    const UnicodeString Pattern, UnicodeString & Prompt, UnicodeString & Default, bool & Delimit) const;
  bool IsPromptPattern(const UnicodeString & Pattern) const;

private:
  TCustomCommand *FChildCustomCommand{nullptr};
};

class TSessionData;
struct NB_CORE_EXPORT TCustomCommandData : public TObject
{
public:
  TCustomCommandData() noexcept;
  explicit TCustomCommandData(const TCustomCommandData &Data) noexcept;
  explicit TCustomCommandData(TTerminal *Terminal) noexcept;
  explicit TCustomCommandData(TSessionData * SessionData);
  explicit TCustomCommandData(
    TSessionData *SessionData, const UnicodeString AUserName,
    const UnicodeString APassword) noexcept;

  __property TSessionData *SessionData = { read = GetSessionData };
  ROProperty<TSessionData *> SessionData{nb::bind(&TCustomCommandData::GetSessionData, this)};
  TSessionData *GetSessionData() const { return GetSessionDataPrivate(); }

  TCustomCommandData &operator=(const TCustomCommandData &Data);

private:
  std::unique_ptr<TSessionData> FSessionData;
  void Init(TSessionData * ASessionData);
  void Init(
    TSessionData *ASessionData, const UnicodeString AUserName,
    const UnicodeString APassword, const UnicodeString AHostKey);

  TSessionData *GetSessionDataPrivate() const;
};

class NB_CORE_EXPORT TFileCustomCommand : public TCustomCommand
{
public:
  TFileCustomCommand() noexcept;
  explicit TFileCustomCommand(const TCustomCommandData & Data, const UnicodeString & APath) noexcept;
  explicit TFileCustomCommand(const TCustomCommandData & Data, const UnicodeString & APath,
    const UnicodeString & AFileName, const UnicodeString & FileList) noexcept;
  virtual ~TFileCustomCommand() = default;

  virtual void Validate(const UnicodeString & Command) override;
  virtual void ValidatePattern(const UnicodeString & Command,
    int32_t Index, int32_t Len, wchar_t PatternCmd, void * Arg) override;

  bool IsFileListCommand(const UnicodeString & Command) const;
  virtual bool IsFileCommand(const UnicodeString & Command) const;
  bool IsRemoteFileCommand(const UnicodeString & Command) const;
  bool IsSiteCommand(const UnicodeString & Command) const;
  bool IsSessionCommand(const UnicodeString & Command) const;
  bool IsPasswordCommand(const UnicodeString & Command) const;

protected:
  virtual int32_t PatternLen(const UnicodeString & Command, int32_t Index) const override;
  virtual bool PatternReplacement(int32_t Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit) const override;

private:
  TCustomCommandData FData;
  UnicodeString FPath;
  UnicodeString FFileName;
  UnicodeString FFileList;
};

typedef TFileCustomCommand TRemoteCustomCommand;
extern UnicodeString FileMasksDelimiters;

