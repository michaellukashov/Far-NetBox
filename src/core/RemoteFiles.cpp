
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <nbutils.h>
#include <Sysutils.hpp>
#include <StrUtils.hpp>
#include <DateUtils.hpp>

#include "RemoteFiles.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "Cryptography.h"
// moved to Common.cpp
#if 0
/* TODO 1 : Path class instead of UnicodeString (handle relativity...) */

const UnicodeString PartialExt(L".filepart");

bool IsUnixStyleWindowsPath(const UnicodeString & Path)
{
  return (Path.Length() >= 3) && IsLetter(Path[1]) && (Path[2] == L':') && (Path[3] == L'/');
}

bool UnixIsAbsolutePath(const UnicodeString & Path)
{
  return
    ((Path.Length() >= 1) && (Path[1] == L'/')) ||
    // we need this for FTP only, but this is unfortunately used in a static context
    IsUnixStyleWindowsPath(Path);
}

UnicodeString UnixIncludeTrailingBackslash(const UnicodeString & Path)
{
  // it used to return "/" when input path was empty
  if (!Path.IsEmpty() && !Path.IsDelimiter(L"/", Path.Length()))
  {
    return Path + L"/";
  }
  else
  {
    return Path;
  }
}

// Keeps "/" for root path
UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & Path, bool Simple)
{
  if (Path.IsEmpty() ||
      (Path == L"/") ||
      !Path.IsDelimiter(L"/", Path.Length()) ||
      (!Simple && ((Path.Length() == 3) && IsUnixStyleWindowsPath(Path))))
  {
    return Path;
  }
  else
  {
    return Path.SubString(1, Path.Length() - 1);
  }
}

UnicodeString SimpleUnixExcludeTrailingBackslash(const UnicodeString & Path)
{
  return UnixExcludeTrailingBackslash(Path, true);
}

UnicodeString UnixCombinePaths(const UnicodeString & Path1, const UnicodeString & Path2)
{
  return UnixIncludeTrailingBackslash(Path1) + Path2;
}

Boolean UnixSamePath(const UnicodeString & Path1, const UnicodeString & Path2)
{
  return (UnixIncludeTrailingBackslash(Path1) == UnixIncludeTrailingBackslash(Path2));
}

bool UnixIsChildPath(const UnicodeString & AParent, const UnicodeString & AChild)
{
  UnicodeString Parent = UnixIncludeTrailingBackslash(AParent);
  UnicodeString Child = UnixIncludeTrailingBackslash(AChild);
  return (Child.SubString(1, Parent.Length()) == Parent);
}

UnicodeString UnixExtractFileDir(const UnicodeString & Path)
{
  int Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return Path.SubString(1, Pos - 1);
  }
  else
  {
    return (Pos == 1) ? UnicodeString(ROOTDIRECTORY) : UnicodeString();
  }
}

// must return trailing backslash
UnicodeString UnixExtractFilePath(const UnicodeString & Path)
{
  int Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 0)
  {
    return Path.SubString(1, Pos);
  }
  else
  {
    return UnicodeString();
  }
}

UnicodeString UnixExtractFileName(const UnicodeString & Path)
{
  int Pos = Path.LastDelimiter(L'/');
  UnicodeString Result;
  if (Pos > 0)
  {
    Result = Path.SubString(Pos + 1, Path.Length() - Pos);
  }
  else
  {
    Result = Path;
  }
  return Result;
}

UnicodeString UnixExtractFileExt(const UnicodeString & Path)
{
  UnicodeString FileName = UnixExtractFileName(Path);
  int Pos = FileName.LastDelimiter(L".");
  return (Pos > 0) ? Path.SubString(Pos, Path.Length() - Pos + 1) : UnicodeString();
}

UnicodeString ExtractFileName(const UnicodeString & Path, bool Unix)
{
  if (Unix)
  {
    return UnixExtractFileName(Path);
  }
  else
  {
    return ExtractFileName(Path);
  }
}

UnicodeString ExtractShortName(const UnicodeString & Path, bool Unix)
{
  UnicodeString Result = ExtractFileName(Path, Unix);
  if (Result.IsEmpty())
  {
    Result = Path;
  }
  return Result;
}

bool ExtractCommonPath(TStrings * Files, UnicodeString & Path)
{
  DebugAssert(Files->Count > 0);

  Path = ExtractFilePath(Files->Strings[0]);
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (int Index = 1; Index < Files->Count; Index++)
    {
      while (!Path.IsEmpty() &&
        (Files->Strings[Index].SubString(1, Path.Length()) != Path))
      {
        int PrevLen = Path.Length();
        Path = ExtractFilePath(ExcludeTrailingBackslash(Path));
        if (Path.Length() == PrevLen)
        {
          Path = L"";
          Result = false;
        }
      }
    }
  }

  return Result;
}

static UnicodeString GetFileListItemPath(TStrings * Files, int Index)
{
  UnicodeString Result;
  if (Files->Objects[Index] != nullptr)
  {
    Result = DebugNotNull(dynamic_cast<TRemoteFile *>(Files->Objects[Index]))->FullFileName;
  }
  else
  {
    Result = Files->Strings[Index];
  }
  return Result;
}

bool UnixExtractCommonPath(TStrings * Files, UnicodeString & Path)
{
  DebugAssert(Files->Count > 0);

  Path = UnixExtractFilePath(GetFileListItemPath(Files, 0));
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (int Index = 1; Index < Files->Count; Index++)
    {
      while (!Path.IsEmpty() &&
        (GetFileListItemPath(Files, Index).SubString(1, Path.Length()) != Path))
      {
        int PrevLen = Path.Length();
        Path = UnixExtractFilePath(UnixExcludeTrailingBackslash(Path));
        if (Path.Length() == PrevLen)
        {
          Path = L"";
          Result = false;
        }
      }
    }
  }

  return Result;
}

bool IsUnixRootPath(const UnicodeString & Path)
{
  return Path.IsEmpty() || (Path == ROOTDIRECTORY);
}

bool IsUnixHiddenFile(const UnicodeString & FileName)
{
  return IsRealFile(FileName) && !FileName.IsEmpty() && (FileName[1] == L'.');
}

UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & Path)
{
  // There's a duplicate implementation in TTerminal::ExpandFileName()
  UnicodeString Result;
  if (Path.IsEmpty())
  {
    Result = Base;
  }
  else if (Path[1] == L'/')
  {
    Result = UnixExcludeTrailingBackslash(Path);
  }
  else
  {
    Result = UnixIncludeTrailingBackslash(
      UnixIncludeTrailingBackslash(Base) + Path);
    int P;
    while ((P = Result.Pos(L"/../")) > 0)
    {
      // special case, "/../" => "/"
      if (P == 1)
      {
        Result = L"/";
      }
      else
      {
        int P2 = Result.SubString(1, P-1).LastDelimiter(L"/");
        DebugAssert(P2 > 0);
        Result.Delete(P2, P - P2 + 3);
      }
    }
    while ((P = Result.Pos(L"/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}

UnicodeString FromUnixPath(const UnicodeString & Path)
{
  return ReplaceStr(Path, L"/", L"\\");
}

UnicodeString ToUnixPath(const UnicodeString & Path)
{
  return ReplaceStr(Path, L"\\", L"/");
}

static void CutFirstDirectory(UnicodeString & S, bool Unix)
{
  bool Root;
  int P;
  UnicodeString Sep = Unix ? L"/" : L"\\";
  if (S == Sep)
  {
    S = L"";
  }
  else
  {
    if (S[1] == Sep[1])
    {
      Root = true;
      S.Delete(1, 1);
    }
    else
    {
      Root = false;
    }
    if (S[1] == L'.')
    {
      S.Delete(1, 4);
    }
    P = S.Pos(Sep[1]);
    if (P)
    {
      S.Delete(1, P);
      S = Ellipsis + Sep + S;
    }
    else
    {
      S = L"";
    }
    if (Root)
    {
      S = Sep + S;
    }
  }
}

UnicodeString MinimizeName(const UnicodeString & FileName, int MaxLen, bool Unix)
{
  UnicodeString Drive, Dir, Name, Result;
  UnicodeString Sep = Unix ? L"/" : L"\\";

  Result = FileName;
  if (Unix)
  {
    int P = Result.LastDelimiter(L"/");
    if (P)
    {
      Dir = Result.SubString(1, P);
      Name = Result.SubString(P + 1, Result.Length() - P);
    }
    else
    {
      Dir = L"";
      Name = Result;
    }
  }
  else
  {
    Dir = ExtractFilePath(Result);
    Name = ExtractFileName(Result);

    if (Dir.Length() >= 2 && Dir[2] == L':')
    {
      Drive = Dir.SubString(1, 2);
      Dir.Delete(1, 2);
    }
  }

  while ((!Dir.IsEmpty() || !Drive.IsEmpty()) && (Result.Length() > MaxLen))
  {
    if (Dir == Sep + Ellipsis + Sep)
    {
      Dir = Ellipsis + Sep;
    }
    else if (Dir == L"")
    {
      Drive = L"";
    }
    else
    {
      CutFirstDirectory(Dir, Unix);
    }
    Result = Drive + Dir + Name;
  }

  if (Result.Length() > MaxLen)
  {
    Result = Result.SubString(1, MaxLen);
  }
  return Result;
}

UnicodeString MakeFileList(TStrings * FileList)
{
  UnicodeString Result;
  for (int Index = 0; Index < FileList->Count; Index++)
  {
    UnicodeString FileName = FileList->Strings[Index];
    // currently this is used for local file only, so no delimiting is done
    AddToList(Result, AddQuotes(FileName), L" ");
  }
  return Result;
}

// copy from BaseUtils.pas
TDateTime ReduceDateTimePrecision(TDateTime DateTime,
  TModificationFmt Precision)
{
  if (Precision == mfNone)
  {
    DateTime = double(0);
  }
  else if (Precision != mfFull)
  {
    unsigned short Y, M, D, H, N, S, MS;

    DecodeDate(DateTime, Y, M, D);
    DecodeTime(DateTime, H, N, S, MS);
    switch (Precision)
    {
      case mfYMDHM:
      case mfMDHM:
        S = 0;
        MS = 0;
        break;

      case mfMDY:
        H = 0;
        N = 0;
        S = 0;
        MS = 0;
        break;

      default:
        DebugFail();
    }

    DateTime = EncodeDateVerbose(Y, M, D) + EncodeTimeVerbose(H, N, S, MS);
  }
  return DateTime;
}

TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2)
{
  return (Precision1 < Precision2) ? Precision1 : Precision2;
}

UnicodeString UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision)
{
  switch (Precision)
  {
    case mfNone:
      return L"";
    case mfMDY:
      return FormatDateTime(L"ddddd", DateTime);
    case mfYMDHM:
    case mfMDHM:
      return FormatDateTime(L"ddddd t", DateTime);
    case mfFull:
    default:
      // Keep consistent with TDirView.GetDisplayInfo
      return FormatDateTime(L"ddddd tt", DateTime);
  }
}

UnicodeString ModificationStr(TDateTime DateTime,
  TModificationFmt Precision)
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DateTime.DecodeDate(&Year, &Month, &Day);
  DateTime.DecodeTime(&Hour, &Min, &Sec, &MSec);
  switch (Precision)
  {
    case mfNone:
      return L"";

    case mfMDY:
      return FORMAT(L"%3s %2d %2d", EngShortMonthNames[Month-1], Day, Year);

    case mfMDHM:
      return FORMAT(L"%3s %2d %2d:%2.2d",
        EngShortMonthNames[Month-1], Day, Hour, Min);

    case mfYMDHM:
      return FORMAT(L"%3s %2d %2d:%2.2d %4d",
        EngShortMonthNames[Month-1], Day, Hour, Min, Year);

    default:
      DebugFail();
      // fall thru

    case mfFull:
      return FORMAT(L"%3s %2d %2d:%2.2d:%2.2d %4d",
        EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year);
  }
}

int GetPartialFileExtLen(const UnicodeString & FileName)
{
  int Result = 0;
  if (EndsText(PartialExt, FileName))
  {
    Result = PartialExt.Length();
  }
  else
  {
    int P = FileName.LastDelimiter(L".");
    if ((P > 0) && (P < FileName.Length()))
    {
      if (IsNumber(MidStr(FileName, P + 1)) &&
          EndsText(PartialExt, FileName.SubString(1, P - 1)))
      {
        Result = PartialExt.Length() + (FileName.Length() - P + 1);
      }
    }
  }
  return Result;
}

int FakeFileImageIndex(UnicodeString FileName, unsigned long Attrs,
  UnicodeString * TypeName)
{
  Attrs |= FILE_ATTRIBUTE_NORMAL;

  TSHFileInfoW SHFileInfo;
  // On Win2k we get icon of "ZIP drive" for ".." (parent directory)
  if ((FileName == PARENTDIRECTORY) ||
      ((FileName.Length() == 2) && (FileName[2] == L':') && IsLetter(FileName[1])) ||
      IsReservedName(FileName))
  {
    FileName = L"dumb";
  }
  // this should be somewhere else, probably in TUnixDirView,
  // as the "partial" overlay is added there too
  int PartialFileExtLen = GetPartialFileExtLen(FileName);
  if (GetPartialFileExtLen(FileName) > 0)
  {
    FileName.SetLength(FileName.Length() - PartialFileExtLen);
  }

  int Icon;
  if (SHGetFileInfo(FileName.c_str(),
        Attrs, &SHFileInfo, sizeof(SHFileInfo),
        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME) != 0)
  {
    if (TypeName != nullptr)
    {
      *TypeName = SHFileInfo.szTypeName;
    }
    Icon = SHFileInfo.iIcon;
  }
  else
  {
    if (TypeName != nullptr)
    {
      *TypeName = L"";
    }
    Icon = -1;
  }

  return Icon;
}

bool SameUserName(const UnicodeString & UserName1, const UnicodeString & UserName2)
{
  // Bitvise reports file owner as "user@host", but we login with "user" only.
  UnicodeString AUserName1 = CopyToChar(UserName1, L'@', true);
  UnicodeString AUserName2 = CopyToChar(UserName2, L'@', true);
  return SameText(AUserName1, AUserName2);
}

UnicodeString FormatMultiFilesToOneConfirmation(const UnicodeString & Target, bool Unix)
{
  UnicodeString Dir;
  UnicodeString Name;
  UnicodeString Path;
  if (Unix)
  {
    Dir = UnixExtractFileDir(Target);
    Name = UnixExtractFileName(Target);
    Path = UnixIncludeTrailingBackslash(Target);
  }
  else
  {
    Dir = ExtractFilePath(Target);
    Name = ExtractFileName(Target);
    Path = IncludeTrailingBackslash(Target);
  }
  return FMTLOAD(MULTI_FILES_TO_ONE, Name, Dir, Path);
}
#endif // if 0


TRemoteToken::TRemoteToken() noexcept :
  FID(0),
  FIDValid(false)
{
}

TRemoteToken::TRemoteToken(const UnicodeString & Name) noexcept :
  FName(Name),
  FID(0),
  FIDValid(false)
{
}

TRemoteToken::TRemoteToken(const TRemoteToken &rhs) noexcept :
  FName(rhs.FName),
  FID(rhs.FID),
  FIDValid(rhs.FIDValid)
{
}

void TRemoteToken::Clear()
{
  FID = 0;
  FIDValid = false;
}

bool TRemoteToken::operator==(const TRemoteToken &rhs) const
{
  return
    (FName == rhs.FName) &&
    (FIDValid == rhs.FIDValid) &&
    (!FIDValid || (FID == rhs.FID));
}

bool TRemoteToken::operator!=(const TRemoteToken &rhs) const
{
  return !(*this == rhs);
}

TRemoteToken &TRemoteToken::operator=(const TRemoteToken &rhs)
{
  if (this != &rhs)
  {
    FName = rhs.FName;
    FIDValid = rhs.FIDValid;
    FID = rhs.FID;
  }
  return *this;
}

int32_t TRemoteToken::Compare(const TRemoteToken &rhs) const
{
  int32_t Result;
  if (!FName.IsEmpty())
  {
    if (!rhs.FName.IsEmpty())
    {
      Result = ::AnsiCompareText(FName, rhs.FName);
    }
    else
    {
      Result = -1;
    }
  }
  else
  {
    if (!rhs.FName.IsEmpty())
    {
      Result = 1;
    }
    else
    {
      if (FIDValid)
      {
        if (rhs.FIDValid)
        {
          Result = (FID < rhs.FID) ? -1 : ((FID > rhs.FID) ? 1 : 0);
        }
        else
        {
          Result = -1;
        }
      }
      else
      {
        if (rhs.FIDValid)
        {
          Result = 1;
        }
        else
        {
          Result = 0;
        }
      }
    }
  }
  return Result;
}

void TRemoteToken::SetID(int32_t Value)
{
  FID = Value;
  FIDValid = Value != 0;
}

bool TRemoteToken::GetNameValid() const
{
  return !FName.IsEmpty();
}

bool TRemoteToken::GetIsSet() const
{
  return !FName.IsEmpty() || FIDValid;
}

UnicodeString TRemoteToken::GetDisplayText() const
{
  if (!FName.IsEmpty())
  {
    return FName;
  }
  if (FIDValid)
  {
    return IntToStr(FID);
  }
  return UnicodeString();
}

UnicodeString TRemoteToken::GetLogText() const
{
  return FORMAT("\"%s\" [%d]", FName, nb::ToInt(FID));
}


TRemoteTokenList *TRemoteTokenList::Duplicate() const
{
  std::unique_ptr<TRemoteTokenList> Result(std::make_unique<TRemoteTokenList>());
  try__catch
  {
    TTokens::const_iterator it = FTokens.begin();
    while (it != FTokens.end())
    {
      Result->Add(*it);
      ++it;
    }
  }
  catch__removed
  ({
    delete Result;
    throw;
  })
  return Result.release();
}

void TRemoteTokenList::Clear()
{
  FTokens.clear();
  FNameMap.clear();
  FIDMap.clear();
}

void TRemoteTokenList::Add(const TRemoteToken &Token)
{
  FTokens.push_back(Token);
  if (Token.GetIDValid())
  {
    // std::pair<TIDMap::iterator, bool> Position =
    FIDMap.insert(TIDMap::value_type(Token.GetID(), FTokens.size() - 1));
  }
  if (Token.GetNameValid())
  {
    // std::pair<TNameMap::iterator, bool> Position =
    FNameMap.insert(TNameMap::value_type(Token.GetName(), FTokens.size() - 1));
  }
}

void TRemoteTokenList::AddUnique(const TRemoteToken &Token)
{
  if (Token.GetIDValid())
  {
    TIDMap::const_iterator it = FIDMap.find(Token.GetID());
    if (it != FIDMap.end())
    {
      // is present already.
      // may have different name (should not),
      // but what can we do about it anyway?
    }
    else
    {
      Add(Token);
    }
  }
  else if (Token.GetNameValid())
  {
    TNameMap::const_iterator it = FNameMap.find(Token.GetName());
    if (it != FNameMap.end())
    {
      // is present already.
    }
    else
    {
      Add(Token);
    }
  }
  else
  {
    // can happen, e.g. with winsshd/SFTP
  }
}

bool TRemoteTokenList::Exists(UnicodeString Name) const
{
  // We should make use of SameUserName
  return (FNameMap.find(Name) != FNameMap.end());
}

const TRemoteToken *TRemoteTokenList::Find(uint32_t ID) const
{
  TIDMap::const_iterator it = FIDMap.find(ID);
  const TRemoteToken *Result = nullptr;
  if (it != FIDMap.end())
  {
    Result = &FTokens[(*it).second];
  }
  return Result;
}

const TRemoteToken *TRemoteTokenList::Find(UnicodeString Name) const
{
  TNameMap::const_iterator it = FNameMap.find(Name);
  const TRemoteToken *Result = nullptr;
  if (it != FNameMap.end())
  {
    Result = &FTokens[(*it).second];
  }
  return Result;
}

void TRemoteTokenList::Log(TTerminal *Terminal, const wchar_t *Title)
{
  if (!FTokens.empty())
  {
    Terminal->LogEvent(FORMAT("Following %s found:", Title));
    for (int32_t Index = 0; Index < nb::ToIntPtr(FTokens.size()); ++Index)
    {
      Terminal->LogEvent(UnicodeString(L"  ") + FTokens[Index].GetLogText());
    }
  }
  else
  {
    Terminal->LogEvent(FORMAT("No %s found.", Title));
  }
}

int32_t TRemoteTokenList::GetCount() const
{
  return nb::ToIntPtr(FTokens.size());
}

const TRemoteToken *TRemoteTokenList::Token(int32_t Index) const
{
  return &FTokens[Index];
}


TRemoteFile::TRemoteFile(TObjectClassId Kind, TRemoteFile *ALinkedByFile) noexcept :
  TPersistent(Kind),
  FModificationFmt(mfFull),
  FLinkedByFile(ALinkedByFile),
  FIconIndex(-1),
  FIsHidden(-1)
{
  Init();
  FLinkedByFile = ALinkedByFile;
}

TRemoteFile::TRemoteFile(TRemoteFile *ALinkedByFile) noexcept :
  TPersistent(OBJECT_CLASS_TRemoteFile)
{
  Init();
  FLinkedByFile = ALinkedByFile;
  FIsEncrypted = false;
  FCalculatedSize = -1;
}

TRemoteFile::~TRemoteFile() noexcept
{
  SAFE_DESTROY(FRights);
  SAFE_DESTROY(FLinkedFile);
}

TRemoteFile *TRemoteFile::Duplicate(bool Standalone) const
{
  std::unique_ptr<TRemoteFile> Result(std::make_unique<TRemoteFile>());
  try__catch
  {
    if (FLinkedFile)
    {
      Result->FLinkedFile = FLinkedFile->Duplicate(true);
      Result->FLinkedFile->FLinkedByFile = Result.get();
    }
    Result->SetRights(FRights);
#define COPY_FP(PROP) Result->F ## PROP = F ## PROP;
    COPY_FP(Terminal);
    COPY_FP(Owner);
    COPY_FP(ModificationFmt);
    COPY_FP(Size);
    COPY_FP(CalculatedSize);
    COPY_FP(FileName);
    COPY_FP(DisplayName);
    COPY_FP(INodeBlocks);
    COPY_FP(Modification);
    COPY_FP(LastAccess);
    COPY_FP(Group);
    COPY_FP(IconIndex);
    COPY_FP(TypeName);
    COPY_FP(IsSymLink);
    COPY_FP(LinkTo);
    COPY_FP(Type);
    COPY_FP(CyclicLink);
    COPY_FP(HumanRights);
    COPY_FP(IsEncrypted);
#undef COPY_FP
    if (Standalone && (!FFullFileName.IsEmpty() || (GetDirectory() != nullptr)))
    {
      Result->FFullFileName = GetFullFileName();
    }
  }
  catch__removed
  ({
    delete Result;
    throw;
  })
  return Result.release();
}

void TRemoteFile::LoadTypeInfo() const
{
  /* TODO : If file is link: Should be attributes taken from linked file? */
#if 0
  uint32_t Attrs = INVALID_FILE_ATTRIBUTES;
  if (GetIsDirectory())
  {
    Attrs |= FILE_ATTRIBUTE_DIRECTORY;
  }
  if (GetIsHidden())
  {
    Attrs |= FILE_ATTRIBUTE_HIDDEN;
  }

  UnicodeString DumbFileName = (GetIsSymLink() && !GetLinkTo().IsEmpty() ? GetLinkTo() : GetFileName());

  FIconIndex = FakeFileImageIndex(DumbFileName, Attrs, &FTypeName);
#endif // #if 0
}

void TRemoteFile::Init()
{
  FDirectory = nullptr;
  FModificationFmt = mfFull;
  FLinkedFile = nullptr;
  FLinkedByFile = nullptr;
  FRights = new TRights();
  FTerminal = nullptr;
  FSize = 0;
  FINodeBlocks = 0;
  FIconIndex = -1;
  FIsHidden = -1;
  FIsEncrypted = false;
  FType = 0;
  FIsSymLink = false;
  FCyclicLink = false;
}

int64_t TRemoteFile::GetSize() const
{
  return GetIsDirectory() ? 0 : FSize;
}

int32_t TRemoteFile::GetIconIndex() const
{
  if (FIconIndex == -1)
  {
    const_cast<TRemoteFile *>(this)->LoadTypeInfo();
  }
  return FIconIndex;
}

UnicodeString TRemoteFile::GetTypeName() const
{
  // check availability of type info by icon index, because type name can be empty
  if (FIconIndex < 0)
  {
    LoadTypeInfo();
  }
  return FTypeName;
}

Boolean TRemoteFile::GetIsHidden() const
{
  bool Result;
  switch (FIsHidden)
  {
  case 0:
    Result = false;
    break;

  case 1:
    Result = true;
    break;

  default:
    Result = base::IsUnixHiddenFile(GetFileName());
    break;
  }

  return Result;
}

void TRemoteFile::SetIsHidden(bool Value)
{
  FIsHidden = Value ? 1 : 0;
}

Boolean TRemoteFile::GetIsDirectory() const
{
  if (IsSymLink && (FLinkedFile != nullptr))
  {
    return FLinkedFile->IsDirectory;
  }
  else
  {
    return (::UpCase(GetType()) == FILETYPE_DIRECTORY);
  }
}

Boolean TRemoteFile::GetIsParentDirectory() const
{
  return wcscmp(FFileName.c_str(), PARENTDIRECTORY) == 0;
}

Boolean TRemoteFile::GetIsThisDirectory() const
{
  return wcscmp(FFileName.c_str(), THISDIRECTORY) == 0;
}

Boolean TRemoteFile::GetIsInaccesibleDirectory() const
{
  Boolean Result = False;
  if (GetIsDirectory())
  {
    DebugAssert(GetTerminal());
    Result = !
       (base::SameUserName(GetTerminal()->TerminalGetUserName(), L"root")) ||
        (((GetRights()->GetRightUndef(TRights::rrOtherExec) != TRights::rsNo)) ||
        ((GetRights()->GetRight(TRights::rrGroupExec) != TRights::rsNo) &&
          GetTerminal()->GetMembership()->Exists(GetFileGroup().GetName())) ||
        ((GetRights()->GetRight(TRights::rrUserExec) != TRights::rsNo) &&
          (base::SameUserName(GetTerminal()->TerminalGetUserName(), GetFileOwner().GetName()))));
  }
  return Result;
}

wchar_t TRemoteFile::GetType() const
{
  return FType;
}

void TRemoteFile::SetType(wchar_t AType)
{
  FType = AType;
  FIsSymLink = (::UpCase(FType) == FILETYPE_SYMLINK);
}

const TRemoteFile *TRemoteFile::GetLinkedFile() const
{
  // do not call FindLinkedFile as it would be called repeatedly for broken symlinks
  return FLinkedFile;
}

bool TRemoteFile::GetBrokenLink() const
{
  DebugAssert(GetTerminal());
  // If file is symlink but we couldn't find linked file we assume broken link
  return (GetIsSymLink() && (FCyclicLink || !FLinkedFile) &&
      GetTerminal()->GetResolvingSymlinks());
  // "!FLinkTo.IsEmpty()" removed because it does not work with SFTP
}

bool TRemoteFile::GetIsTimeShiftingApplicable() const
{
  return GetIsTimeShiftingApplicable(GetModificationFmt());
}

bool TRemoteFile::GetIsTimeShiftingApplicable(TModificationFmt ModificationFmt)
{
  return (ModificationFmt == mfMDHM) || (ModificationFmt == mfYMDHM) || (ModificationFmt == mfFull);
}

void TRemoteFile::ShiftTimeInSeconds(int64_t Seconds)
{
  ShiftTimeInSeconds(FModification, GetModificationFmt(), Seconds);
  ShiftTimeInSeconds(FLastAccess, GetModificationFmt(), Seconds);
}

void TRemoteFile::ShiftTimeInSeconds(TDateTime &DateTime, TModificationFmt ModificationFmt, int64_t Seconds)
{
  if ((Seconds != 0) && GetIsTimeShiftingApplicable(ModificationFmt))
  {
    DebugAssert(int(DateTime) != 0);
    DateTime = IncSecond(DateTime, Seconds);
  }
}

void TRemoteFile::SetModification(const TDateTime & Value)
{
  if (FModification != Value)
  {
    FModificationFmt = mfFull;
    FModification = Value;
  }
}

UnicodeString TRemoteFile::GetUserModificationStr() const
{
  return base::UserModificationStr(GetModification(), FModificationFmt);
}

UnicodeString TRemoteFile::GetModificationStr() const
{
  return base::ModificationStr(GetModification(), FModificationFmt);
}

UnicodeString TRemoteFile::GetExtension() const
{
  return base::UnixExtractFileExt(FFileName);
}

void TRemoteFile::SetRights(const TRights * Value)
{
  FRights->Assign(Value);
}

UnicodeString TRemoteFile::GetRightsStr() const
{
  // note that HumanRights is typically an empty string
  // (with an exception of Perm-fact-only MLSD FTP listing)
  return FRights->GetUnknown() ? GetHumanRights() : FRights->GetText();
}

void TRemoteFile::SetListingStr(const UnicodeString & Value)
{
  // Value stored in 'Value' can be used for error message
  UnicodeString Line = Value;
  FIconIndex = -1;
  try
  {
    UnicodeString Col;

    // Do we need to do this (is ever TAB is LS output)?
    Line = ReplaceChar(Line, L'\t', L' ');

    SetType(Line[1]);
    Line.Delete(1, 1);

    auto GetNCol = [&]()
    {
      if (Line.IsEmpty())
        throw Exception("");
      int32_t P = Line.Pos(L' ');
      if (P)
      {
        Col = Line;
        Col.SetLength(P - 1);
        Line.Delete(1, P);
      }
      else
      {
        Col = Line;
        Line.Clear();
      }
    };
    auto GetCol = [&]()
    {
      GetNCol();
      Line = ::TrimLeft(Line);
    };

    // Rights string may contain special permission attributes (S,t, ...)
    TODO("maybe no longer necessary, once we can handle the special permissions");
    GetRightsNotConst()->SetAllowUndef(True);
    // On some system there is no space between permissions and node blocks count columns
    // so we get only first 9 characters and trim all following spaces (if any)
    GetRightsNotConst()->SetText(Line.SubString(1, 9));
    Line.Delete(1, 9);
    // Rights column maybe followed by '+', '@' or '.' signs, we ignore them
    // (On MacOS, there may be a space in between)
    if (!Line.IsEmpty() && ((Line[1] == L'+') || (Line[1] == L'@') || (Line[1] == L'.')))
    {
      Line.Delete(1, 1);
    }
    else if ((Line.Length() >= 2) && (Line[1] == L' ') &&
             ((Line[2] == L'+') || (Line[2] == L'@') || (Line[2] == L'.')))
    {
      Line.Delete(1, 2);
    }
    Line = Line.TrimLeft();

    GetCol();
    if (!::TryStrToInt64(Col, FINodeBlocks))
    {
      // if the column is not an integer, suppose it's owner
      // (Android BusyBox)
      FINodeBlocks = 0;
    }
    else
    {
      GetCol();
    }

    FOwner.SetName(Col);

    // #60 17.10.01: group name can contain space
    FGroup.SetName(L"");
    GetCol();
    int64_t ASize;
    do
    {
      FGroup.SetName(FGroup.GetName() + Col);
      GetCol();
      // SSH FS link like
      // d????????? ? ? ? ? ? name
      if ((FGroup.GetName() == L"?") && (Col == L"?"))
      {
        ASize = 0;
      }
      else
      {
        DebugAssert(!Col.IsEmpty());
        // for devices etc.. there is additional column ending by comma, we ignore it
        if (Col[Col.Length()] == L',')
          GetCol();
        ASize = ::StrToInt64Def(Col, -1);
        // if it's not a number (file size) we take it as part of group name
        // (at least on CygWin, there can be group with space in its name)
        if (ASize < 0)
          Col = L" " + Col;
      }
    }
    while (ASize < 0);

    // Do not read modification time and filename (test close to the end of this block) if it is already set.
    // if (::IsZero(FModification.GetValue()) && GetFileName().IsEmpty())
    // Do not read modification time and filename (test close to the end of this block) if it is already set.
    if (::IsZero(FModification.GetValue()) == 0)
    {
      Word Year = 0, Month = 0, Day = 0, Hour = 0, Min = 0, Sec = 0;
      Word CurrYear = 0, CurrMonth = 0, CurrDay = 0;
      ::DecodeDate(::Date(), CurrYear, CurrMonth, CurrDay);

      GetCol();
      // SSH FS link, see above
      if (Col == L"?")
      {
        GetCol();
        FModificationFmt = mfNone;
        FModification = 0;
        FLastAccess = 0;
      }
      else
      {
        bool DayMonthFormat = false;
        // format yyyy-mm-dd hh:mm:ss.ms ? // example: 2017-07-27 10:44:52.404136754 +0300 .
        int Y, M, D;
        int Filled =
          swscanf(Col.c_str(), L"%04d-%02d-%02d", &Y, &M, &D);
        if (Filled == 3)
        {
          Year = nb::ToWord(Y);
          Month = nb::ToWord(M);
          Day = nb::ToWord(D);
          GetCol();
          int H, Mn, S, MS;
          Filled = swscanf(Col.c_str(), L"%02d:%02d:%02d.%d", &H, &Mn, &S, &MS);
          if (Filled == 4)
          {
            Hour = nb::ToWord(H);
            Min = nb::ToWord(Mn);
            Sec = nb::ToWord(S);
            FModificationFmt = mfFull;
            // skip TZ (TODO)
            // do not trim leading space of filename
            GetNCol();
          }
          else
          {
            // format yyyy-mm-dd hh:mm:ss ? // example: 2017-07-27 10:44:52 +0300 TZ
            Filled = swscanf(Col.c_str(), L"%02d:%02d:%02d", &H, &Mn, &S);
            if (Filled == 3)
            {
              Hour = nb::ToWord(H);
              Min = nb::ToWord(Mn);
              Sec = nb::ToWord(S);
              FModificationFmt = mfFull;
              // skip TZ (TODO)
              // do not trim leading space of filename
              GetNCol();
            }
          }
        } else {
        // format dd mmm or mmm dd ?
        Day = nb::ToWord(::StrToIntDef(Col, 0));
        if (Day > 0)
        {
          DayMonthFormat = true;
          GetCol();
        }
        Month = 0;
        auto Col2Month = [&]()
        {
          for (Word IMonth = 0; IMonth < 12; IMonth++)
            if (!Col.CompareIC(EngShortMonthNames[IMonth]))
            {
              Month = IMonth;
              Month++;
              break;
            }
        };

        Col2Month();
        // if the column is not known month name, it may have been "yyyy-mm-dd"
        // for --full-time format
        if ((Month == 0) && (Col.Length() == 10) && (Col[5] == L'-') && (Col[8] == L'-'))
        {
          Year = nb::ToWord(Col.SubString(1, 4).ToIntPtr());
          Month = nb::ToWord(Col.SubString(6, 2).ToIntPtr());
          Day = nb::ToWord(Col.SubString(9, 2).ToIntPtr());
          GetCol();
          Hour = nb::ToWord(Col.SubString(1, 2).ToIntPtr());
          Min = nb::ToWord(Col.SubString(4, 2).ToIntPtr());
          if (Col.Length() >= 8)
          {
            Sec = nb::ToWord(::StrToInt64(Col.SubString(7, 2)));
          }
          else
          {
            Sec = 0;
          }
          FModificationFmt = mfFull;
          // skip TZ (TODO)
          // do not trim leading space of filename
          GetNCol();
        }
        else
        {
          bool FullTime = false;
          // or it may have been day name for another format of --full-time
          if (Month == 0)
          {
            GetCol();
            Col2Month();
            // neither standard, not --full-time format
            if (Month == 0)
            {
              Abort();
            }
            else
            {
              FullTime = true;
            }
          }

          if (Day == 0)
          {
            GetNCol();
            Day = nb::ToWord(::StrToInt64(Col));
          }
          if ((Day < 1) || (Day > 31))
          {
            Abort();
          }

          // second full-time format
          // ddd mmm dd hh:nn:ss yyyy
          if (FullTime)
          {
            GetCol();
            if (Col.Length() != 8)
            {
              Abort();
            }
            Hour = nb::ToWord(::StrToInt64(Col.SubString(1, 2)));
            Min = nb::ToWord(::StrToInt64(Col.SubString(4, 2)));
            Sec = nb::ToWord(::StrToInt64(Col.SubString(7, 2)));
            FModificationFmt = mfFull;
            // do not trim leading space of filename
            GetNCol();
            Year = nb::ToWord(::StrToInt64(Col));
          }
          else
          {
            // for format dd mmm the below description seems not to be true,
            // the year is not aligned to 5 characters
            if (DayMonthFormat)
            {
              GetCol();
            }
            else
            {
              // Time/Year indicator is always 5 characters long (???), on most
              // systems year is aligned to right (_YYYY), but on some to left (YYYY_),
              // we must ensure that trailing space is also deleted, so real
              // separator space is not treated as part of file name
              Col = Line.SubString(1, 6).Trim();
              Line.Delete(1, 6);
            }
          }
          {
            // GETNCOL; // We don't want to trim input strings (name with space at beginning???)
            // Check if we got time (contains :) or year
            int32_t P;
            if ((P = nb::ToWord(Col.Pos(L':'))) > 0)
            {
              Hour = nb::ToWord(::StrToInt64(Col.SubString(1, P - 1)));
              Min = nb::ToWord(::StrToInt64(Col.SubString(P + 1, Col.Length() - P)));
              if ((Hour > 23) || (Min > 59))
                Abort();
              // When we don't got year, we assume current year
              // with exception that the date would be in future
              // in this case we assume last year.
              ::DecodeDate(::Date(), Year, CurrMonth, CurrDay);
              if ((Month > CurrMonth) ||
                  (Month == CurrMonth && Day > CurrDay))
              {
                Year--;
              }
              Sec = 0;
              FModificationFmt = mfMDHM;
            }
            else
            {
              Year = nb::ToWord(::StrToInt64(Col));
              if (Year > 10000) Abort();
              // When we didn't get time we assume midnight
              Hour = 0; Min = 0; Sec = 0;
              FModificationFmt = mfMDY;
            }
          }
        }}

        if (Year == 0)
          Year = CurrYear;
        if (Month == 0)
          Month = CurrMonth;
        if (Day == 0)
          Day = CurrDay;
        FModification = EncodeDateVerbose(Year, Month, Day) + EncodeTimeVerbose(Hour, Min, Sec, 0);
        // adjust only when time is known,
        // adjusting default "midnight" time makes no sense
        if ((FModificationFmt == mfMDHM) ||
             DebugAlwaysFalse(FModificationFmt == mfYMDHM) ||
             (FModificationFmt == mfFull))
        {
          DebugAssert(Terminal != nullptr);
          FModification = ::AdjustDateTimeFromUnix(FModification,
            GetTerminal()->GetSessionData()->GetDSTMode());
        }

        if (::IsZero(FLastAccess.GetValue()))
        {
          FLastAccess = FModification;
        }
      }

      // separating space is already deleted, other spaces are treated as part of name

      // see comment at the beginning of the block
      if (FileName().IsEmpty())
      {
        FSize = ASize;

        FLinkTo.Clear();
        if (GetIsSymLink())
        {
          int32_t P = Line.Pos(SYMLINKSTR);
          if (P)
          {
            FLinkTo = Line.SubString(
                P + nb::StrLength(SYMLINKSTR), Line.Length() - P + nb::StrLength(SYMLINKSTR) + 1);
            Line.SetLength(P - 1);
          }
          else
          {
            Abort();
          }
        }
        FFileName = base::UnixExtractFileName(::Trim(Line));
      }
    }
  }
  catch (Exception &E)
  {
    throw ETerminal(&E, FMTLOAD(LIST_LINE_ERROR, Value), HELP_LIST_LINE_ERROR);
  }
}

void TRemoteFile::Complete()
{
  DebugAssert(GetTerminal() != nullptr);
  if (GetIsSymLink() && GetTerminal()->GetResolvingSymlinks())
  {
    FindLinkedFile();
  }
}

void TRemoteFile::SetEncrypted()
{
  FIsEncrypted = true;
  if (Size > TEncryption::GetOverhead())
  {
    SetSize(GetSize() - TEncryption::GetOverhead());
  }
}

void TRemoteFile::FindLinkedFile()
{
  DebugAssert(GetTerminal() && GetIsSymLink());

  if (FLinkedFile)
  {
    SAFE_DESTROY(FLinkedFile);
  }
  FLinkedFile = nullptr;

  FCyclicLink = false;
  if (!GetLinkTo().IsEmpty())
  {
    // check for cyclic link
    TRemoteFile *LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      if (LinkedBy->GetLinkTo() == GetLinkTo())
      {
        // this is currently redundant information, because it is used only to
        // detect broken symlink, which would be otherwise detected
        // by FLinkedFile == nullptr
        FCyclicLink = true;
        break;
      }
      LinkedBy = LinkedBy->FLinkedByFile;
    }
  }

  if (FCyclicLink)
  {
    TRemoteFile *LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      LinkedBy->FCyclicLink = true;
      LinkedBy = LinkedBy->FLinkedByFile;
    }
  }
  else
  {
    DebugAssert(GetTerminal()->GetResolvingSymlinks());
    GetTerminalNotConst()->SetExceptionOnFail(true);
    try
    {
      try__finally
      {
        GetTerminalNotConst()->ReadSymlink(this, FLinkedFile);
      },
      __finally
      {
        GetTerminalNotConst()->SetExceptionOnFail(false);
      } end_try__finally
    }
    catch (Exception &E)
    {
      if (isa<EFatal>(&E))
      {
        throw;
      }
      GetTerminalNotConst()->GetLog()->AddException(&E);
    }
  }
}

