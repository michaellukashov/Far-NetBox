//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "CopyParam.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"
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
}
//---------------------------------------------------------------------------
void TCopyParamType::Default()
{
  // when changing defaults, make sure GetInfoStr() can handle it
  SetFileNameCase(ncNoChange);
  SetPreserveReadOnly(false);
  SetPreserveTime(true);
  FRights.SetNumber(TRights::rfDefault);
  SetPreserveRights(false); // Was True until #106
  SetIgnorePermErrors(false);
  FAsciiFileMask.SetMasks(UnicodeString(L"*.*html; *.htm; *.txt; *.php; *.php3; *.cgi; *.c; *.cpp; *.h; *.pas; "
    L"*.bas; *.tex; *.pl; *.js; .htaccess; *.xtml; *.css; *.cfg; *.ini; *.sh; *.xml"));
  SetTransferMode(tmBinary);
  SetAddXToDirectories(true);
  SetResumeSupport(rsSmart);
  SetResumeThreshold(100 * 1024); // (100 KiB)
  SetInvalidCharsReplacement(TokenReplacement);
  SetLocalInvalidChars(::LocalInvalidChars);
  SetCalculateSize(true);
  SetFileMask(L"*.*");
  FNegativeExclude = false;
  GetIncludeFileMask().SetMasks(L"");
  SetClearArchive(false);
  SetCPSLimit(0);
  SetNewerOnly(false);
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::GetInfoStr(
  const UnicodeString & Separator, intptr_t Options) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  DoGetInfoStr(Separator, Options, Result, SomeAttrIncluded);
  return Result;
}
//---------------------------------------------------------------------------
bool TCopyParamType::AnyUsableCopyParam(intptr_t Options) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  DoGetInfoStr(L";", Options, Result, SomeAttrIncluded);
  return SomeAttrIncluded;
}
//---------------------------------------------------------------------------
void TCopyParamType::DoGetInfoStr(
  const UnicodeString & Separator, intptr_t Options,
  UnicodeString & Result, bool & SomeAttrIncluded) const
{
  TCopyParamType Defaults;

  bool SomeAttrExcluded = false;
  SomeAttrIncluded = false;
  #define ADD(STR, EXCEPT) \
    if (FLAGCLEAR(Options, EXCEPT)) \
    { \
      AddToList(Result, (STR), Separator); \
      SomeAttrIncluded = true; \
    } \
    else \
    { \
      SomeAttrExcluded = true; \
    }

  bool TransferModeDiffers =
    ((GetTransferMode() != Defaults.GetTransferMode()) ||
     ((GetTransferMode() == tmAutomatic) && !(GetAsciiFileMask() == Defaults.GetAsciiFileMask())));

  if (FLAGCLEAR(Options, cpaIncludeMaskOnly | cpaNoTransferMode))
  {
    // Adding Transfer type unconditionally
    bool FormatMask;
    int Ident;
    switch (GetTransferMode())
    {
      case tmBinary:
        FormatMask = false;
        Ident = 2;
        break;
      case tmAscii:
        FormatMask = false;
        Ident = 3;
        break;
      case tmAutomatic:
      default:
        FormatMask = !(GetAsciiFileMask() == Defaults.GetAsciiFileMask());
        Ident = FormatMask ? 4 : 5;
        break;
    }
    UnicodeString S = FORMAT(LoadStrPart(COPY_INFO_TRANSFER_TYPE2, 1),
      (LoadStrPart(COPY_INFO_TRANSFER_TYPE2, Ident)));
    if (FormatMask)
    {
      S = FORMAT(S, (GetAsciiFileMask().GetMasks()));
    }
    AddToList(Result, S, Separator);

    if (TransferModeDiffers)
    {
      ADD("", cpaIncludeMaskOnly | cpaNoTransferMode);
    }
  }
  else
  {
    if (TransferModeDiffers)
    {
      SomeAttrExcluded = true;
    }
  }

  if (GetFileNameCase() != Defaults.GetFileNameCase())
  {
    ADD(FORMAT(LoadStrPart(COPY_INFO_FILENAME, 1).c_str(),
      LoadStrPart(COPY_INFO_FILENAME, GetFileNameCase() + 2).c_str()),
      cpaIncludeMaskOnly);
  }

  if ((GetInvalidCharsReplacement() == NoReplacement) !=
        (Defaults.GetInvalidCharsReplacement() == NoReplacement))
  {
    assert(GetInvalidCharsReplacement() == NoReplacement);
    if (GetInvalidCharsReplacement() == NoReplacement)
    {
      ADD(LoadStr(COPY_INFO_DONT_REPLACE_INV_CHARS).c_str(), cpaIncludeMaskOnly);
    }
  }

  if ((GetPreserveRights() != Defaults.GetPreserveRights()) ||
      (GetPreserveRights() &&
       ((GetRights() != Defaults.GetRights()) || (GetAddXToDirectories() != Defaults.GetAddXToDirectories()))))
  {
    assert(GetPreserveRights());

    if (GetPreserveRights())
    {
      UnicodeString RightsStr = GetRights().GetText();
      if (GetAddXToDirectories())
      {
        RightsStr += L", " + LoadStr(COPY_INFO_ADD_X_TO_DIRS);
      }
      ADD(FORMAT(LoadStr(COPY_INFO_PERMISSIONS).c_str(), RightsStr.c_str()),
        cpaIncludeMaskOnly | cpaNoRights);
    }
  }

  if (GetPreserveTime() != Defaults.GetPreserveTime())
  {
    ADD(LoadStr(GetPreserveTime() ? COPY_INFO_TIMESTAMP : COPY_INFO_DONT_PRESERVE_TIME).c_str(),
      cpaIncludeMaskOnly | cpaNoPreserveTime);
  }

  if ((GetPreserveRights() || GetPreserveTime()) &&
      (GetIgnorePermErrors() != Defaults.GetIgnorePermErrors()))
  {
    assert(GetIgnorePermErrors());

    if (GetIgnorePermErrors())
    {
      ADD(LoadStr(COPY_INFO_IGNORE_PERM_ERRORS).c_str(),
        cpaIncludeMaskOnly | cpaNoIgnorePermErrors);
    }
  }

  if (GetPreserveReadOnly() != Defaults.GetPreserveReadOnly())
  {
    assert(GetPreserveReadOnly());
    if (GetPreserveReadOnly())
    {
      ADD(LoadStr(COPY_INFO_PRESERVE_READONLY).c_str(),
        cpaIncludeMaskOnly | cpaNoPreserveReadOnly);
    }
  }

  if (GetCalculateSize() != Defaults.GetCalculateSize())
  {
    assert(!GetCalculateSize());
    if (!GetCalculateSize())
    {
      ADD(LoadStr(COPY_INFO_DONT_CALCULATE_SIZE).c_str(), cpaIncludeMaskOnly);
    }
  }

  if (GetClearArchive() != Defaults.GetClearArchive())
  {
    assert(GetClearArchive());
    if (GetClearArchive())
    {
      ADD(LoadStr(COPY_INFO_CLEAR_ARCHIVE),
        cpaIncludeMaskOnly | cpaNoClearArchive);
    }
  }

  if (GetIncludeFileMask() == Defaults.GetIncludeFileMask())
  {
    ADD(FORMAT(LoadStr(COPY_INFO_FILE_MASK), GetIncludeFileMask().GetMasks().c_str()),
      cpaNoIncludeMask);
  }

  if (GetCPSLimit() > 0)
  {
    ADD(FMTLOAD(COPY_INFO_CPS_LIMIT, static_cast<int>(GetCPSLimit() / 1024)).c_str(), cpaIncludeMaskOnly);
  }

  if (GetNewerOnly() != Defaults.GetNewerOnly())
  {
    if (ALWAYS_TRUE(GetNewerOnly()))
    {
      ADD(StripHotkey(LoadStr(COPY_PARAM_NEWER_ONLY)), cpaIncludeMaskOnly | cpaNoNewerOnly);
    }
  }

  if (SomeAttrExcluded)
  {
    Result += (Result.IsEmpty() ? UnicodeString() : Separator) +
      FORMAT(LoadStrPart(COPY_INFO_NOT_USABLE, 1).c_str(),
        LoadStrPart(COPY_INFO_NOT_USABLE, (SomeAttrIncluded ? 2 : 3)).c_str());
  }
  else if (Result.IsEmpty())
  {
    Result = LoadStr(COPY_INFO_DEFAULT);
  }
  #undef ADD
}
//---------------------------------------------------------------------------
void TCopyParamType::Assign(const TCopyParamType * Source)
{
  assert(Source != NULL);
  #define COPY(Prop) Set ## Prop(Source->Get ## Prop())
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
  COPY(IncludeFileMask);
  COPY(ClearArchive);
  COPY(CPSLimit);
  COPY(NewerOnly);
  #undef COPY
}
//---------------------------------------------------------------------------
TCopyParamType & TCopyParamType::operator =(const TCopyParamType & rhp)
{
  Assign(&rhp);
  return *this;
}
//---------------------------------------------------------------------------
void TCopyParamType::SetLocalInvalidChars(const UnicodeString & Value)
{
  if (Value != GetLocalInvalidChars())
  {
    FLocalInvalidChars = Value;
    FTokenizibleChars = FLocalInvalidChars + TokenPrefix;
  }
}
//---------------------------------------------------------------------------
bool TCopyParamType::GetReplaceInvalidChars() const
{
  return (GetInvalidCharsReplacement() != NoReplacement);
}
//---------------------------------------------------------------------------
void TCopyParamType::SetReplaceInvalidChars(bool Value)
{
  if (GetReplaceInvalidChars() != Value)
  {
    SetInvalidCharsReplacement((Value ? TokenReplacement : NoReplacement));
  }
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::ValidLocalFileName(const UnicodeString & FileName) const
{
  return ::ValidLocalFileName(FileName, GetInvalidCharsReplacement(), FTokenizibleChars, LocalInvalidChars);
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::RestoreChars(const UnicodeString & FileName) const
{
  UnicodeString Result = FileName; 
  if (GetInvalidCharsReplacement() == TokenReplacement)
  {
    wchar_t * InvalidChar = const_cast<wchar_t *>(Result.c_str());
    while ((InvalidChar = wcschr(InvalidChar, TokenPrefix)) != NULL)
    {
      intptr_t Index = InvalidChar - Result.c_str() + 1;
      if (Result.Length() >= Index + 2)
      {
        UnicodeString Hex = Result.SubString(Index + 1, 2);
        wchar_t Char = static_cast<wchar_t>(HexToByte(Hex));
        if ((Char != L'\0') &&
            ((FTokenizibleChars.Pos(Char) > 0) ||
             (((Char == L' ') || (Char == L'.')) && (Index == Result.Length() - 2))))
        {
          Result[Index] = Char;
          Result.Delete(Index + 1, 2);
          InvalidChar = const_cast<wchar_t *>(Result.c_str() + Index);
        }
        else if ((Hex == L"00") &&
                 ((Index == Result.Length() - 2) || (Result[Index + 3] == L'.')) &&
                 IsReservedName(Result.SubString(1, Index - 1) + Result.SubString(Index + 3, Result.Length() - Index - 3 + 1)))
        {
          Result.Delete(Index, 3);
          InvalidChar = const_cast<wchar_t *>(Result.c_str() + Index - 1);
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
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::ValidLocalPath(const UnicodeString & Path) const
{
  UnicodeString Result;
  UnicodeString Path2 = Path;
  while (!Path2.IsEmpty())
  {
    if (!Result.IsEmpty())
    {
      Result += L"\\";
    }
    Result += ValidLocalFileName(CutToChar(Path2, L'\\', false));
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::ChangeFileName(const UnicodeString & FileName,
  TOperationSide Side, bool FirstLevel) const
{
  UnicodeString Result = FileName;
  if (FirstLevel)
  {
    Result = MaskFileName(Result, GetFileMask());
  }
  switch (GetFileNameCase())
  {
    case ncUpperCase:
      Result = Result.UpperCase();
      break;
    case ncLowerCase:
      Result = Result.LowerCase();
      break;
    case ncFirstUpperCase:
      Result = Result.SubString(1, 1).UpperCase() +
        Result.SubString(2, Result.Length()-1).LowerCase();
      break;
    case ncLowerCaseShort:
      if ((Result.Length() <= 12) && (Result.Pos(L".") <= 9) &&
          (Result == Result.UpperCase()))
      {
        Result = Result.LowerCase();
      }
      break;
    case ncNoChange:
    default:
      /*nothing*/
      break;
  }
  if (Side == osRemote)
  {
    Result = ValidLocalFileName(Result);
  }
  else
  {
    Result = RestoreChars(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TCopyParamType::UseAsciiTransfer(const UnicodeString & FileName,
  TOperationSide Side, const TFileMasks::TParams & Params) const
{
  switch (GetTransferMode())
  {
    case tmBinary: return false;
    case tmAscii: return true;
    case tmAutomatic: return GetAsciiFileMask().Matches(FileName, (Side == osLocal),
      false, &Params);
    default: assert(false); return false;
  }
}
//---------------------------------------------------------------------------
TRights TCopyParamType::RemoteFileRights(uintptr_t Attrs) const
{
  TRights R = GetRights();
  if ((Attrs & faDirectory) && GetAddXToDirectories())
    R.AddExecute();
  return R;
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::GetLogStr() const
{
  wchar_t CaseC[] = L"NULFS";
  wchar_t ModeC[] = L"BAM";
  wchar_t ResumeC[] = L"YSN";
  return FORMAT(
    L"  PrTime: %s; PrRO: %s; Rght: %s; PrR: %s (%s); FnCs: %c; RIC: %s; "
       L"Resume: %c (%d); CalcS: %s; Mask: %s\n"
    L"  TM: %c; ClAr: %s; CPS: %u; NewerOnly: %s; InclM: %s\n"
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
    BooleanToEngStr(GetNewerOnly()).c_str(),
    GetIncludeFileMask().GetMasks().c_str(),
    GetAsciiFileMask().GetMasks().c_str());
}
//---------------------------------------------------------------------------
uintptr_t TCopyParamType::LocalFileAttrs(const TRights & Rights) const
{
  uintptr_t Result = 0;
  if (GetPreserveReadOnly() && !Rights.GetRight(TRights::rrUserWrite))
  {
    Result |= faReadOnly;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TCopyParamType::AllowResume(__int64 Size) const
{
  switch (GetResumeSupport())
  {
    case rsOn: return true;
    case rsOff: return false;
    case rsSmart: return (Size >= GetResumeThreshold());
    default: assert(false); return false;
  }
}
//---------------------------------------------------------------------------
bool TCopyParamType::AllowAnyTransfer() const
{
  return GetIncludeFileMask().GetMasks().IsEmpty();
}
//---------------------------------------------------------------------------
bool TCopyParamType::AllowTransfer(const UnicodeString & FileName,
  TOperationSide Side, bool Directory, const TFileMasks::TParams & Params) const
{
  bool Result = true;
  if (!GetIncludeFileMask().GetMasks().IsEmpty())
  {
    Result = GetIncludeFileMask().Matches(FileName, (Side == osLocal),
      Directory, &Params);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamType::Load(THierarchicalStorage * Storage)
{
  SetAddXToDirectories(Storage->ReadBool(L"AddXToDirectories", GetAddXToDirectories()));
  GetAsciiFileMask().SetMasks(Storage->ReadString(L"Masks", GetAsciiFileMask().GetMasks()));
  SetFileNameCase(static_cast<TFileNameCase>(Storage->ReadInteger(L"FileNameCase", GetFileNameCase())));
  SetPreserveReadOnly(Storage->ReadBool(L"PreserveReadOnly", GetPreserveReadOnly()));
  SetPreserveTime(Storage->ReadBool(L"PreserveTime", GetPreserveTime()));
  SetPreserveRights(Storage->ReadBool(L"PreserveRights", GetPreserveRights()));
  SetIgnorePermErrors(Storage->ReadBool(L"IgnorePermErrors", GetIgnorePermErrors()));
  FRights.SetText(Storage->ReadString(L"Text", GetRights().GetText()));
  SetTransferMode(static_cast<TTransferMode>(Storage->ReadInteger(L"TransferMode", GetTransferMode())));
  SetResumeSupport(static_cast<TResumeSupport>(Storage->ReadInteger(L"ResumeSupport", GetResumeSupport())));
  SetResumeThreshold(Storage->ReadInt64(L"ResumeThreshold", GetResumeThreshold()));
  SetInvalidCharsReplacement(static_cast<wchar_t>(Storage->ReadInteger(L"ReplaceInvalidChars",
    static_cast<int>(GetInvalidCharsReplacement()))));
  SetLocalInvalidChars(Storage->ReadString(L"LocalInvalidChars", GetLocalInvalidChars()));
  SetCalculateSize(Storage->ReadBool(L"CalculateSize", GetCalculateSize()));
  if (Storage->ValueExists(L"IncludeFileMask"))
  {
    GetIncludeFileMask().SetMasks(Storage->ReadString(L"IncludeFileMask", GetIncludeFileMask().GetMasks()));
  }
  else if (Storage->ValueExists(L"ExcludeFileMask"))
  {
    UnicodeString ExcludeFileMask = Storage->ReadString(L"ExcludeFileMask", UnicodeString());
    if (!ExcludeFileMask.IsEmpty())
    {
      bool NegativeExclude = Storage->ReadBool(L"NegativeExclude", false);
      if (NegativeExclude)
      {
        GetIncludeFileMask().SetMasks(ExcludeFileMask);
      }
      // convert at least simple cases to new format
      else if (ExcludeFileMask.Pos(IncludeExcludeFileMasksDelimiter) == 0)
      {
        GetIncludeFileMask().SetMasks(UnicodeString(IncludeExcludeFileMasksDelimiter) + ExcludeFileMask);
      }
    }
  }
  SetClearArchive(Storage->ReadBool(L"ClearArchive", GetClearArchive()));
  SetCPSLimit(Storage->ReadInteger(L"CPSLimit", GetCPSLimit()));
  SetNewerOnly(Storage->ReadBool(L"NewerOnly", GetNewerOnly()));
}
//---------------------------------------------------------------------------
void TCopyParamType::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteBool(L"AddXToDirectories", GetAddXToDirectories());
  Storage->WriteString(L"Masks", GetAsciiFileMask().GetMasks());
  Storage->WriteInteger(L"FileNameCase", GetFileNameCase());
  Storage->WriteBool(L"PreserveReadOnly", GetPreserveReadOnly());
  Storage->WriteBool(L"PreserveTime", GetPreserveTime());
  Storage->WriteBool(L"PreserveRights", GetPreserveRights());
  Storage->WriteBool(L"IgnorePermErrors", GetIgnorePermErrors());
  Storage->WriteString(L"Text", GetRights().GetText());
  Storage->WriteInteger(L"TransferMode", GetTransferMode());
  Storage->WriteInteger(L"ResumeSupport", GetResumeSupport());

  Storage->WriteInt64(L"ResumeThreshold", GetResumeThreshold());
  Storage->WriteInteger(L"ReplaceInvalidChars", static_cast<unsigned int>(GetInvalidCharsReplacement()));
  Storage->WriteString(L"LocalInvalidChars", GetLocalInvalidChars());
  Storage->WriteBool(L"CalculateSize", GetCalculateSize());
  Storage->WriteString(L"IncludeFileMask", GetIncludeFileMask().GetMasks());
  Storage->DeleteValue(L"ExcludeFileMask"); // obsolete
  Storage->DeleteValue(L"NegativeExclude"); // obsolete
  Storage->WriteBool(L"ClearArchive", GetClearArchive());
  Storage->WriteInteger(L"CPSLimit", GetCPSLimit());
  Storage->WriteBool(L"NewerOnly", GetNewerOnly());
}
//---------------------------------------------------------------------------
#define C(Property) (Get ## Property() == rhp.Get ## Property())
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
    C(IncludeFileMask) &&
    C(ClearArchive) &&
    C(CPSLimit) &&
    C(NewerOnly) &&
    true;
}
#undef C
//---------------------------------------------------------------------------
const TFileMasks & TCopyParamType::GetAsciiFileMask() const
{
  return FAsciiFileMask;
}
//---------------------------------------------------------------------------
TFileMasks & TCopyParamType::GetAsciiFileMask()
{
  return FAsciiFileMask;
}
//---------------------------------------------------------------------------
void TCopyParamType::SetAsciiFileMask(TFileMasks Value)
{
  FAsciiFileMask = Value;
}
//---------------------------------------------------------------------------
const TFileNameCase & TCopyParamType::GetFileNameCase() const
{
  return FFileNameCase;
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::GetLocalInvalidChars() const
{
  return FLocalInvalidChars;
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::GetFileMask() const
{
  return FFileMask;
}
//---------------------------------------------------------------------------
void TCopyParamType::SetFileMask(const UnicodeString & Value)
{
  FFileMask = Value;
}
//---------------------------------------------------------------------------
const TFileMasks & TCopyParamType::GetIncludeFileMask() const
{
  return FIncludeFileMask;
}
//---------------------------------------------------------------------------
TFileMasks & TCopyParamType::GetIncludeFileMask()
{
  return FIncludeFileMask;
}
//---------------------------------------------------------------------------
void TCopyParamType::SetIncludeFileMask(TFileMasks Value)
{
  FIncludeFileMask = Value;
}
//---------------------------------------------------------------------------
uintptr_t GetSpeedLimit(const UnicodeString & Text)
{
  uintptr_t Speed = 0;
  if (AnsiSameText(Text, LoadStr(SPEED_UNLIMITED)))
  {
    Speed = 0;
  }
  else
  {
    intptr_t SSpeed = 0;
    if (!TryStrToInt(Text, SSpeed) ||
        (SSpeed < 0))
    {
      throw Exception(FMTLOAD(SPEED_INVALID, Text.c_str()));
    }
    Speed = SSpeed;
  }
  return Speed * 1024;
}
//---------------------------------------------------------------------------
UnicodeString SetSpeedLimit(uintptr_t Limit)
{
  UnicodeString Text;
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
