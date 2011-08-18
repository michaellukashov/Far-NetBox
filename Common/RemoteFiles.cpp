//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "RemoteFiles.h"

#include <SysUtils.hpp>

#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
/* TODO 1 : Path class instead of wstring (handle relativity...) */
//---------------------------------------------------------------------------
wstring UnixIncludeTrailingBackslash(const wstring Path)
{
  // it used to return "/" when input path was empty
  if (!Path.IsEmpty() && !Path.IsDelimiter("/", Path.Length()))
  {
    return Path + "/";
  }
  else
  {
    return Path;
  }
}
//---------------------------------------------------------------------------
// Keeps "/" for root path
wstring UnixExcludeTrailingBackslash(const wstring Path)
{
  if ((Path.Length() > 1) && Path.IsDelimiter("/", Path.Length()))
      return Path.SubString(1, Path.Length() - 1);
    else return Path;
}
//---------------------------------------------------------------------------
bool UnixComparePaths(const wstring Path1, const wstring Path2)
{
  return (UnixIncludeTrailingBackslash(Path1) == UnixIncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool UnixIsChildPath(wstring Parent, wstring Child)
{
  Parent = UnixIncludeTrailingBackslash(Parent);
  Child = UnixIncludeTrailingBackslash(Child);
  return (Child.SubString(1, Parent.Length()) == Parent);
}
//---------------------------------------------------------------------------
wstring UnixExtractFileDir(const wstring Path)
{
  int Pos = Path.LastDelimiter('/');
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return Path.SubString(1, Pos - 1);
  }
  else
  {
    return (Pos == 1) ? wstring("/") : wstring();
  }
}
//---------------------------------------------------------------------------
// must return trailing backslash
wstring UnixExtractFilePath(const wstring Path)
{
  int Pos = Path.LastDelimiter('/');
  // it used to return Path when no slash was found
  return (Pos > 0) ? Path.SubString(1, Pos) : wstring();
}
//---------------------------------------------------------------------------
wstring UnixExtractFileName(const wstring Path)
{
  int Pos = Path.LastDelimiter('/');
  wstring Result;
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
wstring UnixExtractFileExt(const wstring Path)
{
  wstring FileName = UnixExtractFileName(Path);
  int Pos = FileName.LastDelimiter(".");
  return (Pos > 0) ? Path.SubString(Pos, Path.Length() - Pos + 1) : wstring();
}
//---------------------------------------------------------------------------
wstring ExtractFileName(const wstring & Path, bool Unix)
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
bool ExtractCommonPath(TStrings * Files, wstring & Path)
{
  assert(Files->Count > 0);

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
          Path = "";
          Result = false;
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool UnixExtractCommonPath(TStrings * Files, wstring & Path)
{
  assert(Files->Count > 0);

  Path = UnixExtractFilePath(Files->Strings[0]);
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (int Index = 1; Index < Files->Count; Index++)
    {
      while (!Path.IsEmpty() &&
        (Files->Strings[Index].SubString(1, Path.Length()) != Path))
      {
        int PrevLen = Path.Length();
        Path = UnixExtractFilePath(UnixExcludeTrailingBackslash(Path));
        if (Path.Length() == PrevLen)
        {
          Path = "";
          Result = false;
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool IsUnixRootPath(const wstring Path)
{
  return Path.IsEmpty() || (Path == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
bool IsUnixHiddenFile(const wstring FileName)
{
  return (FileName != ROOTDIRECTORY) && (FileName != PARENTDIRECTORY) &&
    !FileName.IsEmpty() && (FileName[1] == '.');
}
//---------------------------------------------------------------------------
wstring AbsolutePath(const wstring & Base, const wstring & Path)
{
  wstring Result;
  if (Path.IsEmpty())
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
    while ((P = Result.Pos("/../")) > 0)
    {
      int P2 = Result.SubString(1, P-1).LastDelimiter("/");
      assert(P2 > 0);
      Result.Delete(P2, P - P2 + 3);
    }
    while ((P = Result.Pos("/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
wstring FromUnixPath(const wstring Path)
{
  return StringReplace(Path, "/", "\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
wstring ToUnixPath(const wstring Path)
{
  return StringReplace(Path, "\\", "/", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
static void CutFirstDirectory(wstring & S, bool Unix)
{
  bool Root;
  int P;
  wstring Sep = Unix ? "/" : "\\";
  if (S == Sep)
  {
    S = "";
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
    if (S[1] == '.')
    {
      S.Delete(1, 4);
    }
    P = S.AnsiPos(Sep[1]);
    if (P)
    {
      S.Delete(1, P);
      S = "..." + Sep + S;
    }
    else
    {
      S = "";
    }
    if (Root)
    {
      S = Sep + S;
    }
  }
}
//---------------------------------------------------------------------------
wstring MinimizeName(const wstring FileName, int MaxLen, bool Unix)
{
  wstring Drive, Dir, Name, Result;
  wstring Sep = Unix ? "/" : "\\";

  Result = FileName;
  if (Unix)
  {
    int P = Result.LastDelimiter("/");
    if (P)
    {
      Dir = Result.SubString(1, P);
      Name = Result.SubString(P + 1, Result.Length() - P);
    }
    else
    {
      Dir = "";
      Name = Result;
    }
  }
  else
  {
    Dir = ExtractFilePath(Result);
    Name = ExtractFileName(Result);

    if (Dir.Length() >= 2 && Dir[2] == ':')
    {
      Drive = Dir.SubString(1, 2);
      Dir.Delete(1, 2);
    }
  }

  while ((!Dir.IsEmpty() || !Drive.IsEmpty()) && (Result.Length() > MaxLen))
  {
    if (Dir == Sep + "..." + Sep)
    {
      Dir = "..." + Sep;
    }
    else if (Dir == "")
    {
      Drive = "";
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
wstring MakeFileList(TStrings * FileList)
{
  wstring Result;
  for (int Index = 0; Index < FileList->Count; Index++)
  {
    if (!Result.IsEmpty())
    {
      Result += " ";
    }

    wstring FileName = FileList->Strings[Index];
    // currently this is used for local file only, so no delimiting is done
    if (FileName.Pos(" ") > 0)
    {
      Result += "\"" + FileName + "\"";
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
wstring UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision)
{
  switch (Precision)
  {
    case mfNone:
      return "";
    case mfMDY:
      return FormatDateTime("ddddd", DateTime);
    case mfMDHM:
      return FormatDateTime("ddddd t", DateTime);
    case mfFull:
    default:
      return FormatDateTime("ddddd tt", DateTime);
  }
}
//---------------------------------------------------------------------------
int FakeFileImageIndex(wstring FileName, unsigned long Attrs,
  wstring * TypeName)
{
  Attrs |= FILE_ATTRIBUTE_NORMAL;

  TSHFileInfo SHFileInfo;
  // On Win2k we get icon of "ZIP drive" for ".." (parent directory)
  if ((FileName == "..") ||
      ((FileName.Length() == 2) && (FileName[2] == ':') &&
       (tolower(FileName[1]) >= 'a') && (tolower(FileName[1]) <= 'z')) ||
      IsReservedName(FileName))
  {
    FileName = "dumb";
  }
  // this should be somewhere else, probably in TUnixDirView,
  // as the "partial" overlay is added there too
  if (AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT))
  {
    static const size_t PartialExtLen = sizeof(PARTIAL_EXT) - 1;
    FileName.SetLength(FileName.Length() - PartialExtLen);
  }

  int Icon;
  if (SHGetFileInfo(FileName.c_str(),
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
      *TypeName = "";
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
TRemoteToken::TRemoteToken(const wstring & Name) :
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
void TRemoteToken::SetID(unsigned int value)
{
  FID = value;
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
wstring TRemoteToken::GetDisplayText() const
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
    return wstring();
  }
}
//---------------------------------------------------------------------------
wstring TRemoteToken::GetLogText() const
{
  return FORMAT("\"%s\" [%d]", (FName, int(FID)));
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
  if (Token.IDValid)
  {
    std::pair<TIDMap::iterator, bool> Position =
      FIDMap.insert(TIDMap::value_type(Token.ID, FTokens.size() - 1));
  }
  if (Token.NameValid)
  {
    std::pair<TNameMap::iterator, bool> Position =
      FNameMap.insert(TNameMap::value_type(Token.Name, FTokens.size() - 1));
  }
}
//---------------------------------------------------------------------------
void TRemoteTokenList::AddUnique(const TRemoteToken & Token)
{
  if (Token.IDValid)
  {
    TIDMap::const_iterator I = FIDMap.find(Token.ID);
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
  else if (Token.NameValid)
  {
    TNameMap::const_iterator I = FNameMap.find(Token.Name);
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
bool TRemoteTokenList::Exists(const wstring & Name) const
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
const TRemoteToken * TRemoteTokenList::Find(const wstring & Name) const
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
    Terminal->LogEvent(FORMAT("Following %s found:", (Title)));
    for (size_t Index = 0; Index < FTokens.size(); Index++)
    {
      Terminal->LogEvent(wstring("  ") + FTokens[Index].LogText);
    }
  }
  else
  {
    Terminal->LogEvent(FORMAT("No %s found.", (Title)));
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
    *Result->Rights = *FRights;
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
    if (Standalone && (!FFullFileName.IsEmpty() || (Directory != NULL)))
    {
      Result->FFullFileName = FullFileName;
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
  if (IsDirectory) Attrs |= FILE_ATTRIBUTE_DIRECTORY;
  if (IsHidden) Attrs |= FILE_ATTRIBUTE_HIDDEN;

  wstring DumbFileName = (IsSymLink && !LinkTo.IsEmpty() ? LinkTo : FileName);

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
wstring TRemoteFile::GetTypeName()
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
      Result = IsUnixHiddenFile(FileName);
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
  return (toupper(Type) == FILETYPE_DIRECTORY);
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsParentDirectory() const
{
  return (FileName == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsThisDirectory() const
{
  return (FileName == THISDIRECTORY);
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetIsInaccesibleDirectory() const
{
  bool Result;
  if (IsDirectory)
  {
    assert(Terminal);
    Result = !
       (((Rights->RightUndef[TRights::rrOtherExec] != TRights::rsNo)) ||
        ((Rights->Right[TRights::rrGroupExec] != TRights::rsNo) &&
         Terminal->Membership->Exists(Group.Name)) ||
        ((Rights->Right[TRights::rrUserExec] != TRights::rsNo) &&
         (AnsiCompareText(Terminal->UserName, Owner.Name) == 0)));
  }
    else Result = false;
  return Result;
}
//---------------------------------------------------------------------------
char TRemoteFile::GetType() const
{
  if (IsSymLink && FLinkedFile) return FLinkedFile->Type;
    else return FType;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetType(char AType)
{
  FType = AType;
  // Allow even non-standard file types (e.g. 'S')
  // if (!wstring("-DL").Pos((char)toupper(FType))) Abort();
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
  assert(Terminal);
  // If file is symlink but we couldn't find linked file we assume broken link
  return (IsSymLink && (FCyclicLink || !FLinkedFile) &&
    Terminal->ResolvingSymlinks);
  // "!FLinkTo.IsEmpty()" removed because it does not work with SFTP
}
//---------------------------------------------------------------------------
void TRemoteFile::ShiftTime(const TDateTime & Difference)
{
  double D = double(Difference);
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
wstring TRemoteFile::GetUserModificationStr()
{
  return ::UserModificationStr(Modification, FModificationFmt);
}
//---------------------------------------------------------------------------
wstring TRemoteFile::GetModificationStr()
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  Modification.DecodeDate(&Year, &Month, &Day);
  Modification.DecodeTime(&Hour, &Min, &Sec, &MSec);
  switch (FModificationFmt)
  {
    case mfNone:
      return "";

    case mfMDY:
      return FORMAT("%3s %2d %2d", (EngShortMonthNames[Month-1], Day, Year));

    case mfMDHM:
      return FORMAT("%3s %2d %2d:%2.2d",
        (EngShortMonthNames[Month-1], Day, Hour, Min));

    default:
      assert(false);
      // fall thru

    case mfFull:
      return FORMAT("%3s %2d %2d:%2.2d:%2.2d %4d",
        (EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year));
  }
}
//---------------------------------------------------------------------------
wstring TRemoteFile::GetExtension()
{
  return UnixExtractFileExt(FFileName);
}
//---------------------------------------------------------------------------
void TRemoteFile::SetRights(TRights * value)
{
  FRights->Assign(value);
}
//---------------------------------------------------------------------------
wstring TRemoteFile::GetRightsStr()
{
  return FRights->Unknown ? wstring() : FRights->Text;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetListingStr(wstring value)
{
  // Value stored in 'value' can be used for error message
  wstring Line = value;
  FIconIndex = -1;
  try
  {
    wstring Col;

    // Do we need to do this (is ever TAB is LS output)?
    Line = ReplaceChar(Line, '\t', ' ');

    Type = Line[1];
    Line.Delete(1, 1);

    #define GETNCOL  \
      { if (Line.IsEmpty()) throw exception(""); \
        int P = Line.Pos(' '); \
        if (P) { Col = Line.SubString(1, P-1); Line.Delete(1, P); } \
          else { Col = Line; Line = ""; } \
      }
    #define GETCOL { GETNCOL; Line = TrimLeft(Line); }

    // Rights string may contain special permission attributes (S,t, ...)
    // (TODO: maybe no longer necessary, once we can handle the special permissions)
    Rights->AllowUndef = true;
    // On some system there is no space between permissions and node blocks count columns
    // so we get only first 9 characters and trim all following spaces (if any)
    Rights->Text = Line.SubString(1, 9);
    Line.Delete(1, 9);
    // Rights column maybe followed by '+', '@' or '.' signs, we ignore them
    // (On MacOS, there may be a space in between)
    if (!Line.IsEmpty() && ((Line[1] == '+') || (Line[1] == '@') || (Line[1] == '.')))
    {
      Line.Delete(1, 1);
    }
    else if ((Line.Length() >= 2) && (Line[1] == ' ') &&
             ((Line[2] == '+') || (Line[2] == '@') || (Line[2] == '.')))
    {
      Line.Delete(1, 2);
    }
    Line = Line.TrimLeft();

    GETCOL;
    FINodeBlocks = StrToInt(Col);

    GETCOL;
    FOwner.Name = Col;

    // #60 17.10.01: group name can contain space
    FGroup.Name = "";
    GETCOL;
    __int64 ASize;
    do
    {
      FGroup.Name = FGroup.Name + Col;
      GETCOL;
      assert(!Col.IsEmpty());
      // for devices etc.. there is additional column ending by comma, we ignore it
      if (Col[Col.Length()] == ',') GETCOL;
      ASize = StrToInt64Def(Col, -1);
      // if it's not a number (file size) we take it as part of group name
      // (at least on CygWin, there can be group with space in its name)
      if (ASize < 0) Col = " " + Col;
    }
    while (ASize < 0);

    // do not read modification time and filename if it is already set
    if (double(FModification) == 0 && FileName.IsEmpty())
    {
      FSize = ASize;

      bool FullTime = false;
      bool DayMonthFormat = false;
      Word Day, Month, Year, Hour, Min, Sec, P;

      GETCOL;
      // format dd mmm or mmm dd ?
      Day = (Word)StrToIntDef(Col, 0);
      if (Day > 0)
      {
        DayMonthFormat = true;
        GETCOL;
      }
      Month = 0;
      #define COL2MONTH \
        for (Word IMonth = 0; IMonth < 12; IMonth++) \
          if (!Col.AnsiCompareIC(EngShortMonthNames[IMonth])) { Month = IMonth; Month++; break; }
      COL2MONTH;
      // if the column is not known month name, it may have been "yyyy-mm-dd"
      // for --full-time format
      if ((Month == 0) && (Col.Length() == 10) && (Col[5] == '-') && (Col[8] == '-'))
      {
        Year = (Word)Col.SubString(1, 4).ToInt();
        Month = (Word)Col.SubString(6, 2).ToInt();
        Day = (Word)Col.SubString(9, 2).ToInt();
        GETCOL;
        Hour = (Word)Col.SubString(1, 2).ToInt();
        Min = (Word)Col.SubString(4, 2).ToInt();
        Sec = (Word)Col.SubString(7, 2).ToInt();
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
        if ((Day < 1) || (Day > 31)) Abort();

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
          if ((P = (Word)Col.Pos(':')) > 0)
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
                (Month == CurrMonth && Day > CurrDay)) Year--;
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
        assert(Terminal != NULL);
        FModification = AdjustDateTimeFromUnix(FModification,
          Terminal->SessionData->DSTMode);
      }

      if (double(FLastAccess) == 0)
      {
        FLastAccess = FModification;
      }

      // separating space is already deleted, other spaces are treated as part of name

      {
        int P;

        FLinkTo = "";
        if (IsSymLink)
        {
          P = Line.Pos(SYMLINKSTR);
          if (P)
          {
            FLinkTo = Line.SubString(
              P + strlen(SYMLINKSTR), Line.Length() - P + strlen(SYMLINKSTR) + 1);
            Line.SetLength(P - 1);
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
  catch (exception &E)
  {
    throw ETerminal(&E, FmtLoadStr(LIST_LINE_ERROR, ARRAYOFCONST((value))));
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::Complete()
{
  assert(Terminal != NULL);
  if (IsSymLink && Terminal->ResolvingSymlinks)
  {
    FindLinkedFile();
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::FindLinkedFile()
{
  assert(Terminal && IsSymLink);

  if (FLinkedFile) delete FLinkedFile;
  FLinkedFile = NULL;

  FCyclicLink = false;
  if (!LinkTo.IsEmpty())
  {
    // check for cyclic link
    TRemoteFile * LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      if (LinkedBy->LinkTo == LinkTo)
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
    assert(Terminal->ResolvingSymlinks);
    Terminal->ExceptionOnFail = true;
    try
    {
      try
      {
        Terminal->ReadSymlink(this, FLinkedFile);
      }
      catch(...)
      {
        Terminal->ExceptionOnFail = false;
      }
    }
    catch (exception &E)
    {
      if (E.InheritsFrom(__classid(EFatal))) throw;
        else Terminal->Log->AddException(&E);
    }
  }
}
//---------------------------------------------------------------------------
wstring TRemoteFile::GetListingStr()
{
  // note that ModificationStr is longer than 12 for mfFull
  wstring LinkPart;
  // expanded from ?: to avoid memory leaks
  if (IsSymLink)
  {
    LinkPart = wstring(SYMLINKSTR) + LinkTo;
  }
  return Format("%s%s %3s %-8s %-8s %9s %-12s %s%s", ARRAYOFCONST((
    Type, Rights->Text, IntToStr(INodeBlocks), Owner.Name,
    Group.Name, IntToStr(Size), ModificationStr, FileName,
    LinkPart)));
}
//---------------------------------------------------------------------------
wstring TRemoteFile::GetFullFileName() const
{
  if (FFullFileName.IsEmpty())
  {
    assert(Terminal);
    assert(Directory != NULL);
    wstring Path;
    if (IsParentDirectory) Path = Directory->ParentPath;
      else
    if (IsDirectory) Path = UnixIncludeTrailingBackslash(Directory->FullDirectory + FileName);
      else Path = Directory->FullDirectory + FileName;
    return Terminal->TranslateLockedPath(Path, true);
  }
  else
  {
    return FFullFileName;
  }
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetHaveFullFileName() const
{
  return !FFullFileName.IsEmpty() || (Directory != NULL);
}
//---------------------------------------------------------------------------
int TRemoteFile::GetAttr()
{
  int Result = 0;
  if (Rights->ReadOnly) Result |= faReadOnly;
  if (IsHidden) Result |= faHidden;
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetTerminal(TTerminal * value)
{
  FTerminal = value;
  if (FLinkedFile)
  {
    FLinkedFile->Terminal = value;
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryFile::TRemoteDirectoryFile() : TRemoteFile()
{
  Modification = double(0);
  ModificationFmt = mfNone;
  LastAccess = Modification;
  Type = 'D';
  Size = 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteParentDirectory::TRemoteParentDirectory(TTerminal * ATerminal)
  : TRemoteDirectoryFile()
{
  FileName = PARENTDIRECTORY;
  Terminal = ATerminal;
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
  File->Directory = this;
}
//---------------------------------------------------------------------------
void TRemoteFileList::DuplicateTo(TRemoteFileList * Copy)
{
  Copy->Clear();
  for (int Index = 0; Index < Count; Index++)
  {
    TRemoteFile * File = Files[Index];
    Copy->AddFile(File->Duplicate(false));
  }
  Copy->FDirectory = Directory;
  Copy->FTimestamp = FTimestamp;
}
//---------------------------------------------------------------------------
void TRemoteFileList::Clear()
{
  FTimestamp = Now();
  TObjectList::Clear();
}
//---------------------------------------------------------------------------
void TRemoteFileList::SetDirectory(wstring value)
{
  FDirectory = UnixExcludeTrailingBackslash(value);
}
//---------------------------------------------------------------------------
wstring TRemoteFileList::GetFullDirectory()
{
  return UnixIncludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::GetFiles(int Index)
{
  return (TRemoteFile *)Items[Index];
}
//---------------------------------------------------------------------------
bool TRemoteFileList::GetIsRoot()
{
  return (Directory == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
wstring TRemoteFileList::GetParentPath()
{
  return UnixExtractFilePath(Directory);
}
//---------------------------------------------------------------------------
__int64 TRemoteFileList::GetTotalSize()
{
  __int64 Result = 0;
  for (int Index = 0; Index < Count; Index++)
    if (!Files[Index]->IsDirectory) Result += Files[Index]->Size;
  return Result;
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::FindFile(const wstring &FileName)
{
  for (int Index = 0; Index < Count; Index++)
    if (Files[Index]->FileName == FileName) return Files[Index];
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
  if (ThisDirectory && !IncludeThisDirectory)
  {
    delete FThisDirectory;
    FThisDirectory = NULL;
  }
  if (ParentDirectory && !IncludeParentDirectory)
  {
    delete FParentDirectory;
    FParentDirectory = NULL;
  }

  TRemoteFileList::Clear();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetDirectory(wstring value)
{
  TRemoteFileList::SetDirectory(value);
  //Load();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::AddFile(TRemoteFile * File)
{
  if (File->IsThisDirectory) FThisDirectory = File;
  if (File->IsParentDirectory) FParentDirectory = File;

  if ((!File->IsThisDirectory || IncludeThisDirectory) &&
      (!File->IsParentDirectory || IncludeParentDirectory))
  {
    TRemoteFileList::AddFile(File);
  }
  File->Terminal = Terminal;
}
//---------------------------------------------------------------------------
void TRemoteDirectory::DuplicateTo(TRemoteFileList * Copy)
{
  TRemoteFileList::DuplicateTo(Copy);
  if (ThisDirectory && !IncludeThisDirectory)
  {
    Copy->AddFile(ThisDirectory->Duplicate(false));
  }
  if (ParentDirectory && !IncludeParentDirectory)
  {
    Copy->AddFile(ParentDirectory->Duplicate(false));
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectory::GetLoaded()
{
  return ((Terminal != NULL) && Terminal->Active && !Directory.IsEmpty());
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

  for (int Index = 0; Index < Count; Index ++)
  {
    if (Files[Index]->Selected)
    {
      FSelectedFiles->Add(Files[Index]->FullFileName);
    }
  }

  return FSelectedFiles;
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeParentDirectory(bool value)
{
  if (IncludeParentDirectory != value)
  {
    FIncludeParentDirectory = value;
    if (value && ParentDirectory)
    {
      assert(IndexOf(ParentDirectory) < 0);
      Add(ParentDirectory);
    }
    else if (!value && ParentDirectory)
    {
      assert(IndexOf(ParentDirectory) >= 0);
      Extract(ParentDirectory);
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeThisDirectory(bool value)
{
  if (IncludeThisDirectory != value)
  {
    FIncludeThisDirectory = value;
    if (value && ThisDirectory)
    {
      assert(IndexOf(ThisDirectory) < 0);
      Add(ThisDirectory);
    }
    else if (!value && ThisDirectory)
    {
      assert(IndexOf(ThisDirectory) >= 0);
      Extract(ThisDirectory);
    }
  }
}
//===========================================================================
TRemoteDirectoryCache::TRemoteDirectoryCache(): TStringList()
{
  FSection = new TCriticalSection();
  Sorted = true;
  Duplicates = dupError;
  CaseSensitive = true;
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

  try
  {
    for (int Index = 0; Index < Count; Index++)
    {
      delete (TRemoteFileList *)Objects[Index];
      Objects[Index] = NULL;
    }
  }
  catch(...)
  {
    TStringList::Clear();
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetIsEmpty() const
{
  TGuard Guard(FSection);

  return (const_cast<TRemoteDirectoryCache*>(this)->Count == 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasFileList(const wstring Directory)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasNewerFileList(const wstring Directory,
  TDateTime Timestamp)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
  if (Index >= 0)
  {
    TRemoteFileList * FileList = dynamic_cast<TRemoteFileList *>(Objects[Index]);
    if (FileList->Timestamp <= Timestamp)
    {
      Index = -1;
    }
  }
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetFileList(const wstring Directory,
  TRemoteFileList * FileList)
{
  TGuard Guard(FSection);

  int Index = IndexOf(UnixExcludeTrailingBackslash(Directory));
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
  assert(FileList);
  TRemoteFileList * Copy = new TRemoteFileList();
  FileList->DuplicateTo(Copy);

  {
    TGuard Guard(FSection);

    // file list cannot be cached already with only one thread, but it can be
    // when directory is loaded by secondary terminal
    DoClearFileList(FileList->Directory, false);
    AddObject(Copy->Directory, Copy);
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::ClearFileList(wstring Directory, bool SubDirs)
{
  TGuard Guard(FSection);
  DoClearFileList(Directory, SubDirs);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::DoClearFileList(wstring Directory, bool SubDirs)
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
      if (Strings[Index].SubString(1, Directory.Length()) == Directory)
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
  delete (TRemoteFileList *)Objects[Index];
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
  return (const_cast<TRemoteDirectoryChangesCache*>(this)->Count == 0);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::SetValue(const wstring & Name,
  const wstring & Value)
{
  int Index = IndexOfName(Name);
  if (Index > 0)
  {
    Delete(Index);
  }
  Values[Name] = Value;
}
//---------------------------------------------------------------------------
wstring TRemoteDirectoryChangesCache::GetValue(const wstring & Name)
{
  wstring Value = Values[Name];
  SetValue(Name, Value);
  return Value;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::AddDirectoryChange(
  const wstring SourceDir, const wstring Change,
  const wstring TargetDir)
{
  assert(!TargetDir.IsEmpty());
  SetValue(TargetDir, "//");
  if (TTerminal::ExpandFileName(Change, SourceDir) != TargetDir)
  {
    wstring Key;
    if (DirectoryChangeKey(SourceDir, Change, Key))
    {
      SetValue(Key, TargetDir);
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChange(
  wstring SourceDir)
{
  for (int Index = 0; Index < Count; Index++)
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
  wstring TargetDir)
{
  wstring Key;
  // hack to clear at least local sym-link change in case symlink is deleted
  DirectoryChangeKey(UnixExcludeTrailingBackslash(UnixExtractFilePath(TargetDir)),
    UnixExtractFileName(TargetDir), Key);

  for (int Index = 0; Index < Count; Index++)
  {
    wstring Name = Names[Index];
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
  const wstring SourceDir, const wstring Change, wstring & TargetDir)
{
  wstring Key;
  bool Result;
  Key = TTerminal::ExpandFileName(Change, SourceDir);
  Result = (IndexOfName(Key) >= 0);
  if (Result)
  {
    TargetDir = GetValue(Key);
    // TargetDir is not "//" here only when Change is full path to symbolic link
    if (TargetDir == "//")
    {
      TargetDir = Key;
    }
  }
  else
  {
    Result = DirectoryChangeKey(SourceDir, Change, Key);
    if (Result)
    {
      wstring Directory = GetValue(Key);
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
void TRemoteDirectoryChangesCache::Serialize(wstring & Data)
{
  Data = "A";
  int ACount = Count;
  if (ACount > FMaxSize)
  {
    TStrings * Limited = new TStringList();
    try
    {
      int Index = ACount - FMaxSize;
      while (Index < ACount)
      {
        Limited->Add(Strings[Index]);
        Index++;
      }
      Data += Limited->Text;
    }
    catch(...)
    {
      delete Limited;
    }
  }
  else
  {
    Data += Text;
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Deserialize(const wstring Data)
{
  if (Data.IsEmpty())
  {
    Text = "";
  }
  else
  {
    Text = Data.c_str() + 1;
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::DirectoryChangeKey(
  const wstring SourceDir, const wstring Change, wstring & Key)
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
        Key = SourceDir + "," + Change;
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
  FUnset = 0;
  Number = 0;
  FUnknown = true;
}
//---------------------------------------------------------------------------
TRights::TRights(unsigned short ANumber)
{
  FAllowUndef = false;
  FSet = 0;
  FUnset = 0;
  Number = ANumber;
}
//---------------------------------------------------------------------------
TRights::TRights(const TRights & Source)
{
  Assign(&Source);
}
//---------------------------------------------------------------------------
void TRights::Assign(const TRights * Source)
{
  FAllowUndef = Source->AllowUndef;
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
  if (AllowUndef || rhr.AllowUndef)
  {
    for (int Right = rrFirst; Right <= rrLast; Right++)
    {
      if (RightUndef[static_cast<TRight>(Right)] !=
            rhr.RightUndef[static_cast<TRight>(Right)])
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    return (Number == rhr.Number);
  }
}
//---------------------------------------------------------------------------
bool TRights::operator ==(unsigned short rhr) const
{
  return (Number == rhr);
}
//---------------------------------------------------------------------------
bool TRights::operator !=(const TRights & rhr) const
{
  return !(*this == rhr);
}
//---------------------------------------------------------------------------
TRights & TRights::operator =(unsigned short rhr)
{
  Number = rhr;
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
  TRights Result(static_cast<unsigned short>(~Number));
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
  if (AllowUndef || rhr.AllowUndef)
  {
    for (int Right = rrFirst; Right <= rrLast; Right++)
    {
      if (RightUndef[static_cast<TRight>(Right)] !=
            rhr.RightUndef[static_cast<TRight>(Right)])
      {
        RightUndef[static_cast<TRight>(Right)] = rsUndef;
      }
    }
  }
  else
  {
    Number &= rhr.Number;
  }
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator &=(unsigned short rhr)
{
  Number &= rhr;
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
  Number |= rhr.Number;
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator |=(unsigned short rhr)
{
  Number |= rhr;
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
void TRights::SetText(const wstring & value)
{
  if (value != Text)
  {
    if ((value.Length() != TextLen) ||
        (!AllowUndef && (value.Pos(UndefSymbol) > 0)) ||
        (value.Pos(" ") > 0))
    {
      throw exception(FMTLOAD(RIGHTS_ERROR, (value)));
    }

    FSet = 0;
    FUnset = 0;
    int Flag = 00001;
    int ExtendedFlag = 01000;
    bool KeepText = false;
    for (int i = TextLen; i >= 1; i--)
    {
      if (value[i] == UnsetSymbol)
      {
        FUnset |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (value[i] == UndefSymbol)
      {
        // do nothing
      }
      else if (value[i] == CombinedSymbols[i - 1])
      {
        FSet |= static_cast<unsigned short>(Flag | ExtendedFlag);
      }
      else if (value[i] == ExtendedSymbols[i - 1])
      {
        FSet |= static_cast<unsigned short>(ExtendedFlag);
        FUnset |= static_cast<unsigned short>(Flag);
      }
      else
      {
        if (value[i] != BasicSymbols[i - 1])
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

    FText = KeepText ? value : wstring();
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
wstring TRights::GetText() const
{
  if (!FText.IsEmpty())
  {
    return FText;
  }
  else
  {
    wstring Result;
    Result.SetLength(TextLen);

    int Flag = 00001;
    int ExtendedFlag = 01000;
    bool ExtendedPos = true;
    char Symbol;
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
void TRights::SetOctal(wstring value)
{
  wstring AValue(value);
  if (AValue.Length() == 3)
  {
    AValue = "0" + AValue;
  }

  if (Octal != AValue)
  {
    bool Correct = (AValue.Length() == 4);
    if (Correct)
    {
      for (int i = 1; (i <= AValue.Length()) && Correct; i++)
      {
        Correct = (AValue[i] >= '0') && (AValue[i] <= '7');
      }
    }

    if (!Correct)
    {
      throw exception(FMTLOAD(INVALID_OCTAL_PERMISSIONS, (value)));
    }

    Number = static_cast<unsigned short>(
      ((AValue[1] - '0') << 9) +
      ((AValue[2] - '0') << 6) +
      ((AValue[3] - '0') << 3) +
      ((AValue[4] - '0') << 0));
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
unsigned long TRights::GetNumberDecadic() const
{
  unsigned long N = NumberSet; // used to be "Number"
  unsigned long Result =
      ((N & 07000) / 01000 * 1000) +
      ((N & 00700) /  0100 *  100) +
      ((N & 00070) /   010 *   10) +
      ((N & 00007) /    01 *    1);

  return Result;
}
//---------------------------------------------------------------------------
wstring TRights::GetOctal() const
{
  wstring Result;
  unsigned short N = NumberSet; // used to be "Number"
  Result.SetLength(4);
  Result[1] = static_cast<char>('0' + ((N & 07000) >> 9));
  Result[2] = static_cast<char>('0' + ((N & 00700) >> 6));
  Result[3] = static_cast<char>('0' + ((N & 00070) >> 3));
  Result[4] = static_cast<char>('0' + ((N & 00007) >> 0));

  return Result;
}
//---------------------------------------------------------------------------
void TRights::SetNumber(unsigned short value)
{
  if ((FSet != value) || ((FSet | FUnset) != rfAllSpecials))
  {
    FSet = value;
    FUnset = static_cast<unsigned short>(rfAllSpecials & ~FSet);
    FText = "";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
unsigned short TRights::GetNumber() const
{
  assert(!IsUndef);
  return FSet;
}
//---------------------------------------------------------------------------
void TRights::SetRight(TRight Right, bool value)
{
  RightUndef[Right] = (value ? rsYes : rsNo);
}
//---------------------------------------------------------------------------
bool TRights::GetRight(TRight Right) const
{
  TState State = RightUndef[Right];
  assert(State != rsUndef);
  return (State == rsYes);
}
//---------------------------------------------------------------------------
void TRights::SetRightUndef(TRight Right, TState value)
{
  if (value != RightUndef[Right])
  {
    assert((value != rsUndef) || AllowUndef);

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

    FText = "";
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
  Right[rrUserWrite] = !value;
  Right[rrGroupWrite] = !value;
  Right[rrOtherWrite] = !value;
}
//---------------------------------------------------------------------------
bool  TRights::GetReadOnly()
{
  return Right[rrUserWrite] && Right[rrGroupWrite] && Right[rrOtherWrite];
}
//---------------------------------------------------------------------------
wstring TRights::GetSimplestStr() const
{
  if (IsUndef)
  {
    return ModeStr;
  }
  else
  {
    return Octal;
  }
}
//---------------------------------------------------------------------------
wstring TRights::GetModeStr() const
{
  wstring Result;
  wstring SetModeStr, UnsetModeStr;
  TRight Right;
  int Index;

  for (int Group = 0; Group < 3; Group++)
  {
    SetModeStr = "";
    UnsetModeStr = "";
    for (int Mode = 0; Mode < 3; Mode++)
    {
      Index = (Group * 3) + Mode;
      Right = static_cast<TRight>(rrUserRead + Index);
      switch (RightUndef[Right])
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
    switch (RightUndef[Right])
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
        Result += "+" + SetModeStr;
      }
      if (!UnsetModeStr.IsEmpty())
      {
        Result += "-" + UnsetModeStr;
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
    if ((RightUndef[static_cast<TRight>(rrUserRead + (Group * 3))] == rsYes) ||
        (RightUndef[static_cast<TRight>(rrUserWrite + (Group * 3))] == rsYes))
    {
      Right[static_cast<TRight>(rrUserExec + (Group * 3))] = true;
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
    FText = "";
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
  return Number;
}
//---------------------------------------------------------------------------
TRights::operator unsigned long() const
{
  return Number;
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
  Rights.AllowUndef = false;
  Rights.Number = 0;
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
  for (int Index = 0; Index < FileList->Count; Index++)
  {
    TRemoteFile * File = (TRemoteFile *)(FileList->Objects[Index]);
    assert(File);
    if (!Index)
    {
      CommonProperties.Rights = *(File->Rights);
      // previously we allowed undef implicitly for directories,
      // now we do it explicitly in properties dialog and only in combination
      // with "recursive" option
      CommonProperties.Rights.AllowUndef = File->Rights->IsUndef;
      CommonProperties.Valid << vpRights;
      if (File->Owner.IsSet)
      {
        CommonProperties.Owner = File->Owner;
        CommonProperties.Valid << vpOwner;
      }
      if (File->Group.IsSet)
      {
        CommonProperties.Group = File->Group;
        CommonProperties.Valid << vpGroup;
      }
    }
    else
    {
      CommonProperties.Rights.AllowUndef = true;
      CommonProperties.Rights &= *File->Rights;
      if (CommonProperties.Owner != File->Owner)
      {
        CommonProperties.Owner.Clear();
        CommonProperties.Valid >> vpOwner;
      };
      if (CommonProperties.Group != File->Group)
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
  if (Storage->ReadBinaryData("Valid", &Buf, sizeof(Buf)) == sizeof(Buf))
  {
    memcpy(&Valid, Buf, sizeof(Valid));
  }

  if (Valid.Contains(vpRights))
  {
    Rights.Text = Storage->ReadString("Rights", Rights.Text);
  }

  // TODO
}
//---------------------------------------------------------------------------
void TRemoteProperties::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteBinaryData(wstring("Valid"),
    static_cast<const void *>(&Valid), sizeof(Valid));

  if (Valid.Contains(vpRights))
  {
    Storage->WriteString("Rights", Rights.Text);
  }

  // TODO
}
