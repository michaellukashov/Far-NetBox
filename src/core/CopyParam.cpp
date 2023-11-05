#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include "CopyParam.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"
#include "Interface.h"

const wchar_t *TransferModeNames[] = { L"binary", L"ascii", L"automatic" };
const int TransferModeNamesCount = _countof(TransferModeNames);

TCopyParamType::TCopyParamType(TObjectClassId Kind) noexcept :
  TObject(Kind)
{
  TCopyParamType::Default();
}

TCopyParamType::TCopyParamType(const TCopyParamType &Source) noexcept :
  TObject(OBJECT_CLASS_TCopyParamType)
{
  TCopyParamType::Assign(&Source);
}

TCopyParamType::~TCopyParamType() noexcept
{
}

void TCopyParamType::Default()
{
  // when changing defaults, make sure GetInfoStr() can handle it
  SetFileNameCase(ncNoChange);
  SetPreserveReadOnly(false);
  SetPreserveTime(true);
  SetPreserveTimeDirs(false);
  FRights.SetNumber(TRights::rfDefault);
  SetPreserveRights(false); // Was True until #106
  SetIgnorePermErrors(false);
  FAsciiFileMask.Masks(UnicodeString("*.*html; *.htm; *.txt; *.php; *.php3; *.cgi; *.c; *.cpp; *.h; *.pas; "
    "*.bas; *.tex; *.pl; *.js; .htaccess; *.xtml; *.css; *.cfg; *.ini; *.sh; *.xml"));
  SetTransferMode(tmBinary);
  SetAddXToDirectories(true);
  SetResumeSupport(rsSmart);
  SetResumeThreshold(100 * 1024); // (100 KB)
  SetInvalidCharsReplacement(TokenReplacement);
  SetLocalInvalidChars(::LocalInvalidChars);
  SetCalculateSize(true);
  SetFileMask(AnyMask);
  GetIncludeFileMask().Masks("");
  SetTransferSkipList(nullptr);
  SetTransferResumeFile("");
  SetClearArchive(false);
  SetRemoveCtrlZ(false);
  SetRemoveBOM(false);
  SetCPSLimit(0);
  SetNewerOnly(false);
  EncryptNewFiles = true;
  ExcludeHiddenFiles = false;
  ExcludeEmptyDirectories = false;
  Size = -1;
  PartOffset = -1;
  PartSize = -1;
  OnceDoneOperation = odoIdle;
  FOnTransferOut = nullptr;
  FOnTransferIn = nullptr;
}

UnicodeString TCopyParamType::GetInfoStr(
  const UnicodeString & Separator, int32_t Attrs) const
{
  UnicodeString Result;
  bool SomeAttrIncluded{false};
  UnicodeString ScriptArgs;
//  UnicodeString AssemblyCode;
  DoGetInfoStr(
    Separator, Attrs, Result, SomeAttrIncluded,
    UnicodeString(), ScriptArgs); //TAssemblyLanguage(0), AssemblyCode);
  return Result;
}

bool TCopyParamType::AnyUsableCopyParam(int32_t Attrs) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  UnicodeString ScriptArgs;
//  UnicodeString AssemblyCode;
  DoGetInfoStr(
    ";", Attrs, Result, SomeAttrIncluded,
    UnicodeString(), ScriptArgs/*TAssemblyLanguage(0), AssemblyCode*/);
  return SomeAttrIncluded;
}

UnicodeString TCopyParamType::GenerateTransferCommandArgs(int Attrs, const UnicodeString & Link) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  UnicodeString ScriptArgs;
  UnicodeString AssemblyCode;
  DoGetInfoStr(
    ";", Attrs, Result, SomeAttrIncluded,
    Link, ScriptArgs); // , TAssemblyLanguage(0), AssemblyCode);
  return ScriptArgs;
}

#if 0

UnicodeString TCopyParamType::GenerateAssemblyCode(TAssemblyLanguage Language, int Attrs) const
{
  UnicodeString Result;
  bool SomeAttrIncluded;
  UnicodeString ScriptArgs;
  UnicodeString AssemblyCode;
  DoGetInfoStr(";", Attrs, Result, SomeAttrIncluded, UnicodeString(), ScriptArgs, Language, AssemblyCode);
  return Result;
}

#endif // #if 0

