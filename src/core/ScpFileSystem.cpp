#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include <StrUtils.hpp>

#include "ScpFileSystem.h"
#include "Terminal.h"
#include "Interface.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "SecureShell.h"

#include <stdio.h>

const int coRaiseExcept = 1;
const int coExpectNoOutput = 2;
const int coWaitForLastLine = 4;
const int coOnlyReturnCode = 8;
const int coIgnoreWarnings = 16;
const int coReadProgress = 32;

const int ecRaiseExcept = 1;
const int ecIgnoreWarnings = 2;
const int ecReadProgress = 4;
const int ecDefault = ecRaiseExcept;

inline void ThrowFileSkipped(Exception *Exception, UnicodeString Message)
{
  throw EFileSkipped(Exception, Message);
}

inline void ThrowScpEror(Exception *Exception, UnicodeString Message)
{
  throw EScp(Exception, Message);
}

#define MaxShellCommand fsLang
#define ShellCommandCount MaxShellCommand + 1
#define MaxCommandLen 40

struct TCommandType
{
  int MinLines;
  int MaxLines;
  bool ModifiesFiles;
  bool ChangesDirectory;
  bool InteractiveCommand;
  char Command[MaxCommandLen];
};

// Only one character! See TSCPFileSystem::ReadCommandOutput()
#define LastLineSeparator ":"
#define LAST_LINE "NetBox: this is end-of-file"
#define FIRST_LINE "NetBox: this is begin-of-file"
extern const TCommandType DefaultCommandSet[];

#define NationalVarCount 10
extern const char NationalVars[NationalVarCount][15];

#define CHECK_CMD DebugAssert((Cmd >=0) && (Cmd <= MaxShellCommand))

class TSessionData;

class TCommandSet : public TObject
{
  NB_DISABLE_COPY(TCommandSet)
private:
  TCommandType CommandSet[ShellCommandCount];
  TSessionData *FSessionData;
  UnicodeString FReturnVar;

public:
  int GetMaxLines(TFSCommand Cmd) const;
  int GetMinLines(TFSCommand Cmd) const;
  bool GetModifiesFiles(TFSCommand Cmd) const;
  bool GetChangesDirectory(TFSCommand Cmd) const;
  bool GetOneLineCommand(TFSCommand Cmd) const;
  void SetCommands(TFSCommand Cmd, UnicodeString Value);
  UnicodeString GetCommands(TFSCommand Cmd) const;
  UnicodeString GetFirstLine() const;
  bool GetInteractiveCommand(TFSCommand Cmd) const;
  UnicodeString GetLastLine() const;
  UnicodeString GetReturnVar() const;

public:
  explicit TCommandSet(TSessionData *ASessionData);
  void Default();
  void CopyFrom(TCommandSet *Source);
  UnicodeString Command(TFSCommand Cmd, fmt::ArgList args);
  FMT_VARIADIC_W(UnicodeString, Command, TFSCommand)

  TStrings *CreateCommandList() const;
  UnicodeString FullCommand(TFSCommand Cmd, fmt::ArgList args);
  FMT_VARIADIC_W(UnicodeString, FullCommand, TFSCommand)

  static UnicodeString ExtractCommand(UnicodeString ACommand);
#if 0
  __property int MaxLines[TFSCommand Cmd]  = { read=GetMaxLines};
  __property int MinLines[TFSCommand Cmd]  = { read=GetMinLines };
  __property bool ModifiesFiles[TFSCommand Cmd]  = { read=GetModifiesFiles };
  __property bool ChangesDirectory[TFSCommand Cmd]  = { read=GetChangesDirectory };
  __property bool OneLineCommand[TFSCommand Cmd]  = { read=GetOneLineCommand };
  __property UnicodeString Commands[TFSCommand Cmd]  = { read=GetCommands, write=SetCommands };
  __property UnicodeString FirstLine = { read = GetFirstLine };
  __property bool InteractiveCommand[TFSCommand Cmd] = { read = GetInteractiveCommand };
  __property UnicodeString LastLine  = { read=GetLastLine };
  __property TSessionData * SessionData  = { read=FSessionData, write=FSessionData };
  __property UnicodeString ReturnVar  = { read=GetReturnVar, write=FReturnVar };
#endif // #if 0
  TSessionData *GetSessionData() const { return FSessionData; }
  void SetSessionData(TSessionData *Value) { FSessionData = Value; }
  void SetReturnVar(UnicodeString Value) { FReturnVar = Value; }
};

const char NationalVars[NationalVarCount][15] =
{
  "LANG", "LANGUAGE", "LC_CTYPE", "LC_COLLATE", "LC_MONETARY", "LC_NUMERIC",
  "LC_TIME", "LC_MESSAGES", "LC_ALL", "HUMAN_BLOCKS"
};
const char FullTimeOption[] = "--full-time";

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
  /*LookupUserGroups*/    {  0,  1, F, F, F, "groups" },
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

TCommandSet::TCommandSet(TSessionData *ASessionData) :
  FSessionData(ASessionData),
  FReturnVar(L"")
{
  DebugAssert(FSessionData);
  Default();
}

void TCommandSet::CopyFrom(TCommandSet *Source)
{
  memmove(&CommandSet, Source->CommandSet, sizeof(CommandSet));
}

void TCommandSet::Default()
{
  DebugAssert(sizeof(CommandSet) == sizeof(DefaultCommandSet));
  memmove(&CommandSet, &DefaultCommandSet, sizeof(CommandSet));
}

int TCommandSet::GetMaxLines(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].MaxLines;
}

int TCommandSet::GetMinLines(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].MinLines;
}

bool TCommandSet::GetModifiesFiles(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].ModifiesFiles;
}

bool TCommandSet::GetChangesDirectory(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].ChangesDirectory;
}

bool TCommandSet::GetInteractiveCommand(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].InteractiveCommand;
}

bool TCommandSet::GetOneLineCommand(TFSCommand /*Cmd*/) const
{
  //CHECK_CMD;
  // #56: we send "echo last line" from all commands on same line
  // just as it was in 1.0
  return True; //CommandSet[Cmd].OneLineCommand;
}

void TCommandSet::SetCommands(TFSCommand Cmd, UnicodeString Value)
{
  CHECK_CMD;
  AnsiString AnsiValue(Value);
  strcpy_s(const_cast<char *>(CommandSet[Cmd].Command), MaxCommandLen, AnsiValue.SubString(1, MaxCommandLen - 1).c_str());
}

UnicodeString TCommandSet::GetCommands(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].Command;
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
  UnicodeString Line = Command(Cmd, args);
  UnicodeString LastLineCmd =
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
    return UnicodeString(L'$') + FReturnVar;
  }
  if (GetSessionData()->GetDetectReturnVar())
  {
    return L'0';
  }
  return UnicodeString(L'$') + GetSessionData()->GetReturnVar();
}

UnicodeString TCommandSet::ExtractCommand(UnicodeString ACommand)
{
  UnicodeString Command = ACommand;
  intptr_t P = Command.Pos(L" ");
  if (P > 0)
  {
    Command.SetLength(P - 1);
  }
  return Command;
}

TStrings *TCommandSet::CreateCommandList() const
{
  TStrings *CommandList = new TStringList();
  for (intptr_t Index = 0; Index < ShellCommandCount; ++Index)
  {
    UnicodeString Cmd = GetCommands(static_cast<TFSCommand>(Index));
    if (!Cmd.IsEmpty())
    {
      Cmd = ExtractCommand(Cmd);
      if ((Cmd != L"%s") && (CommandList->IndexOf(Cmd) < 0))
        CommandList->Add(Cmd);
    }
  }
  return CommandList;
}

//===========================================================================
TSCPFileSystem::TSCPFileSystem(TTerminal *ATerminal) :
  TCustomFileSystem(OBJECT_CLASS_TSCPFileSystem, ATerminal),
  FSecureShell(nullptr),
  FCommandSet(nullptr),
  FOutput(nullptr),
  FReturnCode(0),
  FProcessingCommand(false),
  FLsFullTime(asAuto),
  FOnCaptureOutput(nullptr),
  FScpFatalError(false)
{
}

