//---------------------------------------------------------------------------
#include "stdafx.h"
#include <ShellAPI.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "RemoteFiles.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------

/* TODO 1 : Path class instead of std::wstring (handle relativity...) */
//---------------------------------------------------------------------------
std::wstring UnixIncludeTrailingBackslash(const std::wstring Path)
{
  // it used to return "/" when input path was empty
  if (!Path.empty() && !::IsDelimiter(Path, L"/", Path.size()))
  {
    return Path + L"/";
  }
  else
  {
    return Path;
  }
}
//---------------------------------------------------------------------------
// Keeps "/" for root path
std::wstring UnixExcludeTrailingBackslash(const std::wstring Path)
{
  if ((Path.size() > 1) && ::IsDelimiter(Path, L"/", Path.size()))
      return Path.substr(0, Path.size());
    else return Path;
}
//---------------------------------------------------------------------------
bool UnixComparePaths(const std::wstring Path1, const std::wstring Path2)
{
  return (UnixIncludeTrailingBackslash(Path1) == UnixIncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool UnixIsChildPath(std::wstring Parent, std::wstring Child)
{
  Parent = UnixIncludeTrailingBackslash(Parent);
  Child = UnixIncludeTrailingBackslash(Child);
  return (Child.substr(0, Parent.size()) == Parent);
}
//---------------------------------------------------------------------------
std::wstring UnixExtractFileDir(const std::wstring Path)
{
  int Pos = ::LastDelimiter(Path, L"/");
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return Path.substr(0, Pos - 1);
  }
  else
  {
    return (Pos == 0) ? std::wstring(L"/") : std::wstring();
  }
}
//---------------------------------------------------------------------------
// must return trailing backslash
std::wstring UnixExtractFilePath(const std::wstring Path)
{
  int Pos = ::LastDelimiter(Path, L"/");
  // it used to return Path when no slash was found
  return (Pos > 0) ? Path.substr(0, Pos) : std::wstring();
}
//---------------------------------------------------------------------------
std::wstring UnixExtractFileName(const std::wstring Path)
{
  int Pos = ::LastDelimiter(Path, L"/");
  std::wstring Result;
  if (Pos > 0)
  {
    Result = Path.substr(Pos + 1, Path.size() - Pos);
  }
  else
  {
    Result = Path;
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring UnixExtractFileExt(const std::wstring Path)
{
  std::wstring FileName = UnixExtractFileName(Path);
  int Pos = ::LastDelimiter(FileName, L".");
  return (Pos > 0) ? Path.substr(Pos, Path.size() - Pos + 1) : std::wstring();
}
//---------------------------------------------------------------------------
std::wstring ExtractFileName(const std::wstring & Path, bool Unix)
{
  if (Unix)
  {
    return UnixExtractFileName(Path);
  }
  else
  {
    return ExtractFileName(Path, false);
  }
}
//---------------------------------------------------------------------------
bool ExtractCommonPath(TStrings * Files, std::wstring & Path)
{
  assert(Files->GetCount() > 0);

  Path = ExtractFilePath(Files->GetString(0));
  bool Result = !Path.empty();
  if (Result)
  {
    for (int Index = 1; Index < Files->GetCount(); Index++)
    {
      while (!Path.empty() &&
        (Files->GetString(Index).substr(0, Path.size()) != Path))
      {
        int PrevLen = Path.size();
        Path = ExtractFilePath(ExcludeTrailingBackslash(Path));
        if (Path.size() == PrevLen)
        {
          Path = L"";
          Result = false;
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool UnixExtractCommonPath(TStrings * Files, std::wstring & Path)
{
  assert(Files->GetCount() > 0);

  Path = UnixExtractFilePath(Files->GetString(0));
  bool Result = !Path.empty();
  if (Result)
  {
    for (int Index = 1; Index < Files->GetCount(); Index++)
    {
      while (!Path.empty() &&
        (Files->GetString(Index).substr(0, Path.size()) != Path))
      {
        int PrevLen = Path.size();
        Path = UnixExtractFilePath(UnixExcludeTrailingBackslash(Path));
        if (Path.size() == PrevLen)
        {
          Path = L"";
          Result = false;
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool IsUnixRootPath(const std::wstring Path)
{
  return Path.empty() || (Path == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
bool IsUnixHiddenFile(const std::wstring FileName)
{
  return (FileName != ROOTDIRECTORY) && (FileName != PARENTDIRECTORY) &&
    !FileName.empty() && (FileName[1] == '.');
}
//---------------------------------------------------------------------------
std::wstring AbsolutePath(const std::wstring & Base, const std::wstring & Path)
{
  std::wstring Result;
  if (Path.empty())
  {
    Result = Base;
  }
  else if (Path[1] == '/')
  {
    Result = UnixExcludeTrailingBackslash(Path);
  }
  else
  {
    Result = UnixIncludeTrailingBackslash(
      UnixIncludeTrailingBackslash(Base) + Path);
    int P;
    while ((P = Result.find(L"/../")) != std::wstring::npos)
    {
      int P2 = ::LastDelimiter(Result.substr(0, P-1), L"/");
      assert(P2 > 0);
      Result.erase(P2, P - P2 + 3);
    }
    while ((P = Result.find(L"/./")) != std::wstring::npos)
    {
      Result.erase(P, 2);
    }
    Result = UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring FromUnixPath(const std::wstring Path)
{
  return ::StringReplace(Path, L"/", L"\\");
}
//---------------------------------------------------------------------------
std::wstring ToUnixPath(const std::wstring Path)
{
  return ::StringReplace(Path, L"\\", L"/");
}
//---------------------------------------------------------------------------
static void CutFirstDirectory(std::wstring & S, bool Unix)
{
  bool Root;
  int P;
  std::wstring Sep = Unix ? L"/" : L"\\";
  if (S == Sep)
  {
    S = L"";
  }
  else
  {
    if (S[1] == Sep[1])
    {
      Root = true;
      S.erase(1, 1);
    }
    else
    {
      Root = false;
    }
    if (S[1] == '.')
    {
      S.erase(1, 4);
    }
    P = ::AnsiPos(S, Sep[1]);
    if (P)
    {
      S.erase(1, P);
      S = L"..." + Sep + S;
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
//---------------------------------------------------------------------------
std::wstring MinimizeName(const std::wstring FileName, int MaxLen, bool Unix)
{
  std::wstring Drive, Dir, Name, Result;
  std::wstring Sep = Unix ? L"/" : L"\\";

  Result = FileName;
  if (Unix)
  {
    int P = ::LastDelimiter(Result, L"/");
    if (P)
    {
      Dir = Result.substr(0, P);
      Name = Result.substr(P, Result.size() - P);
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
    Name = ExtractFileName(Result, false);

    if (Dir.size() >= 2 && Dir[1] == L':')
    {
      Drive = Dir.substr(0, 2);
      Dir.erase(0, 2);
    }
  }

  while ((!Dir.empty() || !Drive.empty()) && (Result.size() > MaxLen))
  {
    if (Dir == Sep + L"..." + Sep)
    {
      Dir = L"..." + Sep;
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

  if (Result.size() > MaxLen)
  {
    Result = Result.substr(0, MaxLen);
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring MakeFileList(TStrings * FileList)
{
  std::wstring Result;
  for (int Index = 0; Index < FileList->GetCount(); Index++)
  {
    if (!Result.empty())
    {
      Result += L" ";
    }

    std::wstring FileName = FileList->GetString(Index);
    // currently this is used for local file only, so no delimiting is done
    if (FileName.find(L" ") != std::wstring::npos)
    {
      Result += L"\"" + FileName + L"\"";
    }
    else
    {
      Result += FileName;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
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
    unsigned int Y, M, D, H, N, S, MS;

    DecodeDate(DateTime, Y, M, D);
    DecodeTime(DateTime, H, N, S, MS);
    switch (Precision)
    {
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
        assert(false);
    }

    DateTime = EncodeDateVerbose(Y, M, D) + EncodeTimeVerbose(H, N, S, MS);
  }
  return DateTime;
}
//---------------------------------------------------------------------------
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2)
{
  return (Precision1 < Precision2) ? Precision1 : Precision2;
}
//---------------------------------------------------------------------------
std::wstring UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision)
{
  switch (Precision)
  {
    case mfNone:
      return L"";
    case mfMDY:
      return FormatDateTime(L"ddddd", DateTime);
    case mfMDHM:
      return FormatDateTime(L"ddddd t", DateTime);
    case mfFull:
    default:
      return FormatDateTime(L"ddddd tt", DateTime);
  }
}
//---------------------------------------------------------------------------
int FakeFileImageIndex(std::wstring FileName, unsigned long Attrs,
  std::wstring *TypeName)
{
  Attrs |= FILE_ATTRIBUTE_NORMAL;

  TSHFileInfo SHFileInfo;
  // On Win2k we get icon of "ZIP drive" for ".." (parent directory)
  if ((FileName == L"..") ||
      ((FileName.size() == 2) && (FileName[2] == ':') &&
       (tolower(FileName[1]) >= 'a') && (tolower(FileName[1]) <= 'z')) ||
      IsReservedName(FileName))
  {
    FileName = L"dumb";
  }
  // this should be somewhere else, probably in TUnixDirView,
  // as the "partial" overlay is added there too
  if (AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT))
  {
    static const size_t PartialExtLen = sizeof(PARTIAL_EXT) - 1;
    FileName.resize(FileName.size() - PartialExtLen);
  }

  int Icon = -1;
  SHFILEINFO ssfi;
  if (SHGetFileInfo((LPCTSTR)std::wstring(FileName).c_str(), 
        Attrs, &ssfi, sizeof(SHFILEINFO),
        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME) != 0)
  {
    if (TypeName != NULL)
    {
      *TypeName = SHFileInfo.GetFileType(FileName);
    }
    Icon = SHFileInfo.GetFileIconIndex(FileName, true);
  }
  else
  {
    if (TypeName != NULL)
    {
      *TypeName = L"";
    }
    Icon = -1;
  }

  return Icon;
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken() :
  FID(0),
  FIDValid(false)
{
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken(const std::wstring & Name) :
  FName(Name),
  FID(0),
  FIDValid(false)
{
}
//---------------------------------------------------------------------------
void TRemoteToken::Clear()
{
  FID = 0;
  FIDValid = false;
}
//---------------------------------------------------------------------------
bool TRemoteToken::operator ==(const TRemoteToken & rht) const
{
  return
    (FName == rht.FName) &&
    (FIDValid == rht.FIDValid) &&
    (!FIDValid || (FID == rht.FID));
}
//---------------------------------------------------------------------------
bool TRemoteToken::operator !=(const TRemoteToken & rht) const
{
  return !(*this == rht);
}
//---------------------------------------------------------------------------
TRemoteToken & TRemoteToken::operator =(const TRemoteToken & rht)
{
  if (this != &rht)
  {
    FName = rht.FName;
    FIDValid = rht.FIDValid;
    FID = rht.FID;
  }
  return *this;
}
//---------------------------------------------------------------------------
int TRemoteToken::Compare(const TRemoteToken & rht) const
{
  int Result;
  if (!FName.empty())
  {
    if (!rht.FName.empty())
    {
      Result = AnsiCompareText(FName, rht.FName);
    }
    else
    {
      Result = -1;
    }
  }
  else
  {
    if (!rht.FName.empty())
    {
      Result = 1;
    }
    else
    {
      if (FIDValid)
      {
        if (rht.FIDValid)
        {
          Result = (FID < rht.FID) ? -1 : ((FID > rht.FID) ? 1 : 0);
        }
        else
        {
          Result = -1;
        }
      }
      else
      {
        if (rht.FIDValid)
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
//---------------------------------------------------------------------------
void TRemoteToken::SetID(unsigned int value)
{
  FID = value;
  FIDValid = true;
}
//---------------------------------------------------------------------------
bool TRemoteToken::GetNameValid() const
{
  return !FName.empty();
}
//---------------------------------------------------------------------------
bool TRemoteToken::GetIsSet() const
{
  return !FName.empty() || FIDValid;
}
//---------------------------------------------------------------------------
std::wstring TRemoteToken::GetDisplayText() const
{
  if (!FName.empty())
  {
    return FName;
  }
  else if (FIDValid)
  {
    return IntToStr(FID);
  }
  else
  {
    return std::wstring();
  }
}
//---------------------------------------------------------------------------
std::wstring TRemoteToken::GetLogText() const
{
  return FORMAT(L"\"%s\" [%d]", FName.c_str(), int(FID));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteTokenList * TRemoteTokenList::Duplicate() const
{
  TRemoteTokenList * Result = new TRemoteTokenList();
  try
  {
    TTokens::const_iterator I = FTokens.begin();
    while (I != FTokens.end())
    {
      Result->Add(*I);
      ++I;
    }
  }
  catch(...)
  {
    delete Result;
    throw;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteTokenList::Clear()
{
  FTokens.clear();
  FNameMap.clear();
  FIDMap.clear();
}
//---------------------------------------------------------------------------
void TRemoteTokenList::Add(const TRemoteToken & Token)
{
  FTokens.push_back(Token);
  if (Token.GetIDValid())
  {
    std::pair<TIDMap::iterator, bool> Position =
      FIDMap.insert(TIDMap::value_type(Token.GetID(), FTokens.size() - 1));
  }
  if (Token.GetNameValid())
  {
    std::pair<TNameMap::iterator, bool> Position =
      FNameMap.insert(TNameMap::value_type(Token.GetName(), FTokens.size() - 1));
  }
}
//---------------------------------------------------------------------------
void TRemoteTokenList::AddUnique(const TRemoteToken & Token)
{
  if (Token.GetIDValid())
  {
    TIDMap::const_iterator I = FIDMap.find(Token.GetID());
    if (I != FIDMap.end())
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
    TNameMap::const_iterator I = FNameMap.find(Token.GetName());
    if (I != FNameMap.end())
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
    // noop
  }
}
//---------------------------------------------------------------------------
bool TRemoteTokenList::Exists(const std::wstring & Name) const
{
  return (FNameMap.find(Name) != FNameMap.end());
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Find(unsigned int ID) const
{
  TIDMap::const_iterator I = FIDMap.find(ID);
  const TRemoteToken * Result;
  if (I != FIDMap.end())
  {
    Result = &FTokens[(*I).second];
  }
  else
  {
    Result = NULL;
  }
  return Result;
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Find(const std::wstring & Name) const
{
  TNameMap::const_iterator I = FNameMap.find(Name);
  const TRemoteToken * Result;
  if (I != FNameMap.end())
  {
    Result = &FTokens[(*I).second];
  }
  else
  {
    Result = NULL;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteTokenList::Log(TTerminal * Terminal, const char * Title)
{
  if (!FTokens.empty())
  {
    Terminal->LogEvent(FORMAT(L"Following %s found:", Title));
    for (size_t Index = 0; Index < FTokens.size(); Index++)
    {
      Terminal->LogEvent(std::wstring(L"  ") + FTokens[Index].GetLogText());
    }
  }
  else
  {
    Terminal->LogEvent(FORMAT(L"No %s found.", Title));
  }
}
//---------------------------------------------------------------------------
int TRemoteTokenList::Count() const
{
  return (int)FTokens.size();
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Token(int Index) const
{
  return &FTokens[Index];
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteFile::TRemoteFile(TRemoteFile * ALinkedByFile):
  TPersistent()
{
  FLinkedFile = NULL;
  FRights = new TRights();
  FIconIndex = -1;
  FCyclicLink = false;
  FModificationFmt = mfFull;
  FLinkedByFile = ALinkedByFile;
  FTerminal = NULL;
  FDirectory = NULL;
  FIsHidden = -1;
  Self = this;
}
//---------------------------------------------------------------------------
TRemoteFile::~TRemoteFile()
{
  delete FRights;
  delete FLinkedFile;
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFile::Duplicate(bool Standalone) const
{
  TRemoteFile * Result;
  Result = new TRemoteFile();
  try
  {
    if (FLinkedFile)
    {
      Result->FLinkedFile = FLinkedFile->Duplicate(true);
      Result->FLinkedFile->FLinkedByFile = Result;
    }
    *Result->GetRights() = *FRights;
    #define COPY_FP(PROP) Result->F ## PROP = F ## PROP;
    COPY_FP(Terminal);
    COPY_FP(Owner);
    COPY_FP(ModificationFmt);
    COPY_FP(Size);
    COPY_FP(FileName);
    COPY_FP(INodeBlocks);
    COPY_FP(Modification);
    COPY_FP(LastAccess);
    COPY_FP(Group);
    COPY_FP(IconIndex);
    COPY_FP(TypeName);
    COPY_FP(IsSymLink);
    COPY_FP(LinkTo);
    COPY_FP(Type);
    COPY_FP(Selected);
    COPY_FP(CyclicLink);
    #undef COPY_FP
    if (Standalone && (!FFullFileName.empty() || (GetDirectory() != NULL)))
    {
      Result->FFullFileName = GetFullFileName();
    }
  }
  catch(...)
  {
    delete Result;
    throw;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::LoadTypeInfo()
{
  /* TODO : If file is link: Should be attributes taken from linked file? */
  unsigned long Attrs = 0;
  if (GetIsDirectory()) Attrs |= FILE_ATTRIBUTE_DIRECTORY;
  if (GetIsHidden()) Attrs |= FILE_ATTRIBUTE_HIDDEN;

  std::wstring DumbFileName = (GetIsSymLink() && !GetLinkTo().empty() ? GetLinkTo() : GetFileName());

  FIconIndex = FakeFileImageIndex(DumbFileName, Attrs, &FTypeName);
}
//---------------------------------------------------------------------------
int TRemoteFile::GetIconIndex() const
{
  if (FIconIndex == -1)
  {
    const_cast<TRemoteFile *>(this)->LoadTypeInfo();
  }
  return FIconIndex;
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetTypeName()
{
  // check avilability of type info by icon index, because type name can be empty
  if (FIconIndex < 0)
  {
    LoadTypeInfo();
  }
  return FTypeName;
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsHidden()
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
      Result = IsUnixHiddenFile(GetFileName());
      break;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetIsHidden(bool value)
{
  FIsHidden = value ? 1 : 0;
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsDirectory() const
{
  return (toupper(GetType()) == FILETYPE_DIRECTORY);
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsThisDirectory() const
{
  return (GetFileName() == THISDIRECTORY);
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsInaccesibleDirectory() const
{
  bool Result;
  if (GetIsDirectory())
  {
    assert(GetTerminal());
    Result = !
       (((GetRights()->GetRightUndef(TRights::rrOtherExec) != TRights::rsNo)) ||
        ((GetRights()->GetRight(TRights::rrGroupExec) != TRights::rsNo) &&
         GetTerminal()->GetMembership()->Exists(GetGroup().GetName())) ||
        ((GetRights()->GetRight(TRights::rrUserExec) != TRights::rsNo) &&
         (AnsiCompareText(GetTerminal()->GetUserName(), GetOwner().GetName()) == 0)));
  }
    else Result = false;
  return Result;
}
//---------------------------------------------------------------------------
char TRemoteFile::GetType() const
{
  if (GetIsSymLink() && FLinkedFile) return FLinkedFile->GetType();
  else return FType;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetType(char AType)
{
  FType = AType;
  // Allow even non-standard file types (e.g. 'S')
  // if (!std::wstring("-DL").find_first_of((char)toupper(FType))) Abort();
  FIsSymLink = ((char)toupper(FType) == FILETYPE_SYMLINK);
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFile::GetLinkedFile()
{
  // it would be called releatedly for broken symlinks
  //if (!FLinkedFile) FindLinkedFile();
  return FLinkedFile;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetLinkedFile(TRemoteFile * value)
{
  if (FLinkedFile != value)
  {
    if (FLinkedFile) delete FLinkedFile;
    FLinkedFile = value;
  }
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetBrokenLink()
{
  assert(GetTerminal());
  // If file is symlink but we couldn't find linked file we assume broken link
  return (GetIsSymLink() && (FCyclicLink || !FLinkedFile) &&
    GetTerminal()->GetResolvingSymlinks());
  // "!FLinkTo.empty()" removed because it does not work with SFTP
}
//---------------------------------------------------------------------------
void TRemoteFile::ShiftTime(const TDateTime & Difference)
{
  double D = double(Difference.operator double());
  if ((D != 0) && (FModificationFmt != mfMDY))
  {
    assert(int(FModification) != 0);
    FModification = double(FModification) + D;
    assert(int(FLastAccess) != 0);
    FLastAccess = double(FLastAccess) + D;
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::SetModification(const TDateTime & value)
{
  if (FModification != value)
  {
    FModificationFmt = mfFull;
    FModification = value;
  }
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetUserModificationStr()
{
  return ::UserModificationStr(GetModification(), FModificationFmt);
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetModificationStr()
{
  unsigned int Year, Month, Day, Hour, Min, Sec, MSec;
  GetModification().DecodeDate(Year, Month, Day);
  GetModification().DecodeTime(Hour, Min, Sec, MSec);
  switch (FModificationFmt)
  {
    case mfNone:
      return L"";

    case mfMDY:
      return FORMAT(L"%3s %2d %2d", EngShortMonthNames[Month-1], Day, Year);

    case mfMDHM:
      return FORMAT(L"%3s %2d %2d:%2.2d",
        EngShortMonthNames[Month-1], Day, Hour, Min);

    default:
      assert(false);
      // fall thru

    case mfFull:
      return FORMAT(L"%3s %2d %2d:%2.2d:%2.2d %4d",
        EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year);
  }
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetExtension()
{
  return UnixExtractFileExt(FFileName);
}
//---------------------------------------------------------------------------
void TRemoteFile::SetRights(TRights * value)
{
  FRights->Assign(value);
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetRightsStr()
{
  return FRights->GetUnknown() ? std::wstring() : FRights->GetText();
}
//---------------------------------------------------------------------------
void TRemoteFile::SetListingStr(std::wstring value)
{
  // Value stored in 'value' can be used for error message
  std::wstring Line = value;
  FIconIndex = -1;
  try
  {
    std::wstring Col;

    // Do we need to do this (is ever TAB is LS output)?
    Line = ReplaceChar(Line, '\t', ' ');
    DEBUG_PRINTF(L"Line1 = %s", Line.c_str());

    SetType(Line[0]);
    Line.erase(0, 1);
    DEBUG_PRINTF(L"Line2 = %s", Line.c_str());

    #define GETNCOL  \
      { if (Line.empty()) throw ExtException(L""); \
        int P = Line.find_first_of(L' '); \
        if (P) { Col = Line.substr(0, P-1); Line.erase(0, P); } \
          else { Col = Line; Line = L""; } \
      }
    #define GETCOL { GETNCOL; Line = ::TrimLeft(Line); }

    // Rights string may contain special permission attributes (S,t, ...)
    // (TODO: maybe no longer necessary, once we can handle the special permissions)
    GetRights()->SetAllowUndef(true);
    // On some system there is no space between permissions and node blocks count columns
    // so we get only first 9 characters and trim all following spaces (if any)
    GetRights()->SetText(Line.substr(0, 9));
    Line.erase(0, 9);
    DEBUG_PRINTF(L"Line3 = %s", Line.c_str());
    // Rights column maybe followed by '+', '@' or '.' signs, we ignore them
    // (On MacOS, there may be a space in between)
    if (!Line.empty() && ((Line[0] == '+') || (Line[0] == '@') || (Line[0] == '.')))
    {
      Line.erase(0, 1);
      DEBUG_PRINTF(L"Line4 = %s", Line.c_str());
    }
    else if ((Line.size() >= 2) && (Line[0] == ' ') &&
             ((Line[1] == '+') || (Line[1] == '@') || (Line[1] == '.')))
    {
      Line.erase(0, 2);
      DEBUG_PRINTF(L"Line5 = %s", Line.c_str());
    }
    Line = ::TrimLeft(Line);
    DEBUG_PRINTF(L"Line6 = %s", Line.c_str());

    GETCOL;
    FINodeBlocks = StrToInt(Col);

    GETCOL;
    FOwner.SetName(Col);

    // #60 17.10.01: group name can contain space
    FGroup.SetName(L"");
    GETCOL;
    __int64 ASize;
    do
    {
      FGroup.SetName(FGroup.GetName() + Col);
      GETCOL;
      assert(!Col.empty());
      // for devices etc.. there is additional column ending by comma, we ignore it
      if (Col[Col.size() - 1] == ',') GETCOL;
      ASize = StrToInt64Def(Col, -1);
      // if it's not a number (file size) we take it as part of group name
      // (at least on CygWin, there can be group with space in its name)
      if (ASize < 0) Col = L" " + Col;
    }
    while (ASize < 0);

    // do not read modification time and filename if it is already set
    if (double(FModification) == 0 && GetFileName().empty())
    {
      FSize = ASize;

      bool FullTime = false;
      bool DayMonthFormat = false;
      unsigned int Day, Month, Year, Hour, Min, Sec, P;

      GETCOL;
      // format dd mmm or mmm dd ?
      Day = (unsigned int)StrToIntDef(Col, 0);
      if (Day > 0)
      {
        DayMonthFormat = true;
        GETCOL;
      }
      Month = 0;
      #define COL2MONTH \
        for (unsigned int IMonth = 0; IMonth < 12; IMonth++) \
          if (!AnsiCompareIC(Col, ::MB2W(EngShortMonthNames[IMonth]))) { Month = IMonth; Month++; break; }
      COL2MONTH;
      // if the column is not known month name, it may have been "yyyy-mm-dd"
      // for --full-time format
      if ((Month == 0) && (Col.size() == 10) && (Col[5] == '-') && (Col[8] == '-'))
      {
        Year = (unsigned int)ToInt(Col.substr(1, 4));
        Month = (unsigned int)ToInt(Col.substr(6, 2));
        Day = (unsigned int)ToInt(Col.substr(9, 2));
        GETCOL;
        Hour = (unsigned int)ToInt(Col.substr(1, 2));
        Min = (unsigned int)ToInt(Col.substr(4, 2));
        Sec = (unsigned int)ToInt(Col.substr(7, 2));
        FModificationFmt = mfFull;
        // skip TZ (TODO)
        // do not trim leading space of filename
        GETNCOL;
      }
      else
      {
        // or it may have been day name for another format of --full-time
        if (Month == 0)
        {
          GETCOL;
          COL2MONTH;
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
        #undef COL2MONTH

        if (Day == 0)
        {
          GETNCOL;
          Day = (unsigned int)StrToInt(Col);
        }
        if ((Day < 1) || (Day > 31)) Abort();

        // second full-time format
        // ddd mmm dd hh:nn:ss yyyy
        if (FullTime)
        {
          GETCOL;
          if (Col.size() != 8)
          {
            Abort();
          }
          Hour = (unsigned int)StrToInt(Col.substr(0, 2));
          Min = (unsigned int)StrToInt(Col.substr(3, 2));
          Sec = (unsigned int)StrToInt(Col.substr(6, 2));
          FModificationFmt = mfFull;
          // do not trim leading space of filename
          GETNCOL;
          Year = (unsigned int)StrToInt(Col);
        }
        else
        {
          // for format dd mmm the below description seems not to be true,
          // the year is not aligned to 5 characters
          if (DayMonthFormat)
          {
            GETCOL;
          }
          else
          {
            // Time/Year indicator is always 5 charactes long (???), on most
            // systems year is aligned to right (_YYYY), but on some to left (YYYY_),
            // we must ensure that trailing space is also deleted, so real
            // separator space is not treated as part of file name
            Col = Trim(Line.substr(0, 6));
            Line.erase(0, 6);
          }
          // GETNCOL; // We don't want to trim input strings (name with space at beginning???)
          // Check if we got time (contains :) or year
          if ((P = (unsigned int)Col.find(L':')) != std::wstring::npos)
          {
            unsigned int CurrMonth, CurrDay;
            Hour = (unsigned int)StrToInt(Col.substr(0, P-1));
            Min = (unsigned int)StrToInt(Col.substr(P, Col.size() - P));
            if (Hour > 23 || Hour > 59) Abort();
            // When we don't got year, we assume current year
            // with exception that the date would be in future
            // in this case we assume last year.
            DecodeDate(Date(), Year, CurrMonth, CurrDay);
            if ((Month > CurrMonth) ||
                (Month == CurrMonth && Day > CurrDay)) Year--;
            Sec = 0;
            FModificationFmt = mfMDHM;
          }
            else
          {
            Year = (unsigned int)StrToInt(Col);
            if (Year > 10000) Abort();
            // When we don't got time we assume midnight
            Hour = 0; Min = 0; Sec = 0;
            FModificationFmt = mfMDY;
          }
        }
      }

      FModification = EncodeDateVerbose(Year, Month, Day) + EncodeTimeVerbose(Hour, Min, Sec, 0);
      // adjust only when time is known,
      // adjusting default "midnight" time makes no sense
      if ((FModificationFmt == mfMDHM) || (FModificationFmt == mfFull))
      {
        assert(GetTerminal() != NULL);
        FModification = AdjustDateTimeFromUnix(FModification,
          GetTerminal()->GetSessionData()->GetDSTMode());
      }

      if (double(FLastAccess) == 0)
      {
        FLastAccess = FModification;
      }

      // separating space is already deleted, other spaces are treated as part of name

      {
        int P;

        FLinkTo = L"";
        if (GetIsSymLink())
        {
          P = Line.find(SYMLINKSTR);
          if (P != std::wstring::npos)
          {
            FLinkTo = Line.substr(
              P + std::wstring(SYMLINKSTR).size(), Line.size() - P + std::wstring(SYMLINKSTR).size() + 1);
            Line.resize(P - 1);
          }
          else
          {
            Abort();
          }
        }
        FFileName = UnixExtractFileName(Line);
      }
    }

    #undef GETNCOL
    #undef GETCOL
  }
  catch (const std::exception &E)
  {
    throw ETerminal(::FmtLoadStr(LIST_LINE_ERROR, value.c_str()), &E);
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::Complete()
{
  assert(GetTerminal() != NULL);
  if (GetIsSymLink() && GetTerminal()->GetResolvingSymlinks())
  {
    FindLinkedFile();
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::FindLinkedFile()
{
  assert(GetTerminal() && GetIsSymLink());

  if (FLinkedFile) delete FLinkedFile;
  FLinkedFile = NULL;

  FCyclicLink = false;
  if (!GetLinkTo().empty())
  {
    // check for cyclic link
    TRemoteFile * LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      if (LinkedBy->GetLinkTo() == GetLinkTo())
      {
        // this is currenly redundant information, because it is used only to
        // detect broken symlink, which would be otherwise detected
        // by FLinkedFile == NULL
        FCyclicLink = true;
        break;
      }
      LinkedBy = LinkedBy->FLinkedByFile;
    }
  }

  if (FCyclicLink)
  {
    TRemoteFile * LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      LinkedBy->FCyclicLink = true;
      LinkedBy = LinkedBy->FLinkedByFile;
    }
  }
  else
  {
    assert(GetTerminal()->GetResolvingSymlinks());
    GetTerminal()->SetExceptionOnFail(true);
    try
    {
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->GetTerminal()->SetExceptionOnFail(false);
        } BOOST_SCOPE_EXIT_END
        GetTerminal()->ReadSymlink(this, FLinkedFile);
      }
    }
    catch (const std::exception &E)
    {
      if (::InheritsFrom<std::exception, EFatal>(&E))
        throw;
      else
        GetTerminal()->GetLog()->AddException(&E);
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetListingStr()
{
  // note that ModificationStr is longer than 12 for mfFull
  std::wstring LinkPart;
  // expanded from ?: to avoid memory leaks
  if (GetIsSymLink())
  {
    LinkPart = std::wstring(SYMLINKSTR) + GetLinkTo();
  }
  return FORMAT(L"%s%s %3s %-8s %-8s %9s %-12s %s%s", 
    GetType(), GetRights()->GetText().c_str(), IntToStr(GetINodeBlocks()).c_str(),
    GetOwner().GetName().c_str(),
    GetGroup().GetName().c_str(), IntToStr(GetSize()).c_str(), GetModificationStr().c_str(),
    GetFileName().c_str(),
    LinkPart.c_str());
}
//---------------------------------------------------------------------------
std::wstring TRemoteFile::GetFullFileName() const
{
  if (FFullFileName.empty())
  {
    assert(GetTerminal());
    assert(GetDirectory() != NULL);
    std::wstring Path;
    if (GetIsParentDirectory()) Path = GetDirectory()->GetParentPath();
      else
    if (GetIsDirectory()) Path = UnixIncludeTrailingBackslash(GetDirectory()->GetFullDirectory() + GetFileName());
      else Path = GetDirectory()->GetFullDirectory() + GetFileName();
    return GetTerminal()->TranslateLockedPath(Path, true);
  }
  else
  {
    return FFullFileName;
  }
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetHaveFullFileName() const
{
  return !FFullFileName.empty() || (GetDirectory() != NULL);
}
//---------------------------------------------------------------------------
int TRemoteFile::GetAttr()
{
  int Result = 0;
  ::Error(SNotImplemented, 215); 
  if (GetRights()->GetReadOnly()) Result |= 0; // FIXME faReadOnly;
  if (GetIsHidden()) Result |= 0; // FIXME faHidden;
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetTerminal(TTerminal * value)
{
  FTerminal = value;
  if (FLinkedFile)
  {
    FLinkedFile->SetTerminal(value);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryFile::TRemoteDirectoryFile() : TRemoteFile()
{
  SetModification(TDateTime(double(0)));
  SetModificationFmt(mfNone);
  SetLastAccess(GetModification());
  SetType('D');
  SetSize(0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteParentDirectory::TRemoteParentDirectory(TTerminal * ATerminal)
  : TRemoteDirectoryFile()
{
  SetFileName(PARENTDIRECTORY);
  SetTerminal(ATerminal);
}
//=== TRemoteFileList ------------------------------------------------------
TRemoteFileList::TRemoteFileList():
  TObjectList()
{
  FTimestamp = Now();
}
//---------------------------------------------------------------------------
void TRemoteFileList::AddFile(TRemoteFile * File)
{
  Add(File);
  File->SetDirectory(this);
}
//---------------------------------------------------------------------------
void TRemoteFileList::DuplicateTo(TRemoteFileList * Copy)
{
  Copy->Clear();
  for (int Index = 0; Index < GetCount(); Index++)
  {
    TRemoteFile * File = GetFile(Index);
    Copy->AddFile(File->Duplicate(false));
  }
  Copy->FDirectory = GetDirectory();
  Copy->FTimestamp = FTimestamp;
}
//---------------------------------------------------------------------------
void TRemoteFileList::Clear()
{
  FTimestamp = Now();
  TObjectList::Clear();
}
//---------------------------------------------------------------------------
void TRemoteFileList::SetDirectory(std::wstring value)
{
  FDirectory = UnixExcludeTrailingBackslash(value);
}
//---------------------------------------------------------------------------
std::wstring TRemoteFileList::GetFullDirectory()
{
  return UnixIncludeTrailingBackslash(GetDirectory());
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::GetFile(int Index)
{
  return (TRemoteFile *)GetItem(Index);
}
//---------------------------------------------------------------------------
bool TRemoteFileList::GetIsRoot()
{
  return (GetDirectory() == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
std::wstring TRemoteFileList::GetParentPath()
{
  return UnixExtractFilePath(GetDirectory());
}
//---------------------------------------------------------------------------
__int64 TRemoteFileList::GetTotalSize()
{
  __int64 Result = 0;
  for (int Index = 0; Index < GetCount(); Index++)
    if (!GetFile(Index)->GetIsDirectory()) Result += GetFile(Index)->GetSize();
  return Result;
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::FindFile(const std::wstring &FileName)
{
  for (int Index = 0; Index < GetCount(); Index++)
    if (GetFile(Index)->GetFileName() == FileName) return GetFile(Index);
  return NULL;
}
//=== TRemoteDirectory ------------------------------------------------------
TRemoteDirectory::TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template) :
  TRemoteFileList(), FTerminal(aTerminal)
{
  FSelectedFiles = NULL;
  FThisDirectory = NULL;
  FParentDirectory = NULL;
  if (Template == NULL)
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
//---------------------------------------------------------------------------
void TRemoteDirectory::Clear()
{
  if (GetThisDirectory() && !GetIncludeThisDirectory())
  {
    delete FThisDirectory;
    FThisDirectory = NULL;
  }
  if (GetParentDirectory() && !GetIncludeParentDirectory())
  {
    delete FParentDirectory;
    FParentDirectory = NULL;
  }

  TRemoteFileList::Clear();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetDirectory(std::wstring value)
{
  TRemoteFileList::SetDirectory(value);
  //Load();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::AddFile(TRemoteFile * File)
{
  if (File->GetIsThisDirectory()) FThisDirectory = File;
  if (File->GetIsParentDirectory()) FParentDirectory = File;

  if ((!File->GetIsThisDirectory() || GetIncludeThisDirectory()) &&
      (!File->GetIsParentDirectory() || GetIncludeParentDirectory()))
  {
    TRemoteFileList::AddFile(File);
  }
  File->SetTerminal(GetTerminal());
}
//---------------------------------------------------------------------------
void TRemoteDirectory::DuplicateTo(TRemoteFileList * Copy)
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
//---------------------------------------------------------------------------
bool TRemoteDirectory::GetLoaded()
{
  return ((GetTerminal() != NULL) && GetTerminal()->GetActive() && !GetDirectory().empty());
}
//---------------------------------------------------------------------------
TStrings * TRemoteDirectory::GetSelectedFiles()
{
  if (!FSelectedFiles)
  {
    FSelectedFiles = new TStringList();
  }
  else
  {
    FSelectedFiles->Clear();
  }

  for (int Index = 0; Index < GetCount(); Index ++)
  {
    if (GetFile(Index)->GetSelected())
    {
      FSelectedFiles->Add(GetFile(Index)->GetFullFileName());
    }
  }

  return FSelectedFiles;
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeParentDirectory(bool value)
{
  if (GetIncludeParentDirectory() != value)
  {
    FIncludeParentDirectory = value;
    if (value && GetParentDirectory())
    {
      assert(IndexOf(GetParentDirectory()) < 0);
      Add(GetParentDirectory());
    }
    else if (!value && GetParentDirectory())
    {
      assert(IndexOf(GetParentDirectory()) >= 0);
     ::Error(SNotImplemented, 216); 
      // FIXME Extract(GetParentDirectory());
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeThisDirectory(bool value)
{
  if (GetIncludeThisDirectory() != value)
  {
    FIncludeThisDirectory = value;
    if (value && GetThisDirectory())
    {
      assert(IndexOf(GetThisDirectory()) < 0);
      Add(GetThisDirectory());
    }
    else if (!value && GetThisDirectory())
    {
      assert(IndexOf(GetThisDirectory()) >= 0);
      ::Error(SNotImplemented, 217); 
      // FIXME Extract(GetThisDirectory());
    }
  }
}
//===========================================================================
TRemoteDirectoryCache::TRemoteDirectoryCache(): TStringList()
{
  FSection = new TCriticalSection();
  Self = this;
  SetSorted(true);
  SetDuplicates(dupError);
  SetCaseSensitive(true);
}
//---------------------------------------------------------------------------
TRemoteDirectoryCache::~TRemoteDirectoryCache()
{
  Clear();
  delete FSection;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::Clear()
{
  TGuard Guard(FSection);

  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->TStringList::Clear();
    } BOOST_SCOPE_EXIT_END
    for (int Index = 0; Index < GetCount(); Index++)
    {
      delete (TRemoteFileList *)GetObject(Index);
      PutObject(Index, NULL);
    }
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetIsEmpty() const
{
  TGuard Guard(FSection);

  return (const_cast<TRemoteDirectoryCache*>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasFileList(const std::wstring Directory)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory).c_str());
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasNewerFileList(const std::wstring Directory,
  TDateTime Timestamp)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory).c_str());
  if (Index >= 0)
  {
    TRemoteFileList * FileList = reinterpret_cast<TRemoteFileList *>(GetObject(Index));
    if (FileList->GetTimestamp() <= Timestamp)
    {
      Index = -1;
    }
  }
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetFileList(const std::wstring Directory,
  TRemoteFileList * FileList)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory).c_str());
  bool Result = (Index >= 0);
  if (Result)
  {
    assert(GetObject(Index) != NULL);
    reinterpret_cast<TRemoteFileList *>(GetObject(Index))->DuplicateTo(FileList);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::AddFileList(TRemoteFileList * FileList)
{
  assert(FileList);
  TRemoteFileList * Copy = new TRemoteFileList();
  FileList->DuplicateTo(Copy);

  {
    TGuard Guard(FSection);

    // file list cannot be cached already with only one thread, but it can be
    // when directory is loaded by secondary terminal
    DoClearFileList(FileList->GetDirectory(), false);
    AddObject(Copy->GetDirectory(), Copy);
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::ClearFileList(std::wstring Directory, bool SubDirs)
{
  TGuard Guard(FSection);
  DoClearFileList(Directory, SubDirs);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::DoClearFileList(std::wstring Directory, bool SubDirs)
{
  Directory = UnixExcludeTrailingBackslash(Directory);
  int Index = IndexOf(Directory.c_str());
  if (Index >= 0)
  {
    Delete(Index);
  }
  if (SubDirs)
  {
    Directory = UnixIncludeTrailingBackslash(Directory);
    Index = GetCount()-1;
    while (Index >= 0)
    {
      if (GetString(Index).substr(0, Directory.size()) == Directory)
      {
        Delete(Index);
      }
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::Delete(int Index)
{
  delete (TRemoteFileList *)GetObject(Index);
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryChangesCache::TRemoteDirectoryChangesCache(int MaxSize) :
  TStringList(),
  FMaxSize(MaxSize)
{
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Clear()
{
  TStringList::Clear();
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetIsEmpty() const
{
  return (const_cast<TRemoteDirectoryChangesCache*>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::SetValue(const std::wstring & Name,
  const std::wstring & Value)
{
  int Index = IndexOfName(Name.c_str());
  if (Index > 0)
  {
    Delete(Index);
  }
  TStringList::SetValue(Name, Value);
}
//---------------------------------------------------------------------------
std::wstring TRemoteDirectoryChangesCache::GetValue(const std::wstring & Name)
{
  std::wstring Value = TStringList::GetValue(Name);
  TStringList::SetValue(Name, Value);
  return Value;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::AddDirectoryChange(
  const std::wstring SourceDir, const std::wstring Change,
  const std::wstring TargetDir)
{
  assert(!TargetDir.empty());
  SetValue(TargetDir, L"//");
  if (TTerminal::ExpandFileName(Change, SourceDir) != TargetDir)
  {
    std::wstring Key;
    if (DirectoryChangeKey(SourceDir, Change, Key))
    {
      SetValue(Key, TargetDir);
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChange(
  std::wstring SourceDir)
{
  for (int Index = 0; Index < GetCount(); Index++)
  {
    if (GetName(Index).substr(0, SourceDir.size()) == SourceDir)
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChangeTarget(
  std::wstring TargetDir)
{
  std::wstring Key;
  // hack to clear at least local sym-link change in case symlink is deleted
  DirectoryChangeKey(UnixExcludeTrailingBackslash(UnixExtractFilePath(TargetDir)),
    UnixExtractFileName(TargetDir), Key);

  for (int Index = 0; Index < GetCount(); Index++)
  {
    std::wstring Name = GetName(Index);
    if ((Name.substr(0, TargetDir.size()) == TargetDir) ||
        (GetValue(Name).substr(0, TargetDir.size()) == TargetDir) ||
        (!Key.empty() && (Name == Key)))
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetDirectoryChange(
  const std::wstring SourceDir, const std::wstring Change, std::wstring & TargetDir)
{
  std::wstring Key;
  bool Result;
  Key = TTerminal::ExpandFileName(Change, SourceDir);
  Result = (IndexOfName(Key.c_str()) >= 0);
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
      std::wstring Directory = GetValue(Key);
      Result = !Directory.empty();
      if (Result)
      {
        TargetDir = Directory;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Serialize(std::wstring & Data)
{
  Data = L"A";
  int ACount = GetCount();
  if (ACount > FMaxSize)
  {
    TStrings * Limited = new TStringList();
    {
      BOOST_SCOPE_EXIT ( (&Limited) )
      {
        delete Limited;
      } BOOST_SCOPE_EXIT_END
      int Index = ACount - FMaxSize;
      while (Index < ACount)
      {
        Limited->Add(GetString(Index));
        Index++;
      }
      Data += Limited->GetText();
    }
  }
  else
  {
    Data += GetText();
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Deserialize(const std::wstring Data)
{
  // DEBUG_PRINTF(L"Data = %s", Data.c_str());
  if (Data.empty())
  {
    SetText(L"");
  }
  else
  {
    SetText(Data.c_str() + 1);
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::DirectoryChangeKey(
  const std::wstring SourceDir, const std::wstring Change, std::wstring & Key)
{
  bool Result = !Change.empty();
  if (Result)
  {
    bool Absolute = TTerminal::IsAbsolutePath(Change);
    Result = !SourceDir.empty() || Absolute;
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
const char TRights::BasicSymbols[] = "rwxrwxrwx";
const char TRights::CombinedSymbols[] = "--s--s--t";
const char TRights::ExtendedSymbols[] = "--S--S--T";
const char TRights::ModeGroups[] = "ugo";
//---------------------------------------------------------------------------
TRights::TRights()
{
  FAllowUndef = false;
  FSet = 0;
  // DEBUG_PRINTF(L"FSet = %o", FSet);
  FUnset = 0;
  SetNumber(0);
  FUnknown = true;
}
//---------------------------------------------------------------------------
TRights::TRights(unsigned short ANumber)
{
  FAllowUndef = false;
  FSet = 0;
  // DEBUG_PRINTF(L"FSet = %o", FSet);
  FUnset = 0;
  SetNumber(ANumber);
}
//---------------------------------------------------------------------------
TRights::TRights(const TRights & Source)
{
  Assign(&Source);
}
//---------------------------------------------------------------------------
void TRights::Assign(const TRights * Source)
{
  FAllowUndef = Source->GetAllowUndef();
  // DEBUG_PRINTF(L"FSet = %o, Source->FSet = %o", FSet, Source->FSet);
  FSet = Source->FSet;
  FUnset = Source->FUnset;
  FText = Source->FText;
  FUnknown = Source->FUnknown;
}
//---------------------------------------------------------------------------
TRights::TFlag TRights::RightToFlag(TRights::TRight Right)
{
  return static_cast<TFlag>(1 << (rrLast - Right));
}
//---------------------------------------------------------------------------
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
    return (GetNumber() == rhr.GetNumber());
  }
}
//---------------------------------------------------------------------------
bool TRights::operator ==(unsigned short rhr) const
{
  return (GetNumber() == rhr);
}
//---------------------------------------------------------------------------
bool TRights::operator !=(const TRights & rhr) const
{
  return !(*this == rhr);
}
//---------------------------------------------------------------------------
TRights & TRights::operator =(unsigned short rhr)
{
  SetNumber(rhr);
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator =(const TRights & rhr)
{
  Assign(&rhr);
  return *this;
}
//---------------------------------------------------------------------------
TRights TRights::operator ~() const
{
  TRights Result(static_cast<unsigned short>(~GetNumber()));
  return Result;
}
//---------------------------------------------------------------------------
TRights TRights::operator &(const TRights & rhr) const
{
  TRights Result(*this);
  Result &= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights TRights::operator &(unsigned short rhr) const
{
  TRights Result(*this);
  Result &= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights & TRights::operator &=(const TRights & rhr)
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
//---------------------------------------------------------------------------
TRights & TRights::operator &=(unsigned short rhr)
{
  SetNumber(GetNumber() & rhr);
  return *this;
}
//---------------------------------------------------------------------------
TRights TRights::operator |(const TRights & rhr) const
{
  TRights Result(*this);
  Result |= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights TRights::operator |(unsigned short rhr) const
{
  TRights Result(*this);
  Result |= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights & TRights::operator |=(const TRights & rhr)
{
  SetNumber(GetNumber() | rhr.GetNumber());
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator |=(unsigned short rhr)
{
  SetNumber(GetNumber() | rhr);
  return *this;
}
//---------------------------------------------------------------------------
void TRights::SetAllowUndef(bool value)
{
  if (FAllowUndef != value)
  {
    assert(!value || ((FSet | FUnset) == rfAllSpecials));
    FAllowUndef = value;
  }
}
//---------------------------------------------------------------------------
void TRights::SetText(const std::wstring & value)
{
  if (value != GetText())
  {
    // DEBUG_PRINTF(L"value = %s, GetText = %s", value.c_str(), GetText().c_str());
    if ((value.size() != TextLen) ||
        (!GetAllowUndef() && (value.find(UndefSymbol) != std::wstring::npos)) ||
        (value.find(L" ") != std::wstring::npos))
    {
      throw ExtException(FMTLOAD(RIGHTS_ERROR, value.c_str()));
    }

    FSet = 0;
    // DEBUG_PRINTF(L"FSet = %o", FSet);
    FUnset = 0;
    int Flag = 00001;
    int ExtendedFlag = 01000;
    bool KeepText = false;
    std::string val = ::W2MB(value.c_str());
    for (int i = TextLen - 1; i >= 0; i--)
    {
      if (val[i] == UnsetSymbol)
      {
        FUnset |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (val[i] == UndefSymbol)
      {
        // do nothing
      }
      else if (val[i] == CombinedSymbols[i - 1])
      {
        FSet |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (val[i] == ExtendedSymbols[i - 1])
      {
        FSet |= static_cast<unsigned short>(ExtendedFlag);
        FUnset |= static_cast<unsigned short>(Flag);
      }
      else
      {
        if (val[i] != BasicSymbols[i - 1])
        {
          KeepText = true;
        }
        FSet |= static_cast<unsigned short>(Flag);
        if ((i + 1) % 3 == 0)
        {
          FUnset |= static_cast<unsigned short>(ExtendedFlag);
        }
      }

      Flag <<= 1;
      if ((i + 1) % 3 == 1)
      {
        ExtendedFlag <<= 1;
      }
    }

    FText = KeepText ? value : std::wstring();
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
std::wstring TRights::GetText() const
{
  // DEBUG_PRINTF(L"FSet = %o, FText = %s", FSet, FText.c_str());
  if (!FText.empty())
  {
    return FText;
  }
  else
  {
    std::string Result;
    Result.resize(TextLen);

    int Flag = 00001;
    int ExtendedFlag = 01000;
    bool ExtendedPos = true;
    char Symbol;
    int i = TextLen - 1;
    while (i >= 0)
    {
      // DEBUG_PRINTF(L"FSet = %o, Flag = %o", FSet, Flag);
      if (ExtendedPos &&
          ((FSet & (Flag | ExtendedFlag)) == (Flag | ExtendedFlag)))
      {
        Symbol = CombinedSymbols[i];
      }
      else if ((FSet & Flag) != 0)
      {
        Symbol = BasicSymbols[i];
      }
      else if (ExtendedPos && ((FSet & ExtendedFlag) != 0))
      {
        Symbol = ExtendedSymbols[i];
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

      // DEBUG_PRINTF(L"Symbol = %c", Symbol);
      Result[i] = Symbol;

      Flag <<= 1;
      i--;
      ExtendedPos = (((i + 1) % 3) == 0);
      if (ExtendedPos)
      {
        ExtendedFlag <<= 1;
      }
    }
    // DEBUG_PRINTF(L"Result = %s", ::MB2W(Result.c_str()).c_str());
    return ::MB2W(Result.c_str());
  }
}
//---------------------------------------------------------------------------
void TRights::SetOctal(std::wstring value)
{
  std::string AValue(::W2MB(value.c_str()));
  if (AValue.size() == 3)
  {
    AValue = "0" + AValue;
  }

  if (GetOctal() != ::MB2W(AValue.c_str()))
  {
    bool Correct = (AValue.size() == 4);
    if (Correct)
    {
      for (int i = 0; (i <= AValue.size()) && Correct; i++)
      {
        Correct = (AValue[i] >= '0') && (AValue[i] <= '7');
      }
    }

    if (!Correct)
    {
      throw ExtException(FMTLOAD(INVALID_OCTAL_PERMISSIONS, value.c_str()));
    }

    SetNumber(static_cast<unsigned short>(
      ((AValue[0] - '0') << 9) +
      ((AValue[1] - '0') << 6) +
      ((AValue[2] - '0') << 3) +
      ((AValue[3] - '0') << 0)));
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
unsigned long TRights::GetNumberDecadic() const
{
  unsigned long N = GetNumberSet(); // used to be "Number"
  unsigned long Result =
      ((N & 07000) / 01000 * 1000) +
      ((N & 00700) /  0100 *  100) +
      ((N & 00070) /   010 *   10) +
      ((N & 00007) /    01 *    1);

  return Result;
}
//---------------------------------------------------------------------------
std::wstring TRights::GetOctal() const
{
  std::string Result;
  unsigned short N = GetNumberSet(); // used to be "Number"
  Result.resize(4);
  Result[0] = static_cast<char>('0' + ((N & 07000) >> 9));
  Result[1] = static_cast<char>('0' + ((N & 00700) >> 6));
  Result[2] = static_cast<char>('0' + ((N & 00070) >> 3));
  Result[3] = static_cast<char>('0' + ((N & 00007) >> 0));

  return ::MB2W(Result.c_str());
}
//---------------------------------------------------------------------------
void TRights::SetNumber(unsigned short value)
{
  if ((FSet != value) || ((FSet | FUnset) != rfAllSpecials))
  {
    FSet = value;
    FUnset = static_cast<unsigned short>(rfAllSpecials & ~FSet);
    FText = L"";
  }
  // DEBUG_PRINTF(L"FSet = %o, value = %o", FSet, value);
  FUnknown = false;
}
//---------------------------------------------------------------------------
unsigned short TRights::GetNumber() const
{
  assert(!GetIsUndef());
  return FSet;
}
//---------------------------------------------------------------------------
void TRights::SetRight(TRight Right, bool value)
{
  SetRightUndef(Right, (value ? rsYes : rsNo));
}
//---------------------------------------------------------------------------
bool TRights::GetRight(TRight Right) const
{
  TState State = GetRightUndef(Right);
  assert(State != rsUndef);
  return (State == rsYes);
}
//---------------------------------------------------------------------------
void TRights::SetRightUndef(TRight Right, TState value)
{
  if (value != GetRightUndef(Right))
  {
    assert((value != rsUndef) || GetAllowUndef());

    TFlag Flag = RightToFlag(Right);

    switch (value)
    {
      case rsYes:
        FSet |= Flag;
        FUnset &= ~Flag;
        break;

      case rsNo:
        FSet &= ~Flag;
        FUnset |= Flag;
        break;

      case rsUndef:
      default:
        FSet &= ~Flag;
        FUnset &= ~Flag;
        break;
    }

    FText = L"";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void TRights::SetReadOnly(bool value)
{
  SetRight(rrUserWrite, !value);
  SetRight(rrGroupWrite, !value);
  SetRight(rrOtherWrite, !value);
}
//---------------------------------------------------------------------------
bool  TRights::GetReadOnly()
{
  return GetRight(rrUserWrite) && GetRight(rrGroupWrite) && GetRight(rrOtherWrite);
}
//---------------------------------------------------------------------------
std::wstring TRights::GetSimplestStr() const
{
  if (GetIsUndef())
  {
    return GetModeStr();
  }
  else
  {
    return GetOctal();
  }
}
//---------------------------------------------------------------------------
std::wstring TRights::GetModeStr() const
{
  std::wstring Result;
  std::wstring SetModeStr, UnsetModeStr;
  TRight Right;
  int Index;

  for (int Group = 0; Group < 3; Group++)
  {
    SetModeStr = L"";
    UnsetModeStr = L"";
    for (int Mode = 0; Mode < 3; Mode++)
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
    }

    if (!SetModeStr.empty() || !UnsetModeStr.empty())
    {
      if (!Result.empty())
      {
        Result += ',';
      }
      Result += ModeGroups[Group];
      if (!SetModeStr.empty())
      {
        Result += L"+" + SetModeStr;
      }
      if (!UnsetModeStr.empty())
      {
        Result += L"-" + UnsetModeStr;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRights::AddExecute()
{
  for (int Group = 0; Group < 3; Group++)
  {
    if ((GetRightUndef(static_cast<TRight>(rrUserRead + (Group * 3))) == rsYes) ||
        (GetRightUndef(static_cast<TRight>(rrUserWrite + (Group * 3))) == rsYes))
    {
      SetRight(static_cast<TRight>(rrUserExec + (Group * 3)), true);
    }
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
void TRights::AllUndef()
{
  if ((FSet != 0) || (FUnset != 0))
  {
    FSet = 0;
    // DEBUG_PRINTF(L"FSet = %o", FSet);
    FUnset = 0;
    FText = L"";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
bool TRights::GetIsUndef() const
{
  return ((FSet | FUnset) != rfAllSpecials);
}
//---------------------------------------------------------------------------
TRights::operator unsigned short() const
{
  return GetNumber();
}
//---------------------------------------------------------------------------
TRights::operator unsigned long() const
{
  return GetNumber();
}
//=== TRemoteProperties -------------------------------------------------------
TRemoteProperties::TRemoteProperties()
{
  Default();
}
//---------------------------------------------------------------------------
TRemoteProperties::TRemoteProperties(const TRemoteProperties & rhp) :
  Valid(rhp.Valid),
  Recursive(rhp.Recursive),
  Rights(rhp.Rights),
  AddXToDirectories(rhp.AddXToDirectories),
  Group(rhp.Group),
  Owner(rhp.Owner),
  Modification(rhp.Modification),
  LastAccess(rhp.Modification)
{
}
//---------------------------------------------------------------------------
void TRemoteProperties::Default()
{
  Valid.Clear();
  AddXToDirectories = false;
  Rights.SetAllowUndef(false);
  Rights.SetNumber(0);
  Group.Clear();
  Owner.Clear();
  Recursive = false;
}
//---------------------------------------------------------------------------
bool TRemoteProperties::operator ==(const TRemoteProperties & rhp) const
{
  bool Result = (Valid == rhp.Valid && Recursive == rhp.Recursive);

  if (Result)
  {
    if ((Valid.Contains(vpRights) &&
          (Rights != rhp.Rights || AddXToDirectories != rhp.AddXToDirectories)) ||
        (Valid.Contains(vpOwner) && (Owner != rhp.Owner)) ||
        (Valid.Contains(vpGroup) && (Group != rhp.Group)) ||
        (Valid.Contains(vpModification) && (Modification != rhp.Modification)) ||
        (Valid.Contains(vpLastAccess) && (LastAccess != rhp.LastAccess)))
    {
      Result = false;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TRemoteProperties::operator !=(const TRemoteProperties & rhp) const
{
  return !(*this == rhp);
}
//---------------------------------------------------------------------------
TRemoteProperties TRemoteProperties::CommonProperties(TStrings * FileList)
{
  // TODO: Modification and LastAccess
  TRemoteProperties CommonProperties;
  for (int Index = 0; Index < FileList->GetCount(); Index++)
  {
    TRemoteFile * File = (TRemoteFile *)(FileList->GetObject(Index));
    assert(File);
    if (!Index)
    {
      CommonProperties.Rights = *(File->GetRights());
      // previously we allowed undef implicitly for directories,
      // now we do it explicitly in properties dialog and only in combination
      // with "recursive" option
      CommonProperties.Rights.SetAllowUndef(File->GetRights()->GetIsUndef());
      CommonProperties.Valid << vpRights;
      if (File->GetOwner().GetIsSet())
      {
        CommonProperties.Owner = File->GetOwner();
        CommonProperties.Valid << vpOwner;
      }
      if (File->GetGroup().GetIsSet())
      {
        CommonProperties.Group = File->GetGroup();
        CommonProperties.Valid << vpGroup;
      }
    }
    else
    {
      CommonProperties.Rights.SetAllowUndef(true);
      CommonProperties.Rights &= *File->GetRights();
      if (CommonProperties.Owner != File->GetOwner())
      {
        CommonProperties.Owner.Clear();
        CommonProperties.Valid >> vpOwner;
      };
      if (CommonProperties.Group != File->GetGroup())
      {
        CommonProperties.Group.Clear();
        CommonProperties.Valid >> vpGroup;
      };
    }
  }
  return CommonProperties;
}
//---------------------------------------------------------------------------
TRemoteProperties TRemoteProperties::ChangedProperties(
  const TRemoteProperties & OriginalProperties, TRemoteProperties NewProperties)
{
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
//---------------------------------------------------------------------------
void TRemoteProperties::Load(THierarchicalStorage * Storage)
{
  unsigned char Buf[sizeof(Valid)];
  if (Storage->ReadBinaryData(L"Valid", &Buf, sizeof(Buf)) == sizeof(Buf))
  {
    memcpy(&Valid, Buf, sizeof(Valid));
  }

  if (Valid.Contains(vpRights))
  {
    Rights.SetText(Storage->ReadString(L"Rights", Rights.GetText()));
  }

  // TODO
}
//---------------------------------------------------------------------------
void TRemoteProperties::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteBinaryData(std::wstring(L"Valid"),
    static_cast<const void *>(&Valid), sizeof(Valid));

  if (Valid.Contains(vpRights))
  {
    Storage->WriteString(L"Rights", Rights.GetText());
  }

  // TODO
}