void TCopyParamType::DoGetInfoStr(
  UnicodeString Separator, int32_t Options,
  UnicodeString &Result, bool &SomeAttrIncluded,
  const UnicodeString /*ALink*/, UnicodeString &/*ScriptArgs*/) const //*TAssemblyLanguage Language, UnicodeString & AssemblyCode) const
{
  TCopyParamType Defaults;
  TCopyParamType ScriptNonDefaults;
  TCopyParamType CodeNonDefaults;

  bool SomeAttrExcluded = false;
  SomeAttrIncluded = false;
#define ADD(STR, EXCEPT) \
    FLAGCLEAR(Options, EXCEPT) ? (AddToList(Result, (STR), Separator), SomeAttrIncluded = true, true) : (SomeAttrExcluded = true, false)

  bool AsciiFileMaskDiffers = (GetTransferMode() == tmAutomatic) && !(GetAsciiFileMask() == Defaults.GetAsciiFileMask());
  bool TransferModeDiffers = ((GetTransferMode() != Defaults.GetTransferMode()) || AsciiFileMaskDiffers);

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
        LoadStrPart(COPY_INFO_TRANSFER_TYPE2, Ident));
    if (FormatMask)
    {
      S = FORMAT(S, GetAsciiFileMask().Masks());
    }
    AddToList(Result, S, Separator);

    if (TransferModeDiffers)
    {
      ADD("", cpaIncludeMaskOnly | cpaNoTransferMode);

#if 0
      ScriptArgs += RtfSwitchValue(TRANSFER_SWITCH, Link, TransferModeNames[TransferMode]);
      const wchar_t *TransferModeMembers[] = { L"Binary", L"Ascii", L"Automatic" };
      AssemblyCode += AssemblyProperty(
          Language, TransferOptionsClassName, L"TransferMode", L"TransferMode", TransferModeMembers[TransferMode], false);
      if (AsciiFileMaskDiffers)
      {
        ScriptNonDefaults.AsciiFileMask = AsciiFileMask;
        CodeNonDefaults.AsciiFileMask = AsciiFileMask;
      }
#endif // #if 0
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
    if (ADD(FORMAT(LoadStrPart(COPY_INFO_FILENAME, 1),
         LoadStrPart(COPY_INFO_FILENAME, FileNameCase + 2)),
         cpaIncludeMaskOnly))
    {
      ScriptNonDefaults.FileNameCase = FileNameCase;
      CodeNonDefaults.FileNameCase = FileNameCase;
    }
  }

  if (InvalidCharsReplacement != Defaults.InvalidCharsReplacement)
  {
    int Except = cpaIncludeMaskOnly;
    if (GetInvalidCharsReplacement() == NoReplacement)
    {
      ADD(LoadStr(COPY_INFO_DONT_REPLACE_INV_CHARS), Except);
    }

    if (FLAGCLEAR(Options, Except))
    {
      ScriptNonDefaults.InvalidCharsReplacement = InvalidCharsReplacement;
      CodeNonDefaults.InvalidCharsReplacement = InvalidCharsReplacement;
    }
  }

  if ((GetPreserveRights() != Defaults.GetPreserveRights()) ||
    (GetPreserveRights() &&
      ((GetRights() != Defaults.GetRights()) || (GetAddXToDirectories() != Defaults.GetAddXToDirectories()))))
  {
    const int Except = cpaIncludeMaskOnly | cpaNoRights;
    if (DebugAlwaysTrue(GetPreserveRights()))
    {
      UnicodeString RightsStr = GetRights().GetText();
      if (GetAddXToDirectories())
      {
        RightsStr += L", " + LoadStr(COPY_INFO_ADD_X_TO_DIRS);
      }
      ADD(FORMAT(LoadStr(COPY_INFO_PERMISSIONS), RightsStr),
        Except);

      if (FLAGCLEAR(Options, Except))
      {
#if 0
        ScriptArgs += RtfSwitchValue(PERMISSIONS_SWITCH, Link, Rights.Octal);

        const UnicodeString FilePermissionsClassName = L"FilePermissions";
        const bool Inline = true;
        UnicodeString FilePermissions =
          AssemblyNewClassInstanceStart(Language, FilePermissionsClassName, Inline) +
          AssemblyProperty(Language, FilePermissionsClassName, L"Octal", Rights.Octal, Inline) +
          AssemblyNewClassInstanceEnd(Language, Inline);

        AssemblyCode += AssemblyPropertyRaw(Language, TransferOptionsClassName, L"FilePermissions", FilePermissions, false);
#endif // #if 0
      }
    }

    if ((GetAddXToDirectories() != Defaults.GetAddXToDirectories()) && FLAGCLEAR(Options, Except))
    {
      ScriptNonDefaults.AddXToDirectories = AddXToDirectories;
      CodeNonDefaults.AddXToDirectories = AddXToDirectories;
    }
  }

  bool APreserveTimeDirs = GetPreserveTime() && GetPreserveTimeDirs();
  if ((GetPreserveTime() != Defaults.GetPreserveTime()) || (APreserveTimeDirs != Defaults.GetPreserveTimeDirs()))
  {
    bool AddPreserveTime = false;
    UnicodeString Str = LoadStr(GetPreserveTime() ? COPY_INFO_TIMESTAMP : COPY_INFO_DONT_PRESERVE_TIME);

    const int ExceptDirs = cpaNoPreserveTimeDirs;
    if (APreserveTimeDirs != Defaults.GetPreserveTimeDirs())
    {
      if (DebugAlwaysTrue(GetPreserveTimeDirs()))
      {
        if (FLAGCLEAR(Options, ExceptDirs))
        {
          Str = FMTLOAD(COPY_INFO_PRESERVE_TIME_DIRS, Str);
          AddPreserveTime = true;
        }
      }
      ADD("", ExceptDirs);
    }

    const int Except = cpaIncludeMaskOnly | cpaNoPreserveTime;
    if (GetPreserveTime() != Defaults.GetPreserveTime())
    {
      if (FLAGCLEAR(Options, Except))
      {
        AddPreserveTime = true;
      }
      ADD("", Except);
    }

    if (AddPreserveTime)
    {
      AddToList(Result, Str, Separator);
    }

    if (FLAGCLEAR(Options, Except))
    {
      if (GetPreserveTime())
      {
        if (GetPreserveTimeDirs() && FLAGCLEAR(Options, ExceptDirs))
        {
          //ScriptArgs += RtfSwitchValue(PRESERVETIME_SWITCH, Link, PRESERVETIMEDIRS_SWITCH_VALUE);
          CodeNonDefaults.PreserveTimeDirs = PreserveTimeDirs;
        }
        else
        {
          //ScriptArgs += RtfSwitch(PRESERVETIME_SWITCH, Link);
        }
      }
      else
      {
#if 0
        ScriptArgs += RtfSwitch(NOPRESERVETIME_SWITCH, Link);
        AssemblyCode += AssemblyProperty(Language, TransferOptionsClassName, L"PreserveTimestamp", false, false);
#endif // #if 0
      }
    }
  }

  if ((GetPreserveRights() || GetPreserveTime()) &&
      (GetIgnorePermErrors() != Defaults.GetIgnorePermErrors()))
  {
    if (DebugAlwaysTrue(GetIgnorePermErrors()))
    {
      if (ADD(LoadStr(COPY_INFO_IGNORE_PERM_ERRORS), cpaIncludeMaskOnly | cpaNoIgnorePermErrors))
      {
        ScriptNonDefaults.IgnorePermErrors = IgnorePermErrors;
        CodeNonDefaults.IgnorePermErrors = IgnorePermErrors;
      }
    }
  }

  if (GetPreserveReadOnly() != Defaults.GetPreserveReadOnly())
  {
    if (DebugAlwaysTrue(GetPreserveReadOnly()))
    {
      if (ADD(LoadStr(COPY_INFO_PRESERVE_READONLY), cpaIncludeMaskOnly | cpaNoPreserveReadOnly))
      {
        ScriptNonDefaults.PreserveReadOnly = PreserveReadOnly;
        CodeNonDefaults.PreserveReadOnly = PreserveReadOnly;
      }
    }
  }

  if (GetCalculateSize() != Defaults.GetCalculateSize())
  {
    if (DebugAlwaysTrue(!GetCalculateSize()))
    {
      ADD(LoadStr(COPY_INFO_DONT_CALCULATE_SIZE), cpaIncludeMaskOnly | cpaNoCalculateSize);
      // Always false in scripting, in assembly controlled by use of FileTransferProgress
    }
  }

  if (GetClearArchive() != Defaults.GetClearArchive())
  {
    if (DebugAlwaysTrue(GetClearArchive()))
    {
      if (ADD(LoadStr(COPY_INFO_CLEAR_ARCHIVE), cpaIncludeMaskOnly | cpaNoClearArchive))
      {
        ScriptNonDefaults.ClearArchive = ClearArchive;
        CodeNonDefaults.ClearArchive = ClearArchive;
      }
    }
  }

  if ((GetTransferMode() == tmAscii) || (GetTransferMode() == tmAutomatic))
  {
    if (GetRemoveBOM() != Defaults.GetRemoveBOM())
    {
      if (DebugAlwaysTrue(GetRemoveBOM()))
      {
        if (ADD(LoadStr(COPY_INFO_REMOVE_BOM), cpaIncludeMaskOnly | cpaNoRemoveBOM | cpaNoTransferMode))
        {
          ScriptNonDefaults.RemoveBOM = RemoveBOM;
          CodeNonDefaults.RemoveBOM = RemoveBOM;
        }
      }
    }

    if (GetRemoveCtrlZ() != Defaults.GetRemoveCtrlZ())
    {
      if (DebugAlwaysTrue(GetRemoveCtrlZ()))
      {
        if (ADD(LoadStr(COPY_INFO_REMOVE_CTRLZ), cpaIncludeMaskOnly | cpaNoRemoveCtrlZ | cpaNoTransferMode))
        {
          ScriptNonDefaults.RemoveCtrlZ = RemoveCtrlZ;
          CodeNonDefaults.RemoveCtrlZ = RemoveCtrlZ;
        }
      }
    }
  }

  if (!(GetIncludeFileMask() == Defaults.GetIncludeFileMask()))
  {
    if (ADD(FORMAT(LoadStr(COPY_INFO_FILE_MASK), IncludeFileMask.Masks), cpaNoIncludeMask))
    {
#if 0
      ScriptArgs += RtfSwitch(FILEMASK_SWITCH, Link, IncludeFileMask.Masks);
      AssemblyCode += AssemblyProperty(Language, TransferOptionsClassName, L"FileMask", IncludeFileMask.Masks, false);
#endif // #if 0
    }
  }

  DebugAssert(FTransferSkipList.get() == nullptr);
  DebugAssert(FTransferResumeFile.IsEmpty());

  if (GetCPSLimit() > 0)
  {
    int32_t LimitKB = intptr_t(GetCPSLimit() / 1024);
    if (ADD(FMTLOAD(COPY_INFO_CPS_LIMIT2, LimitKB), cpaIncludeMaskOnly))
    {
#if 0
      ScriptArgs += RtfSwitch(SPEED_SWITCH, Link, LimitKB);
      AssemblyCode += AssemblyProperty(Language, TransferOptionsClassName, L"SpeedLimit", LimitKB, false);
#endif // #if 0
    }
  }

  if (GetNewerOnly() != Defaults.GetNewerOnly())
  {
    if (DebugAlwaysTrue(GetNewerOnly()))
    {
      if (ADD(StripHotkey(LoadStr(COPY_PARAM_NEWER_ONLY)), cpaIncludeMaskOnly | cpaNoNewerOnly))
      {
//        ScriptArgs += RtfSwitch(NEWERONLY_SWICH, Link);
        CodeNonDefaults.NewerOnly = NewerOnly;
      }
    }
  }

  if (EncryptNewFiles != Defaults.EncryptNewFiles)
  {
    if (!DebugAlwaysFalse(EncryptNewFiles))
    {
      if (ADD(StripHotkey(LoadStr(COPY_INFO_DONT_ENCRYPT_NEW_FILES)), cpaIncludeMaskOnly | cpaNoEncryptNewFiles))
      {
        ScriptNonDefaults.EncryptNewFiles = EncryptNewFiles;
        CodeNonDefaults.EncryptNewFiles = EncryptNewFiles;
      }
    }
  }

  if (ExcludeHiddenFiles != Defaults.ExcludeHiddenFiles)
  {
    if (DebugAlwaysTrue(ExcludeHiddenFiles))
    {
      if (ADD(StripHotkey(LoadStr(COPY_INFO_EXCLUDE_HIDDEN_FILES)), cpaNoIncludeMask))
      {
        ScriptNonDefaults.ExcludeHiddenFiles = ExcludeHiddenFiles;
        CodeNonDefaults.ExcludeHiddenFiles = ExcludeHiddenFiles;
      }
    }
  }

  if (ExcludeEmptyDirectories != Defaults.ExcludeEmptyDirectories)
  {
    if (DebugAlwaysTrue(ExcludeEmptyDirectories))
    {
      if (ADD(StripHotkey(LoadStr(COPY_INFO_EXCLUDE_EMPTY_DIRS)), 0))
      {
        ScriptNonDefaults.ExcludeEmptyDirectories = ExcludeEmptyDirectories;
        CodeNonDefaults.ExcludeEmptyDirectories = ExcludeEmptyDirectories;
      }
    }
  }

  bool ResumeThresholdDiffers = ((GetResumeSupport() == rsSmart) && (GetResumeThreshold() != Defaults.GetResumeThreshold()));
  if (((GetResumeSupport() != Defaults.GetResumeSupport()) || ResumeThresholdDiffers) &&
    (GetTransferMode() != tmAscii) && FLAGCLEAR(Options, cpaNoResumeSupport))
  {
    UnicodeString Value;
    UnicodeString CodeState;
    int32_t ResumeThresholdKB = nb::ToIntPtr(GetResumeThreshold() / 1024);
    switch (GetResumeSupport())
    {
    case rsOff:
      Value = ToggleNames[ToggleOff];
      CodeState = "Off";
      break;

    case rsOn:
      Value = ToggleNames[ToggleOn];
      CodeState = "On";
      break;

    case rsSmart:
      Value = IntToStr(ResumeThresholdKB);
      break;
    }
//    ScriptArgs += RtfSwitchValue(RESUMESUPPORT_SWITCH, Link, Value);

#if 0
    const UnicodeString ResumeSupportClassName = "TransferResumeSupport";
    const bool Inline = true;
    UnicodeString ResumeSupportCode =
      AssemblyNewClassInstanceStart(Language, ResumeSupportClassName, Inline);
#endif // #if 0
    if (GetResumeSupport() == rsSmart)
    {
//      ResumeSupportCode += AssemblyProperty(Language, ResumeSupportClassName, "Threshold", ResumeThresholdKB, Inline);
    }
    else
    {
//      ResumeSupportCode += AssemblyProperty(Language, ResumeSupportClassName, "State", "TransferResumeSupportState", CodeState, Inline);
    }
#if 0
    ResumeSupportCode += AssemblyNewClassInstanceEnd(Language, Inline);

    AssemblyCode += AssemblyPropertyRaw(Language, TransferOptionsClassName, "ResumeSupport", ResumeSupportCode, false);
#endif // #if 0
  }