void TSCPFileSystem::Init(void *Data)
{
  FSecureShell = get_as<TSecureShell>(Data);
  DebugAssert(FSecureShell);
  FCommandSet = new TCommandSet(FTerminal->GetSessionData());
  FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
  FScpFatalError = false;
  FOutput = new TStringList();
  FProcessingCommand = false;
  FOnCaptureOutput = nullptr;

  FFileSystemInfo.ProtocolBaseName = L"SCP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
}

TSCPFileSystem::~TSCPFileSystem()
{
  SAFE_DESTROY(FCommandSet);
  SAFE_DESTROY(FOutput);
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

const TSessionInfo &TSCPFileSystem::GetSessionInfo() const
{
  return FSecureShell->GetSessionInfo();
}

const TFileSystemInfo &TSCPFileSystem::GetFileSystemInfo(bool Retrieve)
{
  if (FFileSystemInfo.AdditionalInfo.IsEmpty() && Retrieve)
  {
    UnicodeString UName;
    FTerminal->SetExceptionOnFail(true);
    try__finally
    {
      SCOPE_EXIT
      {
        FTerminal->SetExceptionOnFail(false);
      };
      try
      {
        AnyCommand(L"uname -a", nullptr);
        for (intptr_t Index = 0; Index < GetOutput()->GetCount(); ++Index)
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
#if 0
      FTerminal->ExceptionOnFail = false;
#endif // #if 0
    };

    FFileSystemInfo.RemoteSystem = UName;
  }

  return FFileSystemInfo;
}

bool TSCPFileSystem::TemporaryTransferFile(UnicodeString /*AFileName*/)
{
  return false;
}

bool TSCPFileSystem::GetStoredCredentialsTried() const
{
  return FSecureShell->GetStoredCredentialsTried();
}

UnicodeString TSCPFileSystem::RemoteGetUserName() const
{
  return FSecureShell->ShellGetUserName();
}

void TSCPFileSystem::Idle()
{
  // Keep session alive
  const TSessionData *Data = FTerminal->GetSessionData();
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

UnicodeString TSCPFileSystem::GetAbsolutePath(UnicodeString APath, bool Local)
{
  return static_cast<const TSCPFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TSCPFileSystem::GetAbsolutePath(UnicodeString APath, bool /*Local*/) const
{
  return base::AbsolutePath(RemoteGetCurrentDirectory(), APath);
}

bool TSCPFileSystem::IsCapable(intptr_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
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
    return true;

  case fcTextMode:
    return FTerminal->GetSessionData()->GetEOLType() != FTerminal->GetConfiguration()->GetLocalEOLType();

  case fcNativeTextMode:
  case fcNewerOnlyUpload:
  case fcTimestampChanging:
  case fcLoadingAdditionalProperties:
  case fcCheckingSpaceAvailable:
  case fcIgnorePermErrors:
  case fcCalculatingChecksum:
  case fcSecondaryShell: // has fcShellAnyCommand
  case fcGroupOwnerChangingByID: // by name
  case fcMoveToQueue:
  case fcLocking:
  case fcPreservingTimestampDirs:
  case fcResumeSupport:
  case fsSkipTransfer:
  case fsParallelTransfers: // does not implement cpNoRecurse
    return false;

  case fcChangePassword:
    return FSecureShell->CanChangePassword();

  default:
    DebugFail();
    return false;
  }
}

UnicodeString TSCPFileSystem::DelimitStr(UnicodeString AStr)
{
  UnicodeString Str = AStr;
  if (!Str.IsEmpty())
  {
    Str = ::DelimitStr(Str, L"\\`$\"");
    if (Str[1] == L'-')
      Str = L"./" + Str;
  }
  return Str;
}

void TSCPFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.IsEmpty())
  {
    FTerminal->LogEvent(FORMAT("Locating to cached directory \"%s\".",
        FCachedDirectoryChange));
    UnicodeString Directory = FCachedDirectoryChange;
    FCachedDirectoryChange.Clear();
    try
    {
      ChangeDirectory(Directory);
    }
    catch (...)
    {
      // when location to cached directory fails, pretend again
      // location in cached directory
      // here used to be check (CurrentDirectory != Directory), but it is
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

void TSCPFileSystem::SendCommand(UnicodeString Cmd)
{
  EnsureLocation();

  UnicodeString Line;
  FSecureShell->ClearStdError();
  FReturnCode = 0;
  FOutput->Clear();
  // We suppose, that 'Cmd' already contains command that ensures,
  // that 'LastLine' will be printed
  FSecureShell->SendLine(Cmd);
  FProcessingCommand = true;
}

bool TSCPFileSystem::IsTotalListingLine(UnicodeString Line)
{
  // On some hosts there is not "total" but "totalt". What's the reason??
  // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
  return !Line.SubString(1, 5).CompareIC(L"total");
}

bool TSCPFileSystem::RemoveLastLine(UnicodeString &Line,
  intptr_t &ReturnCode, UnicodeString ALastLine)
{
  UnicodeString LastLine = ALastLine;
  bool IsLastLine = false;
  if (LastLine.IsEmpty())
  {
    LastLine = LAST_LINE;
  }
  // #55: fixed so, even when last line of command output does not
  // contain CR/LF, we can recognize last line
  intptr_t Pos = Line.Pos(LastLine);
  if (Pos)
  {
    // 2003-07-14: There must be nothing after return code number to
    // consider string as last line. This fixes bug with 'set' command
    // in console window
    UnicodeString ReturnCodeStr = Line.SubString(Pos + LastLine.Length() + 1,
        Line.Length() - Pos + LastLine.Length());
    int64_t Code = 0;
    if (::TryStrToInt64(ReturnCodeStr, Code))
    {
      IsLastLine = true;
      Line.SetLength(Pos - 1);
    }
    ReturnCode = static_cast<intptr_t>(Code);
  }
  return IsLastLine;
}

bool TSCPFileSystem::IsLastLine(UnicodeString &Line)
{
  bool Result = false;
  try
  {
    Result = RemoveLastLine(Line, FReturnCode, FCommandSet->GetLastLine());
  }
  catch (Exception &E)
  {
    FTerminal->TerminalError(&E, LoadStr(CANT_DETECT_RETURN_CODE));
  }
  return Result;
}

void TSCPFileSystem::SkipFirstLine()
{
  UnicodeString Line = FSecureShell->ReceiveLine();
  if (Line != FCommandSet->GetFirstLine())
  {
    FTerminal->TerminalError(nullptr, FMTLOAD(FIRST_LINE_EXPECTED, Line));
  }
}

void TSCPFileSystem::ReadCommandOutput(intptr_t Params, const UnicodeString *Cmd)
{
  try__finally
  {
    SCOPE_EXIT
    {
      FProcessingCommand = false;
    };
    if (Params & coWaitForLastLine)
    {
      bool IsLast;
      intptr_t Total = 0;
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
    if (Params & coRaiseExcept)
    {
      UnicodeString Message = FSecureShell->GetStdError();
      if ((Params & coExpectNoOutput) && FOutput->GetCount())
      {
        if (!Message.IsEmpty())
        {
          Message += L"\n";
        }
        Message += FOutput->GetText();
      }
      while (!Message.IsEmpty() && (Message.LastDelimiter(L"\n\r") == Message.Length()))
      {
        Message.SetLength(Message.Length() - 1);
      }

      bool WrongReturnCode =
        (GetReturnCode() > 1) || (GetReturnCode() == 1 && !(Params & coIgnoreWarnings));

      if (FOnCaptureOutput != nullptr)
      {
        FOnCaptureOutput(::Int64ToStr(GetReturnCode()), cotExitCode);
      }

      if ((Params & coOnlyReturnCode) && WrongReturnCode)
      {
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, GetReturnCode()));
      }
      else if (!(Params & coOnlyReturnCode) &&
        ((!Message.IsEmpty() && ((FOutput->GetCount() == 0) || !(Params & coIgnoreWarnings))) ||
          WrongReturnCode))
      {
        DebugAssert(Cmd != nullptr);
        UnicodeString Str = Cmd ? *Cmd : UnicodeString();
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED, Str, GetReturnCode(), Message));
      }
    }
  }
  __finally
  {
#if 0
    FProcessingCommand = false;
#endif // #if 0
  };
}

void TSCPFileSystem::ExecCommand(UnicodeString Cmd, intptr_t Params,
  UnicodeString CmdString)
{
  if (Params < 0)
  {
    Params = ecDefault;
  }

  TOperationVisualizer Visualizer(FTerminal->GetUseBusyCursor());

  SendCommand(Cmd);

  intptr_t COParams = coWaitForLastLine;
  if (Params & ecRaiseExcept)
  {
    COParams |= coRaiseExcept;
  }
  if (Params & ecIgnoreWarnings)
  {
    COParams |= coIgnoreWarnings;
  }
  if (Params & ecReadProgress)
  {
    COParams |= coReadProgress;
  }
  ReadCommandOutput(COParams, &CmdString);
}

#if defined(__BORLANDC__)
void TSCPFileSystem::ExecCommand(TFSCommand Cmd, const TVarRec *args,
  int size, intptr_t Params)
{
  if (Params < 0)
    Params = ecDefault;
  UnicodeString FullCommand = FCommandSet->FullCommand(Cmd, args, size);
  UnicodeString Command = FCommandSet->Command(Cmd, args, size);
  ExecCommand(FullCommand, Params, Command);
  if (Params & ecRaiseExcept)
  {
    Integer MinL = FCommandSet->GetMinLines(Cmd);
    Integer MaxL = FCommandSet->GetMaxLines(Cmd);
    if (((MinL >= 0) && (MinL > FOutput->GetCount())) ||
      ((MaxL >= 0) && (MaxL > FOutput->GetCount())))
    {
      FTerminal->TerminalError(FMTLOAD(INVALID_OUTPUT_ERROR,
          ARRAYOFCONST((FullCommand, GetOutput()->GetText()))));
    }
  }
}
#endif // if defined(__BORLANDC__)

void TSCPFileSystem::ExecCommand(TFSCommand Cmd, intptr_t Params, fmt::ArgList args)
{
  UnicodeString FullCommand = FCommandSet->FullCommand(Cmd, args);
  UnicodeString Command = FCommandSet->Command(Cmd, args);
  ExecCommand(FullCommand, Params, Command);
  if (Params & ecRaiseExcept)
  {
    int MinL = FCommandSet->GetMinLines(Cmd);
    int MaxL = FCommandSet->GetMaxLines(Cmd);
    if (((MinL >= 0) && (MinL > ToInt(FOutput->GetCount()))) ||
      ((MaxL >= 0) && (MaxL > ToInt(FOutput->GetCount()))))
    {
      FTerminal->TerminalError(FMTLOAD(INVALID_OUTPUT_ERROR,
          FullCommand, GetOutput()->GetText()));
    }
  }
}

UnicodeString TSCPFileSystem::RemoteGetCurrentDirectory() const
{
  return FCurrentDirectory;
}

void TSCPFileSystem::DoStartup()
{
  // Capabilities of SCP protocol are fixed
  FTerminal->SaveCapabilities(FFileSystemInfo);

  const TSessionData *Data = FTerminal->GetSessionData();
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
  catch (Exception &E)
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
  const TSessionData *Data = FTerminal->GetSessionData();
  switch (Data->GetNotUtf())
  {
  case asOn:
    FSecureShell->SetUtfStrings(false); // noop
    break;

  case asOff:
    FSecureShell->SetUtfStrings(true);
    break;

  case asAuto:
    {
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
      catch (Exception &)
      {
        // ignore non-fatal errors
        if (!FTerminal->GetActive())
        {
          throw;
        }
      }
    }
    break;

  default:
    DebugFail();
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
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(SKIP_STARTUP_MESSAGE_ERROR), 0, HELP_SKIP_STARTUP_MESSAGE_ERROR);
  }
}

