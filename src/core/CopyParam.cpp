//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Common.h"
#include "CopyParam.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
TCopyParamType::TCopyParamType()
{
  Default();
}
//---------------------------------------------------------------------------
TCopyParamType::TCopyParamType(const TCopyParamType & Source)
{
  Assign(&Source);
}
//---------------------------------------------------------------------------
TCopyParamType::~TCopyParamType()
{
  // DEBUG_PRINTF(L"begin");
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TCopyParamType::Default()
{
  // when changing defaults, make sure GetInfoStr() can handle it
  SetFileNameCase(ncNoChange);
  SetPreserveReadOnly(false);
  SetPreserveTime(true);
  FRights.SetNumber(TRights::rfDefault);
  SetPreserveRights(false); // Was true until #106
  SetIgnorePermErrors(false);
  FAsciiFileMask.SetMasks(L"*.*html; *.htm; *.txt; *.php; *.php3; *.cgi; *.c; *.cpp; *.h; *.pas; "
    L"*.bas; *.tex; *.pl; *.js; .htaccess; *.xtml; *.css; *.cfg; *.ini; *.sh; *.xml");
  SetTransferMode(tmAutomatic);
  SetAddXToDirectories(true);
  SetResumeSupport(rsSmart);
  SetResumeThreshold(100 * 1024); // (100 KiB)
  SetInvalidCharsReplacement(TokenReplacement);
  SetLocalInvalidChars(L"/\\:*?\"<>|");
  SetCalculateSize(true);
  SetFileMask(L"*.*");
  GetExcludeFileMask().SetMasks(L"");
  SetNegativeExclude(false);
  SetClearArchive(false);
  SetCPSLimit(0);
}
//---------------------------------------------------------------------------
std::wstring TCopyParamType::GetInfoStr(const std::wstring &Separator, int Options) const
{
  TCopyParamType Defaults;
  std::wstring Result;

  bool SomeAttrExcluded = false;
  #define ADD(STR, EXCEPT) \
    if (FLAGCLEAR(Options, EXCEPT)) \
    { \
      Result += (Result.empty() ? std::wstring() : Separator) + (STR); \
    } \
    else \
    { \
      SomeAttrExcluded = true; \
    }

  if ((GetTransferMode() != Defaults.GetTransferMode()) ||
      ((GetTransferMode() == tmAutomatic) && !(GetAsciiFileMask() == Defaults.GetAsciiFileMask())))
  {
    std::wstring S = FORMAT(LoadStrPart(COPY_INFO_TRANSFER_TYPE, 1).c_str(),
      LoadStrPart(COPY_INFO_TRANSFER_TYPE, GetTransferMode() + 2).c_str());
    if (GetTransferMode() == tmAutomatic)
    {
      S = FORMAT(S.c_str(), GetAsciiFileMask().GetMasks().c_str());
    }
    ADD(S, cpaExcludeMaskOnly | cpaNoTransferMode);
  }

  if (GetFileNameCase() != Defaults.GetFileNameCase())
  {
    ADD(FORMAT(LoadStrPart(COPY_INFO_FILENAME, 1).c_str(),
      LoadStrPart(COPY_INFO_FILENAME, GetFileNameCase() + 2).c_str()),
      cpaExcludeMaskOnly);
  }

  if ((GetInvalidCharsReplacement() == NoReplacement) !=
        (Defaults.GetInvalidCharsReplacement() == NoReplacement))
  {
    assert(GetInvalidCharsReplacement() == NoReplacement);
    if (GetInvalidCharsReplacement() == NoReplacement)
    {
      ADD(LoadStr(COPY_INFO_DONT_REPLACE_INV_CHARS).c_str(), cpaExcludeMaskOnly);
    }
  }

  if ((GetPreserveRights() != Defaults.GetPreserveRights()) ||
      (GetPreserveRights() &&
       ((GetRights() != Defaults.GetRights()) || (GetAddXToDirectories() != Defaults.GetAddXToDirectories()))))
  {
    assert(GetPreserveRights());

    if (GetPreserveRights())
    {
      std::wstring RightsStr = GetRights().GetText();
      if (GetAddXToDirectories())
      {
        RightsStr += L", " + LoadStr(COPY_INFO_ADD_X_TO_DIRS);
      }
      ADD(FORMAT(LoadStr(COPY_INFO_PERMISSIONS).c_str(), RightsStr.c_str()),
        cpaExcludeMaskOnly | cpaNoRights);
    }
  }

  if (GetPreserveTime() != Defaults.GetPreserveTime())
  {
    ADD(LoadStr(GetPreserveTime() ? COPY_INFO_TIMESTAMP : COPY_INFO_DONT_PRESERVE_TIME).c_str(),
      cpaExcludeMaskOnly | cpaNoPreserveTime);
  }

  if ((GetPreserveRights() || GetPreserveTime()) &&
      (GetIgnorePermErrors() != Defaults.GetIgnorePermErrors()))
  {
    assert(GetIgnorePermErrors());

    if (GetIgnorePermErrors())
    {
      ADD(LoadStr(COPY_INFO_IGNORE_PERM_ERRORS).c_str(),
        cpaExcludeMaskOnly | cpaNoIgnorePermErrors);
    }
  }

  if (GetPreserveReadOnly() != Defaults.GetPreserveReadOnly())
  {
    assert(GetPreserveReadOnly());
    if (GetPreserveReadOnly())
    {
      ADD(LoadStr(COPY_INFO_PRESERVE_READONLY).c_str(),
        cpaExcludeMaskOnly | cpaNoPreserveReadOnly);
    }
  }

  if (GetCalculateSize() != Defaults.GetCalculateSize())
  {
    assert(!GetCalculateSize());
    if (!GetCalculateSize())
    {
      ADD(LoadStr(COPY_INFO_DONT_CALCULATE_SIZE).c_str(), cpaExcludeMaskOnly);
    }
  }

  if (GetClearArchive() != Defaults.GetClearArchive())
  {
    assert(GetClearArchive());
    if (GetClearArchive())
    {
      ADD(LoadStr(COPY_INFO_CLEAR_ARCHIVE),
        cpaExcludeMaskOnly | cpaNoClearArchive);
    }
  }

  if (((GetNegativeExclude() != Defaults.GetNegativeExclude()) && !(GetExcludeFileMask() == L"")) ||
      !(GetExcludeFileMask() == Defaults.GetExcludeFileMask()))
  {
    ADD(FORMAT(LoadStr(GetNegativeExclude() ? COPY_INFO_INCLUDE_MASK : COPY_INFO_EXCLUDE_MASK).c_str(),
      GetExcludeFileMask().GetMasks().c_str()),
      cpaNoExcludeMask);
  }

  if (GetCPSLimit() > 0)
  {
    ADD(FMTLOAD(COPY_INFO_CPS_LIMIT, static_cast<int>(GetCPSLimit() / 1024)).c_str(), cpaExcludeMaskOnly);
    ADD(L"", cpaExcludeMaskOnly);
  }

  if (SomeAttrExcluded)
  {
    Result += (Result.empty() ? std::wstring() : Separator) +
      FORMAT(LoadStrPart(COPY_INFO_NOT_USABLE, 1).c_str(),
        (LoadStrPart(COPY_INFO_NOT_USABLE, (Result.empty() ? 3 : 2)).c_str()));
  }
  else if (Result.empty())
  {
    Result = LoadStr(COPY_INFO_DEFAULT);
  }
  #undef ADD

  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamType::Assign(const TCopyParamType * Source)
{
  assert(Source != NULL);
  #define COPY(Prop) Set##Prop(Source->Get##Prop())
  COPY(FileNameCase);
  COPY(PreserveReadOnly);
  COPY(PreserveTime);
  COPY(Rights);
  COPY(AsciiFileMask);
  COPY(TransferMode);
  COPY(AddXToDirectories);
  COPY(PreserveRights);
  COPY(IgnorePermErrors);
  COPY(ResumeSupport);
  COPY(ResumeThreshold);
  COPY(InvalidCharsReplacement);
  COPY(LocalInvalidChars);
  COPY(CalculateSize);
  COPY(FileMask);
  COPY(ExcludeFileMask);
  COPY(NegativeExclude);
  COPY(ClearArchive);
  COPY(CPSLimit);
  #undef COPY
}
//---------------------------------------------------------------------------
TCopyParamType & TCopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}
//---------------------------------------------------------------------------
void TCopyParamType::SetLocalInvalidChars(const std::wstring &value)
{
  if (value != GetLocalInvalidChars())
  {
    FLocalInvalidChars = value;
    FTokenizibleChars = FLocalInvalidChars + TokenPrefix;
  }
}
//---------------------------------------------------------------------------
bool TCopyParamType::GetReplaceInvalidChars() const
{
  return (GetInvalidCharsReplacement() != NoReplacement);
}
//---------------------------------------------------------------------------
void TCopyParamType::SetReplaceInvalidChars(bool value)
{
  if (GetReplaceInvalidChars() != value)
  {
    SetInvalidCharsReplacement((value ? TokenReplacement : NoReplacement));
  }
}
//---------------------------------------------------------------------------
wchar_t *TCopyParamType::ReplaceChar(std::wstring &FileName, wchar_t *InvalidChar) const
{
    size_t Index = InvalidChar - FileName.c_str();
    DEBUG_PRINTF(L"FileName = %s, InvalidChar = %s", FileName.c_str(), InvalidChar);
    if (GetInvalidCharsReplacement() == TokenReplacement)
    {
      FileName.insert(Index + 1, CharToHex(FileName[Index]));
      FileName[Index] = TokenPrefix;
      InvalidChar = const_cast<wchar_t *>(FileName.c_str() + Index + 3);
    }
    else
    {
      FileName[Index] = GetInvalidCharsReplacement();
      InvalidChar++;
    }
    return InvalidChar;
}
//---------------------------------------------------------------------------
std::wstring TCopyParamType::ValidLocalFileName(const std::wstring &FileName) const
{
  std::wstring fileName = FileName;
  if (GetInvalidCharsReplacement() != NoReplacement)
  {
    bool ATokenReplacement = (GetInvalidCharsReplacement() == TokenReplacement);
    std::wstring chars = ATokenReplacement ? FTokenizibleChars : GetLocalInvalidChars();
    const wchar_t *Chars = chars.c_str();
    wchar_t * InvalidChar = const_cast<wchar_t *>(fileName.c_str());
    while ((InvalidChar = wcspbrk(InvalidChar, Chars)) != NULL)
    {
      size_t Pos = (InvalidChar - fileName.c_str());
      char Char;
      if ((GetInvalidCharsReplacement() == TokenReplacement) &&
          (*InvalidChar == TokenPrefix) &&
          (((fileName.size() - Pos) <= 1) ||
           (((Char = HexToChar(fileName.substr(Pos + 1, 2))) == '\0') ||
            (FTokenizibleChars.find_first_of(Char) == std::wstring::npos))))
      {
        InvalidChar++;
      }
      else
      {
        InvalidChar = ReplaceChar(fileName, InvalidChar);
      }
    }

    // Windows trim trailing space or dot, hence we must encode it to preserve it
    if (!fileName.empty() &&
        ((fileName[fileName.size() - 1] == ' ') ||
         (fileName[fileName.size() - 1] == '.')))
    {
      ReplaceChar(fileName, const_cast<wchar_t *>(fileName.c_str() + fileName.size() - 1));
    }

    if (IsReservedName(fileName))
    {
      size_t P = fileName.find_first_of(L".");
      if (P == std::wstring::npos)
      {
        P = fileName.size();
      }
      fileName.insert(P, L"%00");
    }
  }
  return fileName;
}
//---------------------------------------------------------------------------
std::wstring TCopyParamType::RestoreChars(const std::wstring &FileName) const
{
  std::wstring fileName = FileName;
  if (GetInvalidCharsReplacement() == TokenReplacement)
  {
    wchar_t * InvalidChar = const_cast<wchar_t *>(fileName.c_str());
    while ((InvalidChar = wcschr(InvalidChar, TokenPrefix)) != NULL)
    {
      size_t Index = InvalidChar - fileName.c_str() + 1;
      ::Error(SNotImplemented, 206); 
      if ((fileName.size() >= Index + 2) &&
          false // FIXME (fileName.ByteType(Index) == mbSingleByte) &&
          // (fileName.ByteType(Index + 1) == mbSingleByte) &&
          // (fileName.ByteType(Index + 2) == mbSingleByte)
          )
      {
        std::wstring Hex = fileName.substr(Index + 1, 2);
        char Char = HexToChar(Hex);
        if ((Char != '\0') &&
            ((FTokenizibleChars.find(Char) != std::wstring::npos) ||
             (((Char == ' ') || (Char == '.')) && (Index == fileName.size() - 2))))
        {
          fileName[Index] = Char;
          fileName.erase(Index + 1, 2);
          InvalidChar = const_cast<wchar_t *>(fileName.c_str() + Index);
        }
        else if ((Hex == L"00") &&
                 ((Index == fileName.size() - 2) || (fileName[Index + 3] == '.')) &&
                 IsReservedName(fileName.substr(0, Index - 1) + fileName.substr(Index + 3, fileName.size() - Index - 3 + 1)))
        {
          fileName.erase(Index, 3);
          InvalidChar = const_cast<wchar_t *>(fileName.c_str() + Index - 1);
        }
        else
        {
          InvalidChar++;
        }
      }
      else
      {
        InvalidChar++;
      }
    }
  }
  return fileName;
}
//---------------------------------------------------------------------------
std::wstring TCopyParamType::ValidLocalPath(const std::wstring &Path) const
{
  std::wstring Result;
  std::wstring path = Path;
  while (!path.empty())
  {
    if (!Result.empty())
    {
      Result += L"\\";
    }
    Result += ValidLocalFileName(CutToChar(path, '\\', false));
  }
  return Result;
}
//---------------------------------------------------------------------------
// not used yet
std::wstring TCopyParamType::Untokenize(const std::wstring &FileName)
{
  wchar_t *Token;
  std::wstring Result = FileName;
  while ((Token = AnsiStrScan(Result.c_str(), TokenPrefix)) != NULL)
  {
    size_t Index = Token - Result.c_str() + 1;
    if (Index > Result.size() - 2)
    {
      Result = FileName;
      break;
    }
    else
    {
      // wchar_t Ch = static_cast<wchar_t>(HexToInt(Result.substr(Index + 1, 2), -1));
      wchar_t Ch = static_cast<wchar_t>(HexToInt(Result.substr(Index + 1, 2), -1));
      if (Ch == '\0')
      {
        Result = FileName;
        break;
      }
      else
      {
        Result[Index] = Ch;
        Result.erase(Index + 1, 2);
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TCopyParamType::ChangeFileName(const std::wstring &FileName,
  TOperationSide Side, bool FirstLevel) const
{
  // DEBUG_PRINTF(L"FirstLevel = %d, Side = %d, FileName = %s", FirstLevel, Side, FileName.c_str());
  std::wstring fileName = FileName;
  if (FirstLevel)
  {
    fileName = ::MaskFileName(fileName, GetFileMask());
  }
  // DEBUG_PRINTF(L"fileName = %s", fileName.c_str());
  switch (GetFileNameCase())
  {
    case ncUpperCase: fileName = ::UpperCase(fileName); break;
    case ncLowerCase: fileName = ::LowerCase(fileName); break;
    case ncFirstUpperCase: fileName = ::UpperCase(fileName.substr(0, 1)) +
      ::LowerCase(fileName.substr(1, fileName.size() - 1)); break;
    case ncLowerCaseShort:
      if ((fileName.size() <= 12) && (fileName.find_first_of(L".") <= 9) &&
          (fileName == ::UpperCase(fileName)))
      {
        fileName = ::LowerCase(fileName);
      }
      break;
    case ncNoChange:
    default:
      /*nothing*/
      break;
  }
  if (Side == osRemote)
  {
    fileName = ValidLocalFileName(fileName);
  }
  else
  {
    fileName = RestoreChars(fileName);
  }
  // DEBUG_PRINTF(L"fileName = %s", fileName.c_str());
  return fileName;
}
//---------------------------------------------------------------------------
bool TCopyParamType::UseAsciiTransfer(const std::wstring &FileName,
  TOperationSide Side, const TFileMasks::TParams & Params) const
{
  switch (GetTransferMode()) {
    case tmBinary: return false;
    case tmAscii: return true;
    case tmAutomatic: return GetAsciiFileMask().Matches(FileName, (Side == osLocal),
      false, &Params);
    default: assert(false); return false;
  }
}
//---------------------------------------------------------------------------
TRights TCopyParamType::RemoteFileRights(int Attrs) const
{
  TRights R = GetRights();
  if ((Attrs & faDirectory) && GetAddXToDirectories())
    R.AddExecute();
  return R;
}
//---------------------------------------------------------------------------
std::wstring TCopyParamType::GetLogStr() const
{
  static wchar_t CaseC[] = L"NULFS";
  static wchar_t ModeC[] = L"BAM";
  static wchar_t ResumeC[] = L"YSN";
  return FORMAT(
    L"  PrTime: %s; PrRO: %s; Rght: %s; PrR: %s (%s); FnCs: %c; RIC: %s; "
    L"Resume: %c (%d); CalcS: %s; Mask: %s\n"
    L"  TM: %c; ClAr: %s; CPS: %u; ExclM(%s): %s\n"
    L"  AscM: %s\n",
    BooleanToEngStr(GetPreserveTime()).c_str(),
     BooleanToEngStr(GetPreserveReadOnly()).c_str(),
     GetRights().GetText().c_str(),
     BooleanToEngStr(GetPreserveRights()).c_str(),
     BooleanToEngStr(GetIgnorePermErrors()).c_str(),
     CaseC[GetFileNameCase()],
     CharToHex(GetInvalidCharsReplacement()).c_str(),
     ResumeC[GetResumeSupport()],
     (int)GetResumeThreshold(),
     BooleanToEngStr(GetCalculateSize()).c_str(),
     GetFileMask().c_str(),
     ModeC[GetTransferMode()],
     BooleanToEngStr(GetClearArchive()).c_str(),
     int(GetCPSLimit()),
     BooleanToEngStr(GetNegativeExclude()).c_str(),
     GetExcludeFileMask().GetMasks().c_str(),
     GetAsciiFileMask().GetMasks().c_str());
}
//---------------------------------------------------------------------------
int TCopyParamType::LocalFileAttrs(const TRights & Rights) const
{
  int Result = 0;
  if (GetPreserveReadOnly() && !Rights.GetRight(TRights::rrUserWrite))
  {
    Result |= faReadOnly;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TCopyParamType::AllowResume(__int64 Size) const
{
  switch (GetResumeSupport()) {
    case rsOn: return true;
    case rsOff: return false;
    case rsSmart: return (Size >= GetResumeThreshold());
    default: assert(false); return false;
  }
}
//---------------------------------------------------------------------------
bool TCopyParamType::AllowAnyTransfer() const
{
  return GetExcludeFileMask().GetMasks().empty();
}
//---------------------------------------------------------------------------
bool TCopyParamType::AllowTransfer(const std::wstring &FileName,
  TOperationSide Side, bool Directory, const TFileMasks::TParams & Params) const
{
  bool Result = true;
  if (!GetExcludeFileMask().GetMasks().empty())
  {
    Result = (GetExcludeFileMask().Matches(FileName, (Side == osLocal),
      Directory, &Params) == GetNegativeExclude());
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamType::Load(THierarchicalStorage * Storage)
{
  SetAddXToDirectories(Storage->Readbool(L"AddXToDirectories", GetAddXToDirectories()));
  GetAsciiFileMask().SetMasks(Storage->ReadString(L"Masks", GetAsciiFileMask().GetMasks()));
  SetFileNameCase(static_cast<TFileNameCase>(Storage->Readint(L"FileNameCase", GetFileNameCase())));
  SetPreserveReadOnly(Storage->Readbool(L"PreserveReadOnly", GetPreserveReadOnly()));
  SetPreserveTime(Storage->Readbool(L"PreserveTime", GetPreserveTime()));
  SetPreserveRights(Storage->Readbool(L"PreserveRights", GetPreserveRights()));
  SetIgnorePermErrors(Storage->Readbool(L"IgnorePermErrors", GetIgnorePermErrors()));
  FRights.SetText(Storage->ReadString(L"Text", GetRights().GetText()));
  SetTransferMode(static_cast<TTransferMode>(Storage->Readint(L"TransferMode", GetTransferMode())));
  SetResumeSupport(static_cast<TResumeSupport>(Storage->Readint(L"ResumeSupport", GetResumeSupport())));
  SetResumeThreshold(Storage->ReadInt64(L"ResumeThreshold", GetResumeThreshold()));
  SetInvalidCharsReplacement(static_cast<char>(Storage->Readint(L"ReplaceInvalidChars",
    static_cast<unsigned char>(GetInvalidCharsReplacement()))));
  SetLocalInvalidChars(Storage->ReadString(L"LocalInvalidChars", GetLocalInvalidChars()));
  SetCalculateSize(Storage->Readbool(L"CalculateSize", GetCalculateSize()));
  GetExcludeFileMask().SetMasks(Storage->ReadString(L"ExcludeFileMask", GetExcludeFileMask().GetMasks()));
  SetNegativeExclude(Storage->Readbool(L"NegativeExclude", GetNegativeExclude()));
  SetClearArchive(Storage->Readbool(L"ClearArchive", GetClearArchive()));
  SetCPSLimit(Storage->Readint(L"CPSLimit", GetCPSLimit()));
}
//---------------------------------------------------------------------------
void TCopyParamType::Save(THierarchicalStorage * Storage) const
{
  Storage->Writebool(L"AddXToDirectories", GetAddXToDirectories());
  Storage->WriteString(L"Masks", GetAsciiFileMask().GetMasks());
  Storage->Writeint(L"FileNameCase", GetFileNameCase());
  Storage->Writebool(L"PreserveReadOnly", GetPreserveReadOnly());
  Storage->Writebool(L"PreserveTime", GetPreserveTime());
  Storage->Writebool(L"PreserveRights", GetPreserveRights());
  Storage->Writebool(L"IgnorePermErrors", GetIgnorePermErrors());
  // DEBUG_PRINTF(L"GetRights().GetText = %s", GetRights().GetText().c_str());
  Storage->WriteString(L"Text", GetRights().GetText());
  Storage->Writeint(L"TransferMode", GetTransferMode());
  Storage->Writeint(L"ResumeSupport", GetResumeSupport());
  Storage->WriteInt64(L"ResumeThreshold", GetResumeThreshold());
  Storage->Writeint(L"ReplaceInvalidChars", static_cast<unsigned char>(GetInvalidCharsReplacement()));
  Storage->WriteString(L"LocalInvalidChars", GetLocalInvalidChars());
  Storage->Writebool(L"CalculateSize", GetCalculateSize());
  Storage->WriteString(L"ExcludeFileMask", GetExcludeFileMask().GetMasks());
  Storage->Writebool(L"NegativeExclude", GetNegativeExclude());
  Storage->Writebool(L"ClearArchive", GetClearArchive());
  Storage->Writeint(L"CPSLimit", GetCPSLimit());
}
//---------------------------------------------------------------------------
#define C(Property) (Get##Property() == rhp.Get##Property())
bool TCopyParamType::operator==(const TCopyParamType & rhp) const
{
  return
    C(AddXToDirectories) &&
    C(AsciiFileMask) &&
    C(FileNameCase) &&
    C(PreserveReadOnly) &&
    C(PreserveTime) &&
    C(PreserveRights) &&
    C(IgnorePermErrors) &&
    C(Rights) &&
    C(TransferMode) &&
    C(ResumeSupport) &&
    C(ResumeThreshold) &&
    C(InvalidCharsReplacement) &&
    C(LocalInvalidChars) &&
    C(CalculateSize) &&
    C(ExcludeFileMask) &&
    C(NegativeExclude) &&
    C(ClearArchive) &&
    C(CPSLimit) &&
    true;
}
#undef C
//---------------------------------------------------------------------------
unsigned long GetSpeedLimit(const std::wstring & Text)
{
  unsigned long Speed = 0;
  if (AnsiSameText(Text, LoadStr(SPEED_UNLIMITED)))
  {
    Speed = 0;
  }
  else
  {
    int SSpeed = 0;
    if (!TryStrToInt(Text, SSpeed) ||
        (SSpeed < 0))
    {
      throw ExtException(FMTLOAD(SPEED_INVALID, Text.c_str()));
    }
    Speed = SSpeed;
  }
  return Speed * 1024;
}
//---------------------------------------------------------------------------
std::wstring SetSpeedLimit(unsigned long Limit)
{
  std::wstring Text;
  if (Limit == 0)
  {
    Text = LoadStr(SPEED_UNLIMITED);
  }
  else
  {
    Text = IntToStr(Limit / 1024);
  }
  return Text;
}
