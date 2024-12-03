
#include <vcl.h>
#pragma hdrstop

#include "ScpFileSystem.h"

#include "Terminal.h"
#include <Common.h>
#include <Exceptions.h>
#include "Interface.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "SecureShell.h"
#include <StrUtils.hpp>

#include <stdio.h>

// #pragma package(smart_init)
#undef FILE_OPERATION_LOOP_TERMINAL
#define FILE_OPERATION_LOOP_TERMINAL FTerminal
//---------------------------------------------------------------------------
constexpr int32_t coRaiseExcept = 1;
constexpr int32_t coExpectNoOutput = 2;
constexpr int32_t coWaitForLastLine = 4;
constexpr int32_t coOnlyReturnCode = 8;
constexpr int32_t coIgnoreWarnings = 16;
constexpr int32_t coReadProgress = 32;
constexpr int32_t coIgnoreStdErr = 64;

constexpr int32_t ecRaiseExcept = 0x01;
constexpr int32_t ecIgnoreWarnings = 0x02;
constexpr int32_t ecReadProgress = 0x04;
constexpr int32_t ecIgnoreStdErr = 0x08;
constexpr int32_t ecNoEnsureLocation = 0x10;
constexpr int32_t ecOnlyReturnCode = 0x20;
constexpr int32_t ecDefault = ecRaiseExcept;
//---------------------------------------------------------------------------
DERIVE_EXT_EXCEPTION(EScpFileSkipped, ESkipFile);
//===========================================================================
constexpr TFSCommand MaxShellCommand = fsLang;
constexpr int32_t ShellCommandCount = MaxShellCommand + 1;
constexpr int32_t MaxCommandLen = 40;
struct TCommandType
{
  CUSTOM_MEM_ALLOCATION_IMPL
  int32_t MinLines{0};
  int32_t MaxLines{0};
  bool ModifiesFiles{false};
  bool ChangesDirectory{false};
  bool InteractiveCommand{false};
  char Command[MaxCommandLen]{};
};

// Only one character! See TSCPFileSystem::ReadCommandOutput()
#define LastLineSeparator ":"
constexpr const wchar_t * LAST_LINE = L"NetBox: this is end-of-file";
constexpr const wchar_t * FIRST_LINE = L"NetBox: this is begin-of-file";
extern const TCommandType DefaultCommandSet[];

#define CHECK_CMD DebugAssert((Cmd >=0) && (Cmd <= MaxShellCommand))

class TSessionData;

class TCommandSet final : public TObject
{
  NB_DISABLE_COPY(TCommandSet)
private:
  TCommandType CommandSet[ShellCommandCount]{};
  TSessionData * FSessionData{nullptr};
  UnicodeString FReturnVar;
public:
  int32_t GetMaxLines(TFSCommand Cmd) const;
  int32_t GetMinLines(TFSCommand Cmd) const;
  bool GetModifiesFiles(TFSCommand Cmd) const;
  bool GetChangesDirectory(TFSCommand Cmd) const;
  bool GetOneLineCommand(TFSCommand Cmd) const;
  void SetCommands(TFSCommand Cmd, const UnicodeString & Value);
  UnicodeString GetCommands(TFSCommand Cmd) const;
  UnicodeString GetFirstLine() const;
  bool GetInteractiveCommand(TFSCommand Cmd) const;
  UnicodeString GetLastLine() const;
  UnicodeString GetReturnVar() const;

public:
  TCommandSet() = delete;
  explicit TCommandSet(TSessionData * ASessionData);
  void Default();
  void CopyFrom(TCommandSet * Source);
  UnicodeString Command(TFSCommand Cmd, fmt::ArgList args);
  FMT_VARIADIC_W(UnicodeString, Command, TFSCommand)
  TStrings * CreateCommandList() const;
  UnicodeString FullCommand(TFSCommand Cmd, fmt::ArgList args);
  FMT_VARIADIC_W(UnicodeString, FullCommand, TFSCommand)
  static UnicodeString ExtractCommand(const UnicodeString & ACommand);
  /*__property int MaxLines[TFSCommand Cmd]  = { read=GetMaxLines};
  __property int MinLines[TFSCommand Cmd]  = { read=GetMinLines };
  __property bool ModifiesFiles[TFSCommand Cmd]  = { read=GetModifiesFiles };
  __property bool ChangesDirectory[TFSCommand Cmd]  = { read=GetChangesDirectory };
  __property bool OneLineCommand[TFSCommand Cmd]  = { read=GetOneLineCommand };
  __property UnicodeString Commands[TFSCommand Cmd]  = { read=GetCommands, write=SetCommands };
  __property UnicodeString FirstLine = { read = GetFirstLine };
  __property bool InteractiveCommand[TFSCommand Cmd] = { read = GetInteractiveCommand };*/
  __property UnicodeString LastLine  = { read=GetLastLine };
  __property TSessionData * SessionData  = { read=FSessionData, write=FSessionData };
  __property UnicodeString ReturnVar  = { read=GetReturnVar, write=FReturnVar };

  TSessionData * GetSessionData() const { return FSessionData; }
  void SetSessionData(TSessionData * Value) { FSessionData = Value; }
  void SetReturnVar(const UnicodeString & Value) { FReturnVar = Value; }
};
//===========================================================================
constexpr const char NationalVars[][15] =
  {"LANG", "LANGUAGE", "LC_CTYPE", "LC_COLLATE", "LC_MONETARY", "LC_NUMERIC",
  "LC_TIME", "LC_MESSAGES", "LC_ALL", "HUMAN_BLOCKS", "BLOCK_SIZE", "LS_BLOCK_SIZE" };
constexpr int32_t NationalVarCount = _countof(NationalVars);
constexpr const char FullTimeOption[] = "--full-time";

#define F false
#define T true
TODO("remove 'mf' and 'cd, it is implemented in TTerminal already");
const TCommandType DefaultCommandSet[ShellCommandCount] =
{
  //                       min max mf cd ia  command
  /*Null*/                { -1, -1, F, F, F, "" },
  /*VarValue*/            { -1, -1, F, F, F, "echo \"$%s\"" /* variable */ },
  /*LastLine*/            { -1, -1, F, F, F, "echo \"%s" LastLineSeparator "%s\"" /* last line, return var */ },
  /*FirstLine*/           { -1, -1, F, F, F, "echo \"%s\"" /* first line */ },
  /*CurrentDirectory*/    {  1,  1, F, F, F, "pwd" },
  /*ChangeDirectory*/     {  0,  0, F, T, F, "cd %s" /* directory */ },
  // list directory can be empty on permission denied, this is handled in ReadDirectory
  /*ListDirectory*/       { -1, -1, F, F, F, "%s %s \"%s\"" /* listing command, options, directory */ },
  /*ListCurrentDirectory*/{ -1, -1, F, F, F, "%s %s" /* listing command, options */ },
  /*ListFile*/            {  1,  1, F, F, F, "%s -d %s \"%s\"" /* listing command, options, file/directory */ },
  /*LookupUsersGroups*/   {  0,  1, F, F, F, "groups" },
  /*Whoami*/              {  0,  1, F, F, F, "whoami" },
  /*CopyToRemote*/        { -1, -1, T, F, T, "scp -r %s -d -t \"%s\"" /* options, directory */ },
  /*CopyToLocal*/         { -1, -1, F, F, T, "scp -r %s -d -f \"%s\"" /* options, file */ },
  /*DeleteFile*/          {  0,  0, T, F, F, "rm -f -r \"%s\"" /* file/directory */},
  /*RenameFile*/          {  0,  0, T, F, F, "mv -f \"%s\" \"%s\"" /* file/directory, new name*/},
  /*CreateDirectory*/     {  0,  0, T, F, F, "mkdir \"%s\"" /* new directory */},
  /*ChangeMode*/          {  0,  0, T, F, F, "chmod %s %s \"%s\"" /* options, mode, filename */},
  /*ChangeGroup*/         {  0,  0, T, F, F, "chgrp %s \"%s\" \"%s\"" /* options, group, filename */},
  /*ChangeOwner*/         {  0,  0, T, F, F, "chown %s \"%s\" \"%s\"" /* options, owner, filename */},
  /*HomeDirectory*/       {  0,  0, F, T, F, "cd" },
  /*Unset*/               {  0,  0, F, F, F, "unset \"%s\"" /* variable */ },
  /*Unalias*/             {  0,  0, F, F, F, "unalias \"%s\"" /* alias */ },
  /*CreateLink*/          {  0,  0, T, F, F, "ln %s \"%s\" \"%s\"" /*symbolic (-s), filename, point to*/},
  /*CopyFile*/            {  0,  0, T, F, F, "cp -p -r -f %s \"%s\" \"%s\"" /* file/directory, target name*/},
  /*AnyCommand*/          {  0, -1, T, T, F, "%s" },
  /*Lang*/                {  0,  1, F, F, F, "printenv LANG"},
};
#undef F
#undef T

TCommandSet::TCommandSet(TSessionData * ASessionData) :
  FSessionData(ASessionData), FReturnVar(L"")
{
  DebugAssert(FSessionData);
  Default();
}

void TCommandSet::CopyFrom(TCommandSet * Source)
{
  memmove(&CommandSet, Source->CommandSet, sizeof(CommandSet));
}

void TCommandSet::Default()
{
  DebugAssert(sizeof(CommandSet) == sizeof(DefaultCommandSet));
  memmove(&CommandSet, &DefaultCommandSet, sizeof(CommandSet));
}

int32_t TCommandSet::GetMaxLines(TFSCommand Cmd) const
{
  CHECK_CMD;
  if ((Cmd >=0) && (Cmd <= MaxShellCommand))
    return CommandSet[Cmd].MaxLines;
  return 0;
}

int32_t TCommandSet::GetMinLines(TFSCommand Cmd) const
{
  CHECK_CMD;
  if ((Cmd >=0) && (Cmd <= MaxShellCommand))
    return CommandSet[Cmd].MinLines;
  return 0;
}

bool TCommandSet::GetModifiesFiles(TFSCommand Cmd) const
{
  CHECK_CMD;
  if ((Cmd >=0) && (Cmd <= MaxShellCommand))
    return CommandSet[Cmd].ModifiesFiles;
  return false;
}

bool TCommandSet::GetChangesDirectory(TFSCommand Cmd) const
{
  CHECK_CMD;
  if ((Cmd >=0) && (Cmd <= MaxShellCommand))
    return CommandSet[Cmd].ChangesDirectory;
  return false;
}

bool TCommandSet::GetInteractiveCommand(TFSCommand Cmd) const
{
  CHECK_CMD;
  if ((Cmd >=0) && (Cmd <= MaxShellCommand))
    return CommandSet[Cmd].InteractiveCommand;
  return false;
}

bool TCommandSet::GetOneLineCommand(TFSCommand /*Cmd*/) const
{
  //CHECK_CMD;
  // #56: we send "echo last line" from all commands on same line
  // just as it was in 1.0
  return True; //CommandSet[Cmd].OneLineCommand;
}

void TCommandSet::SetCommands(TFSCommand Cmd, const UnicodeString & Value)
{
  CHECK_CMD;
  const AnsiString AnsiValue(Value);
  strcpy_s(const_cast<char *>(CommandSet[Cmd].Command), MaxCommandLen, AnsiValue.SubString(1, MaxCommandLen - 1).c_str());
}

UnicodeString TCommandSet::GetCommands(TFSCommand Cmd) const
{
  CHECK_CMD;
  if ((Cmd >=0) && (Cmd <= MaxShellCommand))
    return CommandSet[Cmd].Command;
  return EmptyStr;
}

UnicodeString TCommandSet::Command(TFSCommand Cmd, fmt::ArgList args)
{
  UnicodeString Result = nb::Sprintf(GetCommands(Cmd), args);
  return Result;
}

UnicodeString TCommandSet::FullCommand(TFSCommand Cmd, fmt::ArgList args)
{
  UnicodeString Separator;
  if (GetOneLineCommand(Cmd))
  {
    Separator = L" ; ";
  }
  else
  {
    Separator = L"\n";
  }
  const UnicodeString Line = Command(Cmd, args);
  const UnicodeString LastLineCmd =
    nb::Sprintf(GetCommands(fsLastLine), GetLastLine(), GetReturnVar());
  UnicodeString FirstLineCmd;
  if (GetInteractiveCommand(Cmd))
  {
    FirstLineCmd = nb::Sprintf(GetCommands(fsFirstLine), GetFirstLine()) + Separator;
  }

  UnicodeString Result;
  if (!Line.IsEmpty())
  {
    Result = FORMAT("%s%s%s%s", FirstLineCmd, Line, Separator, LastLineCmd);
  }
  else
  {
    Result = FORMAT("%s%s", FirstLineCmd, LastLineCmd);
  }
  return Result;
}

