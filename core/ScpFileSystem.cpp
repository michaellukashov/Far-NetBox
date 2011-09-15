//---------------------------------------------------------------------------
#include "stdafx.h"
#include "ScpFileSystem.h"

#include "Terminal.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "TextsCore.h"
#include "SecureShell.h"

#include <stdio.h>
//---------------------------------------------------------------------------
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
#define LAST_LINE L"WinSCP: this is end-of-file"
#define FIRST_LINE L"WinSCP: this is begin-of-file"
extern const TCommandType DefaultCommandSet[];

#define NationalVarCount 10
extern const wchar_t NationalVars[NationalVarCount][15];

#define CHECK_CMD assert((Cmd >=0) && (Cmd <= MaxShellCommand))

class TSessionData;
//---------------------------------------------------------------------------
class TCommandSet
{
private:
  TCommandType CommandSet[ShellCommandCount];
  TSessionData * FSessionData;
  std::wstring FReturnVar;
public:
  TCommandSet(TSessionData *aSessionData);
  void Default();
  void CopyFrom(TCommandSet * Source);
  std::wstring Command(TFSCommand Cmd, ...);
  TStrings * CreateCommandList();
  std::wstring FullCommand(TFSCommand Cmd, ...);
  static std::wstring ExtractCommand(std::wstring Command);
  // __property int MaxLines[TFSCommand Cmd]  = { read=GetMaxLines};
  int GetMaxLines(TFSCommand Cmd);
  // __property int MinLines[TFSCommand Cmd]  = { read=GetMinLines };
  int GetMinLines(TFSCommand Cmd);
  // __property bool ModifiesFiles[TFSCommand Cmd]  = { read=GetModifiesFiles };
  bool GetModifiesFiles(TFSCommand Cmd);
  // __property bool ChangesDirectory[TFSCommand Cmd]  = { read=GetChangesDirectory };
  bool GetChangesDirectory(TFSCommand Cmd);
  // __property bool OneLineCommand[TFSCommand Cmd]  = { read=GetOneLineCommand };
  bool GetOneLineCommand(TFSCommand Cmd);
  // __property std::wstring Commands[TFSCommand Cmd]  = { read=GetCommands, write=SetCommands };
  std::wstring GetCommand(TFSCommand Cmd);
  void SetCommand(TFSCommand Cmd, std::wstring value);
  // __property std::wstring FirstLine = { read = GetFirstLine };
  std::wstring GetFirstLine();
  // __property bool InteractiveCommand[TFSCommand Cmd] = { read = GetInteractiveCommand };
  bool GetInteractiveCommand(TFSCommand Cmd);
  // __property std::wstring LastLine  = { read=GetLastLine };
  std::wstring GetLastLine();
  //  __property TSessionData * SessionData  = { read=FSessionData, write=FSessionData };
  TSessionData * GetSessionData() { return FSessionData; }
  void SetSessionData(TSessionData * value) { FSessionData = value; }
  // __property std::wstring ReturnVar  = { read=GetReturnVar, write=FReturnVar };
  std::wstring GetReturnVar();
  void SetReturnVar(std::wstring value) { FReturnVar = value; }
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
  memcpy(&CommandSet, Source->CommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
void TCommandSet::Default()
{
  assert(sizeof(CommandSet) == sizeof(DefaultCommandSet));
  memcpy(&CommandSet, &DefaultCommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
int TCommandSet::GetMaxLines(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].MaxLines;
}
//---------------------------------------------------------------------------
int TCommandSet::GetMinLines(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].MinLines;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetModifiesFiles(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].ModifiesFiles;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetChangesDirectory(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].ChangesDirectory;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetInteractiveCommand(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].InteractiveCommand;
}
//---------------------------------------------------------------------------
bool TCommandSet::GetOneLineCommand(TFSCommand /*Cmd*/)
{
  //CHECK_CMD;
  // #56: we send "echo last line" from all commands on same line
  // just as it was in 1.0
  return true; //CommandSet[Cmd].OneLineCommand;
}
//---------------------------------------------------------------------------
void TCommandSet::SetCommand(TFSCommand Cmd, std::wstring value)
{
  CHECK_CMD;
  wcscpy((wchar_t *)CommandSet[Cmd].Command, value.substr(1, MaxCommandLen - 1).c_str());
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::GetCommand(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].Command;
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::Command(TFSCommand Cmd, ...)
{
  va_list args;
  va_start(args, Cmd);
  if (args) return FORMAT(GetCommand(Cmd).c_str(), args);
    else return GetCommand(Cmd);
  va_end(args);
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::FullCommand(TFSCommand Cmd, ...)
{
  std::wstring Separator;
  if (GetOneLineCommand(Cmd)) Separator = L" ; ";
    else Separator = L"\n";
  va_list args;
  va_start(args, Cmd);
  std::wstring Line = Command(Cmd, args);
  va_end(args);
  std::wstring LastLineCmd =
    Command(fsLastLine, GetLastLine(), GetReturnVar().c_str());
  std::wstring FirstLineCmd;
  if (GetInteractiveCommand(Cmd))
    FirstLineCmd = Command(fsFirstLine, GetFirstLine()) + Separator;

  std::wstring Result;
  if (!Line.empty())
    Result = FORMAT(L"%s%s%s%s", FirstLineCmd.c_str(), Line.c_str(), Separator.c_str(), LastLineCmd.c_str());
  else
    Result = FORMAT(L"%s%s", FirstLineCmd, LastLineCmd);
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::GetFirstLine()
{
  return FIRST_LINE;
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::GetLastLine()
{
  return LAST_LINE;
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::GetReturnVar()
{
  assert(GetSessionData());
  if (!FReturnVar.empty()) return std::wstring(L"$") + FReturnVar;
    else
  if (GetSessionData()->GetDetectReturnVar()) return L"0";
    else return std::wstring(L"$") + GetSessionData()->GetReturnVar();
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::ExtractCommand(std::wstring Command)
{
  int P = Command.find_first_of(L" ");
  if (P > 0)
  {
    Command.resize(P-1);
  }
  return Command;
}
//---------------------------------------------------------------------------
TStrings * TCommandSet::CreateCommandList()
{
  TStrings * CommandList = new TStringList();
  for (int Index = 0; Index < ShellCommandCount; Index++)
  {
    std::wstring Cmd = GetCommand((TFSCommand)Index);
    if (!Cmd.empty())
    {
      Cmd = ExtractCommand(Cmd);
      if ((Cmd != L"%s") && (CommandList->IndexOf(Cmd.c_str()) < 0))
        CommandList->Add(Cmd);
    }
  }
  return CommandList;
}
//===========================================================================
TSCPFileSystem::TSCPFileSystem(TTerminal * ATerminal, TSecureShell * SecureShell):
  TCustomFileSystem(ATerminal)
{
  FSecureShell = SecureShell;
  FCommandSet = new TCommandSet(FTerminal->GetSessionData());
  FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
  FOutput = new TStringList();
  FProcessingCommand = false;
  FOnCaptureOutput = NULL;

  FFileSystemInfo.ProtocolBaseName = L"SCP";
  FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
  // capabilities of SCP protocol are fixed
  for (int Index = 0; Index < fcCount; Index++)
  {
    FFileSystemInfo.IsCapable[Index] = IsCapable((TFSCapability)Index);
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
bool TSCPFileSystem::GetActive()
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
  if (FFileSystemInfo.AdditionalInfo.empty() && Retrieve)
  {
    std::wstring UName;
    FTerminal->SetExceptionOnFail(true);
    try
    {
      try
      {
        AnyCommand(L"uname -a", NULL);
        for (int Index = 0; Index < GetOutput()->GetCount(); Index++)
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
    catch(...)
    {
      FTerminal->SetExceptionOnFail (false);
    }

    FFileSystemInfo.RemoteSystem = UName;
  }

  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::TemporaryTransferFile(const std::wstring & /*FileName*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::GetStoredCredentialsTried()
{
  return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
std::wstring TSCPFileSystem::GetUserName()
{
  return FSecureShell->GetUserName();
}
//---------------------------------------------------------------------------
void TSCPFileSystem::Idle()
{
  // Keep session alive
  if ((FTerminal->GetSessionData()->GetPingType ()!= ptOff) &&
      (Now() - FSecureShell->GetLastDataSent()> FTerminal->GetSessionData()->GetPingIntervalDT()))
  {
    if ((FTerminal->GetSessionData()->GetPingType() == ptDummyCommand) &&
        FSecureShell->GetReady())
    {
      if (!FProcessingCommand)
      {
        ExecCommand(fsNull, 0, NULL);
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
std::wstring TSCPFileSystem::AbsolutePath(std::wstring Path, bool /*Local*/)
{
  return ::AbsolutePath(GetCurrentDirectory(), Path);
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsCapable(int Capability) const
{
  assert(FTerminal);
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
std::wstring TSCPFileSystem::DelimitStr(std::wstring Str)
{
  if (!Str.empty())
  {
    Str = ::DelimitStr(Str, L"\\`$\"");
    if (Str[1] == L'-') Str = L"./" + Str;
  }
  return Str;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::EnsureLocation()
{
  if (!FCachedDirectoryChange.empty())
  {
    FTerminal->LogEvent(FORMAT(L"Locating to cached directory \"%s\".",
      (FCachedDirectoryChange)));
    std::wstring Directory = FCachedDirectoryChange;
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
      // false always (currentdirectory is already set to cached directory),
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
void TSCPFileSystem::SendCommand(const std::wstring Cmd)
{
  EnsureLocation();

  std::wstring Line;
  FSecureShell->ClearStdError();
  FReturnCode = 0;
  FOutput->Clear();
  // We suppose, that 'Cmd' already contains command that ensures,
  // that 'LastLine' will be printed
  FSecureShell->SendLine(Cmd);
  FProcessingCommand = true;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsTotalListingLine(const std::wstring Line)
{
  // On some hosts there is not "total" but "totalt". What's the reason??
  // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
  return !::AnsiCompareIC(Line.substr(1, 5), L"total");
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::RemoveLastLine(std::wstring & Line,
    int & ReturnCode, std::wstring LastLine)
{
  bool IsLastLine = false;
  if (LastLine.empty()) LastLine = LAST_LINE;
  // #55: fixed so, even when last line of command output does not
  // contain CR/LF, we can recognize last line
  int Pos = Line.find_first_of(LastLine);
  if (Pos)
  {
    // 2003-07-14: There must be nothing after return code number to
    // consider string as last line. This fixes bug with 'set' command
    // in console window
    std::wstring ReturnCodeStr = Line.substr(Pos + LastLine.size() + 1,
      Line.size() - Pos + LastLine.size());
    if (TryStrToInt(ReturnCodeStr, ReturnCode))
    {
      IsLastLine = true;
      Line.resize(Pos - 1);
    }
  }
  return IsLastLine;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsLastLine(std::wstring & Line)
{
  bool Result = false;
  try
  {
    Result = RemoveLastLine(Line, FReturnCode, FCommandSet->GetLastLine());
  }
  catch (std::exception &E)
  {
    FTerminal->TerminalError(&E, LoadStr(CANT_DETECT_RETURN_CODE));
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SkipFirstLine()
{
  std::wstring Line = FSecureShell->ReceiveLine();
  if (Line != FCommandSet->GetFirstLine())
  {
    FTerminal->TerminalError(NULL, FMTLOAD(FIRST_LINE_EXPECTED, Line.c_str()));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadCommandOutput(int Params, const std::wstring * Cmd)
{
  try
  {
    if (Params & coWaitForLastLine)
    {
      std::wstring Line;
      bool IsLast;
      unsigned int Total = 0;
      // #55: fixed so, even when last line of command output does not
      // contain CR/LF, we can recognize last line
      do
      {
        Line = FSecureShell->ReceiveLine();
        IsLast = IsLastLine(Line);
        if (!IsLast || !Line.empty())
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
      std::wstring Message = FSecureShell->GetStdError();
      if ((Params & coExpectNoOutput) && FOutput->GetCount())
      {
        if (!Message.empty()) Message += L"\n";
        Message += FOutput->GetText();
      }
      while (!Message.empty() && (::LastDelimiter(Message, L"\n\r") == Message.size()))
      {
        Message.resize(Message.size() - 1);
      }

      bool WrongReturnCode =
        (GetReturnCode() > 1) || (GetReturnCode() == 1 && !(Params & coIgnoreWarnings));

      if (Params & coOnlyReturnCode && WrongReturnCode)
      {
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, GetReturnCode()));
      }
        else
      if (!(Params & coOnlyReturnCode) &&
          ((!Message.empty() && ((FOutput->GetCount() == 0) || !(Params & coIgnoreWarnings))) ||
           WrongReturnCode))
      {
        assert(Cmd != NULL);
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED, *Cmd, GetReturnCode(), Message.c_str()));
      }
    }
  }
  catch(...)
  {
    FProcessingCommand = false;
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ExecCommand(const std::wstring & Cmd, int Params,
  const std::wstring & CmdString)
{
  if (Params < 0) Params = ecDefault;
  if (FTerminal->GetUseBusyCursor())
  {
    Busy(true);
  }
  try
  {
    SendCommand(Cmd);

    int COParams = coWaitForLastLine;
    if (Params & ecRaiseExcept) COParams |= coRaiseExcept;
    if (Params & ecIgnoreWarnings) COParams |= coIgnoreWarnings;
    if (Params & ecReadProgress) COParams |= coReadProgress;
    ReadCommandOutput(COParams, &CmdString);
  }
  catch(...)
  {
    if (FTerminal->GetUseBusyCursor())
    {
      Busy(false);
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ExecCommand(TFSCommand Cmd, int Params, ...)
{
  if (Params < 0) Params = ecDefault;
  va_list args;
  va_start(args, Params);
  std::wstring FullCommand = FCommandSet->FullCommand(Cmd, args);
  std::wstring Command = FCommandSet->Command(Cmd, args);
  ExecCommand(FullCommand, Params, Command);
  va_end(args);
  if (Params & ecRaiseExcept)
  {
    int MinL = FCommandSet->GetMinLines(Cmd);
    int MaxL = FCommandSet->GetMaxLines(Cmd);
    if (((MinL >= 0) && (MinL > FOutput->GetCount())) ||
        ((MaxL >= 0) && (MaxL > FOutput->GetCount())))
    {
      FTerminal->TerminalError(::FmtLoadStr(INVALID_OUTPUT_ERROR,
        FullCommand.c_str(), GetOutput()->GetText().c_str()));
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TSCPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::DoStartup()
{
  // SkipStartupMessage and DetectReturnVar must succeed,
  // otherwise session is to be closed.
  FTerminal->SetExceptionOnFail (true);
  SkipStartupMessage();
  if (FTerminal->GetSessionData()->GetDetectReturnVar()) DetectReturnVar();
  FTerminal->SetExceptionOnFail (false);

  #define COND_OPER(OPER) if (FTerminal->GetSessionData()->Get##OPER()) OPER()
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
    ExecCommand(fsNull, 0, NULL);
  }
  catch (std::exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(SKIP_STARTUP_MESSAGE_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::LookupUsersGroups()
{
  ExecCommand(fsLookupUsersGroups);
  FTerminal->FUsers.Clear();
  FTerminal->FGroups.Clear();
  if (FOutput->GetCount() > 0)
  {
    std::wstring Groups = FOutput->GetString(0);
    while (!Groups.empty())
    {
      std::wstring NewGroup = CutToChar(Groups, ' ', false);
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
    std::wstring ReturnVars[2] = { L"status", L"?" };
    std::wstring NewReturnVar = L"";
    FTerminal->LogEvent(L"Detecting variable containing return code of last command.");
    for (int Index = 0; Index < 2; Index++)
    {
      bool Success = true;

      try
      {
        FTerminal->LogEvent(FORMAT(L"Trying \"$%s\".", ReturnVars[Index]));
        ExecCommand(fsVarValue, 0, ReturnVars[Index].c_str());
        if ((GetOutput()->GetCount() != 1) || (StrToIntDef(GetOutput()->GetString(0), 256) > 255))
        {
          FTerminal->LogEvent(L"The response is not numerical exit code");
          Abort();
        }
      }
      catch (EFatal &E)
      {
        // if fatal error occurs, we need to exit ...
        throw;
      }
      catch (std::exception &E)
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

    if (NewReturnVar.empty())
    {
      Abort();
    }
      else
    {
      FCommandSet->SetReturnVar (NewReturnVar);
      FTerminal->LogEvent(FORMAT(L"Return code variable \"%s\" selected.",
        FCommandSet->GetReturnVar().c_str()));
    }
  }
  catch (std::exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(DETECT_RETURNVAR_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ClearAlias(std::wstring Alias)
{
  if (!Alias.empty())
  {
    // this command usually fails, because there will never be
    // aliases on all commands -> see last false parametr
    ExecCommand(fsUnalias, 0, Alias.c_str(), false);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ClearAliases()
{
  try
  {
    FTerminal->LogEvent(L"Clearing all aliases.");
    ClearAlias(TCommandSet::ExtractCommand(FTerminal->GetSessionData()->GetListingCommand()));
    TStrings * CommandList = FCommandSet->CreateCommandList();
    try
    {
      for (int Index = 0; Index < CommandList->GetCount(); Index++)
      {
        ClearAlias(CommandList->GetString(Index));
      }
    }
    catch(...)
    {
      delete CommandList;
    }
  }
  catch (std::exception &E)
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
    for (int Index = 0; Index < NationalVarCount; Index++)
    {
      ExecCommand(fsUnset, 0, NationalVars[Index], false);
    }
  }
  catch (std::exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNSET_NATIONAL_ERROR));
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.empty())
  {
    ExecCommand(fsCurrentDirectory);
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
  ExecCommand(fsHomeDirectory);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::AnnounceFileListOperation()
{
  // noop
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ChangeDirectory(const std::wstring Directory)
{
  std::wstring ToDir;
  if (!Directory.empty() &&
      ((Directory[1] != L'~') || (Directory.substr(1, 2) == L"~ ")))
  {
    ToDir = L"\"" + DelimitStr(Directory) + L"\"";
  }
  else
  {
    ToDir = DelimitStr(Directory);
  }
  ExecCommand(fsChangeDirectory, 0, ToDir.c_str());
  FCachedDirectoryChange = L"";
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CachedChangeDirectory(const std::wstring Directory)
{
  FCachedDirectoryChange = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList);
  // emtying file list moved before command execution
  FileList->Clear();

  bool Again;

  do
  {
    Again = false;
    try
    {
      int Params = ecDefault | ecReadProgress |
        FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
      const wchar_t * Options =
        ((FLsFullTime == asAuto) || (FLsFullTime == asOn)) ? FullTimeOption : L"";
      bool ListCurrentDirectory = (FileList->GetDirectory() == FTerminal->GetCurrentDirectory());
      if (ListCurrentDirectory)
      {
        FTerminal->LogEvent(L"Listing current directory.");
        ExecCommand(fsListCurrentDirectory,
          0, FTerminal->GetSessionData()->GetListingCommand(), Options, Params);
      }
        else
      {
        FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".",
          (FileList->GetDirectory())));
        ExecCommand(fsListDirectory,
          0, FTerminal->GetSessionData()->GetListingCommand(), Options,
            DelimitStr(FileList->GetDirectory()),
          Params);
      }

      TRemoteFile * File;

      // If output is not empty, we have succesfully got file listing,
      // otherwise there was an error, in case it was "permission denied"
      // we try to get at least parent directory (see "else" statement below)
      if (FOutput->GetCount() > 0)
      {
        // Copy LS command output, because eventual symlink analysis would
        // modify FTerminal->Output
        TStringList * OutputCopy = new TStringList();
        try
        {
          OutputCopy->Assign(FOutput);

          // delete leading "total xxx" line
          // On some hosts there is not "total" but "totalt". What's the reason??
          // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
          if (IsTotalListingLine(OutputCopy->GetString(0)))
          {
            OutputCopy->Delete(0);
          }

          for (int Index = 0; Index < OutputCopy->GetCount(); Index++)
          {
            File = CreateRemoteFile(OutputCopy->GetString(Index));
            FileList->AddFile(File);
          }
        }
        catch(...)
        {
          delete OutputCopy;
        }
      }
      else
      {
        bool Empty;
        if (ListCurrentDirectory)
        {
          // Empty file list -> probably "permision denied", we
          // at least get link to parent directory ("..")
          FTerminal->ReadFile(
            UnixIncludeTrailingBackslash(FTerminal->FFiles->GetDirectory()) +
              PARENTDIRECTORY, File);
          Empty = (File == NULL);
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
          throw ExtException(FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory().c_str()));
        }
      }

      if (FLsFullTime == asAuto)
      {
          FTerminal->LogEvent(
            FORMAT(L"Directory listing with %s succeed, next time all errors during "
              L"directory listing will be displayed immediatelly.",
              FullTimeOption));
          FLsFullTime = asOn;
      }
    }
    catch(std::exception & E)
    {
      if (FTerminal->GetActive())
      {
        if (FLsFullTime == asAuto)
        {
          FTerminal->FLog->AddException(&E);
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
void TSCPFileSystem::ReadFile(const std::wstring FileName,
  TRemoteFile *& File)
{
  CustomReadFile(FileName, File, NULL);
}
//---------------------------------------------------------------------------
TRemoteFile * TSCPFileSystem::CreateRemoteFile(
  const std::wstring & ListingStr, TRemoteFile * LinkedByFile)
{
  TRemoteFile * File = new TRemoteFile(LinkedByFile);
  try
  {
    File->SetTerminal (FTerminal);
    File->SetListingStr (ListingStr);
    File->ShiftTime(FTerminal->GetSessionData()->GetTimeDifference());
    File->Complete();
  }
  catch(...)
  {
    delete File;
    throw;
  }

  return File;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CustomReadFile(const std::wstring FileName,
  TRemoteFile *& File, TRemoteFile * ALinkedByFile)
{
  File = NULL;
  int Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  const wchar_t * Options = (FLsFullTime == asOn) ? FullTimeOption : L"";
  ExecCommand(fsListFile,
    Params, FTerminal->GetSessionData()->GetListingCommand(), Options, DelimitStr(FileName).c_str());
  if (FOutput->GetCount())
  {
    int LineIndex = 0;
    if (IsTotalListingLine(FOutput->GetString(LineIndex)) && FOutput->GetCount() > 1)
    {
      LineIndex++;
    }

    File = CreateRemoteFile(FOutput->GetString(LineIndex), ALinkedByFile);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::DeleteFile(const std::wstring FileName,
  const TRemoteFile * File, int Params, TRmSessionAction & Action)
{
  USEDPARAM(File);
  USEDPARAM(Params);
  Action.Recursive();
  assert(FLAGCLEAR(Params, dfNoRecursive) || (File && File->GetIsSymLink()));
  ExecCommand(fsDeleteFile, 0, DelimitStr(FileName));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::RenameFile(const std::wstring FileName,
  const std::wstring NewName)
{
  ExecCommand(fsRenameFile, 0, DelimitStr(FileName), DelimitStr(NewName));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CopyFile(const std::wstring FileName,
  const std::wstring NewName)
{
  ExecCommand(fsCopyFile, 0, DelimitStr(FileName), DelimitStr(NewName));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CreateDirectory(const std::wstring DirName)
{
  ExecCommand(fsCreateDirectory, 0, DelimitStr(DirName));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CreateLink(const std::wstring FileName,
  const std::wstring PointTo, bool Symbolic)
{
  ExecCommand(fsCreateLink, 0, 
    Symbolic ? L"-s" : L"", DelimitStr(PointTo), DelimitStr(FileName));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ChangeFileToken(const std::wstring & DelimitedName,
  const TRemoteToken & Token, TFSCommand Cmd, const std::wstring & RecursiveStr)
{
  std::wstring Str;
  if (Token.GetIDValid())
  {
    Str = IntToStr(Token.GetID());
  }
  else if (Token.GetNameValid())
  {
    Str = Token.GetName();
  }

  if (!Str.empty())
  {
    ExecCommand(Cmd, 0, RecursiveStr, Str, DelimitedName);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::ChangeFileProperties(const std::wstring FileName,
  const TRemoteFile * File, const TRemoteProperties * Properties,
  TChmodSessionAction & Action)
{
  assert(Properties);
  bool IsDirectory = File && File->GetIsDirectory();
  bool Recursive = Properties->Recursive && IsDirectory;
  std::wstring RecursiveStr = Recursive ? L"-R" : L"";

  std::wstring DelimitedName = DelimitStr(FileName);
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
    // options. Otherwise we have to add X after recusive command
    if (!Recursive && IsDirectory && Properties->AddXToDirectories)
      Rights.AddExecute();

    Action.Rights(Rights);
    if (Recursive)
    {
      Action.Recursive();
    }

    if ((Rights.GetNumberSet() | Rights.GetNumberUnset()) != TRights::rfNo)
    {
      ExecCommand(fsChangeMode,
        0, RecursiveStr, Rights.GetSimplestStr(), DelimitedName);
    }

    // if file is directory and we do recursive mode settings with
    // add-x-to-directories option on, add those X
    if (Recursive && IsDirectory && Properties->AddXToDirectories)
    {
      Rights.AddExecute();
      ExecCommand(fsChangeMode,
        0, L"", Rights.GetSimplestStr(), DelimitedName);
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
void TSCPFileSystem::CalculateFilesChecksum(const std::wstring & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CustomCommandOnFile(const std::wstring FileName,
    const TRemoteFile * File, std::wstring Command, int Params,
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
    // FIXME FTerminal->ProcessDirectory(FileName, FTerminal->CustomCommandOnFile,
      // &AParams);
  }

  if (!Dir || (Params & ccApplyToDirectories))
  {
    TCustomCommandData Data(FTerminal);
    std::wstring Cmd = TRemoteCustomCommand(
      Data, FTerminal->GetCurrentDirectory(), FileName, L"").
      Complete(Command, true);

    AnyCommand(Cmd, OutputEvent);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CaptureOutput(const std::wstring & AddedLine, bool StdError)
{
  int ReturnCode;
  std::wstring Line = AddedLine;
  if (StdError ||
      !RemoveLastLine(Line, ReturnCode) ||
      !Line.empty())
  {
    assert(FOnCaptureOutput != NULL);
    // FIXME FOnCaptureOutput(Line, StdError);
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::AnyCommand(const std::wstring Command,
  TCaptureOutputEvent OutputEvent)
{
  assert(FSecureShell->GetOnCaptureOutput() == NULL);
  if (OutputEvent != NULL)
  {
    // FSecureShell->SetOnCaptureOutput(CaptureOutput);
    FOnCaptureOutput = OutputEvent;
  }

  try
  {
    ExecCommand(fsAnyCommand, 0, Command,
      ecDefault | ecIgnoreWarnings);
  }
  catch(...)
  {
    FOnCaptureOutput = NULL;
    FSecureShell->SetOnCaptureOutput (NULL);
  }
}
//---------------------------------------------------------------------------
std::wstring TSCPFileSystem::FileUrl(const std::wstring FileName)
{
  return FTerminal->FileUrl(L"scp", FileName);
}
//---------------------------------------------------------------------------
TStrings * TSCPFileSystem::GetFixedPaths()
{
  return NULL;
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SpaceAvailable(const std::wstring Path,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPResponse(bool * GotLastLine)
{
  // Taken from scp.c response() and modified

  char Resp;
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
      std::wstring Msg = FSecureShell->ReceiveLine();
      // FIXME std::wstring Line = std::wstring(Resp) + Msg;
      std::wstring Line = Msg;
      if (IsLastLine(Line))
      {
        if (GotLastLine != NULL)
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
          if (GotLastLine != NULL)
          {
            *GotLastLine = true;
          }
          throw;
        }
      }
        else
      if (Resp == 1)
      {
        FTerminal->LogEvent(L"SCP remote side error (1):");
      }
        else
      {
        FTerminal->LogEvent(L"SCP remote side fatal error (2):");
      }

      if (Resp == 1)
      {
        THROW_FILE_SKIPPED(NULL, Msg);
      }
        else
      {
        THROW_SCP_ERROR(NULL, Msg);
      }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  // scp.c: source(), toremote()
  assert(FilesToCopy && OperationProgress);

  Params &= ~(cpAppend | cpResume);
  std::wstring Options = L"";
  bool CheckExistence = UnixComparePaths(TargetDir, FTerminal->GetCurrentDirectory()) &&
    (FTerminal->FFiles != NULL) && FTerminal->FFiles->GetLoaded();
  bool CopyBatchStarted = false;
  bool Failed = true;
  bool GotLastLine = false;

  std::wstring TargetDirFull = UnixIncludeTrailingBackslash(TargetDir);

  if (CopyParam->GetPreserveRights()) Options = L"-p";
  if (FTerminal->GetSessionData()->GetScp1Compatibility()) Options += L" -1";

  SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
    0, Options, DelimitStr(UnixExcludeTrailingBackslash(TargetDir))));
  SkipFirstLine();

  try
  {
    try
    {
      SCPResponse(&GotLastLine);

      // This can happen only if SCP command is not executed and return code is 0
      // It has never happened to me (return code is usually 127)
      if (GotLastLine)
      {
        throw std::exception("");
      }
    }
    catch(std::exception & E)
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

    for (int IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; IFile++)
    {
      std::wstring FileName = FilesToCopy->GetString(IFile);
      bool CanProceed;

      std::wstring FileNameOnly =
        CopyParam->ChangeFileName(ExtractFileName(FileName, true), osLocal, true);

      if (CheckExistence)
      {
        // previously there was assertion on FTerminal->FFiles->Loaded, but it
        // fails for scripting, if 'ls' is not issued before.
        // formally we should call CheckRemoteFile here but as checking is for
        // free here (almost) ...
        TRemoteFile * File = FTerminal->FFiles->FindFile(FileNameOnly);
        if (File != NULL)
        {
          int Answer;
          if (File->GetIsDirectory())
          {
            std::wstring Message = FMTLOAD(DIRECTORY_OVERWRITE, FileNameOnly.c_str());
            TQueryParams QueryParams(qpNeverAskAgainCheck);
            SUSPEND_OPERATION
            (
              Answer = FTerminal->ConfirmFileOverwrite(
                FileNameOnly /*not used*/, NULL,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll,
                &QueryParams, osRemote, Params, OperationProgress, Message);
            );
          }
          else
          {
            __int64 MTime;
            TOverwriteFileParams FileParams;
            FTerminal->OpenLocalFile(FileName, GENERIC_READ,
              NULL, NULL, NULL, &MTime, NULL,
              &FileParams.SourceSize);
            FileParams.SourceTimestamp = UnixToDateTime(MTime,
              FTerminal->GetSessionData()->GetDSTMode());
            FileParams.DestSize = File->GetSize();
            FileParams.DestTimestamp = File->GetModification();

            TQueryButtonAlias Aliases[1];
            Aliases[0].Button = qaAll;
            Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
            TQueryParams QueryParams(qpNeverAskAgainCheck);
            QueryParams.Aliases = Aliases;
            QueryParams.AliasesCount = LENOF(Aliases);
            SUSPEND_OPERATION
            (
              Answer = FTerminal->ConfirmFileOverwrite(
                FileNameOnly, &FileParams,
                qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
                &QueryParams, osRemote, Params, OperationProgress);
            );
          }

          switch (Answer)
          {
            case qaYes:
              CanProceed = true;
              break;

            case qaCancel:
              if (!OperationProgress->Cancel) OperationProgress->Cancel = csCancel;
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

          if (DirectoryExists(FileName))
          {
            FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir)+
              FileNameOnly, true);
          }
        }

        try
        {
          SCPSource(FileName, TargetDirFull,
            CopyParam, Params, OperationProgress, 0);
          OperationProgress->Finish(FileName, true, OnceDoneOperation);
        }
        catch (EScpFileSkipped &E)
        {
          TQueryParams Params(qpAllowContinueOnError);
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()),
                &E,
              qaOK | qaAbort, &Params, qtError) == qaAbort)
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
  catch(...)
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
          };
          /* TODO 1 : Show stderror to user? */
          FSecureShell->ClearStdError();

          ReadCommandOutput(coExpectNoOutput | coWaitForLastLine | coOnlyReturnCode |
            (Failed ? 0 : coRaiseExcept));
        }
      }
      catch (std::exception &E)
      {
        // Only log error message (it should always succeed, but
        // some pending error maybe in queque) }
        FTerminal->GetLog()->AddException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPSource(const std::wstring FileName,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, int Level)
{
  std::wstring DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(FileName, true), osLocal, Level == 0);

  FTerminal->LogEvent(FORMAT(L"File: \"%s\"", (FileName)));

  OperationProgress->SetFile(FileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
    THROW_SKIP_FILE_NULL;
  }

  HANDLE File;
  int Attrs;
  __int64 MTime, ATime;
  __int64 Size;

  FTerminal->OpenLocalFile(FileName, GENERIC_READ,
    &Attrs, &File, NULL, &MTime, &ATime, &Size);

  bool Dir = FLAGSET(Attrs, faDirectory);
  TSafeHandleStream * Stream = new TSafeHandleStream((HANDLE)File);
  try
  {
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      SCPDirectorySource(FileName, TargetDir, CopyParam, Params, OperationProgress, Level);
    }
    else
    {
      std::wstring AbsoluteFileName = FTerminal->AbsolutePath(TargetDir + DestFileName, false);

      assert(File);

      // File is regular file (not directory)
      FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", (FileName)));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->LocalSize);
      OperationProgress->TransferingFile = false;

      // Will we use ASCII of BINARY file tranfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(FileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        std::wstring((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
          L" transfer mode selected.");

      TUploadSessionAction Action(FTerminal->GetLog());
      Action.FileName(ExpandUNCFileName(FileName));
      Action.Destination(AbsoluteFileName);

      TRights Rights = CopyParam->RemoteFileRights(Attrs);

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
            BlockBuf.LoadStream(Stream, OperationProgress->LocalBlockSize(), true);
          );

          OperationProgress->AddLocalyUsed(BlockBuf.GetSize());

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
            if (OperationProgress->LocalyUsed)
            {
              __int64 X = OperationProgress->LocalSize;
              X *= AsciiBuf.GetSize();
              X /= OperationProgress->LocalyUsed;
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
              (!OperationProgress->AsciiTransfer || OperationProgress->IsLocalyDone()))
          {
            std::wstring Buf;

            if (CopyParam->GetPreserveTime())
            {
              // Send last file access and modification time
              // TVarRec don't understand 'unsigned int' -> we use sprintf()
              wprintf((wchar_t *)Buf.c_str(), L"T%lu 0 %lu 0", static_cast<unsigned long>(MTime),
                static_cast<unsigned long>(ATime));
              FSecureShell->SendLine(Buf);
              SCPResponse();
            }

            // Send file modes (rights), filesize and file name
            // TVarRec don't understand 'unsigned int' -> we use sprintf()
            wprintf((wchar_t *)Buf.c_str(), L"C%s %Ld %s",
              Rights.GetOctal().data(),
              (OperationProgress->AsciiTransfer ? (__int64)AsciiBuf.GetSize()  :
                OperationProgress->LocalSize),
              DestFileName.data());
            FSecureShell->SendLine(Buf);
            SCPResponse();
            // Indicate we started transfering file, we need to finish it
            // If not, it's fatal error
            OperationProgress->TransferingFile = true;

            // If we're doing ASCII transfer, this is last pass
            // so we send whole file
            /* TODO : We can't send file above 32bit size in ASCII mode! */
            if (OperationProgress->AsciiTransfer)
            {
              FTerminal->LogEvent(FORMAT(L"Sending ASCII data (%u bytes)",
                (AsciiBuf.GetSize())));
              // Should be equal, just in case it's rounded (see above)
              OperationProgress->ChangeTransferSize(AsciiBuf.GetSize());
              while (!OperationProgress->IsTransferDone())
              {
                unsigned long BlockSize = OperationProgress->TransferBlockSize();
                FSecureShell->Send(
                  AsciiBuf.GetData() + (unsigned int)OperationProgress->TransferedSize,
                  BlockSize);
                OperationProgress->AddTransfered(BlockSize);
                if (OperationProgress->Cancel == csCancelTransfer)
                {
                  throw ExtException(L""); // FIXME USER_TERMINATED);
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
                (BlockBuf.GetSize())));
            }
            FSecureShell->Send(BlockBuf.GetData(), BlockBuf.GetSize());
            OperationProgress->AddTransfered(BlockBuf.GetSize());
          }

          if ((OperationProgress->Cancel == csCancelTransfer) ||
              (OperationProgress->Cancel == csCancel && !OperationProgress->TransferingFile))
          {
              throw ExtException(L""); // FIXME USER_TERMINATED);
          }
        }
        while (!OperationProgress->IsLocalyDone() || !OperationProgress->IsTransferDone());

        FSecureShell->SendNull();
        try
        {
          SCPResponse();
          // If one of two following exceptions occurs, it means, that remote
          // side already know, that file transfer finished, even if it failed
          // so we don't have to throw EFatal
        }
        catch (EScp &E)
        {
          // SCP protocol fatal error
          OperationProgress->TransferingFile = false;
          throw;
        }
        catch (EScpFileSkipped &E)
        {
          // SCP protocol non-fatal error
          OperationProgress->TransferingFile = false;
          throw;
        }

        // We succeded transfering file, from now we can handle exceptions
        // normally -> no fatal error
        OperationProgress->TransferingFile = false;
      }
      catch (std::exception &E)
      {
        // EScpFileSkipped is derived from EScpSkipFile,
        // but is does not indicate file skipped by user here
        if (dynamic_cast<EScpFileSkipped *>(&E) != NULL)
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
        TTouchSessionAction(FTerminal->GetLog(), AbsoluteFileName,
          UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode()));
      }
      if (CopyParam->GetPreserveRights())
      {
        TChmodSessionAction(FTerminal->GetLog(), AbsoluteFileName,
          Rights);
      }
    }
  }
  catch(...)
  {
    if (File != NULL)
    {
      CloseHandle(File);
    }
    delete Stream;
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
  {
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
      THROWOSIFFALSE(FileSetAttr(FileName, Attrs & ~faArchive) == 0);
    )
  }

  FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory finished.", FileName.c_str()));
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPDirectorySource(const std::wstring DirectoryName,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, int Level)
{
  int Attrs;

  FTerminal->LogEvent(FORMAT(L"Entering directory \"%s\".", (DirectoryName)));

  OperationProgress->SetFile(DirectoryName);
  std::wstring DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(DirectoryName, true), osLocal, Level == 0);

  // Get directory attributes
  FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, DirectoryName.c_str()),
    Attrs = FileGetAttr(DirectoryName);
    if (Attrs == -1) RaiseLastOSError();
  )

  std::wstring TargetDirFull = UnixIncludeTrailingBackslash(TargetDir + DestFileName);

  std::wstring Buf;

  /* TODO 1: maybe send filetime */

  // Send directory modes (rights), filesize and file name
  Buf = FORMAT(L"D%s 0 %s",
    (CopyParam->RemoteFileRights(Attrs).GetOctal(), DestFileName));
  FSecureShell->SendLine(Buf);
  SCPResponse();

  try
  {
    int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    // TSearchRec SearchRec;
    WIN32_FIND_DATA SearchRec;
    bool FindOK;
// FIXME 
    // FILE_OPERATION_LOOP (L"", // FIXME MTLOAD(LIST_DIR_ERROR, (DirectoryName)),
      // FindOK = (bool)(FindFirst(IncludeTrailingBackslash(DirectoryName) + L"*.*",
        // FindAttrs, SearchRec) == 0);
    // );

    try
    {
      while (FindOK && !OperationProgress->Cancel)
      {
        std::wstring FileName = IncludeTrailingBackslash(DirectoryName) + SearchRec.cFileName;
        try
        {
          if ((SearchRec.cFileName != L".") && (SearchRec.cFileName != L".."))
          {
            SCPSource(FileName, TargetDirFull, CopyParam, Params, OperationProgress, Level + 1);
          }
        }
        // Previously we catched EScpSkipFile, making error being displayed
        // even when file was excluded by mask. Now the EScpSkipFile is special
        // case without error message.
        catch (EScpFileSkipped &E)
        {
          TQueryParams Params(qpAllowContinueOnError);
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()),
                &E,
                  qaOK | qaAbort, &Params, qtError) == qaAbort)
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
// FIXME 
        // FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
          // FindOK = (FindNext(SearchRec) == 0);
        // );
      };
    }
    catch(...)
    {
      // FIXME FindClose(SearchRec);
    }

    /* TODO : Delete also read-only directories. */
    /* TODO : Show error message on failure. */
    if (!OperationProgress->Cancel)
    {
      if (FLAGSET(Params, cpDelete))
      {
        RemoveDir(DirectoryName);
      }
      else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
      {
      // FIXME
        // FILE_OPERATION_LOOP (L"", FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
          // THROWOSIFFALSE(FileSetAttr(DirectoryName, Attrs & ~faArchive) == 0);
        // )
      }
    }
  }
  catch(...)
  {
    if (FTerminal->GetActive())
    {
      // Tell remote side, that we're done.
      FTerminal->LogEvent(FORMAT(L"Leaving directory \"%s\".", DirectoryName.c_str()));
      FSecureShell->SendLine(L"E");
      SCPResponse();
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  bool CloseSCP = false;
  Params &= ~(cpAppend | cpResume);
  std::wstring Options = L"";
  if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime()) Options = L"-p";
  if (FTerminal->GetSessionData()->GetScp1Compatibility()) Options += L" -1";

  FTerminal->LogEvent(FORMAT(L"Copying %d files/directories to local directory "
    L"\"%s\"", (FilesToCopy->GetCount(), TargetDir)));
  FTerminal->LogEvent(CopyParam->GetLogStr());

  try
  {
    for (int IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; IFile++)
    {
      std::wstring FileName = FilesToCopy->GetString(IFile);
      TRemoteFile * File = (TRemoteFile *)FilesToCopy->GetObject(IFile);
      assert(File);

      try
      {
        bool Success = true; // Have to be set to true (see ::SCPSink)
        SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
          Options, DelimitStr(FileName)));
        SkipFirstLine();

        // Filename is used for error messaging and excluding files only
        // Send in full path to allow path-based excluding
        std::wstring FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
        SCPSink(TargetDir, FullFileName, UnixExtractFilePath(FullFileName),
          CopyParam, Success, OperationProgress, Params, 0);
        // operation succeded (no exception), so it's ok that
        // remote side closed SCP, but we continue with next file
        if (OperationProgress->Cancel == csRemoteAbort)
        {
          OperationProgress->Cancel = csContinue;
        }

        // Move operation -> delete file/directory afterwards
        // but only if copying succeded
        if ((Params & cpDelete) && Success && !OperationProgress->Cancel)
        {
          try
          {
            FTerminal->SetExceptionOnFail (true);
            try
            {
              FILE_OPERATION_LOOP(L"", // FFIXME MTLOAD(DELETE_FILE_ERROR, (FileName)),
                // pass full file name in FileName, in case we are not moving
                // from current directory
                FTerminal->DeleteFile(FileName, File)
              );
            }
            catch(...)
            {
              FTerminal->SetExceptionOnFail (false);
            }
          }
          catch (EFatal &E)
          {
            throw;
          }
          catch (...)
          {
            // If user selects skip (or abort), nothing special actualy occurs
            // we just run DoFinished with Success = false, so file won't
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
  catch(...)
  {
    // In case that copying doesn't cause fatal error (ie. connection is
    // still active) but wasn't succesful (exception or user termination)
    // we need to ensure, that SCP on remote side is closed
    if (FTerminal->GetActive() && (CloseSCP ||
        (OperationProgress->Cancel == csCancel) ||
        (OperationProgress->Cancel == csCancelTransfer)))
    {
      bool LastLineRead;

      // If we get LastLine, it means that remote side 'scp' is already
      // terminated, so we need not to terminate it. There is also
      // possibility that remote side waits for confirmation, so it will hang.
      // This should not happen (hope)
      std::wstring Line = FSecureShell->ReceiveLine();
      LastLineRead = IsLastLine(Line);
      if (!LastLineRead)
      {
        SCPSendError((OperationProgress->Cancel ? L"Terminated by user." : L"std::exception"), true);
      }
      // Just in case, remote side already sent some more data (it's probable)
      // but we don't want to raise exception (user asked to terminate, it's not error)
      int ECParams = coOnlyReturnCode;
      if (!LastLineRead) ECParams |= coWaitForLastLine;
      ReadCommandOutput(ECParams);
    }
  }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPError(const std::wstring Message, bool Fatal)
{
  SCPSendError(Message, Fatal);
  THROW_FILE_SKIPPED(NULL, Message);
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPSendError(const std::wstring Message, bool Fatal)
{
  char ErrorLevel = (char)(Fatal ? 2 : 1);
  FTerminal->LogEvent(FORMAT(L"Sending SCP error (%d) to remote side:",
    ((int)ErrorLevel)));
  FSecureShell->Send(&ErrorLevel, 1);
  // We don't send exact error message, because some unspecified
  // characters can terminate remote scp
  FSecureShell->SendLine(L"scp: error");
}
//---------------------------------------------------------------------------
void TSCPFileSystem::SCPSink(const std::wstring TargetDir,
  const std::wstring FileName, const std::wstring SourceDir,
  const TCopyParamType * CopyParam, bool & Success,
  TFileOperationProgressType * OperationProgress, int Params,
  int Level)
{
  struct
  {
    int SetTime;
    FILETIME AcTime;
    FILETIME WrTime;
    TRights RemoteRights;
    int Attrs;
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
    if (FileData.SetTime) FileData.SetTime--;

    // In case of error occured before control record arrived.
    // We can finally use full path here, as we get current path in FileName param
    // (we used to set the file into OperationProgress->GetFileName(), but it collided
    // with progress outputing, particularly for scripting)
    std::wstring AbsoluteFileName = FileName;

    try
    {
      // Receive control record
      std::wstring Line = FSecureShell->ReceiveLine();

      if (Line.size() == 0) FTerminal->FatalError(NULL, LoadStr(SCP_EMPTY_LINE));

      if (IsLastLine(Line))
      {
        // Remote side finished copying, so remote SCP was closed
        // and we don't need to terminate it manualy, see CopyToLocal()
        OperationProgress->Cancel = csRemoteAbort;
        /* TODO 1 : Show stderror to user? */
        FSecureShell->ClearStdError();
        try
        {
          // coIgnoreWarnings should allow batch transfer to continue when
          // download of one the files failes (user denies overwritting
          // of target local file, no read permissions...)
          ReadCommandOutput(coExpectNoOutput | coRaiseExcept |
            coOnlyReturnCode | coIgnoreWarnings);
          if (!Initialized)
          {
            throw std::exception("");
          }
        }
        catch(std::exception & E)
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

        // First characted distinguish type of control record
        char Ctrl = Line[1];
        Line.erase(1, 1);

        switch (Ctrl) {
          case 1:
            // Error (already logged by ReceiveLine())
            THROW_FILE_SKIPPED(NULL, FMTLOAD(REMOTE_ERROR, Line.c_str()));

          case 2:
            // Fatal error, terminate copying
            FTerminal->TerminalError(Line);
            return; // Unreachable

          case 'E': // Exit
            FSecureShell->SendNull();
            return;

          case 'T':
            unsigned long MTime, ATime;
            if (swscanf(Line.c_str(), L"%ld %*d %ld %*d",  &MTime, &ATime) == 2)
            {
              FileData.AcTime = DateTimeToFileTime(UnixToDateTime(ATime,
                FTerminal->GetSessionData()->GetDSTMode()), FTerminal->GetSessionData()->GetDSTMode());
              FileData.WrTime = DateTimeToFileTime(UnixToDateTime(MTime,
                FTerminal->GetSessionData()->GetDSTMode()), FTerminal->GetSessionData()->GetDSTMode());
              SourceTimestamp = UnixToDateTime(MTime,
                FTerminal->GetSessionData()->GetDSTMode());
              FSecureShell->SendNull();
              // File time is only valid until next pass
              FileData.SetTime = 2;
              continue;
            }
              else
            {
              SCPError(LoadStr(SCP_ILLEGAL_TIME_FORMAT), false);
            }

          case 'C':
          case 'D':
            break; // continue pass switch{}

          default:
            FTerminal->FatalError(NULL, FMTLOAD(SCP_INVALID_CONTROL_RECORD, Ctrl, Line.c_str()));
        }

        TFileMasks::TParams MaskParams;

        // We reach this point only if control record was 'C' or 'D'
        try
        {
          FileData.RemoteRights.SetOctal (CutToChar(Line, ' ', true));
          // do not trim leading spaces of the filename
          __int64 TSize = StrToInt64(TrimRight(CutToChar(Line, ' ', false)));
          MaskParams.Size = TSize;
          // Security fix: ensure the file ends up where we asked for it.
          // (accept only filename, not path)
          std::wstring OnlyFileName = UnixExtractFileName(Line);
          if (Line != OnlyFileName)
          {
            FTerminal->LogEvent(FORMAT(L"Warning: Remote host set a compound pathname '%s'", (Line)));
          }

          OperationProgress->SetFile(OnlyFileName);
          AbsoluteFileName = SourceDir + OnlyFileName;
          OperationProgress->SetTransferSize(TSize);
        }
        catch (std::exception &E)
        {
          SUSPEND_OPERATION (
            FTerminal->GetLog()->AddException(&E);
          );
          SCPError(LoadStr(SCP_ILLEGAL_FILE_DESCRIPTOR), false);
        }

        // last possibility to cancel transfer before it starts
        if (OperationProgress->Cancel)
        {
          THROW_SKIP_FILE(NULL, LoadStr(USER_TERMINATED));
        }

        bool Dir = (Ctrl == L'D');
        std::wstring SourceFullName = SourceDir + OperationProgress->FileName;
        if (!CopyParam->AllowTransfer(SourceFullName, osRemote, Dir, MaskParams))
        {
          FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer",
            AbsoluteFileName.c_str()));
          SkipConfirmed = true;
          SCPError(L"", false);
        }

        std::wstring DestFileName =
          IncludeTrailingBackslash(TargetDir) +
          CopyParam->ChangeFileName(OperationProgress->FileName, osRemote,
            Level == 0);

        FileData.Attrs = FileGetAttr(DestFileName);
        // If getting attrs failes, we suppose, that file/folder doesn't exists
        FileData.Exists = (FileData.Attrs != -1);
        if (Dir)
        {
          if (FileData.Exists && !(FileData.Attrs & faDirectory))
          {
            SCPError(FMTLOAD(NOT_DIRECTORY_ERROR, DestFileName.c_str()),
                false);
          }

          if (!FileData.Exists)
          {
            // FIXME
            // FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFileName.c_str()),
              // if (!ForceDirectories(DestFileName)) RaiseLastOSError();
            // );
            /* SCP: can we set the timestamp for directories ? */
          }
          std::wstring FullFileName = SourceDir + OperationProgress->FileName;
          SCPSink(DestFileName, FullFileName, UnixIncludeTrailingBackslash(FullFileName),
            CopyParam, Success, OperationProgress, Params, Level + 1);
          continue;
        }
          else
        if (Ctrl == 'C')
        {
          TDownloadSessionAction Action(FTerminal->GetLog());
          Action.FileName(AbsoluteFileName);

          try
          {
            HANDLE File = NULL;
            TStream * FileStream = NULL;

            /* TODO 1 : Turn off read-only attr */

            try
            {
              try
              {
                if (FileExists(DestFileName))
                {
                  __int64 MTime;
                  TOverwriteFileParams FileParams;
                  FileParams.SourceSize = OperationProgress->TransferSize;
                  FileParams.SourceTimestamp = SourceTimestamp;
                  FTerminal->OpenLocalFile(DestFileName, GENERIC_READ,
                    NULL, NULL, NULL, &MTime, NULL,
                    &FileParams.DestSize);
                  FileParams.DestTimestamp = UnixToDateTime(MTime,
                    FTerminal->GetSessionData()->GetDSTMode());

                  TQueryButtonAlias Aliases[1];
                  Aliases[0].Button = qaAll;
                  Aliases[0].Alias = LoadStr(YES_TO_NEWER_BUTTON);
                  TQueryParams QueryParams(qpNeverAskAgainCheck);
                  QueryParams.Aliases = Aliases;
                  QueryParams.AliasesCount = LENOF(Aliases);

                  int Answer;
                  SUSPEND_OPERATION (
                    Answer = FTerminal->ConfirmFileOverwrite(
                      OperationProgress->FileName, &FileParams,
                      qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll,
                      &QueryParams, osLocal, Params, OperationProgress);
                  );

                  switch (Answer)
                  {
                    case qaCancel:
                      OperationProgress->Cancel = csCancel; // continue on next case
                    case qaNo:
                      SkipConfirmed = true;
                      // FIXME EXCEPTION;
                  }
                }

                Action.Destination(DestFileName);

                if (!FTerminal->CreateLocalFile(DestFileName, OperationProgress,
                       &File, FLAGSET(Params, cpNoConfirmation)))
                {
                  SkipConfirmed = true;
                  // FIXME EXCEPTION;
                }

                FileStream = new TSafeHandleStream((HANDLE)File);
              }
              catch (std::exception &E)
              {
                // In this step we can still cancel transfer, so we do it
                SCPError(::MB2W(E.what()), false);
                throw;
              }

              // We succeded, so we confirm transfer to remote side
              FSecureShell->SendNull();
              // From now we need to finish file transfer, if not it's fatal error
              OperationProgress->TransferingFile = true;

              // Suppose same data size to transfer as to write
              // (not true with ASCII transfer)
              OperationProgress->SetLocalSize(OperationProgress->TransferSize);

              // Will we use ASCII of BINARY file tranfer?
              OperationProgress->SetAsciiTransfer(
                CopyParam->UseAsciiTransfer(SourceFullName, osRemote, MaskParams));
              FTerminal->LogEvent(std::wstring((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
                L" transfer mode selected.");

              try
              {
                // Buffer for one block of data
                TFileBuffer BlockBuf;
                bool ConvertToken = false;

                do
                {
                  BlockBuf.SetSize (OperationProgress->TransferBlockSize());
                  BlockBuf.SetPosition (0);

                  FSecureShell->Receive(BlockBuf.GetData(), BlockBuf.GetSize());
                  OperationProgress->AddTransfered(BlockBuf.GetSize());

                  if (OperationProgress->AsciiTransfer)
                  {
                    unsigned int PrevBlockSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetSessionData()->GetEOLType(),
                      FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                    OperationProgress->SetLocalSize(
                      OperationProgress->LocalSize - PrevBlockSize + BlockBuf.GetSize());
                  }

                  // This is crucial, if it fails during file transfer, it's fatal error
                  // FIXME
                  // FILE_OPERATION_LOOP_EX (false, FMTLOAD(WRITE_ERROR, DestFileName.c_str()),
                    // BlockBuf.WriteToStream(FileStream, BlockBuf.Size);
                  // );

                  OperationProgress->AddLocalyUsed(BlockBuf.GetSize());

                  if (OperationProgress->Cancel == csCancelTransfer)
                  {
                    throw ExtException(L""); // FIXME USER_TERMINATED);
                  }
                }
                while (!OperationProgress->IsLocalyDone() || !
                    OperationProgress->IsTransferDone());
              }
              catch (std::exception &E)
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
              catch (EScp &E)
              {
                FSecureShell->SendNull();
                throw;
              }
              catch (EScpFileSkipped &E)
              {
                FSecureShell->SendNull();
                throw;
              }

              FSecureShell->SendNull();

              if (FileData.SetTime && CopyParam->GetPreserveTime())
              {
                SetFileTime(File, NULL, &FileData.AcTime, &FileData.WrTime);
              }
            }
            catch(...)
            {
              if (File) CloseHandle(File);
              if (FileStream) delete FileStream;
            }
          }
          catch(std::exception & E)
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

          if (FileData.Attrs == -1) FileData.Attrs = faArchive;
          int NewAttrs = CopyParam->LocalFileAttrs(FileData.RemoteRights);
          if ((NewAttrs & FileData.Attrs) != NewAttrs)
          {
            // FIXME
            // FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFileName.c_str()),
              // THROWOSIFFALSE(FileSetAttr(DestFileName, FileData.Attrs | NewAttrs) == 0);
            // );
          }
        }
      }
    }
    catch (EScpFileSkipped &E)
    {
      if (!SkipConfirmed)
      {
        SUSPEND_OPERATION (
          TQueryParams Params(qpAllowContinueOnError);
          if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, AbsoluteFileName.c_str()),
                &E, qaOK | qaAbort, &Params, qtError) == qaAbort)
          {
            OperationProgress->Cancel = csCancel;
          }
          FTerminal->FLog->AddException(&E);
        );
      }
      // this was inside above condition, but then transfer was considered
      // succesfull, even when for example user refused to overwrite file
      Success = false;
    }
    catch (EScpSkipFile &E)
    {
      SCPSendError(E.GetMessage(), false);
      Success = false;
      if (!FTerminal->HandleException(&E)) throw;
    }
  }
}