void TSCPFileSystem::LookupUsersGroups()
{
  ExecCommand(fsLookupUsersGroups, 0);
  FTerminal->GetUsers()->Clear();
  FTerminal->GetGroups()->Clear();
  if (FOutput->GetCount() > 0)
  {
    UnicodeString Groups = FOutput->GetString(0);
    while (!Groups.IsEmpty())
    {
      UnicodeString NewGroup = CutToChar(Groups, L' ', false);
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
    UnicodeString ReturnVars[2] = { L"status", L"?" };
    UnicodeString NewReturnVar;
    FTerminal->LogEvent("Detecting variable containing return code of last command.");
    for (intptr_t Index = 0; Index < 2; ++Index)
    {
      bool Success = true;

      try
      {
        FTerminal->LogEvent(FORMAT("Trying \"$%s\".", ReturnVars[Index]));
        ExecCommand(fsVarValue, 0, ReturnVars[Index]);
        UnicodeString Str = GetOutput()->GetCount() > 0 ? GetOutput()->GetString(0) : L"";
        intptr_t Val = ::StrToIntDef(Str, 256);
        if ((GetOutput()->GetCount() != 1) || Str.IsEmpty() || (Val > 255))
        {
          FTerminal->LogEvent("The response is not numerical exit code");
          Abort();
        }
      }
      catch (EFatal &)
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
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(DETECT_RETURNVAR_ERROR));
  }
}

void TSCPFileSystem::ClearAlias(UnicodeString Alias)
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
      for (intptr_t Index = 0; Index < CommandList->GetCount(); ++Index)
      {
        ClearAlias(CommandList->GetString(Index));
      }
    }
    __finally
    {
#if 0
      delete CommandList;
#endif // #if 0
    };
  }
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNALIAS_ALL_ERROR));
  }
}

