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
  FAsciiFileMask.SetMasks(L"*.*html; *.htm; *.txt; *.php; *.php3; *.cgi; *.c; *.cpp; *.h; *.pas; "
    L"*.bas; *.tex; *.pl; *.js; .htaccess; *.xtml; *.css; *.cfg; *.ini; *.sh; *.xml");
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
}
//---------------------------------------------------------------------------
UnicodeString TCopyParamType::GetInfoStr(const UnicodeString & Separator, int Options) const
{
  TCopyParamType Defaults;
  UnicodeString Result;

  bool SomeAttrExcluded = false;
  #define ADD(STR, EXCEPT) \
    if (FLAGCLEAR(Options, EXCEPT)) \
    { \
      Result += (Result.IsEmpty() ? UnicodeString() : Separator) + (STR); \
    } \
    else \
    { \
      SomeAttrExcluded = true; \
    }

  if ((GetTransferMode() != Defaults.GetTransferMode()) ||
      ((GetTransferMode() == tmAutomatic) && !(GetAsciiFileMask() == Defaults.GetAsciiFileMask())))
  {
    UnicodeString S = FORMAT(LoadStrPart(COPY_INFO_TRANSFER_TYPE, 1).c_str(),
      LoadStrPart(COPY_INFO_TRANSFER_TYPE, GetTransferMode() + 2).c_str());
    if (GetTransferMode() == tmAutomatic)
    {
      S = FORMAT(S.c_str(), GetAsciiFileMask().GetMasks().c_str());
    }
    ADD(S, cpaIncludeMaskOnly | cpaNoTransferMode);
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

  if (SomeAttrExcluded)
  {
    Result += (Result.IsEmpty() ? UnicodeString() : Separator) +
      FORMAT(LoadStrPart(COPY_INFO_NOT_USABLE, 1).c_str(),
        (LoadStrPart(COPY_INFO_NOT_USABLE, (Result.IsEmpty() ? 3 : 2)).c_str()));
  }
  else if (Result.IsEmpty())
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
  UnicodeString FileName2 = FileName; 
  if (GetInvalidCharsReplacement() == TokenReplacement)
  {
    wchar_t * InvalidChar = const_cast<wchar_t *>(FileName2.c_str());
    while ((InvalidChar = wcschr(InvalidChar, TokenPrefix)) != NULL)
    {
      intptr_t Index = InvalidChar - FileName2.c_str() + 1;
      if (FileName2.Length() >= Index + 2)
      {
        UnicodeString Hex = FileName2.SubString(Index + 1, 2);
        wchar_t Char = static_cast<wchar_t>(HexToByte(Hex));
        if ((Char != L'\0') &&
            ((FTokenizibleChars.Pos(Char) > 0) ||
             (((Char == L' ') || (Char == L'.')) && (Index == FileName2.Length() - 2))))
        {
          FileName2[Index] = Char;
          FileName2.Delete(Index + 1, 2);
          InvalidChar = const_cast<wchar_t *>(FileName2.c_str() + Index);
        }
        else if ((Hex == L"00") &&
                 ((Index == FileName2.Length() - 2) || (FileName2[Index + 3] == L'.')) &&
                 IsReservedName(FileName2.SubString(1, Index - 1) + FileName2.SubString(Index + 3, FileName2.Length() - Index - 3 + 1)))
        {
          FileName2.Delete(Index, 3);
          InvalidChar = const_cast<wchar_t *>(FileName2.c_str() + Index - 1);
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
  return FileName2;
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
  CALLSTACK;
  TRACEFMT("1 [%s] [%d] [%d]", FileName.c_str(), int(Side), int(FirstLevel));
  UnicodeString FileName2 = FileName;
  if (FirstLevel)
  {
    FileName2 = MaskFileName(FileName2, GetFileMask());
  }
  switch (GetFileNameCase()) {
    case ncUpperCase: FileName2 = FileName2.UpperCase(); break;
    case ncLowerCase: FileName2 = FileName2.LowerCase(); break;
    case ncFirstUpperCase: FileName2 = FileName2.SubString(1, 1).UpperCase() +
      FileName2.SubString(2, FileName2.Length()-1).LowerCase(); break;
    case ncLowerCaseShort:
      if ((FileName2.Length() <= 12) && (FileName2.Pos(L".") <= 9) &&
          (FileName2 == FileName2.UpperCase()))
      {
        FileName2 = FileName2.LowerCase();
      }
      break;
    case ncNoChange:
    default:
      /*nothing*/
      break;
  }
  if (Side == osRemote)
  {
    FileName2 = ValidLocalFileName(FileName2);
  }
  else
  {
    FileName2 = RestoreChars(FileName2);
  }
  TRACEFMT("2 [%s]", FileName2.c_str());
  return FileName2;
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
TRights TCopyParamType::RemoteFileRights(intptr_t Attrs) const
{
  CALLSTACK;
  TRights R = GetRights();
  if ((Attrs & faDirectory) && GetAddXToDirectories())
    R.AddExecute();
  TRACEFMT("Rights [%x] [%x] [%x] [%d]", int(GetRights().GetNumberSet()), int(GetRights().GetNumberUnset()), int(R.GetNumberSet()), int(R.GetNumberUnset()));
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
    L"  TM: %c; ClAr: %s; CPS: %u; InclM: %s\n"
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
     GetIncludeFileMask().GetMasks().c_str(),
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
  CALLSTACK;
  bool Result = true;
  if (!GetIncludeFileMask().GetMasks().IsEmpty())
  {
    Result = GetIncludeFileMask().Matches(FileName, (Side == osLocal),
      Directory, &Params);
  }
  TRACEFMT("1 [%s] [%d] [%d] [=%d]", FileName.c_str(), int(Side), int(Directory), int(Result));
  return Result;
}
//---------------------------------------------------------------------------
void TCopyParamType::Load(THierarchicalStorage * Storage)
{
  CALLSTACK;
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
      bool NegativeExclude = Storage->ReadBool(L"NegativeExclude", GetNegativeExclude());
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
unsigned long GetSpeedLimit(const UnicodeString & Text)
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
      throw Exception(FMTLOAD(SPEED_INVALID, Text.c_str()));
    }
    Speed = SSpeed;
  }
  return Speed * 1024;
}
//---------------------------------------------------------------------------
UnicodeString SetSpeedLimit(unsigned long Limit)
{
  UnicodeString Text;
  if (Limit == 0)
  {
    Text = LoadStr(SPEED_UNLIMITED);
  }
  else
  {
    Text = IntToStr(int(Limit / 1024));
  }
  return Text;
}
