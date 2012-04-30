//---------------------------------------------------------------------------
#include "stdafx.h"
#include "FileMasks.h"

#include "Common.h"
#include "TextsCore.h"
#include "RemoteFiles.h"
#include "PuttyTools.h"
#include "Terminal.h"
//---------------------------------------------------------------------------
const wchar_t IncludeExcludeFileMasksDelimiter = L'|';
static UnicodeString IncludeExcludeFileMasksDelimiterStr = UnicodeString(1, L' ') + IncludeExcludeFileMasksDelimiter + L' ';
static UnicodeString FileMasksDelimiters = L";,";
static UnicodeString AllFileMasksDelimiters = FileMasksDelimiters + IncludeExcludeFileMasksDelimiter;
static UnicodeString DirectoryMaskDelimiters = L"/\\";
static UnicodeString FileMasksDelimiterStr = UnicodeString(1, FileMasksDelimiters[1]) + L' ';
//---------------------------------------------------------------------------

inline int IsSlash(wchar_t x) { return x==L'\\' || x==L'/'; }
// inline wchar_t __cdecl Upper(wchar_t Ch) { CharUpperBuff(&Ch, 1); return Ch; }
// inline wchar_t __cdecl Lower(wchar_t Ch) { CharLowerBuff(&Ch, 1); return Ch; }
//---------------------------------------------------------------------------

const wchar_t *PointToName(const wchar_t *lpwszPath,const wchar_t *lpwszEndPtr)
{
    if (!lpwszPath)
    {
        return NULL;
    }

    if (*lpwszPath && *(lpwszPath+1)==L':') { lpwszPath+=2; }

    const wchar_t *lpwszNamePtr = lpwszEndPtr;

    if (!lpwszNamePtr)
    {
        lpwszNamePtr=lpwszPath;

        while (*lpwszNamePtr) { lpwszNamePtr++; }
    }

    while (lpwszNamePtr != lpwszPath)
    {
        if (IsSlash(*lpwszNamePtr))
        {
            return lpwszNamePtr+1;
        }

        lpwszNamePtr--;
    }

    if (IsSlash(*lpwszPath))
    {
        return lpwszPath+1;
    }
    else
    {
        return lpwszPath;
    }
}

const wchar_t *PointToName(const wchar_t *lpwszPath)
{
    return PointToName(lpwszPath, NULL);
}

const wchar_t *PointToName(UnicodeString &strPath)
{
    const wchar_t *lpwszPath = strPath.c_str();
    const wchar_t *lpwszEndPtr = lpwszPath + strPath.Length();
    return PointToName(lpwszPath, lpwszEndPtr);
}

static int CmpName(const wchar_t *pattern,const wchar_t *str, bool skippath, bool CmpNameSearchMode);

static int CmpName_Body(const wchar_t *pattern,const wchar_t *str, bool CmpNameSearchMode)
{
    wchar_t stringc,patternc,rangec;
    int match;

    for (;; ++str)
    {
        /* $ 01.05.2001 DJ
           используем инлайновые версии
        */
        stringc=Upper(*str);
        patternc=Upper(*pattern++);

        switch (patternc)
        {
        case 0:
            return !stringc;
        case L'?':

            if (!stringc)
            {
                return FALSE;
            }

            break;
        case L'*':

            if (!*pattern)
            {
                return TRUE;
            }

            /* $ 01.05.2001 DJ
               оптимизированная ветка работает и для имен с несколькими
               точками
            */
            if (*pattern==L'.')
            {
                if (pattern[1]==L'*' && !pattern[2])
                {
                    return TRUE;
                }

                if (!wcspbrk(pattern, L"*?["))
                {
                    const wchar_t *dot = wcsrchr(str, L'.');

                    if (!pattern[1])
                    {
                        return (!dot || !dot[1]);
                    }

                    const wchar_t *patdot = wcschr(pattern+1, L'.');

                    if (patdot  && !dot)
                    {
                        return FALSE;
                    }

                    if (!patdot && dot )
                    {
                        return !StringCmpI(pattern+1,dot+1);
                    }
                }
            }

            do
            {
                if(CmpName(pattern,str,false,CmpNameSearchMode))
                {
                    return TRUE;
                }
            }
            while (*str++);

            return FALSE;
        case L'[':

            if (!wcschr(pattern,L']'))
            {
                if (patternc != stringc)
                {
                    return (FALSE);
                }

                break;
            }

            if (*pattern && *(pattern+1)==L']')
            {
                if (*pattern!=*str)
                {
                    return FALSE;
                }

                pattern+=2;
                break;
            }

            match = 0;

            while ((rangec = Upper(*pattern++)) != 0)
            {
                if (rangec == L']')
                {
                    if (match)
                    {
                        break;
                    }
                    else
                    {
                        return FALSE;
                    }
                }

                if (match)
                {
                    continue;
                }

                if (rangec == L'-' && *(pattern - 2) != L'[' && *pattern != L']')
                {
                    match = (stringc <= Upper(*pattern) &&
                             Upper(*(pattern - 2)) <= stringc);
                    pattern++;
                }
                else
                {
                    match = (stringc == rangec);
                }
            }

            if (!rangec)
            {
                return FALSE;
            }

            break;
        default:

            if (patternc != stringc)
            {
                if (patternc==L'.' && !stringc && !CmpNameSearchMode)
                {
                    return(*pattern!=L'.' && CmpName(pattern,str,true,CmpNameSearchMode));
                }
                else
                {
                    return FALSE;
                }
            }

            break;
        }
    }
}