void TSCPFileSystem::UnsetNationalVars()
{
  try
  {
    FTerminal->LogEvent("Clearing national user variables.");
    for (intptr_t Index = 0; Index < NationalVarCount; ++Index)
    {
      ExecCommand(fsUnset, 0, UnicodeString(NationalVars[Index]));
    }
  }
  catch (Exception &E)
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

void TSCPFileSystem::ChangeDirectory(UnicodeString Directory)
{
  UnicodeString ToDir;
  if (!Directory.IsEmpty() &&
    ((Directory[1] != L'~') || (Directory.SubString(1, 2) == L"~ ")))
  {
    ToDir = L"\"" + DelimitStr(Directory) + L"\"";
  }
  else
  {
    ToDir = DelimitStr(Directory);
  }
  ExecCommand(fsChangeDirectory, 0, ToDir);
  FCachedDirectoryChange.Clear();
}

void TSCPFileSystem::CachedChangeDirectory(UnicodeString Directory)
{
  FCachedDirectoryChange = base::UnixExcludeTrailingBackslash(Directory);
}

void TSCPFileSystem::ReadDirectory(TRemoteFileList *FileList)
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
      intptr_t Params = ecDefault | ecReadProgress |
        FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
      UnicodeString Options =
        ((FLsFullTime == asAuto) || (FLsFullTime == asOn)) ? FullTimeOption : "";
      bool ListCurrentDirectory = (FileList->GetDirectory() == FTerminal->RemoteGetCurrentDirectory());
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
          DelimitStr(FileList->GetDirectory()));
      }

      // If output is not empty, we have successfully got file listing,
      // otherwise there was an error, in case it was "permission denied"
      // we try to get at least parent directory (see "else" statement below)
      if (FOutput->GetCount() > 0)
      {
        // Copy LS command output, because eventual symlink analysis would
        // modify FTerminal->Output
        std::unique_ptr<TStringList> OutputCopy(new TStringList());
        try__finally
        {
          OutputCopy->Assign(FOutput);

          // delete leading "total xxx" line
          // On some hosts there is not "total" but "totalt". What's the reason??
          // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
          if (IsTotalListingLine(OutputCopy->GetString(0)))
          {
            OutputCopy->Delete(0);
          }

          for (intptr_t Index = 0; Index < OutputCopy->GetCount(); ++Index)
          {
            UnicodeString OutputLine = OutputCopy->GetString(Index);
            if (!OutputLine.IsEmpty())
            {
              TRemoteFile *File = CreateRemoteFile(OutputLine);
              FileList->AddFile(File);
            }
          }
        }
        __finally
        {
#if 0
          delete OutputCopy;
#endif // #if 0
        };
      }
      else
      {
        bool Empty = true;
        if (ListCurrentDirectory)
        {
          TRemoteFile *File = nullptr;
          // Empty file list -> probably "permission denied", we
          // at least get link to parent directory ("..")
          FTerminal->ReadFile(
            base::UnixIncludeTrailingBackslash(FTerminal->GetFiles()->GetDirectory()) +
            PARENTDIRECTORY, File);
          Empty = (File == nullptr);
          if (!Empty)
          {
            DebugAssert(File->GetIsParentDirectory());
            FileList->AddFile(File);
          }
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
    catch (Exception &E)
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

void TSCPFileSystem::ReadSymlink(TRemoteFile *SymlinkFile,
  TRemoteFile *&File)
{
  CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
}

void TSCPFileSystem::ReadFile(UnicodeString AFileName,
  TRemoteFile *&File)
{
  CustomReadFile(AFileName, File, nullptr);
}

TRemoteFile *TSCPFileSystem::CreateRemoteFile(
  UnicodeString ListingStr, TRemoteFile *LinkedByFile)
{
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(LinkedByFile));
  try__catch
  {
    File->SetTerminal(FTerminal);
    File->SetListingStr(ListingStr);
    File->ShiftTimeInSeconds(TimeToSeconds(FTerminal->GetSessionData()->GetTimeDifference()));
    File->Complete();
  }
#if 0
  catch (...)
  {
    delete File;
    throw;
  }
#endif // #if 0

  return File.release();
}

void TSCPFileSystem::CustomReadFile(UnicodeString AFileName,
  TRemoteFile *&File, TRemoteFile *ALinkedByFile)
{
  File = nullptr;
  intptr_t Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  UnicodeString Options = (FLsFullTime == asOn) ? FullTimeOption : "";
  ExecCommand(fsListFile, Params,
    FTerminal->GetSessionData()->GetListingCommand(), Options, DelimitStr(AFileName));
  if (FOutput->GetCount())
  {
    intptr_t LineIndex = 0;
    if (IsTotalListingLine(FOutput->GetString(LineIndex)) && FOutput->GetCount() > 1)
    {
      ++LineIndex;
    }

    File = CreateRemoteFile(FOutput->GetString(LineIndex), ALinkedByFile);
  }
}

void TSCPFileSystem::RemoteDeleteFile(UnicodeString AFileName,
  const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action)
{
  DebugUsedParam(AFile);
  DebugUsedParam(Params);
  Action.Recursive();
  DebugAssert(FLAGCLEAR(Params, dfNoRecursive) || (AFile && AFile->GetIsSymLink()));
  ExecCommand(fsDeleteFile, Params, DelimitStr(AFileName));
}

void TSCPFileSystem::RemoteRenameFile(UnicodeString AFileName,
  UnicodeString ANewName)
{
  ExecCommand(fsRenameFile, 0, DelimitStr(AFileName), DelimitStr(ANewName));
}

void TSCPFileSystem::RemoteCopyFile(UnicodeString AFileName,
  UnicodeString ANewName)
{
  UnicodeString DelimitedFileName = DelimitStr(AFileName);
  UnicodeString DelimitedNewName = DelimitStr(ANewName);
  const UnicodeString AdditionalSwitches = L"-T";
  try
  {
    ExecCommand(fsCopyFile, 0, AdditionalSwitches, DelimitedFileName, DelimitedNewName);
  }
  catch (Exception &)
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

void TSCPFileSystem::RemoteCreateDirectory(UnicodeString ADirName)
{
  ExecCommand(fsCreateDirectory, 0, DelimitStr(ADirName));
}

void TSCPFileSystem::CreateLink(UnicodeString AFileName,
  UnicodeString PointTo, bool Symbolic)
{
  ExecCommand(fsCreateLink, 0,
    Symbolic ? L"-s" : L"", DelimitStr(PointTo), DelimitStr(AFileName));
}

void TSCPFileSystem::ChangeFileToken(UnicodeString DelimitedName,
  const TRemoteToken &Token, TFSCommand Cmd, UnicodeString RecursiveStr)
{
  UnicodeString Str;
  if (Token.GetIDValid())
  {
    Str = ::IntToStr(Token.GetID());
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

void TSCPFileSystem::ChangeFileProperties(UnicodeString AFileName,
  const TRemoteFile *AFile, const TRemoteProperties *Properties,
  TChmodSessionAction &Action)
{
  DebugAssert(Properties);
  bool IsDirectory = AFile && AFile->GetIsDirectory();
  bool Recursive = Properties->Recursive && IsDirectory;
  UnicodeString RecursiveStr = Recursive ? L"-R" : L"";

  UnicodeString DelimitedName = DelimitStr(AFileName);
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
        RecursiveStr, Rights.GetSimplestStr(), DelimitedName);
    }

    // if file is directory and we do recursive mode settings with
    // add-x-to-directories option on, add those X
    if (Recursive && IsDirectory && Properties->AddXToDirectories)
    {
      Rights.AddExecute();
      ExecCommand(fsChangeMode, 0,
        L"", Rights.GetSimplestStr(), DelimitedName);
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

void TSCPFileSystem::CalculateFilesChecksum(UnicodeString /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  DebugFail();
}

void TSCPFileSystem::CustomCommandOnFile(UnicodeString AFileName,
  const TRemoteFile *AFile, UnicodeString Command, intptr_t Params,
  TCaptureOutputEvent OutputEvent)
{
  DebugAssert(AFile);
  bool Dir = AFile->GetIsDirectory() && FTerminal->CanRecurseToDirectory(AFile);
  if (Dir && (Params & ccRecursive))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    FTerminal->ProcessDirectory(AFileName, nb::bind(&TTerminal::CustomCommandOnFile, FTerminal),
      &AParams);
  }

  if (!Dir || (Params & ccApplyToDirectories))
  {
    TCustomCommandData Data(FTerminal);
    UnicodeString Cmd = TRemoteCustomCommand(
        Data, FTerminal->RemoteGetCurrentDirectory(), AFileName, L"").
      Complete(Command, true);

    if (!FTerminal->DoOnCustomCommand(Cmd))
    {
      AnyCommand(Cmd, OutputEvent);
    }
  }
}

void TSCPFileSystem::CaptureOutput(UnicodeString AddedLine, TCaptureOutputType OutputType)
{
  intptr_t ReturnCode;
  UnicodeString Line = AddedLine;
  // TSecureShell never uses cotExitCode
  DebugAssert((OutputType == cotOutput) || (OutputType == cotError));
  if ((OutputType == cotError) || DebugAlwaysFalse(OutputType == cotExitCode) ||
    !RemoveLastLine(Line, ReturnCode) ||
    !Line.IsEmpty())
  {
    DebugAssert(FOnCaptureOutput != nullptr);
    FOnCaptureOutput(Line, OutputType);
  }
}

void TSCPFileSystem::AnyCommand(UnicodeString Command,
  TCaptureOutputEvent OutputEvent)
{
  DebugAssert(!FSecureShell->GetOnCaptureOutput());
  if (OutputEvent)
  {
    FSecureShell->SetOnCaptureOutput(nb::bind(&TSCPFileSystem::CaptureOutput, this));
    FOnCaptureOutput = OutputEvent;
  }

  try__finally
  {
    SCOPE_EXIT
    {
      FOnCaptureOutput = nullptr;
      FSecureShell->SetOnCaptureOutput(nullptr);
    };
    ExecCommand(fsAnyCommand,
      ecDefault | ecIgnoreWarnings, Command);
  }
  __finally
  {
#if 0
    FOnCaptureOutput = nullptr;
    FSecureShell->OnCaptureOutput = nullptr;
#endif // #if 0
  };
}

TStrings *TSCPFileSystem::GetFixedPaths() const
{
  return nullptr;
}

void TSCPFileSystem::SpaceAvailable(UnicodeString /*APath*/,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  DebugFail();
}

// transfer protocol

uintptr_t TSCPFileSystem::ConfirmOverwrite(
  UnicodeString ASourceFullFileName,
  UnicodeString ATargetFileName, TOperationSide Side,
  const TOverwriteFileParams *FileParams, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress)
{
  TSuspendFileOperationProgress Suspend(OperationProgress);

  TQueryButtonAlias Aliases[3];
  Aliases[0].Button = qaAll;
  Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
  Aliases[0].GroupWith = qaYes;
  Aliases[0].GrouppedShiftState = ssCtrl;
  Aliases[1].Button = qaYesToAll;
  Aliases[1].GroupWith = qaYes;
  Aliases[1].GrouppedShiftState = ssShift;
  Aliases[2].Button = qaNoToAll;
  Aliases[2].GroupWith = qaNo;
  Aliases[2].GrouppedShiftState = ssShift;
  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = Aliases;
  QueryParams.AliasesCount = _countof(Aliases);
  uintptr_t Answer =
    FTerminal->ConfirmFileOverwrite(
      ASourceFullFileName, ATargetFileName, FileParams,
      qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
      &QueryParams, Side, CopyParam, Params, OperationProgress);
  return Answer;
}

void TSCPFileSystem::SCPResponse(bool *GotLastLine)
{
  // Taken from scp.c response() and modified

  uint8_t Resp;
  FSecureShell->Receive(&Resp, 1);

  switch (Resp)
  {
  case 0: /* ok */
    FTerminal->LogEvent("SCP remote side confirmation (0)");
    return;

  default:
  case 1: /* error */
  case 2: /* fatal error */
    // pscp adds 'Resp' to 'Msg', why?
    UnicodeString Msg = FSecureShell->ReceiveLine();
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
      catch (...)
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
        FTerminal->LogEvent(L"SCP remote side error (1), fatal error detected from error message");
        Resp = 2;
        FScpFatalError = true;
      }
      else
      {
        FTerminal->LogEvent(L"SCP remote side error (1)");
      }
    }
    else
    {
      FTerminal->LogEvent(L"SCP remote side fatal error (2)");
      FScpFatalError = true;
    }

    if (Resp == 1)
    {
      ThrowFileSkipped(nullptr, Msg);
    }
    else
    {
      ThrowScpEror(nullptr, Msg);
    }
  }
}

void TSCPFileSystem::CopyToRemote(const TStrings *AFilesToCopy,
  UnicodeString TargetDir, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress,
  TOnceDoneOperation &OnceDoneOperation)
{
  // scp.c: source(), toremote()
  DebugAssert(AFilesToCopy && OperationProgress);

  Params &= ~(cpAppend | cpResume);
  bool CheckExistence = base::UnixSamePath(TargetDir, FTerminal->RemoteGetCurrentDirectory()) &&
    (FTerminal->GetFiles() != nullptr) && FTerminal->GetFiles()->GetLoaded();
  bool CopyBatchStarted = false;
  bool Failed = true;
  bool GotLastLine = false;
  TCopyParamType CopyParamCur;
  CopyParamCur.Assign(CopyParam);

  UnicodeString TargetDirFull = base::UnixIncludeTrailingBackslash(TargetDir);

  UnicodeString Options = InitOptionsStr(CopyParam);

  FScpFatalError = false;
  SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
      Options, DelimitStr(base::UnixExcludeTrailingBackslash(TargetDir))));
  SkipFirstLine();

  try__finally
  {
    SCOPE_EXIT
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
        catch (Exception &E)
        {
          // Only log error message (it should always succeed, but
          // some pending error maybe in queue) }
          FTerminal->GetLog()->AddException(&E);
        }
      }
    };
    try
    {
      SCPResponse(&GotLastLine);

      // This can happen only if SCP command is not executed and return code is 0
      // It has never happened to me (return code is usually 127)
      if (GotLastLine)
      {
        throw Exception(L"");
      }
    }
    catch (Exception &E)
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

    for (intptr_t IFile = 0; (IFile < AFilesToCopy->GetCount()) &&
      !OperationProgress->GetCancel(); ++IFile)
    {
      UnicodeString FileName = AFilesToCopy->GetString(IFile);
      TRemoteFile *File1 = AFilesToCopy->GetAs<TRemoteFile>(IFile);
      UnicodeString RealFileName = File1 ? File1->GetFileName() : FileName;
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
        TRemoteFile *File2 = FTerminal->FFiles->FindFile(FileNameOnly);
        if (File2 != nullptr)
        {
          uintptr_t Answer;
		  if (!CopyParam->GetPreserveRights())
			  CopyParamCur.SetRights(*File2->GetRights());
          if (File2->GetIsDirectory())
          {
            UnicodeString Message = FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly);
            TQueryParams QueryParams(qpNeverAskAgainCheck);

            TSuspendFileOperationProgress Suspend(OperationProgress);
            Answer = FTerminal->ConfirmFileOverwrite(
                FileName, FileNameOnly, nullptr,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
                &QueryParams, osRemote, &CopyParamCur, Params, OperationProgress, Message);
          }
          else
          {
            int64_t MTime = 0;
            TOverwriteFileParams FileParams;
            FTerminal->TerminalOpenLocalFile(FileName, GENERIC_READ, nullptr,
              nullptr, nullptr, &MTime, nullptr,
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

          if (::DirectoryExists(ApiPath(::ExtractFilePath(FileName))))
          {
            FTerminal->DirectoryModified(base::UnixIncludeTrailingBackslash(TargetDir) +
              FileNameOnly, true);
          }
        }

        try
        {
          SCPSource(FileName, File1, TargetDirFull,
            &CopyParamCur, Params, OperationProgress, 0);
          OperationProgress->Finish(RealFileName, true, OnceDoneOperation);
        }
        catch (EFileSkipped &E)
        {
          TQueryParams QueryParams(qpAllowContinueOnError);

          TSuspendFileOperationProgress Suspend1(OperationProgress);

          if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName), &E,
              qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
          {
            OperationProgress->SetCancel(csCancel);
          }
          OperationProgress->Finish(FileName, false, OnceDoneOperation);
          if (!FTerminal->HandleException(&E))
          {
            throw;
          }
        }
        catch (ESkipFile &E)
        {
          OperationProgress->Finish(FileName, false, OnceDoneOperation);

          {
            TSuspendFileOperationProgress Suspend(OperationProgress);
            // If ESkipFile occurs, just log it and continue with next file
            if (!FTerminal->HandleException(&E))
            {
              throw;
            }
          }
        }
        catch (...)
        {
          OperationProgress->Finish(FileName, false, OnceDoneOperation);
          throw;
        }
      }
    }
    Failed = false;
  }
  __finally
  {
#if 0
    // Tell remote side, that we're done.
    if (FTerminal->Active)
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
      catch (Exception &E)
      {
        // Only log error message (it should always succeed, but
        // some pending error maybe in queue) }
        FTerminal->Log->AddException(&E);
      }
    }