const TRemoteFile * TRemoteFile::Resolve() const
{
  const TRemoteFile * Result = this;
  while (Result->LinkedFile != nullptr)
  {
    Result = Result->LinkedFile;
  }
  return Result;
}

UnicodeString TRemoteFile::GetListingStr() const
{
  // note that ModificationStr is longer than 12 for mfFull
  UnicodeString LinkPart;
  // expanded from ?: to avoid memory leaks
  if (GetIsSymLink())
  {
    LinkPart = UnicodeString(SYMLINKSTR) + GetLinkTo();
  }
  return FORMAT("%s%s %3s %-8s %-8s %9s %-12s %s%s",
      GetType(), GetRights()->GetText(), ::Int64ToStr(FINodeBlocks), GetFileOwner().GetName(), GetFileGroup().GetName(),
      ::Int64ToStr(GetSize()),  // explicitly using size even for directories
      GetModificationStr(), GetFileName(),
      LinkPart);
}

UnicodeString TRemoteFile::GetFullFileName() const
{
  UnicodeString Result;
  if (FFullFileName.IsEmpty())
  {
    DebugAssert(GetTerminal());
    DebugAssert(GetDirectory() != nullptr);
    if (GetIsParentDirectory())
    {
      Result = GetDirectory()->GetParentPath();
    }
    else if (GetIsDirectory())
    {
      Result = base::UnixIncludeTrailingBackslash(GetDirectory()->GetFullDirectory() + GetFileName());
    }
    else
    {
      Result = GetDirectory()->GetFullDirectory() + GetFileName();
    }
  }
  else
  {
    Result = FFullFileName;
  }
  return Result;
}

