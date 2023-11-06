#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <StrUtils.hpp>

#include "FileMasks.h"

#include "TextsCore.h"
#include "Terminal.h"

const wchar_t IncludeExcludeFileMasksDelimiter = L'|';
UnicodeString FileMasksDelimiters = L";,";
static UnicodeString AllFileMasksDelimiters = FileMasksDelimiters + IncludeExcludeFileMasksDelimiter;
static UnicodeString DirectoryMaskDelimiters = L"/\\";
static UnicodeString FileMasksDelimiterStr = UnicodeString(FileMasksDelimiters[1]) + L' ';
static UnicodeString MaskSymbols = L"?*[";

EFileMasksException::EFileMasksException(
    UnicodeString AMessage, int32_t AErrorStart, int32_t AErrorLen) noexcept :
  Exception(AMessage),
  ErrorStart(AErrorStart),
  ErrorLen(AErrorLen)
{
}

static UnicodeString MaskFilePart(const UnicodeString Part, const UnicodeString Mask, bool& Masked)
{
  UnicodeString Result;
  int32_t RestStart = 1;
  bool Delim = false;
  for (int32_t Index = 1; Index <= Mask.Length(); ++Index)
  {
    switch (Mask[Index])
    {
    case L'\\':
      if (!Delim)
      {
        Delim = true;
        Masked = false;
        break;
      }

    case L'*':
      if (!Delim)
      {
        Result += Part.SubString(RestStart, Part.Length() - RestStart + 1);
        RestStart = Part.Length() + 1;
        Masked = true;
        break;
      }

    case L'?':
      if (!Delim)
      {
        if (RestStart <= Part.Length())
        {
          Result += Part[RestStart];
          RestStart++;
        }
        Masked = true;
        break;
      }

    default:
      Result += Mask[Index];
      RestStart++;
      Delim = false;
      break;
    }
  }
  return Result;
}

UnicodeString MaskFileName(UnicodeString AFileName, const UnicodeString Mask)
{
  UnicodeString FileName = AFileName;
  if (IsEffectiveFileNameMask(Mask))
  {
    bool Masked = false;
    int32_t P = Mask.LastDelimiter(L".");
    if (P > 0)
    {
      int32_t P2 = FileName.LastDelimiter(L".");
      // only dot at beginning of file name is not considered as
      // name/ext separator
      UnicodeString FileExt = P2 > 1 ?
        FileName.SubString(P2 + 1, FileName.Length() - P2) : UnicodeString();
      FileExt = MaskFilePart(FileExt, Mask.SubString(P + 1, Mask.Length() - P), Masked);
      if (P2 > 1)
      {
        FileName.SetLength(P2 - 1);
      }
      FileName = MaskFilePart(FileName, Mask.SubString(1, P - 1), Masked);
      if (!FileExt.IsEmpty())
      {
        FileName += L"." + FileExt;
      }
    }
    else
    {
      FileName = MaskFilePart(FileName, Mask, Masked);
    }
  }
  return FileName;
}

bool IsFileNameMask(const UnicodeString AMask)
{
  bool Result = AMask.IsEmpty(); // empty mask is the same as *
  if (!Result)
  {
    MaskFilePart(UnicodeString(), AMask, Result);
  }
  return Result;
}

bool IsEffectiveFileNameMask(const UnicodeString AMask)
{
  return !AMask.IsEmpty() && (AMask != L"*") && (AMask != L"*.*");
}

UnicodeString DelimitFileNameMask(UnicodeString AMask)
{
  UnicodeString Mask = AMask;
  for (int32_t Index = 1; Index <= Mask.Length(); ++Index)
  {
    if (wcschr(L"\\*?", Mask[Index]) != nullptr)
    {
      Mask.Insert(L"\\", Index);
      ++Index;
    }
  }
  return Mask;
}


TFileMasks::TParams::TParams() noexcept :
  Size(0)
{
}

UnicodeString TFileMasks::TParams::ToString() const
{
  return UnicodeString(L"[") + ::Int64ToStr(Size) + L"/" + ::DateTimeToString(Modification) + L"]";
}


bool TFileMasks::IsMask(const UnicodeString Mask)
{
  return (Mask.LastDelimiter(MaskSymbols) > 0);
}

UnicodeString TFileMasks::EscapeMask(const UnicodeString & S)
{
  UnicodeString Result = S;
  if (Result.LastDelimiter(MaskSymbols) > 0) // optimization
  {
    for (int Index = 1; Index < Result.Length(); Index++)
    {
      if (MaskSymbols.Pos(Result[Index]) > 0)
      {
        Result.Insert(L"[", Index);
        Index += 2;
        Result.Insert(L"]", Index);
      }
    }
  }
  return Result;
}

UnicodeString TFileMasks::NormalizeMask(const UnicodeString Mask, const UnicodeString AAnyMask)
{
  if (!IsEffectiveFileNameMask(Mask))
  {
    return AAnyMask;
  }
  return Mask;
}