#endif // #if 0
  };
}

void TSCPFileSystem::SCPSource(UnicodeString AFileName,
  const TRemoteFile *AFile,
  const UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, intptr_t Level)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : AFileName;
  UnicodeString DestFileName =
    FTerminal->ChangeFileName(
      CopyParam, base::ExtractFileName(RealFileName, false), osLocal, Level == 0);

  FTerminal->LogEvent(FORMAT("File: \"%s\"", RealFileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(AFileName, CopyParam, OperationProgress))
  {
    ThrowSkipFileNull();
  }

  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  uintptr_t LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
  int64_t MTime, ATime;
  int64_t Size;

  FTerminal->TerminalOpenLocalFile(AFileName, GENERIC_READ,
    &LocalFileAttrs, &LocalFileHandle, nullptr, &MTime, &ATime, &Size);

  bool Dir = FLAGSET(LocalFileAttrs, faDirectory);
  std::unique_ptr<TSafeHandleStream> Stream(new TSafeHandleStream(LocalFileHandle));
  try__finally
  {
    SCOPE_EXIT
    {
      if (LocalFileHandle != INVALID_HANDLE_VALUE)
      {
        SAFE_CLOSE_HANDLE(LocalFileHandle);
      }
    };
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      SCPDirectorySource(AFileName, TargetDir, CopyParam, Params, OperationProgress, Level);
    }
    else
    {
      UnicodeString AbsoluteFileName = FTerminal->GetAbsolutePath(/* TargetDir + */DestFileName, false);
      DebugAssert(LocalFileHandle != INVALID_HANDLE_VALUE);

      // File is regular file (not directory)
      FTerminal->LogEvent(FORMAT("Copying \"%s\" to remote directory started.", RealFileName));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->GetLocalSize());
      OperationProgress->SetTransferringFile(false);

      TDateTime Modification = ::UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());

      // Will we use ASCII of BINARY file transfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      MaskParams.Modification = Modification;
      UnicodeString BaseFileName = FTerminal->GetBaseFileName(RealFileName);
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(BaseFileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        UnicodeString((OperationProgress->GetAsciiTransfer() ? L"Ascii" : L"Binary")) +
        L" transfer mode selected.");

      TUploadSessionAction Action(FTerminal->GetActionLog());
      Action.SetFileName(::ExpandUNCFileName(AFileName));
      Action.Destination(AbsoluteFileName);

      TRights Rights = CopyParam->RemoteFileRights(LocalFileAttrs);

      try
      {
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
          FileOperationLoopCustom(FTerminal, OperationProgress, !OperationProgress->GetTransferringFile(),
            FMTLOAD(READ_ERROR, AFileName), "",
          [&]()
          {
            BlockBuf.LoadStream(Stream.get(), OperationProgress->LocalBlockSize(), true);
          });

          OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

          // We do ASCII transfer: convert EOL of current block
          // (we don't convert whole buffer, cause it would produce
          // huge memory-transfers while inserting/deleting EOL characters)
          // Than we add current block to file buffer
          if (OperationProgress->GetAsciiTransfer())
          {
            int ConvertParams =
              FLAGMASK(CopyParam->GetRemoveCtrlZ(), cpRemoveCtrlZ) |
              FLAGMASK(CopyParam->GetRemoveBOM(), cpRemoveBOM);
            BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
              FTerminal->GetSessionData()->GetEOLType(),
              ConvertParams, ConvertToken);
            BlockBuf.GetMemory()->Seek(0, soFromBeginning);
            AsciiBuf.ReadStream(BlockBuf.GetMemory(), BlockBuf.GetSize(), true);
            // We don't need it any more
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
              // TVarRec don't understand 'uint32_t' -> we use sprintf()
              Buf = FORMAT(L"T%lu 0 %lu 0", static_cast<uint32_t>(MTime),
                  static_cast<uint32_t>(ATime));
              FSecureShell->SendLine(Buf);
              SCPResponse();
            }

            // Send file modes (rights), filesize and file name
            // TVarRec don't understand 'uint32_t' -> we use sprintf()
            int64_t sz = OperationProgress->GetAsciiTransfer() ? AsciiBuf.GetSize() :
              OperationProgress->GetLocalSize();
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
              while (!OperationProgress->IsTransferDone())
              {
                uintptr_t BlockSize = OperationProgress->TransferBlockSize();
                FSecureShell->Send(
                  reinterpret_cast<uint8_t *>(AsciiBuf.GetData() + static_cast<intptr_t>(OperationProgress->GetTransferredSize())),
                  BlockSize);
                OperationProgress->AddTransferred(BlockSize);
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
            FSecureShell->Send(reinterpret_cast<const uint8_t *>(BlockBuf.GetData()), ToInt(BlockBuf.GetSize()));
            OperationProgress->AddTransferred(BlockBuf.GetSize());
          }

          if ((OperationProgress->GetCancel() == csCancelTransfer) ||
            (OperationProgress->GetCancel() == csCancel && !OperationProgress->GetTransferringFile()))
          {
            throw Exception(MainInstructions(LoadStr(USER_TERMINATED)));
          }
        }
        while (!OperationProgress->IsLocallyDone() || !OperationProgress->IsTransferDone());

        FSecureShell->SendNull();
        try
        {
          SCPResponse();
          // If one of two following exceptions occurs, it means, that remote
          // side already know, that file transfer finished, even if it failed
          // so we don't have to throw EFatal
        }
        catch (EScp &)
        {
          // SCP protocol fatal error
          OperationProgress->SetTransferringFile(false);
          throw;
        }
        catch (EFileSkipped &)
        {
          // SCP protocol non-fatal error
          OperationProgress->SetTransferringFile(false);
          throw;
        }

        // We succeeded transferring file, from now we can handle exceptions
        // normally -> no fatal error
        OperationProgress->SetTransferringFile(false);
      }
      catch (Exception &E)
      {
        // EScpFileSkipped is derived from ESkipFile,
        // but is does not indicate file skipped by user here
        if (isa<EFileSkipped>(&E))
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
        TTouchSessionAction(FTerminal->GetActionLog(), AbsoluteFileName, Modification);
      }
      if (CopyParam->GetPreserveRights())
      {
        TChmodSessionAction(FTerminal->GetActionLog(), AbsoluteFileName,
          Rights);
      }

      FTerminal->LogFileDone(OperationProgress, AbsoluteFileName);
    }
  }
  __finally
  {
#if 0
    if (File != nullptr)
    {
      CloseHandle(File);
    }
    delete Stream;
#endif // #if 0
  };

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, AFileName), "",
      [&]()
      {
        THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(AFileName)));
      });
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
  {
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, AFileName), "",
    [&]()
    {
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(AFileName), LocalFileAttrs & ~faArchive));
    });
  }

  FTerminal->LogEvent(FORMAT("Copying \"%s\" to remote directory finished.", AFileName));
}