#if 0
  std::unique_ptr<TStringList> RawOptions;
  __removed std::unique_ptr<TOptionsStorage> OptionsStorage;

  RawOptions = std::make_unique<TStringList>();
  __removed OptionsStorage = std::make_unique<TOptionsStorage>(RawOptions.get(), true);
  __removed ScriptNonDefaults.Save(OptionsStorage.get(), &Defaults);

  if (RawOptions->Count > 0)
  {
    ScriptArgs +=
      RtfSwitch(RAWTRANSFERSETTINGS_SWITCH, ALink) +
      FORMAT("[%d]", RawOptions->Count) +
      StringsToParams(RawOptions.get());
  }

  RawOptions.reset(new TStringList());
  OptionsStorage.reset(new TOptionsStorage(RawOptions.get(), true));
  CodeNonDefaults.Save(OptionsStorage.get(), &Defaults);

  if (!AssemblyCode.IsEmpty())
  {
    AssemblyCode =
      AssemblyNewClassInstanceStart(Language, TransferOptionsClassName, false) +
      AssemblyCode +
      AssemblyNewClassInstanceEnd(Language, false);
  }

  if (RawOptions->Count > 0)
  {
    if (AssemblyCode.IsEmpty())
    {
      AssemblyCode = AssemblyVariableDeclaration(Language) + AssemblyNewClassInstance(Language, TransferOptionsClassName, false);
      if (Language == alCSharp)
      {
        AssemblyCode += RtfText(L"()");
      }
      AssemblyCode += AssemblyStatementSeparator(Language) + RtfPara;
    }
    else
    {
      AssemblyCode += RtfPara;
    }

    AssemblyCode +=
      AssemblyAddRawSettings(Language, RawOptions.get(), TransferOptionsClassName, L"AddRawSettings");
  }

  if (SomeAttrExcluded)
  {
    Result += (Result.IsEmpty() ? UnicodeString() : Separator) +
      FORMAT(LoadStrPart(COPY_INFO_NOT_USABLE, 1),
        LoadStrPart(COPY_INFO_NOT_USABLE, (SomeAttrIncluded ? 2 : 3)));
  }
  else if (Result.IsEmpty())
  {
    Result = LoadStr(COPY_INFO_DEFAULT);
  }
