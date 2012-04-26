//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"
#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "PuttyIntf.h"
#endif

#include "RemoteFiles.h"

#ifndef _MSC_VER
#include <SysUtils.hpp>
#endif

#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
/* TODO 1 : Path class instead of UnicodeString (handle relativity...) */
//---------------------------------------------------------------------------
UnicodeString __fastcall UnixIncludeTrailingBackslash(const UnicodeString Path)
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
UnicodeString __fastcall UnixExcludeTrailingBackslash(const UnicodeString Path)
{
  if ((Path.Length() > 1) && Path.IsDelimiter(L"/", Path.Length()))
      return Path.SubString(1, Path.Length() - 1);
    else return Path;
}
//---------------------------------------------------------------------------
Boolean __fastcall UnixComparePaths(const UnicodeString Path1, const UnicodeString Path2)
{
  return (UnixIncludeTrailingBackslash(Path1) == UnixIncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool __fastcall UnixIsChildPath(UnicodeString Parent, UnicodeString Child)
{
  Parent = UnixIncludeTrailingBackslash(Parent);
  Child = UnixIncludeTrailingBackslash(Child);
  return (Child.SubString(1, Parent.Length()) == Parent);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UnixExtractFileDir(const UnicodeString Path)
{
  int Pos = Path.LastDelimiter(L'/');
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
UnicodeString __fastcall UnixExtractFilePath(const UnicodeString Path)
{
  int Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  return (Pos > 0) ? Path.SubString(1, Pos) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UnixExtractFileName(const UnicodeString Path)
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
//---------------------------------------------------------------------------
UnicodeString __fastcall UnixExtractFileExt(const UnicodeString Path)
{
  UnicodeString FileName = UnixExtractFileName(Path);
  int Pos = FileName.LastDelimiter(L".");
  return (Pos > 0) ? Path.SubString(Pos, Path.Length() - Pos + 1) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ExtractFileName(const UnicodeString & Path, bool Unix)
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
//---------------------------------------------------------------------------
bool __fastcall ExtractCommonPath(TStrings * Files, UnicodeString & Path)
{
  assert(Files->GetCount() > 0);

  Path = ExtractFilePath(Files->GetString(0));
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (size_t Index = 1; Index < Files->GetCount(); Index++)
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
//---------------------------------------------------------------------------
bool __fastcall UnixExtractCommonPath(TStrings * Files, UnicodeString & Path)
{
  assert(Files->GetCount() > 0);

  Path = UnixExtractFilePath(Files->GetString(0));
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (size_t Index = 1; Index < Files->GetCount(); Index++)
    {
      while (!Path.IsEmpty() &&
        (Files->Strings[Index].SubString(1, Path.Length()) != Path))
      {
        size_t PrevLen = Path.Length();
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
bool __fastcall IsUnixRootPath(const UnicodeString Path)
{
  return Path.IsEmpty() || (Path == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
bool __fastcall IsUnixHiddenFile(const UnicodeString FileName)
{
  return (FileName != ROOTDIRECTORY) && (FileName != PARENTDIRECTORY) &&
    !FileName.IsEmpty() && (FileName[1] == L'.');
}
//---------------------------------------------------------------------------
UnicodeString __fastcall AbsolutePath(const UnicodeString & Base, const UnicodeString & Path)
{
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
      int P2 = Result.SubString(1, P-1).LastDelimiter(L"/");
      assert(P2 > 0);
      Result.Delete(P2, P - P2 + 3);
    }
    while ((P = Result.Pos(L"/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FromUnixPath(const UnicodeString Path)
{
  return StringReplace(Path, L"/", L"\\");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ToUnixPath(const UnicodeString Path)
{
  return StringReplace(Path, L"\\", L"/");
}
//---------------------------------------------------------------------------
static void __fastcall CutFirstDirectory(UnicodeString & S, bool Unix)
{
  bool Root;
  size_t P;
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
UnicodeString __fastcall MinimizeName(const UnicodeString FileName, int MaxLen, bool Unix)
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
UnicodeString __fastcall MakeFileList(TStrings * FileList)
{
  UnicodeString Result;
  for (size_t Index = 0; Index < FileList->GetCount(); Index++)
  {
    if (!Result.IsEmpty())
    {
      Result += L" ";
    }

    UnicodeString FileName = FileList->GetString(Index);
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
TDateTime __fastcall ReduceDateTimePrecision(TDateTime DateTime,
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
TModificationFmt __fastcall LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2)
{
  return (Precision1 < Precision2) ? Precision1 : Precision2;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UserModificationStr(TDateTime DateTime,
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
int __fastcall FakeFileImageIndex(UnicodeString FileName, unsigned long Attrs,
  UnicodeString * TypeName)
{
  Attrs |= FILE_ATTRIBUTE_NORMAL;

  TSHFileInfoW SHFileInfo;
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

  int Icon;
  if (SHGetFileInfo(UnicodeString(FileName).c_str(),
        Attrs, &SHFileInfo, sizeof(SHFileInfo),
        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME) != 0)
  {
    if (TypeName != NULL)
    {
      *TypeName = SHFileInfo.szTypeName;
    }
    Icon = SHFileInfo.iIcon;
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
TRemoteToken::TRemoteToken(const UnicodeString & Name) :
  FName(Name),
  FID(0),
  FIDValid(false)
{
}
//---------------------------------------------------------------------------
void __fastcall TRemoteToken::Clear()
{
  FID = 0;
  FIDValid = false;
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteToken::operator ==(const TRemoteToken & rht) const
{
  return
    (FName == rht.FName) &&
    (FIDValid == rht.FIDValid) &&
    (!FIDValid || (FID == rht.FID));
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteToken::operator !=(const TRemoteToken & rht) const
{
  return !(*this == rht);
}
//---------------------------------------------------------------------------
TRemoteToken & __fastcall TRemoteToken::operator =(const TRemoteToken & rht)
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
int __fastcall TRemoteToken::Compare(const TRemoteToken & rht) const
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
void __fastcall TRemoteToken::SetID(unsigned int value)
{
  FID = value;
  FIDValid = true;
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteToken::GetNameValid() const
{
  return !FName.IsEmpty();
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteToken::GetIsSet() const
{
  return !FName.IsEmpty() || FIDValid;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteToken::GetDisplayText() const
{
  if (!FName.IsEmpty())
  {
    return FName;
  }
  else if (FIDValid)
  {
    return IntToStr(static_cast<int>(FID));
  }
  else
  {
    return UnicodeString();
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteToken::GetLogText() const
{
  return FORMAT(L"\"%s\" [%d]", FName.c_str(), static_cast<int>(FID));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteTokenList * __fastcall TRemoteTokenList::Duplicate() const
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
void __fastcall TRemoteTokenList::Clear()
{
  FTokens.clear();
  FNameMap.clear();
  FIDMap.clear();
}
//---------------------------------------------------------------------------
void __fastcall TRemoteTokenList::Add(const TRemoteToken & Token)
{
  FTokens.push_back(Token);
  if (Token.GetIDValid())
  {
    std::pair<TIDMap::iterator, bool> Position =
      FIDMap.insert(TIDMap::value_type(Token.ID, FTokens.size() - 1));
  }
  if (Token.GetNameValid())
  {
    std::pair<TNameMap::iterator, bool> Position =
      FNameMap.insert(TNameMap::value_type(Token.GetName(), FTokens.size() - 1));
  }
}
//---------------------------------------------------------------------------
void __fastcall TRemoteTokenList::AddUnique(const TRemoteToken & Token)
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
bool __fastcall TRemoteTokenList::Exists(const UnicodeString & Name) const
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
void __fastcall TRemoteTokenList::Log(TTerminal * Terminal, const wchar_t * Title)
{
  if (!FTokens.empty())
  {
    Terminal->LogEvent(FORMAT(L"Following %s found:", Title));
    for (size_t Index = 0; Index < FTokens.size(); Index++)
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
int __fastcall TRemoteTokenList::Count() const
{
  return (int)FTokens.size();
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::GetToken(size_t Index) const
{
  return &FTokens[Index];
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteFile::TRemoteFile(TRemoteFile * ALinkedByFile):
  System::TPersistent(),
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
  FIsHidden(0),
  Self(NULL)
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
TRemoteFile * __fastcall TRemoteFile::Duplicate(bool Standalone) const
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
void __fastcall TRemoteFile::LoadTypeInfo()
{
  /* TODO : If file is link: Should be attributes taken from linked file? */
  unsigned long Attrs = 0;
  if (GetIsDirectory()) { Attrs |= FILE_ATTRIBUTE_DIRECTORY; }
  if (GetIsHidden()) { Attrs |= FILE_ATTRIBUTE_HIDDEN; }

  UnicodeString DumbFileName = (GetIsSymLink() && !GetLinkTo().IsEmpty() ? GetLinkTo() : GetFileName());

  FIconIndex = FakeFileImageIndex(DumbFileName, Attrs, &FTypeName);
}
//---------------------------------------------------------------------------
Integer __fastcall TRemoteFile::GetIconIndex() const
{
  if (FIconIndex == -1)
  {
    const_cast<TRemoteFile *>(this)->LoadTypeInfo();
  }
  return FIconIndex;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFile::GetTypeName()
{
  // check avilability of type info by icon index, because type name can be empty
  if (FIconIndex < 0)
  {
    LoadTypeInfo();
  }
  return FTypeName;
}
//---------------------------------------------------------------------------
Boolean __fastcall TRemoteFile::GetIsHidden()
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
      Result = IsUnixHiddenFile(FileName);
      break;
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::SetIsHidden(bool value)
{
  FIsHidden = value ? 1 : 0;
}
//---------------------------------------------------------------------------
Boolean __fastcall TRemoteFile::GetIsDirectory() const
{
  return (toupper(GetType()) == FILETYPE_DIRECTORY);
}
//---------------------------------------------------------------------------
Boolean __fastcall TRemoteFile::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
Boolean __fastcall TRemoteFile::GetIsThisDirectory() const
{
  return (GetFileName() == THISDIRECTORY);
}
//---------------------------------------------------------------------------
Boolean __fastcall TRemoteFile::GetIsInaccesibleDirectory() const
{
  Boolean Result;
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
  else { Result = False; }
  return Result;
}
//---------------------------------------------------------------------------
wchar_t __fastcall TRemoteFile::GetType() const
{
  if (GetIsSymLink() && FLinkedFile) { return FLinkedFile->GetType(); }
  else { return FType; }
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::SetType(wchar_t AType)
{
  FType = AType;
  FIsSymLink = ((wchar_t)towupper(FType) == FILETYPE_SYMLINK);
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TRemoteFile::GetLinkedFile()
{
  // do not call FindLinkedFile as it would be called releatedly for broken symlinks
  return FLinkedFile;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::SetLinkedFile(TRemoteFile * value)
{
  if (FLinkedFile != value)
  {
    if (FLinkedFile) { delete FLinkedFile; }
    FLinkedFile = value;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteFile::GetBrokenLink()
{
  assert(GetTerminal());
  // If file is symlink but we couldn't find linked file we assume broken link
  return (GetIsSymLink() && (FCyclicLink || !FLinkedFile) &&
    GetTerminal()->GetResolvingSymlinks());
  // "!FLinkTo.IsEmpty()" removed because it does not work with SFTP
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::ShiftTime(const TDateTime & Difference)
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
void __fastcall TRemoteFile::SetModification(const TDateTime & value)
{
  if (FModification != value)
  {
    FModificationFmt = mfFull;
    FModification = value;
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFile::GetUserModificationStr()
{
  return ::UserModificationStr(GetModification(), FModificationFmt);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFile::GetModificationStr()
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  GetModification().DecodeDate(&Year, &Month, &Day);
  GetModification().DecodeTime(&Hour, &Min, &Sec, &MSec);
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
UnicodeString __fastcall TRemoteFile::GetExtension()
{
  return UnixExtractFileExt(FFileName);
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::SetRights(TRights * value)
{
  FRights->Assign(value);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFile::GetRightsStr()
{
  return FRights->GetUnknown() ? UnicodeString() : FRights->GetText();
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::SetListingStr(UnicodeString value)
{
  // DEBUG_PRINTF(L"begin, value = %s", value.c_str());
  // Value stored in 'value' can be used for error message
  UnicodeString Line = value;
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
        Integer P = Line.Pos(L' '); \
        if (P) { Col = Line.SubString(1, P-1); Line.Delete(1, P); } \
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

      GETCOL;
      // format dd mmm or mmm dd ?
      Day = static_cast<unsigned Word>(StrToIntDef(Col, 0));
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
        Day = static_cast<Word>(Col.SubString(8, 2).ToInt());
        GETCOL;
        Hour = static_cast<Word>(Col.SubString(1, 2).ToInt());
        Min = static_cast<Word>(Col.SubString(4, 2)).ToInt();
        Sec = static_cast<Word>(Col.SubString(7, 2).ToInt());
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
          Day = (Word)StrToInt(Col);
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
          Hour = (Word)StrToInt(Col.SubString(1, 2));
          Min = (Word)StrToInt(Col.SubString(4, 2));
          Sec = (Word)StrToInt(Col.SubString(7, 2));
          FModificationFmt = mfFull;
          // do not trim leading space of filename
          GETNCOL;
          Year = (Word)StrToInt(Col);
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
            Col = Line.SubString(1, 6).Trim();
            Line.Delete(1, 6);
          }
          // GETNCOL; // We don't want to trim input strings (name with space at beginning???)
          // Check if we got time (contains :) or year
          if ((P = (Word)Col.Pos(L':')) > 0)
          {
            Word CurrMonth, CurrDay;
            Hour = (Word)StrToInt(Col.SubString(1, P-1));
            Min = (Word)StrToInt(Col.SubString(P+1, Col.Length() - P));
            if (Hour > 23 || Hour > 59) Abort();
            // When we don't got year, we assume current year
            // with exception that the date would be in future
            // in this case we assume last year.
            DecodeDate(Date(), Year, CurrMonth, CurrDay);
            if ((Month > CurrMonth) ||
                (Month == CurrMonth && Day > CurrDay)) { Year--; }
            Sec = 0;
            FModificationFmt = mfMDHM;
          }
          else
          {
            Year = (Word)StrToInt(Col);
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
        int P;

        FLinkTo = L"";
        if (GetIsSymLink())
        {
          P = Line.Pos(SYMLINKSTR);
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
        // DEBUG_PRINTF(L"Line = %s, FFileName = '%s'", Line.c_str(), FFileName.c_str());
      }
    }

    #undef GETNCOL
    #undef GETCOL
  }
  catch (Exception &E)
  {
    throw ETerminal(FmtLoadStr(LIST_LINE_ERROR, value.c_str()), &E);
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::Complete()
{
  assert(GetTerminal() != NULL);
  if (GetIsSymLink() && GetTerminal()->GetResolvingSymlinks())
  {
    FindLinkedFile();
  }
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::FindLinkedFile()
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
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->GetTerminal()->SetExceptionOnFail(false);
        } BOOST_SCOPE_EXIT_END
        GetTerminal()->ReadSymlink(this, FLinkedFile);
      }
    }
    catch (Exception &E)
    {
      if (E.InheritsFrom<EFatal>()) throw;
      else
      {
        Exception Ex(&E);
        GetTerminal()->GetLog()->AddException(&Ex);
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFile::GetListingStr()
{
  // note that ModificationStr is longer than 12 for mfFull
  UnicodeString LinkPart;
  // expanded from ?: to avoid memory leaks
  if (GetIsSymLink())
  {
    LinkPart = UnicodeString(SYMLINKSTR) + GetLinkTo();
  }
  return FORMAT(L"%s%s %3s %-8s %-8s %9s %-12s %s%s",
    GetType(), GetRights()->GetText().c_str(), IntToStr(GetINodeBlocks()).c_str(), GetOwner().GetName().c_str(),
    GetGroup().GetName().c_str(), Int64ToStr(GetSize()).c_str(), GetModificationStr().c_str(), GetFileName().c_str(),
    LinkPart.c_str());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFile::GetFullFileName() const
{
  if (FFullFileName.IsEmpty())
  {
    assert(GetTerminal());
    assert(GetDirectory() != NULL);
    UnicodeString Path;
    if (GetIsParentDirectory()) { Path = GetDirectory()->GetParentPath(); }
    else
    if (GetIsDirectory()) { Path = UnixIncludeTrailingBackslash(GetDirectory()->GetFullDirectory() + GetFileName()); }
      else { Path = GetDirectory()->GetFullDirectory() + GetFileName(); }
    return GetTerminal()->TranslateLockedPath(Path, true);
  }
  else
  {
    return FFullFileName;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteFile::GetHaveFullFileName() const
{
  return !FFullFileName.IsEmpty() || (GetDirectory() != NULL);
}
//---------------------------------------------------------------------------
Integer __fastcall TRemoteFile::GetAttr()
{
  Integer Result = 0;
  if (GetRights()->GetReadOnly()) { Result |= faReadOnly; }
  if (GetIsHidden()) { Result |= faHidden; }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFile::SetTerminal(TTerminal * value)
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
  System::TObjectList()
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
void __fastcall TRemoteFileList::DuplicateTo(TRemoteFileList * Copy)
{
  Copy->Clear();
  for (size_t Index = 0; Index < GetCount(); Index++)
  {
    TRemoteFile * File = GetFile(Index);
    Copy->AddFile(File->Duplicate(false));
  }
  Copy->FDirectory = GetDirectory();
  Copy->FTimestamp = FTimestamp;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFileList::Clear()
{
  FTimestamp = Now();
  TObjectList::Clear();
}
//---------------------------------------------------------------------------
void __fastcall TRemoteFileList::SetDirectory(UnicodeString value)
{
  FDirectory = UnixExcludeTrailingBackslash(value);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFileList::GetFullDirectory()
{
  return UnixIncludeTrailingBackslash(GetDirectory());
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TRemoteFileList::GetFiles(Integer Index)
{
  return static_cast<TRemoteFile *>(GetItem(Index));
}
//---------------------------------------------------------------------------
Boolean __fastcall TRemoteFileList::GetIsRoot()
{
  return (GetDirectory() == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TRemoteFileList::GetParentPath()
{
  return UnixExtractFilePath(GetDirectory());
}
//---------------------------------------------------------------------------
__int64 __fastcall TRemoteFileList::GetTotalSize()
{
  __int64 Result = 0;
  for (size_t Index = 0; Index < GetCount(); Index++)
    if (!GetFile(Index)->GetIsDirectory()) { Result += GetFiles(Index)->GetSize(); }
  return Result;
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TRemoteFileList::FindFile(const UnicodeString &FileName)
{
  for (size_t Index = 0; Index < GetCount(); Index++)
    if (GetFiles(Index)->GetFileName() == FileName) { return GetFiles(Index); }
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
void __fastcall TRemoteDirectory::Clear()
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
void __fastcall TRemoteDirectory::SetDirectory(UnicodeString value)
{
  TRemoteFileList::SetDirectory(value);
  //Load();
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectory::AddFile(TRemoteFile * File)
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
void __fastcall TRemoteDirectory::DuplicateTo(TRemoteFileList * Copy)
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
bool __fastcall TRemoteDirectory::GetLoaded()
{
  return ((GetTerminal() != NULL) && GetTerminal()->GetActive() && !GetDirectory().IsEmpty());
}
//---------------------------------------------------------------------------
TStrings * __fastcall TRemoteDirectory::GetSelectedFiles()
{
  if (!FSelectedFiles)
  {
    FSelectedFiles = new TStringList();
  }
  else
  {
    FSelectedFiles->Clear();
  }

  for (size_t Index = 0; Index < GetCount(); Index ++)
  {
    if (GetFiles(Index)->GetSelected())
    {
      FSelectedFiles->Add(GetFiles(Index)->GetFullFileName());
    }
  }

  return FSelectedFiles;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectory::SetIncludeParentDirectory(Boolean value)
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
      Extract(GetParentDirectory());
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectory::SetIncludeThisDirectory(Boolean value)
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
      Extract(GetThisDirectory());
    }
  }
}
//===========================================================================
TRemoteDirectoryCache::TRemoteDirectoryCache(): TStringList()
{
  FSection = new TCriticalSection();
  SetSorted(true);
  SetDuplicates(System::dupError);
  SetCaseSensitive(true);
  Self = this;
}
//---------------------------------------------------------------------------
TRemoteDirectoryCache::~TRemoteDirectoryCache()
{
  Clear();
  delete FSection;
  FSection = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectoryCache::Clear()
{
  TGuard Guard(FSection);

  BOOST_SCOPE_EXIT ( (&Self) )
  {
    Self->TStringList::Clear();
  } BOOST_SCOPE_EXIT_END
  for (size_t Index = 0; Index < GetCount(); Index++)
  {
    delete (TRemoteFileList *)GetObject(Index);
    PutObject(Index, NULL);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteDirectoryCache::GetIsEmpty() const
{
  TGuard Guard(FSection);

  return (const_cast<TRemoteDirectoryCache*>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteDirectoryCache::HasFileList(const UnicodeString Directory)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteDirectoryCache::HasNewerFileList(const UnicodeString Directory,
  TDateTime Timestamp)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  if (Index >= 0)
  {
    TRemoteFileList * FileList = dynamic_cast<TRemoteFileList *>(GetObjects(Index));
    if (FileList->GetTimestamp() <= Timestamp)
    {
      Index = -1;
    }
  }
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool __fastcall TRemoteDirectoryCache::GetFileList(const UnicodeString Directory,
  TRemoteFileList * FileList)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  bool Result = (Index >= 0);
  if (Result)
  {
    assert(GetObjects(Index) != NULL);
    dynamic_cast<TRemoteFileList *>(GetObjects(Index))->DuplicateTo(FileList);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectoryCache::AddFileList(TRemoteFileList * FileList)
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
void __fastcall TRemoteDirectoryCache::ClearFileList(UnicodeString Directory, bool SubDirs)
{
  TGuard Guard(FSection);
  DoClearFileList(Directory, SubDirs);
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectoryCache::DoClearFileList(UnicodeString Directory, bool SubDirs)
{
  Directory = UnixExcludeTrailingBackslash(Directory);
  int Index = IndexOf(Directory);
  if (Index >= 0)
  {
    Delete(Index);
  }
  if (SubDirs)
  {
    Directory = UnixIncludeTrailingBackslash(Directory);
    Index = Count-1;
    while (Index >= 0)
    {
      if (GetStrings(Index).SubString(1, Directory.Length()) == Directory)
      {
        Delete(Index);
      }
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TRemoteDirectoryCache::Delete(int Index)
{
  delete (TRemoteFileList *)GetObjects(Index);
  System::TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryChangesCache::TRemoteDirectoryChangesCache(int MaxSize) :
  System::TStringList(),
  FMaxSize(MaxSize)
{
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Clear()
{
  System::TStringList::Clear();
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetIsEmpty() const
{
  return (const_cast<TRemoteDirectoryChangesCache *>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::SetValue(const UnicodeString Name,
    const UnicodeString Value)
{
  size_t Index = IndexOfName(Name.c_str());
  if (Index != NPOS)
  {
    Delete(Index);
  }
  System::TStringList::SetValue(Name, Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteDirectoryChangesCache::GetValue(const UnicodeString Name)
{
  UnicodeString Value = System::TStringList::GetValue(Name);
  System::TStringList::SetValue(Name, Value);
  return Value;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::AddDirectoryChange(
  const UnicodeString SourceDir, const UnicodeString Change,
  const UnicodeString TargetDir)
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
  const UnicodeString SourceDir)
{
  for (size_t Index = 0; Index < GetCount(); Index++)
  {
    if (GetName(Index).SubString(0, SourceDir.Length()) == SourceDir)
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChangeTarget(
  const UnicodeString TargetDir)
{
  UnicodeString Key;
  // hack to clear at least local sym-link change in case symlink is deleted
  DirectoryChangeKey(UnixExcludeTrailingBackslash(UnixExtractFilePath(TargetDir)),
                     UnixExtractFileName(TargetDir), Key);

  for (size_t Index = 0; Index < GetCount(); Index++)
  {
    UnicodeString Name = GetName(Index);
    if ((Name.SubString(0, TargetDir.Length()) == TargetDir) ||
        (GetValue(Name).SubString(0, TargetDir.Length()) == TargetDir) ||
        (!Key.IsEmpty() && (Name == Key)))
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetDirectoryChange(
  const UnicodeString SourceDir, const UnicodeString Change, UnicodeString & TargetDir)
{
  // DEBUG_PRINTF(L"begin, SourceDir = %s, Change = %s", SourceDir.c_str(), Change.c_str());
  UnicodeString Key;
  bool Result;
  Key = TTerminal::ExpandFileName(Change, SourceDir);
  if (Key.IsEmpty())
  {
    Key = L"/";
  }
  // DEBUG_PRINTF(L"Key = %s", Key.c_str());
  Result = (IndexOfName(Key.c_str()) != NPOS);
  // DEBUG_PRINTF(L"Result = %d", Result);
  if (Result)
  {
    TargetDir = GetValue(Key);
    // TargetDir is not "//" here only when Change is full path to symbolic link
    // DEBUG_PRINTF(L"TargetDir = %s", TargetDir.c_str());
    if (TargetDir == L"//")
    {
      TargetDir = Key;
    }
    // DEBUG_PRINTF(L"TargetDir = %s", TargetDir.c_str());
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
        // DEBUG_PRINTF(L"TargetDir = %s", TargetDir.c_str());
      }
    }
  }
  // DEBUG_PRINTF(L"end, Result = %d, TargetDir = %s", Result, TargetDir.c_str());
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Serialize(UnicodeString & Data)
{
  Data = L"A";
  size_t ACount = GetCount();
  if (ACount > FMaxSize)
  {
    System::TStrings * Limited = new System::TStringList();
    {
      BOOST_SCOPE_EXIT ( (&Limited) )
      {
        delete Limited;
      } BOOST_SCOPE_EXIT_END
      size_t Index = ACount - FMaxSize;
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
void TRemoteDirectoryChangesCache::Deserialize(const UnicodeString Data)
{
  // DEBUG_PRINTF(L"Data = %s", Data.c_str());
  if (Data.IsEmpty())
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
  const UnicodeString SourceDir, const UnicodeString Change, UnicodeString & Key)
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
  assert(Source);
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
void TRights::SetText(const UnicodeString value)
{
  if (value != GetText())
  {
    // DEBUG_PRINTF(L"value = %s, GetText = %s", value.c_str(), GetText().c_str());
    if ((value.Length() != TextLen) ||
        (!GetAllowUndef() && (value.Pos(UndefSymbol) != UnicodeString::npos)) ||
        (value.Pos(L" ") != UnicodeString::npos))
    {
      throw ExtException(FMTLOAD(RIGHTS_ERROR, value.c_str()));
    }

    FSet = 0;
    // DEBUG_PRINTF(L"FSet = %o", FSet);
    FUnset = 0;
    int Flag = 00001;
    int ExtendedFlag = 01000;
    bool KeepText = false;
    std::string val = System::W2MB(value.c_str());
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
      else if (val[i] == CombinedSymbols[i])
      {
        FSet |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (val[i] == ExtendedSymbols[i])
      {
        FSet |= static_cast<unsigned short>(ExtendedFlag);
        FUnset |= static_cast<unsigned short>(Flag);
      }
      else
      {
        if (val[i] != BasicSymbols[i])
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

    FText = KeepText ? value : UnicodeString();
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetText() const
{
  // DEBUG_PRINTF(L"FSet = %o, FText = %s", FSet, FText.c_str());
  if (!FText.IsEmpty())
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
    // DEBUG_PRINTF(L"Result = %s", System::MB2W(Result.c_str()).c_str());
    return System::MB2W(Result.c_str());
  }
}
//---------------------------------------------------------------------------
void TRights::SetOctal(const UnicodeString value)
{
  std::string AValue(System::W2MB(value.c_str()));
  if (AValue.Length() == 3)
  {
    AValue = "0" + AValue;
  }

  if (GetOctal() != System::MB2W(AValue.c_str()))
  {
    bool Correct = (AValue.Length() == 4);
    if (Correct)
    {
      for (size_t i = 0; (i < AValue.Length()) && Correct; i++)
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
UnicodeString TRights::GetOctal() const
{
  std::string Result;
  unsigned short N = GetNumberSet(); // used to be "Number"
  Result.resize(4);
  Result[0] = static_cast<char>('0' + ((N & 07000) >> 9));
  Result[1] = static_cast<char>('0' + ((N & 00700) >> 6));
  Result[2] = static_cast<char>('0' + ((N & 00070) >> 3));
  Result[3] = static_cast<char>('0' + ((N & 00007) >> 0));

  return System::MB2W(Result.c_str());
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
  size_t Index;

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
        Result += ',';
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
TRemoteProperties TRemoteProperties::CommonProperties(System::TStrings * FileList)
{
  // TODO: Modification and LastAccess
  TRemoteProperties CommonProperties;
  for (size_t Index = 0; Index < FileList->GetCount(); Index++)
  {
    TRemoteFile * File = static_cast<TRemoteFile *>(FileList->GetObject(Index));
    assert(File);
    if (!Index)
    {
      CommonProperties.Rights.Assign(File->GetRights());
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