void TSCPFileSystem::SCPDirectorySource(UnicodeString DirectoryName,
  UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, intptr_t Level)
{
  DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;

  FTerminal->LogEvent(FORMAT("Entering directory \"%s\".", DirectoryName));

  OperationProgress->SetFile(DirectoryName);
  UnicodeString DestFileName =
    FTerminal->ChangeFileName(
      CopyParam, base::ExtractFileName(DirectoryName, false), osLocal, Level == 0);

  // Get directory attributes
  FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_GET_ATTRS, DirectoryName), "",
  [&]()
  {
    LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DirectoryName));
    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      ::RaiseLastOSError();
    }
  });

  UnicodeString TargetDirFull = base::UnixIncludeTrailingBackslash(TargetDir + DestFileName);

  /* TODO 1: maybe send filetime */

  // Send directory modes (rights), filesize and file name
  UnicodeString Buf = FORMAT("D%s 0 %s",
      CopyParam->RemoteFileRights(LocalFileAttrs).GetOctal(), DestFileName);
  FSecureShell->SendLine(Buf);
  SCPResponse();

  try__finally
  {
    SCOPE_EXIT
    {
      if (FTerminal->GetActive())
      {
        // Tell remote side, that we're done.
        FTerminal->LogEvent(FORMAT("Leaving directory \"%s\".", DirectoryName));
        FSecureShell->SendLine(L"E");
        SCPResponse();
      }
    };

    DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    TSearchRecChecked SearchRec;
    bool FindOK = false;
    FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName), "",
    [&]()
    {
      UnicodeString Path = ::IncludeTrailingBackslash(DirectoryName) + L"*.*";
      FindOK = ::FindFirstChecked(Path,
          FindAttrs, SearchRec) == 0;
    });

    try__finally
    {
      SCOPE_EXIT
      {
        base::FindClose(SearchRec);
      };
      while (FindOK && !OperationProgress->GetCancel())
      {
        UnicodeString FileName = ::IncludeTrailingBackslash(DirectoryName) + SearchRec.Name;
        try
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
          {
            SCPSource(FileName, nullptr, TargetDirFull, CopyParam, Params, OperationProgress, Level + 1);
          }
        }
        // Previously we caught ESkipFile, making error being displayed
        // even when file was excluded by mask. Now the ESkipFile is special
        // case without error message.
        catch (EFileSkipped &E)
        {
          TQueryParams QueryParams(qpAllowContinueOnError);
          TSuspendFileOperationProgress Suspend(OperationProgress);

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
        catch (ESkipFile &E)
        {
          // If ESkipFile occurs, just log it and continue with next file
          TSuspendFileOperationProgress Suspend(OperationProgress);
          if (!FTerminal->HandleException(&E))
          {
            throw;
          }
        }
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(LIST_DIR_ERROR, DirectoryName), "",
        [&]()
        {
          FindOK = (::FindNextChecked(SearchRec) == 0);
        });
      }
    }
    __finally
    {
#if 0
      FindClose(SearchRec);
#endif // #if 0
    };

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
        FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DirectoryName), "",
        [&]()
        {
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DirectoryName), LocalFileAttrs & ~faArchive));
        });
      }
    }
  }
  __finally
  {
#if 0
    if (FTerminal->Active)
    {
      // Tell remote side, that we're done.
      FTerminal->LogEvent(FORMAT("Leaving directory \"%s\".", DirectoryName));
      FSecureShell->SendLine(L"E");
      SCPResponse();
    }
#endif // #if 0
  };
}