UnicodeString TCommandSet::GetFirstLine() const
{
  return FIRST_LINE;
}

UnicodeString TCommandSet::GetLastLine() const
{
  return LAST_LINE;
}

UnicodeString TCommandSet::GetReturnVar() const
{
  DebugAssert(GetSessionData());
  if (!FReturnVar.IsEmpty())
  {
    return UnicodeString(1, L'$') + FReturnVar;
  }
  if (GetSessionData()->GetDetectReturnVar())
  {
    return UnicodeString(1, L'0');
  }
  return UnicodeString(1, L'$') + GetSessionData()->GetReturnVar();
}

UnicodeString TCommandSet::ExtractCommand(const UnicodeString & ACommand)
{
  UnicodeString Command = ACommand;
  const int32_t P = Command.Pos(L" ");
  if (P > 0)
  {
    Command.SetLength(P - 1);
  }
  return Command;
}

TStrings * TCommandSet::CreateCommandList() const
{
  std::unique_ptr<TStrings> CommandList(std::make_unique<TStringList>());
  for (int32_t Index = 0; Index < ShellCommandCount; ++Index)
  {
    UnicodeString Cmd = GetCommands(static_cast<TFSCommand>(Index));
    if (!Cmd.IsEmpty())
    {
      Cmd = ExtractCommand(Cmd);
      if ((Cmd != L"%s") && (CommandList->IndexOf(Cmd) < 0))
        CommandList->Add(Cmd);
    }
  }
  return CommandList.release();
}
//===========================================================================
TSCPFileSystem::TSCPFileSystem(TTerminal * ATerminal) noexcept :
  TCustomFileSystem(OBJECT_CLASS_TSCPFileSystem, ATerminal)
{
}

void TSCPFileSystem::Init(void * Data)
{
  DebugAssert(Data);
  FSecureShell = static_cast<TSecureShell *>(Data);
  DebugAssert(FSecureShell);
  FCommandSet = std::make_unique<TCommandSet>(FTerminal->GetSessionData());
  FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
  FScpFatalError = false;
  FOutput = std::make_unique<TStringList>();
  FProcessingCommand = false;
  FOnCaptureOutput = nullptr;

  FFileSystemInfo.ProtocolBaseName = L"SCP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
}

TSCPFileSystem::~TSCPFileSystem() noexcept
{
#if defined(__BORLANDC__)
  delete FCommandSet;
  delete FOutput;
  delete FSecureShell;
#endif
  SAFE_DESTROY(FSecureShell);
}

void TSCPFileSystem::Open()
{
  // this is used for reconnects only
  FSecureShell->Open();
}

void TSCPFileSystem::Close()
{
  FSecureShell->Close();
}

bool TSCPFileSystem::GetActive() const
{
  return FSecureShell->GetActive();
}

void TSCPFileSystem::CollectUsage()
{
  FSecureShell->CollectUsage();
}

const TSessionInfo & TSCPFileSystem::GetSessionInfo() const
{
  return FSecureShell->GetSessionInfo();
}

const TFileSystemInfo & TSCPFileSystem::GetFileSystemInfo(bool Retrieve)
{
  if (FFileSystemInfo.AdditionalInfo.IsEmpty() && Retrieve)
  {
    UnicodeString UName;
    FTerminal->SetExceptionOnFail(true);
    try__finally
    {
      try
      {
        AnyCommand(L"uname -a", nullptr);
        for (int32_t Index = 0; Index < GetOutput()->GetCount(); ++Index)
        {
          if (Index > 0)
          {
            UName += L"; ";
          }
          UName += GetOutput()->GetString(Index);
        }
      }
      catch (...)
      {
        if (!FTerminal->GetActive())
        {
          throw;
        }
      }
    }
    __finally
    {
      FTerminal->SetExceptionOnFail(false);
    } end_try__finally

    FFileSystemInfo.RemoteSystem = UName;
  }

  return FFileSystemInfo;
}

bool TSCPFileSystem::TemporaryTransferFile(const UnicodeString & /*AFileName*/)
{
  return false;
}

bool TSCPFileSystem::GetStoredCredentialsTried() const
{
  return FSecureShell->GetStoredCredentialsTried();
}

UnicodeString TSCPFileSystem::GetUserName() const
{
  return FSecureShell->ShellGetUserName();
}

void TSCPFileSystem::Idle()
{
  // Keep session alive
  const TSessionData * Data = FTerminal->GetSessionData();
  if ((Data->GetPingType() != ptOff) &&
      (Now() - FSecureShell->GetLastDataSent() > Data->GetPingIntervalDT()))
  {
    if ((Data->GetPingType() == ptDummyCommand) &&
        FSecureShell->GetReady())
    {
      if (!FProcessingCommand)
      {
        ExecCommand(fsNull, 0);
      }
      else
      {
        FTerminal->LogEvent("Cannot send keepalive, command is being executed");
        // send at least SSH-level keepalive, if nothing else, it at least updates
        // LastDataSent, no the next keepalive attempt is postponed
        FSecureShell->KeepAlive();
      }
    }
    else
    {
      FSecureShell->KeepAlive();
    }
  }

  FSecureShell->Idle();
}

UnicodeString TSCPFileSystem::AbsolutePath(const UnicodeString & APath, bool Local)
{
  return static_cast<const TSCPFileSystem *>(this)->AbsolutePath(APath, Local);
}

UnicodeString TSCPFileSystem::AbsolutePath(const UnicodeString & APath, bool /*Local*/) const
{
  return base::AbsolutePath(GetCurrentDirectory(), APath);
}

bool TSCPFileSystem::IsCapable(int32_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability) {
    case fcUserGroupListing:
    case fcModeChanging:
    case fcModeChangingUpload:
    case fcPreservingTimestampUpload:
    case fcGroupChanging:
    case fcOwnerChanging:
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcSymbolicLink:
    case fcResolveSymlink:
    case fcRename:
    case fcRemoteMove:
    case fcRemoteCopy:
    case fcRemoveCtrlZUpload:
    case fcRemoveBOMUpload:
    case fcCalculatingChecksum:
    case fcMoveOverExistingFile:
      return true;

    case fcTextMode:
      return FTerminal->GetSessionData()->GetEOLType() != FTerminal->GetConfiguration()->GetLocalEOLType();

    case fcAclChangingFiles:
    case fcNativeTextMode:
    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcLoadingAdditionalProperties:
    case fcCheckingSpaceAvailable:
    case fcIgnorePermErrors:
    case fcSecondaryShell: // has fcShellAnyCommand
    case fcGroupOwnerChangingByID: // by name
    case fcMoveToQueue:
    case fcLocking:
    case fcPreservingTimestampDirs:
    case fcResumeSupport:
    case fcSkipTransfer:
    case fcParallelTransfers: // does not implement cpNoRecurse
    case fcParallelFileTransfers:
    case fcTransferOut:
    case fcTransferIn:
      return false;

    case fcChangePassword:
      return FSecureShell->CanChangePassword();

    default:
      DebugFail();
      return false;
  }
}

void TSCPFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.IsEmpty())
  {
    FTerminal->LogEvent(FORMAT("Locating to cached directory \"%s\".",
      FCachedDirectoryChange));
    const UnicodeString Directory = FCachedDirectoryChange;
    FCachedDirectoryChange.Clear();
    try
    {
      ChangeDirectory(Directory);
    }
    catch(...)
    {
      // when location to cached directory fails, pretend again
      // location in cached directory
      // here used to check (CurrentDirectory != Directory), but it is
      // false always (current directory is already set to cached directory),
      // making the condition below useless. check removed.
      if (FTerminal->GetActive())
      {
        FCachedDirectoryChange = Directory;
      }
      throw;
    }
  }
}

void TSCPFileSystem::SendCommand(const UnicodeString & Cmd, bool NoEnsureLocation)
{
  if (!NoEnsureLocation)
  {
    EnsureLocation();
  }

  UnicodeString Line;
  FSecureShell->ClearStdError();
  FReturnCode = 0;
  FOutput->Clear();
  // We suppose, that 'Cmd' already contains command that ensures,
  // that 'LastLine' will be printed
  FTerminal->LogEvent(FORMAT("Raw command: %s", Cmd));
  FSecureShell->SendLine(Cmd);
  FProcessingCommand = true;
}

bool TSCPFileSystem::IsTotalListingLine(const UnicodeString & Line)
{
  // On some hosts there is not "total" but "totalt". What's the reason??
  // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
  return !Line.SubString(1, 5).CompareIC(L"total");
}

bool TSCPFileSystem::RemoveLastLine(UnicodeString & Line,
  int32_t & ReturnCode, const UnicodeString & ALastLine)
{
  UnicodeString LastLine = ALastLine;
  bool IsLastLine = false;
  if (LastLine.IsEmpty())
  {
    LastLine = LAST_LINE;
  }
  // #55: fixed so, even when last line of command output does not
  // contain CR/LF, we can recognize last line
  const int32_t Pos = Line.Pos(LastLine);
  if (Pos)
  {
    // 2003-07-14: There must be nothing after return code number to
    // consider string as last line. This fixes bug with 'set' command
    // in console window
    const UnicodeString ReturnCodeStr = Line.SubString(Pos + LastLine.Length() + 1,
      Line.Length() - Pos + LastLine.Length());
    int64_t Code = 0;
    if (::TryStrToInt64(ReturnCodeStr, Code))
    {
      IsLastLine = true;
      Line.SetLength(Pos - 1);
    }
    ReturnCode = nb::ToInt32(Code);
  }
  return IsLastLine;
}

bool TSCPFileSystem::IsLastLine(UnicodeString & Line)
{
  bool Result = false;
  try
  {
    Result = RemoveLastLine(Line, FReturnCode, FCommandSet->GetLastLine());
  }
  catch(Exception & E)
  {
    FTerminal->TerminalError(&E, LoadStr(CANT_DETECT_RETURN_CODE));
  }
  return Result;
}

void TSCPFileSystem::SkipFirstLine()
{
  const UnicodeString Line = FSecureShell->ReceiveLine();
  if (Line != FCommandSet->GetFirstLine())
  {
    FTerminal->TerminalError(nullptr, FMTLOAD(FIRST_LINE_EXPECTED, Line));
  }
}

void TSCPFileSystem::ReadCommandOutput(int32_t Params, const UnicodeString * Cmd)
{
  try__finally
  {
    if (FLAGSET(Params, coWaitForLastLine))
    {
      // UnicodeString Line;
      bool IsLast = false;
      int32_t Total = 0;
      // #55: fixed so, even when last line of command output does not
      // contain CR/LF, we can recognize last line
      do
      {
        UnicodeString Line = FSecureShell->ReceiveLine();
        IsLast = IsLastLine(Line);
        if (!IsLast || !Line.IsEmpty())
        {
          FOutput->Add(Line);
          if (FLAGSET(Params, coReadProgress))
          {
            Total++;

            if (Total % 10 == 0)
            {
              bool Cancel; //dummy
              FTerminal->DoReadDirectoryProgress(Total, 0, Cancel);
            }
          }
        }
      }
      while (!IsLast);
    }

    if (FLAGSET(Params, coRaiseExcept))
    {
      UnicodeString Message = FSecureShell->GetStdError();
      if (FLAGSET(Params, coExpectNoOutput) && (FOutput->Count > 0))
      {
        AddToList(Message, FOutput->GetText(), L"\n");
      }

      while (!Message.IsEmpty() && (Message.LastDelimiter(L"\n\r") == Message.Length()))
      {
        Message.SetLength(Message.Length() - 1);
      }

      const bool WrongReturnCode =
        (FReturnCode > 1) ||
        ((FReturnCode == 1) && FLAGCLEAR(Params, coIgnoreWarnings));

      if (!FOnCaptureOutput.empty())
      {
        FOnCaptureOutput(::Int64ToStr(GetReturnCode()), cotExitCode);
      }

      if (FLAGSET(Params, coOnlyReturnCode))
      {
        if (WrongReturnCode)
        {
          FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, FReturnCode));
        }
      }
      else
      {
        const bool IsStdErrOnlyError = (FLAGCLEAR(Params, coIgnoreWarnings) && FLAGCLEAR(Params, coIgnoreStdErr));
        const bool WrongOutput = !Message.IsEmpty() && ((FOutput->Count() == 0) || IsStdErrOnlyError);
        if (WrongOutput || WrongReturnCode)
        {
          DebugAssert(Cmd != nullptr);
          if (Message.IsEmpty())
          {
            FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, FReturnCode));
          }
          else
          {
            throw ETerminal(MainInstructions(FMTLOAD(COMMAND_FAILED2, *Cmd, FReturnCode)), Message);
          }
        }
      }
    }
  }
  __finally
  {
    FProcessingCommand = false;
  } end_try__finally
}