UnicodeString TFileMasks::ComposeMaskStr(
  TStrings * MasksStr, bool Directory)
{
  UnicodeString Result;
  UnicodeString ResultNoDirMask;
  for (int32_t Index = 0; Index < MasksStr->GetCount(); ++Index)
  {
    UnicodeString Str = MasksStr->GetString(Index).Trim();
    if (!Str.IsEmpty())
    {
      for (int32_t P = 1; P <= Str.Length(); P++)
      {
        if (Str.IsDelimiter(AllFileMasksDelimiters, P))
        {
          Str.Insert(Str[P], P);
          P++;
        }
      }

      UnicodeString StrNoDirMask;
      if (Directory)
      {
        StrNoDirMask = Str;
        Str = MakeDirectoryMask(Str);
      }
      else
      {
        while (Str.IsDelimiter(DirectoryMaskDelimiters, Str.Length()))
        {
          Str.SetLength(Str.Length() - 1);
        }
        StrNoDirMask = Str;
      }

      AddToList(Result, Str, FileMasksDelimiterStr);
      AddToList(ResultNoDirMask, StrNoDirMask, FileMasksDelimiterStr);
    }
  }

  // For directories, the above will add slash at the end of masks,
  // breaking size and time masks and thus circumventing their validation.
  // This performs as hoc validation to cover the scenario.
  // For files this makes no difference, but no harm either
  TFileMasks Temp(Directory ? 1 : 0);
  Temp = ResultNoDirMask;

  return Result;
}

UnicodeString TFileMasks::ComposeMaskStr(
  TStrings *IncludeFileMasksStr, TStrings *ExcludeFileMasksStr,
  TStrings *IncludeDirectoryMasksStr, TStrings *ExcludeDirectoryMasksStr)
{
  UnicodeString IncludeMasks = ComposeMaskStr(IncludeFileMasksStr, false);
  AddToList(IncludeMasks, ComposeMaskStr(IncludeDirectoryMasksStr, true), FileMasksDelimiterStr);
  UnicodeString ExcludeMasks = ComposeMaskStr(ExcludeFileMasksStr, false);
  AddToList(ExcludeMasks, ComposeMaskStr(ExcludeDirectoryMasksStr, true), FileMasksDelimiterStr);

  UnicodeString Result = IncludeMasks;
  if (!ExcludeMasks.IsEmpty())
  {
    if (!Result.IsEmpty())
    {
      Result += L' ';
    }
    Result += UnicodeString(IncludeExcludeFileMasksDelimiter) + L' ' + ExcludeMasks;
  }
  return Result;
}

TFileMasks::TFileMasks() noexcept
{
  Init();
}

TFileMasks::TFileMasks(int32_t ForceDirectoryMasks) noexcept
{
  Init();
  FForceDirectoryMasks = ForceDirectoryMasks;
}

TFileMasks::TFileMasks(const TFileMasks &Source) noexcept
{
  Init();
  DoCopy(Source);
}

TFileMasks::TFileMasks(const UnicodeString AMasks) noexcept
{
  Init();
  SetStr(AMasks, false);
}

TFileMasks::~TFileMasks() noexcept
{
  Clear();
}

void TFileMasks::DoCopy(const TFileMasks & Source)
{
  FForceDirectoryMasks = Source.FForceDirectoryMasks;
  FNoImplicitMatchWithDirExcludeMask = Source.FNoImplicitMatchWithDirExcludeMask;
  FAllDirsAreImplicitlyIncluded = Source.FAllDirsAreImplicitlyIncluded;
  FLocalRoot = Source.FLocalRoot;
  FRemoteRoot = Source.FRemoteRoot;
  SetStr(Source.Masks, false);
}

void TFileMasks::Init()
{
  FForceDirectoryMasks = -1;
  FNoImplicitMatchWithDirExcludeMask = false;
  FAllDirsAreImplicitlyIncluded = false;
  FAnyRelative = false;

  DoInit(false);
}

void TFileMasks::DoInit(bool Delete)
{
  for (size_t Index = 0; Index < LENOF(FMasksStr); Index++)
  {
    if (Delete)
    {
      SAFE_DESTROY(FMasksStr[Index]);
    }
    FMasksStr[Index] = nullptr;
  }
}

void TFileMasks::Clear()
{
  DoInit(true);

  for (size_t Index = 0; Index < LENOF(FMasks); Index++)
  {
    Clear(FMasks[Index]);
  }
}

void TFileMasks::Clear(TMasks & Masks)
{
  TMasks::iterator it = Masks.begin();
  while (it != Masks.end())
  {
    delete (*it).FileNameMask;
    delete (*it).RemoteDirectoryMask;
    delete (*it).LocalDirectoryMask;
    ++it;
  }
  Masks.clear();
}