void TSCPFileSystem::CopyToLocal(const TStrings *AFilesToCopy,
  UnicodeString TargetDir, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress,
  TOnceDoneOperation &OnceDoneOperation)
{
  bool CloseSCP = False;
  Params &= ~(cpAppend | cpResume);
  UnicodeString Options = InitOptionsStr(CopyParam);

  FTerminal->LogEvent(FORMAT("Copying %d files/directories to local directory "
      "\"%s\"", AFilesToCopy->GetCount(), TargetDir));
  FTerminal->LogEvent(CopyParam->GetLogStr());

  try__finally
  {
    SCOPE_EXIT
    {
      // In case that copying doesn't cause fatal error (ie. connection is
      // still active) but wasn't successful (exception or user termination)
      // we need to ensure, that SCP on remote side is closed
      if (FTerminal->GetActive() && (CloseSCP ||
          (OperationProgress->GetCancel() == csCancel) ||
          (OperationProgress->GetCancel() == csCancelTransfer)))
      {
        // If we get LastLine, it means that remote side 'scp' is already
        // terminated, so we need not to terminate it. There is also
        // possibility that remote side waits for confirmation, so it will hang.
        // This should not happen (hope)
        UnicodeString Line = FSecureShell->ReceiveLine();
        bool LastLineRead = IsLastLine(Line);
        if (!LastLineRead)
        {
          SCPSendError((OperationProgress->GetCancel() ? L"Terminated by user." : L"Exception"), true);
        }
        // Just in case, remote side already sent some more data (it's probable)
        // but we don't want to raise exception (user asked to terminate, it's not error)
        intptr_t ECParams = coOnlyReturnCode;
        if (!LastLineRead)
        {
          ECParams |= coWaitForLastLine;
        }
        ReadCommandOutput(ECParams);
      }
    };

    for (intptr_t IFile = 0; (IFile < AFilesToCopy->GetCount()) &&
      !OperationProgress->GetCancel(); ++IFile)
    {
      UnicodeString FileName = AFilesToCopy->GetString(IFile);
      TRemoteFile *File = AFilesToCopy->GetAs<TRemoteFile>(IFile);
      DebugAssert(File);

      try
      {
        bool Success = true; // Have to be set to True (see ::SCPSink)
        SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
            Options, DelimitStr(FileName)));
        SkipFirstLine();

        // Filename is used for error messaging and excluding files only
        // Send in full path to allow path-based excluding
        UnicodeString FullFileName = base::UnixExcludeTrailingBackslash(File->GetFullFileName());
        UnicodeString TargetDirectory = CreateTargetDirectory(File->GetFileName(), TargetDir, CopyParam);
        SCPSink(TargetDirectory, FullFileName, base::UnixExtractFilePath(FullFileName), File,
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
              SCOPE_EXIT
              {
                FTerminal->SetExceptionOnFail(false);
              };
              FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(DELETE_FILE_ERROR, FileName), "",
              [&]()
              {
                // pass full file name in FileName, in case we are not moving
                // from current directory
                FTerminal->RemoteDeleteFile(FileName, File);
              });
            }
            __finally
            {
#if 0
              FTerminal->ExceptionOnFail = false;
#endif // #if 0
            };
          }
          catch (EFatal &)
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

        OperationProgress->Finish(FileName,
          (!OperationProgress->GetCancel() && Success), OnceDoneOperation);
      }
      catch (...)
      {
        OperationProgress->Finish(FileName, false, OnceDoneOperation);
        CloseSCP = (OperationProgress->GetCancel() != csRemoteAbort);
        throw;
      }
    }
  }
  __finally
  {
#if 0
    // In case that copying doesn't cause fatal error (ie. connection is
    // still active) but wasn't successful (exception or user termination)
    // we need to ensure, that SCP on remote side is closed
    if (FTerminal->Active && (CloseSCP ||
        (OperationProgress->Cancel == csCancel) ||
        (OperationProgress->Cancel == csCancelTransfer)))
    {
      bool LastLineRead;

      // If we get LastLine, it means that remote side 'scp' is already
      // terminated, so we need not to terminate it. There is also
      // possibility that remote side waits for confirmation, so it will hang.
      // This should not happen (hope)
      UnicodeString Line = FSecureShell->ReceiveLine();
      LastLineRead = IsLastLine(Line);
      if (!LastLineRead)
      {
        SCPSendError((OperationProgress->Cancel ? L"Terminated by user." : L"Exception"), true);
      }
      // Just in case, remote side already sent some more data (it's probable)
      // but we don't want to raise exception (user asked to terminate, it's not error)
      intptr_t ECParams = coOnlyReturnCode;
      if (!LastLineRead) ECParams |= coWaitForLastLine;
      ReadCommandOutput(ECParams);
    }
#endif // #if 0
  };
}

void TSCPFileSystem::SCPError(const UnicodeString Message, bool Fatal)
{
  SCPSendError(Message, Fatal);
  ThrowFileSkipped(nullptr, Message);
}

void TSCPFileSystem::SCPSendError(const UnicodeString Message, bool Fatal)
{
  uint8_t ErrorLevel = static_cast<uint8_t>(Fatal ? 2 : 1);
  FTerminal->LogEvent(FORMAT("Sending SCP error (%d) to remote side:",
      ToInt(ErrorLevel)));
  FSecureShell->Send(&ErrorLevel, 1);
  // We don't send exact error message, because some unspecified
  // characters can terminate remote scp
  FSecureShell->SendLine(FORMAT("scp: error: %s", Message));
}

