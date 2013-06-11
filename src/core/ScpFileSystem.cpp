//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "ScpFileSystem.h"

#include "Terminal.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "TextsCore.h"
#include "SecureShell.h"

#include <stdio.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#undef FILE_OPERATION_LOOP_EX
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
#define THROW_FILE_SKIPPED(EXCEPTION, MESSAGE) \
  throw EScpFileSkipped(EXCEPTION, MESSAGE)

#define THROW_SCP_ERROR(EXCEPTION, MESSAGE) \
  throw EScp(EXCEPTION, MESSAGE)
//===========================================================================
#define MaxShellCommand fsAnyCommand
#define ShellCommandCount MaxShellCommand + 1
#define MaxCommandLen 40
struct TCommandType
{
  int MinLines;
  int MaxLines;
  bool ModifiesFiles;
  bool ChangesDirectory;
  bool InteractiveCommand;
  wchar_t Command[MaxCommandLen];
};

// Only one character! See TSCPFileSystem::ReadCommandOutput()
#define LastLineSeparator L":"
#define LAST_LINE L"NetBox: this is end-of-file"
#define FIRST_LINE L"NetBox: this is begin-of-file"
extern const TCommandType DefaultCommandSet[];

#define NationalVarCount 10
extern const wchar_t NationalVars[NationalVarCount][15];

#define CHECK_CMD assert((Cmd >=0) && (Cmd <= MaxShellCommand))

class TSessionData;
//---------------------------------------------------------------------------
class TCommandSet : public TObject
{
public:
  void SetMasks(const UnicodeString & Value);
  int GetMaxLines(TFSCommand Cmd) const;
  int GetMinLines(TFSCommand Cmd) const;
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
  TCommandSet(TSessionData *aSessionData);
  void Default();
  void CopyFrom(TCommandSet * Source);
#if defined(__BORLANDC__)
  UnicodeString Command(TFSCommand Cmd, const TVarRec * args, int size) const;
#else
  UnicodeString Command(TFSCommand Cmd, ...) const;
  UnicodeString Command(TFSCommand Cmd, va_list args) const;
#endif
  TStrings * CreateCommandList();
#if defined(__BORLANDC__)
  UnicodeString FullCommand(TFSCommand Cmd, const TVarRec * args, int size) const;
#else
  UnicodeString FullCommand(TFSCommand Cmd, ...) const;
  UnicodeString FullCommand(TFSCommand Cmd, va_list args) const;
#endif
  static UnicodeString ExtractCommand(const UnicodeString & Command);
  TSessionData * GetSessionData() const { return FSessionData; }
  void SetSessionData(TSessionData * Value) { FSessionData = Value; }
  void SetReturnVar(const UnicodeString & Value) { FReturnVar = Value; }

private:
  TCommandType CommandSet[ShellCommandCount];
  TSessionData * FSessionData;
  UnicodeString FReturnVar;

private:
  NB_DISABLE_COPY(TCommandSet)
};
//===========================================================================
const wchar_t NationalVars[NationalVarCount][15] =
  {L"LANG", L"LANGUAGE", L"LC_CTYPE", L"LC_COLLATE", L"LC_MONETARY", L"LC_NUMERIC",
   L"LC_TIME", L"LC_MESSAGES", L"LC_ALL", L"HUMAN_BLOCKS" };