bool TFileMasks::MatchesMasks(
  const UnicodeString FileName, bool Local, bool Directory,
  const UnicodeString Path, const TParams * Params, const TMasks & Masks, bool Recurse)
{
  bool Result = false;

  TMasks::const_iterator it = Masks.begin();
  while (!Result && (it != Masks.end()))
  {
    const TMask & Mask = *it;
    Masks::TMask * DirectoryMask = Local ? Mask.LocalDirectoryMask : Mask.RemoteDirectoryMask;
    Result =
      MatchesMaskMask(Mask.DirectoryMaskKind, DirectoryMask, Path) &&
      MatchesMaskMask(Mask.FileNameMaskKind, Mask.FileNameMask, FileName);

    if (Result)
    {
      bool HasSize = (Params != nullptr);

      switch (Mask.HighSizeMask)
      {
      case TMask::None:
        Result = true;
        break;

      case TMask::Open:
        Result = HasSize && (Params->Size < Mask.HighSize);
        break;

      case TMask::Close:
        Result = HasSize && (Params->Size <= Mask.HighSize);
        break;
      }

      if (Result)
      {
        switch (Mask.LowSizeMask)
        {
        case TMask::None:
          Result = true;
          break;

        case TMask::Open:
          Result = HasSize && (Params->Size > Mask.LowSize);
          break;

        case TMask::Close:
          Result = HasSize && (Params->Size >= Mask.LowSize); //-V595
          break;
        }
      }

      bool HasModification = (Params != nullptr);

      if (Result)
      {
        switch (Mask.HighModificationMask)
        {
        case TMask::None:
          Result = true;
          break;

        case TMask::Open:
          Result = HasModification && (Params->Modification < Mask.HighModification);
          break;

        case TMask::Close:
          Result = HasModification && (Params->Modification <= Mask.HighModification);
          break;
        }
      }

      if (Result)
      {
        switch (Mask.LowModificationMask)
        {
        case TMask::None:
          Result = true;
          break;

        case TMask::Open:
          Result = HasModification && (Params->Modification > Mask.LowModification);
          break;

        case TMask::Close:
          Result = HasModification && (Params->Modification >= Mask.LowModification);
          break;
        }
      }
    }

    ++it;
  }

  if (!Result && Directory && !base::IsUnixRootPath(Path) && Recurse)
  {
    UnicodeString ParentFileName = base::UnixExtractFileName(Path);
    UnicodeString ParentPath = base::SimpleUnixExcludeTrailingBackslash(base::UnixExtractFilePath(Path));
    // Pass Params down or not?
    // Currently it includes Size/Time only, what is not used for directories.
    // So it depends on future use. Possibly we should make a copy
    // and pass on only relevant fields.
    Result = MatchesMasks(ParentFileName, Local, true, ParentPath, Params, Masks, Recurse);
  }

  return Result;
}

bool TFileMasks::DoMatches(
  const UnicodeString & FileName, bool Local, bool Directory, const UnicodeString & Path, const TParams * Params,
  bool RecurseInclude, bool & ImplicitMatch) const
{
  bool ImplicitIncludeMatch = (FAllDirsAreImplicitlyIncluded && Directory) || FMasks[MASK_INDEX(Directory, true)].empty();
  bool ExplicitIncludeMatch = MatchesMasks(FileName, Local, Directory, Path, Params, FMasks[MASK_INDEX(Directory, true)], RecurseInclude);
  bool Result =
    (ImplicitIncludeMatch || ExplicitIncludeMatch) &&
    !MatchesMasks(FileName, Local, Directory, Path, Params, FMasks[MASK_INDEX(Directory, false)], false);
  ImplicitMatch =
    Result && ImplicitIncludeMatch && !ExplicitIncludeMatch &&
    ((Directory && FNoImplicitMatchWithDirExcludeMask) || FMasks[MASK_INDEX(Directory, false)].empty());
  return Result;
}

bool TFileMasks::Matches(const UnicodeString & FileName, bool Local,
  bool Directory, const TParams * Params) const
{
  bool ImplicitMatch;
  return Matches(FileName, Local, Directory, Params, true, ImplicitMatch);
}

bool TFileMasks::Matches(const UnicodeString FileName, bool Local,
  bool Directory, const TParams * Params, bool RecurseInclude, bool & ImplicitMatch) const
{
  bool Result;
  if (Local)
  {
    UnicodeString Path = ::ExtractFilePath(FileName);
    if (!Path.IsEmpty())
    {
      Path = base::ToUnixPath(::ExcludeTrailingBackslash(Path));
    }
    Result = DoMatches(base::ExtractFileName(FileName, false), Local, Directory, Path, Params,
      RecurseInclude, ImplicitMatch);
  }
  else
  {
    Result = DoMatches(base::UnixExtractFileName(FileName), Local, Directory,
      base::SimpleUnixExcludeTrailingBackslash(base::UnixExtractFilePath(FileName)), Params,
      RecurseInclude, ImplicitMatch);
  }
  return Result;
}

bool TFileMasks::MatchesFileName(const UnicodeString & FileName, bool Directory, const TParams * Params) const
{
  bool ImplicitMatch;
  return DoMatches(FileName, false, Directory, EmptyStr, Params, true, ImplicitMatch);
}

bool TFileMasks::operator ==(const TFileMasks & rhm) const
{
  return (Masks() == rhm.Masks());
}

TFileMasks & TFileMasks::operator =(const UnicodeString & rhs)
{
  SetMasks(rhs);
  return *this;
}

TFileMasks & TFileMasks::operator =(const TFileMasks & rhm)
{
  DoInit(true);
  DoCopy(rhm);
  return *this;
}

bool TFileMasks::operator ==(const UnicodeString & rhs) const
{
  return (Masks() == rhs);
}

void TFileMasks::ThrowError(int32_t Start, int32_t End) const
{
  throw EFileMasksException(
    FMTLOAD(MASK_ERROR, Masks().SubString(Start, End - Start + 1)),
    Start, End - Start + 1);
}

Masks::TMask * TFileMasks::DoCreateMaskMask(const UnicodeString & Str)
{
  return new Masks::TMask(Str);
}

void TFileMasks::CreateMaskMask(
  const UnicodeString Mask, int32_t Start, int32_t End, bool Ex, TMask::TKind & MaskKind, Masks::TMask *& MaskMask)
{
  try
  {
    DebugAssert(MaskMask == nullptr);
    if (Ex && !IsEffectiveFileNameMask(Mask))
    {
      MaskKind = TMask::TKind::Any;
      MaskMask = nullptr;
    }
    else
    {
      MaskKind = (Ex && (Mask == L"*.")) ? TMask::TKind::NoExt : TMask::TKind::Regular;
      MaskMask = DoCreateMaskMask(Mask);
    }
  }
  catch(...)
  {
    ThrowError(Start, End);
  }
}

