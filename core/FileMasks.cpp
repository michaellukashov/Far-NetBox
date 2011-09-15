//---------------------------------------------------------------------------
#include "stdafx.h"
#include "FileMasks.h"

#include "Common.h"
#include "TextsCore.h"
#include "RemoteFiles.h"
#include "PuttyTools.h"
#include "Terminal.h"
//---------------------------------------------------------------------------
EFileMasksException::EFileMasksException(
    std::wstring Message, int AErrorStart, int AErrorLen) :
  std::exception(::W2MB(Message.c_str()).c_str())
{
  ErrorStart = AErrorStart;
  ErrorLen = AErrorLen;
}
//---------------------------------------------------------------------------
std::wstring MaskFilePart(const std::wstring Part, const std::wstring Mask, bool& Masked)
{
  std::wstring Result;
  int RestStart = 1;
  bool Delim = false;
  for (int Index = 1; Index <= Mask.size(); Index++)
  {
    switch (Mask[Index])
    {
      case '\\':
        if (!Delim)
        {
          Delim = true;
          Masked = true;
          break;
        }

      case '*':
        if (!Delim)
        {
          Result += Part.substr(RestStart, Part.size() - RestStart + 1);
          RestStart = Part.size() + 1;
          Masked = true;
          break;
        }

      case '?':
        if (!Delim)
        {
          if (RestStart <= Part.size())
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
std::wstring MaskFileName(std::wstring FileName, const std::wstring Mask)
{
  if (!Mask.empty() && (Mask != L"*") && (Mask != L"*.*"))
  {
    bool Masked;
    int P = ::LastDelimiter(Mask, L".");
    if (P > 0)
    {
      int P2 = ::LastDelimiter(FileName, L".");
      // only dot at beginning of file name is not considered as
      // name/ext separator
      std::wstring FileExt = P2 > 1 ?
        FileName.substr(P2 + 1, FileName.size() - P2) : std::wstring();
      FileExt = MaskFilePart(FileExt, Mask.substr(P + 1, Mask.size() - P), Masked);
      if (P2 > 1)
      {
        FileName.resize(P2 - 1);
      }
      FileName = MaskFilePart(FileName, Mask.substr(1, P - 1), Masked);
      if (!FileExt.empty())
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
//---------------------------------------------------------------------------
bool IsFileNameMask(const std::wstring Mask)
{
  bool Masked = false;
  MaskFilePart(L"", Mask, Masked);
  return Masked;
}
//---------------------------------------------------------------------------
std::wstring DelimitFileNameMask(std::wstring Mask)
{
  for (int i = 1; i <= Mask.size(); i++)
  {
    if (wcschr(L"\\*?", Mask[i]) != NULL)
    {
      Mask.insert(i, L"\\");
      i++;
    }
  }
  return Mask;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileMasks::TParams::TParams() :
  Size(0)
{
}
//---------------------------------------------------------------------------
std::wstring TFileMasks::TParams::ToString() const
{
  return std::wstring(L"[") + IntToStr(Size) + L"]";
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TFileMasks::IsMask(const std::wstring Mask)
{
  return (::LastDelimiter(Mask, L"?*[") > 0);
}
//---------------------------------------------------------------------------
bool TFileMasks::IsAnyMask(const std::wstring & Mask)
{
  return Mask.empty() || (Mask == L"*.*") || (Mask == L"*");
}
//---------------------------------------------------------------------------
std::wstring TFileMasks::NormalizeMask(const std::wstring & Mask, const std::wstring & AnyMask)
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
TFileMasks::TFileMasks()
{
}
//---------------------------------------------------------------------------
TFileMasks::TFileMasks(const TFileMasks & Source)
{
  SetStr(Source.GetMasks(), false);
}
//---------------------------------------------------------------------------
TFileMasks::TFileMasks(const std::wstring & AMasks)
{
  SetStr(AMasks, false);
}
//---------------------------------------------------------------------------
TFileMasks::~TFileMasks()
{
  Clear();
}
//---------------------------------------------------------------------------
void TFileMasks::Clear()
{
  Clear(FIncludeMasks);
  Clear(FExcludeMasks);
}
//---------------------------------------------------------------------------
void TFileMasks::Clear(TMasks & Masks)
{
  TMasks::iterator I = Masks.begin();
  while (I != Masks.end())
  {
    ReleaseMaskMask((*I).FileNameMask);
    ReleaseMaskMask((*I).DirectoryMask);
    I++;
  }
  Masks.clear();
}
//---------------------------------------------------------------------------
void TFileMasks::Negate()
{
  FStr = L"";
  FIncludeMasks.swap(FExcludeMasks);
}
//---------------------------------------------------------------------------
bool TFileMasks::MatchesMasks(const std::wstring FileName, bool Directory,
  const std::wstring Path, const TParams * Params, const TMasks & Masks)
{
  bool Result = false;

  TMasks::const_iterator I = Masks.begin();
  while (!Result && (I != Masks.end()))
  {
    const TMask & Mask = *I;
    Result =
      (!Mask.DirectoryOnly || Directory) &&
      MatchesMaskMask(Mask.DirectoryMask, Path) &&
      MatchesMaskMask(Mask.FileNameMask, FileName);

    if (Result)
    {
      bool HasSize = !Directory && (Params != NULL);

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
    }

    I++;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TFileMasks::Matches(const std::wstring FileName, bool Directory,
  const std::wstring Path, const TParams * Params) const
{
  bool Result =
    (FIncludeMasks.empty() || MatchesMasks(FileName, Directory, Path, Params, FIncludeMasks)) &&
    !MatchesMasks(FileName, Directory, Path, Params, FExcludeMasks);
  return Result;
}
//---------------------------------------------------------------------------
bool TFileMasks::Matches(const std::wstring FileName, bool Local,
  bool Directory, const TParams * Params) const
{
  bool Result;
  if (Local)
  {
    std::wstring Path = ExtractFilePath(FileName);
    if (!Path.empty())
    {
      Path = ToUnixPath(ExcludeTrailingBackslash(Path));
    }
    Result = Matches(ExtractFileName(FileName, true), Directory, Path, Params);
  }
  else
  {
    Result = Matches(UnixExtractFileName(FileName), Directory,
      UnixExcludeTrailingBackslash(UnixExtractFilePath(FileName)), Params);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TFileMasks::operator ==(const TFileMasks & rhm) const
{
  return (GetMasks() == rhm.GetMasks());
}
//---------------------------------------------------------------------------
TFileMasks & TFileMasks::operator =(const std::wstring & rhs)
{
  SetMasks(rhs);
  return *this;
}
//---------------------------------------------------------------------------
TFileMasks & TFileMasks::operator =(const TFileMasks & rhm)
{
  SetMasks(rhm.GetMasks());
  return *this;
}
//---------------------------------------------------------------------------
bool TFileMasks::operator ==(const std::wstring & rhs) const
{
  return (GetMasks() == rhs);
}
//---------------------------------------------------------------------------
void TFileMasks::ThrowError(int Start, int End)
{
  throw EFileMasksException(
    FMTLOAD(MASK_ERROR, GetMasks().substr(Start, End - Start + 1).c_str()),
    Start, End - Start + 1);
}
//---------------------------------------------------------------------------
void TFileMasks::CreateMaskMask(const std::wstring & Mask, int Start, int End,
  bool Ex, TMaskMask & MaskMask)
{
  try
  {
    if (Ex && IsAnyMask(Mask))
    {
      MaskMask.Kind = TMaskMask::Any;
      MaskMask.Mask = NULL;
      assert(MaskMask.Mask == NULL);
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
void TFileMasks::ReleaseMaskMask(TMaskMask & MaskMask)
{
  delete MaskMask.Mask;
}
//---------------------------------------------------------------------------
void TFileMasks::TrimEx(std::wstring & Str, int & Start, int & End)
{
  std::wstring Buf = TrimLeft(Str);
  Start += Str.size() - Buf.size();
  Str = ::TrimRight(Buf);
  End -= Buf.size() - Str.size();
}
//---------------------------------------------------------------------------
bool TFileMasks::MatchesMaskMask(const TMaskMask & MaskMask, const std::wstring & Str)
{
  bool Result;
  if (MaskMask.Kind == TMaskMask::Any)
  {
    Result = true;
  }
  else if ((MaskMask.Kind == TMaskMask::NoExt) && (Str.find_first_of(L".") == 0))
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
void TFileMasks::SetMasks(const std::wstring value)
{
  if (FStr != value)
  {
    SetStr(value, false);
  }
}
//---------------------------------------------------------------------------
void TFileMasks::SetMask(const std::wstring & Mask)
{
  SetStr(Mask, true);
}
//---------------------------------------------------------------------------
void TFileMasks::SetStr(const std::wstring Str, bool SingleMask)
{
  std::wstring Backup = FStr;
  try
  {
    FStr = Str;
    Clear();

    int NextMaskFrom = 1;
    bool Include = true;
    while (NextMaskFrom <= Str.size())
    {
      int MaskStart = NextMaskFrom;
      char NextMaskDelimiter;
      std::wstring MaskStr;
      if (SingleMask)
      {
        MaskStr = Str;
        NextMaskFrom = Str.size() + 1;
        NextMaskDelimiter = '\0';
      }
      else
      {
        MaskStr = CopyToChars(Str, NextMaskFrom, L";,|", false, &NextMaskDelimiter);
      }
      int MaskEnd = NextMaskFrom - 1;

      TrimEx(MaskStr, MaskStart, MaskEnd);

      if (!MaskStr.empty())
      {
        TMask Mask;
        Mask.Str = MaskStr;
        Mask.DirectoryOnly = false;
        Mask.FileNameMask.Kind = TMaskMask::Any;
        Mask.FileNameMask.Mask = NULL;
        Mask.DirectoryMask.Kind = TMaskMask::Any;
        Mask.DirectoryMask.Mask = NULL;
        Mask.HighSizeMask = TMask::None;
        Mask.LowSizeMask = TMask::None;

        char NextPartDelimiter = '\0';
        int NextPartFrom = 1;
        while (NextPartFrom <= MaskStr.size())
        {
          char PartDelimiter = NextPartDelimiter;
          int PartFrom = NextPartFrom;
          std::wstring PartStr = CopyToChars(MaskStr, NextPartFrom, L"<>", false, &NextPartDelimiter);

          int PartStart = MaskStart + PartFrom - 1;
          int PartEnd = MaskStart + NextPartFrom - 2;

          TrimEx(PartStr, PartStart, PartEnd);

          if (PartDelimiter != '\0')
          {
            bool Low = (PartDelimiter == '>');
            TMask::TSizeMask & SizeMask = (Low ? Mask.LowSizeMask : Mask.HighSizeMask);
            __int64 & Size = (Low ? Mask.LowSize : Mask.HighSize);

            bool Result = (SizeMask == TMask::None);

            if (!Result)
            {
              // include delimiter into size part
              ThrowError(PartStart - 1, PartEnd);
            }
            else
            {
              if ((PartStr.size() >= 1) && (PartStr[1] == '='))
              {
                SizeMask = TMask::Close;
                PartStr.erase(1, 1);
              }
              else
              {
                SizeMask = TMask::Open;
              }

              Size = ParseSize(PartStr);
            }
          }
          else if (!PartStr.empty())
          {
            int D = ::LastDelimiter(PartStr, L"\\/");

            Mask.DirectoryOnly = (D > 0) && (D == PartStr.size());

            if (Mask.DirectoryOnly)
            {
              PartStr.resize(PartStr.size() - 1);
              D = ::LastDelimiter(PartStr, L"\\/");
            }

            if (D > 0)
            {
              // make sure sole "/" (root dir) is preserved as is
              CreateMaskMask(
                UnixExcludeTrailingBackslash(ToUnixPath(PartStr.substr(1, D))),
                PartStart, PartStart + D - 1, false,
                Mask.DirectoryMask);
              CreateMaskMask(
                PartStr.substr(D + 1, PartStr.size() - D),
                PartStart + D, PartEnd, true,
                Mask.FileNameMask);
            }
            else
            {
              CreateMaskMask(PartStr, PartStart, PartEnd, true, Mask.FileNameMask);
            }
          }
        }

        (Include ? FIncludeMasks : FExcludeMasks).push_back(Mask);
      }

      if (NextMaskDelimiter == '|')
      {
        if (Include)
        {
          Include = false;
        }
        else
        {
          ThrowError(NextMaskFrom - 1, Str.size());
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
#define TEXT_TOKEN '\255'
//---------------------------------------------------------------------------
const char TCustomCommand::NoQuote = '\0';
const std::wstring TCustomCommand::Quotes = L"\"'";
//---------------------------------------------------------------------------
TCustomCommand::TCustomCommand()
{
}
//---------------------------------------------------------------------------
void TCustomCommand::GetToken(
  const std::wstring & Command, int Index, int & Len, char & PatternCmd)
{
  assert(Index <= Command.size());
  const wchar_t * Ptr = Command.c_str() + Index - 1;

  if (Ptr[0] == '!')
  {
    PatternCmd = Ptr[1];
    if (PatternCmd == '!')
    {
      Len = 2;
    }
    else
    {
      Len = PatternLen(Index, PatternCmd);
    }

    if (Len < 0)
    {
      throw ExtException(FMTLOAD(CUSTOM_COMMAND_UNKNOWN, PatternCmd, Index));
    }
    else if (Len > 0)
    {
      if ((Command.size() - Index + 1) < Len)
      {
        throw ExtException(FMTLOAD(CUSTOM_COMMAND_UNTERMINATED, PatternCmd, Index));
      }
    }
    else if (Len == 0)
    {
      const wchar_t * PatternEnd = wcschr(Ptr + 1, L'!');
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
    const wchar_t * NextPattern = wcschr(Ptr, L'!');
    if (NextPattern == NULL)
    {
      Len = Command.size() - Index + 1;
    }
    else
    {
      Len = NextPattern - Ptr;
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TCustomCommand::Complete(const std::wstring & Command,
  bool LastPass)
{
  std::wstring Result;
  int Index = 1;

  while (Index <= Command.size())
  {
    int Len;
    char PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);

    if (PatternCmd == TEXT_TOKEN)
    {
      Result += Command.substr(Index, Len);
    }
    else if (PatternCmd == '!')
    {
      if (LastPass)
      {
        Result += '!';
      }
      else
      {
        Result += Command.substr(Index, Len);
      }
    }
    else
    {
      char Quote = NoQuote;
      if ((Index > 1) && (Index + Len - 1 < Command.size()) &&
          ::IsDelimiter(Command, Quotes, Index - 1) &&
          ::IsDelimiter(Command, Quotes, Index + Len) &&
          (Command[Index - 1] == Command[Index + Len]))
      {
        Quote = Command[Index - 1];
      }
      std::wstring Pattern = Command.substr(Index, Len);
      std::wstring Replacement;
      bool Delimit = true;
      if (PatternReplacement(Index, Pattern, Replacement, Delimit))
      {
        if (!LastPass)
        {
          Replacement = ::StringReplace(Replacement, L"!", L"!!"); // TReplaceFlags() << rfReplaceAll);
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
void TCustomCommand::DelimitReplacement(std::wstring & Replacement, char Quote)
{
  Replacement = ShellDelimitStr(Replacement, Quote);
}
//---------------------------------------------------------------------------
void TCustomCommand::Validate(const std::wstring & Command)
{
  CustomValidate(Command, NULL);
}
//---------------------------------------------------------------------------
void TCustomCommand::CustomValidate(const std::wstring & Command,
  void * Arg)
{
  int Index = 1;

  while (Index <= Command.size())
  {
    int Len;
    char PatternCmd;
    GetToken(Command, Index, Len, PatternCmd);
    ValidatePattern(Command, Index, Len, PatternCmd, Arg);

    Index += Len;
  }
}
//---------------------------------------------------------------------------
bool TCustomCommand::FindPattern(const std::wstring & Command,
  char PatternCmd)
{
  bool Result = false;
  int Index = 1;

  while (!Result && (Index <= Command.size()))
  {
    int Len;
    char APatternCmd;
    GetToken(Command, Index, Len, APatternCmd);
    if (((PatternCmd != '!') && (PatternCmd == APatternCmd)) ||
        ((PatternCmd == '!') && (Len == 1) && (APatternCmd != TEXT_TOKEN)))
    {
      Result = true;
    }

    Index += Len;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TCustomCommand::ValidatePattern(const std::wstring & /*Command*/,
  int /*Index*/, int /*Len*/, char /*PatternCmd*/, void * /*Arg*/)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TInteractiveCustomCommand::TInteractiveCustomCommand(
  TCustomCommand * ChildCustomCommand)
{
  FChildCustomCommand = ChildCustomCommand;
}
//---------------------------------------------------------------------------
void TInteractiveCustomCommand::Prompt(int /*Index*/,
  const std::wstring & /*Prompt*/, std::wstring & Value)
{
  Value = L"";
}
//---------------------------------------------------------------------------
int TInteractiveCustomCommand::PatternLen(int Index, char PatternCmd)
{
  int Len;
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
bool TInteractiveCustomCommand::PatternReplacement(int Index, const std::wstring & Pattern,
  std::wstring & Replacement, bool & Delimit)
{
  bool Result;
  if ((Pattern.size() >= 3) && (Pattern[2] == '?'))
  {
    std::wstring PromptStr;
    int Pos = Pattern.substr(3, Pattern.size() - 2).find_first_of(L"?");
    if (Pos > 0)
    {
      Replacement = Pattern.substr(3 + Pos, Pattern.size() - 3 - Pos);
      if ((Pos > 1) && (Pattern[3 + Pos - 2] == '\\'))
      {
        Delimit = false;
        Pos--;
      }
      PromptStr = Pattern.substr(3, Pos - 1);
    }
    else
    {
      PromptStr = Pattern.substr(3, Pattern.size() - 3);
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
TCustomCommandData::TCustomCommandData(TTerminal * Terminal)
{
  HostName = Terminal->GetSessionData()->GetHostName();
  UserName = Terminal->GetSessionData()->GetUserName();
  Password = Terminal->GetPassword();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileCustomCommand::TFileCustomCommand()
{
}
//---------------------------------------------------------------------------
TFileCustomCommand::TFileCustomCommand(const TCustomCommandData & Data,
  const std::wstring & Path)
{
  FData = Data;
  FPath = Path;
}
//---------------------------------------------------------------------------
TFileCustomCommand::TFileCustomCommand(const TCustomCommandData & Data,
    const std::wstring & Path, const std::wstring & FileName,
    const std::wstring & FileList) :
  TCustomCommand()
{
  FData = Data;
  FPath = Path;
  FFileName = FileName;
  FFileList = FileList;
}
//---------------------------------------------------------------------------
int TFileCustomCommand::PatternLen(int /*Index*/, char PatternCmd)
{
  int Len;
  switch (toupper(PatternCmd))
  {
    case '@':
    case 'U':
    case 'P':
    case '/':
    case '&':
      Len = 2;
      break;

    default:
      Len = 1;
      break;
  }
  return Len;
}
//---------------------------------------------------------------------------
bool TFileCustomCommand::PatternReplacement(int /*Index*/,
  const std::wstring & Pattern, std::wstring & Replacement, bool & Delimit)
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
    assert(Pattern.size() == 1);
    Replacement = FFileName;
  }

  return true;
}
//---------------------------------------------------------------------------
void TFileCustomCommand::Validate(const std::wstring & Command)
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
void TFileCustomCommand::ValidatePattern(const std::wstring & /*Command*/,
  int Index, int /*Len*/, char PatternCmd, void * Arg)
{
  int * Found = static_cast<int *>(Arg);

  assert(Index > 0);

  if (PatternCmd == '&')
  {
    Found[0] = Index;
  }
  else if ((PatternCmd != TEXT_TOKEN) && (PatternLen(Index, PatternCmd) == 1))
  {
    Found[1] = Index;
  }
}
//---------------------------------------------------------------------------
bool TFileCustomCommand::IsFileListCommand(const std::wstring & Command)
{
  return FindPattern(Command, '&');
}
//---------------------------------------------------------------------------
bool TFileCustomCommand::IsFileCommand(const std::wstring & Command)
{
  return FindPattern(Command, '!') || FindPattern(Command, '&');
}
//---------------------------------------------------------------------------
