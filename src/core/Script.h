
#pragma once
#if 0
#ifndef ScriptH
#define ScriptH

#include <time.h>
#include "Option.h"
#include "Terminal.h"
#include "FileOperationProgress.h"

class TTerminal;
class TScript;
class TScriptCommands;
class TStoredSessionList;
class TTerminalList;

class TScriptProgress
{
public:
  TFileOperation Operation;
  TOperationSide Side;
  UnicodeString FileName;
  UnicodeString Directory;
  unsigned int OverallProgress;
  unsigned int FileProgress;
  unsigned int CPS;
  bool Cancel;
};

typedef void (__closure *TScriptPrintEvent)(TScript * Script, const UnicodeString Str, bool Error);
typedef void (__closure *TScriptSynchronizeStartStop)(TScript * Script,
  const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
  const TCopyParamType & CopyParam, int SynchronizeParams);
typedef void (__closure *TScriptProgressEvent)(TScript * Script, TScriptProgress & Progress);

class TScriptProcParams : public TOptions
{
public:
  TScriptProcParams(const UnicodeString & FullCommand, const UnicodeString & ParamsStr);

  __property UnicodeString ParamsStr = { read = FParamsStr };
  __property UnicodeString FullCommand = { read = FFullCommand };

private:
  UnicodeString FParamsStr;
  UnicodeString FFullCommand;
};

class TScript
{
public:
  enum TBatchMode { BatchOff, BatchOn, BatchAbort, BatchContinue };

  TScript(bool LimitedOutput);
  virtual ~TScript();

  void Command(UnicodeString Cmd);
  void Log(TLogLineType Type, const UnicodeString & Str, TTerminal * ATerminal = nullptr);
  void PrintLine(const UnicodeString Str, bool Error = false, TTerminal * ATerminal = nullptr);
  void StartInteractive();

  void Synchronize(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
    int SynchronizeParams, TSynchronizeChecklist ** Checklist);

  static void RequireParams(TScriptProcParams * Parameters, int MinParams);

  __property TScriptPrintEvent OnPrint = { read = FOnPrint, write = FOnPrint };
  __property TExtendedExceptionEvent OnShowExtendedException = { read = FOnShowExtendedException, write = FOnShowExtendedException };
  __property TSynchronizeDirectory OnTerminalSynchronizeDirectory = { read = FOnTerminalSynchronizeDirectory, write = FOnTerminalSynchronizeDirectory };
  __property TScriptSynchronizeStartStop OnSynchronizeStartStop = { read = FOnSynchronizeStartStop, write = FOnSynchronizeStartStop };
  __property TScriptProgressEvent OnProgress = { read = FOnProgress, write = FOnProgress };
  __property TCopyParamType CopyParam = { read = FCopyParam, write = SetCopyParam };
  __property int SynchronizeParams = { read = FSynchronizeParams, write = SetSynchronizeParams };
  __property TBatchMode Batch = { read = FBatch };
  __property TTerminal * Terminal = { read = FTerminal };
  __property bool Groups = { read = FGroups, write = FGroups };
  __property bool WantsProgress = { read = FWantsProgress, write = FWantsProgress };
  __property bool Interactive = { read = FInteractive, write = FInteractive };
  __property TTransferOutEvent OnTransferOut = { read = FOnTransferOut, write = FOnTransferOut };
  __property TTransferInEvent OnTransferIn = { read = FOnTransferIn, write = FOnTransferIn };

protected:
  TTerminal * FTerminal;
  TTerminal * FLoggingTerminal;
  TScriptCommands * FCommands;
  TScriptPrintEvent FOnPrint;
  TExtendedExceptionEvent FOnShowExtendedException;
  TSynchronizeDirectory FOnTerminalSynchronizeDirectory;
  TScriptSynchronizeStartStop FOnSynchronizeStartStop;
  TScriptProgressEvent FOnProgress;
  TCopyParamType FCopyParam;
  bool FIncludeFileMaskOptionUsed;
  TBatchMode FBatch;
  TBatchMode FInteractiveBatch;
  bool FConfirm;
  bool FInteractiveConfirm;
  bool FEcho;
  bool FFailOnNoMatch;
  int FSynchronizeParams;
  int FSynchronizeMode;
  bool FKeepingUpToDate;
  UnicodeString FSynchronizeIntro;
  bool FLimitedOutput;
  int FSessionReopenTimeout;
  int FInteractiveSessionReopenTimeout;
  bool FGroups;
  bool FWantsProgress;
  bool FInteractive;
  TTransferOutEvent FOnTransferOut;
  TTransferInEvent FOnTransferIn;
  TStrings * FPendingLogLines;
  bool FWarnNonDefaultCopyParam;
  bool FWarnNonDefaultSynchronizeParams;