void TSCPFileSystem::InvalidOutputError(const UnicodeString & Command)
{
  FTerminal->TerminalError(FMTLOAD(INVALID_OUTPUT_ERROR, Command, Output->Text));
}

void TSCPFileSystem::ExecCommand(TFSCommand Cmd, int32_t Params,
  fmt::ArgList args)
{
  if (Params < 0) { Params = ecDefault; }

  const UnicodeString FullCommand = FCommandSet->FullCommand(Cmd, args);
  const UnicodeString Command = FCommandSet->Command(Cmd, args);

  const TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  SendCommand(FullCommand, FLAGSET(Params, ecNoEnsureLocation));

  const int32_t COParams =
    coWaitForLastLine |
    FLAGMASK(FLAGSET(Params, ecRaiseExcept), coRaiseExcept) |
    FLAGMASK(FLAGSET(Params, ecOnlyReturnCode), coOnlyReturnCode) |
    FLAGMASK(FLAGSET(Params, ecIgnoreWarnings), coIgnoreWarnings) |
    FLAGMASK(FLAGSET(Params, ecReadProgress), coReadProgress) |
    FLAGMASK(FLAGSET(Params, ecIgnoreStdErr), coIgnoreStdErr);

  ReadCommandOutput(COParams, &Command);

  if (Params & ecRaiseExcept)
  {
    const int32_t MinL = FCommandSet->GetMinLines(Cmd);
    const int32_t MaxL = FCommandSet->GetMaxLines(Cmd);
    if (((MinL >= 0) && (MinL > nb::ToInt32(FOutput->GetCount()))) ||
        ((MaxL >= 0) && (MaxL > nb::ToInt32(FOutput->GetCount()))))
    {
      InvalidOutputError(FullCommand);
    }
  }
}

UnicodeString TSCPFileSystem::GetCurrentDirectory() const
{
  return FCurrentDirectory;
}

void TSCPFileSystem::DoStartup()
{
  // Capabilities of SCP protocol are fixed
  FTerminal->SaveCapabilities(FFileSystemInfo);

  const TSessionData * Data = FTerminal->GetSessionData();
  // SkipStartupMessage and DetectReturnVar must succeed,
  // otherwise session is to be closed.
  try
  {
    FTerminal->SetExceptionOnFail(true);
    SkipStartupMessage();
    if (Data->GetDetectReturnVar())
    {
      DetectReturnVar();
    }
    FTerminal->SetExceptionOnFail(false);
  }
  catch(Exception & E)
  {
    FTerminal->FatalError(&E, L"");
  }

  // Needs to be done before UnsetNationalVars()
  DetectUtf();

#define COND_OPER(OPER) if (Data->Get##OPER()) OPER()
  COND_OPER(ClearAliases);
  COND_OPER(UnsetNationalVars);
#undef COND_OPER
}

void TSCPFileSystem::DetectUtf()
{
  const TSessionData * Data = FTerminal->GetSessionData();
  switch (Data->GetNotUtf())
  {
    case asOn:
      FSecureShell->SetUtfStrings(false); // noop
      break;

    case asOff:
      FSecureShell->SetUtfStrings(true);
      break;

    default:
      DebugFail();
    case asAuto:
      FSecureShell->SetUtfStrings(false); // noop
      try
      {
        ExecCommand(fsLang, 0);

        if ((FOutput->GetCount() >= 1) &&
            ::AnsiContainsText(FOutput->GetString(0), L"UTF-8"))
        {
          FSecureShell->SetUtfStrings(true);
        }
      }
      catch(Exception &)
      {
        // ignore non-fatal errors
        if (!FTerminal->GetActive())
        {
          throw;
        }
      }
      break;
  }

  if (FSecureShell->GetUtfStrings())
  {
    FTerminal->LogEvent("We will use UTF-8");
  }
  else
  {
    FTerminal->LogEvent("We will not use UTF-8");
  }
}

void TSCPFileSystem::SkipStartupMessage()
{
  try
  {
    FTerminal->LogEvent("Skipping host startup message (if any).");
    ExecCommand(fsNull, 0);
  }
  catch (Exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(SKIP_STARTUP_MESSAGE_ERROR), 0, HELP_SKIP_STARTUP_MESSAGE_ERROR);
  }
}

void TSCPFileSystem::LookupUsersGroups()
{
  ExecCommand(fsWhoami, 0);
  FTerminal->GetUsers()->Clear();
  if (FOutput->GetCount() > 0)
  {
    const UnicodeString CurrentUser = FOutput->GetString(0);
    FTerminal->GetUsers()->Add(TRemoteToken(CurrentUser));
  }
  ExecCommand(fsLookupUsersGroups, 0);
  FTerminal->GetGroups()->Clear();
  if (FOutput->GetCount() > 0)
  {
    UnicodeString Groups = FOutput->GetString(0);
    while (!Groups.IsEmpty())
    {
      const UnicodeString NewGroup = CutToChar(Groups, L' ', false);
      FTerminal->GetGroups()->Add(TRemoteToken(NewGroup));
      FTerminal->GetMembership()->Add(TRemoteToken(NewGroup));
    }
  }
}

void TSCPFileSystem::DetectReturnVar()
{
  // This suppose that something was already executed (probably SkipStartupMessage())
  // or return code variable is already set on start up.

  try
  {
    // #60 17.10.01: "status" and "?" switched
    const UnicodeString ReturnVars[2] = { "status", "?" };
    UnicodeString NewReturnVar;
    FTerminal->LogEvent("Detecting variable containing return code of last command.");
    for (int32_t Index = 0; Index < 2; ++Index)
    {
      bool Success = true;

      try
      {
        FTerminal->LogEvent(FORMAT("Trying \"$%s\".", ReturnVars[Index]));
        ExecCommand(fsVarValue, 0, ReturnVars[Index]);
        const UnicodeString Str = GetOutput()->GetCount() > 0 ? GetOutput()->GetString(0) : L"";
        const int32_t Val = ::StrToIntDef(Str, 256);
        if ((GetOutput()->GetCount() != 1) || Str.IsEmpty() || (Val > 255))
        {
          FTerminal->LogEvent("The response is not numerical exit code");
          Abort();
        }
      }
      catch(EFatal &)
      {
        // if fatal error occurs, we need to exit ...
        throw;
      }
      catch (Exception &)
      {
        // ...otherwise, we will try next variable (if any)
        Success = false;
      }

      if (Success)
      {
        NewReturnVar = ReturnVars[Index];
        break;
      }
    }

    if (NewReturnVar.IsEmpty())
    {
      ThrowExtException();
    }
    else
    {
      FCommandSet->SetReturnVar(NewReturnVar);
      FTerminal->LogEvent(FORMAT("Return code variable \"%s\" selected.",
        FCommandSet->GetReturnVar()));
    }
  }
  catch(Exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(DETECT_RETURNVAR_ERROR));
  }
}

void TSCPFileSystem::ClearAlias(const UnicodeString & Alias)
{
  if (!Alias.IsEmpty())
  {
    // this command usually fails, because there will never be
    // aliases on all commands -> see last False parameter
    ExecCommand(fsUnalias, 0, Alias);
  }
}

void TSCPFileSystem::ClearAliases()
{
  try
  {
    FTerminal->LogEvent("Clearing all aliases.");
    ClearAlias(TCommandSet::ExtractCommand(FTerminal->GetSessionData()->GetListingCommand()));
    std::unique_ptr<TStrings> CommandList(FCommandSet->CreateCommandList());
    try__finally
    {
      for (int32_t Index = 0; Index < CommandList->GetCount(); ++Index)
      {
        ClearAlias(CommandList->GetString(Index));
      }
    }
    __finally__removed
    {
#if defined(__BORLANDC__)
      delete CommandList;
#endif // defined(__BORLANDC__)
    } end_try__finally
  }
  catch(Exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(UNALIAS_ALL_ERROR));
  }
}

void TSCPFileSystem::UnsetNationalVars()
{
  try
  {
    FTerminal->LogEvent("Clearing national user variables.");
    for (int32_t Index = 0; Index < NationalVarCount; ++Index)
    {
      ExecCommand(fsUnset, 0, UnicodeString(NationalVars[Index]), false);
    }
  }
  catch(Exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(UNSET_NATIONAL_ERROR));
  }
}

void TSCPFileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    ExecCommand(fsCurrentDirectory, 0);
    FCurrentDirectory = base::UnixExcludeTrailingBackslash(FOutput->GetString(0));
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
  }
}

void TSCPFileSystem::HomeDirectory()
{
  ExecCommand(fsHomeDirectory, 0);
}

void TSCPFileSystem::AnnounceFileListOperation()
{
  // noop
}

void TSCPFileSystem::ChangeDirectory(const UnicodeString & ADirectory)
{
  int32_t Params = ecDefault;
  UnicodeString Directory = ADirectory;
  try
  {
    EnsureLocation();
  }
  catch (...)
  {
    if (FTerminal->Active && DebugAlwaysTrue(!FCachedDirectoryChange.IsEmpty()))
    {
      Params |= ecNoEnsureLocation;
      Directory = base::AbsolutePath(FTerminal->AbsolutePath(FCachedDirectoryChange, true), Directory);
      FTerminal->LogEvent(
        FORMAT(L"Cannot locate to cached directory, assuming that target absolute path is \"%s\".", Directory));
    }
    else
    {
      throw;
    }
  }

  UnicodeString ToDir;
  // This effectively disallows entering subdirectories starting with ~ and containing space
  if (!Directory.IsEmpty() &&
      ((Directory[1] != L'~') || (Directory.SubString(1, 2) == L"~ ")))
  {
    ToDir = ShellQuoteStr(Directory);
  }
  else
  {
    ToDir = DelimitStr(Directory);
  }
  ExecCommand(fsChangeDirectory, 0, ToDir, Params);
  FCachedDirectoryChange.Clear();
}

void TSCPFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCachedDirectoryChange = base::UnixExcludeTrailingBackslash(Directory);
}

void TSCPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  DebugAssert(FileList);
  // emptying file list moved before command execution
  FileList->Reset();

  bool Again;

  do
  {
    Again = false;
    try
    {
      const int32_t Params = ecDefault | ecReadProgress |
        FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
      const UnicodeString Options =
        ((FLsFullTime == asAuto) || (FLsFullTime == asOn)) ? FullTimeOption : "";
      const bool ListCurrentDirectory = (FileList->GetDirectory() == FTerminal->GetCurrentDirectory());
      if (ListCurrentDirectory)
      {
        FTerminal->LogEvent("Listing current directory.");
        ExecCommand(fsListCurrentDirectory, Params,
          FTerminal->GetSessionData()->GetListingCommand(), Options);
      }
      else
      {
        FTerminal->LogEvent(FORMAT("Listing directory \"%s\".",
          FileList->GetDirectory()));
        ExecCommand(fsListDirectory, Params,
          FTerminal->GetSessionData()->GetListingCommand(), Options,
            DelimitStr(FileList->GetDirectory())
          );
      }

      // If output is not empty, we have successfully got file listing,
      // otherwise there was an error, in case it was "permission denied"
      // we try to get at least parent directory (see "else" statement below)
      if (FOutput->GetCount() > 0)
      {
        // Copy LS command output, because eventual symlink analysis would
        // modify FTerminal->Output
        std::unique_ptr<TStringList> OutputCopy(std::make_unique<TStringList>());
        try__finally
        {
          OutputCopy->Assign(FOutput.get());

          // delete leading "total xxx" line
          // On some hosts there is not "total" but "totalt". What's the reason??
          // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
          if (IsTotalListingLine(OutputCopy->GetString(0)))
          {
            OutputCopy->Delete(0);
          }

          auto CheckForEsc = FTerminal->GetOnCheckForEsc();
          for (int32_t Index = 0; Index < OutputCopy->GetCount(); ++Index)
          {
            if (CheckForEsc != nullptr && CheckForEsc())
            {
              break;
            }
            UnicodeString OutputLine = OutputCopy->GetString(Index);
            if (!OutputLine.IsEmpty())
            {
              std::unique_ptr<TRemoteFile> File(CreateRemoteFile(OutputCopy->GetString(Index)));
              if (FTerminal->IsValidFile(File.get()))
              {
                FileList->AddFile(File.release());
              }
            }
          }
        }
        __finally__removed
        {
#if defined(__BORLANDC__)
          delete OutputCopy;
#endif // defined(__BORLANDC__)
        } end_try__finally
      }
      else
      {
        bool Empty = false;
        if (ListCurrentDirectory)
        {
          // Empty file list -> probably "permission denied", we
          // at least get link to parent directory ("..")
          TRemoteFile * File = FTerminal->ReadFile(base::UnixCombinePaths(FTerminal->GetFiles()->GetDirectory(), PARENTDIRECTORY));
          Empty = (File == nullptr);
          if (!Empty)
          {
            DebugAssert(File->GetIsParentDirectory());
            FileList->AddFile(File);
          }
        }
        else
        {
          Empty = true;
        }

        if (Empty)
        {
          throw ExtException(
            nullptr, FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory()),
            HELP_EMPTY_DIRECTORY);
        }
      }

      if (FLsFullTime == asAuto)
      {
          FTerminal->LogEvent(
            FORMAT("Directory listing with %s succeed, next time all errors during "
              "directory listing will be displayed immediately.",
              UnicodeString(FullTimeOption)));
          FLsFullTime = asOn;
      }
    }
    catch(Exception & E)
    {
      if (FTerminal->GetActive())
      {
        if (FLsFullTime == asAuto)
        {
          FTerminal->GetLog()->AddException(&E);
          FLsFullTime = asOff;
          Again = true;
          FTerminal->LogEvent(
            FORMAT("Directory listing with %s failed, try again regular listing.",
            UnicodeString(FullTimeOption)));
        }
        else
        {
          throw;
        }
      }
      else
      {
        throw;
      }
    }
  }
  while (Again);
}

void TSCPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& AFile)
{
  CustomReadFile(SymlinkFile->GetLinkTo(), AFile, SymlinkFile);
}

void TSCPFileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  CustomReadFile(AFileName, AFile, nullptr);
}

TRemoteFile * TSCPFileSystem::CreateRemoteFile(
  const UnicodeString & ListingStr, TRemoteFile * LinkedByFile)
{
  std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>(LinkedByFile));
  try__catch
  {
    File->SetTerminal(FTerminal);
    File->SetListingStr(ListingStr);
    File->ShiftTimeInSeconds(TimeToSeconds(FTerminal->GetSessionData()->GetTimeDifference()));
    File->Complete();
  }
  __catch__removed
  {
#if defined(__BORLANDC__)
    delete File;
    throw;
#endif // defined(__BORLANDC__)
  } end_try__catch

  return File.release();
}

void TSCPFileSystem::CustomReadFile(const UnicodeString & AFileName,
  TRemoteFile *& File, TRemoteFile * ALinkedByFile)
{
  File = nullptr;
  const int32_t Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  const UnicodeString Options = (FLsFullTime == asOn) ? FullTimeOption : "";
  const UnicodeString FileName = ALinkedByFile ? ALinkedByFile->GetFullLinkName() : AFileName;
  ExecCommand(fsListFile, Params,
    FTerminal->GetSessionData()->GetListingCommand(), Options, DelimitStr(FileName)
    );
  if (FOutput->GetCount())
  {
    int32_t LineIndex = 0;
    if (IsTotalListingLine(FOutput->GetString(LineIndex)) && FOutput->GetCount() > 1)
    {
      ++LineIndex;
    }

    File = CreateRemoteFile(FOutput->GetString(LineIndex), ALinkedByFile);
  }
}

void TSCPFileSystem::DeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, int32_t Params, TRmSessionAction & Action)
{
  DebugUsedParam(AFile);
  DebugUsedParam(Params);
  Action.Recursive();
  DebugAssert(FLAGCLEAR(Params, dfNoRecursive) || (AFile && AFile->GetIsSymLink()));
  ExecCommand(fsDeleteFile, Params, DelimitStr(AFileName));
}

void TSCPFileSystem::RenameFile(
  const UnicodeString & AFileName, const TRemoteFile * /*AFile*/, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  ExecCommand(fsRenameFile, 0, DelimitStr(AFileName), DelimitStr(ANewName));
}

void TSCPFileSystem::CopyFile(
  const UnicodeString & AFileName, const TRemoteFile * /*AFile*/, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  const UnicodeString DelimitedFileName = DelimitStr(AFileName);
  const UnicodeString DelimitedNewName = DelimitStr(ANewName);
  const UnicodeString AdditionalSwitches = L"-T";
  try
  {
    ExecCommand(fsCopyFile, 0, AdditionalSwitches, DelimitedFileName, DelimitedNewName);
  }
  catch(Exception &)
  {
    if (FTerminal->GetActive())
    {
      // The -T is GNU switch and may not be available on all platforms.
      // https://lists.gnu.org/archive/html/bug-coreutils/2004-07/msg00000.html
      FTerminal->LogEvent(FORMAT("Attempt with %s failed, trying without", AdditionalSwitches));
      ExecCommand(fsCopyFile, 0, L"", DelimitedFileName, DelimitedNewName);
    }
    else
    {
      throw;
    }
  }
}

void TSCPFileSystem::CreateDirectory(const UnicodeString & ADirName, bool /*Encrypt*/)
{
  ExecCommand(fsCreateDirectory, 0, DelimitStr(ADirName));
}

void TSCPFileSystem::CreateLink(const UnicodeString & AFileName,
  const UnicodeString & APointTo, bool Symbolic)
{
  ExecCommand(fsCreateLink, 0,
    Symbolic ? L"-s" : L"", DelimitStr(APointTo), DelimitStr(AFileName));
}

void TSCPFileSystem::ChangeFileToken(const UnicodeString & DelimitedName,
  const TRemoteToken & Token, TFSCommand Cmd, const UnicodeString & RecursiveStr)
{
  UnicodeString Str;
  if (Token.GetIDValid())
  {
    Str = IntToStr(nb::ToInt32(Token.GetID()));
  }
  else if (Token.GetNameValid())
  {
    Str = Token.GetName();
  }

  if (!Str.IsEmpty())
  {
    ExecCommand(Cmd, 0, RecursiveStr, Str, DelimitedName);
  }
}

void TSCPFileSystem::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * AFile, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  DebugAssert(Properties);
  const int32_t Directory = ((AFile == nullptr) ? -1 : (AFile->GetIsDirectory() ? 1 : 0));
  const bool IsDirectory = (Directory > 0);
  const bool Recursive = Properties->Recursive && IsDirectory;
  const UnicodeString RecursiveStr = Recursive ? L"-R" : L"";

  const UnicodeString DelimitedName = DelimitStr(AFileName);
  // change group before permissions as chgrp change permissions
  if (Properties->Valid.Contains(vpGroup))
  {
    ChangeFileToken(DelimitedName, Properties->Group, fsChangeGroup, RecursiveStr);
  }
  if (Properties->Valid.Contains(vpOwner))
  {
    ChangeFileToken(DelimitedName, Properties->Owner, fsChangeOwner, RecursiveStr);
  }
  if (Properties->Valid.Contains(vpRights))
  {
    TRights Rights = Properties->Rights;

    // if we don't set modes recursively, we may add X at once with other
    // options. Otherwise we have to add X after recursive command
    if (!Recursive && IsDirectory && Properties->AddXToDirectories)
      Rights.AddExecute();

    Action.Rights(Rights);
    if (Recursive)
    {
      Action.Recursive();
    }

    if ((Rights.GetNumberSet() | Rights.GetNumberUnset()) != TRights::rfNo)
    {
      ExecCommand(fsChangeMode, 0,
        RecursiveStr, Rights.GetChmodStr(Directory), DelimitedName);
    }

    // if file is directory and we do recursive mode settings with
    // add-x-to-directories option on, add those X
    if (Recursive && IsDirectory && Properties->AddXToDirectories)
    {
      Rights.AddExecute();
      ExecCommand(fsChangeMode, 0,
        L"", Rights.GetChmodStr(Directory), DelimitedName);
    }
  }
  else
  {
    Action.Cancel();
  }
  DebugAssert(!Properties->Valid.Contains(vpLastAccess));
  DebugAssert(!Properties->Valid.Contains(vpModification));
}

bool TSCPFileSystem::LoadFilesProperties(TStrings * /*FileList*/)
{
  DebugFail();
  return false;
}

UnicodeString TSCPFileSystem::CalculateFilesChecksumInitialize(const UnicodeString & Alg)
{
  std::unique_ptr<TStrings> Algs(std::make_unique<TStringList>());
  GetSupportedChecksumAlgs(Algs.get());
  return FindIdent(Alg, Algs.get());
}

UnicodeString TSCPFileSystem::ParseFileChecksum(
  const UnicodeString & Line, const UnicodeString & FileName, const UnicodeString & Command)
{
  const int32_t P = Line.Pos(L" ");
  if ((P < 0) ||
      (P == Line.Length()) ||
      ((Line[P + 1] != L' ') && (Line[P + 1] != L'*')) ||
      (Line.SubString(P + 2, Line.Length() - P - 1) != FileName))
  {
    InvalidOutputError(Command);
  }
  return Line.SubString(1, P - 1);
}

void TSCPFileSystem::ProcessFileChecksum(
  TCalculatedChecksumEvent && OnCalculatedChecksum, TChecksumSessionAction & Action, TFileOperationProgressType * OperationProgress,
  bool FirstLevel, const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Checksum)
{
  const bool Success = !Checksum.IsEmpty();
  if (Success)
  {
    if (!OnCalculatedChecksum.empty())
    {
      OnCalculatedChecksum(base::UnixExtractFileName(FileName), Alg, Checksum);
    }
    Action.Checksum(Alg, Checksum);
  }
  if (FirstLevel)
  {
    TOnceDoneOperation OnceDoneOperation; // not used
    OperationProgress->Finish(FileName, Success, OnceDoneOperation);
  }
}

