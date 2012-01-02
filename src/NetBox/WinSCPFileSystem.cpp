//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "FarUtil.h"
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
#include "ScpFileSystem.h"
#include <Bookmarks.h>
#include <GUITools.h>
// FAR WORKAROUND
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
  {
    BOOST_SCOPE_EXIT ( (&ColumnTitles) )
    {
      delete ColumnTitles;
    } BOOST_SCOPE_EXIT_END
    ColumnTitles->Add(FarPlugin->GetMsg(SESSION_NAME_COL_TITLE)); 
    for (int Index = 0; Index < PANEL_MODES_COUNT; Index++)
    {
      PanelModes->SetPanelMode(Index, L"N", L"0", ColumnTitles, false, false, false);
    }
  }
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
  unsigned long & /*Flags*/, std::wstring & FileName, __int64 & /*Size*/,
  unsigned long & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
  std::wstring & /*Owner*/, void *& UserData, int & /*CustomColumnNumber*/)
{
  FileName = UnixExtractFileName(FSessionData->GetName());
  UserData = FSessionData;
}
//---------------------------------------------------------------------------
TSessionFolderPanelItem::TSessionFolderPanelItem(std::wstring Folder):
  TCustomFarPanelItem(),
  FFolder(Folder)
{
}
//---------------------------------------------------------------------------
void TSessionFolderPanelItem::GetData(
  unsigned long & /*Flags*/, std::wstring & FileName, __int64 & /*Size*/,
  unsigned long & FileAttributes,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
  std::wstring & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
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
  unsigned long & /*Flags*/, std::wstring & FileName, __int64 & Size,
  unsigned long & FileAttributes,
  TDateTime & LastWriteTime, TDateTime & LastAccess,
  unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
  std::wstring & Owner, void *& UserData, int & CustomColumnNumber)
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
std::wstring TRemoteFilePanelItem::GetCustomColumnData(int Column)
{
  switch (Column) {
    case 0: return FRemoteFile->GetGroup().GetName();
    case 1: return FRemoteFile->GetRightsStr();
    case 2: return FRemoteFile->GetRights()->GetOctal();
    case 3: return FRemoteFile->GetLinkTo();
    default: assert(false); return std::wstring();
  }
}
//---------------------------------------------------------------------------
void TRemoteFilePanelItem::TranslateColumnTypes(std::wstring & ColumnTypes,
  TStrings * ColumnTitles)
{
  std::wstring AColumnTypes = ColumnTypes;
  ColumnTypes = L"";
  std::wstring Column;
  std::wstring Title;
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
  {
    BOOST_SCOPE_EXIT ( (&ColumnTitles) )
    {
      delete ColumnTitles;
    } BOOST_SCOPE_EXIT_END
    if (FarConfiguration->GetCustomPanelModeDetailed())
    {
      std::wstring ColumnTypes = FarConfiguration->GetColumnTypesDetailed();
      std::wstring StatusColumnTypes = FarConfiguration->GetStatusColumnTypesDetailed();

      TranslateColumnTypes(ColumnTypes, ColumnTitles);
      TranslateColumnTypes(StatusColumnTypes, NULL);

      PanelModes->SetPanelMode(5 /*detailed */,
        ColumnTypes, FarConfiguration->GetColumnWidthsDetailed(),
        ColumnTitles, FarConfiguration->GetFullScreenDetailed(), false, true, false,
        StatusColumnTypes, FarConfiguration->GetStatusColumnWidthsDetailed());
    }
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
  virtual void Prompt(int Index, const std::wstring & Prompt,
    std::wstring & Value);

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
  const std::wstring &Prompt, std::wstring & Value)
{
  std::wstring APrompt = Prompt;
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
class TKeepaliveThread : public TSimpleThread
{
public:
  TKeepaliveThread(TWinSCPFileSystem * FileSystem, TDateTime Interval);
  virtual ~TKeepaliveThread()
  {}
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
  TSimpleThread()
{
  FEvent = CreateEvent(NULL, false, false, NULL);

  FFileSystem = FileSystem;
  FInterval = Interval;
  Start();
}
//---------------------------------------------------------------------------
void TKeepaliveThread::Terminate()
{
  SetEvent(FEvent);
}
//---------------------------------------------------------------------------
void TKeepaliveThread::Execute()
{
  while (!IsFinished())
  {
    static long MillisecondsPerDay = 24 * 60 * 60 * 1000;
    if ((WaitForSingleObject(FEvent, (DWORD)(double(FInterval) * MillisecondsPerDay)) != WAIT_FAILED) &&
        !IsFinished())
    {
      FFileSystem->KeepaliveThreadCallback();
    }
  }
  CloseHandle(FEvent);
}
//---------------------------------------------------------------------------
TWinSCPFileSystem::TWinSCPFileSystem(TCustomFarPlugin * APlugin) :
  TCustomFarFileSystem(APlugin)
{
  Self = this;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::Init(TSecureShell * SecureShell)
{
  TCustomFarFileSystem::Init();
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

  FLastMultipleEditReadOnly = false;
  FEditorPendingSave = false;
  FOutputLog = false;
}

//---------------------------------------------------------------------------
TWinSCPFileSystem::~TWinSCPFileSystem()
{
  // DEBUG_PRINTF(L"FTerminal = %x", FTerminal);
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
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::HandleException(const std::exception *E, int OpMode)
{
  // ::Error(SNotImplemented, 120);
  if ((GetTerminal() != NULL) && ::InheritsFrom<std::exception, EFatal>(E))
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
  {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->TCustomFarFileSystem::Close();
      } BOOST_SCOPE_EXIT_END
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
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::GetOpenPluginInfoEx(long unsigned & Flags,
  std::wstring & /*HostFile*/, std::wstring & CurDir, std::wstring & Format,
  std::wstring & PanelTitle, TFarPanelModes * PanelModes, int & /*StartPanelMode*/,
  int & /*StartSortMode*/, bool & /*StartSortOrder*/, TFarKeyBarTitles * KeyBarTitles,
  std::wstring & ShortcutData)
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
      PanelTitle = FORMAT(L" %s:%s ", Format.c_str(), CurDir.c_str());
    }
    else
    {
      PanelTitle = FORMAT(L" %s ", CurDir.c_str());
    }
    ShortcutData = FORMAT(L"%s\1%s", FTerminal->GetSessionData()->GetSessionUrl().c_str(), CurDir.c_str());

    TRemoteFilePanelItem::SetPanelModes(PanelModes);
    TRemoteFilePanelItem::SetKeyBarTitles(KeyBarTitles);
  }
  else
  {
    CurDir = FSessionsFolder;
    Format = L"winscp";
    Flags = OPIF_USESORTGROUPS | OPIF_USEHIGHLIGHTING | OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE;
    PanelTitle = FORMAT(L" %s ", GetMsg(NB_STORED_SESSION_TITLE).c_str());

    TSessionPanelItem::SetPanelModes(PanelModes);
    TSessionPanelItem::SetKeyBarTitles(KeyBarTitles);
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::GetFindDataEx(TObjectList * PanelItems, int OpMode)
{
  bool Result;
  // DEBUG_PRINTF(L"begin: Connected = %d", Connected());
  if (Connected())
  {
    assert(!FNoProgress);
    // OPM_FIND is used also for calculation of directory size (F3, quick view).
    // However directory is usually read from SetDirectory, so FNoProgress
    // seems to have no effect here.
    // Do not know if OPM_SILENT is even used.
    FNoProgress = FLAGSET(OpMode, OPM_FIND) || FLAGSET(OpMode, OPM_SILENT);
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FNoProgress = false;
        } BOOST_SCOPE_EXIT_END
      if (FReloadDirectory && FTerminal->GetActive())
      {
        FReloadDirectory = false;
        FTerminal->ReloadDirectory();
      }

      TRemoteFile * File;
      for (size_t Index = 0; Index < FTerminal->GetFiles()->GetCount(); Index++)
      {
        File = FTerminal->GetFiles()->GetFile(Index);
        PanelItems->Add((TObject *)new TRemoteFilePanelItem(File));
      }
    }
    Result = true;
  }
  else if (SessionList())
  {
    Result = true;
    assert(StoredSessions);
    StoredSessions->Load();
    std::wstring Folder = UnixIncludeTrailingBackslash(FSessionsFolder);
    TSessionData * Data;
    TStringList * ChildPaths = new TStringList();
    {
        BOOST_SCOPE_EXIT ( (&ChildPaths) )
        {
          delete ChildPaths;
        } BOOST_SCOPE_EXIT_END
      ChildPaths->SetCaseSensitive(false);
      // DEBUG_PRINTF(L"StoredSessions->GetCount = %d", StoredSessions->GetCount());
      for (size_t Index = 0; Index < StoredSessions->GetCount(); Index++)
      {
        Data = StoredSessions->GetSession(Index);
        if (Data->GetName().substr(0, Folder.size()) == Folder)
        {
          std::wstring Name = Data->GetName().substr(
            Folder.size(), Data->GetName().size() - Folder.size());
          int Slash = Name.find_first_of(L'/');
          if (Slash != std::wstring::npos)
          {
            Name.resize(Slash);
            if (ChildPaths->IndexOf(Name.c_str()) < 0)
            {
              PanelItems->Add((TObject *)new TSessionFolderPanelItem(Name));
              ChildPaths->Add(Name);
            }
          }
          else
          {
            PanelItems->Add((TObject *)new TSessionPanelItem(Data));
          }
        }
      }
    }
    if (!FNewSessionsFolder.empty())
    {
      PanelItems->Add((TObject *)new TSessionFolderPanelItem(FNewSessionsFolder));
    }
    if (PanelItems->GetCount() == 0)
    {
      PanelItems->Add((TObject *)new THintPanelItem(GetMsg(NEW_SESSION_HINT)));
    }
    TWinSCPFileSystem * OppositeFileSystem =
      dynamic_cast<TWinSCPFileSystem *>(GetOppositeFileSystem());
    if ((OppositeFileSystem != NULL) && !OppositeFileSystem->Connected() &&
        !OppositeFileSystem->FLoadingSessionList)
    {
      FLoadingSessionList = true;
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FLoadingSessionList = false;
        } BOOST_SCOPE_EXIT_END
        UpdatePanel(false, true);
        RedrawPanel(true);
      }
    }
  }
  else
  {
    Result = false;
  }
  // DEBUG_PRINTF(L"Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DuplicateRenameSession(TSessionData * Data,
  bool Duplicate)
{
  assert(Data);
  std::wstring Name = Data->GetName();
  if (FPlugin->InputBox(GetMsg(Duplicate ? DUPLICATE_SESSION_TITLE : RENAME_SESSION_TITLE),
        GetMsg(Duplicate ? DUPLICATE_SESSION_PROMPT : RENAME_SESSION_PROMPT),
        Name, NULL) &&
      !Name.empty() && (Name != Data->GetName()))
  {
    TNamedObject * EData = StoredSessions->FindByName(Name);
    if ((EData != NULL) && (EData != Data))
    {
      throw ExtException(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR).c_str(), Name.c_str()));
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
  // DEBUG_PRINTF(L"begin");
  TFarPanelItem * SessionItem = GetPanelInfo()->FindUserData(Data);
  // DEBUG_PRINTF(L"SessionItem = %x", SessionItem);
  if (SessionItem != NULL)
  {
    GetPanelInfo()->SetFocusedItem(SessionItem);
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::EditConnectSession(TSessionData * Data, bool Edit)
{
  // DEBUG_PRINTF(L"begin: Data = %x, Edit = %d", Data, Edit);
  TSessionData * OrigData = Data;
  bool NewData = !Data;
  bool FillInConnect = !Edit && !Data->GetCanLogin();
  if (NewData || FillInConnect)
  {
    Data = new TSessionData(L"");
  }

  {
      BOOST_SCOPE_EXIT ( (&Data) (&NewData) (&FillInConnect) )
      {
        if (NewData || FillInConnect)
        {
          delete Data;
        }
      } BOOST_SCOPE_EXIT_END
    if (FillInConnect)
    {
      Data->Assign(OrigData);
      Data->SetName(L"");
    }

    TSessionActionEnum Action;
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
            std::wstring Name =
              UnixIncludeTrailingBackslash(FSessionsFolder) + Data->GetSessionName();
            // DEBUG_PRINTF(L"Name = %s", Name.c_str());
            if (FPlugin->InputBox(GetMsg(NEW_SESSION_NAME_TITLE),
                  GetMsg(NEW_SESSION_NAME_PROMPT), Name, 0) &&
                !Name.empty())
            {
              if (StoredSessions->FindByName(Name))
              {
                throw ExtException(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR).c_str(), Name.c_str()));
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
            std::wstring OrigName = OrigData->GetName();
            OrigData->Assign(Data);
            OrigData->SetName(OrigName);
          }

          // modified only, explicit
          StoredSessions->Save(false, true);
          if (UpdatePanel())
          {
            if (SelectSession != NULL)
            {
              // DEBUG_PRINTF(L"SelectSession = %x", SelectSession);
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
        if (GetPanelInfo()->GetItemCount())
        {
          GetPanelInfo()->SetFocusedIndex(0);
        }
      }
    }
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ProcessEventEx(int Event, void * Param)
{
  bool Result = false;
  if (Connected())
  {
    if (Event == FE_COMMAND)
    {
      std::wstring Command = (wchar_t *)Param;
      if (!::Trim(Command).empty() &&
          (::LowerCase(Command.substr(0, 3)) != L"cd "))
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
  const std::wstring &AddedLine, bool /*StdError*/)
{
  if (FOutputLog)
  {
    FPlugin->WriteConsole(AddedLine + L"\n");
  }
  if (FCapturedLog != NULL)
  {
    FCapturedLog->Add(AddedLine);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::RequireLocalPanel(TFarPanelInfo * Panel, std::wstring Message)
{
  if (Panel->GetIsPlugin() || (Panel->GetType() != ptFile))
  {
    throw ExtException(Message);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::RequireCapability(int Capability)
{
  if (!FTerminal->GetIsCapable(static_cast<TFSCapability>(Capability)))
  {
    throw ExtException(FORMAT(GetMsg(OPERATION_NOT_SUPPORTED).c_str(),
      FTerminal->GetFileSystemInfo().ProtocolName.c_str()));
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::EnsureCommandSessionFallback(TFSCapability Capability)
{
  bool Result = FTerminal->GetIsCapable(Capability) ||
    FTerminal->GetCommandSessionOpened();

  if (!Result)
  {
    if (!GUIConfiguration->GetConfirmCommandSession())
    {
      Result = true;
    }
    else
    {
      TMessageParams Params;
      Params.Params = qpNeverAskAgainCheck;
      int Answer = MoreMessageDialog(FORMAT(GetMsg(PERFORM_ON_COMMAND_SESSION).c_str(),
        FTerminal->GetFileSystemInfo().ProtocolName.c_str(),
         FTerminal->GetFileSystemInfo().ProtocolName.c_str()), NULL,
        qtConfirmation, qaOK | qaCancel, &Params);
      if (Answer == qaNeverAskAgain)
      {
        GUIConfiguration->SetConfirmCommandSession(false);
        Result = true;
      }
      else
      {
        Result = (Answer == qaOK);
      }
    }

    if (Result)
    {
      ConnectTerminal(FTerminal->GetCommandSession());
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ExecuteCommand(const std::wstring &Command)
{
  if (FTerminal->AllowedAnyCommand(Command) &&
      EnsureCommandSessionFallback(fcAnyCommand))
  {
    FTerminal->BeginTransaction();
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          if (Self->FTerminal->GetActive())
          {
            Self->FTerminal->EndTransaction();
            Self->UpdatePanel();
          }
          else
          {
            Self->RedrawPanel();
            Self->RedrawPanel(true);
          }
        } BOOST_SCOPE_EXIT_END
      FarControl(FCTL_SETCMDLINE, 0, (LONG_PTR)L"");
      FPlugin->ShowConsoleTitle(Command);
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->FPlugin->ScrollTerminalScreen(1);
            Self->FPlugin->SaveTerminalScreen();
            Self->FPlugin->ClearConsoleTitle();
          } BOOST_SCOPE_EXIT_END
        FPlugin->ShowTerminalScreen();

        FOutputLog = true;
        captureoutput_slot_type OutputEvent = boost::bind(&TWinSCPFileSystem::TerminalCaptureLog, this, _1, _2);
        FTerminal->AnyCommand(Command, &OutputEvent);
      }
    }
  }
  return true;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ProcessKeyEx(int Key, unsigned int ControlState)
{
  bool Handled = false;
  // DEBUG_PRINTF(L"begin");

  TFarPanelItem * Focused = GetPanelInfo()->GetFocusedItem();

  if ((Key == 'W') && (ControlState & PKF_SHIFT) &&
        (ControlState & PKF_ALT))
  {
    WinSCPPlugin()->CommandsMenu(true);
    Handled = true;
  }
  else if (SessionList())
  {
    TSessionData * Data = NULL;

    // DEBUG_PRINTF(L"Focused = %x", Focused);
    if (Focused)
    {
        // DEBUG_PRINTF(L"Focused->GetIsFile = %d, Focused->GetUserData = %x", Focused->GetIsFile(), Focused->GetUserData());
    }
    if ((Focused != NULL) && Focused->GetIsFile() && Focused->GetUserData())
    {
      Data = (TSessionData *)Focused->GetUserData();
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

    // DEBUG_PRINTF(L"Key = %x, VK_F4 = %x, ControlState = %d", Key, VK_F4, ControlState);
    if (Key == VK_F4 && (ControlState == 0))
    {
      // DEBUG_PRINTF(L"Data = %x, StoredSessions->GetCount = %d", Data, StoredSessions->GetCount());
      if ((Data != NULL) || (StoredSessions->GetCount() == 0))
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
         FarConfiguration->GetEditorMultiple())
    {
      MultipleEdit();
      Handled = true;
    }
  }
  // DEBUG_PRINTF(L"end");
  return Handled;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::CreateLink()
{
  // DEBUG_PRINTF(L"begin");
  RequireCapability(fcResolveSymlink);
  RequireCapability(fcSymbolicLink);

  bool Edit = false;
  TRemoteFile * File = NULL;
  std::wstring FileName;
  std::wstring PointTo;
  bool SymbolicLink = true;

  if (GetPanelInfo()->GetFocusedItem() && GetPanelInfo()->GetFocusedItem()->GetUserData())
  {
    File = (TRemoteFile *)GetPanelInfo()->GetFocusedItem()->GetUserData();

    Edit = File->GetIsSymLink() && GetTerminal()->GetSessionData()->GetResolveSymlinks();
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
        GetTerminal()->GetIsCapable(fcHardLink)))
  {
    if (Edit)
    {
      assert(File->GetFileName() == FileName);
      int Params = dfNoRecursive;
      GetTerminal()->SetExceptionOnFail(true);
      {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->GetTerminal()->SetExceptionOnFail(false);
        } BOOST_SCOPE_EXIT_END
        GetTerminal()->DeleteFile(L"", File, &Params);
      }
    }
    GetTerminal()->CreateLink(FileName, PointTo, SymbolicLink);
    if (UpdatePanel())
    {
      RedrawPanel();
    }
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TemporarilyDownloadFiles(
  TStrings * FileList, TCopyParamType CopyParam, std::wstring & TempDir)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);

  TempDir = FPlugin->TemporaryDir();
  if (TempDir.empty() || !ForceDirectories(TempDir))
  {
    throw ExtException(FMTLOAD(CREATE_TEMP_DIR_ERROR, TempDir.c_str()));
  }

  FTerminal->SetExceptionOnFail(true);
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->FTerminal->SetExceptionOnFail(false);
    } BOOST_SCOPE_EXIT_END
    try
    {
      FTerminal->CopyToLocal(FileList, TempDir, &CopyParam, cpTemporary);
    }
    catch (...)
    {
      try
      {
        RecursiveDeleteFile(::ExcludeTrailingBackslash(TempDir), false);
      }
      catch (...)
      {
      }
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ApplyCommand()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  if (FileList != NULL)
  {
    {
        BOOST_SCOPE_EXIT ( (&FileList) )
        {
            delete FileList;
        } BOOST_SCOPE_EXIT_END
      int Params = FarConfiguration->GetApplyCommandParams();
      std::wstring Command = FarConfiguration->GetApplyCommandCommand();
      if (ApplyCommandDialog(Command, Params))
      {
        FarConfiguration->SetApplyCommandParams(Params);
        FarConfiguration->SetApplyCommandCommand(Command);
        if (FLAGCLEAR(Params, ccLocal))
        {
          if (EnsureCommandSessionFallback(fcShellAnyCommand))
          {
            TCustomCommandData Data(GetTerminal());
            TRemoteCustomCommand RemoteCustomCommand(Data, GetTerminal()->GetCurrentDirectory());
            TFarInteractiveCustomCommand InteractiveCustomCommand(
              FPlugin, &RemoteCustomCommand);

            Command = InteractiveCustomCommand.Complete(Command, false);

            {
              BOOST_SCOPE_EXIT ( (&Self) )
              {
                Self->GetPanelInfo()->ApplySelection();
                if (Self->UpdatePanel())
                {
                  Self->RedrawPanel();
                }
              } BOOST_SCOPE_EXIT_END
              captureoutput_signal_type OutputEvent;
              FOutputLog = false;
              if (FLAGSET(Params, ccShowResults))
              {
                assert(!FNoProgress);
                FNoProgress = true;
                FOutputLog = true;
                OutputEvent.connect(boost::bind(&TWinSCPFileSystem::TerminalCaptureLog, this, _1, _2));
              }

              if (FLAGSET(Params, ccCopyResults))
              {
                assert(FCapturedLog == NULL);
                FCapturedLog = new TStringList();
                OutputEvent.connect(boost::bind(&TWinSCPFileSystem::TerminalCaptureLog, this, _1, _2));
              }

              {
                BOOST_SCOPE_EXIT ( (&Self) (&Params) )
                {
                  if (FLAGSET(Params, ccShowResults))
                  {
                    Self->FNoProgress = false;
                    Self->FPlugin->ScrollTerminalScreen(1);
                    Self->FPlugin->SaveTerminalScreen();
                  }

                  if (FLAGSET(Params, ccCopyResults))
                  {
                    Self->FPlugin->FarCopyToClipboard(Self->FCapturedLog);
                    SAFE_DESTROY(Self->FCapturedLog);
                  }
                } BOOST_SCOPE_EXIT_END
                if (FLAGSET(Params, ccShowResults))
                {
                  FPlugin->ShowTerminalScreen();
                }

                FTerminal->CustomCommandOnFiles(Command, Params, FileList, OutputEvent);
              }
            }
          }
        }
        else
        {
          TCustomCommandData Data(GetTerminal());
          TLocalCustomCommand LocalCustomCommand(Data, GetTerminal()->GetCurrentDirectory());
          TFarInteractiveCustomCommand InteractiveCustomCommand(FPlugin,
            &LocalCustomCommand);

          Command = InteractiveCustomCommand.Complete(Command, false);

          TStrings * LocalFileList = NULL;
          TStrings * RemoteFileList = NULL;
          {
            BOOST_SCOPE_EXIT ( (&RemoteFileList) (&LocalFileList) )
            {
              delete RemoteFileList;
              delete LocalFileList;
            } BOOST_SCOPE_EXIT_END
            bool FileListCommand = LocalCustomCommand.IsFileListCommand(Command);
            bool LocalFileCommand = LocalCustomCommand.HasLocalFileName(Command);

            if (LocalFileCommand)
            {
              TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
              RequireLocalPanel(AnotherPanel, GetMsg(APPLY_COMMAND_LOCAL_PATH_REQUIRED));

              LocalFileList = CreateSelectedFileList(osLocal, AnotherPanel);

              if (FileListCommand)
              {
                if ((LocalFileList == NULL) || (LocalFileList->GetCount() != 1))
                {
                  throw ExtException(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH1));
                }
              }
              else
              {
                if ((LocalFileList == NULL) ||
                    ((LocalFileList->GetCount() != 1) &&
                     (FileList->GetCount() != 1) &&
                     (LocalFileList->GetCount() != FileList->GetCount())))
                {
                  throw ExtException(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH));
                }
              }
            }

            std::wstring TempDir;

            TemporarilyDownloadFiles(FileList, GUIConfiguration->GetDefaultCopyParam(), TempDir);

            {
                BOOST_SCOPE_EXIT ( (&TempDir) )
                {
                    RecursiveDeleteFile(ExcludeTrailingBackslash(TempDir), false);
                } BOOST_SCOPE_EXIT_END
              RemoteFileList = new TStringList();

              TMakeLocalFileListParams MakeFileListParam;
              MakeFileListParam.FileList = RemoteFileList;
              MakeFileListParam.IncludeDirs = FLAGSET(Params, ccApplyToDirectories);
              MakeFileListParam.Recursive =
                FLAGSET(Params, ccRecursive) && !FileListCommand;

              ProcessLocalDirectory(TempDir, boost::bind(&TTerminal::MakeLocalFileList, FTerminal, _1, _2, _3), &MakeFileListParam);

              TFileOperationProgressType Progress(boost::bind(&TWinSCPFileSystem::OperationProgress, this, _1, _2), boost::bind(&TWinSCPFileSystem::OperationFinished, this, _1, _2, _3, _4, _5, _6));

              Progress.Start(foCustomCommand, osRemote, FileListCommand ? 1 : FileList->GetCount());

              {
                  BOOST_SCOPE_EXIT ( (&Progress) )
                  {
                      Progress.Stop();
                  } BOOST_SCOPE_EXIT_END
                if (FileListCommand)
                {
                  std::wstring LocalFile;
                  std::wstring FileList = MakeFileList(RemoteFileList);

                  if (LocalFileCommand)
                  {
                    assert(LocalFileList->GetCount() == 1);
                    LocalFile = LocalFileList->GetString(0);
                  }

                  TCustomCommandData Data(FTerminal);
                  TLocalCustomCommand CustomCommand(Data,
                    GetTerminal()->GetCurrentDirectory(), L"", LocalFile, FileList);
                  ExecuteShellAndWait(FPlugin->GetHandle(), CustomCommand.Complete(Command, true),
                    processmessages_signal_type());
                }
                else if (LocalFileCommand)
                {
                  if (LocalFileList->GetCount() == 1)
                  {
                    std::wstring LocalFile = LocalFileList->GetString(0);

                    for (size_t Index = 0; Index < RemoteFileList->GetCount(); Index++)
                    {
                      std::wstring FileName = RemoteFileList->GetString(Index);
                      TCustomCommandData Data(FTerminal);
                      TLocalCustomCommand CustomCommand(Data,
                        GetTerminal()->GetCurrentDirectory(), FileName, LocalFile, L"");
                      ExecuteShellAndWait(FPlugin->GetHandle(),
                        CustomCommand.Complete(Command, true), processmessages_signal_type());
                    }
                  }
                  else if (RemoteFileList->GetCount() == 1)
                  {
                    std::wstring FileName = RemoteFileList->GetString(0);

                    for (size_t Index = 0; Index < LocalFileList->GetCount(); Index++)
                    {
                      TCustomCommandData Data(FTerminal);
                      TLocalCustomCommand CustomCommand(
                        Data, GetTerminal()->GetCurrentDirectory(),
                        FileName, LocalFileList->GetString(Index), L"");
                      ExecuteShellAndWait(FPlugin->GetHandle(),
                        CustomCommand.Complete(Command, true), processmessages_signal_type());
                    }
                  }
                  else
                  {
                    if (LocalFileList->GetCount() != RemoteFileList->GetCount())
                    {
                      throw ExtException(GetMsg(CUSTOM_COMMAND_PAIRS_DOWNLOAD_FAILED));
                    }

                    for (size_t Index = 0; Index < LocalFileList->GetCount(); Index++)
                    {
                      std::wstring FileName = RemoteFileList->GetString(Index);
                      TCustomCommandData Data(FTerminal);
                      TLocalCustomCommand CustomCommand(
                        Data, GetTerminal()->GetCurrentDirectory(),
                        FileName, LocalFileList->GetString(Index), L"");
                      ExecuteShellAndWait(FPlugin->GetHandle(),
                        CustomCommand.Complete(Command, true), processmessages_signal_type());
                    }
                  }
                }
                else
                {
                  for (size_t Index = 0; Index < RemoteFileList->GetCount(); Index++)
                  {
                    TCustomCommandData Data(FTerminal);
                    TLocalCustomCommand CustomCommand(Data,
                      GetTerminal()->GetCurrentDirectory(), RemoteFileList->GetString(Index), L"", L"");
                    ExecuteShellAndWait(FPlugin->GetHandle(),
                      CustomCommand.Complete(Command, true), processmessages_signal_type());
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::Synchronize(const std::wstring &LocalDirectory,
  const std::wstring &RemoteDirectory, TTerminal::TSynchronizeMode Mode,
  const TCopyParamType &CopyParam, int Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options)
{
  TSynchronizeChecklist * AChecklist = NULL;
  {
      BOOST_SCOPE_EXIT ( (&AChecklist) (&Checklist) )
      {
        if (Checklist == NULL)
        {
          delete AChecklist;
        }
        else
        {
          *Checklist = AChecklist;
        }
      } BOOST_SCOPE_EXIT_END
    FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
    FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = true;
    {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FPlugin->ClearConsoleTitle();
        Self->FPlugin->RestoreScreen(Self->FSynchronizationSaveScreenHandle);
      } BOOST_SCOPE_EXIT_END
      AChecklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
        Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
        boost::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this, _1, _2, _3, _4), Options);
    }

    FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
    FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = false;
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FPlugin->ClearConsoleTitle();
          Self->FPlugin->RestoreScreen(Self->FSynchronizationSaveScreenHandle);
        } BOOST_SCOPE_EXIT_END
        FTerminal->SynchronizeApply(AChecklist, LocalDirectory, RemoteDirectory,
            &CopyParam, Params | TTerminal::spNoConfirmation,
            boost::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this, _1, _2, _3, _4));
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeAllowSelectedOnly()
{
  return
    (GetPanelInfo()->GetSelectedCount() > 0) ||
    (GetAnotherPanelInfo()->GetSelectedCount() > 0);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::GetSynchronizeOptions(
  int Params, TSynchronizeOptions & Options)
{
  // DEBUG_PRINTF(L"begin");
  if (FLAGSET(Params, spSelectedOnly) && SynchronizeAllowSelectedOnly())
  {
    Options.Filter = new TStringList();
    Options.Filter->SetCaseSensitive(false);
    Options.Filter->SetDuplicates(dupAccept);

    if (GetPanelInfo()->GetSelectedCount() > 0)
    {
      CreateFileList(GetPanelInfo()->GetItems(), osRemote, true, L"", true, Options.Filter);
    }
    if (GetAnotherPanelInfo()->GetSelectedCount() > 0)
    {
      CreateFileList(GetAnotherPanelInfo()->GetItems(), osLocal, true, L"", true, Options.Filter);
    }
    Options.Filter->Sort();
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::FullSynchronize(bool Source)
{
  TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  std::wstring LocalDirectory = AnotherPanel->GetCurrentDirectory();
  std::wstring RemoteDirectory = FTerminal->GetCurrentDirectory();

  bool SaveMode = !(GUIConfiguration->GetSynchronizeModeAuto() < 0);
  TTerminal::TSynchronizeMode Mode =
    (SaveMode ? (TTerminal::TSynchronizeMode)GUIConfiguration->GetSynchronizeModeAuto() :
      (Source ? TTerminal::smLocal : TTerminal::smRemote));
  int Params = GUIConfiguration->GetSynchronizeParams();
  bool SaveSettings = false;

  TGUICopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();
  TUsableCopyParamAttrs CopyParamAttrs = GetTerminal()->UsableCopyParamAttrs(0);
  int Options =
    FLAGMASK(!FTerminal->GetIsCapable(fcTimestampChanging), fsoDisableTimestamp) |
    FLAGMASK(SynchronizeAllowSelectedOnly(), fsoAllowSelectedOnly);
  if (FullSynchronizeDialog(Mode, Params, LocalDirectory, RemoteDirectory,
        &CopyParam, SaveSettings, SaveMode, Options, CopyParamAttrs))
  {
    TSynchronizeOptions SynchronizeOptions;
    GetSynchronizeOptions(Params, SynchronizeOptions);

    if (SaveSettings)
    {
      GUIConfiguration->SetSynchronizeParams(Params);
      if (SaveMode)
      {
        GUIConfiguration->SetSynchronizeModeAuto(Mode);
      }
    }

    TSynchronizeChecklist * Checklist = NULL;
    {
        BOOST_SCOPE_EXIT ( (&Self) (&Checklist) )
        {
          delete Checklist;
          if (Self->UpdatePanel())
          {
            Self->RedrawPanel();
          }
        } BOOST_SCOPE_EXIT_END
      FPlugin->SaveScreen(FSynchronizationSaveScreenHandle);
      FPlugin->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
      FSynchronizationStart = Now();
      FSynchronizationCompare = true;
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->FPlugin->ClearConsoleTitle();
            Self->FPlugin->RestoreScreen(Self->FSynchronizationSaveScreenHandle);
          } BOOST_SCOPE_EXIT_END
        Checklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
          Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
          boost::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this, _1, _2, _3, _4), &SynchronizeOptions);
      }

      if (Checklist->GetCount() == 0)
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
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
              Self->FPlugin->ClearConsoleTitle();
              Self->FPlugin->RestoreScreen(Self->FSynchronizationSaveScreenHandle);
            } BOOST_SCOPE_EXIT_END
            FTerminal->SynchronizeApply(Checklist, LocalDirectory, RemoteDirectory,
                &CopyParam, Params | TTerminal::spNoConfirmation,
                boost::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this, _1, _2, _3, _4));
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalSynchronizeDirectory(
  const std::wstring &LocalDirectory, const std::wstring &RemoteDirectory,
  bool &Continue, bool Collect)
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    static const int ProgressWidth = 48;
    static std::wstring ProgressTitle;
    static std::wstring ProgressTitleCompare;
    static std::wstring LocalLabel;
    static std::wstring RemoteLabel;
    static std::wstring StartTimeLabel;
    static std::wstring TimeElapsedLabel;

    if (ProgressTitle.empty())
    {
      ProgressTitle = GetMsg(SYNCHRONIZE_PROGRESS_TITLE);
      ProgressTitleCompare = GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE);
      LocalLabel = GetMsg(SYNCHRONIZE_PROGRESS_LOCAL);
      RemoteLabel = GetMsg(SYNCHRONIZE_PROGRESS_REMOTE);
      StartTimeLabel = GetMsg(SYNCHRONIZE_PROGRESS_START_TIME);
      TimeElapsedLabel = GetMsg(SYNCHRONIZE_PROGRESS_ELAPSED);
    }

    std::wstring Message;

    Message = LocalLabel + MinimizeName(LocalDirectory,
      ProgressWidth - LocalLabel.size(), false);
    Message += ::StringOfChar(L' ', ProgressWidth - Message.size()) + L"\n";
    Message += RemoteLabel + MinimizeName(RemoteDirectory,
      ProgressWidth - RemoteLabel.size(), true) + L"\n";
    Message += StartTimeLabel + FSynchronizationStart.TimeString() + L"\n";
    Message += TimeElapsedLabel +
      FormatDateTimeSpan(Configuration->GetTimeFormat(), TDateTime(Now() - FSynchronizationStart)) + L"\n";

    FPlugin->Message(0, (Collect ? ProgressTitleCompare : ProgressTitle), Message);

    if (FPlugin->CheckForEsc() &&
        (MoreMessageDialog(GetMsg(CANCEL_OPERATION), NULL,
          qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      // DEBUG_PRINTF(L"after MoreMessageDialog");
      Continue = false;
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::Synchronize()
{
  TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  TSynchronizeParamType Params;
  Params.LocalDirectory = AnotherPanel->GetCurrentDirectory();
  Params.RemoteDirectory = FTerminal->GetCurrentDirectory();
  int UnusedParams = (GUIConfiguration->GetSynchronizeParams() &
    (TTerminal::spPreviewChanges | TTerminal::spTimestamp |
     TTerminal::spNotByTime | TTerminal::spBySize));
  Params.Params = GUIConfiguration->GetSynchronizeParams() & ~UnusedParams;
  Params.Options = GUIConfiguration->GetSynchronizeOptions();
  bool SaveSettings = false;
  TSynchronizeController Controller(
    boost::bind(&TWinSCPFileSystem::DoSynchronize, this, _1, _2, _3, _4, _5, _6, _7, _8),
    boost::bind(&TWinSCPFileSystem::DoSynchronizeInvalid, this, _1, _2, _3),
    boost::bind(&TWinSCPFileSystem::DoSynchronizeTooManyDirectories, this, _1, _2));
  assert(FSynchronizeController == NULL);
  FSynchronizeController = &Controller;

  {
      BOOST_SCOPE_EXIT ( (&Self) )
      {
        Self->FSynchronizeController = NULL;
        // plugin might have been closed during some synchronisation already
        if (!Self->FClosed)
        {
          if (Self->UpdatePanel())
          {
            Self->RedrawPanel();
          }
        }
      } BOOST_SCOPE_EXIT_END
    TCopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();
    int CopyParamAttrs = GetTerminal()->UsableCopyParamAttrs(0).Upload;
    int Options =
      FLAGMASK(SynchronizeAllowSelectedOnly(), soAllowSelectedOnly);
    if (SynchronizeDialog(Params, &CopyParam,
        boost::bind(&TSynchronizeController::StartStop, &Controller, _1, _2, _3, _4, _5, _6, _7, _8),
        SaveSettings, Options, CopyParamAttrs,
        boost::bind(&TWinSCPFileSystem::GetSynchronizeOptions, this, _1, _2)) &&
        SaveSettings)
    {
      GUIConfiguration->SetSynchronizeParams(Params.Params | UnusedParams);
      GUIConfiguration->SetSynchronizeOptions(Params.Options);
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronize(
  TSynchronizeController * /*Sender*/, const std::wstring &LocalDirectory,
  const std::wstring &RemoteDirectory, const TCopyParamType &CopyParam,
  const TSynchronizeParamType &Params, TSynchronizeChecklist **Checklist,
  TSynchronizeOptions *Options, bool Full)
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
  catch (const std::exception &E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    throw;
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, const std::wstring &Directory,
  const std::wstring &ErrorStr)
{
  std::wstring Message;
  if (!Directory.empty())
  {
    Message = FORMAT(GetMsg(WATCH_ERROR_DIRECTORY).c_str(), Directory.c_str());
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
  if (MaxDirectories < GUIConfiguration->GetMaxWatchDirectories())
  {
    MaxDirectories = GUIConfiguration->GetMaxWatchDirectories();
  }
  else
  {
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    int Result = MoreMessageDialog(
      FORMAT(GetMsg(TOO_MANY_WATCH_DIRECTORIES).c_str(), MaxDirectories, MaxDirectories), NULL,
      qtConfirmation, qaYes | qaNo, &Params);

    if ((Result == qaYes) || (Result == qaNeverAskAgain))
    {
      MaxDirectories *= 2;
      if (Result == qaNeverAskAgain)
      {
        GUIConfiguration->SetMaxWatchDirectories(MaxDirectories);
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
  const std::wstring &AName, std::wstring &Value)
{
  std::wstring Name = AName;
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
  // DEBUG_PRINTF(L"begin");
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

      {
          BOOST_SCOPE_EXIT ( (&FileList) )
          {
            delete FileList;
          } BOOST_SCOPE_EXIT_END
        std::wstring Target = FTerminal->GetCurrentDirectory();
        std::wstring FileMask = L"*.*";
        if (RemoteTransferDialog(FileList, Target, FileMask, Move))
        {
          {
              BOOST_SCOPE_EXIT ( (&Self) )
              {
                Self->GetPanelInfo()->ApplySelection();
                if (Self->UpdatePanel())
                {
                  Self->RedrawPanel();
                }
              } BOOST_SCOPE_EXIT_END
            if (Move)
            {
              GetTerminal()->MoveFiles(FileList, Target, FileMask);
            }
            else
            {
              GetTerminal()->CopyFiles(FileList, Target, FileMask);
            }
          }
        }
      }
    }
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::RenameFile()
{
  // DEBUG_PRINTF(L"begin");
  TFarPanelItem * PanelItem = GetPanelInfo()->GetFocusedItem();
  assert(PanelItem != NULL);

  if (!PanelItem->GetIsParentDirectory())
  {
    RequireCapability(fcRename);

    TRemoteFile *File = static_cast<TRemoteFile *>(PanelItem->GetUserData());
    std::wstring NewName = File->GetFileName();
    if (RenameFileDialog(File, NewName))
    {
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            if (Self->UpdatePanel())
            {
              Self->RedrawPanel();
            }
          } BOOST_SCOPE_EXIT_END
        GetTerminal()->RenameFile(File, NewName, true);
      }
    }
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::FileProperties()
{
  // DEBUG_PRINTF(L"begin");
  TStrings * FileList = CreateSelectedFileList(osRemote);
  if (FileList)
  {
    assert(!FPanelItems);

    {
        BOOST_SCOPE_EXIT ( (&FileList) )
        {
          delete FileList;
        } BOOST_SCOPE_EXIT_END
      TRemoteProperties CurrentProperties;

      bool Cont = true;
      if (!GetTerminal()->LoadFilesProperties(FileList))
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
        if (FTerminal->GetIsCapable(fcModeChanging)) Flags |= cpMode;
        if (FTerminal->GetIsCapable(fcOwnerChanging)) Flags |= cpOwner;
        if (FTerminal->GetIsCapable(fcGroupChanging)) Flags |= cpGroup;

        TRemoteProperties NewProperties = CurrentProperties;
        if (PropertiesDialog(FileList, FTerminal->GetCurrentDirectory(),
            FTerminal->GetGroups(), FTerminal->GetUsers(),
            // NULL, NULL,
            &NewProperties, Flags))
        {
          NewProperties = TRemoteProperties::ChangedProperties(CurrentProperties,
            NewProperties);
          {
              BOOST_SCOPE_EXIT ( (&Self) )
              {
                Self->GetPanelInfo()->ApplySelection();
                if (Self->UpdatePanel())
                {
                  Self->RedrawPanel();
                }
              } BOOST_SCOPE_EXIT_END
            FTerminal->ChangeFilesProperties(FileList, &NewProperties);
          }
        }
      }
    }
  }
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::InsertTokenOnCommandLine(std::wstring Token, bool Separate)
{
  if (!Token.empty())
  {
    if (Token.find_first_of(L" ") != std::wstring::npos)
    {
      Token = FORMAT(L"\"%s\"", Token.c_str());
    }

    if (Separate)
    {
      Token += L" ";
    }

    FarControl(FCTL_INSERTCMDLINE, 0, (LONG_PTR)Token.c_str());
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::InsertSessionNameOnCommandLine()
{
  TFarPanelItem * Focused = GetPanelInfo()->GetFocusedItem();

  if (Focused != NULL)
  {
    TSessionData * SessionData = reinterpret_cast<TSessionData *>(Focused->GetUserData());
    std::wstring Name;
    if (SessionData != NULL)
    {
      Name = SessionData->GetName();
    }
    else
    {
      Name = UnixIncludeTrailingBackslash(FSessionsFolder);
      if (!Focused->GetIsParentDirectory())
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
  TFarPanelItem * Focused = GetPanelInfo()->GetFocusedItem();

  if (Focused != NULL)
  {
    if (!Focused->GetIsParentDirectory())
    {
      TRemoteFile * File = reinterpret_cast<TRemoteFile *>(Focused->GetUserData());
      if (File != NULL)
      {
        if (Full)
        {
          InsertTokenOnCommandLine(File->GetFullFileName(), true);
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
  {
      BOOST_SCOPE_EXIT ( (&FileList) (&FileNames) )
      {
        delete FileList;
        delete FileNames;
      } BOOST_SCOPE_EXIT_END
    if (FileList != NULL)
    {
      for (size_t Index = 0; Index < FileList->GetCount(); Index++)
      {
        TRemoteFile * File = reinterpret_cast<TRemoteFile *>(FileList->GetObject(Index));
        if (File != NULL)
        {
          FileNames->Add(File->GetFullFileName());
        }
        else
        {
          assert(false);
        }
      }
    }
    else
    {
      if ((GetPanelInfo()->GetSelectedCount() == 0) &&
          GetPanelInfo()->GetFocusedItem()->GetIsParentDirectory())
      {
        FileNames->Add(UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory()));
      }
    }

    FPlugin->FarCopyToClipboard(FileNames);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::GetSpaceAvailable(const std::wstring &Path,
  TSpaceAvailable & ASpaceAvailable, bool & Close)
{
  // terminal can be already closed (e.g. dropped connection)
  if ((GetTerminal() != NULL) && GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    try
    {
      GetTerminal()->SpaceAvailable(Path, ASpaceAvailable);
    }
    catch (const std::exception & E)
    {
      if (!GetTerminal()->GetActive())
      {
        Close = true;
      }
      DEBUG_PRINTF(L"before HandleException");
      HandleException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ShowInformation()
{
  TSessionInfo SessionInfo = GetTerminal()->GetSessionInfo();
  TFileSystemInfo FileSystemInfo = GetTerminal()->GetFileSystemInfo();
  getspaceavailable_signal_type OnGetSpaceAvailable;
  if (GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    OnGetSpaceAvailable.connect(boost::bind(&TWinSCPFileSystem::GetSpaceAvailable, this, _1, _2, _3));
  }
  FileSystemInfoDialog(SessionInfo, FileSystemInfo, GetTerminal()->GetCurrentDirectory(),
    OnGetSpaceAvailable);
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::AreCachesEmpty()
{
  assert(Connected());
  return FTerminal->GetAreCachesEmpty();
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
  ::OpenSessionInPutty(GUIConfiguration->GetPuttyPath(), FTerminal->GetSessionData(),
    GUIConfiguration->GetPuttyPassword() ? GetTerminal()->GetPassword() : std::wstring());
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
  {
      BOOST_SCOPE_EXIT ( (&BookmarkList) )
      {
        delete BookmarkList;
      } BOOST_SCOPE_EXIT_END
    std::wstring Directory = FTerminal->GetCurrentDirectory();
    std::wstring SessionKey = FTerminal->GetSessionData()->GetSessionKey();
    TBookmarkList * CurrentBookmarkList;

    CurrentBookmarkList = FarConfiguration->GetBookmark(SessionKey);
    if (CurrentBookmarkList != NULL)
    {
      BookmarkList->Assign(CurrentBookmarkList);
    }

    if (Add)
    {
      TBookmark *Bookmark = new TBookmark;
      Bookmark->SetRemote(Directory);
      Bookmark->SetName(Directory);
      BookmarkList->Add(Bookmark);
      FarConfiguration->SetBookmark(SessionKey, BookmarkList);
    }

    bool Result = OpenDirectoryDialog(Add, Directory, BookmarkList);

    FarConfiguration->SetBookmark(SessionKey, BookmarkList);

    if (Result)
    {
      FTerminal->ChangeDirectory(Directory);
      if (UpdatePanel(true))
      {
        RedrawPanel();
      }
    }
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

  if (FarConfiguration->GetConfirmSynchronizedBrowsing())
  {
    std::wstring Message = FSynchronisingBrowse ?
      GetMsg(SYNCHRONIZE_BROWSING_ON) : GetMsg(SYNCHRONIZE_BROWSING_OFF);
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    if (MoreMessageDialog(Message, NULL, qtInformation, qaOK, &Params) ==
          qaNeverAskAgain)
    {
      FarConfiguration->SetConfirmSynchronizedBrowsing(false);
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeBrowsing(std::wstring NewPath)
{
  bool Result;
  TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
  std::wstring OldPath = AnotherPanel->GetCurrentDirectory();
  // IncludeTrailingBackslash to expand C: to C:\.
  if (!FarControl((int)INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, 
        (LONG_PTR)IncludeTrailingBackslash(NewPath).c_str()))
  {
    Result = false;
  }
  else
  {
    ResetCachedInfo();
    AnotherPanel = GetAnotherPanelInfo();
    if (!ComparePaths(AnotherPanel->GetCurrentDirectory(), NewPath))
    {
      // FAR WORKAROUND
      // If FCTL_SETANOTHERPANELDIR above fails, Far default current
      // directory to initial (?) one. So move this back to
      // previous directory.
      FarControl((int)INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, (LONG_PTR)OldPath.c_str());
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
bool TWinSCPFileSystem::SetDirectoryEx(const std::wstring &Dir, int OpMode)
{
  // DEBUG_PRINTF(L"begin, Dir = %s", Dir.c_str());
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
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->FSavedFindFolder = L"";
          } BOOST_SCOPE_EXIT_END
        Result = SetDirectoryEx(FSavedFindFolder, OpMode);
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
      FSessionsFolder = AbsolutePath(L"/" + FSessionsFolder, Dir);
      // DEBUG_PRINTF(L"FSessionsFolder = %s", FSessionsFolder.c_str());
      assert(FSessionsFolder[0] == L'/');
      FSessionsFolder.erase(0, 1);
      FNewSessionsFolder = L"";
    }
    else
    {
      assert(!FNoProgress);
      bool Normal = FLAGCLEAR(OpMode, OPM_FIND | OPM_SILENT);
      std::wstring PrevPath = FTerminal->GetCurrentDirectory();
      FNoProgress = !Normal;
      if (!FNoProgress)
      {
        FPlugin->ShowConsoleTitle(GetMsg(CHANGING_DIRECTORY_TITLE));
      }
      FTerminal->SetExceptionOnFail(true);
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->FTerminal->SetExceptionOnFail(false);
            if (!Self->FNoProgress)
            {
              Self->FPlugin->ClearConsoleTitle();
            }
            Self->FNoProgress = false;
          } BOOST_SCOPE_EXIT_END
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

      if (Normal && FSynchronisingBrowse &&
          (PrevPath != FTerminal->GetCurrentDirectory()))
      {
        TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
        if (AnotherPanel->GetIsPlugin() || (AnotherPanel->GetType() != ptFile))
        {
          MoreMessageDialog(GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED), NULL, qtError, qaOK);
        }
        else
        {
          try
          {
            std::wstring RemotePath = UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory());
            std::wstring FullPrevPath = UnixIncludeTrailingBackslash(PrevPath);
            std::wstring ALocalPath;
            if (RemotePath.substr(0, FullPrevPath.size()) == FullPrevPath)
            {
              ALocalPath = IncludeTrailingBackslash(AnotherPanel->GetCurrentDirectory()) +
                FromUnixPath(RemotePath.substr(FullPrevPath.size() + 1,
                  RemotePath.size() - FullPrevPath.size()));
            }
            else if (FullPrevPath.substr(0, RemotePath.size()) == RemotePath)
            {
              std::wstring NewLocalPath;
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
              if (MoreMessageDialog(FORMAT(GetMsg(SYNC_DIR_BROWSE_CREATE).c_str(), ALocalPath.c_str()),
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
          catch (const std::exception & E)
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
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::MakeDirectoryEx(std::wstring & Name, int OpMode)
{
  if (Connected())
  {
    assert(!(OpMode & OPM_SILENT) || !Name.empty());

    TRemoteProperties Properties = GUIConfiguration->GetNewDirectoryProperties();
    bool SaveSettings = false;

    if ((OpMode & OPM_SILENT) ||
        CreateDirectoryDialog(Name, &Properties, SaveSettings))
    {
      if (SaveSettings)
      {
        GUIConfiguration->SetNewDirectoryProperties(Properties);
      }

      FPlugin->ShowConsoleTitle(GetMsg(CREATING_FOLDER));
      {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->FPlugin->ClearConsoleTitle();
          } BOOST_SCOPE_EXIT_END
        FTerminal->CreateDirectory(Name, &Properties);
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
      /* TODO: set focus on created session directory
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
      */
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
void TWinSCPFileSystem::ProcessSessions(TObjectList * PanelItems,
  const processsession_slot_type &ProcessSession, void * Param)
{
  processsession_signal_type sig;
  sig.connect(ProcessSession);
  for (size_t Index = 0; Index < PanelItems->GetCount(); Index++)
  {
    TFarPanelItem * PanelItem = (TFarPanelItem *)PanelItems->GetItem(Index);
    assert(PanelItem);
    if (PanelItem->GetIsFile())
    {
      if (PanelItem->GetUserData() != NULL)
      {
        sig(static_cast<TSessionData *>(PanelItem->GetUserData()), Param);
        PanelItem->SetSelected(false);
      }
      else
      {
        assert(PanelItem->GetFileName() == GetMsg(NEW_SESSION_HINT));
      }
    }
    else
    {
      assert(PanelItem->GetUserData() == NULL);
      std::wstring Folder = UnixIncludeTrailingBackslash(
        UnixIncludeTrailingBackslash(FSessionsFolder) + PanelItem->GetFileName());
      size_t Index = 0;
      while (Index < StoredSessions->GetCount())
      {
        TSessionData *Data = StoredSessions->GetSession(Index);
        if (Data->GetName().substr(0, Folder.size()) == Folder)
        {
          if (StoredSessions->GetSession(Index) != Data)
          {
            Index--;
          }
          sig(Data, Param);
          // DEBUG_PRINTF(L"Index = %d, StoredSessions->GetCount = %d", Index, StoredSessions->GetCount());
        }
        Index++;
      }
      PanelItem->SetSelected(false);
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::DeleteFilesEx(TObjectList * PanelItems, int OpMode)
{
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osRemote);
    FPanelItems = PanelItems;
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FPanelItems = NULL;
          SAFE_DESTROY(Self->FFileList);
        } BOOST_SCOPE_EXIT_END
      std::wstring Query;
      bool Recycle = FTerminal->GetSessionData()->GetDeleteToRecycleBin() &&
        !FTerminal->IsRecycledFile(FFileList->GetString(0));
      if (PanelItems->GetCount() > 1)
      {
        Query = FORMAT(GetMsg(Recycle ? RECYCLE_FILES_CONFIRM : DELETE_FILES_CONFIRM).c_str(),
          PanelItems->GetCount());
      }
      else
      {
        Query = FORMAT(GetMsg(Recycle ? RECYCLE_FILE_CONFIRM : DELETE_FILE_CONFIRM).c_str(),
          ((TFarPanelItem *)PanelItems->GetItem(0))->GetFileName().c_str());
      }

      if ((OpMode & OPM_SILENT) || !FarConfiguration->GetConfirmDeleting() ||
        (MoreMessageDialog(Query, NULL, qtConfirmation, qaOK | qaCancel) == qaOK))
      {
        FTerminal->DeleteFiles(FFileList);
      }
    }
    return true;
  }
  else if (SessionList())
  {
    if ((OpMode & OPM_SILENT) || !FarConfiguration->GetConfirmDeleting() ||
      (MoreMessageDialog(GetMsg(DELETE_SESSIONS_CONFIRM), NULL, qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      ProcessSessions(PanelItems, boost::bind(&TWinSCPFileSystem::DeleteSession, this, _1, _2), NULL);
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
  std::wstring DestPath;
};
//---------------------------------------------------------------------------
int TWinSCPFileSystem::GetFilesEx(TObjectList * PanelItems, bool Move,
  std::wstring & DestPath, int OpMode)
{
  int Result;
  // DEBUG_PRINTF(L"begin, DestPath = %s, Connected = %d", DestPath.c_str(), Connected());
  if (Connected())
  {
    // FAR WORKAROUND
    // is it?
    // Probable reason was that search result window displays files from several
    // directories and the plugin can hold data for one directory only
    if (OpMode & OPM_FIND)
    {
      throw ExtException(GetMsg(VIEW_FROM_FIND_NOT_SUPPORTED));
    }

    FFileList = CreateFileList(PanelItems, osRemote);
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FPanelItems = NULL;
          SAFE_DESTROY(Self->FFileList);
        } BOOST_SCOPE_EXIT_END
      bool EditView = (OpMode & (OPM_EDIT | OPM_VIEW)) != 0;
      bool Confirmed =
        (OpMode & OPM_SILENT) &&
        (!EditView || FarConfiguration->GetEditorDownloadDefaultMode());

      TGUICopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();
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
          GetTerminal()->UsableCopyParamAttrs(Params).Download |
          FLAGMASK(EditView, cpaNoExcludeMask);
        int Options =
          FLAGMASK(EditView, coTempTransfer | coDisableNewerOnly);
        Confirmed = CopyDialog(false, Move, FFileList, DestPath,
          &CopyParam, Options, CopyParamAttrs);

        if (Confirmed && !EditView && CopyParam.GetQueue())
        {
          // these parameters are known only after transfer dialog
          Params |=
            FLAGMASK(CopyParam.GetQueueNoConfirmation(), cpNoConfirmation) |
            FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly);
          QueueAddItem(new TDownloadQueueItem(FTerminal, FFileList,
            DestPath, &CopyParam, Params));
          Confirmed = false;
        }
      }

      if (Confirmed)
      {
        if ((FFileList->GetCount() == 1) && (OpMode & OPM_EDIT))
        {
          FOriginalEditFile = IncludeTrailingBackslash(DestPath) +
            UnixExtractFileName(FFileList->GetString(0));
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
          FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly);
        FTerminal->CopyToLocal(FFileList, DestPath, &CopyParam, Params);
        Result = 1;
      }
      else
      {
        Result = -1;
      }
    }
  }
  else if (SessionList())
  {
    std::wstring Title = GetMsg(EXPORT_SESSION_TITLE);
    std::wstring Prompt;
    if (PanelItems->GetCount() == 1)
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSION_PROMPT).c_str(),
        ((TFarPanelItem *)PanelItems->GetItem(0))->GetFileName().c_str());
    }
    else
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSIONS_PROMPT).c_str(), PanelItems->GetCount());
    }

    bool AResult = (OpMode & OPM_SILENT) ||
      FPlugin->InputBox(Title, Prompt, DestPath, 0, L"Copy");
    if (AResult)
    {
      TExportSessionParam Param;
      Param.DestPath = DestPath;
      ProcessSessions(PanelItems, boost::bind(&TWinSCPFileSystem::ExportSession, this, _1, _2), &Param);
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
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ExportSession(TSessionData * Data, void * AParam)
{
  TExportSessionParam & Param = *static_cast<TExportSessionParam *>(AParam);

  THierarchicalStorage * Storage = NULL;
  TSessionData * ExportData = NULL;
  TSessionData * FactoryDefaults = new TSessionData(L"");
  {
      BOOST_SCOPE_EXIT ( (&FactoryDefaults) (&Storage) (&ExportData) )
      {
        delete FactoryDefaults;
        delete Storage;
        delete ExportData;
      } BOOST_SCOPE_EXIT_END
    ExportData = new TSessionData(Data->GetName());
    ExportData->Assign(Data);
    ExportData->SetModified(true);
    Storage = new TIniFileStorage(IncludeTrailingBackslash(Param.DestPath) +
      GUIConfiguration->GetDefaultCopyParam().ValidLocalFileName(ExportData->GetName()) + L".ini");
    if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), true))
    {
      ExportData->Save(Storage, false, FactoryDefaults);
    }
  }
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::UploadFiles(bool Move, int OpMode, bool Edit,
  std::wstring DestPath)
{
  int Result = 1;
  // DEBUG_PRINTF(L"DestPath = %s, Edit = %d", DestPath.c_str(), Edit);
  bool Confirmed = (OpMode & OPM_SILENT);
  bool Ask = !Confirmed;

  TGUICopyParamType CopyParam;

  if (Edit)
  {
    CopyParam = FLastEditCopyParam;
    Confirmed = FarConfiguration->GetEditorUploadSameOptions();
    Ask = false;
  }
  else
  {
    CopyParam = GUIConfiguration->GetDefaultCopyParam();
  }

  // these parameters are known in advance
  int Params =
    FLAGMASK(Move, cpDelete);

  if (!Confirmed)
  {
    int CopyParamAttrs =
      GetTerminal()->UsableCopyParamAttrs(Params).Upload |
      FLAGMASK(Edit, (cpaNoExcludeMask | cpaNoClearArchive));
    // heurictics: do not ask for target directory when uploaded file
    // was downloaded in edit mode
    int Options =
      FLAGMASK(Edit, coTempTransfer) |
      FLAGMASK(Edit || !GetTerminal()->GetIsCapable(fcNewerOnlyUpload), coDisableNewerOnly);
    Confirmed = CopyDialog(true, Move, FFileList, DestPath,
      &CopyParam, Options, CopyParamAttrs);

    if (Confirmed && !Edit && CopyParam.GetQueue())
    {
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(CopyParam.GetQueueNoConfirmation(), cpNoConfirmation) |
        FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly);
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
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FNoProgressFinish = false;
        } BOOST_SCOPE_EXIT_END
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(!Ask, cpNoConfirmation) |
        FLAGMASK(Edit, cpTemporary) |
        FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly)
        ;
      FTerminal->CopyToRemote(FFileList, DestPath, &CopyParam, Params);
    }
  }
  else
  {
    Result = -1;
  }
  // DEBUG_PRINTF(L"end, Result = %d", Result);
  return Result;
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode)
{
  int Result;
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osLocal);
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
          Self->FPanelItems = NULL;
          SAFE_DESTROY(Self->FFileList);
        } BOOST_SCOPE_EXIT_END
      FPanelItems = PanelItems;

      // if file is saved under different name, FAR tries to upload original file,
      // but let's be robust and check for new name, in case it changes.
      // OMP_EDIT is set since 1.70 final, only.
      // When comparing, beware that one path may be long path and the other short
      // (since 1.70 alpha 6, DestPath in GetFiles is short path,
      // while current path in PutFiles is long path)
      if (FLAGCLEAR(OpMode, OPM_SILENT) && (FFileList->GetCount() == 1) &&
          (CompareFileName(FFileList->GetString(0), FOriginalEditFile) ||
           CompareFileName(FFileList->GetString(0), FLastEditFile)))
      {
        // editor should be closed already
        assert(FLastEditorID < 0);

        if (FarConfiguration->GetEditorUploadOnSave())
        {
          // already uploaded from EE_REDRAW
          Result = -1;
        }
        else
        {
          // just in case file was saved under different name
          FFileList->PutString(0, FLastEditFile);

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
bool TWinSCPFileSystem::ImportSessions(TObjectList * PanelItems, bool /*Move*/,
  int OpMode)
{
  bool Result = (OpMode & OPM_SILENT) ||
    (MoreMessageDialog(GetMsg(IMPORT_SESSIONS_PROMPT), NULL,
      qtConfirmation, qaOK | qaCancel) == qaOK);

  if (Result)
  {
    std::wstring FileName;
    TFarPanelItem * PanelItem;
    for (size_t i = 0; i < PanelItems->GetCount(); i++)
    {
      PanelItem = (TFarPanelItem *)PanelItems->GetItem(i);
      bool AnyData = false;
      FileName = PanelItem->GetFileName();
      if (PanelItem->GetIsFile())
      {
        THierarchicalStorage * Storage = NULL;
        {
          BOOST_SCOPE_EXIT ( (&Storage) )
          {
            delete Storage;
          } BOOST_SCOPE_EXIT_END
          Storage = new TIniFileStorage(::IncludeTrailingBackslash(GetCurrentDir()) + FileName);
          if (Storage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), false) &&
              Storage->HasSubKeys())
          {
            AnyData = true;
            StoredSessions->Load(Storage, true);
            // modified only, explicit
            StoredSessions->Save(false, true);
          }
        }
      }
      if (!AnyData)
      {
        throw ExtException(FORMAT(GetMsg(IMPORT_SESSIONS_EMPTY).c_str(), FileName.c_str()));
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
    PanelInfo = this->GetPanelInfo();
  }

  TStrings * Result;
  TFarPanelItem * PanelItem = GetPanelInfo()->GetFocusedItem();
  if (PanelItem->GetIsParentDirectory())
  {
    Result = NULL;
  }
  else
  {
    Result = new TStringList();
    assert((Side == osLocal) || PanelItem->GetUserData());
    std::wstring FileName = PanelItem->GetFileName();
    if (Side == osLocal)
    {
      FileName = IncludeTrailingBackslash(GetPanelInfo()->GetCurrentDirectory()) + FileName;
    }
    Result->AddObject(FileName, (TObject *)PanelItem->GetUserData());
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateSelectedFileList(
  TOperationSide Side, TFarPanelInfo * PanelInfo)
{
  if (PanelInfo == NULL)
  {
    PanelInfo = this->GetPanelInfo();
  }

  TStrings * Result;
  if (GetPanelInfo()->GetSelectedCount() > 0)
  {
    Result = CreateFileList(GetPanelInfo()->GetItems(), Side, true,
      GetPanelInfo()->GetCurrentDirectory());
  }
  else
  {
    Result = CreateFocusedFileList(Side, PanelInfo);
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateFileList(TObjectList * PanelItems,
  TOperationSide Side, bool SelectedOnly, std::wstring Directory, bool FileNameOnly,
  TStrings * AFileList)
{
  TStrings * FileList = (AFileList == NULL ? new TStringList() : AFileList);
  try
  {
    std::wstring FileName;
    TFarPanelItem * PanelItem;
    TObject * Data = NULL;
    for (size_t Index = 0; Index < PanelItems->GetCount(); Index++)
    {
      PanelItem = (TFarPanelItem *)PanelItems->GetItem(Index);
      assert(PanelItem);
      if ((!SelectedOnly || PanelItem->GetSelected()) &&
          !PanelItem->GetIsParentDirectory())
      {
        FileName = PanelItem->GetFileName();
        if (Side == osRemote)
        {
          Data = (TRemoteFile *)PanelItem->GetUserData();
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
              FileName = ExtractFileName(FileName, false);
            }
          }
        }
        FileList->AddObject(FileName, Data);
      }
    }

    if (FileList->GetCount() == 0)
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
  // DEBUG_PRINTF(L"FTerminal->GetSessionData()->Name = %s", FTerminal->GetSessionData()->Name.c_str());
  if (!FTerminal->GetSessionData()->GetName().empty())
  {
    // DEBUG_PRINTF(L"FTerminal->GetCurrentDirectory = %s", FTerminal->GetCurrentDirectory().c_str());
    FTerminal->GetSessionData()->SetRemoteDirectory(FTerminal->GetCurrentDirectory());

    TSessionData * Data;
    Data = (TSessionData *)StoredSessions->FindByName(FTerminal->GetSessionData()->GetName());
    if (Data)
    {
      bool Changed = false;
      if (GetTerminal()->GetSessionData()->GetUpdateDirectories())
      {
        Data->SetRemoteDirectory(GetTerminal()->GetSessionData()->GetRemoteDirectory());
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
  FTerminal = new TTerminal();
  FTerminal->Init(Data, Configuration);
  try
  {
    FTerminal->SetOnQueryUser(boost::bind(&TWinSCPFileSystem::TerminalQueryUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
    FTerminal->SetOnPromptUser(boost::bind(&TWinSCPFileSystem::TerminalPromptUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
    FTerminal->SetOnDisplayBanner(boost::bind(&TWinSCPFileSystem::TerminalDisplayBanner, this, _1, _2, _3, _4, _5));
    FTerminal->SetOnShowExtendedException(boost::bind(&TWinSCPFileSystem::TerminalShowExtendedException, this, _1, _2, _3));
    FTerminal->SetOnChangeDirectory(boost::bind(&TWinSCPFileSystem::TerminalChangeDirectory, this, _1));
    FTerminal->SetOnReadDirectory(boost::bind(&TWinSCPFileSystem::TerminalReadDirectory, this, _1, _2));
    FTerminal->SetOnStartReadDirectory(boost::bind(&TWinSCPFileSystem::TerminalStartReadDirectory, this, _1));
    FTerminal->SetOnReadDirectoryProgress(boost::bind(&TWinSCPFileSystem::TerminalReadDirectoryProgress, this, _1, _2, _3));
    FTerminal->SetOnInformation(boost::bind(&TWinSCPFileSystem::TerminalInformation, this, _1, _2, _3, _4));
    FTerminal->SetOnFinished(boost::bind(&TWinSCPFileSystem::OperationFinished, this, _1, _2, _3, _4, _5, _6));
    FTerminal->SetOnProgress(boost::bind(&TWinSCPFileSystem::OperationProgress, this, _1, _2));
    FTerminal->SetOnDeleteLocalFile(boost::bind(&TWinSCPFileSystem::TerminalDeleteLocalFile, this, _1, _2));
    ConnectTerminal(FTerminal);

    FTerminal->SetOnClose(boost::bind(&TWinSCPFileSystem::TerminalClose, this, _1));

    assert(FQueue == NULL);
    FQueue = new TTerminalQueue(FTerminal, Configuration);
    FQueue->Init();
    FQueue->SetTransfersLimit(GUIConfiguration->GetQueueTransfersLimit());
    FQueue->SetOnQueryUser(boost::bind(&TWinSCPFileSystem::TerminalQueryUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
    FQueue->SetOnPromptUser(boost::bind(&TWinSCPFileSystem::TerminalPromptUser, this, _1, _2, _3, _4, _5, _6, _7, _8));
    FQueue->SetOnShowExtendedException(boost::bind(&TWinSCPFileSystem::TerminalShowExtendedException, this, _1, _2, _3));
    FQueue->SetOnListUpdate(boost::bind(&TWinSCPFileSystem::QueueListUpdate, this, _1));
    FQueue->SetOnQueueItemUpdate(boost::bind(&TWinSCPFileSystem::QueueItemUpdate, this, _1, _2));
    FQueue->SetOnEvent(boost::bind(&TWinSCPFileSystem::QueueEvent, this, _1, _2));

    assert(FQueueStatus == NULL);
    FQueueStatus = FQueue->CreateStatus(NULL);

    // TODO: Create instance of TKeepaliveThread here, once its implementation
    // is complete

    Result = true;
  }
  catch (const std::exception &E)
  {
    FTerminal->ShowExtendedException(&E);
    SAFE_DESTROY(FTerminal);
    delete FQueue;
    FQueue = NULL;
  }

  FSynchronisingBrowse = GUIConfiguration->GetSynchronizeBrowsing();

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
  TTerminal * Terminal, std::wstring Msg)
{
  assert(FAuthenticationLog != NULL);
  FAuthenticationLog->Add(Msg);
  TStringList * AuthenticationLogLines = new TStringList();
  {
      BOOST_SCOPE_EXIT ( (&AuthenticationLogLines) )
      {
        delete AuthenticationLogLines;
      } BOOST_SCOPE_EXIT_END
    size_t Width = 42;
    size_t Height = 11;
    FarWrapText(::TrimRight(FAuthenticationLog->GetText()), AuthenticationLogLines, Width);
    size_t Count;
    std::wstring Message;
    if (AuthenticationLogLines->GetCount() == 0)
    {
      Message = ::StringOfChar(' ', Width) + L"\n";
      Count = 1;
    }
    else
    {
      while (AuthenticationLogLines->GetCount() > Height)
      {
        AuthenticationLogLines->Delete(0);
      }
      AuthenticationLogLines->PutString(0, AuthenticationLogLines->GetString(0) +
          ::StringOfChar(' ', Width - AuthenticationLogLines->GetString(0).size()));
      Message = AnsiReplaceStr(AuthenticationLogLines->GetText(), L"\r", L"");
      Count = AuthenticationLogLines->GetCount();
    }

    Message += ::StringOfChar(L'\n', Height - Count);

    FPlugin->Message(0, GetTerminal()->GetSessionData()->GetSessionName(), Message);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalInformation(
  TTerminal * Terminal, const std::wstring & Str, bool /*Status*/, bool Active)
{
  if (Active)
  {
    if (GetTerminal() && (GetTerminal()->GetStatus() == ssOpening))
    {
      if (FAuthenticationLog == NULL)
      {
        FAuthenticationLog = new TStringList();
        FPlugin->SaveScreen(FAuthenticationSaveScreenHandle);
        FPlugin->ShowConsoleTitle(GetTerminal()->GetSessionData()->GetSessionName());
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
    std::wstring Directory = FTerminal->GetCurrentDirectory();
    int Index = FPathHistory->IndexOf(Directory.c_str());
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
        FORMAT(L"%s (%d)", GetMsg(READING_DIRECTORY_TITLE).c_str(), Progress));
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
void TWinSCPFileSystem::TerminalDeleteLocalFile(const std::wstring &FileName,
  bool Alternative)
{
  if (!RecursiveDeleteFile(FileName,
        (FLAGSET(FPlugin->FarSystemSettings(), FSS_DELETETORECYCLEBIN)) != Alternative))
  {
    throw ExtException(FORMAT(GetMsg(DELETE_LOCAL_FILE_ERROR).c_str(), FileName.c_str()));
  }
}
//---------------------------------------------------------------------------
int TWinSCPFileSystem::MoreMessageDialog(std::wstring Str,
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
  const std::wstring &Query, TStrings * MoreMessages, int Answers,
  const TQueryParams * Params, int & Answer, TQueryType Type, void * /*Arg*/)
{
  TMessageParams AParams;
  std::wstring AQuery = Query;

  if (Params != NULL)
  {
    if (Params->Params & qpFatalAbort)
    {
      AQuery = FORMAT(GetMsg(WARN_FATAL_ERROR).c_str(), AQuery.c_str());
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
  TPromptKind Kind, std::wstring Name, std::wstring Instructions,
  TStrings * Prompts, TStrings * Results, bool & Result,
  void * /*Arg*/)
{
  if (Kind == pkPrompt)
  {
    assert(Instructions.empty());
    assert(Prompts->GetCount() == 1);
    assert((Prompts->GetObject(0)) != NULL);
    std::wstring AResult = Results->GetString(0);

    Result = FPlugin->InputBox(Name, StripHotKey(Prompts->GetString(0)), AResult, FIB_NOUSELASTHISTORY);
    if (Result)
    {
      Results->PutString(0, AResult);
    }
  }
  else
  {
    Result = PasswordDialog(GetTerminal()->GetSessionData(), Kind, Name, Instructions,
      Prompts, Results, GetTerminal()->GetStoredCredentialsTried());
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalDisplayBanner(
  TTerminal * /*Terminal*/, std::wstring SessionName,
  const std::wstring &Banner, bool & NeverShowAgain, int Options)
{
  BannerDialog(SessionName, Banner, NeverShowAgain, Options);
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalShowExtendedException(
  TTerminal * /*Terminal*/, const std::exception * E, void * /*Arg*/)
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
  TOperationSide Side, bool /*Temp*/, const std::wstring & FileName, bool Success,
  TOnceDoneOperation & /*DisconnectWhenComplete*/)
{
  // DEBUG_PRINTF(L"begin");
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
      TObjectList *PanelItems = GetPanelInfo()->GetItems();
      for (size_t Index = 0; Index < PanelItems->GetCount(); Index++)
      {
        if (((TFarPanelItem *)PanelItems->GetItem(Index))->GetFileName() == FileName)
        {
          PanelItem = (TFarPanelItem *)PanelItems->GetItem(Index);
          break;
        }
      }
    }
    else
    {
      assert(FFileList);
      assert(FPanelItems->GetCount() == FFileList->GetCount());
      int Index = FFileList->IndexOf(FileName.c_str());
      assert(Index >= 0);
      PanelItem = (TFarPanelItem *)FPanelItems->GetItem(Index);
    }

    assert(PanelItem->GetFileName() ==
      ((Side == osLocal) ? ExtractFileName(FileName, false) : FileName));
    if (Success)
    {
      PanelItem->SetSelected(false);
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
  // DEBUG_PRINTF(L"end");
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
    static std::wstring ProgressFileLabel;
    static std::wstring TargetDirLabel;
    static std::wstring StartTimeLabel;
    static std::wstring TimeElapsedLabel;
    static std::wstring BytesTransferedLabel;
    static std::wstring CPSLabel;
    static std::wstring TimeLeftLabel;

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

    std::wstring Message1;
    std::wstring ProgressBar1;
    std::wstring Message2;
    std::wstring ProgressBar2;
    std::wstring Title = GetMsg(Captions[(int)ProgressData.Operation - 1]);
    std::wstring FileName = ProgressData.FileName;
    // for upload from temporary directory,
    // do not show source directory
    if (TransferOperation && (ProgressData.Side == osLocal) && ProgressData.Temp)
    {
      FileName = ExtractFileName(FileName, false);
    }
    Message1 = ProgressFileLabel + MinimizeName(FileName,
      ProgressWidth - ProgressFileLabel.size(), ProgressData.Side == osRemote) + L"\n";
    // for downloads to temporary directory,
    // do not show target directory
    if (TransferOperation && !((ProgressData.Side == osRemote) && ProgressData.Temp))
    {
      Message1 += TargetDirLabel + MinimizeName(ProgressData.Directory,
        ProgressWidth - TargetDirLabel.size(), ProgressData.Side == osLocal) + L"\n";
    }
    ProgressBar1 = ProgressBar(ProgressData.OverallProgress(), ProgressWidth) + L"\n";
    if (TransferOperation)
    {
      Message2 = L"\1\n";
      std::wstring StatusLine;
      std::wstring Value;

      Value = FormatDateTimeSpan(Configuration->GetTimeFormat(), ProgressData.TimeElapsed());
      StatusLine = TimeElapsedLabel +
        ::StringOfChar(L' ', ProgressWidth / 2 - 1 - TimeElapsedLabel.size() - Value.size()) +
        Value + L"  ";

      std::wstring LabelText;
      if (ProgressData.TotalSizeSet)
      {
        Value = FormatDateTimeSpan(Configuration->GetTimeFormat(), ProgressData.TotalTimeLeft());
        // DEBUG_PRINTF(L"Value = %s", Value.c_str());
        LabelText = TimeLeftLabel;
      }
      else
      {
        Value = ProgressData.StartTime.TimeString();
        LabelText = StartTimeLabel;
      }
      StatusLine = StatusLine + LabelText +
        ::StringOfChar(' ', ProgressWidth - StatusLine.size() -
        LabelText.size() - Value.size()) +
        Value;
      Message2 += StatusLine + L"\n";

      Value = FormatBytes(ProgressData.TotalTransfered);
      StatusLine = BytesTransferedLabel +
        ::StringOfChar(' ', ProgressWidth / 2 - 1 - BytesTransferedLabel.size() - Value.size()) +
        Value + L"  ";
      Value = FORMAT(L"%s/s", FormatBytes(ProgressData.CPS()).c_str());
      StatusLine = StatusLine + CPSLabel +
        ::StringOfChar(' ', ProgressWidth - StatusLine.size() -
        CPSLabel.size() - Value.size()) +
        Value;
      Message2 += StatusLine + L"\n";
      ProgressBar2 += ProgressBar(ProgressData.TransferProgress(), ProgressWidth) + L"\n";
    }
    std::wstring Message =
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
std::wstring TWinSCPFileSystem::ProgressBar(int Percentage, int Width)
{
  std::wstring Result;
  // 0xB0 - 0x2591
  // 0xDB - 0x2588
  Result = ::StringOfChar(0x2588, (Width - 5) * Percentage / 100);
  Result += ::StringOfChar(0x2591, (Width - 5) - Result.size());
  Result += FORMAT(L"%4d%%", Percentage);
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
    for (int Index = 0; Index < FQueueStatus->GetActiveCount(); Index++)
    {
      QueueItem = FQueueStatus->GetItem(Index);
      if (QueueItem->GetUserData() != NULL)
      {
        QueueItem->Update();
        Result = FQueueStatus;
      }

      if (GUIConfiguration->GetQueueAutoPopup() &&
          TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
      {
        QueueItem->ProcessUserAction();
      }
    }
  }

  if (FRefreshRemoteDirectory)
  {
    if ((GetTerminal() != NULL) && GetTerminal()->GetActive())
    {
      GetTerminal()->RefreshDirectory();
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
        if (Hidden && FarConfiguration->GetQueueBeep())
        {
          MessageBeep(MB_OK);
        }
        break;

      case qePendingUserAction:
        if (Hidden && !GUIConfiguration->GetQueueAutoPopup() && FarConfiguration->GetQueueBeep())
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

    if ((Item->GetStatus() == TQueueItem::qsDone) && (GetTerminal() != NULL))
    {
      FRefreshLocalDirectory = (QueueItem == NULL) ||
        (!QueueItem->GetInfo()->ModifiedLocal.empty());
      FRefreshRemoteDirectory = (QueueItem == NULL) ||
        (!QueueItem->GetInfo()->ModifiedRemote.empty());
    }

    if (QueueItem != NULL)
    {
      QueueItem->SetUserData((void*)true);
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
    {
        BOOST_SCOPE_EXIT ( (&ProgressData) )
        {
          ProgressData.Resume();
        } BOOST_SCOPE_EXIT_END
      TCancelStatus ACancel;
      int Result;
      if (ProgressData.TransferingFile &&
          (ProgressData.TimeExpected() > GUIConfiguration->GetIgnoreCancelBeforeFinish()))
      {
        Result = MoreMessageDialog(GetMsg(CANCEL_OPERATION_FATAL), NULL,
          qtWarning, qaYes | qaNo | qaCancel);
      }
      else
      {
        Result = MoreMessageDialog(GetMsg(CANCEL_OPERATION), NULL,
          qtConfirmation, qaOK | qaCancel);
        DEBUG_PRINTF(L"after MoreMessageDialog: Result = %d", Result);
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
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::UploadFromEditor(bool NoReload, std::wstring FileName,
  std::wstring DestPath)
{
  assert(FFileList == NULL);
  FFileList = new TStringList();
  assert(FTerminal->GetAutoReadDirectory());
  bool PrevAutoReadDirectory = FTerminal->GetAutoReadDirectory();
  if (NoReload)
  {
    FTerminal->SetAutoReadDirectory(false);
    if (UnixComparePaths(DestPath, FTerminal->GetCurrentDirectory()))
    {
      FReloadDirectory = true;
    }
  }

  {
      BOOST_SCOPE_EXIT ( (&Self) (&PrevAutoReadDirectory) )
      {
        Self->FTerminal->SetAutoReadDirectory(PrevAutoReadDirectory);
        SAFE_DESTROY(Self->FFileList);
      } BOOST_SCOPE_EXIT_END
    FFileList->Add(FileName);
    UploadFiles(false, 0, true, DestPath);
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::UploadOnSave(bool NoReload)
{
  TFarEditorInfo * Info = FPlugin->EditorInfo();
  if (Info != NULL)
  {
    {
        BOOST_SCOPE_EXIT ( (&Info) )
        {
          delete Info;
        } BOOST_SCOPE_EXIT_END
      bool NativeEdit =
        (FLastEditorID >= 0) &&
        (FLastEditorID == Info->GetEditorID()) &&
        !FLastEditFile.empty();

      TMultipleEdits::iterator I = FMultipleEdits.find(Info->GetEditorID());
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
        {
            BOOST_SCOPE_EXIT ( (&Info) )
            {
              delete Info;
            } BOOST_SCOPE_EXIT_END
          TMultipleEdits::iterator I = FMultipleEdits.find(Info->GetEditorID());
          if (I != FMultipleEdits.end())
          {
            std::wstring FullFileName = UnixIncludeTrailingBackslash(I->second.Directory) +
              I->second.FileName;
            FPlugin->FarEditorControl(ECTL_SETTITLE, (void *)FullFileName.c_str());
          }
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
        {
            BOOST_SCOPE_EXIT ( (&Info) )
            {
              delete Info;
            } BOOST_SCOPE_EXIT_END
          if (!FLastEditFile.empty() &&
              AnsiSameText(FLastEditFile, Info->GetFileName()))
          {
            FLastEditorID = Info->GetEditorID();
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
              MultipleEdit.FileName = ExtractFileName(Info->GetFileName(), false);
              MultipleEdit.Directory = FLastMultipleEditDirectory;
              MultipleEdit.LocalFileName = Info->GetFileName();
              MultipleEdit.PendingSave = false;
              FMultipleEdits[Info->GetEditorID()] = MultipleEdit;
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
      {
            BOOST_SCOPE_EXIT ( (&Info) )
            {
              delete Info;
            } BOOST_SCOPE_EXIT_END
        if (FLastEditorID == Info->GetEditorID())
        {
          FLastEditorID = -1;
        }

        TMultipleEdits::iterator I = FMultipleEdits.find(Info->GetEditorID());
        if (I != FMultipleEdits.end())
        {
          if (I->second.PendingSave)
          {
            UploadFromEditor(true, Info->GetFileName(), I->second.Directory);
            // reload panel content (if uploaded to current directory.
            // no need for RefreshPanel as panel is not visible yet.
            UpdatePanel();
          }

          if (::DeleteFile(Info->GetFileName()))
          {
            // remove directory only if it is empty
            // (to avoid deleting another directory if user uses "save as")
            ::RemoveDir(ExcludeTrailingBackslash(ExtractFilePath(Info->GetFileName())));
          }

          FMultipleEdits.erase(I);
        }
      }
    }
  }
  else if (Event == EE_SAVE)
  {
    TFarEditorInfo * Info = FPlugin->EditorInfo();
    if (Info != NULL)
    {
      {
            BOOST_SCOPE_EXIT ( (&Info) )
            {
              delete Info;
            } BOOST_SCOPE_EXIT_END
        if ((FLastEditorID >= 0) && (FLastEditorID == Info->GetEditorID()))
        {
          // if the file is saved under different name ("save as"), we upload
          // the file back under that name
          FLastEditFile = Info->GetFileName();

          if (FarConfiguration->GetEditorUploadOnSave())
          {
            FEditorPendingSave = true;
          }
        }

        TMultipleEdits::iterator I = FMultipleEdits.find(Info->GetEditorID());
        if (I != FMultipleEdits.end())
        {
          if (I->second.LocalFileName != Info->GetFileName())
          {
            // update file name (after "save as")
            I->second.LocalFileName = Info->GetFileName();
            I->second.FileName = ::ExtractFileName(Info->GetFileName(), true);
            // update editor title
            std::wstring FullFileName = UnixIncludeTrailingBackslash(I->second.Directory) +
              I->second.FileName;
            // note that we need to reset the title periodically (see EE_REDRAW)
            FPlugin->FarEditorControl(ECTL_SETTITLE, (void *)FullFileName.c_str());
          }

          if (FarConfiguration->GetEditorUploadOnSave())
          {
            FEditorPendingSave = true;
          }
          else
          {
            I->second.PendingSave = true;
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::EditViewCopyParam(TCopyParamType & CopyParam)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);
  // we have no way to give FAR back the modified filename, so make sure we
  // fail downloading file not valid on windows
  CopyParam.SetReplaceInvalidChars(false);
  CopyParam.SetFileMask(L"");
  CopyParam.SetExcludeFileMask(TFileMasks());
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::MultipleEdit()
{
  if ((GetPanelInfo()->GetFocusedItem() != NULL) &&
      GetPanelInfo()->GetFocusedItem()->GetIsFile() &&
      (GetPanelInfo()->GetFocusedItem()->GetUserData() != NULL))
  {
    TStrings * FileList = CreateFocusedFileList(osRemote);
    assert((FileList == NULL) || (FileList->GetCount() == 1));

    if (FileList != NULL)
    {
      {
            BOOST_SCOPE_EXIT ( (&FileList) )
            {
              delete FileList;
            } BOOST_SCOPE_EXIT_END
        if (FileList->GetCount() == 1)
        {
          MultipleEdit(FTerminal->GetCurrentDirectory(), FileList->GetString(0),
            (TRemoteFile*)FileList->GetObject(0));
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::MultipleEdit(std::wstring Directory,
  std::wstring FileName, TRemoteFile * File)
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

  std::wstring FullFileName = UnixIncludeTrailingBackslash(Directory) + FileName;

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
    switch (MoreMessageDialog(FORMAT(GetMsg(EDITOR_ALREADY_LOADED).c_str(), FullFileName.c_str()),
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
    std::wstring TempDir;
    TGUICopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();
    EditViewCopyParam(CopyParam);

    TStrings * FileList = new TStringList;
    assert(!FNoProgressFinish);
    FNoProgressFinish = true;
    {
        BOOST_SCOPE_EXIT ( (&Self) (&FileList) )
        {
          Self->FNoProgressFinish = false;
          delete FileList;
        } BOOST_SCOPE_EXIT_END
      FileList->AddObject(FullFileName, File);
      TemporarilyDownloadFiles(FileList, CopyParam, TempDir);
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
  {
    BOOST_SCOPE_EXIT ( (&MenuItems) )
    {
      delete MenuItems;
    } BOOST_SCOPE_EXIT_END
    TEditHistories::const_iterator i = FEditHistories.begin();
    while (i != FEditHistories.end())
    {
      MenuItems->Add(MinimizeName(UnixIncludeTrailingBackslash((*i).Directory) + (*i).FileName,
        FPlugin->MaxMenuItemLength(), true));
      ++i;
    }

    MenuItems->Add(L"");
    MenuItems->SetItemFocused(MenuItems->GetCount() - 1);

    const int BreakKeys[] = { VK_F4, 0 };

    int BreakCode;
    int Result = FPlugin->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
      GetMsg(MENU_EDIT_HISTORY), L"", MenuItems, BreakKeys, BreakCode);

    if ((Result >= 0) && (Result < int(FEditHistories.size())))
    {
      TRemoteFile * File;
      std::wstring FullFileName =
        UnixIncludeTrailingBackslash(FEditHistories[Result].Directory) + FEditHistories[Result].FileName;
      FTerminal->ReadFile(FullFileName, File);
      {
            BOOST_SCOPE_EXIT ( (&File) )
            {
              delete File;
            } BOOST_SCOPE_EXIT_END
        if (!File->GetHaveFullFileName())
        {
          File->SetFullFileName(FullFileName);
        }
        MultipleEdit(FEditHistories[Result].Directory,
          FEditHistories[Result].FileName, File);
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::IsLogging()
{
  return
    Connected() && FTerminal->GetLog()->GetLoggingToFile();
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::ShowLog()
{
  assert(Connected() && FTerminal->GetLog()->GetLoggingToFile());
  FPlugin->Viewer(FTerminal->GetLog()->GetCurrentFileName(), VF_NONMODAL);
}