bool TRemoteFile::GetHaveFullFileName() const
{
  return !FFullFileName.IsEmpty() || (GetDirectory() != nullptr);
}

int32_t TRemoteFile::GetAttr() const
{
  int32_t Result = 0;
  if (GetRights()->GetReadOnly())
  {
    Result |= faReadOnly;
  }
  if (GetIsHidden())
  {
    Result |= faHidden;
  }
  return Result;
}

void TRemoteFile::SetTerminal(const TTerminal * Value)
{
  FTerminal = Value;
  if (FLinkedFile)
  {
    FLinkedFile->SetTerminal(Value);
  }
}


TRemoteDirectoryFile::TRemoteDirectoryFile() noexcept :
  TRemoteFile(OBJECT_CLASS_TRemoteDirectoryFile)
{
  Init();
}

TRemoteDirectoryFile::TRemoteDirectoryFile(TObjectClassId Kind) noexcept :
  TRemoteFile(Kind)
{
  Init();
}

void TRemoteDirectoryFile::Init()
{
  SetModification(TDateTime(0.0));
  SetModificationFmt(mfNone);
  SetLastAccess(GetModification());
  SetType(L'D');
  SetSize(0);
}


TRemoteParentDirectory::TRemoteParentDirectory(TTerminal *ATerminal) noexcept :
  TRemoteDirectoryFile(OBJECT_CLASS_TRemoteParentDirectory)
{
  SetFileName(PARENTDIRECTORY);
  SetTerminal(ATerminal);
}

