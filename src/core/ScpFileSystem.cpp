//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"

#include <stdio.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>
#endif

#include "ScpFileSystem.h"

#include "Terminal.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "TextsCore.h"
#include "SecureShell.h"

#include <stdio.h>
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
#undef FILE_OPERATION_LOOP_EX
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(Self->FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
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
class TCommandSet
{
private:
  TCommandType CommandSet[ShellCommandCount];
  TSessionData * FSessionData;
  UnicodeString FReturnVar;
public:
  int __fastcall GetMaxLines(TFSCommand Cmd);
  int __fastcall GetMinLines(TFSCommand Cmd);
  bool __fastcall GetModifiesFiles(TFSCommand Cmd);
  bool __fastcall GetChangesDirectory(TFSCommand Cmd);
  bool __fastcall GetOneLineCommand(TFSCommand Cmd);
  void __fastcall SetCommands(TFSCommand Cmd, UnicodeString value);
  UnicodeString __fastcall GetCommands(TFSCommand Cmd);
  UnicodeString __fastcall GetFirstLine();
  bool __fastcall GetInteractiveCommand(TFSCommand Cmd);
  UnicodeString __fastcall GetLastLine();
  UnicodeString __fastcall GetReturnVar();
public:
  /* __fastcall */ TCommandSet(TSessionData *aSessionData);
  void __fastcall Default();
  void __fastcall CopyFrom(TCommandSet * Source);
#ifndef _MSC_VER
  UnicodeString __fastcall Command(TFSCommand Cmd, const TVarRec * args, int size);
#else
  UnicodeString __fastcall Command(TFSCommand Cmd, ...);
  UnicodeString __fastcall Command(TFSCommand Cmd, va_list args);
#endif
  TStrings * __fastcall CreateCommandList();
#ifndef _MSC_VER
  UnicodeString __fastcall FullCommand(TFSCommand Cmd, const TVarRec * args, int size);
#else
  UnicodeString __fastcall FullCommand(TFSCommand Cmd, ...);
  UnicodeString __fastcall FullCommand(TFSCommand Cmd, va_list args);
#endif
  static UnicodeString __fastcall ExtractCommand(UnicodeString Command);
#ifndef _MSC_VER
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
#else
  TSessionData * __fastcall GetSessionData() { return FSessionData; }
  void __fastcall SetSessionData(TSessionData * value) { FSessionData = value; }
  void __fastcall SetReturnVar(const UnicodeString value) { FReturnVar = value; }
#endif
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
/* __fastcall */ TCommandSet::TCommandSet(TSessionData *aSessionData):
  FSessionData(aSessionData), FReturnVar(L"")
{
  assert(FSessionData);
  Default();
}
//---------------------------------------------------------------------------
void __fastcall TCommandSet::CopyFrom(TCommandSet * Source)
{
  memmove(&CommandSet, Source->CommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
void __fastcall TCommandSet::Default()
{
  assert(sizeof(CommandSet) == sizeof(DefaultCommandSet));
  memmove(&CommandSet, &DefaultCommandSet, sizeof(CommandSet));
}
//---------------------------------------------------------------------------
int __fastcall TCommandSet::GetMaxLines(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].MaxLines;
}
//---------------------------------------------------------------------------
int __fastcall TCommandSet::GetMinLines(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].MinLines;
}
//---------------------------------------------------------------------------
bool __fastcall TCommandSet::GetModifiesFiles(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].ModifiesFiles;
}
//---------------------------------------------------------------------------
bool __fastcall TCommandSet::GetChangesDirectory(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].ChangesDirectory;
}
//---------------------------------------------------------------------------
bool __fastcall TCommandSet::GetInteractiveCommand(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].InteractiveCommand;
}
//---------------------------------------------------------------------------
bool __fastcall TCommandSet::GetOneLineCommand(TFSCommand /*Cmd*/)
{
  //CHECK_CMD;
  // #56: we send "echo last line" from all commands on same line
  // just as it was in 1.0
  return True; //CommandSet[Cmd].OneLineCommand;
}
//---------------------------------------------------------------------------
void __fastcall TCommandSet::SetCommands(TFSCommand Cmd, UnicodeString value)
{
  CHECK_CMD;
  wcscpy(const_cast<wchar_t *>(CommandSet[Cmd].Command), value.SubString(1, MaxCommandLen - 1).c_str());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCommandSet::GetCommands(TFSCommand Cmd)
{
  CHECK_CMD;
  return CommandSet[Cmd].Command;
}
//---------------------------------------------------------------------------
#ifndef _MSC_VER
UnicodeString __fastcall TCommandSet::Command(TFSCommand Cmd, const TVarRec * args, int size)
{
  if (args) return Format(Commands[Cmd], args, size);
    else return Commands[Cmd];
}
#endif
//---------------------------------------------------------------------------
UnicodeString __fastcall TCommandSet::Command(TFSCommand Cmd, ...)
{
  UnicodeString result;
  va_list args;
  va_start(args, Cmd);
  result = Command(Cmd, args);
  va_end(args);
  return result;
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::Command(TFSCommand Cmd, va_list args)
{
  UnicodeString result;
  result = ::Format(GetCommands(Cmd).c_str(), args);
  return result.c_str();
}
//---------------------------------------------------------------------------
#ifndef _MSC_VER
UnicodeString __fastcall TCommandSet::FullCommand(TFSCommand Cmd, const TVarRec * args, int size)
{
  UnicodeString Separator;
  if (OneLineCommand[Cmd]) Separator = L" ; ";
    else Separator = L"\n";
  UnicodeString Line = Command(Cmd, args, size);
  UnicodeString LastLineCmd =
    Command(fsLastLine, ARRAYOFCONST((LastLine, ReturnVar)));
  UnicodeString FirstLineCmd;
  if (InteractiveCommand[Cmd])
    FirstLineCmd = Command(fsFirstLine, ARRAYOFCONST((FirstLine))) + Separator;

  UnicodeString Result;
  if (!Line.IsEmpty())
    Result = FORMAT(L"%s%s%s%s", (FirstLineCmd, Line, Separator, LastLineCmd));
  else
    Result = FORMAT(L"%s%s", (FirstLineCmd, LastLineCmd));
  return Result;
}
#endif
//---------------------------------------------------------------------------
UnicodeString __fastcall TCommandSet::FullCommand(TFSCommand Cmd, ...)
{
  UnicodeString Result;
  va_list args;
  va_start(args, Cmd);
  Result = FullCommand(Cmd, args);
  va_end(args);
  return Result.c_str();
}
//---------------------------------------------------------------------------
UnicodeString TCommandSet::FullCommand(TFSCommand Cmd, va_list args)
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
UnicodeString __fastcall TCommandSet::GetFirstLine()
{
  return FIRST_LINE;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCommandSet::GetLastLine()
{
  return LAST_LINE;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCommandSet::GetReturnVar()
{
  assert(GetSessionData());
  if (!FReturnVar.IsEmpty())
  {
    return UnicodeString(L"$") + FReturnVar;
  }
  else if (GetSessionData()->GetDetectReturnVar())
  {
    return L"0";
  }
  else
  {
    return UnicodeString(L"$") + GetSessionData()->GetReturnVar();
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCommandSet::ExtractCommand(UnicodeString Command)
{
  int P = Command.Pos(L" ");
  if (P > 0)
  {
    Command.SetLength(P-1);
  }
  return Command;
}
//---------------------------------------------------------------------------
TStrings * __fastcall TCommandSet::CreateCommandList()
{
  TStrings * CommandList = new TStringList();
  for (Integer Index = 0; Index < ShellCommandCount; Index++)
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
/* __fastcall */ TSCPFileSystem::TSCPFileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(ATerminal)
{
  Self = this;
}

void __fastcall TSCPFileSystem::Init(TSecureShell * SecureShell)
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
    FFileSystemInfo.IsCapable[Index] = IsCapable(static_cast<TFSCapability>(Index));
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TSCPFileSystem::~TSCPFileSystem()
{
  delete FCommandSet;
  delete FOutput;
  delete FSecureShell;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::Open()
{
  FSecureShell->Open();
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::Close()
{
  FSecureShell->Close();
}
//---------------------------------------------------------------------------
bool __fastcall TSCPFileSystem::GetActive()
{
  return FSecureShell->GetActive();
}
//---------------------------------------------------------------------------
const TSessionInfo & __fastcall TSCPFileSystem::GetSessionInfo()
{
  return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & __fastcall TSCPFileSystem::GetFileSystemInfo(bool Retrieve)
{
  if (FFileSystemInfo.AdditionalInfo.IsEmpty() && Retrieve)
  {
    UnicodeString UName;
    FTerminal->SetExceptionOnFail(true);
    // try
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FTerminal->SetExceptionOnFail(false);
      } BOOST_SCOPE_EXIT_END
      try
      {
        AnyCommand(L"uname -a", NULL);
        for (int Index = 0; Index < GetOutput()->GetCount(); Index++)
        {
          if (Index > 0)
          {
            UName += L"; ";
          }
          UName += GetOutput()->GetStrings(Index);
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
#ifndef _MSC_VER
    __finally
    {
      FTerminal->ExceptionOnFail = false;
    }
#endif

    FFileSystemInfo.RemoteSystem = UName;
  }

  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool __fastcall TSCPFileSystem::TemporaryTransferFile(const UnicodeString & /*FileName*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TSCPFileSystem::GetStoredCredentialsTried()
{
  return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSCPFileSystem::GetUserName()
{
  return FSecureShell->GetUserName();
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::Idle()
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
        ExecCommand2(fsNull);
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
UnicodeString __fastcall TSCPFileSystem::AbsolutePath(UnicodeString Path, bool /*Local*/)
{
  return ::AbsolutePath(GetCurrentDirectory(), Path);
}
//---------------------------------------------------------------------------
bool __fastcall TSCPFileSystem::IsCapable(int Capability) const
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
UnicodeString __fastcall TSCPFileSystem::DelimitStr(UnicodeString Str)
{
  if (!Str.IsEmpty())
  {
    Str = ::DelimitStr(Str, L"\\`$\"");
    if (Str[1] == L'-') Str = L"./"+Str;
  }
  return Str;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::EnsureLocation()
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
void __fastcall TSCPFileSystem::SendCommand(const UnicodeString Cmd)
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
bool __fastcall TSCPFileSystem::IsTotalListingLine(const UnicodeString Line)
{
  // On some hosts there is not "total" but "totalt". What's the reason??
  // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
  return !Line.SubString(1, 5).CompareIC(L"total");
}
//---------------------------------------------------------------------------
bool __fastcall TSCPFileSystem::RemoveLastLine(UnicodeString & Line,
    int & ReturnCode, UnicodeString LastLine)
{
  bool IsLastLine = false;
  if (LastLine.IsEmpty()) { LastLine = LAST_LINE; }
  // #55: fixed so, even when last line of command output does not
  // contain CR/LF, we can recognize last line
  int Pos = Line.Pos(LastLine);
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
bool __fastcall TSCPFileSystem::IsLastLine(UnicodeString & Line)
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
void __fastcall TSCPFileSystem::SkipFirstLine()
{
  UnicodeString Line = FSecureShell->ReceiveLine();
  if (Line != FCommandSet->GetFirstLine())
  {
    FTerminal->TerminalError(NULL, FMTLOAD(FIRST_LINE_EXPECTED, Line.c_str()));
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadCommandOutput(int Params, const UnicodeString * Cmd)
{
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FProcessingCommand = false;
    } BOOST_SCOPE_EXIT_END
    if (Params & coWaitForLastLine)
    {
      UnicodeString Line;
      bool IsLast = true;
      unsigned int Total = 0;
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
        if (!Message.IsEmpty()) { Message += L"\n"; }
        Message += FOutput->GetText();
      }
      while (!Message.IsEmpty() && (Message.LastDelimiter(L"\n\r") == Message.Length()))
      {
        Message.SetLength(Message.Length() - 1);
      }

      bool WrongReturnCode =
        (GetReturnCode() > 1) || (GetReturnCode() == 1 && !(Params & coIgnoreWarnings));

      if (Params & coOnlyReturnCode && WrongReturnCode)
      {
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, GetReturnCode()));
      }
      else
      if (!(Params & coOnlyReturnCode) &&
          ((!Message.IsEmpty() && ((FOutput->GetCount() == 0) || !(Params & coIgnoreWarnings))) ||
           WrongReturnCode))
      {
        assert(Cmd != NULL);
        FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED, Cmd->c_str(), GetReturnCode(), Message.c_str()));
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    FProcessingCommand = false;
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ExecCommand(const UnicodeString & Cmd, int Params,
  const UnicodeString & CmdString)
{
  if (Params < 0) { Params = ecDefault; }
  if (FTerminal->GetUseBusyCursor())
  {
    Busy(true);
  }
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      if (Self->FTerminal->GetUseBusyCursor())
      {
        Busy(false);
      }
    } BOOST_SCOPE_EXIT_END
    SendCommand(Cmd);

    int COParams = coWaitForLastLine;
    if (Params & ecRaiseExcept) { COParams |= coRaiseExcept; }
    if (Params & ecIgnoreWarnings) { COParams |= coIgnoreWarnings; }
    if (Params & ecReadProgress) { COParams |= coReadProgress; }
    ReadCommandOutput(COParams, &CmdString);
  }
#ifndef _MSC_VER
  __finally
  {
    if (FTerminal->UseBusyCursor)
    {
      Busy(false);
    }
  }
#endif
}
//---------------------------------------------------------------------------
#ifndef _MSC_VER
void __fastcall TSCPFileSystem::ExecCommand(TFSCommand Cmd, const TVarRec * args,
  int size, int Params)
{
  if (Params < 0) Params = ecDefault;
  UnicodeString FullCommand = FCommandSet->FullCommand(Cmd, args, size);
  UnicodeString Command = FCommandSet->Command(Cmd, args, size);
  ExecCommand(FullCommand, Params, Command);
  if (Params & ecRaiseExcept)
  {
    Integer MinL = FCommandSet->MinLines[Cmd];
    Integer MaxL = FCommandSet->MaxLines[Cmd];
    if (((MinL >= 0) && (MinL > FOutput->Count)) ||
        ((MaxL >= 0) && (MaxL > FOutput->Count)))
    {
      FTerminal->TerminalError(FmtLoadStr(INVALID_OUTPUT_ERROR,
        ARRAYOFCONST((FullCommand, Output->Text))));
    }
  }
}
#endif
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ExecCommand2(TFSCommand Cmd, ...)
{
  int Params = 0;
  va_list args;
  va_start(args, Cmd);
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
UnicodeString __fastcall TSCPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::DoStartup()
{
  // SkipStartupMessage and DetectReturnVar must succeed,
  // otherwise session is to be closed.
  FTerminal->SetExceptionOnFail(true);
  SkipStartupMessage();
  if (FTerminal->GetSessionData()->GetDetectReturnVar()) { DetectReturnVar(); }
  FTerminal->SetExceptionOnFail(false);

  #define COND_OPER(OPER) if (FTerminal->GetSessionData()->Get##OPER()) OPER()
  COND_OPER(ClearAliases);
  COND_OPER(UnsetNationalVars);
  #undef COND_OPER
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SkipStartupMessage()
{
  try
  {
    FTerminal->LogEvent(L"Skipping host startup message (if any).");
    ExecCommand2(fsNull);
  }
  catch (Exception & E)
  {
    FTerminal->CommandError(&E, LoadStr(SKIP_STARTUP_MESSAGE_ERROR));
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::LookupUsersGroups()
{
  ExecCommand2(fsLookupUsersGroups);
  FTerminal->FUsers.Clear();
  FTerminal->FGroups.Clear();
  if (FOutput->GetCount() > 0)
  {
    UnicodeString Groups = FOutput->GetStrings(0);
    while (!Groups.IsEmpty())
    {
      UnicodeString NewGroup = CutToChar(Groups, L' ', false);
      FTerminal->FGroups.Add(TRemoteToken(NewGroup));
      FTerminal->FMembership.Add(TRemoteToken(NewGroup));
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::DetectReturnVar()
{
  // This suppose that something was already executed (probably SkipStartupMessage())
  // or return code variable is already set on start up.

  try
  {
    // #60 17.10.01: "status" and "?" switched
    UnicodeString ReturnVars[2] = { L"status", L"?" };
    UnicodeString NewReturnVar = L"";
    FTerminal->LogEvent(L"Detecting variable containing return code of last command.");
    for (int Index = 0; Index < 2; Index++)
    {
      bool Success = true;

      try
      {
        FTerminal->LogEvent(FORMAT(L"Trying \"$%s\".", ReturnVars[Index].c_str()));
        ExecCommand2(fsVarValue, ReturnVars[Index].c_str());
        UnicodeString str = GetOutput()->GetCount() > 0 ? GetOutput()->GetStrings(0) : L"";
        int val = StrToIntDef(str, 256);
        if ((GetOutput()->GetCount() != 1) || str.IsEmpty() || (val > 255))
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
      catch (Exception &E)
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
void __fastcall TSCPFileSystem::ClearAlias(UnicodeString Alias)
{
  if (!Alias.IsEmpty())
  {
    // this command usually fails, because there will never be
    // aliases on all commands -> see last False parametr
    ExecCommand2(fsUnalias, Alias.c_str(), false);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ClearAliases()
{
  try
  {
    FTerminal->LogEvent(L"Clearing all aliases.");
    ClearAlias(TCommandSet::ExtractCommand(FTerminal->GetSessionData()->GetListingCommand()));
    TStrings * CommandList = FCommandSet->CreateCommandList();
    // try
    {
      BOOST_SCOPE_EXIT ( (&CommandList) )
      {
        delete CommandList;
      } BOOST_SCOPE_EXIT_END
      for (int Index = 0; Index < CommandList->GetCount(); Index++)
      {
        ClearAlias(CommandList->GetStrings(Index));
      }
    }
#ifndef _MSC_VER
    __finally
    {
      delete CommandList;
    }
#endif
  }
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNALIAS_ALL_ERROR));
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::UnsetNationalVars()
{
  try
  {
    FTerminal->LogEvent(L"Clearing national user variables.");
    for (int Index = 0; Index < NationalVarCount; Index++)
    {
      ExecCommand2(fsUnset, NationalVars[Index], false);
    }
  }
  catch (Exception &E)
  {
    FTerminal->CommandError(&E, LoadStr(UNSET_NATIONAL_ERROR));
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadCurrentDirectory()
{
  if (FCachedDirectoryChange.IsEmpty())
  {
    ExecCommand2(fsCurrentDirectory);
    FCurrentDirectory = UnixExcludeTrailingBackslash(FOutput->GetStrings(0));
  }
  else
  {
    FCurrentDirectory = FCachedDirectoryChange;
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::HomeDirectory()
{
  ExecCommand2(fsHomeDirectory);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::AnnounceFileListOperation()
{
  // noop
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ChangeDirectory(const UnicodeString Directory)
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
  ExecCommand2(fsChangeDirectory, ToDir.c_str());
  FCachedDirectoryChange = L"";
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CachedChangeDirectory(const UnicodeString Directory)
{
  FCachedDirectoryChange = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadDirectory(TRemoteFileList * FileList)
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
        ExecCommand2(fsListCurrentDirectory,
          FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, Params);
      }
      else
      {
        FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".",
          FileList->GetDirectory().c_str()));
        ExecCommand2(fsListDirectory,
          FTerminal->GetSessionData()->GetListingCommand().c_str(), Options,
            DelimitStr(FileList->GetDirectory().c_str()).c_str(),
          Params);
      }

      TRemoteFile * File = NULL;

      // If output is not empty, we have succesfully got file listing,
      // otherwise there was an error, in case it was "permission denied"
      // we try to get at least parent directory (see "else" statement below)
      if (FOutput->GetCount() > 0)
      {
        // Copy LS command output, because eventual symlink analysis would
        // modify FTerminal->Output
        TStringList * OutputCopy = new TStringList();
        // try
        {
          BOOST_SCOPE_EXIT ( (&OutputCopy) )
          {
            delete OutputCopy;
          } BOOST_SCOPE_EXIT_END
          OutputCopy->Assign(FOutput);

          // delete leading "total xxx" line
          // On some hosts there is not "total" but "totalt". What's the reason??
          // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
          if (IsTotalListingLine(OutputCopy->GetStrings(0)))
          {
            OutputCopy->Delete(0);
          }

          for (int Index = 0; Index < OutputCopy->GetCount(); Index++)
          {
            File = CreateRemoteFile(OutputCopy->GetStrings(Index));
            FileList->AddFile(File);
          }
        }
#ifndef _MSC_VER
        __finally
        {
          delete OutputCopy;
        }
#endif
      }
      else
      {
        bool Empty = true;
        if (ListCurrentDirectory)
        {
          // Empty file list -> probably "permision denied", we
          // at least get link to parent directory ("..")
          FTerminal->ReadFile(
            UnixIncludeTrailingBackslash(FTerminal->FFiles->GetDirectory()) +
              PARENTDIRECTORY, File);
          Empty = (File == NULL); // || (wcscmp(File->GetFileName().c_str(), PARENTDIRECTORY) == 0));
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
              L"directory listing will be displayed immediatelly.",
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
void __fastcall TSCPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadFile(const UnicodeString FileName,
  TRemoteFile *& File)
{
  CustomReadFile(FileName, File, NULL);
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TSCPFileSystem::CreateRemoteFile(
  const UnicodeString & ListingStr, TRemoteFile * LinkedByFile)
{
  TRemoteFile * File = new TRemoteFile(LinkedByFile);
  try
  {
    File->SetTerminal(FTerminal);
    File->SetListingStr(ListingStr);
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
void __fastcall TSCPFileSystem::CustomReadFile(const UnicodeString FileName,
  TRemoteFile *& File, TRemoteFile * ALinkedByFile)
{
  File = NULL;
  int Params = ecDefault |
    FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
  // the auto-detection of --full-time support is not implemented for fsListFile,
  // so we use it only if we already know that it is supported (asOn).
  const wchar_t * Options = (FLsFullTime == asOn) ? FullTimeOption : L"";
  ExecCommand2(fsListFile,
    FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, DelimitStr(FileName).c_str(),
    Params);
  if (FOutput->GetCount())
  {
    int LineIndex = 0;
    if (IsTotalListingLine(FOutput->GetStrings(LineIndex)) && FOutput->GetCount() > 1)
    {
      LineIndex++;
    }

    File = CreateRemoteFile(FOutput->GetStrings(LineIndex), ALinkedByFile);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::DeleteFile(const UnicodeString FileName,
  const TRemoteFile * File, int Params, TRmSessionAction & Action)
{
  USEDPARAM(File);
  USEDPARAM(Params);
  Action.Recursive();
  assert(FLAGCLEAR(Params, dfNoRecursive) || (File && File->GetIsSymLink()));
  ExecCommand2(fsDeleteFile, DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::RenameFile(const UnicodeString FileName,
  const UnicodeString NewName)
{
  ExecCommand2(fsRenameFile, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CopyFile(const UnicodeString FileName,
  const UnicodeString NewName)
{
  ExecCommand2(fsCopyFile, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CreateDirectory(const UnicodeString DirName)
{
  ExecCommand2(fsCreateDirectory, DelimitStr(DirName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CreateLink(const UnicodeString FileName,
  const UnicodeString PointTo, bool Symbolic)
{
  ExecCommand2(fsCreateLink,
    Symbolic ? L"-s" : L"", DelimitStr(PointTo).c_str(), DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ChangeFileToken(const UnicodeString & DelimitedName,
  const TRemoteToken & Token, TFSCommand Cmd, const UnicodeString & RecursiveStr)
{
  UnicodeString Str;
  if (Token.GetIDValid())
  {
    Str = IntToStr(static_cast<int>(Token.GetID()));
  }
  else if (Token.GetNameValid())
  {
    Str = Token.GetName();
  }

  if (!Str.IsEmpty())
  {
    ExecCommand2(Cmd, RecursiveStr.c_str(), Str.c_str(), DelimitedName.c_str());
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ChangeFileProperties(const UnicodeString FileName,
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
      ExecCommand2(fsChangeMode,
        RecursiveStr.c_str(), Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
    }

    // if file is directory and we do recursive mode settings with
    // add-x-to-directories option on, add those X
    if (Recursive && IsDirectory && Properties->AddXToDirectories)
    {
      Rights.AddExecute();
      ExecCommand2(fsChangeMode,
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
bool __fastcall TSCPFileSystem::LoadFilesProperties(TStrings * /*FileList*/ )
{
  assert(false);
  return false;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CalculateFilesChecksum(const UnicodeString & /*Alg*/,
  TStrings * /*FileList*/, TStrings * /*Checksums*/,
  TCalculatedChecksumEvent /*OnCalculatedChecksum*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TSCPFileSystem::CustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile * File, UnicodeString Command, int Params,
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
    FTerminal->ProcessDirectory(FileName, fastdelegate::bind(&TTerminal::CustomCommandOnFile, FTerminal, _1, _2, _3),
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
void /* __fastcall */ TSCPFileSystem::CaptureOutput(const UnicodeString & AddedLine, bool StdError)
{
  int ReturnCode;
  UnicodeString Line = AddedLine;
  if (StdError ||
      !RemoveLastLine(Line, ReturnCode) ||
      !Line.IsEmpty())
  {
    assert(!FOnCaptureOutput.empty());
    FOnCaptureOutput(Line, StdError);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::AnyCommand(const UnicodeString Command,
  TCaptureOutputEvent OutputEvent)
{
  assert(FSecureShell->GetOnCaptureOutput().empty());
  if (OutputEvent != NULL)
  {
    FSecureShell->SetOnCaptureOutput(fastdelegate::bind(&TSCPFileSystem::CaptureOutput, this, _1, _2));
    FOnCaptureOutput = OutputEvent;
  }
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FOnCaptureOutput.clear();
      Self->FSecureShell->GetOnCaptureOutput().clear();
    } BOOST_SCOPE_EXIT_END
    ExecCommand2(fsAnyCommand, Command.c_str(),
      ecDefault | ecIgnoreWarnings);
  }
#ifndef _MSC_VER
  __finally
  {
    FOnCaptureOutput = NULL;
    FSecureShell->OnCaptureOutput = NULL;
  }
#endif
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSCPFileSystem::FileUrl(const UnicodeString FileName)
{
  return FTerminal->FileUrl(L"scp", FileName);
}
//---------------------------------------------------------------------------
TStrings * __fastcall TSCPFileSystem::GetFixedPaths()
{
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SpaceAvailable(const UnicodeString Path,
  TSpaceAvailable & /*ASpaceAvailable*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPResponse(bool * GotLastLine)
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
void __fastcall TSCPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  // scp.c: source(), toremote()
  assert(FilesToCopy && OperationProgress);

  Params &= ~(cpAppend | cpResume);
  UnicodeString Options = L"";
  bool CheckExistence = UnixComparePaths(TargetDir, FTerminal->GetCurrentDirectory()) &&
    (FTerminal->FFiles != NULL) && FTerminal->FFiles->GetLoaded();
  bool CopyBatchStarted = false;
  bool Failed = true;
  bool GotLastLine = false;

  UnicodeString TargetDirFull = UnixIncludeTrailingBackslash(TargetDir);

  if (CopyParam->GetPreserveRights()) { Options = L"-p"; }
  if (FTerminal->GetSessionData()->GetScp1Compatibility()) { Options += L" -1"; }

  SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
    Options.c_str(), DelimitStr(UnixExcludeTrailingBackslash(TargetDir)).c_str()));
  SkipFirstLine();

  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&GotLastLine) (&CopyBatchStarted)
      (&Failed) )
    {
      // Tell remote side, that we're done.
      if (Self->FTerminal->GetActive())
      {
        try
        {
          if (!GotLastLine)
          {
            if (CopyBatchStarted)
            {
              // What about case, remote side sends fatal error ???
              // (Not sure, if it causes remote side to terminate scp)
              Self->FSecureShell->SendLine(L"E");
              Self->SCPResponse();
            };
            /* TODO 1 : Show stderror to user? */
            Self->FSecureShell->ClearStdError();

            Self->ReadCommandOutput(coExpectNoOutput | coWaitForLastLine | coOnlyReturnCode |
              (Failed ? 0 : coRaiseExcept));
          }
        }
        catch (Exception &E)
        {
          // Only log error message (it should always succeed, but
          // some pending error maybe in queque) }
          Self->FTerminal->GetLog()->AddException(&E);
        }
      }
    } BOOST_SCOPE_EXIT_END
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

    for (int IFile = 0; (IFile < FilesToCopy->GetCount()) &&
      !OperationProgress->Cancel; IFile++)
    {
      UnicodeString FileName = FilesToCopy->GetStrings(IFile);
      bool CanProceed = false;

      UnicodeString FileNameOnly =
        CopyParam->ChangeFileName(ExtractFileName(FileName, false), osLocal, true);

      if (CheckExistence)
      {
        // previously there was assertion on FTerminal->FFiles->Loaded, but it
        // fails for scripting, if 'ls' is not issued before.
        // formally we should call CheckRemoteFile here but as checking is for
        // free here (almost) ...
        TRemoteFile * File = FTerminal->FFiles->FindFile(FileNameOnly);
        if (File != NULL)
        {
          unsigned int Answer;
          if (File->GetIsDirectory())
          {
            UnicodeString Message = FMTLOAD(PROMPT_DIRECTORY_OVERWRITE, FileNameOnly.c_str());
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
              if (!OperationProgress->Cancel) { OperationProgress->Cancel = csCancel; }
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
          SCPSource(FileName, TargetDirFull,
            CopyParam, Params, OperationProgress, 0);
          OperationProgress->Finish(FileName, true, OnceDoneOperation);
        }
        catch (EScpFileSkipped &E)
        {
          TQueryParams Params(qpAllowContinueOnError);
          SUSPEND_OPERATION (
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()), &E,
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
#ifndef _MSC_VER
  __finally
  {
    // Tell remote side, that we're done.
    if (FTerminal->Active)
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
      catch (Exception &E)
      {
        // Only log error message (it should always succeed, but
        // some pending error maybe in queque) }
        FTerminal->Log->AddException(&E);
      }
    }
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPSource(const UnicodeString FileName,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, int Level)
{
  UnicodeString DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(FileName, false), osLocal, Level == 0);

  FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

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
  TSafeHandleStream * Stream = new TSafeHandleStream(File);
  // try
  {
    BOOST_SCOPE_EXIT ( (&File) (&Stream) )
    {
      if (File != NULL)
      {
        ::CloseHandle(File);
      }
      delete Stream;
    } BOOST_SCOPE_EXIT_END
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      SCPDirectorySource(FileName, TargetDir, CopyParam, Params, OperationProgress, Level);
    }
    else
    {
      UnicodeString AbsoluteFileName = FTerminal->AbsolutePath(/* TargetDir + */DestFileName, false);
      assert(File);

      // File is regular file (not directory)
      FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", FileName.c_str()));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->LocalSize);
      OperationProgress->TransferingFile = false;

      TDateTime Modification = UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());

      // Will we use ASCII of BINARY file tranfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      MaskParams.Modification = Modification;
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(FileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
          L" transfer mode selected.");

      TUploadSessionAction Action(FTerminal->GetActionLog());
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
              Buf.SetLength(40);
              // Send last file access and modification time
              // TVarRec don't understand 'unsigned int' -> we use sprintf()
              swprintf_s(const_cast<wchar_t *>(Buf.c_str()), Buf.Length(), L"T%lu 0 %lu 0", static_cast<unsigned long>(MTime),
                static_cast<unsigned long>(ATime));
              FSecureShell->SendLine(Buf.c_str());
              SCPResponse();
            }

            // Send file modes (rights), filesize and file name
            // TVarRec don't understand 'unsigned int' -> we use sprintf()
            Buf.Clear();
            Buf.SetLength(MAX_PATH * 2);
            swprintf_s(const_cast<wchar_t *>(Buf.c_str()), Buf.Length(), L"C%s %ld %s",
              Rights.GetOctal().c_str(),
              static_cast<int>(OperationProgress->AsciiTransfer ? (__int64)AsciiBuf.GetSize() :
                OperationProgress->LocalSize),
              DestFileName.c_str());
            FSecureShell->SendLine(Buf.c_str());
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
                AsciiBuf.GetSize()));
              // Should be equal, just in case it's rounded (see above)
              OperationProgress->ChangeTransferSize(AsciiBuf.GetSize());
              while (!OperationProgress->IsTransferDone())
              {
                unsigned long BlockSize = OperationProgress->TransferBlockSize();
                FSecureShell->Send(
                  reinterpret_cast<unsigned char *>(AsciiBuf.GetData() + (unsigned int)OperationProgress->TransferedSize),
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
            FSecureShell->Send(reinterpret_cast<const unsigned char *>(BlockBuf.GetData()), BlockBuf.GetSize());
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
      catch (Exception &E)
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
        TTouchSessionAction(FTerminal->GetActionLog(), AbsoluteFileName, Modification);
      }
      if (CopyParam->GetPreserveRights())
      {
        TChmodSessionAction(FTerminal->GetActionLog(), AbsoluteFileName,
          Rights);
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    if (File != NULL)
    {
      CloseHandle(File);
    }
    delete Stream;
  }
#endif

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(Sysutils::DeleteFile(FileName));
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
void __fastcall TSCPFileSystem::SCPDirectorySource(const UnicodeString DirectoryName,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, int Level)
{
  int Attrs = 0;

  FTerminal->LogEvent(FORMAT(L"Entering directory \"%s\".", DirectoryName.c_str()));

  OperationProgress->SetFile(DirectoryName);
  UnicodeString DestFileName = CopyParam->ChangeFileName(
    ExtractFileName(DirectoryName, false), osLocal, Level == 0);

  // Get directory attributes
  FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, DirectoryName.c_str()),
    Attrs = FileGetAttr(DirectoryName);
    if (Attrs == -1) { RaiseLastOSError(); }
  )

  UnicodeString TargetDirFull = UnixIncludeTrailingBackslash(TargetDir + DestFileName);

  UnicodeString Buf;

  /* TODO 1: maybe send filetime */

  // Send directory modes (rights), filesize and file name
  Buf = FORMAT(L"D%s 0 %s",
    CopyParam->RemoteFileRights(Attrs).GetOctal().c_str(), DestFileName.c_str());
  FSecureShell->SendLine(Buf);
  SCPResponse();

  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&DirectoryName) )
    {
      if (Self->FTerminal->GetActive())
      {
        // Tell remote side, that we're done.
        Self->FTerminal->LogEvent(FORMAT(L"Leaving directory \"%s\".", DirectoryName.c_str()));
        Self->FSecureShell->SendLine(L"E");
        Self->SCPResponse();
      }
    } BOOST_SCOPE_EXIT_END
    int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
    TSearchRec SearchRec;
    memset(&SearchRec, 0, sizeof(SearchRec));
    bool FindOK = false;
    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
      UnicodeString path = IncludeTrailingBackslash(DirectoryName) + L"*.*";
      FindOK = FindFirst(path.c_str(),
        FindAttrs, SearchRec) == 0;
    );

    // try
    {
      BOOST_SCOPE_EXIT ( (&SearchRec) )
      {
        FindClose(SearchRec);
      } BOOST_SCOPE_EXIT_END
      while (FindOK && !OperationProgress->Cancel)
      {
        UnicodeString FileName = IncludeTrailingBackslash(DirectoryName) + SearchRec.Name;
        try
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
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
            if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()), &E,
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
        FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
          FindOK = (FindNext(SearchRec) == 0);
        );
      };
    }
#ifndef _MSC_VER
    __finally
    {
      FindClose(SearchRec);
    }
#endif

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
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
          THROWOSIFFALSE(FileSetAttr(DirectoryName, Attrs & ~faArchive) == 0);
        )
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    if (FTerminal->Active)
    {
      // Tell remote side, that we're done.
      FTerminal->LogEvent(FORMAT(L"Leaving directory \"%s\".", (DirectoryName)));
      FSecureShell->SendLine(L"E");
      SCPResponse();
    }
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const UnicodeString TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  bool CloseSCP = False;
  Params &= ~(cpAppend | cpResume);
  UnicodeString Options = L"";
  if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime()) { Options = L"-p"; }
  if (FTerminal->GetSessionData()->GetScp1Compatibility()) { Options += L" -1"; }

  FTerminal->LogEvent(FORMAT(L"Copying %d files/directories to local directory "
    L"\"%s\"", FilesToCopy->GetCount(), TargetDir.c_str()));
  FTerminal->LogEvent(CopyParam->GetLogStr());

  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) (&CloseSCP) (&OperationProgress) )
    {
      // In case that copying doesn't cause fatal error (ie. connection is
      // still active) but wasn't succesful (exception or user termination)
      // we need to ensure, that SCP on remote side is closed
      if (Self->FTerminal->GetActive() && (CloseSCP ||
          (OperationProgress->Cancel == csCancel) ||
          (OperationProgress->Cancel == csCancelTransfer)))
      {
        bool LastLineRead = false;

        // If we get LastLine, it means that remote side 'scp' is already
        // terminated, so we need not to terminate it. There is also
        // possibility that remote side waits for confirmation, so it will hang.
        // This should not happen (hope)
        UnicodeString Line = Self->FSecureShell->ReceiveLine();
        LastLineRead = Self->IsLastLine(Line);
        if (!LastLineRead)
        {
          Self->SCPSendError((OperationProgress->Cancel ? L"Terminated by user." : L"Exception"), true);
        }
        // Just in case, remote side already sent some more data (it's probable)
        // but we don't want to raise exception (user asked to terminate, it's not error)
        int ECParams = coOnlyReturnCode;
        if (!LastLineRead) { ECParams |= coWaitForLastLine; }
        Self->ReadCommandOutput(ECParams);
      }
    } BOOST_SCOPE_EXIT_END
    for (int IFile = 0; (IFile < FilesToCopy->GetCount()) &&
         !OperationProgress->Cancel; IFile++)
    {
      UnicodeString FileName = FilesToCopy->GetStrings(IFile);
      TRemoteFile * File = (TRemoteFile *)FilesToCopy->GetObjects(IFile);
      assert(File);

      try
      {
        bool Success = true; // Have to be set to True (see ::SCPSink)
        SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
          Options.c_str(), DelimitStr(FileName).c_str()));
        SkipFirstLine();

        // Filename is used for error messaging and excluding files only
        // Send in full path to allow path-based excluding
        UnicodeString FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
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
            FTerminal->SetExceptionOnFail(true);
            // try
            {
              BOOST_SCOPE_EXIT ( (&Self) )
              {
                Self->FTerminal->SetExceptionOnFail(false);
              } BOOST_SCOPE_EXIT_END
              FILE_OPERATION_LOOP(FMTLOAD(DELETE_FILE_ERROR, FileName.c_str()),
                // pass full file name in FileName, in case we are not moving
                // from current directory
                FTerminal->DeleteFile(FileName, File)
              );
            }
#ifndef _MSC_VER
            __finally
            {
              FTerminal->ExceptionOnFail = false;
            }
#endif
          }
          catch (EFatal &E)
          {
            throw;
          }
          catch (...)
          {
            // If user selects skip (or abort), nothing special actualy occurs
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
#ifndef _MSC_VER
  __finally
  {
    // In case that copying doesn't cause fatal error (ie. connection is
    // still active) but wasn't succesful (exception or user termination)
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
      int ECParams = coOnlyReturnCode;
      if (!LastLineRead) ECParams |= coWaitForLastLine;
      ReadCommandOutput(ECParams);
    }
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPError(const UnicodeString Message, bool Fatal)
{
  SCPSendError(Message, Fatal);
  THROW_FILE_SKIPPED(NULL, Message);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPSendError(const UnicodeString Message, bool Fatal)
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
void __fastcall TSCPFileSystem::SCPSink(const UnicodeString TargetDir,
  const UnicodeString FileName, const UnicodeString SourceDir,
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
    if (FileData.SetTime) { FileData.SetTime--; }

    // In case of error occured before control record arrived.
    // We can finally use full path here, as we get current path in FileName param
    // (we used to set the file into OperationProgress->GetFileName(), but it collided
    // with progress outputing, particularly for scripting)
    UnicodeString AbsoluteFileName = FileName;

    try
    {
      // Receive control record
      UnicodeString Line = FSecureShell->ReceiveLine();

      if (Line.Length() == 0) { FTerminal->FatalError(NULL, LoadStr(SCP_EMPTY_LINE)); }

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

        // First characted distinguish type of control record
        wchar_t Ctrl = Line[1];
        Line.Delete(1, 1);

        switch (Ctrl) {
          case 1:
            // Error (already logged by ReceiveLine())
            THROW_FILE_SKIPPED(NULL, FMTLOAD(REMOTE_ERROR, Line.c_str()));

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
              SCPError(LoadStr(SCP_ILLEGAL_TIME_FORMAT), False);
            }

          case L'C':
          case L'D':
            break; // continue pass switch{}

          default:
            FTerminal->FatalError(NULL, FMTLOAD(SCP_INVALID_CONTROL_RECORD, Ctrl, Line.c_str()));
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

          OperationProgress->SetFile(OnlyFileName);
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
          THROW_SKIP_FILE(NULL, LoadStr(USER_TERMINATED));
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

        FileData.Attrs = FileGetAttr(DestFileName);
        // If getting attrs failes, we suppose, that file/folder doesn't exists
        FileData.Exists = (FileData.Attrs != -1);
        if (Dir)
        {
          if (FileData.Exists && !(FileData.Attrs & faDirectory))
          {
            SCPError(FMTLOAD(NOT_DIRECTORY_ERROR, DestFileName.c_str()), false);
          }

          if (!FileData.Exists)
          {
            FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFileName.c_str()),
              if (!ForceDirectories(DestFileName)) { RaiseLastOSError(); }
            );
            /* SCP: can we set the timestamp for directories ? */
          }
          UnicodeString FullFileName = SourceDir + OperationProgress->FileName;
          SCPSink(DestFileName, FullFileName, UnixIncludeTrailingBackslash(FullFileName),
            CopyParam, Success, OperationProgress, Params, Level + 1);
          continue;
        }
          else
        if (Ctrl == L'C')
        {
          TDownloadSessionAction Action(FTerminal->GetActionLog());
          Action.FileName(AbsoluteFileName);

          try
          {
            HANDLE File = NULL;
            TStream * FileStream = NULL;

            /* TODO 1 : Turn off read-only attr */

            // try
            {
              BOOST_SCOPE_EXIT ( (&File) (&FileStream) )
              {
                if (File) { CloseHandle(File); }
                if (FileStream) { delete FileStream; }
              } BOOST_SCOPE_EXIT_END
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

                  unsigned int Answer;
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
                      EXCEPTION;
                  }
                }

                Action.Destination(DestFileName);

                if (!FTerminal->CreateLocalFile(DestFileName, OperationProgress,
                       &File, FLAGSET(Params, cpNoConfirmation)))
                {
                  SkipConfirmed = true;
                  EXCEPTION;
                }

                FileStream = new TSafeHandleStream(File);
              }
              catch (Exception &E)
              {
                // In this step we can still cancel transfer, so we do it
                SCPError(E.GetMessage(), false);
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
                    unsigned int PrevBlockSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetSessionData()->GetEOLType(),
                      FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                    OperationProgress->SetLocalSize(
                      OperationProgress->LocalSize - PrevBlockSize + BlockBuf.GetSize());
                  }

                  // This is crucial, if it fails during file transfer, it's fatal error
                  FILE_OPERATION_LOOP_EX (false, FMTLOAD(WRITE_ERROR, DestFileName.c_str()),
                    BlockBuf.WriteToStream(FileStream, BlockBuf.GetSize());
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
#ifndef _MSC_VER
            __finally
            {
              if (File) CloseHandle(File);
              if (FileStream) delete FileStream;
            }
#endif
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

          if (FileData.Attrs == -1) { FileData.Attrs = faArchive; }
          int NewAttrs = CopyParam->LocalFileAttrs(FileData.RemoteRights);
          if ((NewAttrs & FileData.Attrs) != NewAttrs)
          {
            FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFileName.c_str()),
              THROWOSIFFALSE(FileSetAttr(DestFileName, FileData.Attrs | NewAttrs) == 0);
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
      if (!FTerminal->HandleException(&E)) { throw; }
    }
  }
}