void TSCPFileSystem::CalculateFilesChecksum(
  const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent && OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  FTerminal->CalculateSubFoldersChecksum(Alg, FileList, std::move(OnCalculatedChecksum), OperationProgress, FirstLevel);

  const TStrings * AlgDefs = FTerminal->GetShellChecksumAlgDefs();
  const int32_t AlgIndex = AlgDefs->IndexOfName(Alg);
  const UnicodeString AlgCommand = (AlgIndex >= 0) ? AlgDefs->GetValueFromIndex(AlgIndex) : Alg;

  int32_t Index = 0;
  while ((Index < FileList->Count) && !OperationProgress->Cancel)
  {
    std::unique_ptr<TStrings> BatchFileList(std::make_unique<TStringList>());
    UnicodeString FileListCommandLine;
    int64_t BatchSize = 0;
    while (Index < FileList->Count)
    {
      const TRemoteFile * File = DebugNotNull(FileList->GetAs<TRemoteFile>(Index));
      if (!File->IsDirectory)
      {
        const UnicodeString FileName = FileList->GetString(Index);
        const UnicodeString FileListCommandLineBak = FileListCommandLine;
        AddToShellFileListCommandLine(FileListCommandLine, FileName);
        BatchSize += File->Size;
        if (!FileListCommandLineBak.IsEmpty() &&
            ((FileListCommandLine.Length() > 2048) ||
             (BatchSize > (1024 * 1024 * 1024))))
        {
          FileListCommandLine = FileListCommandLineBak;
          break;
        }
        else
        {
          BatchFileList->AddObject(FileName, File);
        }
      }
      Index++;
    }

    if (!FileListCommandLine.IsEmpty())
    {
      bool IndividualCommands = false;
      std::unique_ptr<TStrings> BatchChecksums(std::make_unique<TStringList>());
      try
      {
        UnicodeString BatchCommand = FORMAT(L"%s %s", AlgCommand, FileListCommandLine);
        AnyCommand(BatchCommand, nullptr);
        if (Output()->Count() != BatchFileList->Count())
        {
          InvalidOutputError(BatchCommand);
        }

        // First do everything that can throw.
        // Only once everything is checked, distribute the results.
        for (int32_t BatchIndex = 0; BatchIndex < BatchFileList->Count; BatchIndex++)
        {
          UnicodeString FileName = BatchFileList->Strings[BatchIndex];
          UnicodeString Line = Output->Strings[BatchIndex];
          UnicodeString Checksum = ParseFileChecksum(Line, FileName, BatchCommand);
          BatchChecksums->Add(Checksum);
        }
      }
      catch (Exception &)
      {
        if (!FTerminal->Active)
        {
          throw;
        }
        else
        {
          IndividualCommands = true;
        }
      }

      if (!IndividualCommands)
      {
        // None of this should throw
        for (int32_t BatchIndex = 0; BatchIndex < BatchFileList->Count; BatchIndex++)
        {
          const UnicodeString FileName = BatchFileList->GetString(BatchIndex);
          const TRemoteFile * File = DebugNotNull(BatchFileList->GetAs<TRemoteFile>(BatchIndex));
          TChecksumSessionAction Action(FTerminal->ActionLog());
          Action.SetFileName(File->FullFileName);
          OperationProgress->SetFile(FileName);
          UnicodeString Checksum = BatchChecksums->Strings[BatchIndex];
          ProcessFileChecksum(std::move(OnCalculatedChecksum), Action, OperationProgress, FirstLevel, FileName, Alg, Checksum);
        }
      }
      else
      {
        FTerminal->LogEvent(
          L"Batch checksum calculation failed, falling back to calculating checksum individually for each file...");

        for (int32_t BatchIndex = 0; BatchIndex < BatchFileList->Count; BatchIndex++)
        {
          const TRemoteFile * File = DebugNotNull(BatchFileList->GetAs<TRemoteFile>(BatchIndex));
          TChecksumSessionAction Action(FTerminal->ActionLog);
          try
          {
            UnicodeString FileName = BatchFileList->Strings[BatchIndex];
            OperationProgress->SetFile(FileName);
            Action.SetFileName(File->FullFileName);

            UnicodeString Checksum;
            try__finally
            {
              const UnicodeString Command = FORMAT(L"%s %s", AlgCommand, ShellQuoteStr(FileName));
              AnyCommand(Command, nullptr);
              if (Output->Count != 1)
              {
                InvalidOutputError(Command);
              }
              const UnicodeString Line = Output->Strings[0];
              Checksum = ParseFileChecksum(Line, FileName, Command);
            }
            __finally
            {
              ProcessFileChecksum(std::move(OnCalculatedChecksum), Action, OperationProgress, FirstLevel, FileName, Alg, Checksum);
            } end_try__finally
          }
          catch (Exception & E)
          {
            FTerminal->RollbackAction(Action, OperationProgress, &E);

            UnicodeString Error = FMTLOAD(CHECKSUM_ERROR, File->FullFileName);
            FTerminal->CommandError(&E, Error);
            // Abort loop.
            // TODO: retries? resume?
            BatchIndex = BatchFileList->Count;
            Index = FileList->Count;
          }
        }
      }
    }
  }
}

void TSCPFileSystem::CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & ACommand, int32_t AParams,
    TCaptureOutputEvent && OutputEvent)
{
  DebugAssert(AFile);
  const bool Dir = AFile->GetIsDirectory() && FTerminal->CanRecurseToDirectory(AFile);
  if (Dir && (AParams & ccRecursive))
  {
    TCustomCommandParams Params;
    Params.Command = ACommand;
    Params.Params = AParams;
    Params.OutputEvent = OutputEvent;
    FTerminal->ProcessDirectory(AFileName, nb::bind(&TTerminal::CustomCommandOnFile, FTerminal),
      &Params);
  }

  if (!Dir || (AParams & ccApplyToDirectories))
  {
    const TCustomCommandData Data(FTerminal);
    const UnicodeString Cmd = TRemoteCustomCommand(
      Data, FTerminal->GetCurrentDirectory(), AFileName, L"").
      Complete(ACommand, true);

    if (!FTerminal->DoOnCustomCommand(Cmd))
    {
      AnyCommand(Cmd, std::move(OutputEvent));
    }
  }
}

void TSCPFileSystem::CaptureOutput(const UnicodeString & AddedLine, TCaptureOutputType OutputType)
{
  int32_t ReturnCode;
  UnicodeString Line = AddedLine;
  // TSecureShell never uses cotExitCode
  DebugAssert((OutputType == cotOutput) || (OutputType == cotError));
  if ((OutputType == cotError) || DebugAlwaysFalse(OutputType == cotExitCode) ||
      !RemoveLastLine(Line, ReturnCode) ||
      !Line.IsEmpty())
  {
    DebugAssert(!FOnCaptureOutput.empty());
    FOnCaptureOutput(Line, OutputType);
  }
}

void TSCPFileSystem::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent && OutputEvent)
{
  DebugAssert(!FSecureShell->GetOnCaptureOutput());
  if (!OutputEvent.empty())
  {
    FSecureShell->SetOnCaptureOutput(nb::bind(&TSCPFileSystem::CaptureOutput, this));
    FOnCaptureOutput = OutputEvent;
  }

  try__finally
  {
    const int32_t Params =
       ecDefault | ecOnlyReturnCode |
      (FTerminal->SessionData->GetExitCode1IsError() ? ecIgnoreStdErr : ecIgnoreWarnings);

    ExecCommand(fsAnyCommand, Params, Command);
  }
  __finally
  {
    FOnCaptureOutput = nullptr;
    FSecureShell->SetOnCaptureOutput(nullptr);
  } end_try__finally
}

TStrings * TSCPFileSystem::GetFixedPaths() const
{
  return nullptr;
}

void TSCPFileSystem::SpaceAvailable(const UnicodeString & /*APath*/,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  DebugFail();
}

// transfer protocol

uint32_t TSCPFileSystem::ConfirmOverwrite(
  const UnicodeString & ASourceFullFileName, const UnicodeString & ATargetFileName, TOperationSide Side,
  const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress)
{
  const TSuspendFileOperationProgress Suspend(OperationProgress);

  TQueryButtonAlias Aliases[3];
  Aliases[0] = TQueryButtonAlias::CreateAllAsYesToNewerGroupedWithYes();
  Aliases[1] = TQueryButtonAlias::CreateYesToAllGroupedWithYes();
  Aliases[2] = TQueryButtonAlias::CreateNoToAllGroupedWithNo();
  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = Aliases;
  QueryParams.AliasesCount = _countof(Aliases);
  const uint32_t Answer =
    FTerminal->ConfirmFileOverwrite(
      ASourceFullFileName, ATargetFileName, FileParams,
      qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
      &QueryParams, Side, CopyParam, Params, OperationProgress);
  return Answer;
}

void TSCPFileSystem::SCPResponse(bool * GotLastLine)
{
  // Taken from scp.c response() and modified

  uint8_t Resp;
  FSecureShell->Receive(&Resp, 1);

  switch (Resp)
  {
    case 0:     /* ok */
      FTerminal->LogEvent("SCP remote side confirmation (0)");
      return;

    default:
    case 1:     /* error */
    case 2:     /* fatal error */
      // pscp adds 'Resp' to 'Msg', why?
      const UnicodeString Msg = FSecureShell->ReceiveLine();
      UnicodeString Line = UnicodeString(reinterpret_cast<char *>(&Resp), 1) + Msg;
      if (IsLastLine(Line))
      {
        if (GotLastLine != nullptr)
        {
          *GotLastLine = true;
        }

        /* TODO 1 : Show stderror to user? */
        FSecureShell->ClearStdError();

        try
        {
          ReadCommandOutput(coExpectNoOutput | coRaiseExcept | coOnlyReturnCode);
        }
        catch(...)
        {
          // when ReadCommandOutput() fails than remote SCP is terminated already
          if (GotLastLine != nullptr)
          {
            *GotLastLine = true;
          }
          throw;
        }
      }
      else if (Resp == 1)
      {
        // While the OpenSSH scp client distinguishes the 1 for error and 2 for fatal errors,
        // the OpenSSH scp server always sends 1 even for fatal errors. Using the error message to tell
        // which errors are fatal and which are not.
        // This error list is valid for OpenSSH 5.3p1 and 7.2p2
        if (SameText(Msg, L"scp: ambiguous target") ||
            StartsText(L"scp: error: unexpected filename: ", Msg) ||
            StartsText(L"scp: protocol error: ", Msg))
        {
          FTerminal->LogEvent("SCP remote side error (1), fatal error detected from error message");
          Resp = 2;
          FScpFatalError = true;
        }
        else
        {
          FTerminal->LogEvent("SCP remote side error (1)");
        }
      }
      else
      {
        FTerminal->LogEvent("SCP remote side fatal error (2)");
        FScpFatalError = true;
      }

      if (Resp == 1)
      {
        throw EScpFileSkipped(nullptr, Msg);
      }
      else
      {
        throw EScp(nullptr, Msg);
      }
  }
}

void TSCPFileSystem::CopyToRemote(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  // scp.c: source(), toremote()
  DebugAssert(AFilesToCopy && OperationProgress);

  Params &= ~(cpAppend | cpResume);
  // UnicodeString Options = L"";
  const bool CheckExistence = base::UnixSamePath(TargetDir, FTerminal->GetCurrentDirectory()) &&
    (FTerminal->GetFiles() != nullptr) && FTerminal->GetFiles()->GetLoaded();
  bool CopyBatchStarted = false;
  bool Failed = true;
  bool GotLastLine = false;
  TCopyParamType CopyParamCur;
  CopyParamCur.Assign(CopyParam);

  const UnicodeString TargetDirFull = base::UnixIncludeTrailingBackslash(TargetDir);

  const UnicodeString Options = InitOptionsStr(CopyParam);

  FScpFatalError = false;
  SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
    Options, DelimitStr(base::UnixExcludeTrailingBackslash(TargetDir))));
  SkipFirstLine();

  try__finally
  {
    try
    {
      SCPResponse(&GotLastLine);

      // This can happen only if SCP command is not executed and return code is 0
      // It has never happened to me (return code is usually 127)
      if (GotLastLine)
      {
        throw Exception("");
      }
    }
    catch(Exception & E)
    {
      if (GotLastLine && FTerminal->GetActive())
      {
        FTerminal->TerminalError(&E, LoadStr(SCP_INIT_ERROR));
      }
      else
      {
        throw;
      }
    }
    CopyBatchStarted = true;

    for (int32_t IFile = 0; (IFile < AFilesToCopy->GetCount()) &&
      !OperationProgress->GetCancel(); ++IFile)
    {
      UnicodeString FileName = AFilesToCopy->GetString(IFile);
      const TRemoteFile * File1 = AFilesToCopy->GetAs<TRemoteFile>(IFile);
      const UnicodeString RealFileName = File1 ? File1->GetFileName() : FileName;
      bool CanProceed = false;

      UnicodeString FileNameOnly =
        FTerminal->ChangeFileName(
          CopyParam, base::ExtractFileName(RealFileName, false), osLocal, true);

      if (CheckExistence)
      {
        // previously there was assertion on FTerminal->FFiles->Loaded, but it
        // fails for scripting, if 'ls' is not issued before.
        // formally we should call CheckRemoteFile here but as checking is for
        // free here (almost) ...
        const TRemoteFile * File2 = FTerminal->FFiles->FindFile(FileNameOnly);
        if (File2 != nullptr)
        {
          uint32_t Answer;
          if (!CopyParam->GetPreserveRights())
            CopyParamCur.SetRights(*File2->GetRights());
          if (File2->GetIsDirectory())
          {
            UnicodeString Message = MainInstructions(FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly));
            TQueryParams QueryParams(qpNeverAskAgainCheck);

            const TSuspendFileOperationProgress Suspend(OperationProgress);
            Answer = FTerminal->ConfirmFileOverwrite(
              FileName, FileNameOnly, nullptr,
              qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
              &QueryParams, osRemote, &CopyParamCur, Params, OperationProgress, Message);
          }
          else
          {
            int64_t MTime = 0;
            TOverwriteFileParams FileParams;
            FTerminal->OpenLocalFile(FileName, GENERIC_READ, 
              nullptr, nullptr, nullptr, &MTime, nullptr,
              &FileParams.SourceSize);
            FileParams.SourceTimestamp = ::UnixToDateTime(MTime,
              FTerminal->GetSessionData()->GetDSTMode());
            FileParams.DestSize = File2->GetSize();
            FileParams.DestTimestamp = File2->GetModification();
            Answer = ConfirmOverwrite(FileName, FileNameOnly, osRemote,
              &FileParams, &CopyParamCur, Params, OperationProgress);
          }

          switch (Answer)
          {
            case qaYes:
              CanProceed = true;
              break;

            case qaCancel:
              OperationProgress->SetCancelAtLeast(csCancel);
              CanProceed = false;
              break;
            case qaNo:
              CanProceed = false;
              break;

            default:
              DebugFail();
              break;
          }
        }
        else
        {
          CanProceed = true;
        }
      }
      else
      {
        CanProceed = true;
      }

      if (CanProceed)
      {
        if (FTerminal->GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (base::DirectoryExists(ApiPath(::ExtractFilePath(FileName))))
          {
            FTerminal->DirectoryModified(TUnixPath::Join(TargetDir,
              FileNameOnly), true);
          }
        }

        const void * Item = nb::ToPtr(AFilesToCopy->Objects[IFile]);

        try
        {
          SCPSource(FileName, TargetDirFull,
            &CopyParamCur, Params, OperationProgress, 0);
          FTerminal->OperationFinish(OperationProgress, Item, FileName, true, OnceDoneOperation);
        }
        catch(EScpFileSkipped & E)
        {
          TQueryParams QueryParams(qpAllowContinueOnError);

          const TSuspendFileOperationProgress Suspend1(OperationProgress);

          if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName), &E,
            qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
          {
            OperationProgress->SetCancel(csCancel);
          }
          FTerminal->OperationFinish(OperationProgress, Item, FileName, false, OnceDoneOperation);
          if (!FTerminal->HandleException(&E))
          {
            throw;
          }
        }
        catch(ESkipFile & E)
        {
          FTerminal->OperationFinish(OperationProgress, Item, FileName, false, OnceDoneOperation);

          {
            const TSuspendFileOperationProgress Suspend(OperationProgress);
            // If ESkipFile occurs, just log it and continue with next file
            if (!FTerminal->HandleException(&E))
            {
              throw;
            }
          }
        }
        catch (...)
        {
          FTerminal->OperationFinish(OperationProgress, Item, FileName, false, OnceDoneOperation);
          throw;
        }
      }
    }
    Failed = false;
  }
  __finally
  {
    // Tell remote side, that we're done.
    if (FTerminal->GetActive())
    {
      try
      {
        if (!GotLastLine)
        {
          if (CopyBatchStarted && !FScpFatalError)
          {
            // What about case, remote side sends fatal error ???
            // (Not sure, if it causes remote side to terminate scp)
            FSecureShell->SendLine(L"E");
            SCPResponse();
          }
          // TODO 1 : Show stderror to user?
          FSecureShell->ClearStdError();

          ReadCommandOutput(coExpectNoOutput | coWaitForLastLine | coOnlyReturnCode |
            (Failed ? 0 : coRaiseExcept));
        }
      }
      catch(Exception & E)
      {
        // Only log error message (it should always succeed, but
        // some pending error maybe in queue)
        FTerminal->GetLog()->AddException(&E);
      }
    }
  } end_try__finally
}