const wchar_t FullTimeOption[] = L"--full-time";
//---------------------------------------------------------------------------
#define F false
#define T true
// TODO: remove "mf" and "cd", it is implemented in TTerminal already
const TCommandType DefaultCommandSet[ShellCommandCount] = {
//                       min max mf cd ia  command
/*Null*/                { -1, -1, F, F, F, L"" },
/*VarValue*/            { -1, -1, F, F, F, L"echo \"$%s\"" /* variable */ },
/*LastLine*/            { -1, -1, F, F, F, L"echo \"%s" LastLineSeparator L"%s\"" /* last line, return var */ },
/*FirstLine*/           { -1, -1, F, F, F, L"echo \"%s\"" /* first line */ },
/*CurrentDirectory*/    {  1,  1, F, F, F, L"pwd" },
/*ChangeDirectory*/     {  0,  0, F, T, F, L"cd %s" /* directory */ },
// list directory can be empty on permission denied, this is handled in ReadDirectory
/*ListDirectory*/       { -1, -1, F, F, F, L"%s %s \"%s\"" /* listing command, options, directory */ },
/*ListCurrentDirectory*/{ -1, -1, F, F, F, L"%s %s" /* listing command, options */ },
/*ListFile*/            {  1,  1, F, F, F, L"%s -d %s \"%s\"" /* listing command, options, file/directory */ },
/*LookupUserGroups*/    {  0,  1, F, F, F, L"groups" },
/*CopyToRemote*/        { -1, -1, T, F, T, L"scp -r %s -d -t \"%s\"" /* options, directory */ },
/*CopyToLocal*/         { -1, -1, F, F, T, L"scp -r %s -d -f \"%s\"" /* options, file */ },
/*DeleteFile*/          {  0,  0, T, F, F, L"rm -f -r \"%s\"" /* file/directory */},
/*RenameFile*/          {  0,  0, T, F, F, L"mv -f \"%s\" \"%s\"" /* file/directory, new name*/},
/*CreateDirectory*/     {  0,  0, T, F, F, L"mkdir \"%s\"" /* new directory */},
/*ChangeMode*/          {  0,  0, T, F, F, L"chmod %s %s \"%s\"" /* options, mode, filename */},
/*ChangeGroup*/         {  0,  0, T, F, F, L"chgrp %s \"%s\" \"%s\"" /* options, group, filename */},
/*ChangeOwner*/         {  0,  0, T, F, F, L"chown %s \"%s\" \"%s\"" /* options, owner, filename */},
/*HomeDirectory*/       {  0,  0, F, T, F, L"cd" },
/*Unset*/               {  0,  0, F, F, F, L"unset \"%s\"" /* variable */ },
/*Unalias*/             {  0,  0, F, F, F, L"unalias \"%s\"" /* alias */ },
/*CreateLink*/          {  0,  0, T, F, F, L"ln %s \"%s\" \"%s\"" /*symbolic (-s), filename, point to*/},
/*CopyFile*/            {  0,  0, T, F, F, L"cp -p -r -f \"%s\" \"%s\"" /* file/directory, target name*/},
/*AnyCommand*/          {  0, -1, T, T, F, L"%s" }
};
#undef F
#undef T
//---------------------------------------------------------------------------
TCommandSet::TCommandSet(TSessionData *aSessionData):
  FSessionData(aSessionData), FReturnVar(L"")
{
  assert(FSessionData);
  Default();
}
//---------------------------------------------------------------------------
void TCommandSet::CopyFrom(TCommandSet * Source)
{
  memmove(&CommandSet, Source->CommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
void TCommandSet::Default()
{
  assert(sizeof(CommandSet) == sizeof(DefaultCommandSet));
  memmove(&CommandSet, &DefaultCommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
int TCommandSet::GetMaxLines(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].MaxLines;
}
//---------------------------------------------------------------------------
int TCommandSet::GetMinLines(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].MinLines;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetModifiesFiles(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].ModifiesFiles;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetChangesDirectory(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].ChangesDirectory;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetInteractiveCommand(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].InteractiveCommand;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetOneLineCommand(TFSCommand /*Cmd*/) const
{
  //CHECK_CMD;
  // #56: we send "echo last line" from all commands on same line
  // just as it was in 1.0
  return True; //CommandSet[Cmd].OneLineCommand;
}
//---------------------------------------------------------------------------
void TCommandSet::SetCommands(TFSCommand Cmd, const UnicodeString & Value)
{
  CHECK_CMD;
  wcscpy(const_cast<wchar_t *>(CommandSet[Cmd].Command), Value.SubString(1, MaxCommandLen - 1).c_str());
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::GetCommands(TFSCommand Cmd) const
{
  CHECK_CMD;
  return CommandSet[Cmd].Command;
}
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
UnicodeString TCommandSet::Command(TFSCommand Cmd, const TVarRec * args, int size) const
{
  if (args)
    return Format(GetCommands(Cmd), args, size);
  else
    return GetCommands(Cmd);
}
#endif
//---------------------------------------------------------------------------
UnicodeString TCommandSet::Command(TFSCommand Cmd, ...) const
{
  UnicodeString Result;
  va_list args;
  va_start(args, Cmd);
  Result = Command(Cmd, args);
  va_end(args);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::Command(TFSCommand Cmd, va_list args) const
{
  UnicodeString Result;
  Result = ::Format(GetCommands(Cmd).c_str(), args);
  return Result.c_str();
}
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
UnicodeString TCommandSet::FullCommand(TFSCommand Cmd, const TVarRec * args, int size)
{
  UnicodeString Separator;
  if (GetOneLineCommand(Cmd)) Separator = L" ; ";
    else Separator = L"\n";
  UnicodeString Line = Command(Cmd, args, size);
  UnicodeString LastLineCmd =
    Command(fsLastLine, ARRAYOFCONST((GetLastLine(), GetReturnVar())));
  UnicodeString FirstLineCmd;
  if (GetInteractiveCommand(Cmd))
    FirstLineCmd = Command(fsFirstLine, ARRAYOFCONST((GetFirstLine()))) + Separator;

  UnicodeString Result;
  if (!Line.IsEmpty())
    Result = FORMAT(L"%s%s%s%s", (FirstLineCmd, Line, Separator, LastLineCmd));
  else
    Result = FORMAT(L"%s%s", (FirstLineCmd, LastLineCmd));
  return Result;
}
#endif
//---------------------------------------------------------------------------
UnicodeString TCommandSet::FullCommand(TFSCommand Cmd, ...) const
{
  UnicodeString Result;
  va_list args;
  va_start(args, Cmd);
  Result = FullCommand(Cmd, args);
  va_end(args);
  return Result.c_str();
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::FullCommand(TFSCommand Cmd, va_list args) const
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
    Command(fsLastLine, GetLastLine().c_str(), GetReturnVar().c_str());
  UnicodeString FirstLineCmd;
  if (GetInteractiveCommand(Cmd))
  {
    FirstLineCmd = Command(fsFirstLine, GetFirstLine().c_str()) + Separator;
  }

  UnicodeString Result;
  if (!Line.IsEmpty())
  {
    Result = FORMAT(L"%s%s%s%s", FirstLineCmd.c_str(), Line.c_str(), Separator.c_str(), LastLineCmd.c_str());
  }
  else
  {
    Result = FORMAT(L"%s%s", FirstLineCmd.c_str(), LastLineCmd.c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::GetFirstLine() const
{
  return FIRST_LINE;
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::GetLastLine() const
{
  return LAST_LINE;
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::GetReturnVar() const
{
  assert(GetSessionData());
  if (!FReturnVar.IsEmpty())
  {
    return UnicodeString(L'$') + FReturnVar;
  }
  else if (GetSessionData()->GetDetectReturnVar())
  {
    return L'0';
  }
  else
  {
    return UnicodeString(L'$') + GetSessionData()->GetReturnVar();
  }
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::ExtractCommand(const UnicodeString & Command)
{
  UnicodeString Result = Command;
  intptr_t P = Result.Pos(L" ");
  if (P > 0)
  {
    Result.SetLength(P-1);
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TCommandSet::CreateCommandList()
{
  TStrings * CommandList = new TStringList();
  for (intptr_t Index = 0; Index < ShellCommandCount; ++Index)
  {
    UnicodeString Cmd = GetCommands(static_cast<TFSCommand>(Index));
    if (!Cmd.IsEmpty())
    {
      Cmd = ExtractCommand(Cmd);
      if ((Cmd != L"%s") && (CommandList->IndexOf(Cmd.c_str()) < 0))
        CommandList->Add(Cmd);
    }
  }
  return CommandList;
}
//===========================================================================
TSCPFileSystem::TSCPFileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(ATerminal)
{
}

void TSCPFileSystem::Init(void * Data)
{
  FSecureShell = reinterpret_cast<TSecureShell *>(Data);
  assert(FSecureShell);
  FCommandSet = new TCommandSet(FTerminal->GetSessionData());
  FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
  FOutput = new TStringList();
  FProcessingCommand = false;
  FOnCaptureOutput = nullptr;

  FFileSystemInfo.ProtocolBaseName = L"SCP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
  // capabilities of SCP protocol are fixed
  for (intptr_t Index = 0; Index < fcCount; ++Index)
  {
    FFileSystemInfo.IsCapable[Index] = IsCapable(Index);
  }
}
//---------------------------------------------------------------------------
TSCPFileSystem::~TSCPFileSystem()
{
  delete FCommandSet;
  delete FOutput;
  delete FSecureShell;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::Open()
{
  FSecureShell->Open();
}
//---------------------------------------------------------------------------
void TSCPFileSystem::Close()
{
  FSecureShell->Close();
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::GetActive() const
{
  return FSecureShell->GetActive();
}
//---------------------------------------------------------------------------
const TSessionInfo & TSCPFileSystem::GetSessionInfo()
{
  return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TSCPFileSystem::GetFileSystemInfo(bool Retrieve)
{
  if (FFileSystemInfo.AdditionalInfo.IsEmpty() && Retrieve)
  {
    UnicodeString UName;
    FTerminal->SetExceptionOnFail(true);
    auto cleanup = finally([&]()
    {
      FTerminal->SetExceptionOnFail(false);
    });
    {
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
      catch(...)
      {
        if (!FTerminal->GetActive())
        {
          throw;
        }
      }
    }

    FFileSystemInfo.RemoteSystem = UName;
  }

  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::TemporaryTransferFile(const UnicodeString & /*FileName*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::GetStoredCredentialsTried()
{
  return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
UnicodeString TSCPFileSystem::GetUserName()
{
  return FSecureShell->GetUserName();
}
//---------------------------------------------------------------------------
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
        ExecCommand2(fsNull, 0);
      }
      else
      {
        FTerminal->LogEvent(L"Cannot send keepalive, command is being executed");
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
//---------------------------------------------------------------------------
UnicodeString TSCPFileSystem::AbsolutePath(const UnicodeString & Path, bool /*Local*/)
{
  return ::AbsolutePath(GetCurrentDirectory(), Path);
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsCapable(intptr_t Capability) const
{
  assert(FTerminal);
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
      return false;

    default:
      assert(false);
      return false;
  }
}
//---------------------------------------------------------------------------
UnicodeString TSCPFileSystem::DelimitStr(const UnicodeString & Str)
{
  UnicodeString Result = Str;
  if (!Result.IsEmpty())
  {
    Result = ::DelimitStr(Result, L"\\`$\"");
    if (Result[1] == L'-') Result = L"./"+Result;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.IsEmpty())
  {
    FTerminal->LogEvent(FORMAT(L"Locating to cached directory \"%s\".",
      FCachedDirectoryChange.c_str()));
    UnicodeString Directory = FCachedDirectoryChange;
    FCachedDirectoryChange = L"";
    try
    {
      ChangeDirectory(Directory);
    }
    catch(...)
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
//---------------------------------------------------------------------------
void TSCPFileSystem::SendCommand(const UnicodeString & Cmd)
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
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsTotalListingLine(const UnicodeString & Line)
{
  // On some hosts there is not "total" but "totalt". What's the reason??
  // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
  return !Line.SubString(1, 5).CompareIC(L"total");
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::RemoveLastLine(UnicodeString & Line,
  intptr_t & ReturnCode, const UnicodeString & ALastLine)
{
  bool IsLastLine = false;
  UnicodeString LastLine = ALastLine;
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
    if (TryStrToInt(ReturnCodeStr, ReturnCode))
    {
      IsLastLine = true;
      Line.SetLength(Pos - 1);
    }
  }
  return IsLastLine;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsLastLine(UnicodeString & Line)
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
//---------------------------------------------------------------------------
void TSCPFileSystem::SkipFirstLine()
{
  UnicodeString Line = FSecureShell->ReceiveLine();
  if (Line != FCommandSet->GetFirstLine())
  {
    FTerminal->TerminalError(nullptr, FMTLOAD(FIRST_LINE_EXPECTED, Line.c_str()));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadCommandOutput(intptr_t Params, const UnicodeString * Cmd)
{
  auto cleanup = finally([&]()
  {
    FProcessingCommand = false;
  });
  {
    if (Params & coWaitForLastLine)
    {
      UnicodeString Line;
      bool IsLast = true;
      intptr_t Total = 0;
      // #55: fixed so, even when last line of command output does not
      // contain CR/LF, we can recognize last line
      do
      {
        Line = FSecureShell->ReceiveLine();
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
              FTerminal->DoReadDirectoryProgress(Total, Cancel);
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

      if ((Params & coOnlyReturnCode) && WrongReturnCode)
      {
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, GetReturnCode()));
      }
      else if (!(Params & coOnlyReturnCode) &&
          ((!Message.IsEmpty() && ((FOutput->GetCount() == 0) || !(Params & coIgnoreWarnings))) ||
           WrongReturnCode))
      {
        assert(Cmd != nullptr);
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED, Cmd->c_str(), GetReturnCode(), Message.c_str()));
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ExecCommand(const UnicodeString & Cmd, intptr_t Params,
  const UnicodeString & CmdString)
{
  if (Params < 0)
  {
    Params = ecDefault;
  }
  if (FTerminal->GetUseBusyCursor())
  {
    Busy(true);
  }
  auto cleanup = finally([&]()
  {
    if (FTerminal->GetUseBusyCursor())
    {
      Busy(false);
    }
  });
  {
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
}
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
void TSCPFileSystem::ExecCommand(TFSCommand Cmd, const TVarRec * args,
  int size, intptr_t Params)
{
  if (Params < 0) Params = ecDefault;
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
      FTerminal->TerminalError(FmtLoadStr(INVALID_OUTPUT_ERROR,
        ARRAYOFCONST((FullCommand, GetOutput()->GetText()))));
    }
  }
}
#endif
//---------------------------------------------------------------------------
void TSCPFileSystem::ExecCommand2(TFSCommand Cmd, intptr_t Params, ...)
{
  va_list args;
  va_start(args, Params);
  UnicodeString FullCommand = FCommandSet->FullCommand(Cmd, args);
  UnicodeString Command = FCommandSet->Command(Cmd, args);
  ExecCommand(FullCommand, Params, Command);
  va_end(args);
  if (Params & ecRaiseExcept)
  {
    int MinL = FCommandSet->GetMinLines(Cmd);
    int MaxL = FCommandSet->GetMaxLines(Cmd);
    if (((MinL >= 0) && (MinL > static_cast<int>(FOutput->GetCount()))) ||
        ((MaxL >= 0) && (MaxL > static_cast<int>(FOutput->GetCount()))))
    {
      FTerminal->TerminalError(::FmtLoadStr(INVALID_OUTPUT_ERROR,
        FullCommand.c_str(), GetOutput()->GetText().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TSCPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::DoStartup()
{
  // SkipStartupMessage and DetectReturnVar must succeed,
  // otherwise session is to be closed.
  FTerminal->SetExceptionOnFail(true);
  SkipStartupMessage();
  const TSessionData * Data = FTerminal->GetSessionData();
  if (Data->GetDetectReturnVar())
  {
    DetectReturnVar();
  }
  FTerminal->SetExceptionOnFail(false);

  #define COND_OPER(OPER) if (Data->Get##OPER()) OPER()
  COND_OPER(ClearAliases);
  COND_OPER(UnsetNationalVars);
  #undef COND_OPER
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SkipStartupMessage()
{
  try
  {
    FTerminal->LogEvent(L"Skipping host startup message (if any).");
    ExecCommand2(fsNull, 0);
  }
  catch (Exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(SKIP_STARTUP_MESSAGE_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::LookupUsersGroups()
{
  ExecCommand2(fsLookupUsersGroups, 0);
  FTerminal->FUsers.Clear();
  FTerminal->FGroups.Clear();
  if (FOutput->GetCount() > 0)
  {
    UnicodeString Groups = FOutput->GetString(0);
    while (!Groups.IsEmpty())
    {
      UnicodeString NewGroup = CutToChar(Groups, L' ', false);
      FTerminal->FGroups.Add(TRemoteToken(NewGroup));
      FTerminal->FMembership.Add(TRemoteToken(NewGroup));
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::DetectReturnVar()
{
  // This suppose that something was already executed (probably SkipStartupMessage())
  // or return code variable is already set on start up.

  try
  {
    // #60 17.10.01: "status" and "?" switched
    UnicodeString ReturnVars[2] = { L"status", L"?" };
    UnicodeString NewReturnVar = L"";
    FTerminal->LogEvent(L"Detecting variable containing return code of last command.");
    for (intptr_t Index = 0; Index < 2; ++Index)
    {
      bool Success = true;

      try
      {
        FTerminal->LogEvent(FORMAT(L"Trying \"$%s\".", ReturnVars[Index].c_str()));
        ExecCommand2(fsVarValue, 0, ReturnVars[Index].c_str());
        UnicodeString Str = GetOutput()->GetCount() > 0 ? GetOutput()->GetString(0) : L"";
        intptr_t Val = StrToIntDef(Str, 256);
        if ((GetOutput()->GetCount() != 1) || Str.IsEmpty() || (Val > 255))
        {
          FTerminal->LogEvent(L"The response is not numerical exit code");
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
      Abort();
    }
    else
    {
      FCommandSet->SetReturnVar(NewReturnVar);
      FTerminal->LogEvent(FORMAT(L"Return code variable \"%s\" selected.",
        FCommandSet->GetReturnVar().c_str()));
    }
  }
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(DETECT_RETURNVAR_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ClearAlias(const UnicodeString & Alias)
{
  if (!Alias.IsEmpty())
  {
    // this command usually fails, because there will never be
    // aliases on all commands -> see last False parameter
    ExecCommand2(fsUnalias, 0, Alias.c_str(), false);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ClearAliases()
{
  try
  {
    FTerminal->LogEvent(L"Clearing all aliases.");
    ClearAlias(TCommandSet::ExtractCommand(FTerminal->GetSessionData()->GetListingCommand()));
    std::auto_ptr<TStrings> CommandList(FCommandSet->CreateCommandList());
    for (intptr_t Index = 0; Index < CommandList->GetCount(); ++Index)
    {
      ClearAlias(CommandList->GetString(Index));
    }
  }
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNALIAS_ALL_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::UnsetNationalVars()
{
  try
  {
    FTerminal->LogEvent(L"Clearing national user variables.");
    for (intptr_t Index = 0; Index < NationalVarCount; ++Index)
    {
      ExecCommand2(fsUnset, 0, NationalVars[Index], false);
    }
  }
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNSET_NATIONAL_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    ExecCommand2(fsCurrentDirectory, 0);
    FCurrentDirectory = UnixExcludeTrailingBackslash(FOutput->GetString(0));
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::HomeDirectory()
{
  ExecCommand2(fsHomeDirectory, 0);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::AnnounceFileListOperation()
{
  // noop
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ChangeDirectory(const UnicodeString & Directory)
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
  ExecCommand2(fsChangeDirectory, 0, ToDir.c_str());
  FCachedDirectoryChange = L"";
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FCachedDirectoryChange = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList);
  // emptying file list moved before command execution
  FileList->Clear();

  bool Again;

  do
  {
    Again = false;
    try
    {
      intptr_t Params = ecDefault | ecReadProgress |
        FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
      const wchar_t * Options =
        ((FLsFullTime == asAuto) || (FLsFullTime == asOn)) ? FullTimeOption : L"";
      bool ListCurrentDirectory = (FileList->GetDirectory() == FTerminal->GetCurrentDirectory());
      if (ListCurrentDirectory)
      {
        FTerminal->LogEvent(L"Listing current directory.");
        ExecCommand2(fsListCurrentDirectory, Params,
          FTerminal->GetSessionData()->GetListingCommand().c_str(), Options);
      }
      else
      {
        FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".",
          FileList->GetDirectory().c_str()));
        ExecCommand2(fsListDirectory, Params,
          FTerminal->GetSessionData()->GetListingCommand().c_str(), Options,
            DelimitStr(FileList->GetDirectory().c_str()).c_str());
      }

      // If output is not empty, we have successfully got file listing,
      // otherwise there was an error, in case it was "permission denied"
      // we try to get at least parent directory (see "else" statement below)
      if (FOutput->GetCount() > 0)
      {
        // Copy LS command output, because eventual symlink analysis would
        // modify FTerminal->Output
        std::auto_ptr<TStringList> OutputCopy(new TStringList());
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
          TRemoteFile * File = nullptr;
          File = CreateRemoteFile(OutputCopy->GetString(Index));
          FileList->AddFile(File);
        }
      }
      else
      {
        bool Empty;
        if (ListCurrentDirectory)
        {
          TRemoteFile * File = nullptr;
          // Empty file list -> probably "permission denied", we
          // at least get link to parent directory ("..")
          FTerminal->ReadFile(
            UnixIncludeTrailingBackslash(FTerminal->FFiles->GetDirectory()) +
              PARENTDIRECTORY, File);
          Empty = (File == nullptr);
          if (!Empty)
          {
            assert(File->GetIsParentDirectory());
            FileList->AddFile(File);
          }
        }
        else
        {
          Empty = true;
        }

        if (Empty)
        {
          throw Exception(FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory().c_str()));
        }
      }

      if (FLsFullTime == asAuto)
      {
          FTerminal->LogEvent(
            FORMAT(L"Directory listing with %s succeed, next time all errors during "
              L"directory listing will be displayed immediately.",
              FullTimeOption));
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
            FORMAT(L"Directory listing with %s failed, try again regular listing.",
              FullTimeOption));
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
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadFile(const UnicodeString & FileName,
  TRemoteFile *& File)
{
  CustomReadFile(FileName, File, nullptr);
}
//---------------------------------------------------------------------------
TRemoteFile * TSCPFileSystem::CreateRemoteFile(
  const UnicodeString & ListingStr, TRemoteFile * LinkedByFile)
{
  std::auto_ptr<TRemoteFile> File(new TRemoteFile(LinkedByFile));
  File->SetTerminal(FTerminal);
  File->SetListingStr(ListingStr);
  File->ShiftTime(FTerminal->GetSessionData()->GetTimeDifference());
  File->Complete();

  return File.release();
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CustomReadFile(const UnicodeString & FileName,
  TRemoteFile *& File, TRemoteFile * ALinkedByFile)
{
  File = nullptr;
  intptr_t Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  const wchar_t * Options = (FLsFullTime == asOn) ? FullTimeOption : L"";
  ExecCommand2(fsListFile, Params,
    FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, DelimitStr(FileName).c_str());
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
//---------------------------------------------------------------------------
void TSCPFileSystem::DeleteFile(const UnicodeString & FileName,
  const TRemoteFile * File, intptr_t Params, TRmSessionAction & Action)
{
  USEDPARAM(File);
  USEDPARAM(Params);
  Action.Recursive();
  assert(FLAGCLEAR(Params, dfNoRecursive) || (File && File->GetIsSymLink()));
  ExecCommand2(fsDeleteFile, Params, DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void TSCPFileSystem::RenameFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  ExecCommand2(fsRenameFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CopyFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  ExecCommand2(fsCopyFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CreateDirectory(const UnicodeString & DirName)
{
  ExecCommand2(fsCreateDirectory, 0, DelimitStr(DirName).c_str());
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CreateLink(const UnicodeString & FileName,
  const UnicodeString & PointTo, bool Symbolic)
{
  ExecCommand2(fsCreateLink, 0,
    Symbolic ? L"-s" : L"", DelimitStr(PointTo).c_str(), DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ChangeFileToken(const UnicodeString & DelimitedName,
  const TRemoteToken & Token, TFSCommand Cmd, const UnicodeString & RecursiveStr)
{
  UnicodeString Str;
  if (Token.GetIDValid())
  {
    Str = IntToStr(Token.GetID());
  }
  else if (Token.GetNameValid())
  {
    Str = Token.GetName();
  }

  if (!Str.IsEmpty())
  {
    ExecCommand2(Cmd, 0, RecursiveStr.c_str(), Str.c_str(), DelimitedName.c_str());
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ChangeFileProperties(const UnicodeString & FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  assert(Properties);
  bool IsDirectory = File && File->GetIsDirectory();
  bool Recursive = Properties->Recursive && IsDirectory;
  UnicodeString RecursiveStr = Recursive ? L"-R" : L"";

  UnicodeString DelimitedName = DelimitStr(FileName);
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
      ExecCommand2(fsChangeMode, 0,
        RecursiveStr.c_str(), Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
    }

    // if file is directory and we do recursive mode settings with
    // add-x-to-directories option on, add those X
    if (Recursive && IsDirectory && Properties->AddXToDirectories)
    {
      Rights.AddExecute();
      ExecCommand2(fsChangeMode, 0,
        L"", Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
    }
  }
  else
  {
    Action.Cancel();
  }
  assert(!Properties->Valid.Contains(vpLastAccess));
  assert(!Properties->Valid.Contains(vpModification));
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::LoadFilesProperties(TStrings * /*FileList*/ )
{
  assert(false);
  return false;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CalculateFilesChecksum(const UnicodeString & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CustomCommandOnFile(const UnicodeString & FileName,
  const TRemoteFile * File, const UnicodeString & Command, intptr_t Params,
  TCaptureOutputEvent OutputEvent)
{
  assert(File);
  bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();
  if (Dir && (Params & ccRecursive))
  {
    TCustomCommandParams AParams;
    AParams.Command = Command;
    AParams.Params = Params;
    AParams.OutputEvent = OutputEvent;
    FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::CustomCommandOnFile, FTerminal),
      &AParams);
  }

  if (!Dir || (Params & ccApplyToDirectories))
  {
    TCustomCommandData Data(FTerminal);
    UnicodeString Cmd = TRemoteCustomCommand(
      Data, FTerminal->GetCurrentDirectory(), FileName, L"").
      Complete(Command, true);

    AnyCommand(Cmd, OutputEvent);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CaptureOutput(const UnicodeString & AddedLine, bool StdError)
{
  intptr_t ReturnCode;
  UnicodeString Line = AddedLine;
  if (StdError ||
      !RemoveLastLine(Line, ReturnCode, UnicodeString()) ||
      !Line.IsEmpty())
  {
    assert(FOnCaptureOutput != nullptr);
    FOnCaptureOutput(Line, StdError);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::AnyCommand(const UnicodeString & Command,
  TCaptureOutputEvent OutputEvent)
{
  assert(!FSecureShell->GetOnCaptureOutput());
  if (OutputEvent)
  {
    FSecureShell->SetOnCaptureOutput(MAKE_CALLBACK(TSCPFileSystem::CaptureOutput, this));
    FOnCaptureOutput = OutputEvent;
  }

  auto cleanup = finally([&]()
  {
    FOnCaptureOutput = nullptr;
    FSecureShell->SetOnCaptureOutput(nullptr);
  });
  {
    ExecCommand2(fsAnyCommand, ecDefault | ecIgnoreWarnings, Command.c_str());
  }
}
//---------------------------------------------------------------------------
UnicodeString TSCPFileSystem::FileUrl(const UnicodeString & FileName) const
{
  return FTerminal->FileUrl(L"scp", FileName);
}
//---------------------------------------------------------------------------
TStrings * TSCPFileSystem::GetFixedPaths()
{
  return nullptr;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SpaceAvailable(const UnicodeString & /*APath*/,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
uintptr_t TSCPFileSystem::ConfirmOverwrite(
  UnicodeString & FileName, TOperationSide Side,
  const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress)
{
  TQueryButtonAlias Aliases[3];
  Aliases[0].Button = qaAll;
  Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
  Aliases[0].GroupWith = qaYes;
  Aliases[0].GrouppedShiftState = TShiftState() << ssCtrl;
  Aliases[1].Button = qaYesToAll;
  Aliases[1].GroupWith = qaYes;
  Aliases[1].GrouppedShiftState = TShiftState() << ssShift;
  Aliases[2].Button = qaNoToAll;
  Aliases[2].GroupWith = qaNo;
  Aliases[2].GrouppedShiftState = TShiftState() << ssShift;
  TQueryParams QueryParams(qpNeverAskAgainCheck);
  QueryParams.Aliases = Aliases;
  QueryParams.AliasesCount = LENOF(Aliases);
  uintptr_t Answer;
  SUSPEND_OPERATION
  (
    Answer = FTerminal->ConfirmFileOverwrite(
      FileName, FileParams,
      qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
      &QueryParams, Side, CopyParam, Params, OperationProgress);
  );
  return Answer;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPResponse(bool * GotLastLine)
{
  // Taken from scp.c response() and modified

  unsigned char Resp;
  FSecureShell->Receive(&Resp, 1);

  switch (Resp)
  {
    case 0:     /* ok */
      FTerminal->LogEvent(L"SCP remote side confirmation (0)");
      return;

    default:
    case 1:     /* error */
    case 2:     /* fatal error */
      // pscp adds 'Resp' to 'Msg', why?
      UnicodeString Msg = FSecureShell->ReceiveLine();
      UnicodeString Line = UnicodeString(static_cast<char>(Resp)) + Msg;
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
        FTerminal->LogEvent(L"SCP remote side error (1):");
      }
      else
      {
        FTerminal->LogEvent(L"SCP remote side fatal error (2):");
      }

      if (Resp == 1)
      {
        THROW_FILE_SKIPPED(nullptr, Msg);
      }
        else
      {
        THROW_SCP_ERROR(nullptr, Msg);
      }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  // scp.c: source(), toremote()
  assert(FilesToCopy && OperationProgress);

  Params &= ~(cpAppend | cpResume);
  UnicodeString Options = L"";
  bool CheckExistence = UnixComparePaths(TargetDir, FTerminal->GetCurrentDirectory()) &&
    (FTerminal->FFiles != nullptr) && FTerminal->FFiles->GetLoaded();
  bool CopyBatchStarted = false;
  bool Failed = true;
  bool GotLastLine = false;

  UnicodeString TargetDirFull = UnixIncludeTrailingBackslash(TargetDir);

  if (CopyParam->GetPreserveRights())
  {
    Options = L"-p";
  }
  if (FTerminal->GetSessionData()->GetScp1Compatibility())
  {
    Options += L" -1";
  }

  SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
    Options.c_str(), DelimitStr(UnixExcludeTrailingBackslash(TargetDir)).c_str()));
  SkipFirstLine();

  auto cleanup = finally([&]()
  {
    // Tell remote side, that we're done.
    if (FTerminal->GetActive())
    {
      try
      {
        if (!GotLastLine)
        {
          if (CopyBatchStarted)
          {
            // What about case, remote side sends fatal error ???
            // (Not sure, if it causes remote side to terminate scp)
            FSecureShell->SendLine(L"E");
            SCPResponse();
          }
          /* TODO 1 : Show stderror to user? */
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
  });
  {
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

    for (intptr_t IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; ++IFile)
    {
      UnicodeString FileName = FilesToCopy->GetString(IFile);
      TRemoteFile * File = dynamic_cast<TRemoteFile *>(FilesToCopy->GetObject(IFile));
      UnicodeString RealFileName = File ? File->GetFileName() : FileName;
      bool CanProceed = false;

      UnicodeString FileNameOnly =
        CopyParam->ChangeFileName(ExtractFileName(RealFileName, false), osLocal, true);

      if (CheckExistence)
      {
        // previously there was assertion on FTerminal->FFiles->Loaded, but it
        // fails for scripting, if 'ls' is not issued before.
        // formally we should call CheckRemoteFile here but as checking is for
        // free here (almost) ...
        TRemoteFile * File = FTerminal->FFiles->FindFile(FileNameOnly);
        if (File != nullptr)
        {
          uintptr_t Answer;
          if (File->GetIsDirectory())
          {
            UnicodeString Message = FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly.c_str());
            TQueryParams QueryParams(qpNeverAskAgainCheck);
            SUSPEND_OPERATION
            (
              Answer = FTerminal->ConfirmFileOverwrite(
                FileNameOnly /*not used*/, nullptr,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
                &QueryParams, osRemote, CopyParam, Params, OperationProgress, Message);
            );
          }
          else
          {
            __int64 MTime;
            TOverwriteFileParams FileParams;
            FTerminal->OpenLocalFile(FileName, GENERIC_READ,
              nullptr, nullptr, nullptr, &MTime, nullptr,
              &FileParams.SourceSize);
            FileParams.SourceTimestamp = UnixToDateTime(MTime,
              FTerminal->GetSessionData()->GetDSTMode());
            FileParams.DestSize = File->GetSize();
            FileParams.DestTimestamp = File->GetModification();
            Answer = ConfirmOverwrite(FileNameOnly, osRemote,
              &FileParams, CopyParam, Params, OperationProgress);
          }

          switch (Answer)
          {
            case qaYes:
              CanProceed = true;
              break;

            case qaCancel:
              if (!OperationProgress->Cancel)
              {
                OperationProgress->Cancel = csCancel;
              }
            case qaNo:
              CanProceed = false;
              break;

            default:
              assert(false);
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

          if (DirectoryExists(::ExtractFilePath(FileName)))
          {
            FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir)+
              FileNameOnly, true);
          }
        }

        try
        {
          SCPSource(FileName, File, TargetDirFull,
            CopyParam, Params, OperationProgress, 0);
          OperationProgress->Finish(RealFileName, true, OnceDoneOperation);
        }
        catch (EScpFileSkipped &E)
        {
          TQueryParams QueryParams(qpAllowContinueOnError);
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()), &E,
              qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
            {
              OperationProgress->Cancel = csCancel;
            }
            OperationProgress->Finish(FileName, false, OnceDoneOperation);
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        catch (EScpSkipFile &E)
        {
          OperationProgress->Finish(FileName, false, OnceDoneOperation);
          // If ESkipFile occurs, just log it and continue with next file
          SUSPEND_OPERATION (
            if (!FTerminal->HandleException(&E)) throw;
          );
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
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPSource(const UnicodeString & FileName,
  const TRemoteFile * File,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, intptr_t Level)
{
  UnicodeString RealFileName = File ? File->GetFileName() : FileName;
  UnicodeString DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(RealFileName, false), osLocal, Level == 0);

  FTerminal->LogEvent(FORMAT(L"File: \"%s\"", RealFileName.c_str()));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", RealFileName.c_str()));
    THROW_SKIP_FILE_NULL;
  }

  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  uintptr_t LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
  __int64 MTime, ATime;
  __int64 Size;

  FTerminal->OpenLocalFile(FileName, GENERIC_READ,
    &LocalFileAttrs, &LocalFileHandle, nullptr, &MTime, &ATime, &Size);

  bool Dir = FLAGSET(LocalFileAttrs, faDirectory);
  std::auto_ptr<TSafeHandleStream> Stream(new TSafeHandleStream(LocalFileHandle));
  auto cleanup = finally([&]()
  {
    if (LocalFileHandle != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(LocalFileHandle);
    }
  });
  {
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      SCPDirectorySource(FileName, TargetDir, CopyParam, Params, OperationProgress, Level);
    }
    else
    {
      UnicodeString AbsoluteFileName = FTerminal->AbsolutePath(/* TargetDir + */DestFileName, false);
      assert(LocalFileHandle);

      // File is regular file (not directory)
      FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", RealFileName.c_str()));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->LocalSize);
      OperationProgress->TransferingFile = false;

      TDateTime Modification = UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());

      // Will we use ASCII of BINARY file transfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      MaskParams.Modification = Modification;
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(RealFileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
          L" transfer mode selected.");

      TUploadSessionAction Action(FTerminal->GetActionLog());
      Action.FileName(ExpandUNCFileName(FileName));
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
          FILE_OPERATION_LOOP_EX (!OperationProgress->TransferingFile,
              FMTLOAD(READ_ERROR, FileName.c_str()),
            BlockBuf.LoadStream(Stream.get(), OperationProgress->LocalBlockSize(), true);
          );

          OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

          // We do ASCII transfer: convert EOL of current block
          // (we don't convert whole buffer, cause it would produce
          // huge memory-transfers while inserting/deleting EOL characters)
          // Than we add current block to file buffer
          if (OperationProgress->AsciiTransfer)
          {
            BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
              FTerminal->GetSessionData()->GetEOLType(), cpRemoveCtrlZ | cpRemoveBOM, ConvertToken);
            BlockBuf.GetMemory()->Seek(0, soFromBeginning);
            AsciiBuf.ReadStream(BlockBuf.GetMemory(), BlockBuf.GetSize(), true);
            // We don't need it any more
            BlockBuf.GetMemory()->Clear();
            // Calculate total size to sent (assume that ratio between
            // size of source and size of EOL-transformed data would remain same)
            // First check if file contains anything (div by zero!)
            if (OperationProgress->LocallyUsed)
            {
              __int64 X = OperationProgress->LocalSize;
              X *= AsciiBuf.GetSize();
              X /= OperationProgress->LocallyUsed;
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
          if (!OperationProgress->TransferingFile &&
              (!OperationProgress->AsciiTransfer || OperationProgress->IsLocallyDone()))
          {
            UnicodeString Buf;

            if (CopyParam->GetPreserveTime())
            {
              // Send last file access and modification time
              // TVarRec don't understand 'unsigned int' -> we use sprintf()
              Buf.sprintf(L"T%lu 0 %lu 0", static_cast<unsigned long>(MTime),
                static_cast<unsigned long>(ATime));
              FSecureShell->SendLine(Buf.c_str());
              SCPResponse();
            }

            // Send file modes (rights), filesize and file name
            // TVarRec don't understand 'unsigned int' -> we use sprintf()
            __int64 sz = OperationProgress->AsciiTransfer ? AsciiBuf.GetSize() :
              OperationProgress->LocalSize;
            Buf.sprintf(L"C%s %lld %s",
              Rights.GetOctal().data(),
              sz,
              DestFileName.data());
            FSecureShell->SendLine(Buf.c_str());
            SCPResponse();
            // Indicate we started transferring file, we need to finish it
            // If not, it's fatal error
            OperationProgress->TransferingFile = true;

            // If we're doing ASCII transfer, this is last pass
            // so we send whole file
            /* TODO : We can't send file above 32bit size in ASCII mode! */
            if (OperationProgress->AsciiTransfer)
            {
              FTerminal->LogEvent(FORMAT(L"Sending ASCII data (%u bytes)",
                AsciiBuf.GetSize()));
              // Should be equal, just in case it's rounded (see above)
              OperationProgress->ChangeTransferSize(AsciiBuf.GetSize());
              while (!OperationProgress->IsTransferDone())
              {
                uintptr_t BlockSize = OperationProgress->TransferBlockSize();
                FSecureShell->Send(
                  reinterpret_cast<unsigned char *>(AsciiBuf.GetData() + (intptr_t)OperationProgress->TransferedSize),
                  BlockSize);
                OperationProgress->AddTransfered(BlockSize);
                if (OperationProgress->Cancel == csCancelTransfer)
                {
                  throw Exception(FMTLOAD(USER_TERMINATED));
                }
              }
            }
          }

          // At end of BINARY transfer pass, send current block
          if (!OperationProgress->AsciiTransfer)
          {
            if (!OperationProgress->TransferedSize)
            {
              FTerminal->LogEvent(FORMAT(L"Sending BINARY data (first block, %u bytes)",
                BlockBuf.GetSize()));
            }
            else if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
            {
              FTerminal->LogEvent(FORMAT(L"Sending BINARY data (%u bytes)",
                BlockBuf.GetSize()));
            }
            FSecureShell->Send(reinterpret_cast<const unsigned char *>(BlockBuf.GetData()), static_cast<int>(BlockBuf.GetSize()));
            OperationProgress->AddTransfered(BlockBuf.GetSize());
          }

          if ((OperationProgress->Cancel == csCancelTransfer) ||
              (OperationProgress->Cancel == csCancel && !OperationProgress->TransferingFile))
          {
            throw Exception(FMTLOAD(USER_TERMINATED));
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
          OperationProgress->TransferingFile = false;
          throw;
        }
        catch (EScpFileSkipped &)
        {
          // SCP protocol non-fatal error
          OperationProgress->TransferingFile = false;
          throw;
        }

        // We succeeded transferring file, from now we can handle exceptions
        // normally -> no fatal error
        OperationProgress->TransferingFile = false;
      }
      catch (Exception &E)
      {
        // EScpFileSkipped is derived from EScpSkipFile,
        // but is does not indicate file skipped by user here
        if (dynamic_cast<EScpFileSkipped *>(&E) != nullptr)
        {
          Action.Rollback(&E);
        }
        else
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }

        // Every exception during file transfer is fatal
        if (OperationProgress->TransferingFile)
        {
          FTerminal->FatalError(&E, FMTLOAD(COPY_FATAL, FileName.c_str()));
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
    }
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(Sysutils::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
  {
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(FileName, LocalFileAttrs & ~faArchive) == 0);
    )
  }

  FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory finished.", FileName.c_str()));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPDirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, intptr_t Level)
{
  DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;

  FTerminal->LogEvent(FORMAT(L"Entering directory \"%s\".", DirectoryName.c_str()));

  OperationProgress->SetFile(DirectoryName);
  UnicodeString DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(DirectoryName, false), osLocal, Level == 0);

  // Get directory attributes
  FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, DirectoryName.c_str()),
    LocalFileAttrs = FTerminal->GetLocalFileAttributes(DirectoryName);
    if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
    {
      RaiseLastOSError();
    }
  )

  UnicodeString TargetDirFull = UnixIncludeTrailingBackslash(TargetDir + DestFileName);

  UnicodeString Buf;

  /* TODO 1: maybe send filetime */

  // Send directory modes (rights), filesize and file name
  Buf = FORMAT(L"D%s 0 %s",
    CopyParam->RemoteFileRights(LocalFileAttrs).GetOctal().c_str(), DestFileName.c_str());
  FSecureShell->SendLine(Buf);
  SCPResponse();

  auto cleanup = finally([&]()
  {
    if (FTerminal->GetActive())
    {
      // Tell remote side, that we're done.
      FTerminal->LogEvent(FORMAT(L"Leaving directory \"%s\".", DirectoryName.c_str()));
      FSecureShell->SendLine(L"E");
      SCPResponse();
    }
  });
  {
    DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    TSearchRec SearchRec;
    memset(&SearchRec, 0, sizeof(SearchRec));
    bool FindOK = false;
    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
      UnicodeString Path = IncludeTrailingBackslash(DirectoryName) + L"*.*";
      FindOK = FindFirstChecked(Path.c_str(),
        FindAttrs, SearchRec) == 0;
    );

    auto cleanup = finally([&]()
    {
      FindClose(SearchRec);
    });
    {
      while (FindOK && !OperationProgress->Cancel)
      {
        UnicodeString FileName = IncludeTrailingBackslash(DirectoryName) + SearchRec.Name;
        try
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
          {
            SCPSource(FileName, nullptr, TargetDirFull, CopyParam, Params, OperationProgress, Level + 1);
          }
        }
        // Previously we caught EScpSkipFile, making error being displayed
        // even when file was excluded by mask. Now the EScpSkipFile is special
        // case without error message.
        catch (EScpFileSkipped &E)
        {
          TQueryParams QueryParams(qpAllowContinueOnError);
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()), &E,
                  qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
            {
              OperationProgress->Cancel = csCancel;
            }
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        catch (EScpSkipFile &E)
        {
          // If ESkipFile occurs, just log it and continue with next file
          SUSPEND_OPERATION (
            if (!FTerminal->HandleException(&E)) throw;
          );
        }
        FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
          FindOK = (FindNextChecked(SearchRec) == 0);
        );
      }
    }

    /* TODO : Delete also read-only directories. */
    /* TODO : Show error message on failure. */
    if (!OperationProgress->Cancel)
    {
      if (FLAGSET(Params, cpDelete))
      {
        FTerminal->RemoveLocalDirectory(DirectoryName);
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
      {
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DirectoryName, LocalFileAttrs & ~faArchive) == 0);
        )
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  bool CloseSCP = False;
  Params &= ~(cpAppend | cpResume);
  UnicodeString Options = L"";
  if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime())
  {
    Options = L"-p";
  }
  if (FTerminal->GetSessionData()->GetScp1Compatibility())
  {
    Options += L" -1";
  }

  FTerminal->LogEvent(FORMAT(L"Copying %d files/directories to local directory "
    L"\"%s\"", FilesToCopy->GetCount(), TargetDir.c_str()));
  FTerminal->LogEvent(CopyParam->GetLogStr());

  auto cleanup = finally([&]()
  {
    // In case that copying doesn't cause fatal error (ie. connection is
    // still active) but wasn't successful (exception or user termination)
    // we need to ensure, that SCP on remote side is closed
    if (FTerminal->GetActive() && (CloseSCP ||
        (OperationProgress->Cancel == csCancel) ||
        (OperationProgress->Cancel == csCancelTransfer)))
    {
      // If we get LastLine, it means that remote side 'scp' is already
      // terminated, so we need not to terminate it. There is also
      // possibility that remote side waits for confirmation, so it will hang.
      // This should not happen (hope)
      UnicodeString Line = FSecureShell->ReceiveLine();
      bool LastLineRead = IsLastLine(Line);
      if (!LastLineRead)
      {
        SCPSendError((OperationProgress->Cancel ? L"Terminated by user." : L"Exception"), true);
      }
      // Just in case, remote side already sent some more data (it's probable)
      // but we don't want to raise exception (user asked to terminate, it's not error)
      int ECParams = coOnlyReturnCode;
      if (!LastLineRead)
      {
        ECParams |= coWaitForLastLine;
      }
      ReadCommandOutput(ECParams);
    }
  });
  {
    for (intptr_t IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; ++IFile)
    {
      UnicodeString FileName = FilesToCopy->GetString(IFile);
      TRemoteFile * File = static_cast<TRemoteFile *>(FilesToCopy->GetObject(IFile));
      assert(File);

      // Filename is used for error messaging and excluding files only
      // Send in full path to allow path-based excluding
      // operation succeeded (no exception), so it's ok that
      // remote side closed SCP, but we continue with next file
      UnicodeString FullFileName = ::UnixExcludeTrailingBackslash(File->GetFullFileName());
      UnicodeString TargetDirectory = TargetDir;
      UnicodeString FileNamePath = ::ExtractFilePath(File->GetFileName());
      if (!FileNamePath.IsEmpty())
      {
        TargetDirectory = ::IncludeTrailingBackslash(TargetDirectory + FileNamePath);
        ::ForceDirectories(TargetDirectory);
      }
      try
      {
        bool Success = true; // Have to be set to True (see ::SCPSink)
        SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
          Options.c_str(), DelimitStr(FileName).c_str()));
        SkipFirstLine();

        // Filename is used for error messaging and excluding files only
        // Send in full path to allow path-based excluding
        // UnicodeString FullFileName = UnixExcludeTrailingBackslash(File->FullFileName);
        SCPSink(FullFileName, File, TargetDirectory, UnixExtractFilePath(FullFileName),
          CopyParam, Success, OperationProgress, Params, 0);
        // operation succeeded (no exception), so it's ok that
        // remote side closed SCP, but we continue with next file
        if (OperationProgress->Cancel == csRemoteAbort)
        {
          OperationProgress->Cancel = csContinue;
        }

        // Move operation -> delete file/directory afterwards
        // but only if copying succeeded
        if ((Params & cpDelete) && Success && !OperationProgress->Cancel)
        {
          try
          {
            FTerminal->SetExceptionOnFail(true);
            auto cleanup = finally([&]()
            {
              FTerminal->SetExceptionOnFail(false);
            });
            {
              FILE_OPERATION_LOOP(FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()),
                // pass full file name in FileName, in case we are not moving
                // from current directory
                FTerminal->DeleteFile(FileName, File, &Params)
              );
            }
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
            if (OperationProgress->Cancel == csCancel)
            {
              OperationProgress->Cancel = csRemoteAbort;
            }
            Success = false;
          }
        }

        OperationProgress->Finish(FileName,
          (!OperationProgress->Cancel && Success), OnceDoneOperation);
      }
      catch (...)
      {
        OperationProgress->Finish(FileName, false, OnceDoneOperation);
        CloseSCP = (OperationProgress->Cancel != csRemoteAbort);
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPError(const UnicodeString & Message, bool Fatal)
{
  SCPSendError(Message, Fatal);
  THROW_FILE_SKIPPED(nullptr, Message);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPSendError(const UnicodeString & Message, bool Fatal)
{
  unsigned char ErrorLevel = static_cast<char>(Fatal ? 2 : 1);
  FTerminal->LogEvent(FORMAT(L"Sending SCP error (%d) to remote side:",
    static_cast<int>(ErrorLevel)));
  FSecureShell->Send(&ErrorLevel, 1);
  // We don't send exact error message, because some unspecified
  // characters can terminate remote scp
  FSecureShell->SendLine(L"scp: error");
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPSink(const UnicodeString & FileName,
  const TRemoteFile * File,
  const UnicodeString & TargetDir,
  const UnicodeString & SourceDir,
  const TCopyParamType * CopyParam, bool & Success,
  TFileOperationProgressType * OperationProgress, intptr_t Params,
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

  while (!OperationProgress->Cancel)
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
    UnicodeString AbsoluteFileName = FileName;

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
        OperationProgress->Cancel = csRemoteAbort;
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

        switch (Ctrl)
        {
          case 1:
            // Error (already logged by ReceiveLine())
            THROW_FILE_SKIPPED(nullptr, FMTLOAD(REMOTE_ERROR, Line.c_str()));

          case 2:
            // Fatal error, terminate copying
            FTerminal->TerminalError(Line);
            return; // Unreachable

          case L'E': // Exit
            FSecureShell->SendNull();
            return;

          case L'T':
            unsigned long MTime, ATime;
            if (swscanf(Line.c_str(), L"%ld %*d %ld %*d",  &MTime, &ATime) == 2)
            {
              const TSessionData * Data = FTerminal->GetSessionData();
              FileData.AcTime = DateTimeToFileTime(UnixToDateTime(ATime,
                Data->GetDSTMode()), Data->GetDSTMode());
              FileData.WrTime = DateTimeToFileTime(UnixToDateTime(MTime,
                Data->GetDSTMode()), Data->GetDSTMode());
              SourceTimestamp = UnixToDateTime(MTime,
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
            FTerminal->FatalError(nullptr, FMTLOAD(SCP_INVALID_CONTROL_RECORD, Ctrl, Line.c_str()));
        }

        TFileMasks::TParams MaskParams;
        MaskParams.Modification = SourceTimestamp;

        // We reach this point only if control record was 'C' or 'D'
        try
        {
          FileData.RemoteRights.SetOctal(CutToChar(Line, L' ', True));
          // do not trim leading spaces of the filename
          __int64 TSize = StrToInt64(CutToChar(Line, L' ', False).TrimRight());
          MaskParams.Size = TSize;
          // Security fix: ensure the file ends up where we asked for it.
          // (accept only filename, not path)
          UnicodeString OnlyFileName = UnixExtractFileName(Line);
          if (Line != OnlyFileName)
          {
            FTerminal->LogEvent(FORMAT(L"Warning: Remote host set a compound pathname '%s'", Line.c_str()));
          }

          OperationProgress->SetFile(File && !File->GetIsDirectory() ?
            UnixExtractFileName(File->GetFileName()) : OnlyFileName);
          AbsoluteFileName = SourceDir + OnlyFileName;
          OperationProgress->SetTransferSize(TSize);
        }
        catch (Exception &E)
        {
          SUSPEND_OPERATION (
            FTerminal->GetLog()->AddException(&E);
          );
          SCPError(LoadStr(SCP_ILLEGAL_FILE_DESCRIPTOR), false);
        }

        // last possibility to cancel transfer before it starts
        if (OperationProgress->Cancel)
        {
          THROW_SKIP_FILE(nullptr, LoadStr(USER_TERMINATED));
        }

        bool Dir = (Ctrl == L'D');
        UnicodeString SourceFullName = SourceDir + OperationProgress->FileName;
        if (!CopyParam->AllowTransfer(SourceFullName, osRemote, Dir, MaskParams))
        {
          FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer",
            AbsoluteFileName.c_str()));
          SkipConfirmed = true;
          SCPError(L"", false);
        }

        UnicodeString DestFileName =
          IncludeTrailingBackslash(TargetDir) +
          CopyParam->ChangeFileName(OperationProgress->FileName, osRemote,
            Level == 0);

        FileData.LocalFileAttrs = FTerminal->GetLocalFileAttributes(DestFileName);
        // If getting attrs fails, we suppose, that file/folder doesn't exists
        FileData.Exists = (FileData.LocalFileAttrs != INVALID_FILE_ATTRIBUTES);
        if (Dir)
        {
          if (FileData.Exists && !(FileData.LocalFileAttrs & faDirectory))
          {
            SCPError(FMTLOAD(NOT_DIRECTORY_ERROR, DestFileName.c_str()), false);
          }

          if (!FileData.Exists)
          {
            FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFileName.c_str()),
              if (!ForceDirectories(DestFileName))
              {
                RaiseLastOSError();
              }
            );
            /* SCP: can we set the timestamp for directories ? */
          }
          UnicodeString FullFileName = SourceDir + OperationProgress->FileName;
          SCPSink(FullFileName, nullptr, DestFileName, UnixIncludeTrailingBackslash(FullFileName),
            CopyParam, Success, OperationProgress, Params, Level + 1);
          continue;
        }
        else if (Ctrl == L'C')
        {
          TDownloadSessionAction Action(FTerminal->GetActionLog());
          Action.FileName(AbsoluteFileName);

          try
          {
            HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
            std::auto_ptr<TStream> FileStream;

            /* TODO 1 : Turn off read-only attr */

            auto cleanup = finally([&]()
            {
              if (LocalFileHandle)
              {
                ::CloseHandle(LocalFileHandle);
              }
              FileStream.reset();
            });
            {
              try
              {
                if (::FileExists(DestFileName))
                {
                  __int64 MTime;
                  TOverwriteFileParams FileParams;
                  FileParams.SourceSize = OperationProgress->TransferSize;
                  FileParams.SourceTimestamp = SourceTimestamp;
                  FTerminal->OpenLocalFile(DestFileName, GENERIC_READ,
                    nullptr, nullptr, nullptr, &MTime, nullptr,
                    &FileParams.DestSize);
                  FileParams.DestTimestamp = UnixToDateTime(MTime,
                    FTerminal->GetSessionData()->GetDSTMode());

                  uintptr_t Answer =
                    ConfirmOverwrite(
                      OperationProgress->FileName, osLocal,
                      &FileParams, CopyParam, Params, OperationProgress);

                  switch (Answer)
                  {
                    case qaCancel:
                      OperationProgress->Cancel = csCancel; // continue on next case
                      // FALLTHROUGH
                    case qaNo:
                      SkipConfirmed = true;
                      EXCEPTION;
                  }
                }

                Action.Destination(DestFileName);

                if (!FTerminal->CreateLocalFile(DestFileName, OperationProgress,
                       &LocalFileHandle, FLAGSET(Params, cpNoConfirmation)))
                {
                  SkipConfirmed = true;
                  EXCEPTION;
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
              OperationProgress->TransferingFile = true;

              // Suppose same data size to transfer as to write
              // (not true with ASCII transfer)
              OperationProgress->SetLocalSize(OperationProgress->TransferSize);

              // Will we use ASCII of BINARY file transfer?
              OperationProgress->SetAsciiTransfer(
                CopyParam->UseAsciiTransfer(SourceFullName, osRemote, MaskParams));
              FTerminal->LogEvent(UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
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

                  FSecureShell->Receive(reinterpret_cast<unsigned char *>(BlockBuf.GetData()), BlockBuf.GetSize());
                  OperationProgress->AddTransfered(BlockBuf.GetSize());

                  if (OperationProgress->AsciiTransfer)
                  {
                    __int64 PrevBlockSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetSessionData()->GetEOLType(),
                      FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                    OperationProgress->SetLocalSize(
                      OperationProgress->LocalSize - PrevBlockSize + BlockBuf.GetSize());
                  }

                  // This is crucial, if it fails during file transfer, it's fatal error
                  FILE_OPERATION_LOOP_EX (false, FMTLOAD(WRITE_ERROR, DestFileName.c_str()),
                    BlockBuf.WriteToStream(FileStream.get(), static_cast<unsigned int>(BlockBuf.GetSize()));
                  );

                  OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

                  if (OperationProgress->Cancel == csCancelTransfer)
                  {
                    throw Exception(FMTLOAD(USER_TERMINATED));
                  }
                }
                while (!OperationProgress->IsLocallyDone() || !
                    OperationProgress->IsTransferDone());
              }
              catch (Exception &E)
              {
                // Every exception during file transfer is fatal
                FTerminal->FatalError(&E,
                  FMTLOAD(COPY_FATAL, OperationProgress->FileName.c_str()));
              }

              OperationProgress->TransferingFile = false;

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
              catch (EScpFileSkipped &)
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

          if (FileData.LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
          {
            FileData.LocalFileAttrs = faArchive;
          }
          DWORD NewAttrs = CopyParam->LocalFileAttrs(FileData.RemoteRights);
          if ((NewAttrs & FileData.LocalFileAttrs) != NewAttrs)
          {
            FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFileName.c_str()),
              THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DestFileName, FileData.LocalFileAttrs | NewAttrs) == 0);
            );
          }
        }
      }
    }
    catch (EScpFileSkipped &E)
    {
      if (!SkipConfirmed)
      {
        SUSPEND_OPERATION (
          TQueryParams QueryParams(qpAllowContinueOnError);
          if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, AbsoluteFileName.c_str()),
                &E, qaOK | qaAbort, &QueryParams, qtError) == qaAbort)
          {
            OperationProgress->Cancel = csCancel;
          }
          FTerminal->GetLog()->AddException(&E);
        );
      }
      // this was inside above condition, but then transfer was considered
      // successful, even when for example user refused to overwrite file
      Success = false;
    }
    catch (EScpSkipFile &E)
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
