//---------------------------------------------------------------------------
#include "stdafx.h"

// #include <StrUtils.hpp>
#include "WinSCPFileSystem.h"
#include "WinSCPPlugin.h"
#include "FarDialog.h"
#include "FarTexts.h"
#include "FarConfiguration.h"
#include "farkeys.hpp"
#include "Common.h"
#include "Exceptions.h"
#include "SessionData.h"
#include <CoreMain.h>
// #include <SysUtils.hpp>
#include "ScpFileSystem.h"
#include <Bookmarks.h>
// #include <GUITools.h>
// #include <CompThread.hpp>
// FAR WORKAROUND
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TSessionPanelItem::TSessionPanelItem(TSessionData * ASessionData):
  TCustomFarPanelItem()
{
  assert(ASessionData);
  FSessionData = ASessionData;
}
//---------------------------------------------------------------------------
void TSessionPanelItem::SetPanelModes(TFarPanelModes * PanelModes)
{
  assert(FarPlugin);
  TStrings * ColumnTitles = new TStringList();
  try
  {
    ColumnTitles->Add(FarPlugin->GetMsg(SESSION_NAME_COL_TITLE));
    for (int Index = 0; Index < PANEL_MODES_COUNT; Index++)
    {
      PanelModes->SetPanelMode(Index, L"N", L"0", ColumnTitles, false, false, false);
    }
  }
  catch(...)
  {
  }
  delete ColumnTitles;
}
//---------------------------------------------------------------------------
void TSessionPanelItem::SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles)
{
  KeyBarTitles->ClearKeyBarTitle(fsNone, 6);
  KeyBarTitles->SetKeyBarTitle(fsNone, 5, FarPlugin->GetMsg(EXPORT_SESSION_KEYBAR));
  KeyBarTitles->ClearKeyBarTitle(fsShift, 1, 3);
  KeyBarTitles->SetKeyBarTitle(fsShift, 4, FarPlugin->GetMsg(NEW_SESSION_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsShift, 5, FarPlugin->GetMsg(COPY_SESSION_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsShift, 6, FarPlugin->GetMsg(RENAME_SESSION_KEYBAR));
  KeyBarTitles->ClearKeyBarTitle(fsShift, 7, 8);
  KeyBarTitles->ClearKeyBarTitle(fsAlt, 6);
  KeyBarTitles->ClearKeyBarTitle(fsCtrl, 4, 11);
}
//---------------------------------------------------------------------------
void TSessionPanelItem::GetData(
  unsigned long & /*Flags*/, wstring & FileName, __int64 & /*Size*/,
  unsigned long & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  unsigned long & /*NumberOfLinks*/, wstring & /*Description*/,
  wstring & /*Owner*/, void *& UserData, int & /*CustomColumnNumber*/)
{
  FileName = UnixExtractFileName(FSessionData->Name);
  UserData = FSessionData;
}
//---------------------------------------------------------------------------
TSessionFolderPanelItem::TSessionFolderPanelItem(wstring Folder):
  TCustomFarPanelItem(),
  FFolder(Folder)
{
}
//---------------------------------------------------------------------------
void TSessionFolderPanelItem::GetData(
  unsigned long & /*Flags*/, wstring & FileName, __int64 & /*Size*/,
  unsigned long & FileAttributes,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  unsigned long & /*NumberOfLinks*/, wstring & /*Description*/,
  wstring & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  FileName = FFolder;
  FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
}
//---------------------------------------------------------------------------
TRemoteFilePanelItem::TRemoteFilePanelItem(TRemoteFile * ARemoteFile):
  TCustomFarPanelItem()
{
  assert(ARemoteFile);
  FRemoteFile = ARemoteFile;
}
//---------------------------------------------------------------------------
void TRemoteFilePanelItem::GetData(
  unsigned long & /*Flags*/, wstring & FileName, __int64 & Size,
  unsigned long & FileAttributes,
  TDateTime & LastWriteTime, TDateTime & LastAccess,
  unsigned long & /*NumberOfLinks*/, wstring & /*Description*/,
  wstring & Owner, void *& UserData, int & CustomColumnNumber)
{
  FileName = FRemoteFile->GetFileName();
  Size = FRemoteFile->GetSize();
  FileAttributes =
    FLAGMASK(FRemoteFile->GetIsDirectory(), FILE_ATTRIBUTE_DIRECTORY) |
    FLAGMASK(FRemoteFile->GetIsHidden(), FILE_ATTRIBUTE_HIDDEN) |
    FLAGMASK(FRemoteFile->GetRights()->GetReadOnly(), FILE_ATTRIBUTE_READONLY) |
    FLAGMASK(FRemoteFile->GetIsSymLink(), FILE_ATTRIBUTE_REPARSE_POINT);
  LastWriteTime = FRemoteFile->GetModification();
  LastAccess = FRemoteFile->GetLastAccess();
  Owner = FRemoteFile->GetOwner().GetName();
  UserData = FRemoteFile;
  CustomColumnNumber = 4;
}
//---------------------------------------------------------------------------
wstring TRemoteFilePanelItem::CustomColumnData(int Column)
{
  switch (Column) {
    case 0: return FRemoteFile->GetGroup().GetName();
    case 1: return FRemoteFile->GetRightsStr();
    case 2: return FRemoteFile->GetRights()->GetOctal();
    case 3: return FRemoteFile->GetLinkTo();
    default: assert(false); return wstring();
  }
}
//---------------------------------------------------------------------------
void TRemoteFilePanelItem::TranslateColumnTypes(wstring & ColumnTypes,
  TStrings * ColumnTitles)
{
  wstring AColumnTypes = ColumnTypes;
  ColumnTypes = L"";
  wstring Column;
  wstring Title;
  while (!AColumnTypes.empty())
  {
    Column = CutToChar(AColumnTypes, ',', false);
    if (Column == L"G")
    {
      Column = L"C0";
      Title = FarPlugin->GetMsg(GROUP_COL_TITLE);
    }
    else if (Column == L"R")
    {
      Column = L"C1";
      Title = FarPlugin->GetMsg(RIGHTS_COL_TITLE);
    }
    else if (Column == L"RO")
    {
      Column = L"C2";
      Title = FarPlugin->GetMsg(RIGHTS_OCTAL_COL_TITLE);
    }
    else if (Column == L"L")
    {
      Column = L"C3";
      Title = FarPlugin->GetMsg(LINK_TO_COL_TITLE);
    }
    else
    {
      Title = L"";
    }
    ColumnTypes += (ColumnTypes.empty() ? L"" : L",") + Column;
    if (ColumnTitles)
    {
      ColumnTitles->Add(Title);
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteFilePanelItem::SetPanelModes(TFarPanelModes * PanelModes)
{
  assert(FarPlugin);
  TStrings * ColumnTitles = new TStringList();
  try
  {
    if (FarConfiguration->GetCustomPanelModeDetailed())
    {
      wstring ColumnTypes = FarConfiguration->GetColumnTypesDetailed();
      wstring StatusColumnTypes = FarConfiguration->GetStatusColumnTypesDetailed();

      TranslateColumnTypes(ColumnTypes, ColumnTitles);
      TranslateColumnTypes(StatusColumnTypes, NULL);

      PanelModes->SetPanelMode(5 /*detailed */,
        ColumnTypes, FarConfiguration->GetColumnWidthsDetailed(),
        ColumnTitles, FarConfiguration->GetFullScreenDetailed(), false, true, false,
        StatusColumnTypes, FarConfiguration->GetStatusColumnWidthsDetailed());
    }
  }
  catch(...)
  {
    delete ColumnTitles;
  }
}
//---------------------------------------------------------------------------
void TRemoteFilePanelItem::SetKeyBarTitles(TFarKeyBarTitles * KeyBarTitles)
{
  KeyBarTitles->ClearKeyBarTitle(fsShift, 1, 3); // archive commands
  KeyBarTitles->SetKeyBarTitle(fsShift, 5, FarPlugin->GetMsg(COPY_TO_FILE_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsShift, 6, FarPlugin->GetMsg(MOVE_TO_FILE_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsAltShift, 12,
    FarPlugin->GetMsg(OPEN_DIRECTORY_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsAltShift, 6,
    FarPlugin->GetMsg(RENAME_FILE_KEYBAR));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TFarInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TFarInteractiveCustomCommand(TCustomFarPlugin * Plugin,
    TCustomCommand * ChildCustomCommand);

protected:
  virtual void Prompt(int Index, const wstring & Prompt,
    wstring & Value);

private:
  TCustomFarPlugin * FPlugin;
};
//---------------------------------------------------------------------------
TFarInteractiveCustomCommand::TFarInteractiveCustomCommand(
  TCustomFarPlugin * Plugin, TCustomCommand * ChildCustomCommand) :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FPlugin = Plugin;
}
//---------------------------------------------------------------------------
void TFarInteractiveCustomCommand::Prompt(int /*Index*/,
  const wstring & Prompt, wstring & Value)
{
  wstring APrompt = Prompt;
  if (APrompt.empty())
  {
    APrompt = FPlugin->GetMsg(APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!FPlugin->InputBox(FPlugin->GetMsg(APPLY_COMMAND_PARAM_TITLE),
        APrompt, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Attempt to allow keepalives from background thread.
// Not finished nor used.
class TKeepaliveThread : public TCompThread
{
public:
  TKeepaliveThread(TWinSCPFileSystem * FileSystem, TDateTime Interval);
  virtual void Execute();
  virtual void Terminate();

private:
  TWinSCPFileSystem * FFileSystem;
  TDateTime FInterval;
  HANDLE FEvent;
};
//---------------------------------------------------------------------------
TKeepaliveThread::TKeepaliveThread(TWinSCPFileSystem * FileSystem,
  TDateTime Interval) :
  TCompThread(true)
{
  FEvent = CreateEvent(NULL, false, false, NULL);

  FFileSystem = FileSystem;
  FInterval = Interval;
  Resume();
}
//---------------------------------------------------------------------------
void TKeepaliveThread::Terminate()
{
  TCompThread::Terminate();
  SetEvent(FEvent);
}
//---------------------------------------------------------------------------
void TKeepaliveThread::Execute()
{
  while (!GetTerminated())
  {
    static long MillisecondsPerDay = 24 * 60 * 60 * 1000;
    if ((WaitForSingleObject(FEvent, double(FInterval) * MillisecondsPerDay) != WAIT_FAILED) &&
        !GetTerminated())
    {
      FFileSystem->KeepaliveThreadCallback();
    }
  }
  CloseHandle(FEvent);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TWinSCPFileSystem::TWinSCPFileSystem(TCustomFarPlugin * APlugin) :
  TCustomFarFileSystem(APlugin)
{
  FReloadDirectory = false;
  FProgressSaveScreenHandle = 0;
  FSynchronizationSaveScreenHandle = 0;
  FAuthenticationSaveScreenHandle = 0;
  FFileList = NULL;
  FPanelItems = NULL;
  FSavedFindFolder = L"";
  FTerminal = NULL;
  FQueue = NULL;
  FQueueStatus = NULL;
  FQueueStatusSection = new TCriticalSection();
  FQueueStatusInvalidated = false;
  FQueueItemInvalidated = false;
  FRefreshLocalDirectory = false;
  FRefreshRemoteDirectory = false;
  FQueueEventPending = false;
  FNoProgress = false;
  FNoProgressFinish = false;
  FKeepaliveThread = NULL;
  FSynchronisingBrowse = false;
  FSynchronizeController = NULL;
  FCapturedLog = NULL;
  FAuthenticationLog = NULL;
  FLastEditorID = -1;
  FLoadingSessionList = false;
  FPathHistory = new TStringList;
}
//---------------------------------------------------------------------------
TWinSCPFileSystem::~TWinSCPFileSystem()
{
  if (FTerminal)
  {
    SaveSession();
  }
  assert(FSynchronizeController == NULL);
  assert(!FAuthenticationSaveScreenHandle);
  assert(!FProgressSaveScreenHandle);
  assert(!FSynchronizationSaveScreenHandle);
  assert(!FFileList);
  assert(!FPanelItems);
  delete FPathHistory;
  FPathHistory = NULL;
  delete FQueue;
  FQueue = NULL;
  delete FQueueStatus;
  FQueueStatus = NULL;
  delete FQueueStatusSection;
  FQueueStatusSection = NULL;
  if (FTerminal != NULL)
  {
    GUIConfiguration->SetSynchronizeBrowsing(FSynchronisingBrowse);
  }
  SAFE_DESTROY(FTerminal);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::HandleException(exception * E, int OpMode)
{
  if ((GetTerminal() != NULL)) // FIXME && E->InheritsFrom(__classid(EFatal)))
  {
    if (!FClosed)
    {
      ClosePlugin();
    }
    GetTerminal()->ShowExtendedException(E);
  }
  else
  {
    TCustomFarFileSystem::HandleException(E, OpMode);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::KeepaliveThreadCallback()
{
  TGuard Guard(FCriticalSection);

  if (Connected())
  {
    FTerminal->Idle();
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SessionList()
{
  return (FTerminal == NULL);
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::Connected()
{
  // Check for active added to avoid "disconnected" message popup repeatedly
  // from "idle"
  return !SessionList() && FTerminal->GetActive();
}
//---------------------------------------------------------------------------
TWinSCPPlugin * TWinSCPFileSystem::WinSCPPlugin()
{
  return dynamic_cast<TWinSCPPlugin*>(FPlugin);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::Close()
{
  try
  {
    SAFE_DESTROY(FKeepaliveThread);

    if (Connected())
    {
      assert(FQueue != NULL);
      if (!FQueue->GetIsEmpty() &&
          (MoreMessageDialog(GetMsg(PENDING_QUEUE_ITEMS), NULL, qtWarning,
             qaOK | qaCancel) == qaOK))
      {
        QueueShow(true);
      }
    }
  }
  catch(...)
  {
  }
  TCustomFarFileSystem::Close();
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::GetOpenPluginInfoEx(long unsigned & Flags,
  wstring & /*HostFile*/, wstring & CurDir, wstring & Format,
  wstring & PanelTitle, TFarPanelModes * PanelModes, int & /*StartPanelMode*/,
  int & /*StartSortMode*/, bool & /*StartSortOrder*/, TFarKeyBarTitles * KeyBarTitles,
  wstring & ShortcutData)
{
  if (!SessionList())
  {
    Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING |
      OPIF_SHOWPRESERVECASE | OPIF_COMPAREFATTIME;

    // When slash is added to the end of path, windows style paths
    // (vandyke: c:/windows/system) are displayed correctly on command-line, but
    // leaved subdirectory is not focused, when entering parent directory.
    CurDir = FTerminal->GetCurrentDirectory();
    Format = FTerminal->GetSessionData()->GetSessionName();
    if (FarConfiguration->GetHostNameInTitle())
    {
      PanelTitle = ::FORMAT(L" %s:%s ", Format.c_str(), CurDir.c_str());
    }
    else
    {
      PanelTitle = ::FORMAT(L" %s ", CurDir.c_str());
    }
    ShortcutData = ::FORMAT(L"%s\1%s", FTerminal->GetSessionData()->GetSessionUrl().c_str(), CurDir.c_str());

    TRemoteFilePanelItem::SetPanelModes(PanelModes);
    TRemoteFilePanelItem::SetKeyBarTitles(KeyBarTitles);
  }
  else
  {
    CurDir = FSessionsFolder;
    Format = L"winscp";
    Flags = OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE;
    PanelTitle = ::FORMAT(L" %s ", GetMsg(STORED_SESSION_TITLE).c_str());

    TSessionPanelItem::SetPanelModes(PanelModes);
    TSessionPanelItem::SetKeyBarTitles(KeyBarTitles);
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::GetFindDataEx(TList * PanelItems, int OpMode)
{
  bool Result;
  if (Connected())
  {
    assert(!FNoProgress);
    // OPM_FIND is used also for calculation of directory size (F3, quick view).
    // However directory is usually read from SetDirectory, so FNoProgress
    // seems to have no effect here.
    // Do not know if OPM_SILENT is even used.
    FNoProgress = FLAGSET(OpMode, OPM_FIND) || FLAGSET(OpMode, OPM_SILENT);
    try
    {
      if (FReloadDirectory && FTerminal->GetActive())
      {
        FReloadDirectory = false;
        FTerminal->ReloadDirectory();
      }

      TRemoteFile * File;
      for (int Index = 0; Index < FTerminal->Files->Count; Index++)
      {
        File = FTerminal->Files->Files[Index];
        PanelItems->Add(new TRemoteFilePanelItem(File));
      }
    }
    catch(...)
    {
      FNoProgress = false;
    }
    Result = true;
  }
  else if (SessionList())
  {
    Result = true;
    assert(StoredSessions);
    StoredSessions->Load();

    wstring Folder = UnixIncludeTrailingBackslash(FSessionsFolder);
    TSessionData * Data;
    TStringList * ChildPaths = new TStringList();
    try
    {
      ChildPaths->CaseSensitive = false;

      for (int Index = 0; Index < StoredSessions->Count; Index++)
      {
        Data = StoredSessions->Sessions[Index];
        if (Data->Name.SubString(1, Folder.Length()) == Folder)
        {
          wstring Name = Data->Name.SubString(
            Folder.Length() + 1, Data->Name.Length() - Folder.Length());
          int Slash = Name.Pos('/');
          if (Slash > 0)
          {
            Name.SetLength(Slash - 1);
            if (ChildPaths->IndexOf(Name) < 0)
            {
              PanelItems->Add(new TSessionFolderPanelItem(Name));
              ChildPaths->Add(Name);
            }
          }
          else
          {
            PanelItems->Add(new TSessionPanelItem(Data));
          }
        }
      }
    }
    catch(...)
    {
      delete ChildPaths;
    }

    if (!FNewSessionsFolder.empty())
    {
      PanelItems->Add(new TSessionFolderPanelItem(FNewSessionsFolder));
    }

    if (PanelItems->Count == 0)
    {
      PanelItems->Add(new THintPanelItem(GetMsg(NEW_SESSION_HINT)));
    }

    TWinSCPFileSystem * OppositeFileSystem =
      dynamic_cast<TWinSCPFileSystem *>(GetOppositeFileSystem());
    if ((OppositeFileSystem != NULL) && !OppositeFileSystem->Connected() &&
        !OppositeFileSystem->FLoadingSessionList)
    {
      FLoadingSessionList = true;
      try
      {
        UpdatePanel(false, true);
        RedrawPanel(true);
      }
      catch(...)
      {
        FLoadingSessionList = false;
      }
    }
  }
  else
  {
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DuplicateRenameSession(TSessionData * Data,
  bool Duplicate)
{
  assert(Data);
  wstring Name = Data->Name;
  if (FPlugin->InputBox(GetMsg(Duplicate ? DUPLICATE_SESSION_TITLE : RENAME_SESSION_TITLE),
        GetMsg(Duplicate ? DUPLICATE_SESSION_PROMPT : RENAME_SESSION_PROMPT),
        Name, 0) &&
      !Name.empty() && (Name != Data->Name))
  {
    TNamedObject * EData = StoredSessions->FindByName(Name);
    if ((EData != NULL) && (EData != Data))
    {
      throw exception(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR), (Name)));
    }
    else
    {
      TSessionData * NData = StoredSessions->NewSession(Name, Data);
      FSessionsFolder = UnixExcludeTrailingBackslash(UnixExtractFilePath(Name));

      // change of letter case during duplication degrades the operation to rename
      if (!Duplicate || (Data == NData))
      {
        Data->Remove();
        if (NData != Data)
        {
          StoredSessions->Remove(Data);
        }
      }

      // modified only, explicit
      StoredSessions->Save(false, true);

      if (UpdatePanel())
      {
        RedrawPanel();

        FocusSession(NData);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::FocusSession(TSessionData * Data)
{
  TFarPanelItem * SessionItem = PanelInfo->FindUserData(Data);
  if (SessionItem != NULL)
  {
    PanelInfo->FocusedItem = SessionItem;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::EditConnectSession(TSessionData * Data, bool Edit)
{
  TSessionData * OrigData = Data;
  bool NewData = !Data;
  bool FillInConnect = !Edit && !Data->CanLogin;

  if (NewData || FillInConnect)
  {
    Data = new TSessionData("");
  }

  try
  {
    if (FillInConnect)
    {
      Data->Assign(OrigData);
      Data->Name = L"";
    }

    TSessionAction Action;
    if (Edit || FillInConnect)
    {
      Action = (FillInConnect ? saConnect : (OrigData == NULL ? saAdd : saEdit));
      if (SessionDialog(Data, Action))
      {
        TSessionData * SelectSession = NULL;
        if ((!NewData && !FillInConnect) || (Action != saConnect))
        {
          if (NewData)
          {
            wstring Name =
              UnixIncludeTrailingBackslash(FSessionsFolder) + Data->GetSessionName();
            if (FPlugin->InputBox(GetMsg(NEW_SESSION_NAME_TITLE),
                  GetMsg(NEW_SESSION_NAME_PROMPT), Name, 0) &&
                !Name.empty())
            {
              if (StoredSessions->FindByName(Name))
              {
                throw exception(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR), (Name)));
              }
              else
              {
                SelectSession = StoredSessions->NewSession(Name, Data);
                FSessionsFolder = UnixExcludeTrailingBackslash(UnixExtractFilePath(Name));
              }
            }
          }
          else if (FillInConnect)
          {
            wstring OrigName = OrigData->Name;
            OrigData->Assign(Data);
            OrigData->Name = OrigName;
          }

          // modified only, explicit
          StoredSessions->Save(false, true);
          if (UpdatePanel())
          {
            if (SelectSession != NULL)
            {
              FocusSession(SelectSession);
            }
            // rarely we need to redraw even when new session is created
            // (e.g. when there there were only the focused hint line before)
            RedrawPanel();
          }
        }
      }
    }
    else
    {
      Action = saConnect;
    }

    if ((Action == saConnect) && Connect(Data))
    {
      if (UpdatePanel())
      {
        RedrawPanel();
        if (PanelInfo->ItemCount)
        {
          PanelInfo->FocusedIndex = 0;
        }
      }
    }
  }
  catch(...)
  {
    if (NewData || FillInConnect)
    {
      delete Data;
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ProcessEventEx(int Event, void * Param)
{
  bool Result = false;
  if (Connected())
  {
    if (Event == FE_COMMAND)
    {
      wstring Command = (char *)Param;
      if (!Command.Trim().empty() &&
          (Command.SubString(1, 3).LowerCase() != L"cd "))
      {
        Result = ExecuteCommand(Command);
      }
    }
    else if (Event == FE_IDLE)
    {
      // FAR WORKAROUND
      // Control(FCTL_CLOSEPLUGIN) does not seem to close plugin when called from
      // ProcessEvent(FE_IDLE). So if TTerminal::Idle() causes session to close
      // we must count on having ProcessEvent(FE_IDLE) called again.
      FTerminal->Idle();
      if (FQueue != NULL)
      {
        FQueue->Idle();
      }
      ProcessQueue(true);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalCaptureLog(
  const wstring & AddedLine, bool /*StdError*/)
{
  if (FOutputLog)
  {
    FPlugin->WriteConsole(AddedLine + "\n");
  }
  if (FCapturedLog != NULL)
  {
    FCapturedLog->Add(AddedLine);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::RequireLocalPanel(TFarPanelInfo * Panel, wstring Message)
{
  if (Panel->IsPlugin || (Panel->Type != ptFile))
  {
    throw exception(Message);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::RequireCapability(int Capability)
{
  if (!FTerminal->IsCapable[static_cast<TFSCapability>(Capability)])
  {
    throw exception(FORMAT(GetMsg(OPERATION_NOT_SUPPORTED),
      (FTerminal->GetFileSystemInfo().ProtocolName)));
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::EnsureCommandSessionFallback(TFSCapability Capability)
{
  bool Result = FTerminal->IsCapable[Capability] ||
    FTerminal->CommandSessionOpened;

  if (!Result)
  {
    if (!GUIConfiguration->ConfirmCommandSession)
    {
      Result = true;
    }
    else
    {
      TMessageParams Params;
      Params.Params = qpNeverAskAgainCheck;
      int Answer = MoreMessageDialog(FORMAT(GetMsg(PERFORM_ON_COMMAND_SESSION),
        (FTerminal->GetFileSystemInfo().ProtocolName,
         FTerminal->GetFileSystemInfo().ProtocolName)), NULL,
        qtConfirmation, qaOK | qaCancel, &Params);
      if (Answer == qaNeverAskAgain)
      {
        GUIConfiguration->ConfirmCommandSession = false;
        Result = true;
      }
      else
      {
        Result = (Answer == qaOK);
      }
    }

    if (Result)
    {
      ConnectTerminal(FTerminal->CommandSession);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ExecuteCommand(const wstring Command)
{
  if (FTerminal->AllowedAnyCommand(Command) &&
      EnsureCommandSessionFallback(fcAnyCommand))
  {
    FTerminal->BeginTransaction();
    try
    {
      FarControl(FCTL_SETCMDLINE, NULL);
      FPlugin->ShowConsoleTitle(Command);
      try
      {
        FPlugin->ShowTerminalScreen();

        FOutputLog = true;
        FTerminal->AnyCommand(Command, TerminalCaptureLog);
      }
      catch(...)
      {
        FPlugin->ScrollTerminalScreen(1);
        FPlugin->SaveTerminalScreen();
        FPlugin->ClearConsoleTitle();
      }
    }
    catch(...)
    {
      if (FTerminal->GetActive())
      {
        FTerminal->EndTransaction();
        UpdatePanel();
      }
      else
      {
        RedrawPanel();
        RedrawPanel(true);
      }
    }
  }
  return true;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ProcessKeyEx(int Key, unsigned int ControlState)
{
  bool Handled = false;

  TFarPanelItem * Focused = PanelInfo->FocusedItem;

  if ((Key == 'W') && (ControlState & PKF_SHIFT) &&
        (ControlState & PKF_ALT))
  {
    WinSCPPlugin()->CommandsMenu(true);
    Handled = true;
  }
  else if (SessionList())
  {
    TSessionData * Data = NULL;

    if ((Focused != NULL) && Focused->IsFile && Focused->UserData)
    {
      Data = (TSessionData *)Focused->UserData;
    }

    if ((Key == 'F') && FLAGSET(ControlState, PKF_CONTROL))
    {
      InsertSessionNameOnCommandLine();
      Handled = true;
    }

    if ((Key == VK_RETURN) && FLAGSET(ControlState, PKF_CONTROL))
    {
      InsertSessionNameOnCommandLine();
      Handled = true;
    }

    if (Key == VK_RETURN && (ControlState == 0) && Data)
    {
      EditConnectSession(Data, false);
      Handled = true;
    }

    if (Key == VK_F4 && (ControlState == 0))
    {
      if ((Data != NULL) || (StoredSessions->Count == 0))
      {
        EditConnectSession(Data, true);
      }
      Handled = true;
    }

    if (Key == VK_F4 && (ControlState & PKF_SHIFT))
    {
      EditConnectSession(NULL, true);
      Handled = true;
    }

    if (((Key == VK_F5) || (Key == VK_F6)) &&
        (ControlState & PKF_SHIFT))
    {
      if (Data != NULL)
      {
        DuplicateRenameSession(Data, Key == VK_F5);
      }
      Handled = true;
    }
  }
  else if (Connected())
  {
    if ((Key == 'F') && (ControlState & PKF_CONTROL))
    {
      InsertFileNameOnCommandLine(true);
      Handled = true;
    }

    if ((Key == VK_RETURN) && FLAGSET(ControlState, PKF_CONTROL))
    {
      InsertFileNameOnCommandLine(false);
      Handled = true;
    }

    if ((Key == 'R') && (ControlState & PKF_CONTROL))
    {
      FReloadDirectory = true;
    }

    if ((Key == 'A') && (ControlState & PKF_CONTROL))
    {
      FileProperties();
      Handled = true;
    }

    if ((Key == 'G') && (ControlState & PKF_CONTROL))
    {
      ApplyCommand();
      Handled = true;
    }

    if ((Key == 'Q') && (ControlState & PKF_SHIFT) &&
          (ControlState & PKF_ALT))
    {
      QueueShow(false);
      Handled = true;
    }

    if ((Key == 'B') && (ControlState & PKF_CONTROL) &&
          (ControlState & PKF_ALT))
    {
      ToggleSynchronizeBrowsing();
      Handled = true;
    }

    if ((Key == VK_INSERT) &&
        (FLAGSET(ControlState, PKF_ALT | PKF_SHIFT) || FLAGSET(ControlState, PKF_CONTROL | PKF_ALT)))
    {
      CopyFullFileNamesToClipboard();
      Handled = true;
    }

    if ((Key == VK_F6) && ((ControlState & (PKF_ALT | PKF_SHIFT)) == PKF_ALT))
    {
      CreateLink();
      Handled = true;
    }

    if (Focused && ((Key == VK_F5) || (Key == VK_F6)) &&
        ((ControlState & (PKF_ALT | PKF_SHIFT)) == PKF_SHIFT))
    {
      TransferFiles((Key == VK_F6));
      Handled = true;
    }

    if (Focused && (Key == VK_F6) &&
        ((ControlState & (PKF_ALT | PKF_SHIFT)) == (PKF_SHIFT | PKF_ALT)))
    {
      RenameFile();
      Handled = true;
    }

    if ((Key == VK_F12) && (ControlState & PKF_SHIFT) &&
        (ControlState & PKF_ALT))
    {
      OpenDirectory(false);
      Handled = true;
    }

    if ((Key == VK_F4) && (ControlState == 0) &&
         FarConfiguration->EditorMultiple)
    {
      MultipleEdit();
      Handled = true;
    }
  }
  return Handled;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::CreateLink()
{
  RequireCapability(fcResolveSymlink);
  RequireCapability(fcSymbolicLink);

  bool Edit = false;
  TRemoteFile * File = NULL;
  wstring FileName;
  wstring PointTo;
  bool SymbolicLink = true;

  if (PanelInfo->FocusedItem && PanelInfo->FocusedItem->UserData)
  {
    File = (TRemoteFile *)PanelInfo->FocusedItem->UserData;

    Edit = File->GetIsSymLink() && Terminal->GetSessionData()->ResolveSymlinks;
    if (Edit)
    {
      FileName = File->GetFileName();
      PointTo = File->GetLinkTo();
    }
    else
    {
      PointTo = File->GetFileName();
    }
  }

  if (LinkDialog(FileName, PointTo, SymbolicLink, Edit,
        Terminal->IsCapable[fcHardLink]))
  {
    if (Edit)
    {
      assert(File->GetFileName() == FileName);
      int Params = dfNoRecursive;
      Terminal->ExceptionOnFail = true;
      try
      {
        Terminal->DeleteFile("", File, &Params);
      }
      catch(...)
      {
        Terminal->ExceptionOnFail = false;
      }
    }
    Terminal->CreateLink(FileName, PointTo, SymbolicLink);
    if (UpdatePanel())
    {
      RedrawPanel();
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TemporarilyDownloadFiles(
  TStrings * FileList, TCopyParamType CopyParam, wstring & TempDir)
{
  CopyParam.FileNameCase = ncNoChange;
  CopyParam.PreserveReadOnly = false;
  CopyParam.ResumeSupport = rsOff;

  TempDir = FPlugin->TemporaryDir();
  if (TempDir.empty() || !ForceDirectories(TempDir))
  {
    throw exception(FMTLOAD(CREATE_TEMP_DIR_ERROR, (TempDir)));
  }

  FTerminal->ExceptionOnFail = true;
  try
  {
    try
    {
      FTerminal->CopyToLocal(FileList, TempDir, &CopyParam, cpTemporary);
    }
    catch(...)
    {
      try
      {
        RecursiveDeleteFile(ExcludeTrailingBackslash(TempDir), false);
      }
      catch(...)
      {
      }
      throw;
    }
  }
  catch(...)
  {
    FTerminal->ExceptionOnFail = false;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ApplyCommand()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  if (FileList != NULL)
  {
    try
    {
      int Params = FarConfiguration->ApplyCommandParams;
      wstring Command = FarConfiguration->ApplyCommandCommand;
      if (ApplyCommandDialog(Command, Params))
      {
        FarConfiguration->ApplyCommandParams = Params;
        FarConfiguration->ApplyCommandCommand = Command;
        if (FLAGCLEAR(Params, ccLocal))
        {
          if (EnsureCommandSessionFallback(fcShellAnyCommand))
          {
            TCustomCommandData Data(Terminal);
            TRemoteCustomCommand RemoteCustomCommand(Data, Terminal->GetCurrentDirectory());
            TFarInteractiveCustomCommand InteractiveCustomCommand(
              FPlugin, &RemoteCustomCommand);

            Command = InteractiveCustomCommand.Complete(Command, false);

            try
            {
              TCaptureOutputEvent OutputEvent = NULL;
              FOutputLog = false;
              if (FLAGSET(Params, ccShowResults))
              {
                assert(!FNoProgress);
                FNoProgress = true;
                FOutputLog = true;
                OutputEvent = TerminalCaptureLog;
              }

              if (FLAGSET(Params, ccCopyResults))
              {
                assert(FCapturedLog == NULL);
                FCapturedLog = new TStringList();
                OutputEvent = TerminalCaptureLog;
              }

              try
              {
                if (FLAGSET(Params, ccShowResults))
                {
                  FPlugin->ShowTerminalScreen();
                }

                FTerminal->CustomCommandOnFiles(Command, Params, FileList, OutputEvent);
              }
              catch(...)
              {
                if (FLAGSET(Params, ccShowResults))
                {
                  FNoProgress = false;
                  FPlugin->ScrollTerminalScreen(1);
                  FPlugin->SaveTerminalScreen();
                }

                if (FLAGSET(Params, ccCopyResults))
                {
                  FPlugin->FarCopyToClipboard(FCapturedLog);
                  SAFE_DESTROY(FCapturedLog);
                }
              }
            }
            catch(...)
            {
              PanelInfo->ApplySelection();
              if (UpdatePanel())
              {
                RedrawPanel();
              }
            }
          }
        }
        else
        {
          TCustomCommandData Data(Terminal);
          TLocalCustomCommand LocalCustomCommand(Data, Terminal->GetCurrentDirectory());
          TFarInteractiveCustomCommand InteractiveCustomCommand(FPlugin,
            &LocalCustomCommand);

          Command = InteractiveCustomCommand.Complete(Command, false);

          TStrings * LocalFileList = NULL;
          TStrings * RemoteFileList = NULL;
          try
          {
            bool FileListCommand = LocalCustomCommand.IsFileListCommand(Command);
            bool LocalFileCommand = LocalCustomCommand.HasLocalFileName(Command);

            if (LocalFileCommand)
            {
              TFarPanelInfo * AnotherPanel = AnotherPanelInfo;
              RequireLocalPanel(AnotherPanel, GetMsg(APPLY_COMMAND_LOCAL_PATH_REQUIRED));

              LocalFileList = CreateSelectedFileList(osLocal, AnotherPanel);

              if (FileListCommand)
              {
                if ((LocalFileList == NULL) || (LocalFileList->Count != 1))
                {
                  throw exception(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH1));
                }
              }
              else
              {
                if ((LocalFileList == NULL) ||
                    ((LocalFileList->Count != 1) &&
                     (FileList->Count != 1) &&
                     (LocalFileList->Count != FileList->Count)))
                {
                  throw exception(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH));
                }
              }
            }

            wstring TempDir;

            TemporarilyDownloadFiles(FileList, GUIConfiguration->DefaultCopyParam, TempDir);

            try
            {
              RemoteFileList = new TStringList();

              TMakeLocalFileListParams MakeFileListParam;
              MakeFileListParam.FileList = RemoteFileList;
              MakeFileListParam.IncludeDirs = FLAGSET(Params, ccApplyToDirectories);
              MakeFileListParam.Recursive =
                FLAGSET(Params, ccRecursive) && !FileListCommand;

              ProcessLocalDirectory(TempDir, &FTerminal->MakeLocalFileList, &MakeFileListParam);

              TFileOperationProgressType Progress(&OperationProgress, &OperationFinished);

              Progress.Start(foCustomCommand, osRemote, FileListCommand ? 1 : FileList->Count);

              try
              {
                if (FileListCommand)
                {
                  wstring LocalFile;
                  wstring FileList = MakeFileList(RemoteFileList);

                  if (LocalFileCommand)
                  {
                    assert(LocalFileList->Count == 1);
                    LocalFile = LocalFileList->Strings[0];
                  }

                  TCustomCommandData Data(FTerminal);
                  TLocalCustomCommand CustomCommand(Data,
                    Terminal->GetCurrentDirectory(), "", LocalFile, FileList);
                  ExecuteShellAndWait(FPlugin->Handle, CustomCommand.Complete(Command, true),
                    TProcessMessagesEvent(NULL));
                }
                else if (LocalFileCommand)
                {
                  if (LocalFileList->Count == 1)
                  {
                    wstring LocalFile = LocalFileList->Strings[0];

                    for (int Index = 0; Index < RemoteFileList->Count; Index++)
                    {
                      wstring FileName = RemoteFileList->Strings[Index];
                      TCustomCommandData Data(FTerminal);
                      TLocalCustomCommand CustomCommand(Data,
                        Terminal->GetCurrentDirectory(), FileName, LocalFile, "");
                      ExecuteShellAndWait(FPlugin->Handle,
                        CustomCommand.Complete(Command, true), TProcessMessagesEvent(NULL));
                    }
                  }
                  else if (RemoteFileList->Count == 1)
                  {
                    wstring FileName = RemoteFileList->Strings[0];

                    for (int Index = 0; Index < LocalFileList->Count; Index++)
                    {
                      TCustomCommandData Data(FTerminal);
                      TLocalCustomCommand CustomCommand(
                        Data, Terminal->GetCurrentDirectory(),
                        FileName, LocalFileList->Strings[Index], "");
                      ExecuteShellAndWait(FPlugin->Handle,
                        CustomCommand.Complete(Command, true), TProcessMessagesEvent(NULL));
                    }
                  }
                  else
                  {
                    if (LocalFileList->Count != RemoteFileList->Count)
                    {
                      throw exception(GetMsg(CUSTOM_COMMAND_PAIRS_DOWNLOAD_FAILED));
                    }

                    for (int Index = 0; Index < LocalFileList->Count; Index++)
                    {
                      wstring FileName = RemoteFileList->Strings[Index];
                      TCustomCommandData Data(FTerminal);
                      TLocalCustomCommand CustomCommand(
                        Data, Terminal->GetCurrentDirectory(),
                        FileName, LocalFileList->Strings[Index], "");
                      ExecuteShellAndWait(FPlugin->Handle,
                        CustomCommand.Complete(Command, true), TProcessMessagesEvent(NULL));
                    }
                  }
                }
                else
                {
                  for (int Index = 0; Index < RemoteFileList->Count; Index++)
                  {
                    TCustomCommandData Data(FTerminal);
                    TLocalCustomCommand CustomCommand(Data,
                      Terminal->GetCurrentDirectory(), RemoteFileList->Strings[Index], "", "");
                    ExecuteShellAndWait(FPlugin->Handle,
                      CustomCommand.Complete(Command, true), TProcessMessagesEvent(NULL));
                  }
                }
              }
              catch(...)
              {
                Progress.Stop();
              }
            }
            catch(...)
            {
              RecursiveDeleteFile(ExcludeTrailingBackslash(TempDir), false);
            }
          }
          catch(...)
          {
            delete RemoteFileList;
            delete LocalFileList;
          }
        }
      }
    }
    catch(...)
    {
      delete FileList;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::Synchronize(const wstring LocalDirectory,
  const wstring RemoteDirectory, TTerminal::TSynchronizeMode Mode,
  const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options)
{
  TSynchronizeChecklist * AChecklist = NULL;
  try
  {
    FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
    FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = true;
    try
    {
      AChecklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
        Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
        TerminalSynchronizeDirectory, Options);
    }
    catch(...)
    {
      FPlugin->ClearConsoleTitle();
      FPlugin->RestoreScreen(FSynchronizationSaveScreenHandle);
    }

    FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
    FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = false;
    try
    {
      FTerminal->SynchronizeApply(AChecklist, LocalDirectory, RemoteDirectory,
        &CopyParam, Params | TTerminal::spNoConfirmation,
        TerminalSynchronizeDirectory);
    }
    catch(...)
    {
      FPlugin->ClearConsoleTitle();
      FPlugin->RestoreScreen(FSynchronizationSaveScreenHandle);
    }
  }
  catch(...)
  {
    if (Checklist == NULL)
    {
      delete AChecklist;
    }
    else
    {
      *Checklist = AChecklist;
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeAllowSelectedOnly()
{
  return
    (PanelInfo->SelectedCount > 0) ||
    (AnotherPanelInfo->SelectedCount > 0);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::GetSynchronizeOptions(
  int Params, TSynchronizeOptions & Options)
{
  if (FLAGSET(Params, spSelectedOnly) && SynchronizeAllowSelectedOnly())
  {
    Options.Filter = new TStringList();
    Options.Filter->CaseSensitive = false;
    Options.Filter->Duplicates = dupAccept;

    if (PanelInfo->SelectedCount > 0)
    {
      CreateFileList(PanelInfo->Items, osRemote, true, "", true, Options.Filter);
    }
    if (AnotherPanelInfo->SelectedCount > 0)
    {
      CreateFileList(AnotherPanelInfo->Items, osLocal, true, "", true, Options.Filter);
    }
    Options.Filter->Sort();
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::FullSynchronize(bool Source)
{
  TFarPanelInfo * AnotherPanel = AnotherPanelInfo;
  RequireLocalPanel(AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  wstring LocalDirectory = AnotherPanel->GetCurrentDirectory();
  wstring RemoteDirectory = FTerminal->GetCurrentDirectory();

  bool SaveMode = !(GUIConfiguration->SynchronizeModeAuto < 0);
  TTerminal::TSynchronizeMode Mode =
    (SaveMode ? (TTerminal::TSynchronizeMode)GUIConfiguration->SynchronizeModeAuto :
      (Source ? TTerminal::smLocal : TTerminal::smRemote));
  int Params = GUIConfiguration->SynchronizeParams;
  bool SaveSettings = false;

  TCopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
  TUsableCopyParamAttrs CopyParamAttrs = Terminal->UsableCopyParamAttrs(0);
  int Options =
    FLAGMASK(!FTerminal->IsCapable[fcTimestampChanging], fsoDisableTimestamp) |
    FLAGMASK(SynchronizeAllowSelectedOnly(), fsoAllowSelectedOnly);
  if (FullSynchronizeDialog(Mode, Params, LocalDirectory, RemoteDirectory,
        &CopyParam, SaveSettings, SaveMode, Options, CopyParamAttrs))
  {
    TSynchronizeOptions SynchronizeOptions;
    GetSynchronizeOptions(Params, SynchronizeOptions);

    if (SaveSettings)
    {
      GUIConfiguration->SynchronizeParams = Params;
      if (SaveMode)
      {
        GUIConfiguration->SynchronizeModeAuto = Mode;
      }
    }

    TSynchronizeChecklist * Checklist = NULL;
    try
    {
      FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
      FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
      FSynchronizationStart = Now();
      FSynchronizationCompare = true;
      try
      {
        Checklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
          Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
          TerminalSynchronizeDirectory, &SynchronizeOptions);
      }
      catch(...)
      {
        FPlugin->ClearConsoleTitle();
        FPlugin->RestoreScreen(FSynchronizationSaveScreenHandle);
      }

      if (Checklist->Count == 0)
      {
        MoreMessageDialog(GetMsg(COMPARE_NO_DIFFERENCES), NULL,
           qtInformation, qaOK);
      }
      else if (FLAGCLEAR(Params, TTerminal::spPreviewChanges) ||
               SynchronizeChecklistDialog(Checklist, Mode, Params,
                 LocalDirectory, RemoteDirectory))
      {
        if (FLAGSET(Params, TTerminal::spPreviewChanges))
        {
          FSynchronizationStart = Now();
        }
        FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
        FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
        FSynchronizationStart = Now();
        FSynchronizationCompare = false;
        try
        {
          FTerminal->SynchronizeApply(Checklist, LocalDirectory, RemoteDirectory,
            &CopyParam, Params | TTerminal::spNoConfirmation,
            TerminalSynchronizeDirectory);
        }
        catch(...)
        {
          FPlugin->ClearConsoleTitle();
          FPlugin->RestoreScreen(FSynchronizationSaveScreenHandle);
        }
      }
    }
    catch(...)
    {
      delete Checklist;
      if (UpdatePanel())
      {
        RedrawPanel();
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalSynchronizeDirectory(
  const wstring LocalDirectory, const wstring RemoteDirectory,
  bool & Continue, bool Collect)
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    static const int ProgressWidth = 48;
    static wstring ProgressTitle;
    static wstring ProgressTitleCompare;
    static wstring LocalLabel;
    static wstring RemoteLabel;
    static wstring StartTimeLabel;
    static wstring TimeElapsedLabel;

    if (ProgressTitle.empty())
    {
      ProgressTitle = GetMsg(SYNCHRONIZE_PROGRESS_TITLE);
      ProgressTitleCompare = GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE);
      LocalLabel = GetMsg(SYNCHRONIZE_PROGRESS_LOCAL);
      RemoteLabel = GetMsg(SYNCHRONIZE_PROGRESS_REMOTE);
      StartTimeLabel = GetMsg(SYNCHRONIZE_PROGRESS_START_TIME);
      TimeElapsedLabel = GetMsg(SYNCHRONIZE_PROGRESS_ELAPSED);
    }

    wstring Message;

    Message = LocalLabel + MinimizeName(LocalDirectory,
      ProgressWidth - LocalLabel.Length(), false);
    Message += wstring::StringOfChar(' ', ProgressWidth - Message.Length()) + "\n";
    Message += RemoteLabel + MinimizeName(RemoteDirectory,
      ProgressWidth - RemoteLabel.Length(), true) + "\n";
    Message += StartTimeLabel + FSynchronizationStart.TimeString() + "\n";
    Message += TimeElapsedLabel +
      FormatDateTimeSpan(Configuration->TimeFormat, Now() - FSynchronizationStart) + "\n";

    FPlugin->Message(0, (Collect ? ProgressTitleCompare : ProgressTitle), Message);

    if (FPlugin->CheckForEsc() &&
        (MoreMessageDialog(GetMsg(CANCEL_OPERATION), NULL,
          qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      Continue = false;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::Synchronize()
{
  TFarPanelInfo * AnotherPanel = AnotherPanelInfo;
  RequireLocalPanel(AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  TSynchronizeParamType Params;
  Params.LocalDirectory = AnotherPanel->GetCurrentDirectory();
  Params.RemoteDirectory = FTerminal->GetCurrentDirectory();
  int UnusedParams = (GUIConfiguration->SynchronizeParams &
    (TTerminal::spPreviewChanges | TTerminal::spTimestamp |
     TTerminal::spNotByTime | TTerminal::spBySize));
  Params.Params = GUIConfiguration->SynchronizeParams & ~UnusedParams;
  Params.Options = GUIConfiguration->SynchronizeOptions;
  bool SaveSettings = false;
  TSynchronizeController Controller(&DoSynchronize, &DoSynchronizeInvalid,
    &DoSynchronizeTooManyDirectories);
  assert(FSynchronizeController == NULL);
  FSynchronizeController = &Controller;

  try
  {
    TCopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
    int CopyParamAttrs = Terminal->UsableCopyParamAttrs(0).Upload;
    int Options =
      FLAGMASK(SynchronizeAllowSelectedOnly(), soAllowSelectedOnly);
    if (SynchronizeDialog(Params, &CopyParam, Controller.StartStop,
          SaveSettings, Options, CopyParamAttrs, GetSynchronizeOptions) &&
        SaveSettings)
    {
      GUIConfiguration->SynchronizeParams = Params.Params | UnusedParams;
      GUIConfiguration->SynchronizeOptions = Params.Options;
    }
  }
  catch(...)
  {
    FSynchronizeController = NULL;
    // plugin might have been closed during some synchronisation already
    if (!FClosed)
    {
      if (UpdatePanel())
      {
        RedrawPanel();
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronize(
  TSynchronizeController * /*Sender*/, const wstring LocalDirectory,
  const wstring RemoteDirectory, const TCopyParamType & CopyParam,
  const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options, bool Full)
{
  try
  {
    int PParams = Params.Params;
    if (!Full)
    {
      PParams |= TTerminal::spNoRecurse | TTerminal::spUseCache |
        TTerminal::spDelayProgress | TTerminal::spSubDirs;
    }
    else
    {
      // if keepuptodate is non-recursive,
      // full sync before has to be non-recursive as well
      if (FLAGCLEAR(Params.Options, soRecurse))
      {
        PParams |= TTerminal::spNoRecurse;
      }
    }
    Synchronize(LocalDirectory, RemoteDirectory, TTerminal::smRemote, CopyParam,
      PParams, Checklist, Options);
  }
  catch(exception & E)
  {
    HandleException(&E);
    throw;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, const wstring Directory,
  const wstring ErrorStr)
{
  wstring Message;
  if (!Directory.empty())
  {
    Message = FORMAT(GetMsg(WATCH_ERROR_DIRECTORY), (Directory));
  }
  else
  {
    Message = GetMsg(WATCH_ERROR_GENERAL);
  }

  MoreMessageDialog(Message, NULL, qtError, qaOK);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronizeTooManyDirectories(
  TSynchronizeController * /*Sender*/, int & MaxDirectories)
{
  if (MaxDirectories < GUIConfiguration->MaxWatchDirectories)
  {
    MaxDirectories = GUIConfiguration->MaxWatchDirectories;
  }
  else
  {
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    int Result = MoreMessageDialog(
      FORMAT(GetMsg(TOO_MANY_WATCH_DIRECTORIES), (MaxDirectories, MaxDirectories)), NULL,
      qtConfirmation, qaYes | qaNo, &Params);

    if ((Result == qaYes) || (Result == qaNeverAskAgain))
    {
      MaxDirectories *= 2;
      if (Result == qaNeverAskAgain)
      {
        GUIConfiguration->MaxWatchDirectories = MaxDirectories;
      }
    }
    else
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::CustomCommandGetParamValue(
  const wstring AName, wstring & Value)
{
  wstring Name = AName;
  if (Name.empty())
  {
    Name = GetMsg(APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!FPlugin->InputBox(GetMsg(APPLY_COMMAND_PARAM_TITLE),
        Name, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TransferFiles(bool Move)
{
  if (Move)
  {
    RequireCapability(fcRemoteMove);
  }

  if (Move || EnsureCommandSessionFallback(fcRemoteCopy))
  {
    TStrings * FileList = CreateSelectedFileList(osRemote);
    if (FileList)
    {
      assert(!FPanelItems);

      try
      {
        wstring Target = FTerminal->GetCurrentDirectory();
        wstring FileMask = L"*.*";
        if (RemoteTransferDialog(FileList, Target, FileMask, Move))
        {
          try
          {
            if (Move)
            {
              Terminal->MoveFiles(FileList, Target, FileMask);
            }
            else
            {
              Terminal->CopyFiles(FileList, Target, FileMask);
            }
          }
          catch(...)
          {
            PanelInfo->ApplySelection();
            if (UpdatePanel())
            {
              RedrawPanel();
            }
          }
        }
      }
      catch(...)
      {
        delete FileList;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::RenameFile()
{
  TFarPanelItem * PanelItem = PanelInfo->FocusedItem;
  assert(PanelItem != NULL);

  if (!PanelItem->IsParentDirectory)
  {
    RequireCapability(fcRename);

    TRemoteFile * File = static_cast<TRemoteFile *>(PanelItem->UserData);
    wstring NewName = File->GetFileName();
    if (RenameFileDialog(File, NewName))
    {
      try
      {
        Terminal->RenameFile(File, NewName, true);
      }
      catch(...)
      {
        if (UpdatePanel())
        {
          RedrawPanel();
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::FileProperties()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  if (FileList)
  {
    assert(!FPanelItems);

    try
    {
      TRemoteProperties CurrentProperties;

      bool Cont = true;
      if (!Terminal->LoadFilesProperties(FileList))
      {
        if (UpdatePanel())
        {
          RedrawPanel();
        }
        else
        {
          Cont = false;
        }
      }

      if (Cont)
      {
        CurrentProperties = TRemoteProperties::CommonProperties(FileList);

        int Flags = 0;
        if (FTerminal->IsCapable[fcModeChanging]) Flags |= cpMode;
        if (FTerminal->IsCapable[fcOwnerChanging]) Flags |= cpOwner;
        if (FTerminal->IsCapable[fcGroupChanging]) Flags |= cpGroup;

        TRemoteProperties NewProperties = CurrentProperties;
        if (PropertiesDialog(FileList, FTerminal->GetCurrentDirectory(),
            FTerminal->GetGroups(), FTerminal->Users, &NewProperties, Flags))
        {
          NewProperties = TRemoteProperties::ChangedProperties(CurrentProperties,
            NewProperties);
          try
          {
            FTerminal->ChangeFilesProperties(FileList, &NewProperties);
          }
          catch(...)
          {
            PanelInfo->ApplySelection();
            if (UpdatePanel())
            {
              RedrawPanel();
            }
          }
        }
      }
    }
    catch(...)
    {
      delete FileList;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::InsertTokenOnCommandLine(wstring Token, bool Separate)
{
  if (!Token.empty())
  {
    if (Token.Pos(" ") > 0)
    {
      Token = FORMAT("\"%s\"", (Token));
    }

    if (Separate)
    {
      Token += L" ";
    }

    FarControl(FCTL_INSERTCMDLINE, Token.c_str());
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::InsertSessionNameOnCommandLine()
{
  TFarPanelItem * Focused = PanelInfo->FocusedItem;

  if (Focused != NULL)
  {
    TSessionData * SessionData = reinterpret_cast<TSessionData *>(Focused->UserData);
    wstring Name;
    if (SessionData != NULL)
    {
      Name = SessionData->Name;
    }
    else
    {
      Name = UnixIncludeTrailingBackslash(FSessionsFolder);
      if (!Focused->IsParentDirectory)
      {
        Name = UnixIncludeTrailingBackslash(Name + Focused->GetFileName());
      }
    }
    InsertTokenOnCommandLine(Name, true);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::InsertFileNameOnCommandLine(bool Full)
{
  TFarPanelItem * Focused = PanelInfo->FocusedItem;

  if (Focused != NULL)
  {
    if (!Focused->IsParentDirectory)
    {
      TRemoteFile * File = reinterpret_cast<TRemoteFile *>(Focused->UserData);
      if (File != NULL)
      {
        if (Full)
        {
          InsertTokenOnCommandLine(File->FullFileName, true);
        }
        else
        {
          InsertTokenOnCommandLine(File->GetFileName(), true);
        }
      }
    }
    else
    {
      InsertTokenOnCommandLine(UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory()), true);
    }
  }
}
//---------------------------------------------------------------------------
// not used
void TWinSCPFileSystem::InsertPathOnCommandLine()
{
  InsertTokenOnCommandLine(FTerminal->GetCurrentDirectory(), false);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::CopyFullFileNamesToClipboard()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  TStrings * FileNames = new TStringList();
  try
  {
    if (FileList != NULL)
    {
      for (int Index = 0; Index < FileList->Count; Index++)
      {
        TRemoteFile * File = reinterpret_cast<TRemoteFile *>(FileList->Objects[Index]);
        if (File != NULL)
        {
          FileNames->Add(File->FullFileName);
        }
        else
        {
          assert(false);
        }
      }
    }
    else
    {
      if ((PanelInfo->SelectedCount == 0) &&
          PanelInfo->FocusedItem->IsParentDirectory)
      {
        FileNames->Add(UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory()));
      }
    }

    FPlugin->FarCopyToClipboard(FileNames);
  }
  catch(...)
  {
    delete FileList;
    delete FileNames;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::GetSpaceAvailable(const wstring Path,
  TSpaceAvailable & ASpaceAvailable, bool & Close)
{
  // terminal can be already closed (e.g. dropped connection)
  if ((Terminal != NULL) && Terminal->IsCapable[fcCheckingSpaceAvailable])
  {
    try
    {
      Terminal->SpaceAvailable(Path, ASpaceAvailable);
    }
    catch(exception & E)
    {
      if (!Terminal->GetActive())
      {
        Close = true;
      }
      HandleException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ShowInformation()
{
  TSessionInfo SessionInfo = Terminal->GetSessionInfo();
  TFileSystemInfo FileSystemInfo = Terminal->GetFileSystemInfo();
  TGetSpaceAvailable OnGetSpaceAvailable = NULL;
  if (Terminal->IsCapable[fcCheckingSpaceAvailable])
  {
    OnGetSpaceAvailable = GetSpaceAvailable;
  }
  FileSystemInfoDialog(SessionInfo, FileSystemInfo, Terminal->GetCurrentDirectory(),
    OnGetSpaceAvailable);
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::AreCachesEmpty()
{
  assert(Connected());
  return FTerminal->AreCachesEmpty;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ClearCaches()
{
  assert(Connected());
  FTerminal->ClearCaches();
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::OpenSessionInPutty()
{
  assert(Connected());
  ::OpenSessionInPutty(GUIConfiguration->PuttyPath, FTerminal->GetSessionData(),
    GUIConfiguration->PuttyPassword ? Terminal->Password : wstring());
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::QueueShow(bool ClosingPlugin)
{
  assert(Connected());
  assert(FQueueStatus != NULL);
  QueueDialog(FQueueStatus, ClosingPlugin);
  ProcessQueue(true);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::OpenDirectory(bool Add)
{
  TBookmarkList * BookmarkList = new TBookmarkList();
  try
  {
    wstring Directory = FTerminal->GetCurrentDirectory();
    wstring SessionKey = FTerminal->GetSessionData()->SessionKey;
    TBookmarkList * CurrentBookmarkList;

    CurrentBookmarkList = FarConfiguration->Bookmarks[SessionKey];
    if (CurrentBookmarkList != NULL)
    {
      BookmarkList->Assign(CurrentBookmarkList);
    }

    if (Add)
    {
      TBookmark * Bookmark = new TBookmark;
      Bookmark->Remote = Directory;
      Bookmark->Name = Directory;
      BookmarkList->Add(Bookmark);
      FarConfiguration->Bookmarks[SessionKey] = BookmarkList;
    }

    bool Result = OpenDirectoryDialog(Add, Directory, BookmarkList);

    FarConfiguration->Bookmarks[SessionKey] = BookmarkList;

    if (Result)
    {
      FTerminal->ChangeDirectory(Directory);
      if (UpdatePanel(true))
      {
        RedrawPanel();
      }
    }
  }
  catch(...)
  {
    delete BookmarkList;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::HomeDirectory()
{
  FTerminal->HomeDirectory();
  if (UpdatePanel(true))
  {
    RedrawPanel();
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::IsSynchronizedBrowsing()
{
  return FSynchronisingBrowse;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ToggleSynchronizeBrowsing()
{
  FSynchronisingBrowse = !FSynchronisingBrowse;

  if (FarConfiguration->ConfirmSynchronizedBrowsing)
  {
    wstring Message = FSynchronisingBrowse ?
      GetMsg(SYNCHRONIZE_BROWSING_ON) : GetMsg(SYNCHRONIZE_BROWSING_OFF);
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    if (MoreMessageDialog(Message, NULL, qtInformation, qaOK, &Params) ==
          qaNeverAskAgain)
    {
      FarConfiguration->ConfirmSynchronizedBrowsing = false;
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeBrowsing(wstring NewPath)
{
  bool Result;
  TFarPanelInfo * AnotherPanel = AnotherPanelInfo;
  wstring OldPath = AnotherPanel->GetCurrentDirectory();
  // IncludeTrailingBackslash to expand C: to C:\.
  if (!FarControl(FCTL_SETANOTHERPANELDIR,
        IncludeTrailingBackslash(NewPath).c_str()))
  {
    Result = false;
  }
  else
  {
    ResetCachedInfo();
    AnotherPanel = AnotherPanelInfo;
    if (!ComparePaths(AnotherPanel->GetCurrentDirectory(), NewPath))
    {
      // FAR WORKAROUND
      // If FCTL_SETANOTHERPANELDIR above fails, Far default current
      // directory to initial (?) one. So move this back to
      // previous directory.
      FarControl(FCTL_SETANOTHERPANELDIR, OldPath.c_str());
      Result = false;
    }
    else
    {
      RedrawPanel(true);
      Result = true;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SetDirectoryEx(const wstring Dir, int OpMode)
{
  if (!SessionList() && !Connected())
  {
    return false;
  }
  // FAR WORKAROUND
  // workaround to ignore "change to root directory" command issued by FAR,
  // before file is opened for viewing/editing from "find file" dialog
  // when plugin uses UNIX style paths
  else if (OpMode & OPM_FIND && OpMode & OPM_SILENT && Dir == L"\\")
  {
    if (FSavedFindFolder.empty())
    {
      return true;
    }
    else
    {
      bool Result;
      try
      {
        Result = SetDirectoryEx(FSavedFindFolder, OpMode);
      }
      catch(...)
      {
        FSavedFindFolder = L"";
      }
      return Result;
    }
  }
  else
  {
    if (OpMode & OPM_FIND && FSavedFindFolder.empty())
    {
      FSavedFindFolder = FTerminal->GetCurrentDirectory();
    }

    if (SessionList())
    {
      FSessionsFolder = AbsolutePath("/" + FSessionsFolder, Dir);
      assert(FSessionsFolder[1] == '/');
      FSessionsFolder.Delete(1, 1);
      FNewSessionsFolder = L"";
    }
    else
    {
      assert(!FNoProgress);
      bool Normal = FLAGCLEAR(OpMode, OPM_FIND | OPM_SILENT);
      wstring PrevPath = FTerminal->GetCurrentDirectory();
      FNoProgress = !Normal;
      if (!FNoProgress)
      {
        FPlugin->ShowConsoleTitle(GetMsg(CHANGING_DIRECTORY_TITLE));
      }
      FTerminal->ExceptionOnFail = true;
      try
      {
        if (Dir == L"\\")
        {
          FTerminal->ChangeDirectory(ROOTDIRECTORY);
        }
        else if ((Dir == PARENTDIRECTORY) && (FTerminal->GetCurrentDirectory() == ROOTDIRECTORY))
        {
          ClosePlugin();
        }
        else
        {
          FTerminal->ChangeDirectory(Dir);
        }
      }
      catch(...)
      {
        FTerminal->ExceptionOnFail = false;
        if (!FNoProgress)
        {
          FPlugin->ClearConsoleTitle();
        }
        FNoProgress = false;
      }

      if (Normal && FSynchronisingBrowse &&
          (PrevPath != FTerminal->GetCurrentDirectory()))
      {
        TFarPanelInfo * AnotherPanel = AnotherPanelInfo;
        if (AnotherPanel->IsPlugin || (AnotherPanel->Type != ptFile))
        {
          MoreMessageDialog(GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED), NULL, qtError, qaOK);
        }
        else
        {
          try
          {
            wstring RemotePath = UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory());
            wstring FullPrevPath = UnixIncludeTrailingBackslash(PrevPath);
            wstring ALocalPath;
            if (RemotePath.SubString(1, FullPrevPath.Length()) == FullPrevPath)
            {
              ALocalPath = IncludeTrailingBackslash(AnotherPanel->GetCurrentDirectory()) +
                FromUnixPath(RemotePath.SubString(FullPrevPath.Length() + 1,
                  RemotePath.Length() - FullPrevPath.Length()));
            }
            else if (FullPrevPath.SubString(1, RemotePath.Length()) == RemotePath)
            {
              wstring NewLocalPath;
              ALocalPath = ExcludeTrailingBackslash(AnotherPanel->GetCurrentDirectory());
              while (!UnixComparePaths(FullPrevPath, RemotePath))
              {
                NewLocalPath = ExcludeTrailingBackslash(ExtractFileDir(ALocalPath));
                if (NewLocalPath == ALocalPath)
                {
                  Abort();
                }
                ALocalPath = NewLocalPath;
                FullPrevPath = UnixExtractFilePath(UnixExcludeTrailingBackslash(FullPrevPath));
              }
            }
            else
            {
              Abort();
            }

            if (!SynchronizeBrowsing(ALocalPath))
            {
              if (MoreMessageDialog(FORMAT(GetMsg(SYNC_DIR_BROWSE_CREATE), (ALocalPath)),
                    NULL, qtInformation, qaYes | qaNo) == qaYes)
              {
                if (!ForceDirectories(ALocalPath))
                {
                  RaiseLastOSError();
                }
                else
                {
                  if (!SynchronizeBrowsing(ALocalPath))
                  {
                    Abort();
                  }
                }
              }
              else
              {
                FSynchronisingBrowse = false;
              }
            }
          }
          catch(exception & E)
          {
            FSynchronisingBrowse = false;
            WinSCPPlugin()->ShowExtendedException(&E);
            MoreMessageDialog(GetMsg(SYNC_DIR_BROWSE_ERROR), NULL, qtInformation, qaOK);
          }
        }
      }
    }

    return true;
  }
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::MakeDirectoryEx(wstring & Name, int OpMode)
{
  if (Connected())
  {
    assert(!(OpMode & OPM_SILENT) || !Name.empty());

    TRemoteProperties Properties = GUIConfiguration->NewDirectoryProperties;
    bool SaveSettings = false;

    if ((OpMode & OPM_SILENT) ||
        CreateDirectoryDialog(Name, &Properties, SaveSettings))
    {
      if (SaveSettings)
      {
        GUIConfiguration->NewDirectoryProperties = Properties;
      }

      FPlugin->ShowConsoleTitle(GetMsg(CREATING_FOLDER));
      try
      {
        FTerminal->CreateDirectory(Name, &Properties);
      }
      catch(...)
      {
        FPlugin->ClearConsoleTitle();
      }
      return 1;
    }
    else
    {
      Name = L"";
      return -1;
    }
  }
  else if (SessionList())
  {
    assert(!(OpMode & OPM_SILENT) || !Name.empty());

    if (((OpMode & OPM_SILENT) ||
         FPlugin->InputBox(GetMsg(CREATE_FOLDER_TITLE),
           StripHotKey(GetMsg(CREATE_FOLDER_PROMPT)),
           Name, 0, MAKE_SESSION_FOLDER_HISTORY)) &&
        !Name.empty())
    {
      TSessionData::ValidateName(Name);
      FNewSessionsFolder = Name;
      return 1;
    }
    else
    {
      Name = L"";
      return -1;
    }
  }
  else
  {
    Name = L"";
    return -1;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DeleteSession(TSessionData * Data, void * /*Param*/)
{
  Data->Remove();
  StoredSessions->Remove(Data);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ProcessSessions(TList * PanelItems,
  TProcessSessionEvent ProcessSession, void * Param)
{
  for (int Index = 0; Index < PanelItems->Count; Index++)
  {
    TFarPanelItem * PanelItem = (TFarPanelItem *)PanelItems->Items[Index];
    assert(PanelItem);
    if (PanelItem->IsFile)
    {
      if (PanelItem->UserData != NULL)
      {
        ProcessSession(static_cast<TSessionData *>(PanelItem->UserData), Param);
        PanelItem->Selected = false;
      }
      else
      {
        assert(PanelItem->GetFileName() == GetMsg(NEW_SESSION_HINT));
      }
    }
    else
    {
      assert(PanelItem->UserData == NULL);
      wstring Folder = UnixIncludeTrailingBackslash(
        UnixIncludeTrailingBackslash(FSessionsFolder) + PanelItem->GetFileName());
      int Index = 0;
      while (Index < StoredSessions->Count)
      {
        TSessionData * Data = StoredSessions->Sessions[Index];
        if (Data->Name.SubString(1, Folder.Length()) == Folder)
        {
          ProcessSession(Data, Param);
          if (StoredSessions->Sessions[Index] != Data)
          {
            Index--;
          }
        }
        Index++;
      }
      PanelItem->Selected = false;
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::DeleteFilesEx(TList * PanelItems, int OpMode)
{
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osRemote);
    FPanelItems = PanelItems;
    try
    {
      wstring Query;
      bool Recycle = FTerminal->GetSessionData()->DeleteToRecycleBin &&
        !FTerminal->IsRecycledFile(FFileList->Strings[0]);
      if (PanelItems->Count > 1)
      {
        Query = FORMAT(GetMsg(Recycle ? RECYCLE_FILES_CONFIRM : DELETE_FILES_CONFIRM),
          (PanelItems->Count));
      }
      else
      {
        Query = FORMAT(GetMsg(Recycle ? RECYCLE_FILE_CONFIRM : DELETE_FILE_CONFIRM),
          (((TFarPanelItem *)PanelItems->Items[0])->GetFileName()));
      }

      if ((OpMode & OPM_SILENT) || !FarConfiguration->ConfirmDeleting ||
        (MoreMessageDialog(Query, NULL, qtConfirmation, qaOK | qaCancel) == qaOK))
      {
        FTerminal->DeleteFiles(FFileList);
      }
    }
    catch(...)
    {
      FPanelItems = NULL;
      SAFE_DESTROY(FFileList);
    }
    return true;
  }
  else if (SessionList())
  {
    if ((OpMode & OPM_SILENT) || !FarConfiguration->ConfirmDeleting ||
      (MoreMessageDialog(GetMsg(DELETE_SESSIONS_CONFIRM), NULL, qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      ProcessSessions(PanelItems, DeleteSession, NULL);
    }
    return true;
  }
  else
  {
    return false;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::QueueAddItem(TQueueItem * Item)
{
  FarConfiguration->CacheFarSettings();
  FQueue->AddItem(Item);
}
//---------------------------------------------------------------------------
struct TExportSessionParam
{
  wstring DestPath;
};
//---------------------------------------------------------------------------
int TWinSCPFileSystem::GetFilesEx(TList * PanelItems, bool Move,
  wstring & DestPath, int OpMode)
{
  int Result;
  if (Connected())
  {
    // FAR WORKAROUND
    // is it?
    // Probable reason was that search result window displays files from several
    // directories and the plugin can hold data for one directory only
    if (OpMode & OPM_FIND)
    {
      throw exception(GetMsg(VIEW_FROM_FIND_NOT_SUPPORTED));
    }

    FFileList = CreateFileList(PanelItems, osRemote);
    try
    {
      bool EditView = (OpMode & (OPM_EDIT | OPM_VIEW)) != 0;
      bool Confirmed =
        (OpMode & OPM_SILENT) &&
        (!EditView || FarConfiguration->EditorDownloadDefaultMode);

      TCopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
      if (EditView)
      {
        EditViewCopyParam(CopyParam);
      }

      // these parameters are known in advance
      int Params =
        FLAGMASK(Move, cpDelete);

      if (!Confirmed)
      {
        int CopyParamAttrs =
          Terminal->UsableCopyParamAttrs(Params).Download |
          FLAGMASK(EditView, cpaNoExcludeMask);
        int Options =
          FLAGMASK(EditView, coTempTransfer | coDisableNewerOnly);
        Confirmed = CopyDialog(false, Move, FFileList, DestPath,
          &CopyParam, Options, CopyParamAttrs);

        if (Confirmed && !EditView && CopyParam.Queue)
        {
          // these parameters are known only after transfer dialog
          Params |=
            FLAGMASK(CopyParam.QueueNoConfirmation, cpNoConfirmation) |
            FLAGMASK(CopyParam.NewerOnly, cpNewerOnly);
          QueueAddItem(new TDownloadQueueItem(FTerminal, FFileList,
            DestPath, &CopyParam, Params));
          Confirmed = false;
        }
      }

      if (Confirmed)
      {
        if ((FFileList->Count == 1) && (OpMode & OPM_EDIT))
        {
          FOriginalEditFile = IncludeTrailingBackslash(DestPath) +
            UnixExtractFileName(FFileList->Strings[0]);
          FLastEditFile = FOriginalEditFile;
          FLastEditCopyParam = CopyParam;
          FLastEditorID = -1;
        }
        else
        {
          FOriginalEditFile = L"";
          FLastEditFile = L"";
          FLastEditorID = -1;
        }

        FPanelItems = PanelItems;
        // these parameters are known only after transfer dialog
        Params |=
          FLAGMASK(EditView, cpTemporary) |
          FLAGMASK(CopyParam.NewerOnly, cpNewerOnly);
        FTerminal->CopyToLocal(FFileList, DestPath, &CopyParam, Params);
        Result = 1;
      }
      else
      {
        Result = -1;
      }
    }
    catch(...)
    {
      FPanelItems = NULL;
      SAFE_DESTROY(FFileList);
    }
  }
  else if (SessionList())
  {
    wstring Title = GetMsg(EXPORT_SESSION_TITLE);
    wstring Prompt;
    if (PanelItems->Count == 1)
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSION_PROMPT),
        (((TFarPanelItem *)PanelItems->Items[0])->GetFileName()));
    }
    else
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSIONS_PROMPT), (PanelItems->Count));
    }

    bool AResult = (OpMode & OPM_SILENT) ||
      FPlugin->InputBox(Title, Prompt, DestPath, 0, "Copy");
    if (AResult)
    {
      TExportSessionParam Param;
      Param.DestPath = DestPath;
      ProcessSessions(PanelItems, ExportSession, &Param);
      Result = 1;
    }
    else
    {
      Result = -1;
    }
  }
  else
  {
    Result = -1;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ExportSession(TSessionData * Data, void * AParam)
{
  TExportSessionParam & Param = *static_cast<TExportSessionParam *>(AParam);

  THierarchicalStorage * Storage = NULL;
  TSessionData * ExportData = NULL;
  TSessionData * FactoryDefaults = new TSessionData("");
  try
  {
    ExportData = new TSessionData(Data->Name);
    ExportData->Assign(Data);
    ExportData->Modified = true;
    Storage = new TIniFileStorage(IncludeTrailingBackslash(Param.DestPath) +
      GUIConfiguration->DefaultCopyParam.ValidLocalFileName(ExportData->Name) + ".ini");
    if (Storage->OpenSubKey(Configuration->StoredSessionsSubKey, true))
    {
      ExportData->Save(Storage, false, FactoryDefaults);
    }
  }
  catch(...)
  {
    delete FactoryDefaults;
    delete Storage;
    delete ExportData;
  }
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::UploadFiles(bool Move, int OpMode, bool Edit,
  wstring DestPath)
{
  int Result = 1;
  bool Confirmed = (OpMode & OPM_SILENT);
  bool Ask = !Confirmed;

  TCopyParamType CopyParam;

  if (Edit)
  {
    CopyParam = FLastEditCopyParam;
    Confirmed = FarConfiguration->EditorUploadSameOptions;
    Ask = false;
  }
  else
  {
    CopyParam = GUIConfiguration->DefaultCopyParam;
  }

  // these parameters are known in advance
  int Params =
    FLAGMASK(Move, cpDelete);

  if (!Confirmed)
  {
    int CopyParamAttrs =
      Terminal->UsableCopyParamAttrs(Params).Upload |
      FLAGMASK(Edit, (cpaNoExcludeMask | cpaNoClearArchive));
    // heurictics: do not ask for target directory when uploaded file
    // was downloaded in edit mode
    int Options =
      FLAGMASK(Edit, coTempTransfer) |
      FLAGMASK(Edit || !Terminal->IsCapable[fcNewerOnlyUpload], coDisableNewerOnly);
    Confirmed = CopyDialog(true, Move, FFileList, DestPath,
      &CopyParam, Options, CopyParamAttrs);

    if (Confirmed && !Edit && CopyParam.Queue)
    {
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(CopyParam.QueueNoConfirmation, cpNoConfirmation) |
        FLAGMASK(CopyParam.NewerOnly, cpNewerOnly);
      QueueAddItem(new TUploadQueueItem(FTerminal, FFileList,
        DestPath, &CopyParam, Params));
      Confirmed = false;
    }
  }

  if (Confirmed)
  {
    assert(!FNoProgressFinish);
    // it does not make sense to unselect file being uploaded from editor,
    // moreover we may upload the file under name that does not exist in
    // remote panel
    FNoProgressFinish = Edit;
    try
    {
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(!Ask, cpNoConfirmation) |
        FLAGMASK(Edit, cpTemporary) |
        FLAGMASK(CopyParam.NewerOnly, cpNewerOnly);
      FTerminal->CopyToRemote(FFileList, DestPath, &CopyParam, Params);
    }
    catch(...)
    {
      FNoProgressFinish = false;
    }
  }
  else
  {
    Result = -1;
  }
  return Result;
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::PutFilesEx(TList * PanelItems, bool Move, int OpMode)
{
  int Result;
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osLocal);
    try
    {
      FPanelItems = PanelItems;

      // if file is saved under different name, FAR tries to upload original file,
      // but let's be robust and check for new name, in case it changes.
      // OMP_EDIT is set since 1.70 final, only.
      // When comparing, beware that one path may be long path and the other short
      // (since 1.70 alpha 6, DestPath in GetFiles is short path,
      // while current path in PutFiles is long path)
      if (FLAGCLEAR(OpMode, OPM_SILENT) && (FFileList->Count == 1) &&
          (CompareFileName(FFileList->Strings[0], FOriginalEditFile) ||
           CompareFileName(FFileList->Strings[0], FLastEditFile)))
      {
        // editor should be closed already
        assert(FLastEditorID < 0);

        if (FarConfiguration->EditorUploadOnSave)
        {
          // already uploaded from EE_REDRAW
          Result = -1;
        }
        else
        {
          // just in case file was saved under different name
          FFileList->Strings[0] = FLastEditFile;

          FOriginalEditFile = L"";
          FLastEditFile = L"";

          Result = UploadFiles(Move, OpMode, true, FTerminal->GetCurrentDirectory());
        }
      }
      else
      {
        Result = UploadFiles(Move, OpMode, false, FTerminal->GetCurrentDirectory());
      }
    }
    catch(...)
    {
      FPanelItems = NULL;
      SAFE_DESTROY(FFileList);
    }
  }
  else if (SessionList())
  {
    if (!ImportSessions(PanelItems, Move, OpMode))
    {
      Result = -1;
    }
    else
    {
      Result = 1;
    }
  }
  else
  {
    Result = -1;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ImportSessions(TList * PanelItems, bool /*Move*/,
  int OpMode)
{
  bool Result = (OpMode & OPM_SILENT) ||
    (MoreMessageDialog(GetMsg(IMPORT_SESSIONS_PROMPT), NULL,
      qtConfirmation, qaOK | qaCancel) == qaOK);

  if (Result)
  {
    wstring FileName;
    TFarPanelItem * PanelItem;
    for (int i = 0; i < PanelItems->Count; i++)
    {
      PanelItem = (TFarPanelItem *)PanelItems->Items[i];
      bool AnyData = false;
      FileName = PanelItem->GetFileName();
      if (PanelItem->IsFile)
      {
        THierarchicalStorage * Storage = NULL;
        try
        {
          Storage = new TIniFileStorage(IncludeTrailingBackslash(GetCurrentDir()) + FileName);
          if (Storage->OpenSubKey(Configuration->StoredSessionsSubKey, false) &&
              Storage->HasSubKeys())
          {
            AnyData = true;
            StoredSessions->Load(Storage, true);
            // modified only, explicit
            StoredSessions->Save(false, true);
          }
        }
        catch(...)
        {
          delete Storage;
        }
      }
      if (!AnyData)
      {
        throw exception(FORMAT(GetMsg(IMPORT_SESSIONS_EMPTY), (FileName)));
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateFocusedFileList(
  TOperationSide Side, TFarPanelInfo * PanelInfo)
{
  if (PanelInfo == NULL)
  {
    PanelInfo = this->PanelInfo;
  }

  TStrings * Result;
  TFarPanelItem * PanelItem = PanelInfo->FocusedItem;
  if (PanelItem->IsParentDirectory)
  {
    Result = NULL;
  }
  else
  {
    Result = new TStringList();
    assert((Side == osLocal) || PanelItem->UserData);
    wstring FileName = PanelItem->GetFileName();
    if (Side == osLocal)
    {
      FileName = IncludeTrailingBackslash(PanelInfo->GetCurrentDirectory()) + FileName;
    }
    Result->AddObject(FileName, (TObject *)PanelItem->UserData);
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateSelectedFileList(
  TOperationSide Side, TFarPanelInfo * PanelInfo)
{
  if (PanelInfo == NULL)
  {
    PanelInfo = this->PanelInfo;
  }

  TStrings * Result;
  if (PanelInfo->SelectedCount > 0)
  {
    Result = CreateFileList(PanelInfo->Items, Side, true,
      PanelInfo->GetCurrentDirectory());
  }
  else
  {
    Result = CreateFocusedFileList(Side, PanelInfo);
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateFileList(TList * PanelItems,
  TOperationSide Side, bool SelectedOnly, wstring Directory, bool FileNameOnly,
  TStrings * AFileList)
{
  TStrings * FileList = (AFileList == NULL ? new TStringList() : AFileList);
  try
  {
    wstring FileName;
    TFarPanelItem * PanelItem;
    TObject * Data = NULL;
    for (int Index = 0; Index < PanelItems->Count; Index++)
    {
      PanelItem = (TFarPanelItem *)PanelItems->Items[Index];
      assert(PanelItem);
      if ((!SelectedOnly || PanelItem->Selected) &&
          !PanelItem->IsParentDirectory)
      {
        FileName = PanelItem->GetFileName();
        if (Side == osRemote)
        {
          Data = (TRemoteFile *)PanelItem->UserData;
          assert(Data);
        }
        if (Side == osLocal)
        {
          if (ExtractFilePath(FileName).empty())
          {
            if (!FileNameOnly)
            {
              if (Directory.empty())
              {
                Directory = GetCurrentDir();
              }
              FileName = IncludeTrailingBackslash(Directory) + FileName;
            }
          }
          else
          {
            if (FileNameOnly)
            {
              FileName = ExtractFileName(FileName);
            }
          }
        }
        FileList->AddObject(FileName, Data);
      }
    }

    if (FileList->Count == 0)
    {
      Abort();
    }
  }
  catch (...)
  {
    if (AFileList == NULL)
    {
      delete FileList;
    }
    throw;
  }
  return FileList;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::SaveSession()
{
  if (!FTerminal->GetSessionData()->Name.empty())
  {
    FTerminal->GetSessionData()->RemoteDirectory = FTerminal->GetCurrentDirectory();

    TSessionData * Data;
    Data = (TSessionData *)StoredSessions->FindByName(FTerminal->GetSessionData()->Name);
    if (Data)
    {
      bool Changed = false;
      if (Terminal->GetSessionData()->UpdateDirectories)
      {
        Data->RemoteDirectory = Terminal->GetSessionData()->RemoteDirectory;
        Changed = true;
      }

      if (Changed)
      {
        // modified only, implicit
        StoredSessions->Save(false, false);
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::Connect(TSessionData * Data)
{
  bool Result = false;
  assert(!FTerminal);
  FTerminal = new TTerminal(Data, Configuration);
  try
  {
    FTerminal->OnQueryUser = TerminalQueryUser;
    FTerminal->OnPromptUser = TerminalPromptUser;
    FTerminal->OnDisplayBanner = TerminalDisplayBanner;
    FTerminal->OnShowExtendedException = TerminalShowExtendedException;
    FTerminal->OnChangeDirectory = TerminalChangeDirectory;
    FTerminal->OnReadDirectory = TerminalReadDirectory;
    FTerminal->OnStartReadDirectory = TerminalStartReadDirectory;
    FTerminal->OnReadDirectoryProgress = TerminalReadDirectoryProgress;
    FTerminal->OnInformation = TerminalInformation;
    FTerminal->OnFinished = OperationFinished;
    FTerminal->OnProgress = OperationProgress;
    FTerminal->OnDeleteLocalFile = TerminalDeleteLocalFile;
    ConnectTerminal(FTerminal);

    FTerminal->OnClose = TerminalClose;

    assert(FQueue == NULL);
    FQueue = new TTerminalQueue(FTerminal, Configuration);
    FQueue->TransfersLimit = GUIConfiguration->QueueTransfersLimit;
    FQueue->OnQueryUser = TerminalQueryUser;
    FQueue->OnPromptUser = TerminalPromptUser;
    FQueue->OnShowExtendedException = TerminalShowExtendedException;
    FQueue->OnListUpdate = QueueListUpdate;
    FQueue->OnQueueItemUpdate = QueueItemUpdate;
    FQueue->OnEvent = QueueEvent;

    assert(FQueueStatus == NULL);
    FQueueStatus = FQueue->CreateStatus(NULL);

    // TODO: Create instance of TKeepaliveThread here, once its implementation
    // is complete

    Result = true;
  }
  catch(exception &E)
  {
    FTerminal->ShowExtendedException(&E);
    SAFE_DESTROY(FTerminal);
    delete FQueue;
    FQueue = NULL;
  }

  FSynchronisingBrowse = GUIConfiguration->SynchronizeBrowsing;

  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ConnectTerminal(TTerminal * Terminal)
{
  Terminal->Open();
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalClose(TObject * /*Sender*/)
{
  // Plugin closure is now invoked from HandleException
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::LogAuthentication(
  TTerminal * Terminal, wstring Msg)
{
  assert(FAuthenticationLog != NULL);
  FAuthenticationLog->Add(Msg);
  TStringList * AuthenticationLogLines = new TStringList();
  try
  {
    int Width = 42;
    int Height = 11;
    FarWrapText(FAuthenticationLog->Text.TrimRight(), AuthenticationLogLines, Width);
    int Count;
    wstring Message;
    if (AuthenticationLogLines->Count == 0)
    {
      Message = wstring::StringOfChar(' ', Width) + "\n";
      Count = 1;
    }
    else
    {
      while (AuthenticationLogLines->Count > Height)
      {
        AuthenticationLogLines->Delete(0);
      }
      AuthenticationLogLines->Strings[0] =
        AuthenticationLogLines->Strings[0] +
          wstring::StringOfChar(' ', Width - AuthenticationLogLines->Strings[0].Length());
      Message = AnsiReplaceStr(AuthenticationLogLines->Text, "\r", "");
      Count = AuthenticationLogLines->Count;
    }

    Message += wstring::StringOfChar('\n', Height - Count);

    FPlugin->Message(0, Terminal->GetSessionData()->GetSessionName(), Message);
  }
  catch(...)
  {
    delete AuthenticationLogLines;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalInformation(
  TTerminal * Terminal, const wstring & Str, bool /*Status*/, bool Active)
{
  if (Active)
  {
    if (Terminal->Status == ssOpening)
    {
      if (FAuthenticationLog == NULL)
      {
        FAuthenticationLog = new TStringList();
        FPlugin->SaveScreen(FAuthenticationSaveScreenHandle);
        FPlugin->ShowConsoleTitle(Terminal->GetSessionData()->GetSessionName());
      }

      LogAuthentication(Terminal, Str);
      FPlugin->UpdateConsoleTitle(Str);
    }
  }
  else
  {
    if (FAuthenticationLog != NULL)
    {
      FPlugin->ClearConsoleTitle();
      FPlugin->RestoreScreen(FAuthenticationSaveScreenHandle);
      SAFE_DESTROY(FAuthenticationLog);
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalChangeDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    wstring Directory = FTerminal->GetCurrentDirectory();
    int Index = FPathHistory->IndexOf(Directory);
    if (Index >= 0)
    {
      FPathHistory->Delete(Index);
    }

    if (!FLastPath.empty())
    {
      FPathHistory->Add(FLastPath);
    }

    FLastPath = Directory;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalStartReadDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    FPlugin->ShowConsoleTitle(GetMsg(READING_DIRECTORY_TITLE));
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalReadDirectoryProgress(
  TObject * /*Sender*/, int Progress, bool & Cancel)
{
  if (Progress < 0)
  {
    if (!FNoProgress && (Progress == -2))
    {
      MoreMessageDialog(GetMsg(DIRECTORY_READING_CANCELLED), NULL,
         qtWarning, qaOK);
    }
  }
  else
  {
    if (FPlugin->CheckForEsc())
    {
      Cancel = true;
    }

    if (!FNoProgress)
    {
      FPlugin->UpdateConsoleTitle(
        FORMAT("%s (%d)", (GetMsg(READING_DIRECTORY_TITLE), Progress)));
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalReadDirectory(TObject * /*Sender*/,
  bool /*ReloadOnly*/)
{
  if (!FNoProgress)
  {
    FPlugin->ClearConsoleTitle();
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalDeleteLocalFile(const wstring FileName,
  bool Alternative)
{
  if (!RecursiveDeleteFile(FileName,
        (FLAGSET(FPlugin->FarSystemSettings(), FSS_DELETETORECYCLEBIN)) != Alternative))
  {
    throw exception(FORMAT(GetMsg(DELETE_LOCAL_FILE_ERROR), (FileName)));
  }
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::MoreMessageDialog(wstring Str,
  TStrings * MoreMessages, TQueryType Type, int Answers, const TMessageParams * Params)
{
  TMessageParams AParams;

  if ((FProgressSaveScreenHandle != 0) ||
      (FSynchronizationSaveScreenHandle != 0))
  {
    if (Params != NULL)
    {
      AParams = *Params;
    }
    Params = &AParams;
    AParams.Flags |= FMSG_WARNING;
  }

  return WinSCPPlugin()->MoreMessageDialog(Str, MoreMessages, Type,
    Answers, Params);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalQueryUser(TObject * /*Sender*/,
  const wstring Query, TStrings * MoreMessages, int Answers,
  const TQueryParams * Params, int & Answer, TQueryType Type, void * /*Arg*/)
{
  TMessageParams AParams;
  wstring AQuery = Query;

  if (Params != NULL)
  {
    if (Params->Params & qpFatalAbort)
    {
      AQuery = FORMAT(GetMsg(WARN_FATAL_ERROR), (AQuery));
    }

    AParams.Aliases = Params->Aliases;
    AParams.AliasesCount = Params->AliasesCount;
    AParams.Params = Params->Params & (qpNeverAskAgainCheck | qpAllowContinueOnError);
    AParams.Timer = Params->Timer;
    AParams.TimerEvent = Params->TimerEvent;
    AParams.TimerMessage = Params->TimerMessage;
    AParams.TimerAnswers = Params->TimerAnswers;
    AParams.Timeout = Params->Timeout;
    AParams.TimeoutAnswer = Params->TimeoutAnswer;
  }

  Answer = MoreMessageDialog(AQuery, MoreMessages, Type, Answers, &AParams);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, wstring Name, wstring Instructions,
  TStrings * Prompts, TStrings * Results, bool & Result,
  void * /*Arg*/)
{
  if (Kind == pkPrompt)
  {
    assert(Instructions.empty());
    assert(Prompts->Count == 1);
    assert(bool(Prompts->Objects[0]));
    wstring AResult = Results->Strings[0];

    Result = FPlugin->InputBox(Name, StripHotKey(Prompts->Strings[0]), AResult, FIB_NOUSELASTHISTORY);
    if (Result)
    {
      Results->Strings[0] = AResult;
    }
  }
  else
  {
    Result = PasswordDialog(Terminal->GetSessionData(), Kind, Name, Instructions,
      Prompts, Results, Terminal->StoredCredentialsTried);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalDisplayBanner(
  TTerminal * /*Terminal*/, wstring SessionName,
  const wstring & Banner, bool & NeverShowAgain, int Options)
{
  BannerDialog(SessionName, Banner, NeverShowAgain, Options);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalShowExtendedException(
  TTerminal * /*Terminal*/, exception * E, void * /*Arg*/)
{
  WinSCPPlugin()->ShowExtendedException(E);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::OperationProgress(
  TFileOperationProgressType & ProgressData, TCancelStatus & /*Cancel*/)
{
  if (FNoProgress)
  {
    return;
  }

  bool First = false;
  if (ProgressData.InProgress && !FProgressSaveScreenHandle)
  {
    FPlugin->SaveScreen(FProgressSaveScreenHandle);
    First = true;
  }

  // operation is finished (or terminated), so we hide progress form
  if (!ProgressData.InProgress && FProgressSaveScreenHandle)
  {
    FPlugin->RestoreScreen(FProgressSaveScreenHandle);
    FPlugin->ClearConsoleTitle();
  }
  else
  {
    ShowOperationProgress(ProgressData, First);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::OperationFinished(TFileOperation Operation,
  TOperationSide Side, bool /*Temp*/, const wstring & FileName, bool Success,
  bool & /*DisconnectWhenComplete*/)
{
  USEDPARAM(Side);

  if ((Operation != foCalculateSize) &&
      (Operation != foGetProperties) &&
      (Operation != foCalculateChecksum) &&
      (FSynchronizationSaveScreenHandle == 0) &&
      !FNoProgress && !FNoProgressFinish)
  {
    TFarPanelItem * PanelItem = NULL;;

    if (!FPanelItems)
    {
      TList * PanelItems = PanelInfo->Items;
      for (int Index = 0; Index < PanelItems->Count; Index++)
      {
        if (((TFarPanelItem *)PanelItems->Items[Index])->GetFileName() == FileName)
        {
          PanelItem = (TFarPanelItem *)PanelItems->Items[Index];
          break;
        }
      }
    }
    else
    {
      assert(FFileList);
      assert(FPanelItems->Count == FFileList->Count);
      int Index = FFileList->IndexOf(FileName);
      assert(Index >= 0);
      PanelItem = (TFarPanelItem *)FPanelItems->Items[Index];
    }

    assert(PanelItem->GetFileName() ==
      ((Side == osLocal) ? ExtractFileName(FileName) : FileName));
    if (Success)
    {
      PanelItem->Selected = false;
    }
  }

  if (Success && (FSynchronizeController != NULL))
  {
    if (Operation == foCopy)
    {
      assert(Side == osLocal);
      FSynchronizeController->LogOperation(soUpload, FileName);
    }
    else if (Operation == foDelete)
    {
      assert(Side == osRemote);
      FSynchronizeController->LogOperation(soDelete, FileName);
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ShowOperationProgress(
  TFileOperationProgressType & ProgressData, bool First)
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  if (Ticks - LastTicks > 500 || First)
  {
    LastTicks = Ticks;

    static const int ProgressWidth = 48;
    static const int Captions[] = {PROGRESS_COPY, PROGRESS_MOVE, PROGRESS_DELETE,
      PROGRESS_SETPROPERTIES, 0, 0, PROGRESS_CALCULATE_SIZE,
      PROGRESS_REMOTE_MOVE, PROGRESS_REMOTE_COPY, PROGRESS_GETPROPERTIES,
      PROGRESS_CALCULATE_CHECKSUM };
    static wstring ProgressFileLabel;
    static wstring TargetDirLabel;
    static wstring StartTimeLabel;
    static wstring TimeElapsedLabel;
    static wstring BytesTransferedLabel;
    static wstring CPSLabel;
    static wstring TimeLeftLabel;

    if (ProgressFileLabel.empty())
    {
      ProgressFileLabel = GetMsg(PROGRESS_FILE_LABEL);
      TargetDirLabel = GetMsg(TARGET_DIR_LABEL);
      StartTimeLabel = GetMsg(START_TIME_LABEL);
      TimeElapsedLabel = GetMsg(TIME_ELAPSED_LABEL);
      BytesTransferedLabel = GetMsg(BYTES_TRANSFERED_LABEL);
      CPSLabel = GetMsg(CPS_LABEL);
      TimeLeftLabel = GetMsg(TIME_LEFT_LABEL);
    }

    bool TransferOperation =
      ((ProgressData.Operation == foCopy) || (ProgressData.Operation == foMove));

    wstring Message1;
    wstring ProgressBar1;
    wstring Message2;
    wstring ProgressBar2;
    wstring Title = GetMsg(Captions[(int)ProgressData.Operation - 1]);
    wstring FileName = ProgressData.FileName;
    // for upload from temporary directory,
    // do not show source directory
    if (TransferOperation && (ProgressData.Side == osLocal) && ProgressData.Temp)
    {
      FileName = ExtractFileName(FileName);
    }
    Message1 = ProgressFileLabel + MinimizeName(FileName,
      ProgressWidth - ProgressFileLabel.Length(), ProgressData.Side == osRemote) + "\n";
    // for downloads to temporary directory,
    // do not show target directory
    if (TransferOperation && !((ProgressData.Side == osRemote) && ProgressData.Temp))
    {
      Message1 += TargetDirLabel + MinimizeName(ProgressData.Directory,
        ProgressWidth - TargetDirLabel.Length(), ProgressData.Side == osLocal) + "\n";
    }
    ProgressBar1 = ProgressBar(ProgressData.OverallProgress(), ProgressWidth) + "\n";
    if (TransferOperation)
    {
      Message2 = L"\1\n";
      wstring StatusLine;
      wstring Value;

      Value = FormatDateTimeSpan(Configuration->TimeFormat, ProgressData.TimeElapsed());
      StatusLine = TimeElapsedLabel +
        wstring::StringOfChar(' ', ProgressWidth / 2 - 1 - TimeElapsedLabel.Length() - Value.Length()) +
        Value + "  ";

      wstring LabelText;
      if (ProgressData.TotalSizeSet)
      {
        Value = FormatDateTimeSpan(Configuration->TimeFormat, ProgressData.TotalTimeLeft());
        LabelText = TimeLeftLabel;
      }
      else
      {
        Value = ProgressData.StartTime.TimeString();
        LabelText = StartTimeLabel;
      }
      StatusLine = StatusLine + LabelText +
        wstring::StringOfChar(' ', ProgressWidth - StatusLine.Length() -
        LabelText.Length() - Value.Length()) +
        Value;
      Message2 += StatusLine + "\n";

      Value = FormatBytes(ProgressData.TotalTransfered);
      StatusLine = BytesTransferedLabel +
        wstring::StringOfChar(' ', ProgressWidth / 2 - 1 - BytesTransferedLabel.Length() - Value.Length()) +
        Value + "  ";
      Value = FORMAT("%s/s", (FormatBytes(ProgressData.CPS())));
      StatusLine = StatusLine + CPSLabel +
        wstring::StringOfChar(' ', ProgressWidth - StatusLine.Length() -
        CPSLabel.Length() - Value.Length()) +
        Value;
      Message2 += StatusLine + "\n";
      ProgressBar2 += ProgressBar(ProgressData.TransferProgress(), ProgressWidth) + "\n";
    }
    wstring Message =
      StrToFar(Message1) + ProgressBar1 + StrToFar(Message2) + ProgressBar2;
    FPlugin->Message(0, Title, Message, NULL, NULL, true);

    if (First)
    {
      FPlugin->ShowConsoleTitle(Title);
    }
    FPlugin->UpdateConsoleTitleProgress((short)ProgressData.OverallProgress());

    if (FPlugin->CheckForEsc())
    {
      CancelConfiguration(ProgressData);
    }
  }
}
//---------------------------------------------------------------------------
wstring TWinSCPFileSystem::ProgressBar(int Percentage, int Width)
{
  wstring Result;
  // OEM character set (Ansi does not have the ascii art we need)
  Result = wstring::StringOfChar('\xDB', (Width - 5) * Percentage / 100);
  Result += wstring::StringOfChar('\xB0', (Width - 5) - Result.Length());
  Result += FORMAT("%4d%%", (Percentage));
  return Result;
}
//---------------------------------------------------------------------------
TTerminalQueueStatus * TWinSCPFileSystem::ProcessQueue(bool Hidden)
{
  TTerminalQueueStatus * Result = NULL;
  assert(FQueueStatus != NULL);

  if (FQueueStatusInvalidated || FQueueItemInvalidated)
  {
    if (FQueueStatusInvalidated)
    {
      TGuard Guard(FQueueStatusSection);

      FQueueStatusInvalidated = false;

      assert(FQueue != NULL);
      FQueueStatus = FQueue->CreateStatus(FQueueStatus);
      Result = FQueueStatus;
    }

    FQueueItemInvalidated = false;

    TQueueItemProxy * QueueItem;
    for (int Index = 0; Index < FQueueStatus->ActiveCount; Index++)
    {
      QueueItem = FQueueStatus->Items[Index];
      if ((bool)QueueItem->UserData)
      {
        QueueItem->Update();
        Result = FQueueStatus;
      }

      if (GUIConfiguration->QueueAutoPopup &&
          TQueueItem::IsUserActionStatus(QueueItem->Status))
      {
        QueueItem->ProcessUserAction();
      }
    }
  }

  if (FRefreshRemoteDirectory)
  {
    if ((Terminal != NULL) && Terminal->GetActive())
    {
      Terminal->RefreshDirectory();
      if (UpdatePanel())
      {
        RedrawPanel();
      }
    }
    FRefreshRemoteDirectory = false;
  }
  if (FRefreshLocalDirectory)
  {
    if (GetOppositeFileSystem() == NULL)
    {
      if (UpdatePanel(false, true))
      {
        RedrawPanel(true);
      }
    }
    FRefreshLocalDirectory = false;
  }

  if (FQueueEventPending)
  {
    TQueueEvent Event;

    {
      TGuard Guard(FQueueStatusSection);
      Event = FQueueEvent;
      FQueueEventPending = false;
    }

    switch (Event)
    {
      case qeEmpty:
        if (Hidden && FarConfiguration->QueueBeep)
        {
          MessageBeep(MB_OK);
        }
        break;

      case qePendingUserAction:
        if (Hidden && !GUIConfiguration->QueueAutoPopup && FarConfiguration->QueueBeep)
        {
          // MB_ICONQUESTION would be more appropriate, but in default Windows Sound
          // schema it has no sound associated
          MessageBeep(MB_OK);
        }
        break;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::QueueListUpdate(TTerminalQueue * Queue)
{
  if (FQueue == Queue)
  {
    FQueueStatusInvalidated = true;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::QueueItemUpdate(TTerminalQueue * Queue,
  TQueueItem * Item)
{
  if (FQueue == Queue)
  {
    TGuard Guard(FQueueStatusSection);

    assert(FQueueStatus != NULL);

    TQueueItemProxy * QueueItem = FQueueStatus->FindByQueueItem(Item);

    if ((Item->Status == TQueueItem::qsDone) && (Terminal != NULL))
    {
      FRefreshLocalDirectory = (QueueItem == NULL) ||
        (!QueueItem->Info->ModifiedLocal.empty());
      FRefreshRemoteDirectory = (QueueItem == NULL) ||
        (!QueueItem->Info->ModifiedRemote.empty());
    }

    if (QueueItem != NULL)
    {
      QueueItem->UserData = (void*)true;
      FQueueItemInvalidated = true;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::QueueEvent(TTerminalQueue * Queue,
  TQueueEvent Event)
{
  TGuard Guard(FQueueStatusSection);
  if (Queue == FQueue)
  {
    FQueueEventPending = true;
    FQueueEvent = Event;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::CancelConfiguration(TFileOperationProgressType & ProgressData)
{
  if (!ProgressData.Suspended)
  {
    ProgressData.Suspend();
    try
    {
      TCancelStatus ACancel;
      int Result;
      if (ProgressData.TransferingFile &&
          (ProgressData.TimeExpected() > GUIConfiguration->IgnoreCancelBeforeFinish))
      {
        Result = MoreMessageDialog(GetMsg(CANCEL_OPERATION_FATAL), NULL,
          qtWarning, qaYes | qaNo | qaCancel);
      }
      else
      {
        Result = MoreMessageDialog(GetMsg(CANCEL_OPERATION), NULL,
          qtConfirmation, qaOK | qaCancel);
      }
      switch (Result) {
        case qaYes:
          ACancel = csCancelTransfer; break;
        case qaOK:
        case qaNo:
          ACancel = csCancel; break;
        default:
          ACancel = csContinue; break;
      }

      if (ACancel > ProgressData.Cancel)
      {
        ProgressData.Cancel = ACancel;
      }
    }
    catch(...)
    {
      ProgressData.Resume();
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::UploadFromEditor(bool NoReload, wstring FileName,
  wstring DestPath)
{
  assert(FFileList == NULL);
  FFileList = new TStringList();
  assert(FTerminal->AutoReadDirectory);
  bool PrevAutoReadDirectory = FTerminal->AutoReadDirectory;
  if (NoReload)
  {
    FTerminal->AutoReadDirectory = false;
    if (UnixComparePaths(DestPath, FTerminal->GetCurrentDirectory()))
    {
      FReloadDirectory = true;
    }
  }

  try
  {
    FFileList->Add(FileName);
    UploadFiles(false, 0, true, DestPath);
  }
  catch(...)
  {
    FTerminal->AutoReadDirectory = PrevAutoReadDirectory;
    SAFE_DESTROY(FFileList);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::UploadOnSave(bool NoReload)
{
  TFarEditorInfo * Info = FPlugin->EditorInfo();
  if (Info != NULL)
  {
    try
    {
      bool NativeEdit =
        (FLastEditorID >= 0) &&
        (FLastEditorID == Info->EditorID) &&
        !FLastEditFile.empty();

      TMultipleEdits::iterator I = FMultipleEdits.find(Info->EditorID);
      bool MultipleEdit = (I != FMultipleEdits.end());

      if (NativeEdit || MultipleEdit)
      {
        // make sure this is reset before any dialog is shown as it may cause recursion
        FEditorPendingSave = false;

        if (NativeEdit)
        {
          assert(FLastEditFile == Info->GetFileName());
          // always upload under the most recent name
          UploadFromEditor(NoReload, FLastEditFile, FTerminal->GetCurrentDirectory());
        }

        if (MultipleEdit)
        {
          UploadFromEditor(NoReload, Info->GetFileName(), I->second.Directory);
          // note that panel gets not refreshed upon switch to
          // panel view. but that's intentional
        }
      }
    }
    catch(...)
    {
      delete Info;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ProcessEditorEvent(int Event, void * /*Param*/)
{
  // EE_REDRAW is the first for optimalisation
  if (Event == EE_REDRAW)
  {
    if (FEditorPendingSave)
    {
      UploadOnSave(true);
    }

    // Whenever editor title is changed (and restored back), it is restored
    // to default FAR text, not to ours (see EE_SAVE). Hence we periodically
    // reset the title.
    static unsigned long LastTicks = 0;
    unsigned long Ticks = GetTickCount();
    if ((LastTicks == 0) || (Ticks - LastTicks > 500))
    {
      LastTicks = Ticks;
      TFarEditorInfo * Info = FPlugin->EditorInfo();
      if (Info != NULL)
      {
        try
        {
          TMultipleEdits::iterator I = FMultipleEdits.find(Info->EditorID);
          if (I != FMultipleEdits.end())
          {
            wstring FullFileName = UnixIncludeTrailingBackslash(I->second.Directory) +
              I->second.FileName;
            FPlugin->FarEditorControl(ECTL_SETTITLE, FullFileName.c_str());
          }
        }
        catch(...)
        {
          delete Info;
        }
      }
    }
  }
  else if (Event == EE_READ)
  {
    // file can be read from active filesystem only anyway. this prevents
    // both filesystems in both panels intercepting the very same file in case the
    // file with the same name is read from both filesystems recently
    if (IsActiveFileSystem())
    {
      TFarEditorInfo * Info = FPlugin->EditorInfo();
      if (Info != NULL)
      {
        try
        {
          if (!FLastEditFile.empty() &&
              AnsiSameText(FLastEditFile, Info->GetFileName()))
          {
            FLastEditorID = Info->EditorID;
            FEditorPendingSave = false;
          }

          if (!FLastMultipleEditFile.empty())
          {
            bool IsLastMultipleEditFile = AnsiSameText(FLastMultipleEditFile, Info->GetFileName());
            assert(IsLastMultipleEditFile);
            if (IsLastMultipleEditFile)
            {
              FLastMultipleEditFile = L"";

              TMultipleEdit MultipleEdit;
              MultipleEdit.FileName = ExtractFileName(Info->GetFileName());
              MultipleEdit.Directory = FLastMultipleEditDirectory;
              MultipleEdit.LocalFileName = Info->GetFileName();
              MultipleEdit.PendingSave = false;
              FMultipleEdits[Info->EditorID] = MultipleEdit;
              if (FLastMultipleEditReadOnly)
              {
                EditorSetParameter Parameter;
                memset(&Parameter, 0, sizeof(Parameter));
                Parameter.Type = ESPT_LOCKMODE;
                Parameter.Param.iParam = TRUE;
                FPlugin->FarEditorControl(ECTL_SETPARAM, &Parameter);
              }
            }
          }
        }
        catch(...)
        {
          delete Info;
        }
      }
    }
  }
  else if (Event == EE_CLOSE)
  {
    if (FEditorPendingSave)
    {
      assert(false); // should not happen, but let's be robust
      UploadOnSave(false);
    }

    TFarEditorInfo * Info = FPlugin->EditorInfo();
    if (Info != NULL)
    {
      try
      {
        if (FLastEditorID == Info->EditorID)
        {
          FLastEditorID = -1;
        }

        TMultipleEdits::iterator I = FMultipleEdits.find(Info->EditorID);
        if (I != FMultipleEdits.end())
        {
          if (I->second.PendingSave)
          {
            UploadFromEditor(true, Info->GetFileName(), I->second.Directory);
            // reload panel content (if uploaded to current directory.
            // no need for RefreshPanel as panel is not visible yet.
            UpdatePanel();
          }

          if (DeleteFile(Info->GetFileName()))
          {
            // remove directory only if it is empty
            // (to avoid deleting another directory if user uses "save as")
            RemoveDir(ExcludeTrailingBackslash(ExtractFilePath(Info->GetFileName())));
          }

          FMultipleEdits.erase(I);
        }
      }
      catch(...)
      {
        delete Info;
      }
    }
  }
  else if (Event == EE_SAVE)
  {
    TFarEditorInfo * Info = FPlugin->EditorInfo();
    if (Info != NULL)
    {
      try
      {
        if ((FLastEditorID >= 0) && (FLastEditorID == Info->EditorID))
        {
          // if the file is saved under different name ("save as"), we upload
          // the file back under that name
          FLastEditFile = Info->GetFileName();

          if (FarConfiguration->EditorUploadOnSave)
          {
            FEditorPendingSave = true;
          }
        }

        TMultipleEdits::iterator I = FMultipleEdits.find(Info->EditorID);
        if (I != FMultipleEdits.end())
        {
          if (I->second.LocalFileName != Info->GetFileName())
          {
            // update file name (after "save as")
            I->second.LocalFileName = Info->GetFileName();
            I->second.FileName = ExtractFileName(Info->GetFileName());
            // update editor title
            wstring FullFileName = UnixIncludeTrailingBackslash(I->second.Directory) +
              I->second.FileName;
            // note that we need to reset the title periodically (see EE_REDRAW)
            FPlugin->FarEditorControl(ECTL_SETTITLE, FullFileName.c_str());
          }

          if (FarConfiguration->EditorUploadOnSave)
          {
            FEditorPendingSave = true;
          }
          else
          {
            I->second.PendingSave = true;
          }
        }
      }
      catch(...)
      {
        delete Info;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::EditViewCopyParam(TCopyParamType & CopyParam)
{
  CopyParam.FileNameCase = ncNoChange;
  CopyParam.PreserveReadOnly = false;
  CopyParam.ResumeSupport = rsOff;
  // we have no way to give FAR back the modified filename, so make sure we
  // fail downloading file not valid on windows
  CopyParam.ReplaceInvalidChars = false;
  CopyParam.FileMask = L"";
  CopyParam.ExcludeFileMask = TFileMasks();
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::MultipleEdit()
{
  if ((PanelInfo->FocusedItem != NULL) &&
      PanelInfo->FocusedItem->IsFile &&
      (PanelInfo->FocusedItem->UserData != NULL))
  {
    TStrings * FileList = CreateFocusedFileList(osRemote);
    assert((FileList == NULL) || (FileList->Count == 1));

    if (FileList != NULL)
    {
      try
      {
        if (FileList->Count == 1)
        {
          MultipleEdit(FTerminal->GetCurrentDirectory(), FileList->Strings[0],
            (TRemoteFile*)FileList->Objects[0]);
        }
      }
      catch(...)
      {
        delete FileList;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::MultipleEdit(wstring Directory,
  wstring FileName, TRemoteFile * File)
{
  TEditHistory EditHistory;
  EditHistory.Directory = Directory;
  EditHistory.FileName = FileName;

  TEditHistories::iterator ih = std::find(FEditHistories.begin(), FEditHistories.end(), EditHistory);
  if (ih != FEditHistories.end())
  {
    FEditHistories.erase(ih);
  }
  FEditHistories.push_back(EditHistory);

  wstring FullFileName = UnixIncludeTrailingBackslash(Directory) + FileName;

  TMultipleEdits::iterator i = FMultipleEdits.begin();
  while (i != FMultipleEdits.end())
  {
    if (UnixComparePaths(Directory, i->second.Directory) &&
        (FileName == i->second.FileName))
    {
      break;
    }
    ++i;
  }

  FLastMultipleEditReadOnly = false;
  bool Edit = true;
  if (i != FMultipleEdits.end())
  {
    TMessageParams Params;
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaYes;
    Aliases[0].Alias = GetMsg(EDITOR_CURRENT);
    Aliases[1].Button = qaNo;
    Aliases[1].Alias = GetMsg(EDITOR_NEW_INSTANCE);
    Aliases[2].Button = qaOK;
    Aliases[2].Alias = GetMsg(EDITOR_NEW_INSTANCE_RO);
    Params.Aliases = Aliases;
    Params.AliasesCount = LENOF(Aliases);
    switch (MoreMessageDialog(FORMAT(GetMsg(EDITOR_ALREADY_LOADED), (FullFileName)),
          NULL, qtConfirmation, qaYes | qaNo | qaOK | qaCancel, &Params))
    {
      case qaYes:
        Edit = false;
        break;

      case qaNo:
        // noop
        break;

      case qaOK:
        FLastMultipleEditReadOnly = true;
        break;

      case qaCancel:
      default:
        Abort();
        break;
    }
  }

  if (Edit)
  {
    wstring TempDir;
    TCopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
    EditViewCopyParam(CopyParam);

    TStrings * FileList = new TStringList;
    assert(!FNoProgressFinish);
    FNoProgressFinish = true;
    try
    {
      FileList->AddObject(FullFileName, File);
      TemporarilyDownloadFiles(FileList, CopyParam, TempDir);
    }
    catch(...)
    {
      FNoProgressFinish = false;
      delete FileList;
    }

    FLastMultipleEditFile = IncludeTrailingBackslash(TempDir) + FileName;
    FLastMultipleEditDirectory = Directory;

    if (FarPlugin->Editor(FLastMultipleEditFile,
           EF_NONMODAL | VF_IMMEDIATERETURN | VF_DISABLEHISTORY, FullFileName))
    {
      assert(FLastMultipleEditFile == L"");
    }
    FLastMultipleEditFile = L"";
  }
  else
  {
    assert(i != FMultipleEdits.end());

    int WindowCount = FarPlugin->FarAdvControl(ACTL_GETWINDOWCOUNT);
    WindowInfo Window;
    Window.Pos = 0;
    while (Window.Pos < WindowCount)
    {
      if (FarPlugin->FarAdvControl(ACTL_GETWINDOWINFO, &Window) != 0)
      {
        if ((Window.Type == WTYPE_EDITOR) &&
            AnsiSameText(Window.Name, i->second.LocalFileName))
        {
          FarPlugin->FarAdvControl(ACTL_SETCURRENTWINDOW, Window.Pos);
          break;
        }
      }
      Window.Pos++;
    }

    assert(Window.Pos < WindowCount);
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::IsEditHistoryEmpty()
{
  return FEditHistories.empty();
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::EditHistory()
{
  TFarMenuItems * MenuItems = new TFarMenuItems();
  try
  {
    TEditHistories::const_iterator i = FEditHistories.begin();
    while (i != FEditHistories.end())
    {
      MenuItems->Add(MinimizeName(UnixIncludeTrailingBackslash((*i).Directory) + (*i).FileName,
        FPlugin->MaxMenuItemLength(), true));
      i++;
    }

    MenuItems->Add("");
    MenuItems->ItemFocused = MenuItems->Count - 1;

    const int BreakKeys[] = { VK_F4, 0 };

    int BreakCode;
    int Result = FPlugin->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
      GetMsg(MENU_EDIT_HISTORY), "", MenuItems, BreakKeys, BreakCode);

    if ((Result >= 0) && (Result < int(FEditHistories.size())))
    {
      TRemoteFile * File;
      wstring FullFileName =
        UnixIncludeTrailingBackslash(FEditHistories[Result].Directory) + FEditHistories[Result].FileName;
      FTerminal->ReadFile(FullFileName, File);
      try
      {
        if (!File->HaveFullFileName)
        {
          File->FullFileName = FullFileName;
        }
        MultipleEdit(FEditHistories[Result].Directory,
          FEditHistories[Result].FileName, File);
      }
      catch(...)
      {
        delete File;
      }
    }
  }
  catch(...)
  {
    delete MenuItems;
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::IsLogging()
{
  return
    Connected() && FTerminal->Log->LoggingToFile;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ShowLog()
{
  assert(Connected() && FTerminal->Log->LoggingToFile);
  FPlugin->Viewer(FTerminal->Log->CurrentFileName, VF_NONMODAL);
}