UnicodeString TFileMasks::MakeDirectoryMask(UnicodeString Str)
{
  DebugAssert(!Str.IsEmpty());
  if (Str.IsEmpty() || !Str.IsDelimiter(DirectoryMaskDelimiters, Str.Length()))
  {
    int32_t D = Str.LastDelimiter(DirectoryMaskDelimiters);
    // if there's any [back]slash anywhere in str,
    // add the same [back]slash at the end, otherwise add slash
    wchar_t Delimiter = (D > 0) ? Str[D] : DirectoryMaskDelimiters[1];
    Str += Delimiter;
  }
  return Str;
}

void TFileMasks::CreateMask(
  const UnicodeString MaskStr, int32_t MaskStart, int32_t /*MaskEnd*/, bool Include)
{
  bool Directory = false; // shut up
  TMask Mask;

  Mask.MaskStr = MaskStr;
  Mask.UserStr = MaskStr;
  Mask.FileNameMaskKind = TMask::TKind::Any;
  Mask.FileNameMask = nullptr;
  Mask.DirectoryMaskKind = TMask::TKind::Any;
  Mask.RemoteDirectoryMask = nullptr;
  Mask.LocalDirectoryMask = nullptr;
  Mask.HighSizeMask = TMask::None;
  Mask.LowSizeMask = TMask::None;
  Mask.HighModificationMask = TMask::None;
  Mask.LowModificationMask = TMask::None;

  wchar_t NextPartDelimiter = L'\0';
  int32_t NextPartFrom = 1;
  while (NextPartFrom <= MaskStr.Length())
  {
    wchar_t PartDelimiter = NextPartDelimiter;
    int32_t PartFrom = NextPartFrom;
    UnicodeString PartStr = CopyToChars(MaskStr, NextPartFrom, "<>", false, &NextPartDelimiter, true);

    int32_t PartStart = MaskStart + PartFrom - 1;
    int32_t PartEnd = MaskStart + NextPartFrom - 1 - 2;

    TrimEx(PartStr, PartStart, PartEnd);

    if (PartDelimiter != L'\0')
    {
      bool Low = (PartDelimiter == L'>');

      TMask::TMaskBoundary Boundary;
      if ((PartStr.Length() >= 1) && (PartStr[1] == L'='))
      {
        Boundary = TMask::Close;
        PartStr.Delete(1, 1);
      }
      else
      {
        Boundary = TMask::Open;
      }

      TDateTime Modification;
      int64_t DummySize;
      if ((!TryStrToInt64(PartStr, DummySize) && TryStrToDateTimeStandard(PartStr, Modification)) ||
        TryRelativeStrToDateTime(PartStr, Modification, false))
      {
        TMask::TMaskBoundary &ModificationMask =
          (Low ? Mask.LowModificationMask : Mask.HighModificationMask);

        if ((ModificationMask != TMask::None) || Directory)
        {
          // include delimiter into size part
          ThrowError(PartStart - 1, PartEnd);
        }

        ModificationMask = Boundary;
        (Low ? Mask.LowModification : Mask.HighModification) = Modification;
      }
      else
      {
        TMask::TMaskBoundary &SizeMask = (Low ? Mask.LowSizeMask : Mask.HighSizeMask);
        int64_t &Size = (Low ? Mask.LowSize : Mask.HighSize);

        if ((SizeMask != TMask::None) || Directory)
        {
          // include delimiter into size part
          ThrowError(PartStart - 1, PartEnd);
        }

        SizeMask = Boundary;
        if (!TryStrToSize(PartStr, Size))
        {
          ThrowError(PartStart, PartEnd);
        }
      }
    }
    else if (!PartStr.IsEmpty())
    {
      int32_t D = PartStr.LastDelimiter(DirectoryMaskDelimiters);

      Directory = (D > 0) && (D == PartStr.Length());

      if (Directory)
      {
        do
        {
          PartStr.SetLength(PartStr.Length() - 1);
          Mask.UserStr.Delete(PartStart - MaskStart + D, 1);
          D--;
        }
        while (!PartStr.IsEmpty() && PartStr.IsDelimiter(DirectoryMaskDelimiters, PartStr.Length()));

        D = PartStr.LastDelimiter(DirectoryMaskDelimiters);

        if (FForceDirectoryMasks == 0)
        {
          Directory = false;
          Mask.MaskStr = Mask.UserStr;
        }
      }
      else if (FForceDirectoryMasks > 0)
      {
        Directory = true;
        Mask.MaskStr.Insert(DirectoryMaskDelimiters[1], PartStart - MaskStart + PartStr.Length());
      }

      if (D > 0)
      {
        // make sure sole "/" (root dir) is preserved as is
        UnicodeString DirectoryMaskStr = base::SimpleUnixExcludeTrailingBackslash(base::ToUnixPath(PartStr.SubString(1, D)));
        UnicodeString RemoteDirectoryMaskStr = DirectoryMaskStr;
        UnicodeString LocalDirectoryMaskStr = DirectoryMaskStr;

        const UnicodeString RelativePrefix = L".";
        const UnicodeString RelativePrefixWithSlash = RelativePrefix + L"/";
        if (DirectoryMaskStr == RelativePrefix)
        {
          FAnyRelative = true;
          if (!FRemoteRoot.IsEmpty())
          {
            RemoteDirectoryMaskStr = base::SimpleUnixExcludeTrailingBackslash(FRemoteRoot);
          }
          if (!FLocalRoot.IsEmpty())
          {
            LocalDirectoryMaskStr = base::SimpleUnixExcludeTrailingBackslash(FLocalRoot);
          }
        }
        else if (StartsStr(RelativePrefixWithSlash, DirectoryMaskStr))
        {
          FAnyRelative = true;
          DirectoryMaskStr.Delete(1, RelativePrefixWithSlash.Length());
          if (!FRemoteRoot.IsEmpty())
          {
            RemoteDirectoryMaskStr = FRemoteRoot + DirectoryMaskStr;
          }
          if (!FLocalRoot.IsEmpty())
          {
            LocalDirectoryMaskStr = FLocalRoot + DirectoryMaskStr;
          }
        }

        CreateMaskMask(
          RemoteDirectoryMaskStr, PartStart, PartStart + D - 1, false,
          Mask.DirectoryMaskKind, Mask.RemoteDirectoryMask);
        if (Mask.RemoteDirectoryMask != nullptr)
        {
          Mask.LocalDirectoryMask = DoCreateMaskMask(LocalDirectoryMaskStr);
        }
        CreateMaskMask(
          PartStr.SubString(D + 1, PartStr.Length() - D),
          PartStart + D, PartEnd, true,
          Mask.FileNameMaskKind, Mask.FileNameMask);
      }
      else
      {
        CreateMaskMask(PartStr, PartStart, PartEnd, true, Mask.FileNameMaskKind, Mask.FileNameMask);
      }
    }
  }

  FMasks[MASK_INDEX(Directory, Include)].push_back(Mask);
}