  virtual void ResetTransfer();
  virtual void ConnectTerminal(TTerminal * ATerminal);
  bool EnsureCommandSessionFallback(
    TFSCapability Capability, TSessionAction & Action);
  void Print(const UnicodeString Str, bool Error = false);
  void CheckSession();
  void CheckParams(TScriptProcParams * Parameters);
  void CopyParamParams(TCopyParamType & CopyParam, TScriptProcParams * Parameters);
  void TransferParamParams(int & Params, TScriptProcParams * Parameters);
  enum TFileListType
  {
    fltDefault =     0x00,
    fltDirectories = 0x01,
    fltQueryServer = 0x02,
    fltMask =        0x04,
    fltLatest =      0x08,
    fltOnlyFile =    0x10,
  };
  TStrings * CreateFileList(TScriptProcParams * Parameters, int Start,
    int End, TFileListType ListType = fltDefault);
  TStrings * CreateLocalFileList(TScriptProcParams * Parameters,
    int Start, int End, TFileListType ListType);
  void FreeFiles(TStrings * FileList);
  void FreeFileList(TStrings * FileList);
  void LogPendingLines(TTerminal * ATerminal);

  void HelpProc(TScriptProcParams * Parameters);
  void CallProc(TScriptProcParams * Parameters);
  void PwdProc(TScriptProcParams * Parameters);
  void CdProc(TScriptProcParams * Parameters);
  void LsProc(TScriptProcParams * Parameters);
  void RmProc(TScriptProcParams * Parameters);
  void RmDirProc(TScriptProcParams * Parameters);
  void MvProc(TScriptProcParams * Parameters);
  void CpProc(TScriptProcParams * Parameters);
  void ChModProc(TScriptProcParams * Parameters);
  void LnProc(TScriptProcParams * Parameters);
  void MkDirProc(TScriptProcParams * Parameters);
  void GetProc(TScriptProcParams * Parameters);
  void PutProc(TScriptProcParams * Parameters);
  void OptionProc(TScriptProcParams * Parameters);
  void AsciiProc(TScriptProcParams * Parameters);
  void BinaryProc(TScriptProcParams * Parameters);
  void SynchronizeProc(TScriptProcParams * Parameters);
  void KeepUpToDateProc(TScriptProcParams * Parameters);
  void EchoProc(TScriptProcParams * Parameters);
  void StatProc(TScriptProcParams * Parameters);
  void ChecksumProc(TScriptProcParams * Parameters);

  void OptionImpl(UnicodeString OptionName, UnicodeString ValueName);
  void SynchronizeDirectories(TScriptProcParams * Parameters,
    UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory, int FirstParam);
  virtual bool HandleExtendedException(Exception * E,
    TTerminal * Terminal = nullptr);
  void TerminalCaptureLog(const UnicodeString & AddedLine, TCaptureOutputType OutputType);
  virtual UnicodeString GetLogCmd(const UnicodeString & FullCommand,
    const UnicodeString & Command, const UnicodeString & Params);
  void SynchronizePreview(
    UnicodeString LocalDirectory, UnicodeString RemoteDirectory,
    TSynchronizeChecklist * Checklist);
  UnicodeString SynchronizeFileRecord(
    const UnicodeString & RootDirectory, const TSynchronizeChecklist::TItem * Item,
    bool Local);
  UnicodeString ListingSysErrorMessage();
  void NoMatch(const UnicodeString & Mask, const UnicodeString & Error);
  void NoMatch(const UnicodeString & Message);

private:
  void Init();
  void SetCopyParam(const TCopyParamType & value);
  void SetSynchronizeParams(int value);
  TTransferMode ParseTransferModeName(UnicodeString Name);
  inline bool IsTerminalLogging(TTerminal * ATerminal);
  void CheckDefaultCopyParam();
  bool HasNonDefaultCopyParams();
  void CheckDefaultSynchronizeParams();
  void NotSupported();
  void CheckMultiFilesToOne(TStrings * FileList, const UnicodeString & Target, bool Unix);
  void LogOption(const UnicodeString & LogStr);
  void DoMvOrCp(TScriptProcParams * Parameters, TFSCapability Capability, bool Cp);
};