void TSCPFileSystem::Source(
  TLocalFileHandle & /*AHandle*/, const UnicodeString & /*ATargetDir*/, UnicodeString & /*ADestFileName*/,
  const TCopyParamType * /*CopyParam*/, int32_t /*Params*/,
  TFileOperationProgressType * /*OperationProgress*/, uint32_t /*Flags*/,
  TUploadSessionAction & /*Action*/, bool & /*ChildError*/)
{
  DebugFail();
}

void TSCPFileSystem::SCPSource(const UnicodeString & AFileName,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int32_t AParams,
  TFileOperationProgressType * OperationProgress, int32_t Level)
{
  const UnicodeString FileName = AFileName;
  const UnicodeString DestFileName =
    FTerminal->ChangeFileName(
      CopyParam, base::ExtractFileName(FileName, false), osLocal, Level == 0);

  FTerminal->LogEvent(FORMAT("File: \"%s\"", FileName));

  OperationProgress->SetFile(FileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, nullptr, CopyParam, OperationProgress))
  {
    throw ESkipFile();
  }

  TLocalFileHandle LocalFileHandle;
  FTerminal->OpenLocalFile(FileName, GENERIC_READ, LocalFileHandle);

  OperationProgress->SetFileInProgress();

  if (LocalFileHandle.Directory)
  {
    SCPDirectorySource(FileName, TargetDir, CopyParam, AParams, OperationProgress, Level);
  }
  else
  {
    const UnicodeString AbsoluteFileName = FTerminal->AbsolutePath(TargetDir + DestFileName, false);
    DebugAssert(CheckHandle(LocalFileHandle.Handle));
    DebugAssert(LocalFileHandle.Handle);
    std::unique_ptr<TStream> Stream(std::make_unique<TSafeHandleStream>(static_cast<THandle>(LocalFileHandle.Handle)));

    // File is regular file (not directory)
    FTerminal->LogEvent(FORMAT("Copying \"%s\" to remote directory started.", AFileName));

    OperationProgress->SetLocalSize(LocalFileHandle.Size);

    // Suppose same data size to transfer as to read
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(OperationProgress->GetLocalSize());
    OperationProgress->SetTransferringFile(false);

    if (LocalFileHandle.Size > (512 * 1024 * 1024))
    {
      OperationProgress->SetAsciiTransfer(false);
      FTerminal->LogEvent(FORMAT(L"Binary transfer mode selected as the file is too large (%s) to be uploaded in Ascii mode using SCP protocol.", Int64ToStr(LocalFileHandle.Size)));
    }
    else
    {
      FTerminal->SelectSourceTransferMode(LocalFileHandle, CopyParam);
    }

    TUploadSessionAction Action(FTerminal->GetActionLog());
    Action.SetFileName(ExpandUNCFileName(FileName));
    Action.Destination(AbsoluteFileName);

    TRights Rights = CopyParam->RemoteFileRights(LocalFileHandle.Attrs);

    try
    {
      TValueRestorer<TSecureShellMode> SecureShellModeRestorer(FSecureShell->Mode, ssmUploading);

      // During ASCII transfer we will load whole file to this buffer
      // than convert EOL and send it at once, because before converting EOL
      // we can't know its size
      TFileBuffer AsciiBuf;
      bool ConvertToken = false;
      do
      {
        // Buffer for one block of data
        TFileBuffer BlockBuf;

        // This is crucial, if it fails during file transfer, it's fatal error
        FILE_OPERATION_LOOP_BEGIN
        {
          BlockBuf.LoadStream(Stream.get(), OperationProgress->LocalBlockSize(), true);
        }
        FILE_OPERATION_LOOP_END_EX(
          FMTLOAD(READ_ERROR, FileName),
          FLAGMASK(!OperationProgress->TransferringFile, folAllowSkip));

        OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

        // We do ASCII transfer: convert EOL of current block
        // (we don't convert whole buffer, because it would produce
        // huge memory-transfers while inserting/deleting EOL characters)
        // Then we add current block to file buffer
        if (OperationProgress->GetAsciiTransfer())
        {
          int32_t ConvertParams =
            FLAGMASK(CopyParam->GetRemoveCtrlZ(), cpRemoveCtrlZ) |
            FLAGMASK(CopyParam->GetRemoveBOM(), cpRemoveBOM);
          BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
            FTerminal->GetSessionData()->GetEOLType(),
            ConvertParams, ConvertToken);
          BlockBuf.GetMemory()->Seek(0, TSeekOrigin::soBeginning);
          AsciiBuf.ReadStream(BlockBuf.GetMemory(), BlockBuf.GetSize(), true);
          // We don't need it anymore
          BlockBuf.GetMemory()->Clear();
          // Calculate total size to sent (assume that ratio between
          // size of source and size of EOL-transformed data would remain same)
          // First check if file contains anything (div by zero!)
          if (OperationProgress->GetLocallyUsed())
          {
            int64_t X = OperationProgress->GetLocalSize();
            X *= AsciiBuf.GetSize();
            X /= OperationProgress->GetLocallyUsed();
            OperationProgress->ChangeTransferSize(X);
          }
          else
          {
            OperationProgress->ChangeTransferSize(0);
          }
        }

        // We send file information on first pass during BINARY transfer
        // and on last pass during ASCII transfer
        // BINARY: We succeeded reading first buffer from file, hopefully
        // we will be able to read whole, so we send file info to remote side
        // This is done, because when reading fails we can't interrupt sending
        // (don't know how to tell other side that it failed)
        if (!OperationProgress->GetTransferringFile() &&
            (!OperationProgress->GetAsciiTransfer() || OperationProgress->IsLocallyDone()))
        {
          UnicodeString Buf;

          if (CopyParam->GetPreserveTime())
          {
            // Send last file access and modification time
            // TVarRec don't understand 'unsigned int' -> we use sprintf()
            Buf = FORMAT("T%lu 0 %lu 0", nb::ToUInt32(LocalFileHandle.MTime),
              nb::ToUInt32(LocalFileHandle.ATime));
            FSecureShell->SendLine(Buf);
            SCPResponse();
          }

          // Send file modes (rights), filesize and file name
          // TVarRec don't understand 'unsigned int' -> we use sprintf()
          int64_t sz = OperationProgress->GetAsciiTransfer() ? AsciiBuf.GetSize() : OperationProgress->GetLocalSize();
          Buf = FORMAT("C%s %lld %s",
            Rights.GetOctal(),
            sz,
            DestFileName);
          FSecureShell->SendLine(Buf);
          SCPResponse();
          // Indicate we started transferring file, we need to finish it
          // If not, it's fatal error
          OperationProgress->SetTransferringFile(true);

          // If we're doing ASCII transfer, this is last pass
          // so we send whole file
          /* TODO : We can't send file above 32bit size in ASCII mode! */
          if (OperationProgress->GetAsciiTransfer())
          {
            FTerminal->LogEvent(FORMAT("Sending ASCII data (%u bytes)",
              AsciiBuf.GetSize()));
            // Should be equal, just in case it's rounded (see above)
            OperationProgress->ChangeTransferSize(AsciiBuf.GetSize());
            while (!OperationProgress->IsTransferDoneChecked())
            {
              const size_t BlockSize = nb::ToSizeT(OperationProgress->TransferBlockSize());
              FSecureShell->Send(
                nb::ToUInt8Ptr(AsciiBuf.GetData() + OperationProgress->GetTransferredSize()),
                BlockSize);
              OperationProgress->AddTransferred(nb::ToInt64(BlockSize));
              if (OperationProgress->GetCancel() == csCancelTransfer)
              {
                throw Exception(MainInstructions(LoadStr(USER_TERMINATED)));
              }
            }
          }
        }

        // At end of BINARY transfer pass, send current block
        if (!OperationProgress->GetAsciiTransfer())
        {
          if (!OperationProgress->GetTransferredSize())
          {
            FTerminal->LogEvent(FORMAT("Sending BINARY data (first block, %u bytes)",
              BlockBuf.GetSize()));
          }
          else if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
          {
            FTerminal->LogEvent(FORMAT("Sending BINARY data (%u bytes)",
              BlockBuf.GetSize()));
          }
          FSecureShell->Send(nb::ToUInt8Ptr(BlockBuf.GetData()), nb::ToInt32(BlockBuf.GetSize()));
          OperationProgress->AddTransferred(BlockBuf.GetSize());
        }

        if ((OperationProgress->GetCancel() == csCancelTransfer) ||
            (OperationProgress->GetCancel() == csCancel && !OperationProgress->GetTransferringFile()))
        {
          throw Exception(MainInstructions(LoadStr(USER_TERMINATED)));
        }
      }
      while (!OperationProgress->IsLocallyDone() || !OperationProgress->IsTransferDoneChecked());

      FSecureShell->SendNull();
      try
      {
        SCPResponse();
        // If one of two following exceptions occurs, it means, that remote
        // side already know, that file transfer finished, even if it failed
        // so we don't have to throw EFatal
      }
      catch(EScp &)
      {
        // SCP protocol fatal error
        OperationProgress->SetTransferringFile(false);
        throw;
      }
      catch(EScpFileSkipped &)
      {
        // SCP protocol non-fatal error
        OperationProgress->SetTransferringFile(false);
        throw;
      }

      // We succeeded transferring file, from now we can handle exceptions
      // normally -> no fatal error
      OperationProgress->SetTransferringFile(false);
    }
    catch(Exception & E)
    {
      // EScpFileSkipped is derived from ESkipFile,
      // but it does not indicate file skipped by user here
      if (rtti::isa<EScpFileSkipped>(&E))
      {
        Action.Rollback(&E);
      }
      else
      {
        FTerminal->RollbackAction(Action, OperationProgress, &E);
      }

      // Every exception during file transfer is fatal
      if (OperationProgress->GetTransferringFile())
      {
        FTerminal->FatalError(&E, FMTLOAD(COPY_FATAL, AFileName));
      }
      else
      {
        throw;
      }
    }

    // With SCP we are not able to distinguish reason for failure
    // (upload itself, touch or chmod).
    // So we always report error with upload action and
    // log touch and chmod actions only if upload succeeds.
    if (CopyParam->GetPreserveTime())
    {
      TTouchSessionAction(FTerminal->GetActionLog(), AbsoluteFileName, LocalFileHandle.Modification);
    }
    if (CopyParam->GetPreserveRights())
    {
      TChmodSessionAction(FTerminal->GetActionLog(), AbsoluteFileName,
        Rights);
    }

    FTerminal->LogFileDone(OperationProgress, AbsoluteFileName, Action);
    // Stream is disposed here
  }

  LocalFileHandle.Release();

  FTerminal->UpdateSource(LocalFileHandle, CopyParam, AParams);

  FTerminal->LogEvent(FORMAT("Copying \"%s\" to remote directory finished.", FileName));
}