TStrings *TFileMasks::GetMasksStr(int32_t Index) const
{
  if (FMasksStr[Index] == nullptr)
  {
    FMasksStr[Index] = new TStringList();
    TMasks::const_iterator it = FMasks[Index].begin();
    while (it != FMasks[Index].end())
    {
      FMasksStr[Index]->Add((*it).UserStr);
      ++it;
    }
  }

  return FMasksStr[Index];
}

void TFileMasks::TrimEx(UnicodeString &Str, int32_t &Start, int32_t &End)
{
  UnicodeString Buf = ::TrimLeft(Str);
  Start += Str.Length() - Buf.Length();
  Str = ::TrimRight(Buf);
  End -= Buf.Length() - Str.Length();
}

bool TFileMasks::MatchesMaskMask(TMask::TKind MaskKind, Masks::TMask * MaskMask, const UnicodeString Str)
{
  bool Result;
  if (MaskKind == TMask::TKind::Any)
  {
    Result = true;
  }
  else if ((MaskKind == TMask::TKind::NoExt) && (Str.Pos(L".") == 0))
  {
    Result = true;
  }
  else
  {
    Result = MaskMask->Matches(Str);
  }
  return Result;
}

void TFileMasks::SetMasks(const UnicodeString & Value)
{
  if (FStr != Value)
  {
    SetStr(Value, false);
  }
}

void TFileMasks::SetMask(const UnicodeString & Mask)
{
  SetStr(Mask, true);
}

void TFileMasks::SetStr(const UnicodeString & Str, bool SingleMask)
{
  FAnyRelative = false;
  UnicodeString Backup = FStr;
  try
  {
    FStr = Str;
    Clear();

    int32_t NextMaskFrom = 1;
    bool Include = true;
    while (NextMaskFrom <= Str.Length())
    {
      int32_t MaskStart = NextMaskFrom;
      wchar_t NextMaskDelimiter;
      UnicodeString MaskStr;
      if (SingleMask)
      {
        MaskStr = Str;
        NextMaskFrom = Str.Length() + 1;
        NextMaskDelimiter = L'\0';
      }
      else
      {
        MaskStr = CopyToChars(Str, NextMaskFrom, AllFileMasksDelimiters, false, &NextMaskDelimiter, true);
      }
      int32_t MaskEnd = NextMaskFrom - 2;

      TrimEx(MaskStr, MaskStart, MaskEnd);

      if (!MaskStr.IsEmpty())
      {
        CreateMask(MaskStr, MaskStart, MaskEnd, Include);
      }

      if (NextMaskDelimiter == IncludeExcludeFileMasksDelimiter)
      {
        if (Include)
        {
          Include = false;
        }
        else
        {
          ThrowError(NextMaskFrom - 1, Str.Length());
        }
      }
    }
  }
  catch(...)
  {
    // this does not work correctly if previous mask was set using SetMask.
    // this should not fail (the mask was validated before),
    // otherwise we end in an infinite loop
    SetStr(Backup, false);
    throw;
  }
}

void TFileMasks::SetRoots(const UnicodeString & LocalRoot, const UnicodeString & RemoteRoot)
{
  if (FAnyRelative) // optimization
  {
    FLocalRoot = EscapeMask(base::UnixIncludeTrailingBackslash(base::ToUnixPath(LocalRoot)));
    FRemoteRoot = EscapeMask(base::UnixIncludeTrailingBackslash(RemoteRoot));
    SetStr(FStr, false);
  }
}