//=== TRemoteFileList ------------------------------------------------------
TRemoteFileList::TRemoteFileList() noexcept :
  TObjectList(OBJECT_CLASS_TRemoteFileList),
  FTimestamp(Now())
{
  SetOwnsObjects(true);
}

TRemoteFileList::TRemoteFileList(TObjectClassId Kind) noexcept :
  TObjectList(Kind),
  FTimestamp(Now())
{
  SetOwnsObjects(true);
}

void TRemoteFileList::AddFile(TRemoteFile *AFile)
{
  if (AFile)
  {
    Add(AFile);
    AFile->SetDirectory(this);
  }
}

TStrings * TRemoteFileList::CloneStrings(TStrings * List)
{
  std::unique_ptr<TStringList> Result(std::make_unique<TStringList>());
  Result->OwnsObjects = true;
  for (int32_t Index = 0; Index < List->Count; Index++)
  {
    TRemoteFile * File = List->GetAs<TRemoteFile>(Index);
    Result->AddObject(List->Strings[Index], File->Duplicate(true));
  }
  return Result.release();
}

bool TRemoteFileList::AnyDirectory(TStrings * List)
{
  bool Result = false;
  for (int32_t Index = 0; !Result && (Index < List->Count); Index++)
  {
    Result = List->GetAs<TRemoteFile>(Index)->IsDirectory;
  }
  return Result;
}

void TRemoteFileList::DuplicateTo(TRemoteFileList *Copy) const
{
  Copy->Reset();
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    TRemoteFile *File = GetFile(Index);
    Copy->AddFile(File->Duplicate(false));
  }
  Copy->FDirectory = GetDirectory();
  Copy->FTimestamp = FTimestamp;
}

void TRemoteFileList::Reset()
{
  FTimestamp = Now();
  TObjectList::Clear();
}

void TRemoteFileList::SetDirectory(const UnicodeString & Value)
{
  FDirectory = base::UnixExcludeTrailingBackslash(Value);
}

UnicodeString TRemoteFileList::GetFullDirectory() const
{
  return base::UnixIncludeTrailingBackslash(GetDirectory());
}

TRemoteFile *TRemoteFileList::GetFile(Integer Index) const
{
  return GetAs<TRemoteFile>(Index);
}

Boolean TRemoteFileList::GetIsRoot() const
{
  return (GetDirectory() == ROOTDIRECTORY);
}

UnicodeString TRemoteFileList::GetParentPath() const
{
  return base::UnixExtractFilePath(GetDirectory());
}

int64_t TRemoteFileList::GetTotalSize() const
{
  int64_t Result = 0;
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    Result += GetFile(Index)->GetSize();
  }
  return Result;
}

TRemoteFile *TRemoteFileList::FindFile(const UnicodeString AFileName) const
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetFile(Index)->GetFileName() == AFileName)
    {
      return GetFile(Index);
    }
  }
  return nullptr;
}
//=== TRemoteDirectory ------------------------------------------------------
TRemoteDirectory::TRemoteDirectory(TTerminal *ATerminal, TRemoteDirectory *Template) noexcept :
  TRemoteFileList(OBJECT_CLASS_TRemoteDirectory),
  FTerminal(ATerminal)
{
  if (Template == nullptr)
  {
    FIncludeThisDirectory = false;
    FIncludeParentDirectory = true;
  }
  else
  {
    FIncludeThisDirectory = Template->FIncludeThisDirectory;
    FIncludeParentDirectory = Template->FIncludeParentDirectory;
  }
}

TRemoteDirectory::~TRemoteDirectory() noexcept
{
  ReleaseRelativeDirectories();
}

void TRemoteDirectory::ReleaseRelativeDirectories()
{
  if ((GetThisDirectory() != nullptr) && !GetIncludeThisDirectory())
  {
    SAFE_DESTROY(FThisDirectory);
  }
  if ((GetParentDirectory() != nullptr) && !GetIncludeParentDirectory())
  {
    SAFE_DESTROY(FParentDirectory);
  }
}

void TRemoteDirectory::Reset()
{
  ReleaseRelativeDirectories();
  TRemoteFileList::Reset();
}

void TRemoteDirectory::SetDirectory(const UnicodeString & Value)
{
  TRemoteFileList::SetDirectory(Value);
}

void TRemoteDirectory::AddFile(TRemoteFile *AFile)
{
  if (AFile->GetIsThisDirectory())
  {
    FThisDirectory = AFile;
  }
  if (AFile->GetIsParentDirectory())
  {
    FParentDirectory = AFile;
  }

  if ((!AFile->GetIsThisDirectory() || GetIncludeThisDirectory()) &&
    (!AFile->GetIsParentDirectory() || GetIncludeParentDirectory()))
  {
    TRemoteFileList::AddFile(AFile);
  }
  AFile->SetTerminal(GetTerminal());
}

void TRemoteDirectory::DuplicateTo(TRemoteFileList *Copy) const
{
  TRemoteFileList::DuplicateTo(Copy);
  if (GetThisDirectory() && !GetIncludeThisDirectory())
  {
    Copy->AddFile(GetThisDirectory()->Duplicate(false));
  }
  if (GetParentDirectory() && !GetIncludeParentDirectory())
  {
    Copy->AddFile(GetParentDirectory()->Duplicate(false));
  }
}

bool TRemoteDirectory::GetLoaded() const
{
  return ((GetTerminal() != nullptr) && GetTerminal()->GetActive() && !GetDirectory().IsEmpty());
}

void TRemoteDirectory::SetIncludeParentDirectory(Boolean Value)
{
  if (GetIncludeParentDirectory() != Value)
  {
    FIncludeParentDirectory = Value;
    if (Value && GetParentDirectory())
    {
      DebugAssert(IndexOf(GetParentDirectory()) < 0);
      Add(GetParentDirectory());
    }
    else if (!Value && GetParentDirectory())
    {
      DebugAssert(IndexOf(GetParentDirectory()) >= 0);
      Extract(GetParentDirectory());
    }
  }
}

void TRemoteDirectory::SetIncludeThisDirectory(Boolean Value)
{
  if (GetIncludeThisDirectory() != Value)
  {
    FIncludeThisDirectory = Value;
    if (Value && GetThisDirectory())
    {
      DebugAssert(IndexOf(GetThisDirectory()) < 0);
      Add(GetThisDirectory());
    }
    else if (!Value && GetThisDirectory())
    {
      DebugAssert(IndexOf(GetThisDirectory()) >= 0);
      Extract(GetThisDirectory());
    }
  }
}
//===========================================================================
TRemoteDirectoryCache::TRemoteDirectoryCache() noexcept
{
  TStringList::SetSorted(true);
  SetDuplicates(dupError);
  TStringList::SetCaseSensitive(true);
}

TRemoteDirectoryCache::~TRemoteDirectoryCache() noexcept
{
  TRemoteDirectoryCache::Clear();
}

void TRemoteDirectoryCache::Clear()
{
  TGuard Guard(FSection); nb::used(Guard);

  try__finally
  {
    for (int32_t Index = 0; Index < GetCount(); ++Index)
    {
      TRemoteFileList *List = GetAs<TRemoteFileList>(Index);
      SAFE_DESTROY(List);
      SetObj(Index, nullptr);
    }
  },
  __finally
  {
    TStringList::Clear();
  } end_try__finally
}

bool TRemoteDirectoryCache::GetIsEmptyPrivate() const
{
  TGuard Guard(FSection); nb::used(Guard);

  return (const_cast<TRemoteDirectoryCache *>(this)->GetCount() == 0);
}

bool TRemoteDirectoryCache::HasFileList(const UnicodeString & Directory) const
{
  TGuard Guard(FSection); nb::used(Guard);

  int32_t Index = IndexOf(base::UnixExcludeTrailingBackslash(Directory));
  return (Index >= 0);
}

bool TRemoteDirectoryCache::HasNewerFileList(const UnicodeString & Directory,
  const TDateTime & Timestamp) const
{
  TGuard Guard(FSection); nb::used(Guard);

  int32_t Index = IndexOf(base::UnixExcludeTrailingBackslash(Directory));
  if (Index >= 0)
  {
    TRemoteFileList *FileList = GetAs<TRemoteFileList>(Index);
    if (FileList->GetTimestamp() <= Timestamp)
    {
      Index = -1;
    }
  }
  return (Index >= 0);
}

bool TRemoteDirectoryCache::GetFileList(const UnicodeString & Directory,
  TRemoteFileList * FileList) const
{
  TGuard Guard(FSection); nb::used(Guard);

  int32_t Index = IndexOf(base::UnixExcludeTrailingBackslash(Directory));
  bool Result = (Index >= 0);
  if (Result)
  {
    DebugAssert(GetObj(Index) != nullptr);
    GetAs<TRemoteFileList>(Index)->DuplicateTo(FileList);
  }
  return Result;
}

void TRemoteDirectoryCache::AddFileList(TRemoteFileList *FileList)
{
  DebugAssert(FileList);
  if (FileList)
  {
    std::unique_ptr<TRemoteFileList> Copy = std::make_unique<TRemoteFileList>();
    FileList->DuplicateTo(Copy.get());

    TGuard Guard(FSection); nb::used(Guard);

    // file list cannot be cached already with only one thread, but it can be
    // when directory is loaded by secondary terminal
    DoClearFileList(FileList->GetDirectory(), false);
    AddObject(Copy->GetDirectory(), Copy.release());
  }
}

void TRemoteDirectoryCache::ClearFileList(const UnicodeString & ADirectory, bool SubDirs)
{
  TGuard Guard(FSection); nb::used(Guard);
  DoClearFileList(ADirectory, SubDirs);
}

void TRemoteDirectoryCache::DoClearFileList(const UnicodeString ADirectory, bool SubDirs)
{
  UnicodeString Directory = base::UnixExcludeTrailingBackslash(ADirectory);
  int32_t Index = IndexOf(Directory);
  if (Index >= 0)
  {
    Delete(Index);
  }
  if (SubDirs)
  {
    UnicodeString DirectoryWithSlash = base::UnixIncludeTrailingBackslash(Directory); // optimization
    Index = GetCount() - 1;
    while (Index >= 0)
    {
      if (base::UnixIsChildPath(DirectoryWithSlash, GetString(Index)))
      {
        Delete(Index);
      }
      Index--;
    }
  }
}

void TRemoteDirectoryCache::Delete(int32_t Index)
{
  TRemoteFileList *List = GetAs<TRemoteFileList>(Index);
  SAFE_DESTROY(List);
  TStringList::Delete(Index);
}


TRemoteDirectoryChangesCache::TRemoteDirectoryChangesCache(int32_t MaxSize) noexcept :
  FMaxSize(MaxSize)
{
  SetCaseSensitive(true);
}

void TRemoteDirectoryChangesCache::Clear()
{
  TStringList::Clear();
}

bool TRemoteDirectoryChangesCache::GetIsEmptyPrivate() const
{
  return (const_cast<TRemoteDirectoryChangesCache *>(this)->GetCount() == 0);
}

void TRemoteDirectoryChangesCache::SetValue(const UnicodeString & Name,
  const UnicodeString & Value)
{
  int32_t Index = IndexOfName(Name);
  if (Index >= 0)
  {
    Delete(Index);
  }
  TStringList::SetValue(Name, Value);
}