void TSCPFileSystem::SCPSink(const UnicodeString TargetDir,
  const UnicodeString AFileName, const UnicodeString SourceDir,
  const TRemoteFile * /*AFile*/,
  const TCopyParamType *CopyParam, bool &Success,
  TFileOperationProgressType *OperationProgress, intptr_t Params,
  intptr_t Level)
{
  struct
  {
    int SetTime;
    FILETIME AcTime;
    FILETIME WrTime;
    TRights RemoteRights;
    DWORD LocalFileAttrs;
    bool Exists;
  } FileData;
  TDateTime SourceTimestamp;

  bool SkipConfirmed = false;
  bool Initialized = (Level > 0);

  FileData.SetTime = 0;

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
    UnicodeString AbsoluteFileName = AFileName;

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
            throw Exception(L"");
          }
        }
        catch (Exception &E)
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

        switch (Ctrl)
        {
          case 1:
            // Error (already logged by ReceiveLine())
            ThrowFileSkipped(nullptr, FMTLOAD(REMOTE_ERROR, Line));

          case 2:
            // Fatal error, terminate copying
            FTerminal->TerminalError(Line);
            return; // Unreachable

          case L'E': // Exit
            FSecureShell->SendNull();
            return;

          case L'T':
            int64_t MTime, ATime;
            MTime = ATime = 0;
            if (swscanf(Line.c_str(), L"%I64d %*d %I64d %*d", &MTime, &ATime) == 2)
            {
              const TSessionData * Data = FTerminal->GetSessionData();
              FileData.AcTime = ::DateTimeToFileTime(::UnixToDateTime(ATime,
                Data->GetDSTMode()), Data->GetDSTMode());
              FileData.WrTime = ::DateTimeToFileTime(::UnixToDateTime(MTime,
                Data->GetDSTMode()), Data->GetDSTMode());
              SourceTimestamp = ::UnixToDateTime(MTime,
                Data->GetDSTMode());
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
        MaskParams.Modification = SourceTimestamp;

        // We reach this point only if control record was 'C' or 'D'
        try
        {
          FileData.RemoteRights.SetOctal(CutToChar(Line, L' ', True));
          // do not trim leading spaces of the filename
          int64_t TSize = ::StrToInt64(CutToChar(Line, L' ', False).TrimRight());
          MaskParams.Size = TSize;
          // Security fix: ensure the file ends up where we asked for it.
          // (accept only filename, not path)
          UnicodeString OnlyFileName = base::UnixExtractFileName(Line);
          if (Line != OnlyFileName)
          {
            FTerminal->LogEvent(FORMAT("Warning: Remote host set a compound pathname '%s'", Line));
          }

          AbsoluteFileName = SourceDir + OnlyFileName;
          OperationProgress->SetFile(AbsoluteFileName);
          OperationProgress->SetTransferSize(TSize);
        }
        catch (Exception &E)
        {
          {
            TSuspendFileOperationProgress Suspend(OperationProgress);
            FTerminal->GetLog()->AddException(&E);
          }
          SCPError(LoadStr(SCP_ILLEGAL_FILE_DESCRIPTOR), false);
        }

        // last possibility to cancel transfer before it starts
        if (OperationProgress->GetCancel())
        {
          ThrowSkipFile(nullptr, MainInstructions(LoadStr(USER_TERMINATED)));
        }

        bool Dir = (Ctrl == L'D');
        UnicodeString BaseFileName = FTerminal->GetBaseFileName(AbsoluteFileName);
        if (!CopyParam->AllowTransfer(BaseFileName, osRemote, Dir, MaskParams))
        {
          FTerminal->LogEvent(FORMAT("File \"%s\" excluded from transfer",
              AbsoluteFileName));
          SkipConfirmed = true;
          SCPError(L"", false);
        }

        if (CopyParam->SkipTransfer(AbsoluteFileName, Dir))
        {
          SkipConfirmed = true;
          SCPError(L"", false);
          OperationProgress->AddSkippedFileSize(MaskParams.Size);
        }

        FTerminal->LogFileDetails(AFileName, SourceTimestamp, MaskParams.Size);

        UnicodeString DestFileNameOnly =
          FTerminal->ChangeFileName(
            CopyParam, OperationProgress->GetFileName(), osRemote,
            Level == 0);
        UnicodeString DestFileName =
          ::IncludeTrailingBackslash(TargetDir) + DestFileNameOnly;

        FileData.LocalFileAttrs = FTerminal->GetLocalFileAttributes(ApiPath(DestFileName));
        // If getting attrs fails, we suppose, that file/folder doesn't exists
        FileData.Exists = (FileData.LocalFileAttrs != INVALID_FILE_ATTRIBUTES);
        if (Dir)
        {
          if (FileData.Exists && !(FileData.LocalFileAttrs & faDirectory))
          {
            SCPError(FMTLOAD(NOT_DIRECTORY_ERROR, DestFileName), false);
          }

          if (!FileData.Exists)
          {
            FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CREATE_DIR_ERROR, DestFileName), "",
            [&]()
            {
              THROWOSIFFALSE(::ForceDirectories(ApiPath(DestFileName)));
            });
            /* SCP: can we set the timestamp for directories ? */
          }
          UnicodeString FullFileName = SourceDir + OperationProgress->GetFileName();
          SCPSink(DestFileName, FullFileName, base::UnixIncludeTrailingBackslash(FullFileName), nullptr,
            CopyParam, Success, OperationProgress, Params, Level + 1);
          continue;
        }
        else if (Ctrl == L'C')
        {
          TDownloadSessionAction Action(FTerminal->GetActionLog());
          Action.SetFileName(AbsoluteFileName);

          try
          {
            HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
            std::unique_ptr<TStream> FileStream;

            /* TODO 1 : Turn off read-only attr */

            try__finally
            {
              SCOPE_EXIT
              {
                SAFE_CLOSE_HANDLE(LocalFileHandle);
                FileStream.reset();
              };
              try
              {
                if (::FileExists(ApiPath(DestFileName)))
                {
                  int64_t MTime = 0;
                  TOverwriteFileParams FileParams;
                  FileParams.SourceSize = OperationProgress->GetTransferSize();
                  FileParams.SourceTimestamp = SourceTimestamp;
                  FTerminal->TerminalOpenLocalFile(DestFileName, GENERIC_READ,
                    nullptr, nullptr, nullptr, &MTime, nullptr,
                    &FileParams.DestSize);
                  FileParams.DestTimestamp = ::UnixToDateTime(MTime,
                      FTerminal->GetSessionData()->GetDSTMode());

                  uintptr_t Answer =
                    ConfirmOverwrite(OperationProgress->GetFileName(), DestFileNameOnly, osLocal,
                      &FileParams, CopyParam, Params, OperationProgress);

                  switch (Answer)
                  {
                  case qaCancel:
                    OperationProgress->SetCancel(csCancel); // continue on next case
                  // FALLTHROUGH
                  case qaNo:
                    SkipConfirmed = true;
                    ThrowExtException();
                  }
                }

                Action.Destination(DestFileName);

                if (!FTerminal->TerminalCreateLocalFile(DestFileName, OperationProgress,
                    FLAGSET(Params, cpResume), FLAGSET(Params, cpNoConfirmation),
                    &LocalFileHandle))
                {
                  SkipConfirmed = true;
                  ThrowExtException();
                }

                FileStream.reset(new TSafeHandleStream(LocalFileHandle));
              }
              catch (Exception &E)
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
              FTerminal->LogEvent(UnicodeString((OperationProgress->GetAsciiTransfer() ? L"Ascii" : L"Binary")) +
                L" transfer mode selected.");

              try
              {
                // Buffer for one block of data
                TFileBuffer BlockBuf;
                bool ConvertToken = false;

                do
                {
                  BlockBuf.SetSize(OperationProgress->TransferBlockSize());
                  BlockBuf.SetPosition(0);

                  FSecureShell->Receive(reinterpret_cast<uint8_t *>(BlockBuf.GetData()), static_cast<intptr_t>(BlockBuf.GetSize()));
                  OperationProgress->AddTransferred(BlockBuf.GetSize());

                  if (OperationProgress->GetAsciiTransfer())
                  {
                    int64_t PrevBlockSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetSessionData()->GetEOLType(),
                      FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                    OperationProgress->SetLocalSize(
                      OperationProgress->GetLocalSize() - PrevBlockSize + BlockBuf.GetSize());
                  }

                  // This is crucial, if it fails during file transfer, it's fatal error
                  FileOperationLoopCustom(FTerminal, OperationProgress, false,
                    FMTLOAD(WRITE_ERROR, DestFileName), "",
                  [&]()
                  {
                    BlockBuf.WriteToStream(FileStream.get(), static_cast<uint32_t>(BlockBuf.GetSize()));
                  });

                  OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

                  if (OperationProgress->GetCancel() == csCancelTransfer)
                  {
                    throw Exception(MainInstructions(LoadStr(USER_TERMINATED)));
                  }
                }
                while (!OperationProgress->IsLocallyDone() || !
                  OperationProgress->IsTransferDone());
              }
              catch (Exception &E)
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
              catch (EScp &)
              {
                FSecureShell->SendNull();
                throw;
              }
              catch (EFileSkipped &)
              {
                FSecureShell->SendNull();
                throw;
              }

              FSecureShell->SendNull();

              if (FileData.SetTime && CopyParam->GetPreserveTime())
              {
                SetFileTime(LocalFileHandle, nullptr, &FileData.AcTime, &FileData.WrTime);
              }
            }
            __finally
            {
#if 0
              if (File) CloseHandle(File);
              if (FileStream) delete FileStream;
#endif // #if 0
            };
          }
          catch (Exception &E)
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

          if (FileData.LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
          {
            FileData.LocalFileAttrs = faArchive;
          }
          DWORD NewAttrs = CopyParam->LocalFileAttrs(FileData.RemoteRights);
          if ((NewAttrs & FileData.LocalFileAttrs) != NewAttrs)
          {
            FileOperationLoopCustom(FTerminal, OperationProgress, True, FMTLOAD(CANT_SET_ATTRS, DestFileName), "",
            [&]()
            {
              THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(ApiPath(DestFileName), FileData.LocalFileAttrs | NewAttrs));
            });
          }

          FTerminal->LogFileDone(OperationProgress, DestFileName);
        }
      }
    }
    catch (EFileSkipped &E)
    {
      if (!SkipConfirmed)
      {
        TSuspendFileOperationProgress Suspend(OperationProgress);
        TQueryParams QueryParams(qpAllowContinueOnError);
        if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, AbsoluteFileName),
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
    catch (ESkipFile &E)
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

void TSCPFileSystem::GetSupportedChecksumAlgs(TStrings * /*Algs*/)
{
  // NOOP
}

void TSCPFileSystem::LockFile(UnicodeString /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TSCPFileSystem::UnlockFile(UnicodeString /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TSCPFileSystem::UpdateFromMain(TCustomFileSystem * /*MainFileSystem*/)
{
  // noop
}

UnicodeString TSCPFileSystem::InitOptionsStr(const TCopyParamType *CopyParam) const
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