void TFileMasks::SetRoots(TStrings * LocalFileList, const UnicodeString & RemoteRoot)
{
  if (FAnyRelative) // optimization
  {
    UnicodeString LocalRoot;
    base::ExtractCommonPath(LocalFileList, LocalRoot);
    SetRoots(LocalRoot, RemoteRoot);
  }
}

void TFileMasks::SetRoots(const UnicodeString & LocalRoot, TStrings * RemoteFileList)
{
  if (FAnyRelative) // optimization
  {
    UnicodeString RemoteRoot;
    base::UnixExtractCommonPath(RemoteFileList, RemoteRoot);
    SetRoots(LocalRoot, RemoteRoot);
  }
}


#define TEXT_TOKEN L'\255'

const wchar_t TCustomCommand::NoQuote = L'\0';
const UnicodeString TCustomCommand::Quotes = L"\"'";

UnicodeString TCustomCommand::Escape(const UnicodeString S)
{
  return ReplaceStr(S, L"!", L"!!");
}

TCustomCommand::TCustomCommand() noexcept
{
}

void TCustomCommand::GetToken(
  const UnicodeString & Command, int32_t Index, int32_t & Len, wchar_t & PatternCmd) const
{
  DebugAssert(Index <= Command.Length());
  const wchar_t * Ptr = Command.c_str() + Index - 1;

  if (Ptr[0] == L'!')
  {
    PatternCmd = Ptr[1];
    if (PatternCmd == L'\0')
    {
      Len = 1;
    }
    else if (PatternCmd == L'!')
    {
      Len = 2;
    }
    else
    {
      Len = PatternLen(Command, Index);
    }

    if (Len <= 0)
    {
      throw Exception(FMTLOAD(CUSTOM_COMMAND_UNKNOWN, PatternCmd, Index));
    }
    else
    {
      if ((Command.Length() - Index + 1) < Len)
      {
        throw Exception(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, PatternCmd, Index));
      }
    }
  }
  else
  {
    PatternCmd = TEXT_TOKEN;
    const wchar_t *NextPattern = wcschr(Ptr, L'!');
    if (NextPattern == nullptr)
    {
      Len = Command.Length() - Index + 1;
    }
    else
    {
      Len = NextPattern - Ptr;
    }
  }
}

void TCustomCommand::PatternHint(int32_t /*Index*/, const UnicodeString & /*Pattern*/)
{
  // noop
}

UnicodeString TCustomCommand::Complete(const UnicodeString & Command,
  bool LastPass)
{
  int32_t Index = 1;
  int32_t PatternIndex = 0;
  while (Index <= Command.Length())
  {
    int32_t Len;
    wchar_t PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);

    if (PatternCmd == TEXT_TOKEN)
    {
    }
    else if (PatternCmd == L'!')
    {
    }
    else
    {
      UnicodeString Pattern = Command.SubString(Index, Len);
      PatternHint(PatternIndex, Pattern);
      PatternIndex++;
    }

    Index += Len;
  }

  UnicodeString Result;
  Index = 1;
  PatternIndex = 0;
  while (Index <= Command.Length())
  {
    int32_t Len;
    wchar_t PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);

    if (PatternCmd == TEXT_TOKEN)
    {
      Result += Command.SubString(Index, Len);
    }
    else if (PatternCmd == L'!')
    {
      if (LastPass)
      {
        Result += L'!';
      }
      else
      {
        Result += Command.SubString(Index, Len);
      }
    }
    else
    {
      wchar_t Quote = NoQuote;
      if ((Index > 1) && (Index + Len - 1 < Command.Length()) &&
          Command.IsDelimiter(Quotes, Index - 1) &&
          Command.IsDelimiter(Quotes, Index + Len) &&
          (Command[Index - 1] == Command[Index + Len]))
      {
        Quote = Command[Index - 1];
      }
      UnicodeString Pattern = Command.SubString(Index, Len);
      UnicodeString Replacement;
      bool Delimit = true;
      if (PatternReplacement(PatternIndex, Pattern, Replacement, Delimit))
      {
        if (!LastPass)
        {
          Replacement = Escape(Replacement);
        }
        if (Delimit)
        {
          DelimitReplacement(Replacement, Quote);
        }
        Result += Replacement;
      }
      else
      {
        Result += Pattern;
      }

      PatternIndex++;
    }

    Index += Len;
  }

  return Result;
}

void TCustomCommand::DelimitReplacement(UnicodeString &Replacement, wchar_t Quote)
{
  Replacement = DelimitStr(Replacement, Quote);
}

void TCustomCommand::Validate(const UnicodeString & Command)
{
  CustomValidate(Command, nullptr);
}

void TCustomCommand::CustomValidate(const UnicodeString & Command,
  void *Arg)
{
  int32_t Index = 1;

  while (Index <= Command.Length())
  {
    int32_t Len;
    wchar_t PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);
    ValidatePattern(Command, Index, Len, PatternCmd, Arg);

    Index += Len;
  }
}

bool TCustomCommand::FindPattern(const UnicodeString & Command,
  wchar_t PatternCmd) const
{
  bool Result = false;
  int32_t Index = 1;

  while (!Result && (Index <= Command.Length()))
  {
    int32_t Len;
    wchar_t APatternCmd;
    GetToken(Command, Index, Len, APatternCmd);
    if (((PatternCmd != L'!') && (towlower(PatternCmd) == towlower(APatternCmd))) ||
        ((PatternCmd == L'!') && (Len == 1) && (APatternCmd != TEXT_TOKEN)) ||
        ((PatternCmd == L'\0') && (APatternCmd != TEXT_TOKEN)))
    {
      Result = true;
    }

    Index += Len;
  }

  return Result;
}