#endif // if 0
#undef ADD
}

void TCopyParamType::Assign(const TCopyParamType *Source)
{
  DebugAssert(Source != nullptr);
  if (!Source)
    return;
  #undef COPY
  #undef COPY2
  #define COPY(Prop) Set ## Prop(Source->Get ## Prop())
  #define COPY2(Prop) F##Prop = Source->F##Prop
  COPY(FileNameCase);
  COPY(PreserveReadOnly);
  COPY(PreserveTime);
  COPY(PreserveTimeDirs);
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
  COPY(TransferSkipList);
  COPY(TransferResumeFile);
  COPY(ClearArchive);
  COPY(RemoveCtrlZ);
  COPY(RemoveBOM);
  COPY(CPSLimit);
  COPY(NewerOnly);
  COPY2(EncryptNewFiles);
  COPY2(ExcludeHiddenFiles);
  COPY2(ExcludeEmptyDirectories);
  COPY2(Size);
  COPY2(PartOffset);
  COPY2(PartSize);
  COPY2(OnceDoneOperation);
  COPY2(OnTransferOut);
  COPY2(OnTransferIn);
#undef COPY
}

TCopyParamType &TCopyParamType::operator=(const TCopyParamType &rhs)
{
  Assign(&rhs);
  return *this;
}