typedef void (__closure *TScriptInputEvent)(TScript * Script, const UnicodeString Prompt, UnicodeString & Str);
typedef void (__closure *TScriptQueryCancelEvent)(TScript * Script, bool & Cancel);
typedef void (__closure *TScriptPrintProgressEvent)(TScript * Script, bool First, const UnicodeString Str);

class TManagementScript : public TScript
{
public:
  TManagementScript(TStoredSessionList * StoredSessions, bool LimitedOutput);
  virtual ~TManagementScript();

  void Connect(const UnicodeString Session, TOptions * Options, bool CheckParams);
  void ReflectSettings();

  void MaskPasswordInCommandLine(UnicodeString & Command, bool Recurse);

  __property TScriptInputEvent OnInput = { read = FOnInput, write = FOnInput };
  __property TScriptQueryCancelEvent OnQueryCancel = { read = FOnQueryCancel, write = FOnQueryCancel };
  __property TPromptUserEvent OnTerminalPromptUser = { read = FOnTerminalPromptUser, write = FOnTerminalPromptUser };
  __property TQueryUserEvent OnTerminalQueryUser = { read = FOnTerminalQueryUser, write = FOnTerminalQueryUser };
  __property TScriptPrintProgressEvent OnPrintProgress = { read = FOnPrintProgress, write = FOnPrintProgress };
  __property bool Continue = { read = FContinue };

protected:
  TScriptInputEvent FOnInput;
  TScriptQueryCancelEvent FOnQueryCancel;
  TPromptUserEvent FOnTerminalPromptUser;
  TQueryUserEvent FOnTerminalQueryUser;
  TScriptPrintProgressEvent FOnPrintProgress;
  TStoredSessionList * FStoredSessions;
  TTerminalList * FTerminalList;
  UnicodeString FLastProgressFile;
  UnicodeString FLastProgressMessage;
  time_t FLastProgressTime;
  time_t FLastProgressEventTime;
  UnicodeString FLastProgressEventDoneFileName;
  bool FLastProgressOverallDone;
  bool FContinue;

  virtual void ResetTransfer();
  void Input(const UnicodeString Prompt, UnicodeString & Str, bool AllowEmpty);
  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase, const UnicodeString & Additional);
  void TerminalOperationProgress(TFileOperationProgressType & ProgressData);
  void TerminalOperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, Boolean Success,
    TOnceDoneOperation & OnceDoneOperation);

  void PrintActiveSession();
  TTerminal * FindSession(const UnicodeString Index);
  void FreeTerminal(TTerminal * Terminal);
  void PrintProgress(bool First, const UnicodeString Str);
  bool QueryCancel();
  void TerminalSynchronizeDirectory(
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    bool & Continue, bool Collect, const TSynchronizeOptions * Options);
  void DoChangeLocalDirectory(UnicodeString Directory);
  void DoClose(TTerminal * Terminal);
  virtual bool HandleExtendedException(Exception * E,
    TTerminal * Terminal = nullptr);
  void TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results, bool & Result, void * Arg);
  void TerminalInitializeLog(TObject * Sender);
  inline bool Synchronizing();
  inline void ShowPendingProgress();
  virtual UnicodeString GetLogCmd(const UnicodeString & FullCommand,
    const UnicodeString & Command, const UnicodeString & Params);
  UnicodeString MaskPasswordInCommand(const UnicodeString & FullCommand,
    const UnicodeString & Command);

  void ExitProc(TScriptProcParams * Parameters);
  void OpenProc(TScriptProcParams * Parameters);
  void CloseProc(TScriptProcParams * Parameters);
  void SessionProc(TScriptProcParams * Parameters);
  void LPwdProc(TScriptProcParams * Parameters);
  void LCdProc(TScriptProcParams * Parameters);
  void LLsProc(TScriptProcParams * Parameters);
};

#endif
#endif //if 0