bool TCustomCommand::HasAnyPatterns(const UnicodeString & Command) const
{
  return FindPattern(Command, L'\0');
}

void TCustomCommand::ValidatePattern(const UnicodeString & /*Command*/,
  int32_t /*Index*/, int32_t /*Len*/, wchar_t /*PatternCmd*/, void * /*Arg*/)
{
}


TInteractiveCustomCommand::TInteractiveCustomCommand(
  TCustomCommand * ChildCustomCommand) noexcept
{
  FChildCustomCommand = ChildCustomCommand;
}

void TInteractiveCustomCommand::Prompt(
  int32_t /*Index*/, const UnicodeString & /*Prompt*/, UnicodeString & Value) const
{
  Value.Clear();
}

void TInteractiveCustomCommand::Execute(
  const UnicodeString & /*Command*/, UnicodeString & Value) const
{
  Value.Clear();
}

int32_t TInteractiveCustomCommand::PatternLen(const UnicodeString & Command, int32_t Index) const
{
  int32_t Len;
  wchar_t PatternCmd = (Index < Command.Length()) ? Command[Index + 1] : L'\0';
  switch (PatternCmd)
  {
    case L'?':
      {
        const wchar_t * Ptr = Command.c_str() + Index - 1;
        const wchar_t * PatternEnd = wcschr(Ptr + 1, L'!');
        if (PatternEnd == nullptr)
        {
          throw Exception(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, Command[Index + 1], Index));
        }
        Len = PatternEnd - Ptr + 1;
      }
      break;

    case L'`':
      {
        const wchar_t * Ptr = Command.c_str() + Index - 1;
        const wchar_t * PatternEnd = wcschr(Ptr + 2, L'`');
        if (PatternEnd == nullptr)
        {
          throw Exception(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, Command[Index + 1], Index));
        }
        Len = PatternEnd - Ptr + 1;
      }
      break;

    default:
      Len = FChildCustomCommand->PatternLen(Command, Index);
      break;
  }
  return Len;
}

bool TInteractiveCustomCommand::IsPromptPattern(const UnicodeString & Pattern) const
{
  return (Pattern.Length() >= 3) && (Pattern[2] == L'?');
}

void TInteractiveCustomCommand::ParsePromptPattern(
  const UnicodeString Pattern, UnicodeString &Prompt, UnicodeString &Default, bool &Delimit) const
{
  int32_t Pos = Pattern.SubString(3, Pattern.Length() - 2).Pos(L"?");
  if (Pos > 0)
  {
    Default = Pattern.SubString(3 + Pos, Pattern.Length() - 3 - Pos);
    if ((Pos > 1) && (Pattern[3 + Pos - 2] == L'\\'))
    {
      Delimit = false;
      Pos--;
    }
    Prompt = Pattern.SubString(3, Pos - 1);
  }
  else
  {
    Prompt = Pattern.SubString(3, Pattern.Length() - 3);
  }
}

bool TInteractiveCustomCommand::PatternReplacement(int32_t Index, const UnicodeString & Pattern,
  UnicodeString & Replacement, bool & Delimit) const
{
  bool Result;
  if (IsPromptPattern(Pattern))
  {
    UnicodeString PromptStr;
    // The PromptStr and Replacement are actually never used
    // as the only implementation (TWinInteractiveCustomCommand) uses
    // prompts and defaults from PatternHint.
    ParsePromptPattern(Pattern, PromptStr, Replacement, Delimit);

    Prompt(Index, PromptStr, Replacement);

    Result = true;
  }
  else if ((Pattern.Length() >= 3) && (Pattern[2] == L'`'))
  {
    UnicodeString Command = Pattern.SubString(3, Pattern.Length() - 3);
    Command = FChildCustomCommand->Complete(Command, true);
    Execute(Command, Replacement);
    Delimit = false;
    Result = true;
  }
  else
  {
    Result = false;
  }

  return Result;
}


TCustomCommandData::TCustomCommandData() noexcept
{
  Init(nullptr);
}

TCustomCommandData::TCustomCommandData(TTerminal * Terminal) noexcept
{
  // Should use FillSessionDataForCode as in TCustomScpExplorerForm::SessionDataForCode
  Init(Terminal->SessionData, Terminal->UserName, Terminal->Password,
    Terminal->GetSessionInfo().HostKeyFingerprintSHA256);
}

TCustomCommandData::TCustomCommandData(TSessionData * SessionData)
{
  Init(SessionData);
}

TCustomCommandData::TCustomCommandData(
  TSessionData * SessionData, const UnicodeString UserName, const UnicodeString Password) noexcept
{
  Init(SessionData, UserName, Password, UnicodeString());
}

void TCustomCommandData::Init(TSessionData * ASessionData)
{
  FSessionData.reset(new TSessionData(L""));
  if (ASessionData != nullptr)
  {
    FSessionData->Assign(ASessionData);
  }
}

void TCustomCommandData::Init(
  TSessionData * ASessionData, const UnicodeString AUserName,
  const UnicodeString APassword, const UnicodeString AHostKey)
{
  Init(ASessionData);
  FSessionData->UserName = AUserName;
  FSessionData->Password = APassword;
  FSessionData->HostKey = AHostKey;
}