void TSCPFileSystem::SCPDirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int32_t Params,
  TFileOperationProgressType * OperationProgress, int32_t Level)
{
  DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;

  FTerminal->LogEvent(FORMAT("Entering directory \"%s\".", DirectoryName));

  OperationProgress->SetFile(DirectoryName);
  const UnicodeString DestFileName =
    FTerminal->ChangeFileName(
      CopyParam, base::ExtractFileName(DirectoryName, false), osLocal, Level == 0);

  // Get directory attributes
  FILE_OPERATION_LOOP_BEGIN
  {
    LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DirectoryName));
    if (!CheckAttribute(LocalFileAttrs))
    {
      ::RaiseLastOSError();
    }
  }
  FILE_OPERATION_LOOP_END(FMTLOAD(CANT_GET_ATTRS, DirectoryName));

  const UnicodeString TargetDirFull = base::UnixIncludeTrailingBackslash(TUnixPath::Join(TargetDir, DestFileName));

  // UnicodeString Buf;

  /* TODO 1: maybe send filetime */

  // Send directory modes (rights), filesize and file name
  const UnicodeString Buf = FORMAT("D%s 0 %s",
    CopyParam->RemoteFileRights(LocalFileAttrs).GetOctal(), DestFileName);
  FSecureShell->SendLine(Buf);
  SCPResponse();

  try__finally
  {
    TSearchRecOwned SearchRec;
    bool FindOK = FTerminal->LocalFindFirstLoop(IncludeTrailingBackslash(DirectoryName) + L"*.*", SearchRec);

    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = SearchRec.GetFilePath();
      try
      {
        if (SearchRec.IsRealFile())
        {
          SCPSource(FileName, TargetDirFull, CopyParam, Params, OperationProgress, Level + 1);
        }
      }
      // Previously we caught ESkipFile, making error being displayed
      // even when file was excluded by mask. Now the ESkipFile is special
      // case without error message.
      catch (EScpFileSkipped &E)
      {
        TQueryParams QueryParams(qpAllowContinueOnError);
        const TSuspendFileOperationProgress Suspend(OperationProgress);

        if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName), &E,
              qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
        {
          OperationProgress->SetCancel(csCancel);
        }
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }
      catch(ESkipFile & E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        const TSuspendFileOperationProgress Suspend(OperationProgress);
        if (!FTerminal->HandleException(&E))
        {
          throw;
        }
      }

      FindOK = FTerminal->LocalFindNextLoop(SearchRec);
    }

    SearchRec.Close();

    /* TODO : Delete also read-only directories. */
    /* TODO : Show error message on failure. */
    if (!OperationProgress->GetCancel())
    {
      if (FLAGSET(Params, cpDelete))
      {
        FTerminal->RemoveLocalDirectory(ApiPath(DirectoryName));
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
      {
        FILE_OPERATION_LOOP_BEGIN
        {
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DirectoryName), LocalFileAttrs & ~faArchive) == true);
        }
        FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, DirectoryName));
      }
    }
  }
  __finally
  {
    if (FTerminal->GetActive())
    {
      // Tell remote side, that we're done.
      FTerminal->LogEvent(FORMAT("Leaving directory \"%s\".", DirectoryName));
      FSecureShell->SendLine(L"E");
      SCPResponse();
    }
  } end_try__finally
}

void TSCPFileSystem::CopyToLocal(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  bool CloseSCP = False;
  Params &= ~(cpAppend | cpResume);
  UnicodeString Options = InitOptionsStr(CopyParam);
  if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime())
    Options = L"-p";
  if (FTerminal->GetSessionData()->GetScp1Compatibility())
    Options += L" -1";

  FTerminal->LogEvent(FORMAT("Copying %d files/directories to local directory "
    "\"%s\"", AFilesToCopy->GetCount(), TargetDir));
  FTerminal->LogEvent(0, CopyParam->LogStr);

  try__finally
  {
    for (int32_t IFile = 0; (IFile < AFilesToCopy->GetCount()) &&
      !OperationProgress->GetCancel(); ++IFile)
    {
      UnicodeString FileName = AFilesToCopy->GetString(IFile);
      TRemoteFile * File = AFilesToCopy->GetAs<TRemoteFile>(IFile);
      DebugAssert(File);

      try
      {
        bool Success = true; // Have to be set to True (see ::SCPSink)
        SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
          Options, DelimitStr(FileName)));
        SkipFirstLine();

        // Filename is used for error messaging and excluding files only
        // Send in full path to allow path-based excluding
        const UnicodeString FullFileName = base::UnixExcludeTrailingBackslash(File->GetFullFileName());
        SCPSink(TargetDir, FullFileName, base::UnixExtractFilePath(FullFileName),
          CopyParam, Success, OperationProgress, Params, 0);
        // operation succeeded (no exception), so it's ok that
        // remote side closed SCP, but we continue with next file
        if (OperationProgress->GetCancel() == csRemoteAbort)
        {
          OperationProgress->SetCancel(csContinue);
        }

        // Move operation -> delete file/directory afterwards
        // but only if copying succeeded
        if ((Params & cpDelete) && Success && !OperationProgress->GetCancel())
        {
          try
          {
            FTerminal->SetExceptionOnFail(true);
            try__finally
            {
              FILE_OPERATION_LOOP_BEGIN
              {
                // pass full file name in FileName, in case we are not moving
                // from current directory
                FTerminal->DeleteFile(FileName, File);
              }
              FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_FILE_ERROR, FileName));
            }
            __finally
            {
              FTerminal->SetExceptionOnFail(false);
            } end_try__finally
          }
          catch(EFatal &)
          {
            throw;
          }
          catch (...)
          {
            // If user selects skip (or abort), nothing special actually occurs
            // we just run DoFinished with Success = False, so file won't
            // be deselected in panel (depends on assigned event handler)

            // On csCancel we would later try to close remote SCP, but it
            // is closed already
            if (OperationProgress->GetCancel() == csCancel)
            {
              OperationProgress->SetCancel(csRemoteAbort);
            }
            Success = false;
          }
        }

        FTerminal->OperationFinish(
          OperationProgress, File, FileName, (!OperationProgress->Cancel && Success), OnceDoneOperation);
      }
      catch (...)
      {
        FTerminal->OperationFinish(OperationProgress, File, FileName, false, OnceDoneOperation);
        CloseSCP = (OperationProgress->GetCancel() != csRemoteAbort);
        throw;
      }
    }
  }
  __finally
  {
    // In case that copying doesn't cause fatal error (i.e. connection is
    // still active) but wasn't successful (exception or user termination)
    // we need to ensure, that SCP on remote side is closed
    if (FTerminal->GetActive() && (CloseSCP ||
        (OperationProgress->GetCancel() == csCancel) ||
        (OperationProgress->GetCancel() == csCancelTransfer)))
    {
      // bool LastLineRead;

      // If we get LastLine, it means that remote side 'scp' is already
      // terminated, so we need not to terminate it. There is also
      // possibility that remote side waits for confirmation, so it will hang.
      // This should not happen (hope)
      UnicodeString Line = FSecureShell->ReceiveLine();
      const bool LastLineRead = IsLastLine(Line);
      if (!LastLineRead)
      {
        SCPSendError((OperationProgress->GetCancel() ? L"Terminated by user." : L"Exception"), true);
      }
      // Just in case, remote side already sent some more data (it's probable)
      // but we don't want to raise exception (user asked to terminate, it's not error)
      int32_t ECParams = coOnlyReturnCode;
      if (!LastLineRead)
      {
        ECParams |= coWaitForLastLine;
      }
      ReadCommandOutput(ECParams);
    }
  } end_try__finally
}

void TSCPFileSystem::Sink(
  const UnicodeString & /*AFileName*/, const TRemoteFile * /*AFile*/,
  const UnicodeString & /*ATargetDir*/, UnicodeString & /*ADestFileName*/, int32_t /*Attrs*/,
  const TCopyParamType * /*CopyParam*/, int32_t /*Params*/, TFileOperationProgressType * /*OperationProgress*/,
  uint32_t /*Flags*/, TDownloadSessionAction & /*Action*/)
{
  DebugFail();
}

void TSCPFileSystem::SCPError(const UnicodeString & Message, bool Fatal)
{
  SCPSendError(Message, Fatal);
  throw EScpFileSkipped(nullptr, Message);
}

void TSCPFileSystem::SCPSendError(const UnicodeString & Message, bool Fatal)
{
  const uint8_t ErrorLevel = static_cast<uint8_t>(Fatal ? 2 : 1);
  FTerminal->LogEvent(FORMAT("Sending SCP error (%d) to remote side:",
    nb::ToInt32(ErrorLevel)));
  FSecureShell->Send(&ErrorLevel, 1);
  // We don't send exact error message, because some unspecified
  // characters can terminate remote scp
  FSecureShell->SendLine(FORMAT("scp: error: %s", Message));
}

