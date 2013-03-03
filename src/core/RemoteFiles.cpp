//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define TRACE_IMAGEINDEX NOTRACING

#include "RemoteFiles.h"

#include <SysUtils.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
/* TODO 1 : Path class instead of UnicodeString (handle relativity...) */
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
// Keeps "/" for root path
UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & Path)
{
  if ((Path.Length() > 1) && Path.IsDelimiter(L"/", Path.Length()))
      return Path.SubString(1, Path.Length() - 1);
    else return Path;
}
//---------------------------------------------------------------------------
Boolean UnixComparePaths(const UnicodeString & Path1, const UnicodeString & Path2)
{
  return (UnixIncludeTrailingBackslash(Path1) == UnixIncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool UnixIsChildPath(const UnicodeString & Parent, const UnicodeString & Child)
{
  UnicodeString Parent2 = UnixIncludeTrailingBackslash(Parent);
  UnicodeString Child2 = UnixIncludeTrailingBackslash(Child);
  return (Child2.SubString(1, Parent2.Length()) == Parent2);
}
//---------------------------------------------------------------------------
UnicodeString UnixExtractFileDir(const UnicodeString & Path)
{
  intptr_t Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return Path.SubString(1, Pos - 1);
  }
  else
  {
    return (Pos == 1) ? UnicodeString(L"/") : UnicodeString();
  }
}
//---------------------------------------------------------------------------
// must return trailing backslash
UnicodeString UnixExtractFilePath(const UnicodeString & Path)
{
  intptr_t Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  return (Pos > 0) ? Path.SubString(1, Pos) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString UnixExtractFileName(const UnicodeString & Path)
{
  intptr_t Pos = Path.LastDelimiter(L'/');
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
//---------------------------------------------------------------------------
UnicodeString UnixExtractFileExt(const UnicodeString & Path)
{
  UnicodeString FileName = UnixExtractFileName(Path);
  intptr_t Pos = FileName.LastDelimiter(L".");
  return (Pos > 0) ? Path.SubString(Pos, Path.Length() - Pos + 1) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileName(const UnicodeString & Path, bool Unix)
{
  if (Unix)
  {
    return UnixExtractFileName(Path);
  }
  else
  {
    return ExtractFilename(Path, L'\\');
  }
}
//---------------------------------------------------------------------------
bool ExtractCommonPath(TStrings * Files, UnicodeString & Path)
{
  assert(Files->GetCount() > 0);

  Path = ExtractFilePath(Files->Strings[0]);
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (intptr_t Index = 1; Index < Files->GetCount(); ++Index)
    {
      while (!Path.IsEmpty() &&
        (Files->Strings[Index].SubString(1, Path.Length()) != Path))
      {
        intptr_t PrevLen = Path.Length();
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
//---------------------------------------------------------------------------
bool UnixExtractCommonPath(TStrings * Files, UnicodeString & Path)
{
  assert(Files->GetCount() > 0);

  Path = UnixExtractFilePath(Files->Strings[0]);
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (intptr_t Index = 1; Index < Files->GetCount(); ++Index)
    {
      while (!Path.IsEmpty() &&
        (Files->Strings[Index].SubString(1, Path.Length()) != Path))
      {
        intptr_t PrevLen = Path.Length();
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
//---------------------------------------------------------------------------
bool IsUnixRootPath(const UnicodeString & Path)
{
  return Path.IsEmpty() || (Path == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
bool IsUnixHiddenFile(const UnicodeString & FileName)
{
  return (FileName != ROOTDIRECTORY) && (FileName != PARENTDIRECTORY) &&
    !FileName.IsEmpty() && (FileName[1] == L'.');
}
//---------------------------------------------------------------------------
UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & Path)
{
  UnicodeString Result;
  if (Path.IsEmpty())
  {
    Result = Base;
  }
  else if (Path[1] == L'/')
  {
    Result = ::UnixExcludeTrailingBackslash(Path);
  }
  else
  {
    Result = ::UnixIncludeTrailingBackslash(
      ::UnixIncludeTrailingBackslash(Base) + Path);
    intptr_t P;
    while ((P = Result.Pos(L"/../")) > 0)
    {
      intptr_t P2 = Result.SubString(1, P-1).LastDelimiter(L"/");
      assert(P2 > 0);
      Result.Delete(P2, P - P2 + 3);
    }
    while ((P = Result.Pos(L"/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = ::UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FromUnixPath(const UnicodeString & Path)
{
  return StringReplace(Path, L"/", L"\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
UnicodeString ToUnixPath(const UnicodeString & Path)
{
  return StringReplace(Path, L"\\", L"/", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
static void CutFirstDirectory(UnicodeString & S, bool Unix)
{
  UnicodeString Sep = Unix ? L"/" : L"\\";
  if (S == Sep)
  {
    S = L"";
  }
  else
  {
    bool Root = false;
    intptr_t P = 0;
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
UnicodeString MinimizeName(const UnicodeString & FileName, intptr_t MaxLen, bool Unix)
{
  UnicodeString Drive, Dir, Name, Result;
  UnicodeString Sep = Unix ? L"/" : L"\\";

  Result = FileName;
  if (Unix)
  {
    intptr_t P = Result.LastDelimiter(L"/");
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
    Name = ExtractFileName(Result, false);

    if (Dir.Length() >= 2 && Dir[2] == L':')
    {
      Drive = Dir.SubString(1, 2);
      Dir.Delete(1, 2);
    }
  }

  while ((!Dir.IsEmpty() || !Drive.IsEmpty()) && (Result.Length() > MaxLen))
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

  if (Result.Length() > MaxLen)
  {
    Result = Result.SubString(1, MaxLen);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString MakeFileList(TStrings * FileList)
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
  {
    if (!Result.IsEmpty())
    {
      Result += L" ";
    }

    UnicodeString FileName = FileList->Strings[Index];
    // currently this is used for local file only, so no delimiting is done
    if (FileName.Pos(L" ") > 0)
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
    DateTime = double(0.0);
  }
  else if (Precision != mfFull)
  {
    unsigned short Y, M, D, H, N, S, MS;

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
UnicodeString UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision)
{
  unsigned short Y, M, D, H, N, S, MS;
  DateTime.DecodeDate(Y, M, D);
  DateTime.DecodeTime(H, N, S, MS);
  switch (Precision)
  {
    case mfNone:
      return L"";
    case mfMDY:
      // return FormatDateTime(L"ddddd", DateTime);
      return FORMAT(L"%02d.%02d.%04d", D, M, Y);
    case mfMDHM:
      // return FormatDateTime(L"ddddd t", DateTime);
      return FORMAT(L"%02d.%02d.%04d %02d:%02d", D, M, Y, H, N);
    case mfFull:
    default:
      // return FormatDateTime(L"ddddd tt", DateTime);
      return FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d", D, M, Y, H, N, S);
  }
}
//---------------------------------------------------------------------------
int FakeFileImageIndex(UnicodeString FileName, unsigned long Attrs,
  UnicodeString * TypeName)
{
  /*CCALLSTACK(TRACE_IMAGEINDEX);
  Attrs |= FILE_ATTRIBUTE_NORMAL;

  CTRACE(TRACE_IMAGEINDEX, "FakeFileImageIndex 1");
  TSHFileInfoW SHFileInfo = {0};
  // On Win2k we get icon of "ZIP drive" for ".." (parent directory)
  if ((FileName == L"..") ||
      ((FileName.Length() == 2) && (FileName[2] == L':') &&
       (towlower(FileName[1]) >= L'a') && (towlower(FileName[1]) <= L'z')) ||
      IsReservedName(FileName))
  {
    FileName = L"dumb";
  }
  // this should be somewhere else, probably in TUnixDirView,
  // as the "partial" overlay is added there too
  if (AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT))
  {
    static const size_t PartialExtLen = LENOF(PARTIAL_EXT) - 1;
    FileName.SetLength(FileName.Length() - PartialExtLen);
  }

  CTRACEFMT(TRACE_IMAGEINDEX, "FakeFileImageIndex 2 [%s] [%d]", FileName.c_str(), int(Attrs));
  int Icon;
  if (SHGetFileInfo(UnicodeString(FileName).c_str(),
        Attrs, &SHFileInfo, sizeof(SHFileInfo),
        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME) != 0)
  {
    CTRACE(TRACE_IMAGEINDEX, "FakeFileImageIndex 2");
    if (TypeName != NULL)
    {
      *TypeName = SHFileInfo.szTypeName;
    }
    Icon = SHFileInfo.iIcon;
  }
  else
  {
    CTRACE(TRACE_IMAGEINDEX, "FakeFileImageIndex 3");
    if (TypeName != NULL)
    {
      *TypeName = L"";
    }
    Icon = -1;
  }
  CTRACEFMT(TRACE_IMAGEINDEX, "FakeFileImageIndex 4 [%d]", Icon);

  return Icon;*/
  return -1;
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken() :
  FID(0),
  FIDValid(false)
{
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken(const UnicodeString & Name) :
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
  if (!FName.IsEmpty())
  {
    if (!rht.FName.IsEmpty())
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
    if (!rht.FName.IsEmpty())
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
void TRemoteToken::SetID(intptr_t Value)
{
  FID = Value;
  FIDValid = true;
}
//---------------------------------------------------------------------------
bool TRemoteToken::GetNameValid() const
{
  return !FName.IsEmpty();
}
//---------------------------------------------------------------------------
bool TRemoteToken::GetIsSet() const
{
  return !FName.IsEmpty() || FIDValid;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteToken::GetDisplayText() const
{
  if (!FName.IsEmpty())
  {
    return FName;
  }
  else if (FIDValid)
  {
    return IntToStr(FID);
  }
  else
  {
    return UnicodeString();
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteToken::GetLogText() const
{
  return FORMAT(L"\"%s\" [%d]", FName.c_str(), static_cast<int>(FID));
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
    // std::pair<TIDMap::iterator, bool> Position =
      FIDMap.insert(TIDMap::value_type(Token.GetID(), FTokens.size() - 1));
  }
  if (Token.GetNameValid())
  {
    // std::pair<TNameMap::iterator, bool> Position =
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
    // can happen, e.g. with winsshd/SFTP
  }
}
//---------------------------------------------------------------------------
bool TRemoteTokenList::Exists(const UnicodeString & Name) const
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
const TRemoteToken * TRemoteTokenList::Find(const UnicodeString & Name) const
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
void TRemoteTokenList::Log(TTerminal * Terminal, const wchar_t * Title)
{
  if (!FTokens.empty())
  {
    Terminal->LogEvent(FORMAT(L"Following %s found:", Title));
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(FTokens.size()); ++Index)
    {
      Terminal->LogEvent(UnicodeString(L"  ") + FTokens[Index].GetLogText());
    }
  }
  else
  {
    Terminal->LogEvent(FORMAT(L"No %s found.", Title));
  }
}
//---------------------------------------------------------------------------
intptr_t TRemoteTokenList::GetCount() const
{
  return static_cast<intptr_t>(FTokens.size());
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Token(intptr_t Index) const
{
  return &FTokens[Index];
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteFile::TRemoteFile(TRemoteFile * ALinkedByFile):
  TPersistent(),
  FDirectory(NULL),
  FSize(0),
  FINodeBlocks(0),
  FIconIndex(0),
  FIsSymLink(false),
  FLinkedFile(NULL),
  FLinkedByFile(NULL),
  FRights(NULL),
  FTerminal(NULL),
  FType(0),
  FSelected(false),
  FCyclicLink(false),
  FIsHidden(0)
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
  CALLSTACK;
  TRemoteFile * Result;
  Result = new TRemoteFile();
  try
  {
    TRACE("1");
    if (FLinkedFile)
    {
      TRACE("2");
      Result->FLinkedFile = FLinkedFile->Duplicate(true);
      Result->FLinkedFile->FLinkedByFile = Result;
    }
    Result->SetRights(FRights);
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
    if (Standalone && (!FFullFileName.IsEmpty() || (GetDirectory() != NULL)))
    {
      TRACE("3");
      Result->FFullFileName = GetFullFileName();
    }
  }
  catch(...)
  {
    TRACE("4");
    delete Result;
    throw;
  }
  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::LoadTypeInfo()
{
  CCALLSTACK(TRACE_IMAGEINDEX);
  /* TODO : If file is link: Should be attributes taken from linked file? */
  /* unsigned long Attrs = 0;
  if (GetIsDirectory()) { Attrs |= FILE_ATTRIBUTE_DIRECTORY; }
  if (GetIsHidden()) { Attrs |= FILE_ATTRIBUTE_HIDDEN; }

  UnicodeString DumbFileName = (GetIsSymLink() && !GetLinkTo().IsEmpty() ? GetLinkTo() : GetFileName());

  FIconIndex = FakeFileImageIndex(DumbFileName, Attrs, &FTypeName); */
}
//---------------------------------------------------------------------------
intptr_t TRemoteFile::GetIconIndex() const
{
  CCALLSTACK(TRACE_IMAGEINDEX);
  if (FIconIndex == -1)
  {
    const_cast<TRemoteFile *>(this)->LoadTypeInfo();
  }
  return FIconIndex;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetTypeName()
{
  // check avilability of type info by icon index, because type name can be empty
  if (FIconIndex < 0)
  {
    LoadTypeInfo();
  }
  return FTypeName;
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsHidden()
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
void TRemoteFile::SetIsHidden(bool Value)
{
  FIsHidden = Value ? 1 : 0;
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsDirectory() const
{
  return (UpCase(GetType()) == FILETYPE_DIRECTORY);
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsThisDirectory() const
{
  return (GetFileName() == THISDIRECTORY);
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsInaccesibleDirectory() const
{
  Boolean Result;
  if (GetIsDirectory())
  {
    assert(GetTerminal());
    Result = !
       (((GetRights()->GetRightUndef(TRights::rrOtherExec) != TRights::rsNo)) ||
        ((GetRights()->GetRight(TRights::rrGroupExec) != TRights::rsNo) &&
         GetTerminal()->GetMembership()->Exists(GetFileGroup().GetName())) ||
        ((GetRights()->GetRight(TRights::rrUserExec) != TRights::rsNo) &&
         (AnsiCompareText(GetTerminal()->GetUserName(), GetFileOwner().GetName()) == 0)));
  }
  else
  {
    Result = False;
  }
  return Result;
}
//---------------------------------------------------------------------------
wchar_t TRemoteFile::GetType() const
{
  if (GetIsSymLink() && FLinkedFile)
  {
    return FLinkedFile->GetType();
  }
  else
  {
     return FType;
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::SetType(wchar_t AType)
{
  FType = AType;
  FIsSymLink = (static_cast<wchar_t>(towupper(FType)) == FILETYPE_SYMLINK);
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFile::GetLinkedFile()
{
  // do not call FindLinkedFile as it would be called releatedly for broken symlinks
  return FLinkedFile;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetLinkedFile(TRemoteFile * Value)
{
  if (FLinkedFile != Value)
  {
    if (FLinkedFile) { delete FLinkedFile; }
    FLinkedFile = Value;
  }
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetBrokenLink()
{
  assert(GetTerminal());
  // If file is symlink but we couldn't find linked file we assume broken link
  return (GetIsSymLink() && (FCyclicLink || !FLinkedFile) &&
    GetTerminal()->GetResolvingSymlinks());
  // "!FLinkTo.IsEmpty()" removed because it does not work with SFTP
}
//---------------------------------------------------------------------------
void TRemoteFile::ShiftTime(const TDateTime & Difference)
{
  double D = static_cast<double>(Difference.operator double());
  if ((abs(D) > std::numeric_limits<double>::epsilon()) && (FModificationFmt != mfMDY))
  {
    assert(static_cast<int>(FModification) != 0);
    FModification = static_cast<double>(FModification) + D;
    assert(static_cast<int>(FLastAccess) != 0);
    FLastAccess = static_cast<double>(FLastAccess) + D;
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::SetModification(const TDateTime & Value)
{
  if (FModification != Value)
  {
    FModificationFmt = mfFull;
    FModification = Value;
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetUserModificationStr()
{
  return ::UserModificationStr(GetModification(), FModificationFmt);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetModificationStr()
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
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
UnicodeString TRemoteFile::GetExtension()
{
  return UnixExtractFileExt(FFileName);
}
//---------------------------------------------------------------------------
void TRemoteFile::SetRights(TRights * Value)
{
  FRights->Assign(Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetRightsStr()
{
  return FRights->GetUnknown() ? UnicodeString() : FRights->GetText();
}
//---------------------------------------------------------------------------
void TRemoteFile::SetListingStr(const UnicodeString & Value)
{
  CALLSTACK;
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

    #define GETNCOL  \
      { if (Line.IsEmpty()) throw Exception(L""); \
        intptr_t P = Line.Pos(L' '); \
        if (P) { Col = Line; Col.SetLength(P-1); Line.Delete(1, P); } \
          else { Col = Line; Line = L""; } \
      }
    #define GETCOL { GETNCOL; Line = TrimLeft(Line); }

    // Rights string may contain special permission attributes (S,t, ...)
    // (TODO: maybe no longer necessary, once we can handle the special permissions)
    GetRights()->SetAllowUndef(True);
    // On some system there is no space between permissions and node blocks count columns
    // so we get only first 9 characters and trim all following spaces (if any)
    GetRights()->SetText(Line.SubString(1, 9));
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

    GETCOL;
    if (!TryStrToInt(Col, FINodeBlocks))
    {
      // if the column is not an integer, suppose it's owner
      // (Android BusyBox)
      FINodeBlocks = 0;
    }
    else
    {
      GETCOL;
    }

    FOwner.SetName(Col);

    // #60 17.10.01: group name can contain space
    FGroup.SetName(L"");
    GETCOL;
    __int64 ASize;
    do
    {
      FGroup.SetName(FGroup.GetName() + Col);
      GETCOL;
      assert(!Col.IsEmpty());
      // for devices etc.. there is additional column ending by comma, we ignore it
      if (Col[Col.Length()] == L',') GETCOL;
      ASize = StrToInt64Def(Col, -1);
      // if it's not a number (file size) we take it as part of group name
      // (at least on CygWin, there can be group with space in its name)
      if (ASize < 0) Col = L" " + Col;
    }
    while (ASize < 0);

    // do not read modification time and filename if it is already set
    if ((fabs(static_cast<double>(FModification)) < std::numeric_limits<double>::epsilon()) && GetFileName().IsEmpty())
    {
      FSize = ASize;

      bool FullTime = false;
      bool DayMonthFormat = false;
      Word Day, Month, Year, Hour, Min, Sec;
      intptr_t P;

      GETCOL;
      // format dd mmm or mmm dd ?
      Day = static_cast<Word>(StrToIntDef(Col, 0));
      if (Day > 0)
      {
        DayMonthFormat = true;
        GETCOL;
      }
      Month = 0;
      #define COL2MONTH \
        for (Word IMonth = 0; IMonth < 12; IMonth++) \
          if (!Col.CompareIC(EngShortMonthNames[IMonth])) { Month = IMonth; Month++; break; }
      COL2MONTH;
      // if the column is not known month name, it may have been "yyyy-mm-dd"
      // for --full-time format
      if ((Month == 0) && (Col.Length() == 10) && (Col[5] == L'-') && (Col[8] == L'-'))
      {
        Year = static_cast<Word>(Col.SubString(1, 4).ToInt());
        Month = static_cast<Word>(Col.SubString(6, 2).ToInt());
        Day = static_cast<Word>(Col.SubString(9, 2).ToInt());
        GETCOL;
        Hour = static_cast<Word>(Col.SubString(1, 2).ToInt());
        Min = static_cast<Word>(Col.SubString(4, 2).ToInt());
        if (Col.Length() >= 8)
        {
          Sec = static_cast<Word>(Sysutils::StrToInt(Col.SubString(7, 2)));
        }
        else
        {
          Sec = 0;
        }
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
          Day = static_cast<Word>(Sysutils::StrToInt(Col));
        }
        if ((Day < 1) || (Day > 31)) { Abort(); }

        // second full-time format
        // ddd mmm dd hh:nn:ss yyyy
        if (FullTime)
        {
          GETCOL;
          if (Col.Length() != 8)
          {
            Abort();
          }
          Hour = static_cast<Word>(Sysutils::StrToInt(Col.SubString(1, 2)));
          Min = static_cast<Word>(Sysutils::StrToInt(Col.SubString(4, 2)));
          Sec = static_cast<Word>(Sysutils::StrToInt(Col.SubString(7, 2)));
          FModificationFmt = mfFull;
          // do not trim leading space of filename
          GETNCOL;
          Year = static_cast<Word>(Sysutils::StrToInt(Col));
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
            // Time/Year indicator is always 5 characters long (???), on most
            // systems year is aligned to right (_YYYY), but on some to left (YYYY_),
            // we must ensure that trailing space is also deleted, so real
            // separator space is not treated as part of file name
            Col = Line.SubString(1, 6).Trim();
            Line.Delete(1, 6);
          }
          // GETNCOL; // We don't want to trim input strings (name with space at beginning???)
          // Check if we got time (contains :) or year
          if ((P = static_cast<Word>(Col.Pos(L':'))) > 0)
          {
            Word CurrMonth, CurrDay;
            Hour = static_cast<Word>(Sysutils::StrToInt(Col.SubString(1, P-1)));
            Min = static_cast<Word>(Sysutils::StrToInt(Col.SubString(P+1, Col.Length() - P)));
            if ((Hour > 23) || (Min > 59)) Abort();
            // When we don't got year, we assume current year
            // with exception that the date would be in future
            // in this case we assume last year.
            DecodeDate(Date(), Year, CurrMonth, CurrDay);
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
            Year = static_cast<Word>(Sysutils::StrToInt(Col));
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

      if (fabs(static_cast<double>(FLastAccess)) < std::numeric_limits<double>::epsilon())
      {
        FLastAccess = FModification;
      }

      // separating space is already deleted, other spaces are treated as part of name

      {
        FLinkTo = L"";
        if (GetIsSymLink())
        {
          intptr_t P = Line.Pos(SYMLINKSTR);
          if (P)
          {
            FLinkTo = Line.SubString(
              P + wcslen(SYMLINKSTR), Line.Length() - P + wcslen(SYMLINKSTR) + 1);
            Line.SetLength(P - 1);
          }
          else
          {
            Abort();
          }
        }
        FFileName = UnixExtractFileName(::Trim(Line));
      }
    }

    #undef GETNCOL
    #undef GETCOL
  }
  catch (Exception &E)
  {
    throw ETerminal(&E, FmtLoadStr(LIST_LINE_ERROR, Value.c_str()));
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

  if (FLinkedFile) { delete FLinkedFile; }
  FLinkedFile = NULL;

  FCyclicLink = false;
  if (!GetLinkTo().IsEmpty())
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
      TRY_FINALLY (
      {
        GetTerminal()->ReadSymlink(this, FLinkedFile);
      }
      ,
      {
        GetTerminal()->SetExceptionOnFail(false);
      }
      );
    }
    catch (Exception &E)
    {
      if (dynamic_cast<EFatal *>(&E) != NULL) throw;
      else
      {
        GetTerminal()->GetLog()->AddException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetListingStr()
{
  // note that ModificationStr is longer than 12 for mfFull
  UnicodeString LinkPart;
  // expanded from ?: to avoid memory leaks
  if (GetIsSymLink())
  {
    LinkPart = UnicodeString(SYMLINKSTR) + GetLinkTo();
  }
  return Format(L"%s%s %3s %-8s %-8s %9s %-12s %s%s",
    GetType(), GetRights()->GetText().c_str(), IntToStr(FINodeBlocks).c_str(), GetFileOwner().GetName().c_str(),
    GetFileGroup().GetName().c_str(), Int64ToStr(GetSize()).c_str(), GetModificationStr().c_str(), GetFileName().c_str(),
    LinkPart.c_str());
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetFullFileName() const
{
  if (FFullFileName.IsEmpty())
  {
    assert(GetTerminal());
    assert(GetDirectory() != NULL);
    UnicodeString Path;
    if (GetIsParentDirectory()) { Path = GetDirectory()->GetParentPath(); }
    else if (GetIsDirectory()) { Path = UnixIncludeTrailingBackslash(GetDirectory()->GetFullDirectory() + GetFileName()); }
    else { Path = GetDirectory()->GetFullDirectory() + GetFileName(); }
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
  return !FFullFileName.IsEmpty() || (GetDirectory() != NULL);
}
//---------------------------------------------------------------------------
intptr_t TRemoteFile::GetAttr()
{
  intptr_t Result = 0;
  if (GetRights()->GetReadOnly()) { Result |= faReadOnly; }
  if (GetIsHidden()) { Result |= faHidden; }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetTerminal(TTerminal * Value)
{
  FTerminal = Value;
  if (FLinkedFile)
  {
    FLinkedFile->SetTerminal(Value);
  }
}
//---------------------------------------------------------------------------
const TRemoteToken & TRemoteFile::GetFileOwner() const
{
  return FOwner;
}
//---------------------------------------------------------------------------
TRemoteToken & TRemoteFile::GetFileOwner()
{
  return FOwner;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFileOwner(TRemoteToken Value)
{
  FOwner = Value;
}
//---------------------------------------------------------------------------
const TRemoteToken & TRemoteFile::GetFileGroup() const
{
  return FGroup;
}
//---------------------------------------------------------------------------
TRemoteToken & TRemoteFile::GetFileGroup()
{
  return FGroup;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFileGroup(TRemoteToken Value)
{
  FGroup = Value;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetFileName() const { return FFileName; }
//---------------------------------------------------------------------------
void TRemoteFile::SetFileName(const UnicodeString & Value)
{
  FFileName = Value;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetLinkTo() const
{
  return FLinkTo;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetLinkTo(const UnicodeString & Value)
{
  FLinkTo = Value;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFullFileName(const UnicodeString & Value)
{
  FFullFileName = Value;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryFile::TRemoteDirectoryFile() : TRemoteFile()
{
  SetModification(TDateTime(0.0));
  SetModificationFmt(mfNone);
  SetLastAccess(GetModification());
  SetType(L'D');
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
TRemoteFileList::TRemoteFileList() :
  TObjectList()
{
  FTimestamp = Now();
  SetOwnsObjects(true);
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
  CALLSTACK;
  Copy->Clear();
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TRemoteFile * File = GetFiles(Index);
    Copy->AddFile(File->Duplicate(false));
  }
  TRACE("1");
  Copy->FDirectory = GetDirectory();
  Copy->FTimestamp = FTimestamp;
  TRACE("/");
}
//---------------------------------------------------------------------------
void TRemoteFileList::Clear()
{
  FTimestamp = Now();
  TObjectList::Clear();
}
//---------------------------------------------------------------------------
void TRemoteFileList::SetDirectory(const UnicodeString & Value)
{
  FDirectory = UnixExcludeTrailingBackslash(Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFileList::GetFullDirectory()
{
  return UnixIncludeTrailingBackslash(GetDirectory());
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::GetFiles(Integer Index)
{
  return static_cast<TRemoteFile *>(Items[Index]);
}
//---------------------------------------------------------------------------
Boolean TRemoteFileList::GetIsRoot()
{
  return (GetDirectory() == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFileList::GetParentPath()
{
  return UnixExtractFilePath(GetDirectory());
}
//---------------------------------------------------------------------------
__int64 TRemoteFileList::GetTotalSize()
{
  __int64 Result = 0;
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (!GetFiles(Index)->GetIsDirectory()) { Result += GetFiles(Index)->GetSize(); }
  }
  return Result;
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::FindFile(const UnicodeString & FileName)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetFiles(Index)->GetFileName() == FileName) { return GetFiles(Index); }
  }
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
void TRemoteDirectory::SetDirectory(const UnicodeString & Value)
{
  TRemoteFileList::SetDirectory(Value);
  //Load();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::AddFile(TRemoteFile * File)
{
  if (File->GetIsThisDirectory()) { FThisDirectory = File; }
  if (File->GetIsParentDirectory()) { FParentDirectory = File; }

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
  CALLSTACK;
  TRemoteFileList::DuplicateTo(Copy);
  if (GetThisDirectory() && !GetIncludeThisDirectory())
  {
    Copy->AddFile(GetThisDirectory()->Duplicate(false));
  }
  if (GetParentDirectory() && !GetIncludeParentDirectory())
  {
    Copy->AddFile(GetParentDirectory()->Duplicate(false));
  }
  TRACE("/");
}
//---------------------------------------------------------------------------
bool TRemoteDirectory::GetLoaded()
{
  return ((GetTerminal() != NULL) && GetTerminal()->GetActive() && !GetDirectory().IsEmpty());
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

  for (intptr_t Index = 0; Index < GetCount(); Index ++)
  {
    if (GetFiles(Index)->GetSelected())
    {
      FSelectedFiles->Add(GetFiles(Index)->GetFullFileName());
    }
  }

  return FSelectedFiles;
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeParentDirectory(Boolean Value)
{
  if (GetIncludeParentDirectory() != Value)
  {
    FIncludeParentDirectory = Value;
    if (Value && GetParentDirectory())
    {
      assert(IndexOf(GetParentDirectory()) < 0);
      Add(GetParentDirectory());
    }
    else if (!Value && GetParentDirectory())
    {
      assert(IndexOf(GetParentDirectory()) >= 0);
      Extract(GetParentDirectory());
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeThisDirectory(Boolean Value)
{
  if (GetIncludeThisDirectory() != Value)
  {
    FIncludeThisDirectory = Value;
    if (Value && GetThisDirectory())
    {
      assert(IndexOf(GetThisDirectory()) < 0);
      Add(GetThisDirectory());
    }
    else if (!Value && GetThisDirectory())
    {
      assert(IndexOf(GetThisDirectory()) >= 0);
      Extract(GetThisDirectory());
    }
  }
}
//===========================================================================
TRemoteDirectoryCache::TRemoteDirectoryCache(): TStringList()
{
  CALLSTACK;
  FSection = new TCriticalSection();
  Sorted = true;
  Duplicates = dupError;
  CaseSensitive = true;
}
//---------------------------------------------------------------------------
TRemoteDirectoryCache::~TRemoteDirectoryCache()
{
  CALLSTACK;
  Clear();
  delete FSection;
  FSection = NULL;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::Clear()
{
  TGuard Guard(FSection);

  TRY_FINALLY (
  {
    for (intptr_t Index = 0; Index < GetCount(); ++Index)
    {
      delete dynamic_cast<TRemoteFileList *>(Objects[Index]);
      Objects(Index, NULL);
    }
  }
  ,
  {
    TStringList::Clear();
  }
  );
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetIsEmpty() const
{
  TGuard Guard(FSection);

  return (const_cast<TRemoteDirectoryCache*>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasFileList(const UnicodeString & Directory)
{
  TGuard Guard(FSection);

  intptr_t Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasNewerFileList(const UnicodeString & Directory,
  TDateTime Timestamp)
{
  TGuard Guard(FSection);

  intptr_t Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  if (Index >= 0)
  {
    TRemoteFileList * FileList = dynamic_cast<TRemoteFileList *>(Objects[Index]);
    if (FileList->GetTimestamp() <= Timestamp)
    {
      Index = -1;
    }
  }
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetFileList(const UnicodeString & Directory,
  TRemoteFileList * FileList)
{
  TGuard Guard(FSection);

  intptr_t Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  bool Result = (Index >= 0);
  if (Result)
  {
    assert(Objects[Index] != NULL);
    dynamic_cast<TRemoteFileList *>(Objects[Index])->DuplicateTo(FileList);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::AddFileList(TRemoteFileList * FileList)
{
  CALLSTACK;
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
  TRACE("/");
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::ClearFileList(const UnicodeString & Directory, bool SubDirs)
{
  TGuard Guard(FSection);
  DoClearFileList(Directory, SubDirs);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::DoClearFileList(const UnicodeString & Directory, bool SubDirs)
{
  UnicodeString Directory2 = UnixExcludeTrailingBackslash(Directory);
  intptr_t Index = IndexOf(Directory2);
  if (Index >= 0)
  {
    Delete(Index);
  }
  if (SubDirs)
  {
    Directory2 = UnixIncludeTrailingBackslash(Directory2);
    Index = GetCount() - 1;
    while (Index >= 0)
    {
      if (Strings[Index].SubString(1, Directory2.Length()) == Directory2)
      {
        Delete(Index);
      }
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::Delete(intptr_t Index)
{
  delete static_cast<TRemoteFileList *>(Objects[Index]);
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryChangesCache::TRemoteDirectoryChangesCache(intptr_t MaxSize) :
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
void TRemoteDirectoryChangesCache::SetValue(const UnicodeString & Name,
  const UnicodeString & Value)
{
  intptr_t Index = IndexOfName(Name);
  if (Index >= 0)
  {
    Delete(Index);
  }
  Values(Name, Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteDirectoryChangesCache::GetValue(const UnicodeString & Name)
{
  UnicodeString Value = Values[Name];
  SetValue(Name, Value);
  return Value;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::AddDirectoryChange(
  const UnicodeString & SourceDir, const UnicodeString & Change,
  const UnicodeString & TargetDir)
{
  assert(!TargetDir.IsEmpty());
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
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChange(
  const UnicodeString & SourceDir)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (Names[Index].SubString(1, SourceDir.Length()) == SourceDir)
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChangeTarget(
  const UnicodeString & TargetDir)
{
  UnicodeString Key;
  // hack to clear at least local sym-link change in case symlink is deleted
  DirectoryChangeKey(UnixExcludeTrailingBackslash(UnixExtractFilePath(TargetDir)),
    UnixExtractFileName(TargetDir), Key);

  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    UnicodeString Name = Names[Index];
    if ((Name.SubString(1, TargetDir.Length()) == TargetDir) ||
        (Values[Name].SubString(1, TargetDir.Length()) == TargetDir) ||
        (!Key.IsEmpty() && (Name == Key)))
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetDirectoryChange(
  const UnicodeString & SourceDir, const UnicodeString & Change, UnicodeString & TargetDir)
{
  UnicodeString Key;
  bool Result;
  Key = TTerminal::ExpandFileName(Change, SourceDir);
  if (Key.IsEmpty())
  {
    Key = L"/";
  }
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
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Serialize(UnicodeString & Data)
{
  Data = L"A";
  intptr_t ACount = GetCount();
  if (ACount > FMaxSize)
  {
    TStrings * Limited = new TStringList();
    TRY_FINALLY (
    {
      intptr_t Index = ACount - FMaxSize;
      while (Index < ACount)
      {
        Limited->Add(Strings[Index]);
        ++Index;
      }
      Data += Limited->Text;
    }
    ,
    {
      delete Limited;
    }
    );
  }
  else
  {
    Data += Text;
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Deserialize(const UnicodeString & Data)
{
  if (Data.IsEmpty())
  {
    Text = L"";
  }
  else
  {
    Text = Data.c_str() + 1;
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::DirectoryChangeKey(
  const UnicodeString & SourceDir, const UnicodeString & Change, UnicodeString & Key)
{
  bool Result = !Change.IsEmpty();
  if (Result)
  {
    bool Absolute = TTerminal::IsAbsolutePath(Change);
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
//---------------------------------------------------------------------------
TRights::TRights()
{
  FAllowUndef = false;
  FSet = 0;
  FUnset = 0;
  SetNumber(0);
  FUnknown = true;
}
//---------------------------------------------------------------------------
TRights::TRights(unsigned short ANumber)
{
  FAllowUndef = false;
  FSet = 0;
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
void TRights::SetAllowUndef(bool Value)
{
  if (FAllowUndef != Value)
  {
    assert(!Value || ((FSet | FUnset) == rfAllSpecials));
    FAllowUndef = Value;
  }
}
//---------------------------------------------------------------------------
void TRights::SetText(const UnicodeString & Value)
{
  if (Value != GetText())
  {
    if ((Value.Length() != TextLen) ||
        (!GetAllowUndef() && (Value.Pos(UndefSymbol) > 0)) ||
        (Value.Pos(L" ") > 0))
    {
      throw Exception(FMTLOAD(RIGHTS_ERROR, Value.c_str()));
    }

    FSet = 0;
    FUnset = 0;
    int Flag = 00001;
    int ExtendedFlag = 01000; //-V536
    bool KeepText = false;
    for (int i = TextLen; i >= 1; i--)
    {
      if (Value[i] == UnsetSymbol)
      {
        FUnset |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (Value[i] == UndefSymbol)
      {
        // do nothing
      }
      else if (Value[i] == CombinedSymbols[i - 1])
      {
        FSet |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (Value[i] == ExtendedSymbols[i - 1])
      {
        FSet |= static_cast<unsigned short>(ExtendedFlag);
        FUnset |= static_cast<unsigned short>(Flag);
      }
      else
      {
        if (Value[i] != BasicSymbols[i - 1])
        {
          KeepText = true;
        }
        FSet |= static_cast<unsigned short>(Flag);
        if (i % 3 == 0)
        {
          FUnset |= static_cast<unsigned short>(ExtendedFlag);
        }
      }

      Flag <<= 1;
      if (i % 3 == 1)
      {
        ExtendedFlag <<= 1;
      }
    }

    FText = KeepText ? Value : UnicodeString();
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetText() const
{
  if (!FText.IsEmpty())
  {
    return FText;
  }
  else
  {
    UnicodeString Result(TextLen, 0);

    int Flag = 00001;
    int ExtendedFlag = 01000; //-V536
    bool ExtendedPos = true;
    wchar_t Symbol;
    int i = TextLen;
    while (i >= 1)
    {
      if (ExtendedPos &&
          ((FSet & (Flag | ExtendedFlag)) == (Flag | ExtendedFlag)))
      {
        Symbol = CombinedSymbols[i - 1];
      }
      else if ((FSet & Flag) != 0)
      {
        Symbol = BasicSymbols[i - 1];
      }
      else if (ExtendedPos && ((FSet & ExtendedFlag) != 0))
      {
        Symbol = ExtendedSymbols[i - 1];
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

      Result[i] = Symbol;

      Flag <<= 1;
      i--;
      ExtendedPos = ((i % 3) == 0);
      if (ExtendedPos)
      {
        ExtendedFlag <<= 1;
      }
    }
    return Result;
  }
}
//---------------------------------------------------------------------------
void TRights::SetOctal(const UnicodeString & Value)
{
  UnicodeString AValue(Value);
  if (AValue.Length() == 3)
  {
    AValue = L"0" + AValue;
  }

  if (GetOctal() != AValue.c_str())
  {
    bool Correct = (AValue.Length() == 4);
    if (Correct)
    {
      for (intptr_t I = 1; (I <= AValue.Length()) && Correct; I++)
      {
        Correct = (AValue[I] >= L'0') && (AValue[I] <= L'7');
      }
    }

    if (!Correct)
    {
      throw Exception(FMTLOAD(INVALID_OCTAL_PERMISSIONS, Value.c_str()));
    }

    SetNumber(static_cast<unsigned short>(
      ((AValue[1] - L'0') << 9) +
      ((AValue[2] - L'0') << 6) +
      ((AValue[3] - L'0') << 3) +
      ((AValue[4] - L'0') << 0)));
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
UnicodeString TRights::GetOctal() const
{
  UnicodeString Result;
  unsigned short N = GetNumberSet(); // used to be "Number"
  Result.SetLength(4);
  Result[1] = static_cast<wchar_t>(L'0' + ((N & 07000) >> 9));
  Result[2] = static_cast<wchar_t>(L'0' + ((N & 00700) >> 6));
  Result[3] = static_cast<wchar_t>(L'0' + ((N & 00070) >> 3));
  Result[4] = static_cast<wchar_t>(L'0' + ((N & 00007) >> 0));

  return Result;
}
//---------------------------------------------------------------------------
void TRights::SetNumber(unsigned short Value)
{
  if ((FSet != Value) || ((FSet | FUnset) != rfAllSpecials))
  {
    FSet = Value;
    FUnset = static_cast<unsigned short>(rfAllSpecials & ~FSet);
    FText = L"";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
unsigned short TRights::GetNumber() const
{
  assert(!GetIsUndef());
  return FSet;
}
//---------------------------------------------------------------------------
void TRights::SetRight(TRight Right, bool Value)
{
  SetRightUndef(Right, (Value ? rsYes : rsNo));
}
//---------------------------------------------------------------------------
bool TRights::GetRight(TRight Right) const
{
  TState State = GetRightUndef(Right);
  assert(State != rsUndef);
  return (State == rsYes);
}
//---------------------------------------------------------------------------
void TRights::SetRightUndef(TRight Right, TState Value)
{
  if (Value != GetRightUndef(Right))
  {
    assert((Value != rsUndef) || GetAllowUndef());

    TFlag Flag = RightToFlag(Right);

    switch (Value)
    {
      case rsYes:
        FSet |= static_cast<unsigned short>(Flag);
        FUnset &= static_cast<unsigned short>(~Flag);
        break;

      case rsNo:
        FSet &= static_cast<unsigned short>(~Flag);
        FUnset |= static_cast<unsigned short>(Flag);
        break;

      case rsUndef:
      default:
        FSet &= static_cast<unsigned short>(~Flag);
        FUnset &= static_cast<unsigned short>(~Flag);
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
void TRights::SetReadOnly(bool Value)
{
  SetRight(rrUserWrite, !Value);
  SetRight(rrGroupWrite, !Value);
  SetRight(rrOtherWrite, !Value);
}
//---------------------------------------------------------------------------
bool TRights::GetReadOnly() const
{
  return GetRight(rrUserWrite) && GetRight(rrGroupWrite) && GetRight(rrOtherWrite);
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetSimplestStr() const
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
UnicodeString TRights::GetModeStr() const
{
  UnicodeString Result;
  UnicodeString SetModeStr, UnsetModeStr;
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
  Recursive = false;
  Rights.SetAllowUndef(false);
  Rights.SetNumber(0);
  Group.Clear();
  Owner.Clear();
  Modification = 0;
  LastAccess = 0;
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
  for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
  {
    TRemoteFile * File = static_cast<TRemoteFile *>(FileList->Objects[Index]);
    assert(File);
    if (!Index)
    {
      CommonProperties.Rights.Assign(File->GetRights());
      // previously we allowed undef implicitly for directories,
      // now we do it explicitly in properties dialog and only in combination
      // with "recursive" option
      CommonProperties.Rights.SetAllowUndef(File->GetRights()->GetIsUndef());
      CommonProperties.Valid << vpRights;
      if (File->GetFileOwner().GetIsSet())
      {
        CommonProperties.Owner = File->GetFileOwner();
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

    NewProperties.Group.SetID(OriginalProperties.Group.GetID());
    NewProperties.Owner.SetID(OriginalProperties.Owner.GetID());
  }
  return NewProperties;
}
//---------------------------------------------------------------------------
void TRemoteProperties::Load(THierarchicalStorage * Storage)
{
  unsigned char Buf[sizeof(Valid)];
  if (static_cast<size_t>(Storage->ReadBinaryData(L"Valid", &Buf, sizeof(Buf))) == sizeof(Buf))
  {
    memmove(&Valid, Buf, sizeof(Valid));
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
  Storage->WriteBinaryData(UnicodeString(L"Valid"),
    static_cast<const void *>(&Valid), sizeof(Valid));

  if (Valid.Contains(vpRights))
  {
    Storage->WriteString(L"Rights", Rights.GetText());
  }

  // TODO
}