int CmpName(const wchar_t *pattern,const wchar_t *str, bool skippath, bool CmpNameSearchMode)
{
    if (!pattern || !str)
    {
        return FALSE;
    }

    if (skippath)
    {
        str = PointToName(str);
    }

    return CmpName_Body(pattern,str,CmpNameSearchMode);
}

namespace Masks
{

bool TMask::GetMatches(const UnicodeString Str)
{
    // DEBUG_PRINTF(L"GetMatches: FMask = %s, Str = %s", FMask.c_str(), Str.c_str());
    return CmpName(FMask.c_str(), Str.c_str(), true, true) == TRUE;
}

} // namespace Masks

//---------------------------------------------------------------------------
EFileMasksException::EFileMasksException(
    const UnicodeString Message, size_t AErrorStart, size_t AErrorLen) :
    std::exception(System::W2MB(Message.c_str()).c_str())
{
    ErrorStart = AErrorStart;
    ErrorLen = AErrorLen;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall MaskFilePart(const UnicodeString Part, const UnicodeString Mask, bool &Masked)
{
    UnicodeString Result;
    size_t RestStart = 0;
    bool Delim = false;
    for (size_t Index = 0; Index < Mask.Length(); Index++)
    {
        switch (Mask[Index])
        {
            case L'\\':
            if (!Delim)
            {
              Delim = true;
              Masked = true;
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
                if (RestStart < Part.Length())
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
//---------------------------------------------------------------------------
UnicodeString __fastcall MaskFileName(const UnicodeString FileName, const UnicodeString Mask)
{
    UnicodeString fileName = FileName;
    if (!Mask.IsEmpty() && (Mask != L"*") && (Mask != L"*.*"))
    {
        bool Masked = false;
        size_t P = ::LastDelimiter(Mask, L".");
        if (P != UnicodeString::npos)
        {
            size_t P2 = ::LastDelimiter(fileName, L".");
            // only dot at beginning of file name is not considered as
            // name/ext separator
            bool hasFileExt = (P2 != UnicodeString::npos) && (P2 > 0);
            UnicodeString FileExt = hasFileExt ?
                                   fileName.SubString(P2, fileName.Length() - P2) : UnicodeString();
            FileExt = MaskFilePart(FileExt, Mask.SubString(P + 1, Mask.Length() - P), Masked);
            if (hasFileExt)
            {
                fileName.SetLength(P2);
            }
            fileName = MaskFilePart(fileName, Mask.SubString(0, P), Masked);
            if (!FileExt.IsEmpty())
            {
                fileName += L"." + FileExt;
            }
        }
        else
        {
            fileName = MaskFilePart(fileName, Mask, Masked);
        }
    }
    return fileName;
}
//---------------------------------------------------------------------------
bool __fastcall IsFileNameMask(const UnicodeString Mask)
{
    bool Masked = false;
    MaskFilePart(L"", Mask, Masked);
    return Masked;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall DelimitFileNameMask(const UnicodeString Mask)
{
    UnicodeString mask = Mask;
    for (size_t i = 0; i <= mask.Length(); i++)
    {
        if (wcschr(L"\\*?", mask[i]) != NULL)
        {
            mask.insert(i, L"\\");
            i++;
        }
    }
    return mask;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileMasks::TParams::TParams() :
    Size(0)
{
}
//---------------------------------------------------------------------------
UnicodeString TFileMasks::TParams::ToString() const
{
    return UnicodeString(L"[") + Int64ToStr(Size) + L"/" + ::DateTimeToString(Modification) + L"]";
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::IsMask(const UnicodeString Mask)
{
    size_t result = ::LastDelimiter(Mask, L"?*[");
    return result != UnicodeString::npos;
}
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::IsAnyMask(const UnicodeString Mask)
{
    return Mask.IsEmpty() || (Mask == L"*.*") || (Mask == L"*");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFileMasks::NormalizeMask(const UnicodeString Mask, const UnicodeString AnyMask)
{
    if (IsAnyMask(Mask))
    {
        return AnyMask;
    }
    else
    {
        return Mask;
    }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFileMasks::ComposeMaskStr(
  System::TStrings * MasksStr, bool Directory)
{
  UnicodeString Result;
  for (size_t I = 0; I < MasksStr->GetCount(); I++)
  {
    UnicodeString Str = ::Trim(MasksStr->GetString(I));
    if (!Str.IsEmpty())
    {
      for (size_t P = 0; P < Str.Length(); P++)
      {
        if (::IsDelimiter(Str, AllFileMasksDelimiters, P))
        {
          Str.insert(P, UnicodeString(1, Str[P]));
          P++;
        }
      }

      if (Directory)
      {
        Str = MakeDirectoryMask(Str);
      }
      else
      {
        while (::IsDelimiter(Str, DirectoryMaskDelimiters, Str.Length()))
        {
          Str.SetLength(Str.Length() - 1);
        }
      }

      AddToList(Result, Str, FileMasksDelimiterStr);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFileMasks::ComposeMaskStr(
  System::TStrings * IncludeFileMasksStr, System::TStrings * ExcludeFileMasksStr,
  System::TStrings * IncludeDirectoryMasksStr, System::TStrings * ExcludeDirectoryMasksStr)
{
  UnicodeString IncludeMasks = ComposeMaskStr(IncludeFileMasksStr, false);
  AddToList(IncludeMasks, ComposeMaskStr(IncludeDirectoryMasksStr, true), FileMasksDelimiterStr);
  UnicodeString ExcludeMasks = ComposeMaskStr(ExcludeFileMasksStr, false);
  AddToList(ExcludeMasks, ComposeMaskStr(ExcludeDirectoryMasksStr, true), FileMasksDelimiterStr);

  UnicodeString Result = IncludeMasks;
  AddToList(Result, ExcludeMasks, IncludeExcludeFileMasksDelimiterStr);
  return Result;
}
//---------------------------------------------------------------------------
TFileMasks::TFileMasks()
{
  Init();
}
//---------------------------------------------------------------------------
TFileMasks::TFileMasks(int ForceDirectoryMasks)
{
  Init();
  FForceDirectoryMasks = ForceDirectoryMasks;
}
//---------------------------------------------------------------------------
TFileMasks::TFileMasks(const TFileMasks &Source)
{
    Init();
    SetStr(Source.GetMasks(), false);
}
//---------------------------------------------------------------------------
TFileMasks::TFileMasks(const UnicodeString AMasks)
{
    Init();
    SetStr(AMasks, false);
}
//---------------------------------------------------------------------------
TFileMasks::~TFileMasks()
{
    Clear();
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::Init()
{
  FForceDirectoryMasks = -1;

  DoInit(false);
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::DoInit(bool Delete)
{
  for (int Index = 0; Index < 4; Index++)
  {
    if (Delete)
    {
      delete FMasksStr[Index];
    }
    FMasksStr[Index] = NULL;
  }
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::Clear()
{
  DoInit(true);

  for (int Index = 0; Index < 4; Index++)
  {
    Clear(FMasks[Index]);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::Clear(TMasks &Masks)
{
    TMasks::iterator I = Masks.begin();
    while (I != Masks.end())
    {
        ReleaseMaskMask((*I).FileNameMask);
        ReleaseMaskMask((*I).DirectoryMask);
        ++I;
    }
    Masks.clear();
}
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::MatchesMasks(const UnicodeString FileName, bool Directory,
                              const UnicodeString Path, const TParams *Params, const TMasks &Masks, bool Recurse)
{
    bool Result = false;

    TMasks::const_iterator I = Masks.begin();
    while (!Result && (I != Masks.end()))
    {
        const TMask &Mask = *I;
        Result =
            MatchesMaskMask(Mask.DirectoryMask, Path) &&
            MatchesMaskMask(Mask.FileNameMask, FileName);

        if (Result)
        {
            bool HasSize = (Params != NULL);

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
                    Result = HasSize && (Params->Size >= Mask.LowSize);
                    break;
                }
            }

      bool HasModification = (Params != NULL);

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

    I++;
  }

  if (!Result && Directory && !IsUnixRootPath(Path) && Recurse)
  {
    UnicodeString ParentFileName = UnixExtractFileName(Path);
    UnicodeString ParentPath = UnixExcludeTrailingBackslash(UnixExtractFilePath(Path));
    // Pass Params down or not?
    // Currently it includes Size/Time only, what is not used for directories.
    // So it depends of future use. Possibly we should make a copy
    // and pass on only relevant fields.
    Result = MatchesMasks(ParentFileName, true, ParentPath, Params, Masks, Recurse);
    }

    return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::Matches(const UnicodeString FileName, bool Directory,
                         const UnicodeString Path, const TParams *Params) const
{
    bool Result =
    (FMasks[MASK_INDEX(Directory, true)].IsEmpty() || MatchesMasks(FileName, Directory, Path, Params, FMasks[MASK_INDEX(Directory, true)], true)) &&
    !MatchesMasks(FileName, Directory, Path, Params, FMasks[MASK_INDEX(Directory, false)], false);
    return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::Matches(const UnicodeString FileName, bool Local,
                         bool Directory, const TParams *Params) const
{
    bool Result;
    if (Local)
    {
        UnicodeString Path = ExtractFilePath(FileName);
        if (!Path.IsEmpty())
        {
            Path = ToUnixPath(ExcludeTrailingBackslash(Path));
        }
        Result = Matches(ExtractFileName(FileName, false), Directory, Path, Params);
    }
    else
    {
        Result = Matches(UnixExtractFileName(FileName), Directory,
                         UnixExcludeTrailingBackslash(UnixExtractFilePath(FileName)), Params);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TFileMasks::GetIsValid(size_t &Start, size_t &Length) const
{
    if (IsMask(FStr) || FStr.IsEmpty())
    {
        Start = 0;
        Length = FStr.Length();
        return true;
    }
    else
    {
        Start = 0;
        Length = 0;
        return false;
    }
}

//---------------------------------------------------------------------------
bool __fastcall TFileMasks::operator ==(const TFileMasks &rhm) const
{
    return (GetMasks() == rhm.GetMasks());
}
//---------------------------------------------------------------------------
TFileMasks & __fastcall TFileMasks::operator =(const UnicodeString rhs)
{
    SetMasks(rhs);
    return *this;
}
//---------------------------------------------------------------------------
TFileMasks & __fastcall TFileMasks::operator =(const TFileMasks &rhm)
{
    SetMasks(rhm.GetMasks());
    return *this;
}
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::operator ==(const UnicodeString rhs) const
{
    return (GetMasks() == rhs);
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::ThrowError(size_t Start, size_t End)
{
    throw EFileMasksException(
        FMTLOAD(MASK_ERROR, GetMasks().SubString(Start, End - Start + 1).c_str()),
        Start, End - Start + 1);
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::CreateMaskMask(const UnicodeString Mask, size_t Start, size_t End,
                                bool Ex, TMaskMask &MaskMask)
{
    try
    {
        assert(MaskMask.Mask == NULL);
        if (Ex && IsAnyMask(Mask))
        {
            MaskMask.Kind = TMaskMask::Any;
            MaskMask.Mask = NULL;
        }
        else
        {
            MaskMask.Kind = (Ex && (Mask == L"*.")) ? TMaskMask::NoExt : TMaskMask::Regular;
            MaskMask.Mask = new Masks::TMask(Mask);
        }
    }
    catch(...)
    {
        ThrowError(Start, End);
    }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFileMasks::MakeDirectoryMask(UnicodeString Str)
{
  assert(!Str.IsEmpty());
  if (Str.IsEmpty() || !(::IsDelimiter(Str, DirectoryMaskDelimiters, Str.Length())))
  {
    size_t D = ::LastDelimiter(Str, DirectoryMaskDelimiters);
    // if there's any [back]slash anywhere in str,
    // add the same [back]slash at the end, otherwise add slash
    wchar_t Delimiter = (D != NPOS) ? Str[D] : DirectoryMaskDelimiters[1];
    Str += Delimiter;
  }
  return Str;
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::CreateMask(
  const UnicodeString & MaskStr, int MaskStart, int /*MaskEnd*/, bool Include)
{
  bool Directory = false; // shut up
  TMask Mask;

  Mask.MaskStr = MaskStr;
  Mask.UserStr = MaskStr;
  Mask.FileNameMask.Kind = TMaskMask::Any;
  Mask.FileNameMask.Mask = NULL;
  Mask.DirectoryMask.Kind = TMaskMask::Any;
  Mask.DirectoryMask.Mask = NULL;
  Mask.HighSizeMask = TMask::None;
  Mask.LowSizeMask = TMask::None;
  Mask.HighModificationMask = TMask::None;
  Mask.LowModificationMask = TMask::None;

  wchar_t NextPartDelimiter = L'\0';
  size_t NextPartFrom = 0;
  while (NextPartFrom < MaskStr.Length())
  {
    wchar_t PartDelimiter = NextPartDelimiter;
    size_t PartFrom = NextPartFrom;
    UnicodeString PartStr = CopyToChars(MaskStr, NextPartFrom, L"<>", false, &NextPartDelimiter, true);

    size_t PartStart = MaskStart + PartFrom - 1;
    size_t PartEnd = MaskStart + NextPartFrom - 1 - 2;

    TrimEx(PartStr, PartStart, PartEnd);

    if (PartDelimiter != L'\0')
    {
      bool Low = (PartDelimiter == L'>');

      TMask::TMaskBoundary Boundary;
      if ((PartStr.Length() >= 1) && (PartStr[0] == L'='))
      {
        Boundary = TMask::Close;
        PartStr.Delete(0, 1);
      }
      else
      {
        Boundary = TMask::Open;
      }

      System::TFormatSettings FormatSettings;
      GetLocaleFormatSettings(System::GetDefaultLCID(), FormatSettings);
      FormatSettings.DateSeparator = L'-';
      FormatSettings.TimeSeparator = L':';
      FormatSettings.ShortDateFormat = L"yyyy/mm/dd";
      FormatSettings.ShortTimeFormat = L"hh:nn:ss";

      System::TDateTime Modification;
      if (TryStrToDateTime(PartStr, Modification, FormatSettings) ||
          TryRelativeStrToDateTime(PartStr, Modification))
      {
        TMask::TMaskBoundary & ModificationMask =
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
        TMask::TMaskBoundary & SizeMask = (Low ? Mask.LowSizeMask : Mask.HighSizeMask);
        __int64 & Size = (Low ? Mask.LowSize : Mask.HighSize);

        if ((SizeMask != TMask::None) || Directory)
        {
          // include delimiter into size part
          ThrowError(PartStart - 1, PartEnd);
        }

        SizeMask = Boundary;
        Size = ParseSize(PartStr);
      }
    }
    else if (!PartStr.IsEmpty())
    {
      int D = ::LastDelimiter(PartStr, DirectoryMaskDelimiters);

      Directory = (D > 0) && (D == PartStr.Length());

      if (Directory)
      {
        do
        {
          PartStr.SetLength(PartStr.Length() - 1);
          Mask.UserStr.Delete(PartStart - MaskStart + D, 1);
          D--;
        }
        while (::IsDelimiter(PartStr, DirectoryMaskDelimiters, PartStr.Length()));

        D = ::LastDelimiter(PartStr, DirectoryMaskDelimiters);

        if (FForceDirectoryMasks == 0)
        {
          Directory = false;
          Mask.MaskStr = Mask.UserStr;
        }
      }
      else if (FForceDirectoryMasks > 0)
      {
        Directory = true;
        Mask.MaskStr.insert(PartStart - MaskStart + PartStr.Length(), UnicodeString(1, DirectoryMaskDelimiters[1]));
      }

      if (D != NPOS)
      {
        // make sure sole "/" (root dir) is preserved as is
        CreateMaskMask(
          UnixExcludeTrailingBackslash(ToUnixPath(PartStr.SubString(1, D))),
          PartStart, PartStart + D - 1, false,
          Mask.DirectoryMask);
        CreateMaskMask(
          PartStr.SubString(D + 1, PartStr.Length() - D),
          PartStart + D, PartEnd, true,
          Mask.FileNameMask);
      }
      else
      {
        CreateMaskMask(PartStr, PartStart, PartEnd, true, Mask.FileNameMask);
      }
    }
  }

  FMasks[MASK_INDEX(Directory, Include)].push_back(Mask);
}
//---------------------------------------------------------------------------
System::TStrings * __fastcall TFileMasks::GetMasksStr(int Index) const
{
  if (FMasksStr[Index] == NULL)
  {
    FMasksStr[Index] = new System::TStringList();
    TMasks::const_iterator I = FMasks[Index].begin();
    while (I != FMasks[Index].end())
    {
      FMasksStr[Index]->Add((*I).UserStr);
      I++;
    }
  }

  return FMasksStr[Index];
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::ReleaseMaskMask(TMaskMask & MaskMask)
{
  delete MaskMask.Mask;
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::TrimEx(UnicodeString &Str, size_t &Start, size_t &End)
{
    UnicodeString Buf = TrimLeft(Str);
    Start += Str.Length() - Buf.Length();
    Str = ::TrimRight(Buf);
    End -= Buf.Length() - Str.Length();
}
//---------------------------------------------------------------------------
bool __fastcall TFileMasks::MatchesMaskMask(const TMaskMask &MaskMask, const UnicodeString Str)
{
    bool Result;
    if (MaskMask.Kind == TMaskMask::Any)
    {
        Result = true;
    }
    else if ((MaskMask.Kind == TMaskMask::NoExt) && (Str.find_first_of(L".") == NPOS))
    {
        Result = true;
    }
    else
    {
        Result = MaskMask.Mask->GetMatches(Str);
    }
    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::SetMasks(const UnicodeString value)
{
    if (FStr != value)
    {
        SetStr(value, false);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::SetMask(const UnicodeString Mask)
{
    SetStr(Mask, true);
}
//---------------------------------------------------------------------------
void __fastcall TFileMasks::SetStr(const UnicodeString Str, bool SingleMask)
{
    UnicodeString Backup = FStr;
    try
    {
        FStr = Str;
        Clear();

        size_t NextMaskFrom = 0;
        bool Include = true;
        while (NextMaskFrom < Str.Length())
        {
            size_t MaskStart = NextMaskFrom;
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
                MaskStr = ::CopyToChars(Str, NextMaskFrom, AllFileMasksDelimiters, false, &NextMaskDelimiter, true);
            }
            size_t MaskEnd = NextMaskFrom - 2;

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
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define TEXT_TOKEN L'\255'
//---------------------------------------------------------------------------
const wchar_t TCustomCommand::NoQuote = L'\0';
const UnicodeString TCustomCommand::Quotes = L"\"'";
//---------------------------------------------------------------------------
TCustomCommand::TCustomCommand()
{
}
//---------------------------------------------------------------------------
void __fastcall TCustomCommand::GetToken(
    const UnicodeString Command, size_t Index, size_t &Len, wchar_t &PatternCmd)
{
    assert(Index < Command.Length());
    const wchar_t *Ptr = Command.c_str() + Index;

    if (Ptr[0] == L'!')
    {
        PatternCmd = Ptr[1];
        if (PatternCmd == L'!')
        {
            Len = 2;
        }
        else
        {
            Len = PatternLen(Index, PatternCmd);
        }

        if (Len == NPOS)
        {
            throw ExtException(FMTLOAD(CUSTOM_COMMAND_UNKNOWN, PatternCmd, Index));
        }
        else if (Len > 0)
        {
            if ((Command.Length() - Index + 1) < Len)
            {
                throw ExtException(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, PatternCmd, Index));
            }
        }
        else if (Len == 0)
        {
            const wchar_t *PatternEnd = wcschr(Ptr + 1, L'!');
            if (PatternEnd == NULL)
            {
                throw ExtException(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, PatternCmd, Index));
            }
            Len = PatternEnd - Ptr + 1;
        }
    }
    else
    {
        PatternCmd = TEXT_TOKEN;
        const wchar_t *NextPattern = wcschr(Ptr, L'!');
        if (NextPattern == NULL)
        {
            Len = Command.Length() - Index + 1;
        }
        else
        {
            Len = NextPattern - Ptr;
        }
    }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomCommand::Complete(const UnicodeString Command,
                                      bool LastPass)
{
    UnicodeString Result;
    size_t Index = 0;

    while (Index < Command.Length())
    {
        size_t Len = 0;
        wchar_t PatternCmd = L'\0';
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
            if ((Index > 0) && (Index + Len - 1 < Command.Length()) &&
                    ::IsDelimiter(Command, Quotes, Index - 1) &&
                    ::IsDelimiter(Command, Quotes, Index + Len) &&
                    (Command[Index - 1] == Command[Index + Len]))
            {
                Quote = static_cast<char>(Command[Index - 1]);
            }
            UnicodeString Pattern = Command.SubString(Index, Len);
            UnicodeString Replacement;
            bool Delimit = true;
            if (PatternReplacement(Index, Pattern, Replacement, Delimit))
            {
                if (!LastPass)
                {
                    Replacement = ::StringReplace(Replacement, L"!", L"!!", TReplaceFlags::Init(rfReplaceAll));
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
        }

        Index += Len;
    }

    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomCommand::DelimitReplacement(UnicodeString &Replacement, wchar_t Quote)
{
    Replacement = ShellDelimitStr(Replacement, Quote);
}
//---------------------------------------------------------------------------
void __fastcall TCustomCommand::Validate(const UnicodeString Command)
{
    CustomValidate(Command, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TCustomCommand::CustomValidate(const UnicodeString Command,
                                    void *Arg)
{
    size_t Index = 0;

    while (Index < Command.Length())
    {
        size_t Len;
        wchar_t PatternCmd;
        GetToken(Command, Index, Len, PatternCmd);
        ValidatePattern(Command, Index, Len, PatternCmd, Arg);

        Index += Len;
    }
}
//---------------------------------------------------------------------------
bool __fastcall TCustomCommand::FindPattern(const UnicodeString Command,
                                 wchar_t PatternCmd)
{
    bool Result = false;
    size_t Index = 0;

    while (!Result && (Index < Command.Length()))
    {
        size_t Len;
        wchar_t APatternCmd;
        GetToken(Command, Index, Len, APatternCmd);
        if (((PatternCmd != L'!') && (PatternCmd == APatternCmd)) ||
                ((PatternCmd == L'!') && (Len == 1) && (APatternCmd != TEXT_TOKEN)))
        {
            Result = true;
        }

        Index += Len;
    }

    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomCommand::ValidatePattern(const UnicodeString /*Command*/,
                                     size_t /*Index*/, size_t /*Len*/, wchar_t /*PatternCmd*/, void * /*Arg*/)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TInteractiveCustomCommand::TInteractiveCustomCommand(
    TCustomCommand *ChildCustomCommand)
{
    FChildCustomCommand = ChildCustomCommand;
}
//---------------------------------------------------------------------------
void __fastcall TInteractiveCustomCommand::Prompt(size_t /*Index*/,
                                       const UnicodeString /*Prompt*/, UnicodeString &Value)
{
    Value = L"";
}
//---------------------------------------------------------------------------
size_t __fastcall TInteractiveCustomCommand::PatternLen(size_t Index, wchar_t PatternCmd)
{
    size_t Len = 0;
    switch (PatternCmd)
    {
    case '?':
        Len = 0;
        break;

    default:
        Len = FChildCustomCommand->PatternLen(Index, PatternCmd);
        break;
    }
    return Len;
}
//---------------------------------------------------------------------------
bool __fastcall TInteractiveCustomCommand::PatternReplacement(size_t Index, const UnicodeString Pattern,
        UnicodeString &Replacement, bool &Delimit)
{
    bool Result;
    if ((Pattern.Length() >= 3) && (Pattern[1] == L'?'))
    {
        UnicodeString PromptStr;
        size_t Pos = Pattern.SubString(2, Pattern.Length() - 2).find_first_of(L"?");
        if (Pos != UnicodeString::npos)
        {
            Replacement = Pattern.SubString(2 + Pos, Pattern.Length() - 3 - Pos);
            if ((Pos > 0) && (Pattern[2 + Pos - 2] == L'\\'))
            {
                Delimit = false;
                Pos--;
            }
            PromptStr = Pattern.SubString(2, Pos - 1);
        }
        else
        {
            PromptStr = Pattern.SubString(2, Pattern.Length() - 3);
        }

        Prompt(Index, PromptStr, Replacement);

        Result = true;
    }
    else
    {
        Result = false;
    }

    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TCustomCommandData::TCustomCommandData()
{
}
//---------------------------------------------------------------------------
TCustomCommandData::TCustomCommandData(TTerminal *Terminal)
{
    HostName = Terminal->GetSessionData()->GetHostNameExpanded();
    UserName = Terminal->GetSessionData()->GetUserNameExpanded();
    Password = Terminal->GetPassword();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileCustomCommand::TFileCustomCommand()
{
}
//---------------------------------------------------------------------------
TFileCustomCommand::TFileCustomCommand(const TCustomCommandData &Data,
                                       const UnicodeString Path)
{
    FData = Data;
    FPath = Path;
}
//---------------------------------------------------------------------------
TFileCustomCommand::TFileCustomCommand(const TCustomCommandData &Data,
                                       const UnicodeString Path, const UnicodeString FileName,
                                       const UnicodeString FileList) :
    TCustomCommand()
{
    FData = Data;
    FPath = Path;
    FFileName = FileName;
    FFileList = FileList;
}
//---------------------------------------------------------------------------
size_t __fastcall TFileCustomCommand::PatternLen(size_t /*Index*/, wchar_t PatternCmd)
{
    size_t Len = 0;
    switch (toupper(PatternCmd))
    {
    case L'@':
    case L'U':
    case L'P':
    case L'/':
    case L'&':
        Len = 2;
        break;

    default:
        Len = 1;
        break;
    }
    return Len;
}
//---------------------------------------------------------------------------
bool __fastcall TFileCustomCommand::PatternReplacement(size_t /*Index*/,
        const UnicodeString Pattern, UnicodeString &Replacement, bool &Delimit)
{
    // keep consistent with TSessionLog::OpenLogFile

    if (Pattern == L"!@")
    {
        Replacement = FData.HostName;
    }
    else if (AnsiSameText(Pattern, L"!u"))
    {
        Replacement = FData.UserName;
    }
    else if (AnsiSameText(Pattern, L"!p"))
    {
        Replacement = FData.Password;
    }
    else if (Pattern == L"!/")
    {
        Replacement = UnixIncludeTrailingBackslash(FPath);
    }
    else if (Pattern == L"!&")
    {
        Replacement = FFileList;
        // already delimited
        Delimit = false;
    }
    else
    {
        assert(Pattern.Length() == 1);
        Replacement = FFileName;
    }

    return true;
}
//---------------------------------------------------------------------------
void __fastcall TFileCustomCommand::Validate(const UnicodeString Command)
{
    int Found[2] = { 0, 0 };
    CustomValidate(Command, &Found);
    if ((Found[0] > 0) && (Found[1] > 0))
    {
        throw ExtException(FMTLOAD(CUSTOM_COMMAND_FILELIST_ERROR,
                                   Found[1], Found[0]));
    }
}
//---------------------------------------------------------------------------
void __fastcall TFileCustomCommand::ValidatePattern(const UnicodeString /*Command*/,
        size_t Index, size_t /*Len*/, wchar_t PatternCmd, void *Arg)
{
    int *Found = static_cast<int *>(Arg);

    assert(Index > 0);

    if (PatternCmd == L'&')
    {
        Found[0] = Index;
    }
    else if ((PatternCmd != TEXT_TOKEN) && (PatternLen(Index, PatternCmd) == 1))
    {
        Found[1] = static_cast<int>(Index);
    }
}
//---------------------------------------------------------------------------
bool __fastcall TFileCustomCommand::IsFileListCommand(const UnicodeString Command)
{
    return FindPattern(Command, L'&');
}
//---------------------------------------------------------------------------
bool __fastcall TFileCustomCommand::IsFileCommand(const UnicodeString Command)
{
    return FindPattern(Command, L'!') || FindPattern(Command, L'&');
}
//---------------------------------------------------------------------------