void TCopyParamType::SetLocalInvalidChars(const UnicodeString & Value)
{
  if (Value != GetLocalInvalidChars())
  {
    FLocalInvalidChars = Value;
    FTokenizibleChars = FLocalInvalidChars; // + TokenPrefix;
  }
}

bool TCopyParamType::GetReplaceInvalidChars() const
{
  return (GetInvalidCharsReplacement() != NoReplacement);
}

void TCopyParamType::SetReplaceInvalidChars(bool Value)
{
  if (GetReplaceInvalidChars() != Value)
  {
    SetInvalidCharsReplacement(Value ? TokenReplacement : NoReplacement);
  }
}

UnicodeString TCopyParamType::ValidLocalFileName(const UnicodeString & AFileName) const
{
  return ::ValidLocalFileName(AFileName, GetInvalidCharsReplacement(), FTokenizibleChars, LOCAL_INVALID_CHARS);
}

UnicodeString TCopyParamType::RestoreChars(const UnicodeString AFileName) const
{
  UnicodeString FileName = AFileName;
  if (GetInvalidCharsReplacement() == TokenReplacement)
  {
    wchar_t *InvalidChar = ToWChar(FileName);
    while ((InvalidChar = wcschr(InvalidChar, TokenPrefix)) != nullptr)
    {
      int32_t Index = InvalidChar - FileName.c_str() + 1;
      if (FileName.Length() >= Index + 2)
      {
        UnicodeString Hex = FileName.SubString(Index + 1, 2);
        wchar_t Char = static_cast<wchar_t>(HexToByte(Hex));
        if ((Char != L'\0') &&
          ((FTokenizibleChars.Pos(Char) > 0) ||
            (((Char == L' ') || (Char == L'.')) && (Index == FileName.Length() - 2))))
        {
          FileName[Index] = Char;
          FileName.Delete(Index + 1, 2);
          InvalidChar = ToWChar(FileName) + Index;
        }
        else if ((Hex == L"00") &&
          ((Index == FileName.Length() - 2) || (FileName[Index + 3] == L'.')) &&
          IsReservedName(FileName.SubString(1, Index - 1) + FileName.SubString(Index + 3, FileName.Length() - Index - 3 + 1)))
        {
          FileName.Delete(Index, 3);
          InvalidChar = ToWChar(FileName) + Index - 1;
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
  return FileName;
}

UnicodeString TCopyParamType::ValidLocalPath(const UnicodeString & APath) const
{
  UnicodeString Result;
  UnicodeString Path = APath;
  while (!Path.IsEmpty())
  {
    if (!Result.IsEmpty())
    {
      Result += L"\\";
    }
    Result += ValidLocalFileName(CutToChar(Path, L'\\', false));
  }
  return Result;
}

UnicodeString TCopyParamType::ChangeFileName(const UnicodeString AFileName,
  TOperationSide Side, bool FirstLevel) const
{
  UnicodeString FileName = AFileName;
  if (FirstLevel)
  {
    FileName = MaskFileName(FileName, GetFileMask());
  }
  switch (GetFileNameCase())
  {
  case ncUpperCase:
    FileName = FileName.UpperCase();
    break;
  case ncLowerCase:
    FileName = FileName.LowerCase();
    break;
  case ncFirstUpperCase:
    FileName = FileName.SubString(1, 1).UpperCase() +
      FileName.SubString(2, FileName.Length() - 1).LowerCase();
    break;
  case ncLowerCaseShort:
    if ((FileName.Length() <= 12) && (FileName.Pos(L".") <= 9) &&
      (FileName == FileName.UpperCase()))
    {
      FileName = FileName.LowerCase();
    }
    break;
  case ncNoChange:
  default:
    /*nothing*/
    break;
  }
  if (Side == osRemote)
  {
    FileName = ValidLocalFileName(FileName);
  }
  else
  {
    FileName = RestoreChars(FileName);
  }
  return FileName;
}

bool TCopyParamType::UseAsciiTransfer(const UnicodeString AFileName,
  TOperationSide Side, const TFileMasks::TParams &Params) const
{
  switch (GetTransferMode())
  {
  case tmBinary:
    return false;
  case tmAscii:
    return true;
  case tmAutomatic:
    return GetAsciiFileMask().Matches(AFileName, (Side == osLocal),
        false, &Params);
  default:
    DebugFail();
    return false;
  }
}

TRights TCopyParamType::RemoteFileRights(uint32_t Attrs) const
{
  TRights R = GetRights();
  if ((Attrs & faDirectory) && GetAddXToDirectories())
    R.AddExecute();
  return R;
}

UnicodeString TCopyParamType::GetLogStr() const
{
  wchar_t CaseC[] = L"NULFS";
  wchar_t ModeC[] = L"BAM";
  wchar_t ResumeC[] = L"YSN";
  // OpenArray (ARRAYOFCONST) supports only up to 19 arguments, so we had to split it
  return
    FORMAT(
      "  PrTime: %s%s; PrRO: %s; Rght: %s; PrR: %s (%s); FnCs: %c; RIC: %s; "
      "Resume: %s (%d); CalcS: %s; Mask: %s\n",
      BooleanToEngStr(GetPreserveTime()),
      UnicodeString(GetPreserveTime() && GetPreserveTimeDirs() ? L"+Dirs" : L""),
      BooleanToEngStr(GetPreserveReadOnly()),
      GetRights().GetText(),
      BooleanToEngStr(GetPreserveRights()),
      BooleanToEngStr(GetIgnorePermErrors()),
      CaseC[GetFileNameCase()],
      CharToHex(GetInvalidCharsReplacement()),
      ResumeC[GetResumeSupport()],
      nb::ToInt(GetResumeThreshold()),
      BooleanToEngStr(GetCalculateSize()),
      GetFileMask()) +
    FORMAT(
      "  TM: %s; ClAr: %s; RemEOF: %s; RemBOM: %s; CPS: %u; NewerOnly: %s; EncryptNewFiles: %s; ExcludeHiddenFiles: %s; ExcludeEmptyDirectories: %s; InclM: %s; ResumeL: %d\n"
      "  AscM: %s\n",
      ModeC[GetTransferMode()],
      BooleanToEngStr(GetClearArchive()),
      BooleanToEngStr(GetRemoveCtrlZ()),
      BooleanToEngStr(GetRemoveBOM()),
      uint32_t(GetCPSLimit()),
      BooleanToEngStr(GetNewerOnly()),
      BooleanToEngStr(EncryptNewFiles),
      BooleanToEngStr(ExcludeHiddenFiles),
      BooleanToEngStr(ExcludeEmptyDirectories),
      IncludeFileMask.Masks,
      ((FTransferSkipList.get() != nullptr) ? FTransferSkipList->Count : 0) + (!FTransferResumeFile.IsEmpty() ? 1 : 0),
      AsciiFileMask.Masks);
}

DWORD TCopyParamType::LocalFileAttrs(const TRights &Rights) const
{
  DWORD Result = 0;
  if (GetPreserveReadOnly() && !Rights.GetRight(TRights::rrUserWrite))
  {
    Result |= faReadOnly;
  }
  return Result;
}

bool TCopyParamType::AllowResume(int64_t Size, const UnicodeString & FileName) const
{
  bool Result;
  if (FileName.Length() + PartialExt.Length() > 255) // it's a different limit than MAX_PATH
  {
    Result = false;
  }
  else
  {
  switch (GetResumeSupport())
    {
  case rsOn:
    return true;
  case rsOff:
    return false;
  case rsSmart:
    return (Size >= GetResumeThreshold());
  default:
    DebugFail();
    return false;
    }
  }
  return Result;
}

bool TCopyParamType::AllowAnyTransfer() const
{
  return
    GetIncludeFileMask().Masks().IsEmpty() &&
    !ExcludeHiddenFiles &&
    !ExcludeEmptyDirectories &&
    ((FTransferSkipList.get() == nullptr) || (FTransferSkipList->Count == 0)) &&
    FTransferResumeFile.IsEmpty();
}

bool TCopyParamType::AllowTransfer(const UnicodeString & AFileName,
  TOperationSide Side, bool Directory, const TFileMasks::TParams & Params, bool Hidden) const
{
  bool Result = true;
  if (Hidden && ExcludeHiddenFiles)
  {
    Result = false;
  }
  else if (!IncludeFileMask.Masks().IsEmpty())
  {
    Result = GetIncludeFileMask().Matches(AFileName, (Side == osLocal),
        Directory, &Params);
  }
  return Result;
}

bool TCopyParamType::SkipTransfer(
  const UnicodeString & AFileName, bool Directory) const
{
  bool Result = false;
  // we deliberately do not filter directories, as path is added to resume list
  // when a transfer of file or directory is started,
  // so for directories we need to recurse and check every single file
  if (!Directory && FTransferSkipList.get() != nullptr)
  {
    Result = (FTransferSkipList->IndexOf(AFileName) >= 0);
  }
  return Result;
}

bool TCopyParamType::ResumeTransfer(const UnicodeString AFileName) const
{
  // Returning true has the same effect as cpResume
  return
    (AFileName == FTransferResumeFile) &&
    DebugAlwaysTrue(!FTransferResumeFile.IsEmpty());
}

const TStrings * TCopyParamType::GetTransferSkipList() const
{
  return FTransferSkipList.get();
}

void TCopyParamType::SetTransferSkipList(const TStrings * Value)
{
  if ((Value == nullptr) || (Value->GetCount() == 0))
  {
    FTransferSkipList.reset(nullptr);
  }
  else
  {
    FTransferSkipList = std::make_unique<TStringList>();
    FTransferSkipList->AddStrings(Value);
    FTransferSkipList->SetSorted(true);
  }
}

void TCopyParamType::Load(THierarchicalStorage *Storage)
{
  SetAddXToDirectories(Storage->ReadBool("AddXToDirectories", GetAddXToDirectories()));
  GetAsciiFileMask().Masks(Storage->ReadString("Masks", GetAsciiFileMask().Masks()));
  SetFileNameCase(static_cast<TFileNameCase>(Storage->ReadInteger("FileNameCase", GetFileNameCase())));
  SetPreserveReadOnly(Storage->ReadBool("PreserveReadOnly", GetPreserveReadOnly()));
  SetPreserveTime(Storage->ReadBool("PreserveTime", GetPreserveTime()));
  SetPreserveTimeDirs(Storage->ReadBool("PreserveTimeDirs", GetPreserveTimeDirs()));
  SetPreserveRights(Storage->ReadBool("PreserveRights", GetPreserveRights()));
  SetIgnorePermErrors(Storage->ReadBool("IgnorePermErrors", GetIgnorePermErrors()));
  FRights.SetText(Storage->ReadString("Text", GetRights().GetText()));
  SetTransferMode(static_cast<TTransferMode>(Storage->ReadInteger("TransferMode", GetTransferMode())));
  SetResumeSupport(static_cast<TResumeSupport>(Storage->ReadInteger("ResumeSupport", GetResumeSupport())));
  SetResumeThreshold(Storage->ReadInt64("ResumeThreshold", GetResumeThreshold()));
  SetInvalidCharsReplacement(static_cast<wchar_t>(Storage->ReadInteger("ReplaceInvalidChars",
        nb::ToInt(GetInvalidCharsReplacement()))));
  SetLocalInvalidChars(Storage->ReadString("LocalInvalidChars", GetLocalInvalidChars()));
  SetCalculateSize(Storage->ReadBool("CalculateSize", GetCalculateSize()));
  if (Storage->ValueExists("IncludeFileMask"))
  {
    GetIncludeFileMask().Masks(Storage->ReadString("IncludeFileMask", GetIncludeFileMask().Masks()));
  }
  else if (Storage->ValueExists("ExcludeFileMask"))
  {
    UnicodeString ExcludeFileMask = Storage->ReadString("ExcludeFileMask", UnicodeString());
    if (!ExcludeFileMask.IsEmpty())
    {
      bool NegativeExclude = Storage->ReadBool("NegativeExclude", false);
      if (NegativeExclude)
      {
        GetIncludeFileMask().Masks(ExcludeFileMask);
      }
      // convert at least simple cases to new format
      else if (ExcludeFileMask.Pos(IncludeExcludeFileMasksDelimiter) == 0)
      {
        GetIncludeFileMask().Masks(UnicodeString(IncludeExcludeFileMasksDelimiter) + ExcludeFileMask);
      }
    }
  }
  SetTransferSkipList(nullptr);
  SetTransferResumeFile("");
  SetClearArchive(Storage->ReadBool("ClearArchive", GetClearArchive()));
  SetRemoveCtrlZ(Storage->ReadBool("RemoveCtrlZ", GetRemoveCtrlZ()));
  SetRemoveBOM(Storage->ReadBool("RemoveBOM", GetRemoveBOM()));
  SetCPSLimit(Storage->ReadInteger("CPSLimit", GetCPSLimit()));
  SetNewerOnly(Storage->ReadBool("NewerOnly", GetNewerOnly()));
  EncryptNewFiles = Storage->ReadBool("EncryptNewFiles", EncryptNewFiles);
  ExcludeHiddenFiles = Storage->ReadBool("ExcludeHiddenFiles", ExcludeHiddenFiles);
  ExcludeEmptyDirectories = Storage->ReadBool("ExcludeEmptyDirectories", ExcludeEmptyDirectories);
  Size = -1;
  PartOffset = -1;
  PartSize = -1;
  OnceDoneOperation = odoIdle;
  FOnTransferOut = nullptr;
  FOnTransferIn = nullptr;
}

void TCopyParamType::Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults) const
{
  nb::used(Storage);
  nb::used(Defaults);
  // Same as in TSessionData::DoSave
#undef WRITE_DATA_EX
#define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
    if ((Defaults != nullptr) && (CONV(Defaults->PROPERTY) == CONV(PROPERTY))) \
    { \
      Storage->DeleteValue(NAME); \
    } \
    else \
    { \
      Storage->Write ## TYPE(NAME, CONV(PROPERTY)); \
    }
  #define WRITE_DATA_CONV(TYPE, NAME, PROPERTY) WRITE_DATA_EX(TYPE, NAME, PROPERTY, WRITE_DATA_CONV_FUNC)
  #undef WRITE_DATA
  #define WRITE_DATA(TYPE, PROPERTY) WRITE_DATA_EX(TYPE, TEXT(#PROPERTY), PROPERTY, )

  WRITE_DATA(Bool, AddXToDirectories);
  WRITE_DATA_EX(String, "Masks", AsciiFileMask.Masks, );
  WRITE_DATA(Integer, FileNameCase);
  WRITE_DATA(Bool, PreserveReadOnly);
  WRITE_DATA(Bool, PreserveTime);
  WRITE_DATA(Bool, PreserveTimeDirs);
  WRITE_DATA(Bool, PreserveRights);
  WRITE_DATA(Bool, IgnorePermErrors);
  WRITE_DATA_EX(String, "Text", Rights.Text, );
  WRITE_DATA(Integer, TransferMode);
  WRITE_DATA(Integer, ResumeSupport);
  WRITE_DATA(Int64, ResumeThreshold);
  #undef WRITE_DATA_CONV_FUNC
  #define WRITE_DATA_CONV_FUNC(X) (uint32_t)(X)
  WRITE_DATA_CONV(Integer, "ReplaceInvalidChars", InvalidCharsReplacement);
  WRITE_DATA(String, LocalInvalidChars);
  WRITE_DATA(Bool, CalculateSize);
  WRITE_DATA_EX(String, "IncludeFileMask", IncludeFileMask.Masks, );
  Storage->DeleteValue(L"ExcludeFileMask"); // obsolete
  Storage->DeleteValue(L"NegativeExclude"); // obsolete
  DebugAssert(FTransferSkipList.get() == nullptr);
  DebugAssert(FTransferResumeFile.IsEmpty());
  WRITE_DATA(Bool, ClearArchive);
  WRITE_DATA(Bool, RemoveCtrlZ);
  WRITE_DATA(Bool, RemoveBOM);
  WRITE_DATA(Integer, CPSLimit);
  WRITE_DATA(Bool, NewerOnly);
  WRITE_DATA(Bool, EncryptNewFiles);
  WRITE_DATA(Bool, ExcludeHiddenFiles);
  WRITE_DATA(Bool, ExcludeEmptyDirectories);
  DebugAssert(Size < 0);
  DebugAssert(PartOffset < 0);
  DebugAssert(PartSize < 0);
  DebugAssert(OnceDoneOperation == odoIdle);
  DebugAssert(FOnTransferOut.empty());
  DebugAssert(FOnTransferIn.empty());
}

#define C(Property) (Get ## Property() == rhp.Get ## Property())
#define C2(Property) (F##Property == rhp.F##Property)

bool TCopyParamType::operator==(const TCopyParamType &rhp) const
{
  DebugAssert(FTransferSkipList.get() == nullptr);
  DebugAssert(FTransferResumeFile.IsEmpty());
  DebugAssert(FOnTransferOut.empty());
  DebugAssert(FOnTransferIn.empty());
  DebugAssert(rhp.FTransferSkipList.get() == nullptr);
  DebugAssert(rhp.FTransferResumeFile.IsEmpty());
  DebugAssert(rhp.FOnTransferOut.empty());
  DebugAssert(rhp.FOnTransferIn.empty());
  return
    C(AddXToDirectories) &&
    C(AsciiFileMask) &&
    C(FileNameCase) &&
    C(PreserveReadOnly) &&
    C(PreserveTime) &&
    C(PreserveTimeDirs) &&
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
    C(RemoveCtrlZ) &&
    C(RemoveBOM) &&
    C(CPSLimit) &&
    C(NewerOnly) &&
    C2(EncryptNewFiles) &&
    C2(ExcludeHiddenFiles) &&
    C2(ExcludeEmptyDirectories) &&
    C2(Size) &&
    C2(PartOffset) &&
    C2(PartSize) &&
    C2(OnceDoneOperation) &&
    true;
}
#undef C2
#undef C

const uint32_t MinSpeed = 8 * 1024;
const uint32_t MaxSpeed = 8 * 1024 * 1024;

static bool TryGetSpeedLimit(const UnicodeString Text, uint32_t &Speed)
{
  bool Result;
  if (AnsiSameText(Text, LoadStr(SPEED_UNLIMITED)))
  {
    Speed = 0;
    Result = true;
  }
  else
  {
    int64_t SSpeed;
    Result = TryStrToInt64(Text, SSpeed) && (SSpeed >= 0);
    if (Result)
    {
      Speed = nb::ToUIntPtr(SSpeed * 1024);
    }
  }
  return Result;
}

uint32_t GetSpeedLimit(const UnicodeString Text)
{
  uint32_t Speed = 0;
  if (!TryGetSpeedLimit(Text, Speed))
  {
    throw Exception(FMTLOAD(SPEED_INVALID, Text));
  }
  return Speed;
}

UnicodeString SetSpeedLimit(uint32_t Limit)
{
  UnicodeString Text;
  if (Limit == 0)
  {
    Text = LoadStr(SPEED_UNLIMITED);
  }
  else
  {
    Text = ::IntToStr(Limit / 1024);
  }
  return Text;
}

void CopySpeedLimits(TStrings *Source, TStrings *Dest)
{
  std::unique_ptr<TStringList> Temp(std::make_unique<TStringList>());

  bool Unlimited = false;
  for (int32_t Index = 0; Index < Source->GetCount(); ++Index)
  {
    UnicodeString Text = Source->GetString(Index);
    uint32_t Speed;
    bool Valid = TryGetSpeedLimit(Text, Speed);
    if ((!Valid || (Speed == 0)) && !Unlimited)
    {
      Temp->Add(LoadStr(SPEED_UNLIMITED));
      Unlimited = true;
    }
    else if (Valid && (Speed > 0))
    {
      Temp->Add(Text);
    }
  }

  if (!Unlimited)
  {
    Temp->Insert(0, LoadStr(SPEED_UNLIMITED));
  }

  Dest->Assign(Temp.get());
}

TOperationSide ReverseOperationSide(TOperationSide Side)
{
  TOperationSide Result;
  switch (Side)
  {
    case osLocal:
      Result = osRemote;
      break;

    case osRemote:
      Result = osLocal;
      break;

    default:
    case osCurrent:
      DebugFail();
      Result = Side;
      break;
  }
  return Result;
}