TCustomCommandData &TCustomCommandData::operator=(const TCustomCommandData & Data)
{
  DebugAssert(Data.SessionData != nullptr);
  FSessionData.reset(new TSessionData(L""));
  FSessionData->Assign(Data.SessionData);
  return *this;
}

TSessionData *TCustomCommandData::GetSessionDataPrivate() const
{
  return FSessionData.get();
}


TFileCustomCommand::TFileCustomCommand() noexcept
{
}

TFileCustomCommand::TFileCustomCommand(const TCustomCommandData & AData,
  const UnicodeString & APath) noexcept
{
  FData = AData;
  FPath = APath;
}

TFileCustomCommand::TFileCustomCommand(const TCustomCommandData & Data,
    const UnicodeString & Path, const UnicodeString & FileName,
    const UnicodeString & FileList) noexcept :
  TCustomCommand()
{
  FData = Data;
  FPath = Path;
  FFileName = FileName;
  FFileList = FileList;
}

int32_t TFileCustomCommand::PatternLen(const UnicodeString & Command, int32_t Index) const
{
  int32_t Len;
  wchar_t PatternCmd = (Index < Command.Length()) ? static_cast<wchar_t>(::towlower(Command[Index + 1])) : L'\0';
  switch (PatternCmd)
  {
    case L's':
    case L'e':
    case L'@':
    case L'u':
    case L'p':
    case L'k':
    case L'#':
    case L'/':
    case L'&':
    case L'n':
      Len = 2;
      break;

    default:
      Len = 1;
      break;
  }
  return Len;
}

bool TFileCustomCommand::PatternReplacement(
  int32_t /*Index*/, const UnicodeString & Pattern, UnicodeString & Replacement, bool & Delimit) const
{
  // keep consistent with TSessionLog::OpenLogFile

  TSessionData *SessionData = FData.GetSessionData();

  if (::SameText(Pattern, L"!s"))
  {
    if (SessionData != nullptr)
    {
      Replacement = FData.SessionData->GenerateSessionUrl(sufSession);
    }
  }
  else if (SameText(Pattern, L"!e"))
  {
    if (FData.SessionData != nullptr)
    {
      Replacement = FData.SessionData->GenerateSessionUrl(sufComplete);
    }
  }
  else if (Pattern == L"!@")
  {
    if (SessionData != nullptr)
    {
      Replacement = SessionData->GetHostNameExpanded();
    }
  }
  else if (::SameText(Pattern, L"!u"))
  {
    if (SessionData != nullptr)
    {
      Replacement = SessionData->SessionGetUserName();
    }
  }
  else if (::SameText(Pattern, L"!p"))
  {
    if (SessionData != nullptr)
    {
      Replacement = NormalizeString(FData.SessionData->Password);
    }
  }
  else if (::SameText(Pattern, L"!#"))
  {
    if (FData.SessionData != nullptr)
    {
      Replacement = IntToStr(FData.SessionData->PortNumber);
    }
  }
  else if (SameText(Pattern, L"!k"))
  {
    if (SessionData != nullptr)
    {
      Replacement = FData.SessionData->ResolvePublicKeyFile();
    }
  }
  else if (Pattern == L"!/")
  {
    Replacement = base::UnixIncludeTrailingBackslash(FPath);
  }
  else if (Pattern == L"!&")
  {
    Replacement = FFileList;
    // already delimited
    Delimit = false;
  }
  else if (::SameText(Pattern, L"!n"))
  {
    if (SessionData != nullptr)
    {
      Replacement = SessionData->GetSessionName();
    }
  }
  else
  {
    DebugAssert(Pattern.Length() == 1);
    Replacement = FFileName;
  }

  return true;
}

void TFileCustomCommand::Validate(const UnicodeString & Command)
{
  int32_t Found[2] = {0, 0};
  CustomValidate(Command, &Found);
  if ((Found[0] > 0) && (Found[1] > 0))
  {
    throw Exception(FMTLOAD(CUSTOM_COMMAND_FILELIST_ERROR,
      Found[1], Found[0]));
  }
}

void TFileCustomCommand::ValidatePattern(const UnicodeString & Command,
  int32_t Index, int32_t /*Len*/, wchar_t PatternCmd, void *Arg)
{
  int32_t *Found = static_cast<int32_t *>(Arg);

  DebugAssert(Index > 0);

  if (PatternCmd == L'&')
  {
    Found[0] = Index;
  }
  else if ((PatternCmd != TEXT_TOKEN) && (PatternLen(Command, Index) == 1))
  {
    Found[1] = Index;
  }
}

bool TFileCustomCommand::IsFileListCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'&');
}

bool TFileCustomCommand::IsRemoteFileCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'!') || FindPattern(Command, L'&');
}

bool TFileCustomCommand::IsFileCommand(const UnicodeString & Command) const
{
  return IsRemoteFileCommand(Command);
}

bool TFileCustomCommand::IsSiteCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'@') || FindPattern(Command, L'S') || FindPattern(Command, L'E');
}

bool TFileCustomCommand::IsSessionCommand(const UnicodeString & Command) const
{
  return
    IsSiteCommand(Command) || IsPasswordCommand(Command) ||
    FindPattern(Command, L'U') || FindPattern(Command, L'#') || FindPattern(Command, L'N') ||
    FindPattern(Command, L'/');
}

bool TFileCustomCommand::IsPasswordCommand(const UnicodeString & Command) const
{
  return FindPattern(Command, L'p');
}

