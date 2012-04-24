//---------------------------------------------------------------------------
#include "stdafx.h"

#include <stdio.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "ScpFileSystem.h"

#include "Terminal.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "TextsCore.h"
#include "SecureShell.h"

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

//---------------------------------------------------------------------------
#define THROW_FILE_SKIPPED(MESSAGE, EXCEPTION) \
  throw EScpFileSkipped(MESSAGE, EXCEPTION)

#define THROW_SCP_ERROR(MESSAGE, EXCEPTION) \
  throw EScp(MESSAGE, EXCEPTION)
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
    TSessionData *FSessionData;
    std::wstring FReturnVar;
public:
    TCommandSet(TSessionData *aSessionData);
    void Default();
    void CopyFrom(TCommandSet *Source);
    std::wstring Command(TFSCommand Cmd, ...);
    std::wstring Command(TFSCommand Cmd, va_list args);
    nb::TStrings *CreateCommandList();
    std::wstring FullCommand(TFSCommand Cmd, ...);
    std::wstring FullCommand(TFSCommand Cmd, va_list args);
    static std::wstring ExtractCommand(const std::wstring Command);
    int GetMaxLines(TFSCommand Cmd);
    int GetMinLines(TFSCommand Cmd);
    bool GetModifiesFiles(TFSCommand Cmd);
    bool GetChangesDirectory(TFSCommand Cmd);
    bool GetOneLineCommand(TFSCommand Cmd);
    std::wstring GetCommand(TFSCommand Cmd);
    void SetCommand(TFSCommand Cmd, const std::wstring value);
    std::wstring GetFirstLine();
    bool GetInteractiveCommand(TFSCommand Cmd);
    std::wstring GetLastLine();
    TSessionData *GetSessionData() { return FSessionData; }
    void SetSessionData(TSessionData *value) { FSessionData = value; }
    std::wstring GetReturnVar();
    void SetReturnVar(const std::wstring value) { FReturnVar = value; }
};
//===========================================================================
const wchar_t NationalVars[NationalVarCount][15] =
{
    L"LANG", L"LANGUAGE", L"LC_CTYPE", L"LC_COLLATE", L"LC_MONETARY", L"LC_NUMERIC",
    L"LC_TIME", L"LC_MESSAGES", L"LC_ALL", L"HUMAN_BLOCKS"
};
const wchar_t FullTimeOption[] = L"--full-time";
//---------------------------------------------------------------------------
#define F false
#define T true
// TODO: remove "mf" and "cd", it is implemented in TTerminal already
const TCommandType DefaultCommandSet[ShellCommandCount] =
{
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
void TCommandSet::CopyFrom(TCommandSet *Source)
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
void TCommandSet::SetCommand(TFSCommand Cmd, const std::wstring value)
{
    CHECK_CMD;
    wcscpy(const_cast<wchar_t *>(CommandSet[Cmd].Command), value.substr(0, MaxCommandLen - 1).c_str());
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
    std::wstring result;
    va_list args;
    va_start(args, Cmd);
    result = Command(Cmd, args);
    va_end(args);
    return result;
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::Command(TFSCommand Cmd, va_list args)
{
    // DEBUG_PRINTF(L"Cmd = %d, GetCommand(Cmd) = %s", Cmd, GetCommand(Cmd).c_str());
    std::wstring result;
    result = ::Format(GetCommand(Cmd).c_str(), args);
    // DEBUG_PRINTF(L"result = %s", result.c_str());
    return result.c_str();
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::FullCommand(TFSCommand Cmd, ...)
{
    std::wstring Result;
    va_list args;
    va_start(args, Cmd);
    Result = FullCommand(Cmd, args);
    va_end(args);
    return Result.c_str();
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::FullCommand(TFSCommand Cmd, va_list args)
{
    std::wstring Separator;
    if (GetOneLineCommand(Cmd))
    {
        Separator = L" ; ";
    }
    else
    {
        Separator = L"\n";
    }
    std::wstring Line = Command(Cmd, args);
    std::wstring LastLineCmd =
        Command(fsLastLine, GetLastLine().c_str(), GetReturnVar().c_str());
    std::wstring FirstLineCmd;
    if (GetInteractiveCommand(Cmd))
    {
        FirstLineCmd = Command(fsFirstLine, GetFirstLine().c_str()) + Separator;
        // DEBUG_PRINTF(L"FirstLineCmd1 = '%s'", FirstLineCmd.c_str());
    }

    std::wstring Result;
    if (!Line.empty())
    {
        Result = FORMAT(L"%s%s%s%s", FirstLineCmd.c_str(), Line.c_str(), Separator.c_str(), LastLineCmd.c_str());
    }
    else
    {
        Result = FORMAT(L"%s%s", FirstLineCmd.c_str(), LastLineCmd.c_str());
    }
    // DEBUG_PRINTF(L"Result = %s", Result.c_str());
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
    if (!FReturnVar.empty())
    {
        return std::wstring(L"$") + FReturnVar;
    }
    else if (GetSessionData()->GetDetectReturnVar())
    {
        return L"0";
    }
    else
    {
        return std::wstring(L"$") + GetSessionData()->GetReturnVar();
    }
}
//---------------------------------------------------------------------------
std::wstring TCommandSet::ExtractCommand(const std::wstring Command)
{
    std::wstring command = Command;
    size_t P = command.find_first_of(L" ");
    if (P != std::wstring::npos)
    {
        command.resize(P);
    }
    return command;
}
//---------------------------------------------------------------------------
nb::TStrings *TCommandSet::CreateCommandList()
{
    nb::TStrings *CommandList = new nb::TStringList();
    for (int Index = 0; Index < ShellCommandCount; Index++)
    {
        std::wstring Cmd = GetCommand(static_cast<TFSCommand>(Index));
        if (!Cmd.empty())
        {
            Cmd = ExtractCommand(Cmd);
            if ((Cmd != L"%s") && (CommandList->IndexOf(Cmd.c_str()) == NPOS))
            {
                CommandList->Add(Cmd);
            }
        }
    }
    return CommandList;
}
//===========================================================================
TSCPFileSystem::TSCPFileSystem(TTerminal *ATerminal) :
    TCustomFileSystem(ATerminal)
{
    Self = this;
}

void __fastcall TSCPFileSystem::Init(TSecureShell *SecureShell)
{
    FSecureShell = SecureShell;
    FCommandSet = new TCommandSet(FTerminal->GetSessionData());
    FLsFullTime = FTerminal->GetSessionData()->GetSCPLsFullTime();
    FOutput = new nb::TStringList();
    FUtfStrings = false;
    FUtfNever = false;
    FProcessingCommand = false;

    FFileSystemInfo.ProtocolBaseName = L"SCP";
    FFileSystemInfo.ProtocolName = FFileSystemInfo.ProtocolBaseName;
    // capabilities of SCP protocol are fixed
    for (int Index = 0; Index < fcCount; Index++)
    {
        FFileSystemInfo.IsCapable[Index] = IsCapable(static_cast<TFSCapability>(Index));
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
bool TSCPFileSystem::GetActive()
{
    return FSecureShell->GetActive();
}
//---------------------------------------------------------------------------
const TSessionInfo &TSCPFileSystem::GetSessionInfo()
{
    return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo &TSCPFileSystem::GetFileSystemInfo(bool Retrieve)
{
    if (FFileSystemInfo.AdditionalInfo.empty() && Retrieve)
    {
        std::wstring UName;
        FTerminal->SetExceptionOnFail(true);
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->FTerminal->SetExceptionOnFail(false);
            } BOOST_SCOPE_EXIT_END
            try
            {
                AnyCommand(L"uname -a", NULL);
                for (size_t Index = 0; Index < GetOutput()->GetCount(); Index++)
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

        FFileSystemInfo.RemoteSystem = UName;
    }

    return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::TemporaryTransferFile(const std::wstring /*FileName*/)
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
void __fastcall TSCPFileSystem::Idle()
{
    // Keep session alive
    if ((FTerminal->GetSessionData()->GetPingType ()!= ptOff) &&
            (nb::Now() - FSecureShell->GetLastDataSent()> FTerminal->GetSessionData()->GetPingIntervalDT()))
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
std::wstring TSCPFileSystem::AbsolutePath(const std::wstring Path, bool /*Local*/)
{
    return ::AbsolutePath(GetCurrentDirectory(), Path);
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsCapable(int Capability) const
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
std::wstring TSCPFileSystem::DelimitStr(const std::wstring Str)
{
    std::wstring str = Str;
    if (!str.empty())
    {
        str = ::DelimitStr(str, L"\\`$\"");
        if (str[0] == L'-') { str = L"./" + str; }
    }
    return str;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::EnsureLocation()
{
    if (!FCachedDirectoryChange.empty())
    {
        FTerminal->LogEvent(FORMAT(L"Locating to cached directory \"%s\".",
                                   FCachedDirectoryChange.c_str()));
        std::wstring Directory = FCachedDirectoryChange;
        FCachedDirectoryChange = L"";
        try
        {
            ChangeDirectory(Directory);
        }
        catch (...)
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
void __fastcall TSCPFileSystem::SendCommand(const std::wstring Cmd)
{
    EnsureLocation();

    std::wstring Line;
    FSecureShell->ClearStdError();
    FReturnCode = 0;
    FOutput->Clear();
    // We suppose, that 'Cmd' already contains command that ensures,
    // that 'LastLine' will be printed
    // DEBUG_PRINTF(L"Cmd = %s", Cmd.c_str());
    FSecureShell->SendLine(Cmd);
    FProcessingCommand = true;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsTotalListingLine(const std::wstring Line)
{
    // On some hosts there is not "total" but "totalt". What's the reason??
    // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
    return !::AnsiCompareIC(Line.substr(0, 5), L"total");
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::RemoveLastLine(std::wstring &Line,
                                    int &ReturnCode, std::wstring LastLine)
{
    bool IsLastLine = false;
    if (LastLine.empty()) { LastLine = LAST_LINE; }
    // #55: fixed so, even when last line of command output does not
    // contain CR/LF, we can recognize last line
    size_t Pos = Line.find(LastLine);
    // DEBUG_PRINTF(L"Line = %s, LastLine = %s, Pos = %d", Line.c_str(), LastLine.c_str(), Pos);
    if (Pos != std::wstring::npos)
    {
        // 2003-07-14: There must be nothing after return code number to
        // consider string as last line. This fixes bug with 'set' command
        // in console window
        std::wstring ReturnCodeStr = ::TrimRight(Line.substr(Pos + LastLine.size() + 1,
                                     Line.size() - Pos + LastLine.size()));
        // DEBUG_PRINTF(L"ReturnCodeStr = '%s'", ReturnCodeStr.c_str());
        if (TryStrToInt(ReturnCodeStr, ReturnCode) || (ReturnCodeStr == L"0"))
        {
            IsLastLine = true;
            // DEBUG_PRINTF(L"Line1 = %s", Line.c_str());
            // if ((Pos != std::wstring::npos) && (Pos != 0))
            {
                Line.resize(Pos);
            }
            // DEBUG_PRINTF(L"Line2 = %s", Line.c_str());
        }
    }
    return IsLastLine;
}
//---------------------------------------------------------------------------
bool TSCPFileSystem::IsLastLine(std::wstring &Line)
{
    bool Result = false;
    try
    {
        Result = RemoveLastLine(Line, FReturnCode, FCommandSet->GetLastLine());
    }
    catch (const std::exception &E)
    {
        FTerminal->TerminalError(&E, LoadStr(CANT_DETECT_RETURN_CODE));
    }
    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SkipFirstLine()
{
    std::wstring Line = FSecureShell->ReceiveLine();
    if (Line != FCommandSet->GetFirstLine())
    {
        FTerminal->TerminalError(NULL, FMTLOAD(FIRST_LINE_EXPECTED, Line.c_str()));
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadCommandOutput(int Params, const std::wstring *Cmd)
{
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FProcessingCommand = false;
        } BOOST_SCOPE_EXIT_END
        if (Params & coWaitForLastLine)
        {
            std::wstring Line;
            bool IsLast = true;
            size_t Total = 0;
            // #55: fixed so, even when last line of command output does not
            // contain CR/LF, we can recognize last line
            do
            {
                Line = FSecureShell->ReceiveLine();
                // DEBUG_PRINTF(L"Line = %s", Line.c_str());
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
                if (!Message.empty()) { Message += L"\n"; }
                Message += FOutput->GetText();
            }
            while (!Message.empty() && (::LastDelimiter(Message, L"\n\r") == Message.size() - 1))
            {
                Message.resize(Message.size() - 1);
            }

            bool WrongReturnCode =
                (GetReturnCode() > 1) || (GetReturnCode() == 1 && !(Params & coIgnoreWarnings));

            if (Params & coOnlyReturnCode && WrongReturnCode)
            {
                FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED_CODEONLY, GetReturnCode()));
            }
            else if (!(Params & coOnlyReturnCode) &&
                     ((!Message.empty() && ((FOutput->GetCount() == 0) || !(Params & coIgnoreWarnings))) ||
                      WrongReturnCode))
            {
                assert(Cmd != NULL);
                FTerminal->TerminalError(FMTLOAD(COMMAND_FAILED, Cmd->c_str(), GetReturnCode(), Message.c_str()));
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ExecCommand(const std::wstring Cmd, int Params,
                                 const std::wstring CmdString)
{
    if (Params < 0) { Params = ecDefault; }
    if (FTerminal->GetUseBusyCursor())
    {
        ::Busy(true);
    }
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            if (Self->FTerminal->GetUseBusyCursor())
            {
                ::Busy(false);
            }
        } BOOST_SCOPE_EXIT_END
        // DEBUG_PRINTF(L"Cmd = %s, CmdString = %s", Cmd.c_str(), CmdString.c_str());
        SendCommand(Cmd);

        int COParams = coWaitForLastLine;
        if (Params & ecRaiseExcept) { COParams |= coRaiseExcept; }
        if (Params & ecIgnoreWarnings) { COParams |= coIgnoreWarnings; }
        if (Params & ecReadProgress) { COParams |= coReadProgress; }
        ReadCommandOutput(COParams, &CmdString);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ExecCommand(TFSCommand Cmd, int Params, ...)
{
    if (Params < 0) { Params = ecDefault; }
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
        if (((MinL >= 0) && (MinL > static_cast<int>(FOutput->GetCount()))) ||
                ((MaxL >= 0) && (MaxL > static_cast<int>(FOutput->GetCount()))))
        {
            FTerminal->TerminalError(::FmtLoadStr(INVALID_OUTPUT_ERROR,
                                                  FullCommand.c_str(), GetOutput()->GetText().c_str()));
        }
    }
}
//---------------------------------------------------------------------------
std::wstring __fastcall TSCPFileSystem::GetCurrentDirectory()
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

  FUtfNever =
    (FTerminal->GetSessionData()->GetNotUtf() == asOn);
  FUtfStrings =
    (FTerminal->GetSessionData()->GetNotUtf() == asOff) ||
    ((FTerminal->GetSessionData()->GetNotUtf() == asAuto) &&
      !FUtfNever);

  if (FUtfStrings)
  {
    FTerminal->LogEvent(L"We will use UTF-8 strings when appropriate");
  }
  else if (FUtfNever)
  {
    FTerminal->LogEvent(L"We will never use UTF-8 strings");
  }
  else
  {
    FTerminal->LogEvent(L"We will use UTF-8 strings for status messages only");
  }

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
        ExecCommand(fsNull, 0, NULL);
    }
    catch (const std::exception &E)
    {
        FTerminal->CommandError(&E, LoadStr(SKIP_STARTUP_MESSAGE_ERROR));
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::LookupUsersGroups()
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
void __fastcall TSCPFileSystem::DetectReturnVar()
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
                FTerminal->LogEvent(FORMAT(L"Trying \"$%s\".", ReturnVars[Index].c_str()));
                ExecCommand(fsVarValue, 0, ReturnVars[Index].c_str());
                // DEBUG_PRINTF(L"GetOutput()->GetCount = %d, GetOutput()->GetString(0) = %s", GetOutput()->GetCount(), GetOutput()->GetString(0).c_str());
                std::wstring str = GetOutput()->GetCount() > 0 ? GetOutput()->GetString(0) : L"";
                int val = StrToIntDef(str, 256);
                if ((GetOutput()->GetCount() != 1) || str.empty() || (val > 255))
                {
                    FTerminal->LogEvent(L"The response is not numerical exit code");
                    nb::Abort();
                }
            }
            catch (const EFatal &E)
            {
                // if fatal error occurs, we need to exit ...
                throw;
            }
            catch (const std::exception &E)
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
            nb::Abort();
        }
        else
        {
            FCommandSet->SetReturnVar(NewReturnVar);
            FTerminal->LogEvent(FORMAT(L"Return code variable \"%s\" selected.",
                                       FCommandSet->GetReturnVar().c_str()));
        }
    }
    catch (const std::exception &E)
    {
        FTerminal->CommandError(&E, LoadStr(DETECT_RETURNVAR_ERROR));
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ClearAlias(const std::wstring Alias)
{
    if (!Alias.empty())
    {
        // this command usually fails, because there will never be
        // aliases on all commands -> see last false parametr
        // DEBUG_PRINTF(L"Alias = %s", Alias.c_str());
        ExecCommand(fsUnalias, 0, Alias.c_str(), false);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ClearAliases()
{
    try
    {
        FTerminal->LogEvent(L"Clearing all aliases.");
        ClearAlias(TCommandSet::ExtractCommand(FTerminal->GetSessionData()->GetListingCommand()));
        nb::TStrings *CommandList = FCommandSet->CreateCommandList();
        {
            BOOST_SCOPE_EXIT ( (&CommandList) )
            {
                delete CommandList;
            } BOOST_SCOPE_EXIT_END
            for (size_t Index = 0; Index < CommandList->GetCount(); Index++)
            {
                ClearAlias(CommandList->GetString(Index));
            }
        }
    }
    catch (const std::exception &E)
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
        for (size_t Index = 0; Index < NationalVarCount; Index++)
        {
            ExecCommand(fsUnset, 0, NationalVars[Index], false);
        }
    }
    catch (const std::exception &E)
    {
        FTerminal->CommandError(&E, LoadStr(UNSET_NATIONAL_ERROR));
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadCurrentDirectory()
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
void __fastcall TSCPFileSystem::HomeDirectory()
{
    ExecCommand(fsHomeDirectory);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::AnnounceFileListOperation()
{
    // noop
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ChangeDirectory(const std::wstring Directory)
{
    std::wstring ToDir;
    if (!Directory.empty() &&
            ((Directory[0] != L'~') || (Directory.substr(0, 2) == L"~ ")))
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
void __fastcall TSCPFileSystem::CachedChangeDirectory(const std::wstring Directory)
{
    FCachedDirectoryChange = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadDirectory(TRemoteFileList *FileList)
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
            const wchar_t *Options =
                ((FLsFullTime == asAuto) || (FLsFullTime == asOn)) ? FullTimeOption : L"";
            bool ListCurrentDirectory = (FileList->GetDirectory() == FTerminal->GetCurrentDirectory());
            if (ListCurrentDirectory)
            {
                FTerminal->LogEvent(L"Listing current directory.");
                ExecCommand(fsListCurrentDirectory,
                            0, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, Params);
            }
            else
            {
                FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".",
                                           FileList->GetDirectory().c_str()));
                ExecCommand(fsListDirectory,
                            0, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options,
                            DelimitStr(FileList->GetDirectory().c_str()).c_str(),
                            Params);
            }

            TRemoteFile *File = NULL;

            // If output is not empty, we have succesfully got file listing,
            // otherwise there was an error, in case it was "permission denied"
            // we try to get at least parent directory (see "else" statement below)
            if (FOutput->GetCount() > 0)
            {
                // Copy LS command output, because eventual symlink analysis would
                // modify FTerminal->Output
                nb::TStringList *OutputCopy = new nb::TStringList();
                {
                    BOOST_SCOPE_EXIT ( (&OutputCopy) )
                    {
                        delete OutputCopy;
                    } BOOST_SCOPE_EXIT_END
                    OutputCopy->Assign(FOutput);

                    // delete leading "total xxx" line
                    // On some hosts there is not "total" but "totalt". What's the reason??
                    // see mail from "Jan Wiklund (SysOp)" <jan@park.se>
                    if (IsTotalListingLine(OutputCopy->GetString(0)))
                    {
                        OutputCopy->Delete(0);
                    }

                    for (size_t Index = 0; Index < OutputCopy->GetCount(); Index++)
                    {
                        File = CreateRemoteFile(OutputCopy->GetString(Index));
                        FileList->AddFile(File);
                    }
                }
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
                    Empty = (File == NULL || (wcscmp(File->GetFileName().c_str(), PARENTDIRECTORY) == 0));
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
        catch (const std::exception &E)
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
void __fastcall TSCPFileSystem::ReadSymlink(TRemoteFile *SymlinkFile,
                                 TRemoteFile *& File)
{
    CustomReadFile(SymlinkFile->GetLinkTo(), File, SymlinkFile);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ReadFile(const std::wstring FileName,
                              TRemoteFile *& File)
{
    CustomReadFile(FileName, File, NULL);
}
//---------------------------------------------------------------------------
TRemoteFile *TSCPFileSystem::CreateRemoteFile(
    const std::wstring ListingStr, TRemoteFile *LinkedByFile)
{
    TRemoteFile *File = new TRemoteFile(LinkedByFile);
    try
    {
        File->SetTerminal(FTerminal);
        File->SetListingStr(ListingStr);
        File->ShiftTime(FTerminal->GetSessionData()->GetTimeDifference());
        File->Complete();
    }
    catch (...)
    {
        delete File;
        throw;
    }

    return File;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CustomReadFile(const std::wstring FileName,
                                    TRemoteFile *& File, TRemoteFile *ALinkedByFile)
{
    File = NULL;
    int Params = ecDefault |
                 FLAGMASK(FTerminal->GetSessionData()->GetIgnoreLsWarnings(), ecIgnoreWarnings);
    // the auto-detection of --full-time support is not implemented for fsListFile,
    // so we use it only if we already know that it is supported (asOn).
    const wchar_t *Options = (FLsFullTime == asOn) ? FullTimeOption : L"";
    ExecCommand(fsListFile,
                Params, FTerminal->GetSessionData()->GetListingCommand().c_str(), Options, DelimitStr(FileName).c_str());
    if (FOutput->GetCount())
    {
        size_t LineIndex = 0;
        if (IsTotalListingLine(FOutput->GetString(LineIndex)) && FOutput->GetCount() > 1)
        {
            LineIndex++;
        }

        File = CreateRemoteFile(FOutput->GetString(LineIndex), ALinkedByFile);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::DeleteFile(const std::wstring FileName,
                                const TRemoteFile *File, int Params, TRmSessionAction &Action)
{
    USEDPARAM(File);
    USEDPARAM(Params);
    Action.Recursive();
    assert(FLAGCLEAR(Params, dfNoRecursive) || (File && File->GetIsSymLink()));
    ExecCommand(fsDeleteFile, 0, DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::RenameFile(const std::wstring FileName,
                                const std::wstring NewName)
{
    ExecCommand(fsRenameFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CopyFile(const std::wstring FileName,
                              const std::wstring NewName)
{
    ExecCommand(fsCopyFile, 0, DelimitStr(FileName).c_str(), DelimitStr(NewName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CreateDirectory(const std::wstring DirName)
{
    ExecCommand(fsCreateDirectory, 0, DelimitStr(DirName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CreateLink(const std::wstring FileName,
                                const std::wstring PointTo, bool Symbolic)
{
    ExecCommand(fsCreateLink, 0,
                Symbolic ? L"-s" : L"", DelimitStr(PointTo).c_str(), DelimitStr(FileName).c_str());
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ChangeFileToken(const std::wstring DelimitedName,
                                     const TRemoteToken &Token, TFSCommand Cmd, const std::wstring RecursiveStr)
{
    std::wstring Str;
    if (Token.GetIDValid())
    {
        Str = IntToStr(static_cast<int>(Token.GetID()));
    }
    else if (Token.GetNameValid())
    {
        Str = Token.GetName();
    }

    if (!Str.empty())
    {
        ExecCommand(Cmd, 0, RecursiveStr.c_str(), Str.c_str(), DelimitedName.c_str());
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::ChangeFileProperties(const std::wstring FileName,
        const TRemoteFile *File, const TRemoteProperties *Properties,
        TChmodSessionAction &Action)
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
        {
            Rights.AddExecute();
        }

        Action.Rights(Rights);
        if (Recursive)
        {
            Action.Recursive();
        }

        if ((Rights.GetNumberSet() | Rights.GetNumberUnset()) != TRights::rfNo)
        {
            ExecCommand(fsChangeMode,
                        0, RecursiveStr.c_str(), Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
        }

        // if file is directory and we do recursive mode settings with
        // add-x-to-directories option on, add those X
        if (Recursive && IsDirectory && Properties->AddXToDirectories)
        {
            Rights.AddExecute();
            ExecCommand(fsChangeMode,
                        0, L"", Rights.GetSimplestStr().c_str(), DelimitedName.c_str());
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
bool TSCPFileSystem::LoadFilesProperties(nb::TStrings * /*FileList*/ )
{
    assert(false);
    return false;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CalculateFilesChecksum(const std::wstring /*Alg*/,
        nb::TStrings * /*FileList*/, nb::TStrings * /*Checksums*/,
        calculatedchecksum_slot_type * /*OnCalculatedChecksum*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CustomCommandOnFile(const std::wstring FileName,
        const TRemoteFile *File, const std::wstring Command, int Params,
        const captureoutput_slot_type &OutputEvent)
{
    assert(File);
    bool Dir = File->GetIsDirectory() && !File->GetIsSymLink();
    if (Dir && (Params & ccRecursive))
    {
        TCustomCommandParams AParams(Command, Params, OutputEvent);
        // AParams.Command = Command;
        // AParams.Params = Params;
        // AParams.OutputEvent.connect(OutputEvent);
        FTerminal->ProcessDirectory(FileName, boost::bind(&TTerminal::CustomCommandOnFile, FTerminal, _1, _2, _3),
                                    &AParams);
    }

    if (!Dir || (Params & ccApplyToDirectories))
    {
        TCustomCommandData Data(FTerminal);
        std::wstring Cmd = TRemoteCustomCommand(
                               Data, FTerminal->GetCurrentDirectory(), FileName, L"").
                           Complete(Command, true);

        AnyCommand(Cmd, &OutputEvent);
    }
}
//---------------------------------------------------------------------------
void TSCPFileSystem::CaptureOutput(const std::wstring AddedLine, bool StdError)
{
    int ReturnCode;
    std::wstring Line = AddedLine;
    // DEBUG_PRINTF(L"Line = %s", Line.c_str());
    if (StdError ||
            !RemoveLastLine(Line, ReturnCode) ||
            !Line.empty())
    {
        assert(!FOnCaptureOutput.empty());
        FOnCaptureOutput(Line, StdError);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::AnyCommand(const std::wstring Command,
                                const captureoutput_slot_type *OutputEvent)
{
    assert(FSecureShell->GetOnCaptureOutput().empty());
    if (OutputEvent)
    {
        FSecureShell->SetOnCaptureOutput(boost::bind(&TSCPFileSystem::CaptureOutput, this, _1, _2));
        FOnCaptureOutput.connect(*OutputEvent);
    }

    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->FOnCaptureOutput.disconnect_all_slots();
            Self->FSecureShell->GetOnCaptureOutput().disconnect_all_slots();
        } BOOST_SCOPE_EXIT_END
        ExecCommand(fsAnyCommand, 0, Command.c_str(),
                    ecDefault | ecIgnoreWarnings);
    }
}
//---------------------------------------------------------------------------
std::wstring TSCPFileSystem::FileUrl(const std::wstring FileName)
{
    return FTerminal->FileUrl(L"scp", FileName);
}
//---------------------------------------------------------------------------
nb::TStrings *TSCPFileSystem::GetFixedPaths()
{
    return NULL;
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SpaceAvailable(const std::wstring Path,
                                    TSpaceAvailable & /*ASpaceAvailable*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPResponse(bool *GotLastLine)
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
        std::wstring MsgW = FSecureShell->ReceiveLine();
        std::string Msg = nb::W2MB(MsgW.c_str(), FTerminal->GetSessionData()->GetCodePageAsNumber());
        std::string Line = Resp + Msg;
        std::wstring LineW = nb::MB2W(Line.c_str(), FTerminal->GetSessionData()->GetCodePageAsNumber());
        if (IsLastLine(LineW))
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
            catch (...)
            {
                // when ReadCommandOutput() fails than remote SCP is terminated already
                if (GotLastLine != NULL)
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
            // DEBUG_PRINTF(L"Msg = %s", MsgW.c_str());
            THROW_FILE_SKIPPED(MsgW, NULL);
        }
        else
        {
            THROW_SCP_ERROR(LineW, NULL);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CopyToRemote(nb::TStrings *FilesToCopy,
                                  const std::wstring TargetDir, const TCopyParamType *CopyParam,
                                  int Params, TFileOperationProgressType *OperationProgress,
                                  TOnceDoneOperation &OnceDoneOperation)
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

    // DEBUG_PRINTF(L"CopyParam->GetPreserveRights = %d", CopyParam->GetPreserveRights());
    if (CopyParam->GetPreserveRights()) { Options = L"-p"; }
    if (FTerminal->GetSessionData()->GetScp1Compatibility()) { Options += L" -1"; }

    // DEBUG_PRINTF(L"TargetDir = %s, Options = %s", TargetDir.c_str(), Options.c_str());
    SendCommand(FCommandSet->FullCommand(fsCopyToRemote,
                                         Options.c_str(), DelimitStr(UnixExcludeTrailingBackslash(TargetDir)).c_str()));
    SkipFirstLine();

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
                catch (const std::exception &E)
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
            // DEBUG_PRINTF(L"GotLastLine = %d", GotLastLine);

            // This can happen only if SCP command is not executed and return code is 0
            // It has never happened to me (return code is usually 127)
            if (GotLastLine)
            {
                throw std::exception("");
            }
        }
        catch (const std::exception &E)
        {
            // DEBUG_PRINTF(L"E.what = %s", nb::MB2W(E.what(), FTerminal->GetSessionData()->GetCodePageAsNumber()).c_str());
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

        for (size_t IFile = 0; (IFile < FilesToCopy->GetCount()) &&
                !OperationProgress->Cancel; IFile++)
        {
            std::wstring FileName = FilesToCopy->GetString(IFile);
            bool CanProceed = false;

            std::wstring FileNameOnly =
                CopyParam->ChangeFileName(ExtractFileName(FileName, false), osLocal, true);
            // DEBUG_PRINTF(L"FileName = %s, CheckExistence = %d, FileNameOnly = %s", FileName.c_str(), CheckExistence, FileNameOnly.c_str());
            if (CheckExistence)
            {
                // previously there was assertion on FTerminal->FFiles->Loaded, but it
                // fails for scripting, if 'ls' is not issued before.
                // formally we should call CheckRemoteFile here but as checking is for
                // free here (almost) ...
                TRemoteFile *File = FTerminal->FFiles->FindFile(FileNameOnly);
                if (File != NULL)
                {
                    int Answer;
                    if (File->GetIsDirectory())
                    {
                        std::wstring Message = FMTLOAD(PROMPT_DIRECTORY_OVERWRITE, FileNameOnly.c_str());
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
                        // DEBUG_PRINTF(L"FileParams.SourceSize = %d", FileParams.SourceSize);
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

                    if (::DirectoryExists(::ExtractFilePath(FileName)))
                    {
                        FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir) +
                                                     FileNameOnly, true);
                    }
                }

                try
                {
                    // DEBUG_PRINTF(L"FileName = %s, TargetDirFull = %s", FileName.c_str(), TargetDirFull.c_str());
                    SCPSource(FileName, TargetDirFull,
                              CopyParam, Params, OperationProgress, 0);
                    OperationProgress->Finish(FileName, true, OnceDoneOperation);
                }
                catch (const EScpFileSkipped &E)
                {
                    TQueryParams params(qpAllowContinueOnError);
                    DEBUG_PRINTF(L"before FTerminal->QueryUserException");
                    SUSPEND_OPERATION (
                        if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()),
                                                          &E,
                                                          qaOK | qaAbort, &params, qtError) == qaAbort)
                        {
                            OperationProgress->Cancel = csCancel;
                        }
                        OperationProgress->Finish(FileName, false, OnceDoneOperation);
                        if (!FTerminal->HandleException(&E)) throw;
                    );
                }
                catch (const EScpSkipFile &E)
                {
                    DEBUG_PRINTF(L"before FTerminal->HandleException");
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
void __fastcall TSCPFileSystem::SCPSource(const std::wstring FileName,
                               const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                               TFileOperationProgressType *OperationProgress, int Level)
{
    std::wstring DestFileName = CopyParam->ChangeFileName(
                                    ExtractFileName(FileName, false), osLocal, Level == 0);
    // DEBUG_PRINTF(L"DestFileName = %s", DestFileName.c_str());
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
    TSafeHandleStream *Stream = new TSafeHandleStream(File);
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
            std::wstring AbsoluteFileName = FTerminal->AbsolutePath(DestFileName, false); // TargetDir +
            // DEBUG_PRINTF(L"AbsoluteFileName = %s", AbsoluteFileName.c_str());
            assert(File);

            // File is regular file (not directory)
            FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", FileName.c_str()));

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
                        BlockBuf.GetMemory()->Seek(0, nb::soFromBeginning);
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
                        std::wstring Buf;

                        if (CopyParam->GetPreserveTime())
                        {
                            Buf.resize(40, 0);
                            // Send last file access and modification time
                            swprintf_s(const_cast<wchar_t *>(Buf.c_str()), Buf.size(), L"T%lu 0 %lu 0", static_cast<unsigned long>(MTime),
                                       static_cast<unsigned long>(ATime));
                            FSecureShell->SendLine(Buf.c_str());
                            SCPResponse();
                        }

                        // Send file modes (rights), filesize and file name
                        Buf.clear();
                        Buf.resize(MAX_PATH * 2, 0);
                        // TODO: use boost::format
                        swprintf_s(const_cast<wchar_t *>(Buf.c_str()), Buf.size(), L"C%s %ld %s",
                                   Rights.GetOctal().c_str(),
                                   static_cast<int>(OperationProgress->AsciiTransfer ? AsciiBuf.GetSize() :
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
                                size_t BlockSize = OperationProgress->TransferBlockSize();
                                FSecureShell->Send(
                                    AsciiBuf.GetData() + OperationProgress->TransferedSize,
                                    BlockSize);
                                OperationProgress->AddTransfered(BlockSize);
                                if (OperationProgress->Cancel == csCancelTransfer)
                                {
                                    throw ExtException(FMTLOAD(USER_TERMINATED));
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
                        FSecureShell->Send(BlockBuf.GetData(), BlockBuf.GetSize());
                        OperationProgress->AddTransfered(BlockBuf.GetSize());
                    }

                    if ((OperationProgress->Cancel == csCancelTransfer) ||
                            (OperationProgress->Cancel == csCancel && !OperationProgress->TransferingFile))
                    {
                        throw ExtException(FMTLOAD(USER_TERMINATED));
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
                catch (const EScp &E)
                {
                    // SCP protocol fatal error
                    OperationProgress->TransferingFile = false;
                    throw;
                }
                catch (const EScpFileSkipped &E)
                {
                    // SCP protocol non-fatal error
                    OperationProgress->TransferingFile = false;
                    throw;
                }

                // We succeded transfering file, from now we can handle exceptions
                // normally -> no fatal error
                OperationProgress->TransferingFile = false;
            }
            catch (const std::exception &E)
            {
                // EScpFileSkipped is derived from EScpSkipFile,
                // but is does not indicate file skipped by user here
                if (dynamic_cast<const EScpFileSkipped *>(&E) != NULL)
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
                TTouchSessionAction(FTerminal->GetActionLog(), AbsoluteFileName,
                                    UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode()));
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
void __fastcall TSCPFileSystem::SCPDirectorySource(const std::wstring DirectoryName,
                                        const std::wstring TargetDir, const TCopyParamType *CopyParam, int Params,
                                        TFileOperationProgressType *OperationProgress, int Level)
{
    int Attrs = 0;

    FTerminal->LogEvent(FORMAT(L"Entering directory \"%s\".", DirectoryName.c_str()));

    OperationProgress->SetFile(DirectoryName);
    std::wstring DestFileName = CopyParam->ChangeFileName(
                                    ExtractFileName(DirectoryName, false), osLocal, Level == 0);

    // Get directory attributes
    FILE_OPERATION_LOOP (FMTLOAD(CANT_GET_ATTRS, DirectoryName.c_str()),
        Attrs = FileGetAttr(DirectoryName);
        if (Attrs == -1)
        {
            RaiseLastOSError();
        }
    )

    std::wstring TargetDirFull = UnixIncludeTrailingBackslash(TargetDir + DestFileName);

    std::wstring Buf;

    /* TODO 1: maybe send filetime */

    // Send directory modes (rights), filesize and file name
    Buf = FORMAT(L"D%s 0 %s",
                 CopyParam->RemoteFileRights(Attrs).GetOctal().c_str(), DestFileName.c_str());
    FSecureShell->SendLine(Buf);
    SCPResponse();

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
        WIN32_FIND_DATA SearchRec;
        memset(&SearchRec, 0, sizeof(SearchRec));
        HANDLE findHandle = 0;
        bool FindOK = false;
        FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
            std::wstring path = IncludeTrailingBackslash(DirectoryName) + L"*.*";
            findHandle = FindFirstFile(path.c_str(), &SearchRec);
            FindOK = (findHandle != 0);
        );

        {
            BOOST_SCOPE_EXIT ( (&findHandle) )
            {
                FindClose(findHandle);
            } BOOST_SCOPE_EXIT_END
            while (FindOK && !OperationProgress->Cancel)
            {
                std::wstring FileName = IncludeTrailingBackslash(DirectoryName) + SearchRec.cFileName;
                try
                {
                    if ((wcscmp(SearchRec.cFileName, THISDIRECTORY) != 0) && (wcscmp(SearchRec.cFileName, PARENTDIRECTORY) != 0))
                    {
                        SCPSource(FileName, TargetDirFull, CopyParam, Params, OperationProgress, Level + 1);
                    }
                }
                // Previously we catched EScpSkipFile, making error being displayed
                // even when file was excluded by mask. Now the EScpSkipFile is special
                // case without error message.
                catch (const EScpFileSkipped &E)
                {
                    TQueryParams params(qpAllowContinueOnError);
                    DEBUG_PRINTF(L"before FTerminal->HandleException");
                    SUSPEND_OPERATION (
                        if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, FileName.c_str()),
                                                          &E,
                                                          qaOK | qaAbort, &params, qtError) == qaAbort)
                        {
                            OperationProgress->Cancel = csCancel;
                        }
                        if (!FTerminal->HandleException(&E)) throw;
                    );
                }
                catch (const EScpSkipFile &E)
                {
                    // If ESkipFile occurs, just log it and continue with next file
                    DEBUG_PRINTF(L"before FTerminal->HandleException");
                    SUSPEND_OPERATION (
                        if (!FTerminal->HandleException(&E)) throw;
                    );
                }
                FindOK = false;
                FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
                    FindOK = (FindNextFile(findHandle, &SearchRec) != 0);
                );
            };
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
                FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
                    THROWOSIFFALSE(FileSetAttr(DirectoryName, Attrs & ~faArchive) == 0);
                )
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::CopyToLocal(nb::TStrings *FilesToCopy,
                                 const std::wstring TargetDir, const TCopyParamType *CopyParam,
                                 int Params, TFileOperationProgressType *OperationProgress,
                                 TOnceDoneOperation &OnceDoneOperation)
{
    bool CloseSCP = false;
    Params &= ~(cpAppend | cpResume);
    std::wstring Options = L"";
    if (CopyParam->GetPreserveRights() || CopyParam->GetPreserveTime()) { Options = L"-p"; }
    if (FTerminal->GetSessionData()->GetScp1Compatibility()) { Options += L" -1"; }

    FTerminal->LogEvent(FORMAT(L"Copying %d files/directories to local directory \"%s\"",
                               FilesToCopy->GetCount(), TargetDir.c_str()));
    FTerminal->LogEvent(CopyParam->GetLogStr());

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
                bool LastLineRead;

                // If we get LastLine, it means that remote side 'scp' is already
                // terminated, so we need not to terminate it. There is also
                // possibility that remote side waits for confirmation, so it will hang.
                // This should not happen (hope)
                std::wstring Line = Self->FSecureShell->ReceiveLine();
                LastLineRead = Self->IsLastLine(Line);
                if (!LastLineRead)
                {
                    Self->SCPSendError((OperationProgress->Cancel ? L"Terminated by user." : L"std::exception"), true);
                }
                // Just in case, remote side already sent some more data (it's probable)
                // but we don't want to raise exception (user asked to terminate, it's not error)
                int ECParams = coOnlyReturnCode;
                if (!LastLineRead) { ECParams |= coWaitForLastLine; }
                Self->ReadCommandOutput(ECParams);
            }
        } BOOST_SCOPE_EXIT_END
        for (size_t IFile = 0; (IFile < FilesToCopy->GetCount()) &&
                !OperationProgress->Cancel; IFile++)
        {
            std::wstring FileName = FilesToCopy->GetString(IFile);
            TRemoteFile *File = (TRemoteFile *)FilesToCopy->GetObject(IFile);
            assert(File);

            try
            {
                bool Success = true; // Have to be set to true (see ::SCPSink)
                SendCommand(FCommandSet->FullCommand(fsCopyToLocal,
                                                     Options.c_str(), DelimitStr(FileName).c_str()));
                SkipFirstLine();

                // Filename is used for error messaging and excluding files only
                // Send in full path to allow path-based excluding
                std::wstring FullFileName = UnixExcludeTrailingBackslash(File->GetFullFileName());
                // DEBUG_PRINTF(L"FileName = '%s', FullFileName = '%s'", FileName.c_str(), FullFileName.c_str());
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
                    }
                    catch (const EFatal &E)
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
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPError(const std::wstring Message, bool Fatal)
{
    SCPSendError(Message, Fatal);
    DEBUG_PRINTF(L"Message = %s", Message.c_str());
    THROW_FILE_SKIPPED(Message, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPSendError(const std::wstring Message, bool Fatal)
{
    char ErrorLevel = static_cast<char>(Fatal ? 2 : 1);
    FTerminal->LogEvent(FORMAT(L"Sending SCP error (%d) to remote side:",
                               static_cast<int>(ErrorLevel)));
    FSecureShell->Send(&ErrorLevel, 1);
    // We don't send exact error message, because some unspecified
    // characters can terminate remote scp
    FSecureShell->SendLine(L"scp: error");
}
//---------------------------------------------------------------------------
void __fastcall TSCPFileSystem::SCPSink(const std::wstring TargetDir,
                             const std::wstring FileName, const std::wstring SourceDir,
                             const TCopyParamType *CopyParam, bool &Success,
                             TFileOperationProgressType *OperationProgress, int Params,
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
    nb::TDateTime SourceTimestamp;

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
        std::wstring AbsoluteFileName = FileName;

        try
        {
            // Receive control record
            std::wstring Line = FSecureShell->ReceiveLine();

            if (Line.size() == 0) { FTerminal->FatalError(NULL, LoadStr(SCP_EMPTY_LINE)); }

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
                catch (const std::exception &E)
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
                char Ctrl = static_cast<char>(Line[0]);
                Line.erase(0, 1);
                // DEBUG_PRINTF(L"Line ='%s', Ctrl = '%c'", Line.c_str(), Ctrl);

                switch (Ctrl)
                {
                case 1:
                    // Error (already logged by ReceiveLine())
                    THROW_FILE_SKIPPED(FMTLOAD(REMOTE_ERROR, Line.c_str()), NULL);

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
                    FileData.RemoteRights.SetOctal(CutToChar(Line, ' ', true));
                    // do not trim leading spaces of the filename
                    __int64 TSize = StrToInt64(::TrimRight(CutToChar(Line, ' ', false)));
                    // DEBUG_PRINTF(L"TSize = %u", TSize);
                    MaskParams.Size = TSize;
                    // Security fix: ensure the file ends up where we asked for it.
                    // (accept only filename, not path)
                    std::wstring OnlyFileName = UnixExtractFileName(Line);
                    // DEBUG_PRINTF(L"Line = '%s', OnlyFileName = '%s'", Line.c_str(), OnlyFileName.c_str());
                    if (Line != OnlyFileName)
                    {
                        FTerminal->LogEvent(FORMAT(L"Warning: Remote host set a compound pathname '%s'", Line.c_str()));
                    }

                    OperationProgress->SetFile(OnlyFileName);
                    AbsoluteFileName = SourceDir + OnlyFileName;
                    // DEBUG_PRINTF(L"AbsoluteFileName = '%s', OnlyFileName = '%s'", AbsoluteFileName.c_str(), OnlyFileName.c_str());
                    OperationProgress->SetTransferSize(TSize);
                }
                catch (const std::exception &E)
                {
                    SUSPEND_OPERATION (
                        FTerminal->GetLog()->AddException(&E);
                    );
                    SCPError(LoadStr(SCP_ILLEGAL_FILE_DESCRIPTOR), false);
                }

                // last possibility to cancel transfer before it starts
                if (OperationProgress->Cancel)
                {
                    THROW_SKIP_FILE(LoadStr(USER_TERMINATED), NULL);
                }

                bool Dir = (Ctrl == 'D');
                std::wstring SourceFullName = SourceDir + OperationProgress->FileName;
                if (!CopyParam->AllowTransfer(SourceFullName, osRemote, Dir, MaskParams))
                {
                    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer",
                                               AbsoluteFileName.c_str()));
                    SkipConfirmed = true;
                    SCPError(L"", false);
                }

                // DEBUG_PRINTF(L"TargetDir = %s", TargetDir.c_str());
                // DEBUG_PRINTF(L"OperationProgress->FileName = %s", OperationProgress->FileName.c_str());
                std::wstring DestFileName =
                    IncludeTrailingBackslash(TargetDir) +
                    CopyParam->ChangeFileName(OperationProgress->FileName, osRemote,
                                              Level == 0);

                // DEBUG_PRINTF(L"DestFileName = %s", DestFileName.c_str());
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
                        FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFileName.c_str()),
                            if (!ForceDirectories(DestFileName))
                            {
                                RaiseLastOSError();
                            }
                        );
                        /* SCP: can we set the timestamp for directories ? */
                    }
                    std::wstring FullFileName = SourceDir + OperationProgress->FileName;
                    SCPSink(DestFileName, FullFileName, UnixIncludeTrailingBackslash(FullFileName),
                            CopyParam, Success, OperationProgress, Params, Level + 1);
                    continue;
                }
                else if (Ctrl == 'C')
                {
                    TDownloadSessionAction Action(FTerminal->GetActionLog());
                    Action.FileName(AbsoluteFileName);

                    try
                    {
                        HANDLE File = 0;
                        nb::TStream *FileStream = NULL;

                        /* TODO 1 : Turn off read-only attr */

                        {
                            BOOST_SCOPE_EXIT ( (&File) (&FileStream) )
                            {
                                if (File) { ::CloseHandle(File); }
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
                            catch (const std::exception &E)
                            {
                                // In this step we can still cancel transfer, so we do it
                                SCPError(nb::MB2W(E.what(), FTerminal->GetSessionData()->GetCodePageAsNumber()), false);
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
                                    BlockBuf.SetSize(OperationProgress->TransferBlockSize());
                                    BlockBuf.SetPosition(0);

                                    FSecureShell->Receive(BlockBuf.GetData(), BlockBuf.GetSize());
                                    OperationProgress->AddTransfered(BlockBuf.GetSize());

                                    if (OperationProgress->AsciiTransfer)
                                    {
                                        size_t PrevBlockSize = BlockBuf.GetSize();
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
                                        throw ExtException(FMTLOAD(USER_TERMINATED));
                                    }
                                }
                                while (!OperationProgress->IsLocallyDone() || !
                                        OperationProgress->IsTransferDone());
                            }
                            catch (const std::exception &E)
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
                            catch (const EScp &E)
                            {
                                FSecureShell->SendNull();
                                throw;
                            }
                            catch (const EScpFileSkipped &E)
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
                    }
                    catch (const std::exception &E)
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
        catch (const EScpFileSkipped &E)
        {
            if (!SkipConfirmed)
            {
                SUSPEND_OPERATION (
                    TQueryParams params(qpAllowContinueOnError);
                    // DEBUG_PRINTF(L"AbsoluteFileName = %s", AbsoluteFileName.c_str());
                    if (FTerminal->QueryUserException(FMTLOAD(COPY_ERROR, AbsoluteFileName.c_str()),
                                                      &E, qaOK | qaAbort, &params, qtError) == qaAbort)
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
        catch (const EScpSkipFile &E)
        {
            DEBUG_PRINTF(L"before FTerminal->HandleException");
            SCPSendError(E.GetMessage(), false);
            Success = false;
            if (!FTerminal->HandleException(&E)) { throw; }
        }
    }
}