void TSCPFileSystem::SCPSink(const UnicodeString & TargetDir,
  const UnicodeString & AFileName, const UnicodeString & SourceDir,
  const TCopyParamType * CopyParam, bool & Success,
  TFileOperationProgressType * OperationProgress, int32_t Params,
  int32_t Level)
{
  struct TFileData
  {
    CUSTOM_MEM_ALLOCATION_IMPL
    int32_t SetTime{0};
    TDateTime Modification;
    TRights RemoteRights;
    DWORD LocalFileAttrs{0};
    bool Exists{false};
  } FileData;

  bool SkipConfirmed = false;
  bool Initialized = (Level > 0);

  FileData.SetTime = 0;

  TValueRestorer<TSecureShellMode> SecureShellModeRestorer(FSecureShell->Mode, ssmDownloading);

  FSecureShell->SendNull();

  while (!OperationProgress->GetCancel())
  {
    // See (switch ... case 'T':)
    if (FileData.SetTime)
    {
      FileData.SetTime--;
    }

    // In case of error occurred before control record arrived.
    // We can finally use full path here, as we get current path in FileName param
    // (we used to set the file into OperationProgress->FileName, but it collided
    // with progress outputting, particularly for scripting)
    const UnicodeString FullFileName = AFileName;

    try
    {
      // Receive control record
      UnicodeString Line = FSecureShell->ReceiveLine();

      if (Line.Length() == 0)
      {
        FTerminal->FatalError(nullptr, LoadStr(SCP_EMPTY_LINE));
      }

      if (IsLastLine(Line))
      {
        // Remote side finished copying, so remote SCP was closed
        // and we don't need to terminate it manually, see CopyToLocal()
        OperationProgress->SetCancel(csRemoteAbort);
        /* TODO 1 : Show stderror to user? */
        FSecureShell->ClearStdError();
        try
        {
          // coIgnoreWarnings should allow batch transfer to continue when
          // download of one the files fails (user denies overwriting
          // of target local file, no read permissions...)
          ReadCommandOutput(coExpectNoOutput | coRaiseExcept |
            coOnlyReturnCode | coIgnoreWarnings);
          if (!Initialized)
          {
            throw Exception("");
          }
        }
        catch(Exception & E)
        {
          if (!Initialized && FTerminal->GetActive())
          {
            FTerminal->TerminalError(&E, LoadStr(SCP_INIT_ERROR));
          }
          else
          {
            throw;
          }
        }
        return;
      }
      else
      {
        Initialized = true;

        // First character distinguish type of control record
        wchar_t Ctrl = Line[1];
        Line.Delete(1, 1);
        int64_t MTime{0}, ATime{0};
        switch (Ctrl)
        {
          case 1:
            // Error (already logged by ReceiveLine())
            throw EScpFileSkipped(nullptr, FMTLOAD(REMOTE_ERROR, Line));

          case 2:
            // Fatal error, terminate copying
            FTerminal->TerminalError(Line);
            return; // Unreachable

          case L'E': // Exit
            FSecureShell->SendNull();
            return;

          case L'T':
            // int64_t MTime, ATime;
            if (swscanf(Line.c_str(), L"%I64d %*d %I64d %*d", &MTime, &ATime) == 2)
            {
              FileData.Modification = ::UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());
              FSecureShell->SendNull();
              // File time is only valid until next pass
              FileData.SetTime = 2;
              continue;
            }
            else
            {
              SCPError(LoadStr(SCP_ILLEGAL_TIME_FORMAT), False);
            }

          case L'C':
          case L'D':
            break; // continue pass switch{}

          default:
            FTerminal->FatalError(nullptr, FMTLOAD(SCP_INVALID_CONTROL_RECORD, Ctrl, Line));
        }

        TFileMasks::TParams MaskParams;
        MaskParams.Modification = FileData.Modification;

        // We reach this point only if control record was 'C' or 'D'
        try
        {
          FileData.RemoteRights.SetOctal(CutToChar(Line, L' ', True));
          // do not trim leading spaces of the filename
          const int64_t TSize = ::StrToInt64(CutToChar(Line, L' ', False).TrimRight());
          MaskParams.Size = TSize;
          // Security fix: ensure the file ends up where we asked for it.
          // (accept only filename, not path)
          UnicodeString OnlyFileName = base::UnixExtractFileName(Line);
          if (Line != OnlyFileName)
          {
            FTerminal->LogEvent(FORMAT("Warning: Remote host set a compound pathname '%s'", Line));
          }
          if ((Level == 0) && (OnlyFileName != base::UnixExtractFileName(AFileName)))
          {
            SCPError(LoadStr(UNREQUESTED_FILE), False);
          }

          const UnicodeString FullFileName2 = SourceDir + OnlyFileName;
          OperationProgress->SetFile(FullFileName2);
          OperationProgress->SetTransferSize(TSize);
        }
        catch(Exception & E)
        {
          {
            const TSuspendFileOperationProgress Suspend(OperationProgress);
            FTerminal->GetLog()->AddException(&E);
          }
          SCPError(LoadStr(SCP_ILLEGAL_FILE_DESCRIPTOR), false);
        }

        // last possibility to cancel transfer before it starts
        if (OperationProgress->GetCancel())
        {
          throw ESkipFile(nullptr, MainInstructions(LoadStr(USER_TERMINATED)));
        }

        const bool Dir = (Ctrl == L'D');
        const UnicodeString BaseFileName = FTerminal->GetBaseFileName(FullFileName);
        if (!CopyParam->AllowTransfer(BaseFileName, osRemote, Dir, MaskParams, base::IsUnixHiddenFile(BaseFileName)))
        {
          FTerminal->LogEvent(FORMAT("File \"%s\" excluded from transfer",
            FullFileName));
          SkipConfirmed = true;
          SCPError(L"", false);
        }

        if (CopyParam->SkipTransfer(FullFileName, Dir))
        {
          SkipConfirmed = true;
          SCPError(L"", false);
          OperationProgress->AddSkippedFileSize(MaskParams.Size);
        }

        FTerminal->LogFileDetails(AFileName, FileData.Modification, MaskParams.Size);

        const UnicodeString DestFileNameOnly =
          FTerminal->ChangeFileName(
            CopyParam, OperationProgress->GetFileName(), osRemote,
            Level == 0);
        const UnicodeString DestFileName =
          ::IncludeTrailingBackslash(TargetDir) + DestFileNameOnly;

        FileData.LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFileName));
        // If getting attrs fails, we suppose, that file/folder doesn't exist
        FileData.Exists = (FileData.LocalFileAttrs != INVALID_FILE_ATTRIBUTES);
        if (Dir)
        {
          if (FileData.Exists && !(FileData.LocalFileAttrs & faDirectory))
          {
            SCPError(FMTLOAD(NOT_DIRECTORY_ERROR, DestFileName), false);
          }

          if (!FileData.Exists)
          {
            FILE_OPERATION_LOOP_BEGIN
            {
              THROWOSIFFALSE(::ForceDirectories(ApiPath(DestFileName)));
            }
            FILE_OPERATION_LOOP_END(FMTLOAD(CREATE_DIR_ERROR, DestFileName));
            /* SCP: can we set the timestamp for directories ? */
          }
          const UnicodeString FullFileName2 = SourceDir + OperationProgress->GetFileName();
          SCPSink(DestFileName, FullFileName2, base::UnixIncludeTrailingBackslash(FullFileName2),
            CopyParam, Success, OperationProgress, Params, Level + 1);
          continue;
        }
        else if (Ctrl == L'C')
        {
          TDownloadSessionAction Action(FTerminal->GetActionLog());
          Action.SetFileName(FTerminal->AbsolutePath(FullFileName, true));

          try
          {
            HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
            std::unique_ptr<TStream> FileStream;

            /* TODO 1 : Turn off read-only attr */

            try__finally
            {
              try
              {
                if (base::FileExists(ApiPath(DestFileName)))
                {
                  int64_t TimeStamp = 0;
                  TOverwriteFileParams FileParams;
                  FileParams.SourceSize = OperationProgress->GetTransferSize();
                  FileParams.SourceTimestamp = FileData.Modification;
                  FTerminal->OpenLocalFile(DestFileName, GENERIC_READ,
                    nullptr, nullptr, nullptr, &TimeStamp, nullptr,
                    &FileParams.DestSize);
                  FileParams.DestTimestamp = ::UnixToDateTime(TimeStamp,
                    FTerminal->GetSessionData()->GetDSTMode());

                  const uint32_t Answer =
                    ConfirmOverwrite(OperationProgress->GetFileName(), DestFileNameOnly, osLocal,
                      &FileParams, CopyParam, Params, OperationProgress);

                  switch (Answer)
                  {
                    case qaCancel:
                      OperationProgress->SetCancel(csCancel); // continue on next case
                    // FALLTHROUGH
                      [[fallthrough]];
                    case qaNo:
                      SkipConfirmed = true;
                      ThrowExtException();
                  }
                }

                Action.Destination(DestFileName);

                if (!FTerminal->CreateLocalFile(DestFileName, OperationProgress,
                       &LocalFileHandle, FLAGSET(Params, cpNoConfirmation)))
                {
                  SkipConfirmed = true;
                  ThrowExtException();
                }

                FileStream = std::make_unique<TSafeHandleStream>(LocalFileHandle);
              }
              catch(Exception & E)
              {
                // In this step we can still cancel transfer, so we do it
                SCPError(E.Message, false);
                throw;
              }

              // We succeeded, so we confirm transfer to remote side
              FSecureShell->SendNull();
              // From now we need to finish file transfer, if not it's fatal error
              OperationProgress->SetTransferringFile(true);

              // Suppose same data size to transfer as to write
              // (not true with ASCII transfer)
              OperationProgress->SetLocalSize(OperationProgress->GetTransferSize());

              // Will we use ASCII of BINARY file transfer?
              OperationProgress->SetAsciiTransfer(
                CopyParam->UseAsciiTransfer(BaseFileName, osRemote, MaskParams));
              FTerminal->LogEvent(
                0, UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
                  L" transfer mode selected.");

              try
              {
                // Buffer for one block of data
                TFileBuffer BlockBuf;
                bool ConvertToken = false;

                do
                {
                  BlockBuf.SetSize(OperationProgress->TransferBlockSize());
                  BlockBuf.Reset();

                  FSecureShell->Receive(nb::ToUInt8Ptr(BlockBuf.GetData()), nb::ToSizeT(BlockBuf.GetSize()));
                  OperationProgress->AddTransferred(BlockBuf.GetSize());

                  if (OperationProgress->GetAsciiTransfer())
                  {
                    const int64_t PrevBlockSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetSessionData()->GetEOLType(),
                      FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                    OperationProgress->SetLocalSize(
                      OperationProgress->GetLocalSize() - PrevBlockSize + BlockBuf.GetSize());
                  }

                  // This is crucial, if it fails during file transfer, it's fatal error
                  FILE_OPERATION_LOOP_BEGIN
                  {
                    BlockBuf.WriteToStream(FileStream.get(), nb::ToUInt32(BlockBuf.GetSize()));
                  }
                  FILE_OPERATION_LOOP_END_EX(FMTLOAD(WRITE_ERROR, DestFileName), folNone);

                  OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

                  if (OperationProgress->GetCancel() == csCancelTransfer)
                  {
                    throw Exception(MainInstructions(LoadStr(USER_TERMINATED)));
                  }
                }
                while (!OperationProgress->IsLocallyDone() || !OperationProgress->IsTransferDoneChecked());
              }
              catch(Exception & E)
              {
                // Every exception during file transfer is fatal
                FTerminal->FatalError(&E,
                  FMTLOAD(COPY_FATAL, OperationProgress->GetFileName()));
              }

              OperationProgress->SetTransferringFile(false);

              try
              {
                SCPResponse();
                // If one of following exception occurs, we still need
                // to send confirmation to other side
              }
              catch(EScp &)
              {
                FSecureShell->SendNull();
                throw;
              }
              catch(EScpFileSkipped &)
              {
                FSecureShell->SendNull();
                throw;
              }

              FSecureShell->SendNull();

              if (FileData.SetTime && CopyParam->GetPreserveTime())
              {
                FTerminal->UpdateTargetTime(LocalFileHandle, FileData.Modification, mfFull, FTerminal->GetSessionData()->GetDSTMode());
              }
            }
            __finally
            {
             SAFE_CLOSE_HANDLE(LocalFileHandle);
             FileStream.reset();
            } end_try__finally
          }
          catch(Exception & E)
          {
            if (SkipConfirmed)
            {
              Action.Cancel();
            }
            else
            {
              FTerminal->RollbackAction(Action, OperationProgress, &E);
            }
            throw;
          }

          if (!CheckAttribute(FileData.LocalFileAttrs))
          {
            FileData.LocalFileAttrs = faArchive;
          }
          const DWORD NewAttrs = CopyParam->LocalFileAttrs(FileData.RemoteRights);
          if ((NewAttrs & FileData.LocalFileAttrs) != NewAttrs)
          {
            FILE_OPERATION_LOOP_BEGIN
            {
              THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DestFileName), FileData.LocalFileAttrs | NewAttrs));
            }
            FILE_OPERATION_LOOP_END(FMTLOAD(CANT_SET_ATTRS, DestFileName));
          }

          FTerminal->LogFileDone(OperationProgress, DestFileName, Action);
        }
      }
    }
    catch(EScpFileSkipped & E)
    {
      if (!SkipConfirmed)
      {
        const TSuspendFileOperationProgress Suspend(OperationProgress);
        TQueryParams QueryParams(qpAllowContinueOnError);
        if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FullFileName),
              &E, qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
        {
          OperationProgress->SetCancel(csCancel);
        }
        FTerminal->GetLog()->AddException(&E);
      }
      // this was inside above condition, but then transfer was considered
      // successful, even when for example user refused to overwrite file
      Success = false;
    }
    catch(ESkipFile & E)
    {
      SCPSendError(E.Message, false);
      Success = false;
      if (!FTerminal->HandleException(&E))
      {
        throw;
      }
    }
  }
}

void TSCPFileSystem::GetSupportedChecksumAlgs(TStrings * Algs)
{
  FTerminal->GetShellChecksumAlgs(Algs);
}

void TSCPFileSystem::LockFile(const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TSCPFileSystem::UnlockFile(const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TSCPFileSystem::UpdateFromMain(TCustomFileSystem * /*MainFileSystem*/)
{
  // noop
}

void TSCPFileSystem::ClearCaches()
{
  // noop
}

UnicodeString TSCPFileSystem::InitOptionsStr(const TCopyParamType * CopyParam) const
{
  UnicodeString Options;
  if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime())
  {
    Options = L"-p";
  }
  if (FTerminal->GetSessionData()->GetScp1Compatibility())
  {
    Options += L" -1";
  }

  return Options;
}