UnicodeString TRemoteDirectoryChangesCache::GetValue(const UnicodeString & Name)
{
  UnicodeString Value = TStringList::GetValue(Name);
  TStringList::SetValue(Name, Value);
  return Value;
}

void TRemoteDirectoryChangesCache::AddDirectoryChange(
  const UnicodeString & SourceDir, const UnicodeString & Change,
  const UnicodeString & TargetDir)
{
  DebugAssert(!TargetDir.IsEmpty());
  SetValue(TargetDir, L"//");
  if (TTerminal::ExpandFileName(Change, SourceDir) != TargetDir)
  {
    UnicodeString Key;
    if (DirectoryChangeKey(SourceDir, Change, Key))
    {
      SetValue(Key, TargetDir);
    }
  }
}

void TRemoteDirectoryChangesCache::ClearDirectoryChange(
  const UnicodeString & SourceDir)
{
  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetName(Index).SubString(1, SourceDir.Length()) == SourceDir)
    {
      Delete(Index);
      Index--;
    }
  }
}

void TRemoteDirectoryChangesCache::ClearDirectoryChangeTarget(
  const UnicodeString & TargetDir)
{
  UnicodeString Key;
  // hack to clear at least local sym-link change in case symlink is deleted
  DirectoryChangeKey(base::UnixExcludeTrailingBackslash(base::UnixExtractFilePath(TargetDir)),
    base::UnixExtractFileName(TargetDir), Key);

  for (int32_t Index = 0; Index < GetCount(); ++Index)
  {
    UnicodeString Name = GetName(Index);
    if ((Name.SubString(1, TargetDir.Length()) == TargetDir) ||
        (GetValue(Name).SubString(1, TargetDir.Length()) == TargetDir) ||
        (!Key.IsEmpty() && (Name == Key)))
    {
      Delete(Index);
      Index--;
    }
  }
}

bool TRemoteDirectoryChangesCache::GetDirectoryChange(
  const UnicodeString & SourceDir, const UnicodeString & Change, UnicodeString & TargetDir) const
{
  UnicodeString Key = TTerminal::ExpandFileName(Change, SourceDir);
  bool Result = (IndexOfName(Key) >= 0);
  if (Result)
  {
    TargetDir = GetValue(Key);
    // TargetDir is not "//" here only when Change is full path to symbolic link
    if (TargetDir == L"//")
    {
      TargetDir = Key;
    }
  }
  else
  {
    Result = DirectoryChangeKey(SourceDir, Change, Key);
    if (Result)
    {
      UnicodeString Directory = GetValue(Key);
      Result = !Directory.IsEmpty();
      if (Result)
      {
        TargetDir = Directory;
      }
    }
  }
  return Result;
}

void TRemoteDirectoryChangesCache::Serialize(UnicodeString &Data) const
{
  Data = L"A";
  int32_t ACount = GetCount();
  if (ACount > FMaxSize)
  {
    std::unique_ptr<TStrings> Limited(std::make_unique<TStringList>());
    try__finally
    {
      int32_t Index = ACount - FMaxSize;
      while (Index < ACount)
      {
        Limited->Add(GetString(Index));
        ++Index;
      }
      Data += Limited->GetText();
    },
    __finally__removed
    ({
      delete Limited;
    }) end_try__finally
  }
  else
  {
    Data += GetText();
  }
}

void TRemoteDirectoryChangesCache::Deserialize(const UnicodeString & Data)
{
  if (Data.IsEmpty())
  {
    SetText(L"");
  }
  else
  {
    SetText(Data.c_str() + 1);
  }
}

bool TRemoteDirectoryChangesCache::DirectoryChangeKey(
  const UnicodeString & SourceDir, const UnicodeString & Change, UnicodeString & Key)
{
  bool Result = !Change.IsEmpty();
  if (Result)
  {
    bool Absolute = base::UnixIsAbsolutePath(Change);
    Result = !SourceDir.IsEmpty() || Absolute;
    if (Result)
    {
      // expanded from ?: to avoid memory leaks
      if (Absolute)
      {
        Key = Change;
      }
      else
      {
        Key = SourceDir + L"," + Change;
      }
    }
  }
  return Result;
}
//=== TRights ---------------------------------------------------------------
const wchar_t TRights::BasicSymbols[] = L"rwxrwxrwx";
const wchar_t TRights::CombinedSymbols[] = L"--s--s--t";
const wchar_t TRights::ExtendedSymbols[] = L"--S--S--T";
const wchar_t TRights::ModeGroups[] = L"ugo";

TRights::TRights() noexcept
{
  SetNumber(0);
  FAllowUndef = false;
  FSet = 0;
  FUnset = 0;
  Number = 0;
  FUnknown = true;
}

TRights::TRights(uint16_t ANumber) noexcept
{
  SetNumber(ANumber);
  FAllowUndef = false;
  FSet = 0;
  FUnset = 0;
  Number = ANumber;
}

TRights::TRights(const TRights &Source) noexcept
{
  Assign(&Source);
}


void TRights::Assign(const TRights *Source)
{
  FAllowUndef = Source->GetAllowUndef();
  FSet = Source->FSet;
  FUnset = Source->FUnset;
  FText = Source->FText;
  FUnknown = Source->FUnknown;
}

TRights::TRight TRights::CalculateRight(TRightGroup Group, TRightLevel Level)
{
  int Result;
  if (Level == rlSpecial)
  {
    Result = rrUserIDExec + Group;
  }
  else
  {
    DebugAssert(rlRead == 0);
    Result = rrUserRead + Level + (Group * 3);
  }
  return static_cast<TRight>(Result);
}

TRights::TFlag TRights::RightToFlag(TRights::TRight Right)
{
  return static_cast<TFlag>(1 << (rrLast - Right));
}

TRights::TFlag TRights::CalculateFlag(TRightGroup Group, TRightLevel Level)
{
  return RightToFlag(CalculateRight(Group, Level));
}

unsigned short TRights::CalculatePermissions(TRightGroup Group, TRightLevel Level, TRightLevel Level2, TRightLevel Level3)
{
  unsigned int Permissions = CalculateFlag(Group, Level);
  if (Level2 != rlNone)
  {
    Permissions |= CalculateFlag(Group, Level2);
  }
  if (Level3 != rlNone)
  {
    Permissions |= CalculateFlag(Group, Level3);
  }
  unsigned short Result = static_cast<unsigned short>(Permissions);
  DebugAssert((Permissions - Result) == 0);
  return Result;
}

bool TRights::operator ==(const TRights & rhr) const
{
  if (GetAllowUndef() || rhr.GetAllowUndef())
  {
    for (int Right = rrFirst; Right <= rrLast; Right++)
    {
      if (GetRightUndef(static_cast<TRight>(Right)) !=
            rhr.GetRightUndef(static_cast<TRight>(Right)))
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    return GetNumber() == rhr.GetNumber();
  }
}

bool TRights::operator==(uint16_t rhr) const
{
  return (GetNumber() == rhr);
}

bool TRights::operator==(TFlag rhr) const
{
  return (GetNumber() == static_cast<uint16_t>(rhr));
}

bool TRights::operator!=(const TRights &rhr) const
{
  return !(*this == rhr);
}

bool TRights::operator!=(TFlag rhr) const
{
  return !(*this == rhr);
}

TRights &TRights::operator=(uint16_t rhr)
{
  SetNumber(rhr);
  return *this;
}

TRights &TRights::operator=(const TRights &rhr)
{
  Assign(&rhr);
  return *this;
}

TRights TRights::operator~() const
{
  TRights Result(static_cast<uint16_t>(~GetNumber()));
  return Result;
}

TRights TRights::operator&(uint16_t rhr) const
{
  TRights Result(*this);
  Result &= rhr;
  return Result;
}

TRights TRights::operator&(const TRights &rhr) const
{
  TRights Result(*this);
  Result &= rhr;
  return Result;
}

TRights TRights::operator&(TFlag rhr) const
{
  TRights Result(*this);
  Result &= static_cast<uint16_t>(rhr);
  return Result;
}

TRights &TRights::operator&=(const TRights &rhr)
{
  if (GetAllowUndef() || rhr.GetAllowUndef())
  {
    for (int Right = rrFirst; Right <= rrLast; Right++)
    {
      if (GetRightUndef(static_cast<TRight>(Right)) !=
            rhr.GetRightUndef(static_cast<TRight>(Right)))
      {
        SetRightUndef(static_cast<TRight>(Right), rsUndef);
      }
    }
  }
  else
  {
    SetNumber(GetNumber() & rhr.GetNumber());
  }
  return *this;
}

TRights &TRights::operator&=(uint16_t rhr)
{
  SetNumber(GetNumber() & rhr);
  return *this;
}

TRights &TRights::operator&=(TFlag rhr)
{
  SetNumber(GetNumber() & static_cast<uint16_t>(rhr));
  return *this;
}

TRights TRights::operator|(const TRights &rhr) const
{
  TRights Result(*this);
  Result |= rhr;
  return Result;
}

TRights TRights::operator|(uint16_t rhr) const
{
  TRights Result(*this);
  Result |= rhr;
  return Result;
}

TRights &TRights::operator|=(const TRights &rhr)
{
  SetNumber(GetNumber() | rhr.GetNumber());
  return *this;
}

TRights &TRights::operator|=(uint16_t rhr)
{
  SetNumber(GetNumber() | rhr);
  return *this;
}

void TRights::SetAllowUndef(bool Value)
{
  if (FAllowUndef != Value)
  {
    DebugAssert(!Value || ((FSet | FUnset) == rfAllSpecials));
    FAllowUndef = Value;
  }
}

void TRights::SetText(const UnicodeString & Value)
{
  if (Value != GetText())
  {
    if ((Value.Length() != TextLen) ||
        (!GetAllowUndef() && (Value.Pos(UndefSymbol) > 0)) ||
        (Value.Pos(L" ") > 0))
    {
      throw Exception(FMTLOAD(RIGHTS_ERROR, Value));
    }

    FSet = 0;
    FUnset = 0;
    int32_t Flag = 00001;
    int ExtendedFlag = 01000; //-V536
    bool KeepText = false;
    for (int32_t Index = TextLen; Index >= 1; Index--)
    {
      if ((Value[Index] == UnsetSymbol) || (Value[Index] == UnsetSymbolWin))
      {
        FUnset |= static_cast<uint16_t>(Flag | ExtendedFlag);
      }
      else if (Value[Index] == UndefSymbol)
      {
        // do nothing
      }
      else if (Value[Index] == CombinedSymbols[Index - 1])
      {
        FSet |= static_cast<uint16_t>(Flag | ExtendedFlag);
      }
      else if (Value[Index] == ExtendedSymbols[Index - 1])
      {
        FSet |= static_cast<uint16_t>(ExtendedFlag);
        FUnset |= static_cast<uint16_t>(Flag);
      }
      else
      {
        if (Value[Index] != BasicSymbols[Index - 1])
        {
          KeepText = true;
        }
        FSet |= static_cast<uint16_t>(Flag);
        if (Index % 3 == 0)
        {
          FUnset |= static_cast<uint16_t>(ExtendedFlag);
        }
      }

      Flag <<= 1;
      if (Index % 3 == 1)
      {
        ExtendedFlag <<= 1;
      }
    }

    FText = KeepText ? Value : UnicodeString();
  }
  FUnknown = false;
}

void TRights::SetTextOverride(const UnicodeString & value)
{
  if (FText != value)
  {
    FText = value;
    FUnknown = false;
  }
}

UnicodeString TRights::GetText() const
{
  if (!FText.IsEmpty())
  {
    return FText;
  }
  else
  {
    UnicodeString Result(TextLen, 0);

    int32_t Flag = 00001;
    int ExtendedFlag = 01000; //-V536
    bool ExtendedPos = true;
    wchar_t Symbol;
    int32_t Index = TextLen;
    while (Index >= 1)
    {
      if (ExtendedPos &&
        ((FSet & (Flag | ExtendedFlag)) == (Flag | ExtendedFlag)))
      {
        Symbol = CombinedSymbols[Index - 1];
      }
      else if ((FSet & Flag) != 0)
      {
        Symbol = BasicSymbols[Index - 1];
      }
      else if (ExtendedPos && ((FSet & ExtendedFlag) != 0))
      {
        Symbol = ExtendedSymbols[Index - 1];
      }
      else if ((!ExtendedPos && ((FUnset & Flag) == Flag)) ||
        (ExtendedPos && ((FUnset & (Flag | ExtendedFlag)) == (Flag | ExtendedFlag))))
      {
        Symbol = UnsetSymbol;
      }
      else
      {
        Symbol = UndefSymbol;
      }

      Result[Index] = Symbol;

      Flag <<= 1;
      Index--;
      ExtendedPos = ((Index % 3) == 0);
      if (ExtendedPos)
      {
        ExtendedFlag <<= 1;
      }
    }
    return Result;
  }
}

void TRights::SetOctal(const UnicodeString & AValue)
{
  UnicodeString Value(AValue);
  if (Value.Length() == 3)
  {
    Value = L"0" + Value;
  }

  if (GetOctal() != Value)
  {
    bool Correct = (Value.Length() == 4);
    if (Correct)
    {
      for (int32_t Index = 1; (Index <= Value.Length()) && Correct; ++Index)
      {
        Correct = (Value[Index] >= L'0') && (Value[Index] <= L'7');
      }
    }

    if (!Correct)
    {
      throw Exception(FMTLOAD(INVALID_OCTAL_PERMISSIONS, AValue));
    }

    SetNumber(static_cast<uint16_t>(
      ((Value[1] - L'0') << 9) +
      ((Value[2] - L'0') << 6) +
      ((Value[3] - L'0') << 3) +
      ((Value[4] - L'0') << 0)));
    FText = L"";
  }
  FUnknown = false;
}

uint32_t TRights::GetNumberDecadic() const
{
  uint32_t N = GetNumberSet(); // used to be "Number"
  uint32_t Result =
      ((N & 07000) / 01000 * 1000) +
      ((N & 00700) /  0100 *  100) +
      ((N & 00070) /   010 *   10) +
      ((N & 00007) /    01 *    1);

  return Result;
}

UnicodeString TRights::GetOctal() const
{
  UnicodeString Result;
  uint16_t N = GetNumberSet(); // used to be "Number"
  Result.SetLength(4);
  Result[1] = static_cast<wchar_t>(L'0' + ((N & 07000) >> 9));
  Result[2] = static_cast<wchar_t>(L'0' + ((N & 00700) >> 6));
  Result[3] = static_cast<wchar_t>(L'0' + ((N & 00070) >> 3));
  Result[4] = static_cast<wchar_t>(L'0' + ((N & 00007) >> 0));

  return Result;
}

void TRights::SetNumber(const uint16_t & Value)
{
  if ((FSet != Value) || ((FSet | FUnset) != rfAllSpecials))
  {
    FSet = Value;
    FUnset = static_cast<uint16_t>(rfAllSpecials & ~FSet);
    FText.Clear();
  }
  FUnknown = false;
}

uint16_t TRights::GetNumber() const
{
  DebugAssert(!GetIsUndef());
  return FSet;
}

void TRights::SetRight(TRight Right, bool Value)
{
  SetRightUndef(Right, (Value ? rsYes : rsNo));
}

bool TRights::GetRight(TRight Right) const
{
  TState State = GetRightUndef(Right);
  DebugAssert(State != rsUndef);
  return (State == rsYes);
}

void TRights::SetRightUndef(TRight Right, TState Value)
{
  if (Value != GetRightUndef(Right))
  {
    DebugAssert((Value != rsUndef) || GetAllowUndef());

    TFlag Flag = RightToFlag(Right);

    switch (Value)
    {
      case rsYes:
        FSet |= static_cast<uint16_t>(Flag);
        FUnset &= static_cast<uint16_t>(~Flag);
        break;

      case rsNo:
        FSet &= static_cast<uint16_t>(~Flag);
        FUnset |= static_cast<uint16_t>(Flag);
        break;

      case rsUndef:
      default:
        FSet &= static_cast<uint16_t>(~Flag);
        FUnset &= static_cast<uint16_t>(~Flag);
        break;
    }

    FText.Clear();
  }
  FUnknown = false;
}

TRights::TState TRights::GetRightUndef(TRight Right) const
{
  TFlag Flag = RightToFlag(Right);
  TState Result;

  if ((FSet & Flag) != 0)
  {
    Result = rsYes;
  }
  else if ((FUnset & Flag) != 0)
  {
    Result = rsNo;
  }
  else
  {
    Result = rsUndef;
  }
  return Result;
}

void TRights::SetReadOnly(bool Value)
{
  SetRight(rrUserWrite, !Value);
  SetRight(rrGroupWrite, !Value);
  SetRight(rrOtherWrite, !Value);
}

bool TRights::GetReadOnly() const
{
  return GetRight(rrUserWrite) && GetRight(rrGroupWrite) && GetRight(rrOtherWrite);
}

UnicodeString TRights::GetChmodStr(int32_t Directory) const
{
  UnicodeString Result;
  if (IsUndef)
  {
    Result = ModeStr;
  }
  else
  {
    Result = Octal;
    if (Directory != 0) // unknown or folder
    {
      // New versions of coreutils need leading 5th zero to unset UID/GID for directories
      Result = UnicodeString(L"0") + Result;
    }
  }
  return Result;
}

UnicodeString TRights::GetModeStr() const
{
  UnicodeString Result;
  UnicodeString SetModeStr, UnsetModeStr;
  TRight Right;
  int32_t Index;

  for (int32_t Group = 0; Group < 3; Group++)
  {
    SetModeStr.Clear();
    UnsetModeStr.Clear();
    for (int32_t Mode = 0; Mode < 3; Mode++)
    {
      Index = (Group * 3) + Mode;
      Right = static_cast<TRight>(rrUserRead + Index);
      switch (GetRightUndef(Right))
      {
        case rsYes:
          SetModeStr += BasicSymbols[Index];
          break;

        case rsNo:
          UnsetModeStr += BasicSymbols[Index];
          break;

        case rsUndef:
          break;
      }
    }

    Right = static_cast<TRight>(rrUserIDExec + Group);
    Index = (Group * 3) + 2;
    switch (GetRightUndef(Right))
    {
      case rsYes:
        SetModeStr += CombinedSymbols[Index];
        break;

      case rsNo:
        UnsetModeStr += CombinedSymbols[Index];
        break;

      case rsUndef:
        break;
    }

    if (!SetModeStr.IsEmpty() || !UnsetModeStr.IsEmpty())
    {
      if (!Result.IsEmpty())
      {
        Result += L',';
      }
      Result += ModeGroups[Group];
      if (!SetModeStr.IsEmpty())
      {
        Result += L"+" + SetModeStr;
      }
      if (!UnsetModeStr.IsEmpty())
      {
        Result += L"-" + UnsetModeStr;
      }
    }
  }
  return Result;
}

void TRights::AddExecute()
{
  for (int Group = 0; Group < 3; Group++)
  {
    if ((GetRightUndef(static_cast<TRight>(rrUserRead + (Group * 3))) == rsYes) ||
        (GetRightUndef(static_cast<TRight>(rrUserWrite + (Group * 3))) == rsYes))
    {
      SetRight(static_cast<TRight>(rrUserExec + (Group * 3)), true);
      FText = L"";
    }
  }
  FUnknown = false;
}

void TRights::AllUndef()
{
  if ((FSet != 0) || (FUnset != 0))
  {
    FSet = 0;
    FUnset = 0;
    FText.Clear();
  }
  FUnknown = false;
}

bool TRights::GetIsUndef() const
{
  return ((FSet | FUnset) != rfAllSpecials);
}

TRights::operator uint16_t() const
{
  return GetNumber();
}

TRights::operator uint32_t() const
{
  return GetNumber();
}

TRights TRights::Combine(const TRights & Other) const
{
  TRights Result = (*this);
  Result |= Other.NumberSet;
  Result &= (unsigned short)~Other.NumberUnset;
  return Result;
}
//=== TRemoteProperties -------------------------------------------------------
TRemoteProperties::TRemoteProperties() :
  TObject(OBJECT_CLASS_TRemoteProperties)
{
  Default();
}

TRemoteProperties::TRemoteProperties(const TRemoteProperties &rhp) :
  TObject(OBJECT_CLASS_TRemoteProperties),
  Valid(rhp.Valid),
  Recursive(rhp.Recursive),
  Rights(rhp.Rights),
  AddXToDirectories(rhp.AddXToDirectories),
  Group(rhp.Group),
  Owner(rhp.Owner),
  Modification(rhp.Modification),
  LastAccess(rhp.Modification),
  Encrypt(rhp.Encrypt)
{
}

void TRemoteProperties::Default()
{
  Valid.Clear();
  AddXToDirectories = false;
  Rights.SetAllowUndef(false);
  Rights.SetNumber(0);
  Group.Clear();
  Owner.Clear();
  Modification = 0;
  LastAccess = 0;
  Recursive = false;
  Encrypt = false;
}

bool TRemoteProperties::operator==(const TRemoteProperties &rhp) const
{
  bool Result = (Valid == rhp.Valid && Recursive == rhp.Recursive);

  if (Result)
  {
    if ((Valid.Contains(vpRights) &&
          (Rights != rhp.Rights || AddXToDirectories != rhp.AddXToDirectories)) ||
        (Valid.Contains(vpOwner) && (Owner != rhp.Owner)) ||
        (Valid.Contains(vpGroup) && (Group != rhp.Group)) ||
        (Valid.Contains(vpModification) && (Modification != rhp.Modification)) ||
        (Valid.Contains(vpLastAccess) && (LastAccess != rhp.LastAccess)) ||
        (Valid.Contains(vpEncrypt) && (Encrypt != rhp.Encrypt)))
    {
      Result = false;
    }
  }
  return Result;
}

bool TRemoteProperties::operator!=(const TRemoteProperties &rhp) const
{
  return !(*this == rhp);
}

TRemoteProperties TRemoteProperties::CommonProperties(TStrings *AFileList)
{
  TODO("Modification and LastAccess");
  TRemoteProperties CommonProperties;
  for (int32_t Index = 0; Index < AFileList->GetCount(); ++Index)
  {
    TRemoteFile *File = AFileList->GetAs<TRemoteFile>(Index);
    DebugAssert(File);
    if (!Index)
    {
      if (!File->Rights->Unknown)
      {
        CommonProperties.Rights = *(File->Rights);
        // previously we allowed undef implicitly for directories,
        // now we do it explicitly in properties dialog and only in combination
        // with "recursive" option
        CommonProperties.Rights.AllowUndef = File->Rights->IsUndef;
        CommonProperties.Valid << vpRights;
      }
      if (File->Owner.IsSet)
      {
        CommonProperties.Owner = File->Owner;
        CommonProperties.Valid << vpOwner;
      }
      if (File->GetFileGroup().GetIsSet())
      {
        CommonProperties.Group = File->GetFileGroup();
        CommonProperties.Valid << vpGroup;
      }
    }
    else
    {
      CommonProperties.Rights.SetAllowUndef(True);
      CommonProperties.Rights &= *File->GetRights();
      if (CommonProperties.Owner != File->GetFileOwner())
      {
        CommonProperties.Owner.Clear();
        CommonProperties.Valid >> vpOwner;
      }
      if (CommonProperties.Group != File->GetFileGroup())
      {
        CommonProperties.Group.Clear();
        CommonProperties.Valid >> vpGroup;
      }
    }
  }
  return CommonProperties;
}

TRemoteProperties TRemoteProperties::ChangedProperties(
  const TRemoteProperties &OriginalProperties, TRemoteProperties &NewProperties)
{
  DebugAssert(!OriginalProperties.Valid.Contains(vpEncrypt));
  // TODO: Modification and LastAccess
  if (!NewProperties.Recursive)
  {
    if (NewProperties.Rights == OriginalProperties.Rights &&
        !NewProperties.AddXToDirectories)
    {
      NewProperties.Valid >> vpRights;
    }

    if (NewProperties.Group == OriginalProperties.Group)
    {
      NewProperties.Valid >> vpGroup;
    }

    if (NewProperties.Owner == OriginalProperties.Owner)
    {
      NewProperties.Valid >> vpOwner;
    }
  }
  return NewProperties;
}

TRemoteProperties &TRemoteProperties::operator=(const TRemoteProperties &other)
{
  Valid = other.Valid;
  Rights = other.Rights;
  Group = other.Group;
  Owner = other.Owner;
  Modification = other.Modification;
  LastAccess = other.LastAccess;
  Recursive = other.Recursive;
  AddXToDirectories = other.AddXToDirectories;
  return *this;
}

void TRemoteProperties::Load(THierarchicalStorage *Storage)
{
  uint8_t Buf[sizeof(Valid)];
  if (nb::ToSizeT(Storage->ReadBinaryData("Valid", &Buf, sizeof(Buf))) == sizeof(Buf))
  {
    memmove(&Valid, Buf, sizeof(Valid));
  }

  if (Valid.Contains(vpRights))
  {
    Rights.SetText(Storage->ReadString("Rights", Rights.GetText()));
  }

  // TODO
}

void TRemoteProperties::Save(THierarchicalStorage *Storage) const
{
  Storage->WriteBinaryData(UnicodeString(L"Valid"),
    static_cast<const void *>(&Valid), sizeof(Valid));

  if (Valid.Contains(vpRights))
  {
    Storage->WriteString("Rights", Rights.GetText());
  }

  // TODO
}


TChecklistItem::TChecklistItem() noexcept :
  Action(saNone), IsDirectory(false), RemoteFile(nullptr), Checked(true), ImageIndex(-1), FDirectoryHasSize(false)
{
  Local.ModificationFmt = mfFull;
  Local.Modification = 0;
  Local.Size = 0;
  Remote.ModificationFmt = mfFull;
  Remote.Modification = 0;
  Remote.Size = 0;
  FLocalLastWriteTime.dwHighDateTime = 0;
  FLocalLastWriteTime.dwLowDateTime = 0;
}

TChecklistItem::~TChecklistItem() noexcept
{
  SAFE_DESTROY(RemoteFile);
}

const UnicodeString& TChecklistItem::GetFileName() const
{
  if (!Remote.FileName.IsEmpty())
  {
    return Remote.FileName;
  }
  else
  {
    DebugAssert(!Local.FileName.IsEmpty());
    return Local.FileName;
  }
}

int64_t TChecklistItem::GetSize() const
{
  return GetSize(Action);
}

int64_t TChecklistItem::GetSize(TChecklistAction AAction) const
{
  if (TSynchronizeChecklist::IsItemSizeIrrelevant(AAction))
  {
    return 0;
  }
  else
  {
    switch (AAction)
    {
      case saUploadNew:
      case saUploadUpdate:
        return Local.Size;

      case saDownloadNew:
      case saDownloadUpdate:
        return Remote.Size;

      default:
        DebugFail();
        return 0;
    }
  }
}


TSynchronizeChecklist::TSynchronizeChecklist() noexcept :
  FList(std::make_unique<TList>())
{
}

TSynchronizeChecklist::~TSynchronizeChecklist() noexcept
{
  for (int Index = 0; Index < FList->Count; Index++)
  {
    TChecklistItem *Item = FList->GetAs<TChecklistItem>(Index);
    SAFE_DESTROY(Item);
  }
//  delete FList;
}

void TSynchronizeChecklist::Add(TChecklistItem* Item)
{
  FList->Add(Item);
}

int32_t TSynchronizeChecklist::Compare(const void * AItem1, const void * AItem2)
{
  const TChecklistItem *Item1 = cast_to<TChecklistItem>(AItem1);
  const TChecklistItem *Item2 = cast_to<TChecklistItem>(AItem2);

  int32_t Result;
  if (!Item1->Local.Directory.IsEmpty())
  {
    Result = ::AnsiCompareText(Item1->Local.Directory, Item2->Local.Directory);
  }
  else
  {
    DebugAssert(!Item1->Remote.Directory.IsEmpty());
    Result = ::AnsiCompareText(Item1->Remote.Directory, Item2->Remote.Directory);
  }

  if (Result == 0)
  {
    Result = ::AnsiCompareText(Item1->GetFileName(), Item2->GetFileName());
  }

  return Result;
}

void TSynchronizeChecklist::Sort()
{
  FList->Sort(Compare);
}

int32_t TSynchronizeChecklist::GetCount() const
{
  return FList->Count;
}

int32_t TSynchronizeChecklist::GetCheckedCount() const
{
  int32_t Result = 0;
  for (int Index = 0; (Index < Count); Index++)
  {
    if (GetItem(Index)->Checked)
    {
      Result++;
    }
  }
  return Result;
}

const TChecklistItem * TSynchronizeChecklist::GetItem(int32_t Index) const
{
  return FList->GetAs<TChecklistItem>(Index);
}

void TSynchronizeChecklist::Update(const TChecklistItem *Item, bool Check, TChecklistAction Action)
{
  // TSynchronizeChecklist owns non-const items so it can manipulate them freely,
  // const_cast here is just an optimization
  TChecklistItem *MutableItem = const_cast<TChecklistItem *>(Item);
  DebugAssert(FList->IndexOf(MutableItem) >= 0);
  MutableItem->Checked = Check;
  MutableItem->Action = Action;
}

void TSynchronizeChecklist::Delete(const TChecklistItem * Item)
{
  // See comment in Update()
  TChecklistItem* MutableItem = const_cast<TChecklistItem*>(Item);
  FList->Extract(MutableItem);
  SAFE_DESTROY(MutableItem);
}

void TSynchronizeChecklist::UpdateDirectorySize(const TChecklistItem* Item, int64_t Size)
{
  // See comment in Update
  TChecklistItem* MutableItem = const_cast<TChecklistItem*>(Item);
  DebugAssert(FList->IndexOf(MutableItem) >= 0);
  if (DebugAlwaysTrue(Item->IsDirectory))
  {
    MutableItem->FDirectoryHasSize = true;

    if (Item->IsRemoteOnly())
    {
      MutableItem->Remote.Size = Size;
    }
    else if (Item->IsLocalOnly())
    {
      MutableItem->Local.Size = Size;
    }
    else
    {
      // "update" actions are not relevant for directories
      DebugFail();
    }
  }
}

TChecklistAction TSynchronizeChecklist::Reverse(TChecklistAction Action)
{
  switch (Action)
  {
    case saUploadNew:
      return saDeleteLocal;

    case saDownloadNew:
      return saDeleteRemote;

    case saUploadUpdate:
      return saDownloadUpdate;

    case saDownloadUpdate:
      return saUploadUpdate;

    case saDeleteRemote:
      return saDownloadNew;

    case saDeleteLocal:
      return saUploadNew;

    default:
    case saNone:
      DebugFail();
      return saNone;
  }
}

bool TSynchronizeChecklist::IsItemSizeIrrelevant(TChecklistAction Action)
{
  switch (Action)
  {
    case saNone:
    case saDeleteRemote:
    case saDeleteLocal:
      return true;

    default:
      return false;
  }
}


TSynchronizeProgress::TSynchronizeProgress(const TSynchronizeChecklist * Checklist) noexcept
{
  FTotalSize = -1;
  FProcessedSize = 0;
  FChecklist = Checklist;
}

int64_t TSynchronizeProgress::ItemSize(const TChecklistItem * ChecklistItem) const
{
  int64_t Result;
  switch (ChecklistItem->Action)
  {
    case TChecklistAction::saDeleteRemote:
    case TChecklistAction::saDeleteLocal:
      Result = ChecklistItem->IsDirectory ? 1024*1024 : 100*1024;
      break;

    default:
      if (ChecklistItem->HasSize())
      {
        Result = ChecklistItem->GetSize();
      }
      else
      {
        DebugAssert(ChecklistItem->IsDirectory);
        Result = 1024*1024;
      }
      break;
  }
  return Result;
}

void TSynchronizeProgress::ItemProcessed(const TChecklistItem * ChecklistItem)
{
  FProcessedSize += ItemSize(ChecklistItem);
}

int64_t TSynchronizeProgress::GetProcessed(const TFileOperationProgressType * CurrentItemOperationProgress) const
{
  DebugAssert(!TFileOperationProgressType::IsIndeterminateOperation(CurrentItemOperationProgress->Operation()));

  // Need to calculate the total size on the first call only,
  // as at the time the constructor it called, we usually do not have sizes of folders caculated yet.
  if (FTotalSize < 0)
  {
    FTotalSize = 0;

    for (int32_t Index = 0; Index < FChecklist->Count; Index++)
    {
      const TChecklistItem * ChecklistItem = FChecklist->GetItem(Index);
      if (ChecklistItem->Checked)
      {
        FTotalSize += ItemSize(ChecklistItem);
      }
    }
  }

  // For (single-item-)delete operation, this should return 0
  int64_t CurrentItemProcessedSize = CurrentItemOperationProgress->GetOperationTransferred();
  return (FProcessedSize + CurrentItemProcessedSize);
}

int32_t TSynchronizeProgress::Progress(const TFileOperationProgressType * CurrentItemOperationProgress) const
{
  int32_t Result;
  if (TFileOperationProgressType::IsIndeterminateOperation(CurrentItemOperationProgress->Operation))
  {
    Result = -1;
  }
  else
  {
    __int64 Processed = GetProcessed(CurrentItemOperationProgress);
    if (FTotalSize > 0)
    {
      Result = (Processed * 100) / FTotalSize;
    }
    else
    {
      Result = 0;
    }
  }
  return Result;
}

TDateTime TSynchronizeProgress::TimeLeft(const TFileOperationProgressType * CurrentItemOperationProgress) const
{
  TDateTime Result;
  int64_t Processed = GetProcessed(CurrentItemOperationProgress);
  if (Processed > 0)
  {
    Result = TDateTime(double(Now() - CurrentItemOperationProgress->StartTime) / Processed * (FTotalSize - Processed));
  }
  return Result;
}

