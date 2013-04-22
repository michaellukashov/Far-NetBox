//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "WinSCPFileSystem.h"
#include "WinSCPPlugin.h"
#include "FarDialog.h"
#include "FarTexts.h"
#include "FarConfiguration.h"
#include <Common.h>
#include <Exceptions.h>
#include <SessionData.h>
#include <CoreMain.h>
#include <ScpFileSystem.h>
#include <Bookmarks.h>
#include <GUITools.h>
#include "guid.h"

#include <SysUtils.hpp>
#include <CompThread.hpp>
#include "PuttyIntf.h"
#include "XmlStorage.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
//------------------------------------------------------------------------------
TSessionPanelItem::TSessionPanelItem(TSessionData * ASessionData):
  TCustomFarPanelItem()
{
  assert(ASessionData);
  FSessionData = ASessionData;
}
//------------------------------------------------------------------------------
void TSessionPanelItem::SetPanelModes(TFarPanelModes * PanelModes)
{
  assert(FarPlugin);
  TStrings * ColumnTitles = new TStringList();
  {
    std::auto_ptr<TStrings> ColumnTitlesPtr;
    ColumnTitlesPtr.reset(ColumnTitles);
    ColumnTitles->Add(FarPlugin->GetMsg(SESSION_NAME_COL_TITLE));
    for (intptr_t Index = 0; Index < PANEL_MODES_COUNT; ++Index)
    {
      PanelModes->SetPanelMode(Index, L"N", L"0", ColumnTitles, false, false, false);
    }
  }
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
void TSessionPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & FileName, __int64 & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& UserData, size_t & /*CustomColumnNumber*/)
{
  FileName = ::UnixExtractFileName(FSessionData->GetName());
  UserData = FSessionData;
}
//------------------------------------------------------------------------------
TSessionFolderPanelItem::TSessionFolderPanelItem(const UnicodeString & Folder):
  TCustomFarPanelItem(),
  FFolder(Folder)
{
}
//------------------------------------------------------------------------------
void TSessionFolderPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & FileName, __int64 & /*Size*/,
  uintptr_t & FileAttributes,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  FileName = FFolder;
  FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
}
//------------------------------------------------------------------------------
TRemoteFilePanelItem::TRemoteFilePanelItem(TRemoteFile * ARemoteFile):
  TCustomFarPanelItem()
{
  assert(ARemoteFile);
  FRemoteFile = ARemoteFile;
}
//------------------------------------------------------------------------------
void TRemoteFilePanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & FileName, __int64 & Size,
  uintptr_t & FileAttributes,
  TDateTime & LastWriteTime, TDateTime & LastAccess,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber)
{
  FileName = FRemoteFile->GetFileName();
  Size = FRemoteFile->GetSize();
  if (Size < 0) Size = 0;
  FileAttributes =
    FLAGMASK(FRemoteFile->GetIsDirectory(), FILE_ATTRIBUTE_DIRECTORY) |
    FLAGMASK(FRemoteFile->GetIsHidden(), FILE_ATTRIBUTE_HIDDEN) |
    FLAGMASK(FRemoteFile->GetRights()->GetReadOnly(), FILE_ATTRIBUTE_READONLY) |
    FLAGMASK(FRemoteFile->GetIsSymLink(), FILE_ATTRIBUTE_REPARSE_POINT);
  LastWriteTime = FRemoteFile->GetModification();
  LastAccess = FRemoteFile->GetLastAccess();
  Owner = FRemoteFile->GetFileOwner().GetName();
  UserData = FRemoteFile;
  CustomColumnNumber = 4;
}
//------------------------------------------------------------------------------
UnicodeString TRemoteFilePanelItem::GetCustomColumnData(size_t Column)
{
  switch (Column) {
    case 0: return FRemoteFile->GetFileGroup().GetName();
    case 1: return FRemoteFile->GetRightsStr();
    case 2: return FRemoteFile->GetRights()->GetOctal();
    case 3: return FRemoteFile->GetLinkTo();
    default: assert(false); return UnicodeString();
  }
}
//------------------------------------------------------------------------------
void TRemoteFilePanelItem::TranslateColumnTypes(UnicodeString & ColumnTypes,
    TStrings * ColumnTitles)
{
  UnicodeString AColumnTypes = ColumnTypes;
  ColumnTypes = L"";
  UnicodeString Column;
  UnicodeString Title;
  while (!AColumnTypes.IsEmpty())
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
    ColumnTypes += (ColumnTypes.IsEmpty() ? L"" : L",") + Column;
    if (ColumnTitles)
    {
      ColumnTitles->Add(Title);
    }
  }
}
//------------------------------------------------------------------------------
void TRemoteFilePanelItem::SetPanelModes(TFarPanelModes * PanelModes)
{
  assert(FarPlugin);
  TStrings * ColumnTitles = new TStringList();
  {
    std::auto_ptr<TStrings> ColumnTitlesPtr;
    ColumnTitlesPtr.reset(ColumnTitles);
    if (FarConfiguration->GetCustomPanelModeDetailed())
    {
      UnicodeString ColumnTypes = FarConfiguration->GetColumnTypesDetailed();
      UnicodeString StatusColumnTypes = FarConfiguration->GetStatusColumnTypesDetailed();

      TranslateColumnTypes(ColumnTypes, ColumnTitles);
      TranslateColumnTypes(StatusColumnTypes, NULL);

      PanelModes->SetPanelMode(5 /*detailed */,
        ColumnTypes, FarConfiguration->GetColumnWidthsDetailed(),
        ColumnTitles, FarConfiguration->GetFullScreenDetailed(), false, true, false,
        StatusColumnTypes, FarConfiguration->GetStatusColumnWidthsDetailed());
    }
  }
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class TFarInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TFarInteractiveCustomCommand(TCustomFarPlugin * Plugin,
    TCustomCommand * ChildCustomCommand);

protected:
  virtual void Prompt(intptr_t Index, const UnicodeString & Prompt,
    UnicodeString & Value);

private:
  TCustomFarPlugin * FPlugin;
};
//------------------------------------------------------------------------------
TFarInteractiveCustomCommand::TFarInteractiveCustomCommand(
  TCustomFarPlugin * Plugin, TCustomCommand * ChildCustomCommand) :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FPlugin = Plugin;
}
//------------------------------------------------------------------------------
void TFarInteractiveCustomCommand::Prompt(intptr_t /*Index*/,
  const UnicodeString & Prompt, UnicodeString & Value)
{
  UnicodeString APrompt = Prompt;
  if (APrompt.IsEmpty())
  {
    APrompt = FPlugin->GetMsg(APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!FPlugin->InputBox(FPlugin->GetMsg(APPLY_COMMAND_PARAM_TITLE),
        APrompt, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Attempt to allow keepalives from background thread.
// Not finished nor used.
class TKeepaliveThread : public TSimpleThread
{
public:
  explicit TKeepaliveThread(TWinSCPFileSystem * FileSystem, TDateTime Interval);
  virtual ~TKeepaliveThread()
  {}
  virtual void Init();
  virtual void Execute();
  virtual void Terminate();

private:
  TWinSCPFileSystem * FFileSystem;
  TDateTime FInterval;
  HANDLE FEvent;
};
//------------------------------------------------------------------------------
TKeepaliveThread::TKeepaliveThread(TWinSCPFileSystem * FileSystem,
  TDateTime Interval) :
  TSimpleThread(),
  FFileSystem(FileSystem),
  FInterval(Interval),
  FEvent(0)
{
}
//------------------------------------------------------------------------------
void TKeepaliveThread::Init()
{
  TSimpleThread::Init();
  FEvent = CreateEvent(NULL, false, false, NULL);
  Start();
}
//------------------------------------------------------------------------------
void TKeepaliveThread::Terminate()
{
  // TCompThread::Terminate();
  SetEvent(FEvent);
}
//------------------------------------------------------------------------------
void TKeepaliveThread::Execute()
{
  while (!IsFinished())
  {
    static long MillisecondsPerDay = 24 * 60 * 60 * 1000;
    if ((WaitForSingleObject(FEvent, static_cast<DWORD>(
         static_cast<double>(FInterval) * MillisecondsPerDay)) != WAIT_FAILED) &&
        !IsFinished())
    {
      FFileSystem->KeepaliveThreadCallback();
    }
  }
  CloseHandle(FEvent);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TWinSCPFileSystem::TWinSCPFileSystem(TCustomFarPlugin * APlugin) :
  TCustomFarFileSystem(APlugin)
{
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::Init(TSecureShell * /* SecureShell */)
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
  FPathHistory = new TStringList();

  FLastMultipleEditReadOnly = false;
  FEditorPendingSave = false;
  FOutputLog = false;
}

//------------------------------------------------------------------------------
TWinSCPFileSystem::~TWinSCPFileSystem()
{
  Disconnect();
  delete FQueueStatusSection;
  FQueueStatusSection = NULL;
  delete FPathHistory;
  FPathHistory = NULL;
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::HandleException(Exception * E, int OpMode)
{
  if ((GetTerminal() != NULL) && ::InheritsFrom<std::exception, EFatal>(E))
  {
    if (!FClosed)
    {
      ClosePanel();
    }
    GetTerminal()->ShowExtendedException(E);
  }
  else
  {
    TCustomFarFileSystem::HandleException(E, OpMode);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::KeepaliveThreadCallback()
{
  TGuard Guard(FCriticalSection);

  if (Connected())
  {
    FTerminal->Idle();
  }
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::SessionList()
{
  return (FTerminal == NULL);
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::Connected()
{
  // Check for active added to avoid "disconnected" message popup repeatedly
  // from "idle"
  return !SessionList() && FTerminal->GetActive();
}
//------------------------------------------------------------------------------
TWinSCPPlugin * TWinSCPFileSystem::WinSCPPlugin()
{
  return dynamic_cast<TWinSCPPlugin *>(FPlugin);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::Close()
{
  TRY_FINALLY (
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
  ,
  {
    TCustomFarFileSystem::Close();
  }
  );
}

//------------------------------------------------------------------------------
void TWinSCPFileSystem::GetOpenPanelInfoEx(OPENPANELINFO_FLAGS &Flags,
    UnicodeString & /*HostFile*/, UnicodeString & CurDir, UnicodeString & Format,
    UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & /*StartPanelMode*/,
    OPENPANELINFO_SORTMODES & /*StartSortMode*/, bool & /*StartSortOrder*/, TFarKeyBarTitles * KeyBarTitles,
    UnicodeString & ShortcutData)
{
  if (!SessionList())
  {
    Flags = !OPIF_DISABLEFILTER | !OPIF_DISABLESORTGROUPS | !OPIF_DISABLEHIGHLIGHTING |
      OPIF_SHOWPRESERVECASE | OPIF_COMPAREFATTIME | OPIF_SHORTCUT;

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
    Format = L"netbox";
    Flags = !OPIF_DISABLESORTGROUPS | !OPIF_DISABLEHIGHLIGHTING | OPIF_USEATTRHIGHLIGHTING |
      OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE | OPIF_SHORTCUT;
    PanelTitle = FORMAT(L" %s ", GetMsg(NB_STORED_SESSION_TITLE).c_str());

    TSessionPanelItem::SetPanelModes(PanelModes);
    TSessionPanelItem::SetKeyBarTitles(KeyBarTitles);
  }
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::GetFindDataEx(TObjectList * PanelItems, int OpMode)
{
  bool Result = false;
  if (Connected())
  {
    assert(!FNoProgress);
    // OPM_FIND is used also for calculation of directory size (F3, quick view).
    // However directory is usually read from SetDirectory, so FNoProgress
    // seems to have no effect here.
    // Do not know if OPM_SILENT is even used.
    FNoProgress = FLAGSET(OpMode, OPM_FIND) || FLAGSET(OpMode, OPM_SILENT);
    TRY_FINALLY (
    {
      if (FReloadDirectory && FTerminal->GetActive())
      {
        FReloadDirectory = false;
        FTerminal->ReloadDirectory();
      }

      // TCustomFileSystem * FileSystem = GetTerminal()->GetFileSystem();
      for (intptr_t Index = 0; Index < FTerminal->GetFiles()->GetCount(); ++Index)
      {
        TRemoteFile * File = FTerminal->GetFiles()->GetFiles(Index);
        /*if (File->GetIsSymLink())
        {
          // Check what kind of symlink this is
          const UnicodeString LinkFileName = File->GetLinkTo();
          TRemoteFile * LinkFile = NULL;
          FileSystem->ReadFile(LinkFileName, LinkFile);
          if ((LinkFile != NULL) && LinkFile->GetIsDirectory())
          {
            File->SetType(FILETYPE_DIRECTORY);
          }
          delete LinkFile;
        }*/
        PanelItems->Add(static_cast<TObject *>(new TRemoteFilePanelItem(File)));
      }
    }
    ,
    {
      FNoProgress = false;
    }
    );
    Result = true;
  }
  else if (SessionList())
  {
    Result = true;
    assert(StoredSessions);
    StoredSessions->Load();
    UnicodeString Folder = FSessionsFolder;
    if (!FSessionsFolder.IsEmpty())
    {
      Folder = ::UnixIncludeTrailingBackslash(FSessionsFolder);
    }
    TSessionData * Data = NULL;
    std::auto_ptr<TStringList> ChildPaths(new TStringList());
    {
      ChildPaths->CaseSensitive = false;
      for (intptr_t Index = 0; Index < StoredSessions->GetCount(); ++Index)
      {
        Data = StoredSessions->GetSession(Index);
        if (Data->GetName().SubString(1, Folder.Length()) == Folder)
        {
          UnicodeString Name = Data->GetName().SubString(
            Folder.Length() + 1, Data->GetName().Length() - Folder.Length());
          intptr_t Slash = Name.Pos(L'/');
          if (Slash > 0)
          {
            Name.SetLength(Slash - 1);
            if (ChildPaths->IndexOf(Name.c_str()) < 0)
            {
              PanelItems->Add(static_cast<TObject *>(new TSessionFolderPanelItem(Name)));
              ChildPaths->Add(Name);
            }
          }
          else
          {
            PanelItems->Add(static_cast<TObject *>(new TSessionPanelItem(Data)));
          }
        }
      }
    }

    if (!FNewSessionsFolder.IsEmpty())
    {
      PanelItems->Add(static_cast<TObject *>(new TSessionFolderPanelItem(FNewSessionsFolder)));
    }
    if (PanelItems->GetCount() == 0)
    {
      PanelItems->Add(static_cast<TObject *>(new THintPanelItem(GetMsg(NEW_SESSION_HINT))));
    }

    TWinSCPFileSystem * OppositeFileSystem =
      dynamic_cast<TWinSCPFileSystem *>(GetOppositeFileSystem());
    if ((OppositeFileSystem != NULL) && !OppositeFileSystem->Connected() &&
        !OppositeFileSystem->FLoadingSessionList)
    {
      FLoadingSessionList = true;
      TRY_FINALLY (
      {
        UpdatePanel(false, true);
        RedrawPanel(true);
      }
      ,
      {
        FLoadingSessionList = false;
      }
      );
    }
    if (!FPrevSessionName.IsEmpty())
    {
      TSessionData * PrevSession = StoredSessions->GetSessionByName(FPrevSessionName);
      FPrevSessionName.Clear();
      if (UpdatePanel())
      {
        RedrawPanel();
        FocusSession(PrevSession);
      }
    }
  }
  else
  {
    Result = false;
  }
  return Result;
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::DuplicateOrRenameSession(TSessionData * Data,
 bool Duplicate)
{
  assert(Data);
  UnicodeString Name = Data->GetName();
  if (WinSCPPlugin()->InputBox(GetMsg(Duplicate ? DUPLICATE_SESSION_TITLE : RENAME_SESSION_TITLE),
        GetMsg(Duplicate ? DUPLICATE_SESSION_PROMPT : RENAME_SESSION_PROMPT),
        Name, NULL) &&
      !Name.IsEmpty() && (Name != Data->GetName()))
  {
    Name = ReplaceChar(Name, L'\\', L'/');
    TNamedObject * EData = StoredSessions->FindByName(Name);
    if ((EData != NULL) && (EData != Data))
    {
      throw Exception(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR).c_str(), Name.c_str()));
    }
    else
    {
      TSessionData * NData = StoredSessions->NewSession(Name, Data);
      FSessionsFolder = ExcludeTrailingBackslash(::UnixExtractFilePath(Name));

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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::FocusSession(TSessionData * Data)
{
  TFarPanelItem * SessionItem = GetPanelInfo()->FindUserData(Data);
  if (SessionItem != NULL)
  {
    GetPanelInfo()->SetFocusedItem(SessionItem);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::EditConnectSession(TSessionData * Data, bool Edit)
{
  bool NewData = !Data;
  bool FillInConnect = !Edit && !Data->GetCanLogin();
  if (NewData || FillInConnect)
  {
    Data = new TSessionData(L"");
  }

  TRY_FINALLY (
  {
    EditConnectSession(Data, Edit, NewData, FillInConnect);
  }
  ,
  {
    if (NewData || FillInConnect)
    {
      delete Data;
    }
  }
  );
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::EditConnectSession(TSessionData * Data, bool Edit, bool NewData, bool FillInConnect)
{
  TSessionData * OrigData = Data;
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
      if ((!NewData && !FillInConnect) || (Action != saConnect))
      {
        TSessionData * SelectSession = NULL;
        if (NewData)
        {
          // UnicodeString Name =
          //    IncludeTrailingBackslash(FSessionsFolder) + Data->GetSessionName();
          UnicodeString Name;
          if (!FSessionsFolder.IsEmpty())
          {
            Name = ::UnixIncludeTrailingBackslash(FSessionsFolder);
          }
          Name += Data->GetSessionName();
          if (WinSCPPlugin()->InputBox(GetMsg(NEW_SESSION_NAME_TITLE),
                                GetMsg(NEW_SESSION_NAME_PROMPT), Name, 0) &&
              !Name.IsEmpty())
          {
            if (StoredSessions->FindByName(Name))
            {
              throw Exception(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR).c_str(), Name.c_str()));
            }
            else
            {
              SelectSession = StoredSessions->NewSession(Name, Data);
              FSessionsFolder = ExcludeTrailingBackslash(::UnixExtractFilePath(Name));
            }
          }
        }
        else if (FillInConnect)
        {
          UnicodeString OrigName = OrigData->GetName();
          OrigData->Assign(Data);
          OrigData->SetName(OrigName);
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
      if (GetPanelInfo()->GetItemCount())
      {
        GetPanelInfo()->SetFocusedIndex(0);
      }
    }
  }
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::ProcessPanelEventEx(intptr_t Event, void *Param)
{
  bool Result = false;
  if (Connected())
  {
    if (Event == FE_COMMAND)
    {
      UnicodeString Command = static_cast<wchar_t *>(Param);
      if (!::Trim(Command).IsEmpty() &&
          (::LowerCase(Command.SubString(1, 3)) != L"cd "))
      {
        Result = ExecuteCommand(Command);
      }
    }
    else if (Event == FE_IDLE)
    {
      // FAR WORKAROUND
      // Control(FCTL_CLOSEPLUGIN) does not seem to close plugin when called from
      // ProcessPanelEvent(FE_IDLE). So if TTerminal::Idle() causes session to close
      // we must count on having ProcessPanelEvent(FE_IDLE) called again.
      FTerminal->Idle();
      if (FQueue != NULL)
      {
        FQueue->Idle();
      }
      ProcessQueue(true);
    }
  }
  else
  {
    if (Event == FE_CLOSE && !FClosed)
    {
      ClosePanel();
    }
  }
  return Result;
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalCaptureLog(
  const UnicodeString & AddedLine, bool /*StdError*/)
{
  if (FOutputLog)
  {
    WinSCPPlugin()->WriteConsole(AddedLine + L"\n");
  }
  if (FCapturedLog != NULL)
  {
    FCapturedLog->Add(AddedLine);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::RequireLocalPanel(TFarPanelInfo * Panel, const UnicodeString & Message)
{
  if (Panel->GetIsPlugin() || (Panel->GetType() != ptFile))
  {
    throw Exception(Message);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::RequireCapability(intptr_t Capability)
{
  if (!FTerminal->GetIsCapable(static_cast<TFSCapability>(Capability)))
  {
    throw Exception(FORMAT(GetMsg(OPERATION_NOT_SUPPORTED).c_str(),
      FTerminal->GetFileSystemInfo().ProtocolName.c_str()));
  }
}
//------------------------------------------------------------------------------
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
      uintptr_t Answer = MoreMessageDialog(FORMAT(GetMsg(PERFORM_ON_COMMAND_SESSION).c_str(),
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
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::ExecuteCommand(const UnicodeString & Command)
{
  if (FTerminal->AllowedAnyCommand(Command) &&
      EnsureCommandSessionFallback(fcAnyCommand))
  {
    FTerminal->BeginTransaction();
    TRY_FINALLY (
    {
      FarControl(FCTL_SETCMDLINE, 0, reinterpret_cast<void *>(L""));
      WinSCPPlugin()->ShowConsoleTitle(Command);
      TRY_FINALLY (
      {
        WinSCPPlugin()->ShowTerminalScreen();

        FOutputLog = true;
        FTerminal->AnyCommand(Command, MAKE_CALLBACK(TWinSCPFileSystem::TerminalCaptureLog, this));
      }
      ,
      {
        WinSCPPlugin()->ScrollTerminalScreen(1);
        WinSCPPlugin()->SaveTerminalScreen();
        WinSCPPlugin()->ClearConsoleTitle();
      }
      );
    }
    ,
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
    );
  }
  return true;
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::ProcessKeyEx(intptr_t Key, uintptr_t ControlState)
{
  bool Handled = false;

  TFarPanelItem * Focused = GetPanelInfo()->GetFocusedItem();

  if ((Key == 'W') && (ControlState & SHIFTMASK) &&
      (ControlState & ALTMASK))
  {
    WinSCPPlugin()->CommandsMenu(true);
    Handled = true;
  }
  else if (SessionList())
  {
    TSessionData * Data = NULL;
    if ((Focused != NULL) && Focused->GetIsFile() && Focused->GetUserData())
    {
      Data = static_cast<TSessionData *>(Focused->GetUserData());
    }

    if ((Key == 'F') && (ControlState & CTRLMASK))
    {
      InsertSessionNameOnCommandLine();
      Handled = true;
    }

    if ((Key == VK_RETURN) && (ControlState & CTRLMASK))
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
      if ((Data != NULL) || (StoredSessions->GetCount() == 0))
      {
        EditConnectSession(Data, true);
      }
      Handled = true;
    }

    if (Key == VK_F4 && (ControlState & SHIFTMASK))
    {
      EditConnectSession(NULL, true);
      Handled = true;
    }

    if (((Key == VK_F5) || (Key == VK_F6)) &&
        (ControlState & SHIFTMASK))
    {
      if (Data != NULL)
      {
        DuplicateOrRenameSession(Data, Key == VK_F5);
      }
      Handled = true;
    }
  }
  else if (Connected())
  {
    if ((Key == 'F') && (ControlState & CTRLMASK))
    {
      InsertFileNameOnCommandLine(true);
      Handled = true;
    }

    if ((Key == VK_RETURN) && (ControlState & CTRLMASK))
    {
      InsertFileNameOnCommandLine(false);
      Handled = true;
    }

    if ((Key == 'R') && (ControlState & CTRLMASK))
    {
      FReloadDirectory = true;
    }

    if ((Key == 'A') && (ControlState & CTRLMASK))
    {
      FileProperties();
      Handled = true;
    }

    if ((Key == 'G') && (ControlState & CTRLMASK))
    {
      ApplyCommand();
      Handled = true;
    }

    if ((Key == 'Q') && (ControlState & SHIFTMASK) &&
        (ControlState & ALTMASK))
    {
      QueueShow(false);
      Handled = true;
    }

    if ((Key == 'B') && (ControlState & CTRLMASK) &&
          (ControlState & ALTMASK))
    {
      ToggleSynchronizeBrowsing();
      Handled = true;
    }

    if ((Key == VK_INSERT) &&
        (FLAGSET(ControlState, ALTMASK | SHIFTMASK) || FLAGSET(ControlState, CTRLMASK | ALTMASK)))
    {
      CopyFullFileNamesToClipboard();
      Handled = true;
    }

    if ((Key == VK_F6) && (ControlState & ALTMASK) && !(ControlState & SHIFTMASK))
    {
      CreateLink();
      Handled = true;
    }

    if (Focused && ((Key == VK_F5) || (Key == VK_F6)) &&
        (ControlState & SHIFTMASK) && !(ControlState & ALTMASK))
    {
      TransferFiles((Key == VK_F6));
      Handled = true;
    }

    if (Focused && (Key == VK_F6) &&
        ((ControlState & SHIFTMASK) && (ControlState & ALTMASK)))
    {
      RenameFile();
      Handled = true;
    }

    if ((Key == VK_F12) && (ControlState & SHIFTMASK) &&
        (ControlState & ALTMASK))
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
  return Handled;
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::CreateLink()
{
  RequireCapability(fcResolveSymlink);
  RequireCapability(fcSymbolicLink);

  bool Edit = false;
  TRemoteFile * File = NULL;
  UnicodeString FileName;
  UnicodeString PointTo;
  bool SymbolicLink = true;

  if (GetPanelInfo()->GetFocusedItem() && GetPanelInfo()->GetFocusedItem()->GetUserData())
  {
    File = static_cast<TRemoteFile *>(GetPanelInfo()->GetFocusedItem()->GetUserData());

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
      intptr_t Params = dfNoRecursive;
      GetTerminal()->SetExceptionOnFail(true);
      TRY_FINALLY (
      {
        GetTerminal()->DeleteFile(L"", File, &Params);
      }
      ,
      {
        GetTerminal()->SetExceptionOnFail(false);
      }
      );
    }
    GetTerminal()->CreateLink(FileName, PointTo, SymbolicLink);
    if (UpdatePanel())
    {
      RedrawPanel();
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TemporarilyDownloadFiles(
  TStrings * FileList, TCopyParamType CopyParam, UnicodeString & TempDir)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);

  TempDir = WinSCPPlugin()->TemporaryDir();
  if (TempDir.IsEmpty() || !ForceDirectories(TempDir))
  {
    throw Exception(FMTLOAD(CREATE_TEMP_DIR_ERROR, TempDir.c_str()));
  }

  FTerminal->SetExceptionOnFail(true);
  TRY_FINALLY (
  {
    try
    {
      FTerminal->CopyToLocal(FileList, TempDir, &CopyParam, cpTemporary);
    }
    catch(...)
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
  ,
  {
    FTerminal->SetExceptionOnFail(false);
  }
  );
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ApplyCommand()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  if (FileList != NULL)
  {
    std::auto_ptr<TStrings> FileListPtr;
    FileListPtr.reset(FileList);
    {
      intptr_t Params = FarConfiguration->GetApplyCommandParams();
      UnicodeString Command = FarConfiguration->GetApplyCommandCommand();
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
              WinSCPPlugin(), &RemoteCustomCommand);

            Command = InteractiveCustomCommand.Complete(Command, false);

            TRY_FINALLY (
            {
              TCaptureOutputEvent OutputEvent = NULL;
              FOutputLog = false;
              if (FLAGSET(Params, ccShowResults))
              {
                assert(!FNoProgress);
                FNoProgress = true;
                FOutputLog = true;
                OutputEvent = MAKE_CALLBACK(TWinSCPFileSystem::TerminalCaptureLog, this);
              }

              if (FLAGSET(Params, ccCopyResults))
              {
                assert(FCapturedLog == NULL);
                FCapturedLog = new TStringList();
                OutputEvent = MAKE_CALLBACK(TWinSCPFileSystem::TerminalCaptureLog, this);
              }
              TRY_FINALLY (
              {
                if (FLAGSET(Params, ccShowResults))
                {
                  WinSCPPlugin()->ShowTerminalScreen();
                }

                FTerminal->CustomCommandOnFiles(Command, Params, FileList, OutputEvent);
              }
              ,
              {
                if (FLAGSET(Params, ccShowResults))
                {
                  FNoProgress = false;
                  WinSCPPlugin()->ScrollTerminalScreen(1);
                  WinSCPPlugin()->SaveTerminalScreen();
                }

                if (FLAGSET(Params, ccCopyResults))
                {
                  WinSCPPlugin()->FarCopyToClipboard(FCapturedLog);
                  SAFE_DESTROY(FCapturedLog);
                }
              }
              );
            }
            ,
            {
              GetPanelInfo()->ApplySelection();
              if (UpdatePanel())
              {
                RedrawPanel();
              }
            }
            );
          }
        }
        else
        {
          TCustomCommandData Data1(GetTerminal());
          TLocalCustomCommand LocalCustomCommand(Data1, GetTerminal()->GetCurrentDirectory());
          TFarInteractiveCustomCommand InteractiveCustomCommand(WinSCPPlugin(),
              &LocalCustomCommand);

          Command = InteractiveCustomCommand.Complete(Command, false);

          TStrings * LocalFileList = NULL;
          TStrings * RemoteFileList = NULL;
          {
            bool FileListCommand = LocalCustomCommand.IsFileListCommand(Command);
            bool LocalFileCommand = LocalCustomCommand.HasLocalFileName(Command);

            if (LocalFileCommand)
            {
              TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
              RequireLocalPanel(AnotherPanel, GetMsg(APPLY_COMMAND_LOCAL_PATH_REQUIRED));

              LocalFileList = CreateSelectedFileList(osLocal, AnotherPanel);
              std::auto_ptr<TStrings> LocalFileListPtr;
              LocalFileListPtr.reset(LocalFileList);

              if (FileListCommand)
              {
                if ((LocalFileList == NULL) || (LocalFileList->GetCount() != 1))
                {
                  throw Exception(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH1));
                }
              }
              else
              {
                if ((LocalFileList == NULL) ||
                    ((LocalFileList->GetCount() != 1) &&
                     (FileList->GetCount() != 1) &&
                     (LocalFileList->GetCount() != FileList->GetCount())))
                {
                  throw Exception(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH));
                }
              }
            }

            UnicodeString TempDir;

            TemporarilyDownloadFiles(FileList, GUIConfiguration->GetDefaultCopyParam(), TempDir);

            TRY_FINALLY (
            {
              RemoteFileList = new TStringList();
              std::auto_ptr<TStrings> RemoteFileListPtr;
              RemoteFileListPtr.reset(RemoteFileList);

              TMakeLocalFileListParams MakeFileListParam;
              MakeFileListParam.FileList = RemoteFileList;
              MakeFileListParam.IncludeDirs = FLAGSET(Params, ccApplyToDirectories);
              MakeFileListParam.Recursive =
                FLAGSET(Params, ccRecursive) && !FileListCommand;

              ProcessLocalDirectory(TempDir, MAKE_CALLBACK(TTerminal::MakeLocalFileList, FTerminal), &MakeFileListParam);

              TFileOperationProgressType Progress(MAKE_CALLBACK(TWinSCPFileSystem::OperationProgress, this), MAKE_CALLBACK(TWinSCPFileSystem::OperationFinished, this));

              Progress.Start(foCustomCommand, osRemote, static_cast<intptr_t>(FileListCommand ? 1 : FileList->GetCount()));
              TRY_FINALLY (
              {
                if (FileListCommand)
                {
                  UnicodeString LocalFile;
                  UnicodeString FileList = MakeFileList(RemoteFileList);

                  if (LocalFileCommand)
                  {
                    assert(LocalFileList->GetCount() == 1);
                    LocalFile = LocalFileList->GetString(0);
                  }

                  TCustomCommandData Data2(FTerminal);
                  TLocalCustomCommand CustomCommand(Data2,
                    GetTerminal()->GetCurrentDirectory(), L"", LocalFile, FileList);
                  ExecuteShellAndWait(WinSCPPlugin()->GetHandle(), CustomCommand.Complete(Command, true),
                    TProcessMessagesEvent());
                }
                else if (LocalFileCommand)
                {
                  if (LocalFileList->GetCount() == 1)
                  {
                    UnicodeString LocalFile = LocalFileList->GetString(0);

                    for (intptr_t Index = 0; Index < RemoteFileList->GetCount(); ++Index)
                    {
                      UnicodeString FileName = RemoteFileList->GetString(Index);
                      TCustomCommandData Data3(FTerminal);
                      TLocalCustomCommand CustomCommand(Data3,
                        GetTerminal()->GetCurrentDirectory(), FileName, LocalFile, L"");
                      ExecuteShellAndWait(WinSCPPlugin()->GetHandle(),
                        CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                    }
                  }
                  else if (RemoteFileList->GetCount() == 1)
                  {
                    UnicodeString FileName = RemoteFileList->GetString(0);

                    for (intptr_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
                    {
                      TCustomCommandData Data4(FTerminal);
                      TLocalCustomCommand CustomCommand(
                        Data4, GetTerminal()->GetCurrentDirectory(),
                        FileName, LocalFileList->GetString(Index), L"");
                      ExecuteShellAndWait(WinSCPPlugin()->GetHandle(),
                        CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                    }
                  }
                  else
                  {
                    if (LocalFileList->GetCount() != RemoteFileList->GetCount())
                    {
                      throw Exception(GetMsg(CUSTOM_COMMAND_PAIRS_DOWNLOAD_FAILED));
                    }

                    for (intptr_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
                    {
                      UnicodeString FileName = RemoteFileList->GetString(Index);
                      TCustomCommandData Data5(FTerminal);
                      TLocalCustomCommand CustomCommand(
                        Data5, GetTerminal()->GetCurrentDirectory(),
                        FileName, LocalFileList->GetString(Index), L"");
                      ExecuteShellAndWait(WinSCPPlugin()->GetHandle(),
                        CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                    }
                  }
                }
                else
                {
                  for (intptr_t Index = 0; Index < RemoteFileList->GetCount(); ++Index)
                  {
                    TCustomCommandData Data6(FTerminal);
                    TLocalCustomCommand CustomCommand(Data6,
                      GetTerminal()->GetCurrentDirectory(), RemoteFileList->GetString(Index), L"", L"");
                    ExecuteShellAndWait(WinSCPPlugin()->GetHandle(),
                      CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                  }
                }
              }
              ,
              {
                Progress.Stop();
              }
              );
            }
            ,
            {
              RecursiveDeleteFile(ExcludeTrailingBackslash(TempDir), false);
            }
            );
          }
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::Synchronize(const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, TTerminal::TSynchronizeMode Mode,
  const TCopyParamType & CopyParam, intptr_t Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options)
{
  TSynchronizeChecklist * AChecklist = NULL;
  TRY_FINALLY (
  {
    WinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
    WinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = true;
    TRY_FINALLY (
    {
      AChecklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
        Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
        MAKE_CALLBACK(TWinSCPFileSystem::TerminalSynchronizeDirectory, this), Options);
    }
    ,
    {
      WinSCPPlugin()->ClearConsoleTitle();
      WinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
    }
    );

    WinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
    WinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = false;
    TRY_FINALLY (
    {
      FTerminal->SynchronizeApply(AChecklist, LocalDirectory, RemoteDirectory,
        &CopyParam, Params | TTerminal::spNoConfirmation,
        MAKE_CALLBACK(TWinSCPFileSystem::TerminalSynchronizeDirectory, this));
    }
    ,
    {
      WinSCPPlugin()->ClearConsoleTitle();
      WinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
    }
    );
  }
  ,
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
  );
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeAllowSelectedOnly()
{
  return
    (GetPanelInfo()->GetSelectedCount() > 0) ||
    (GetAnotherPanelInfo()->GetSelectedCount() > 0);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::GetSynchronizeOptions(
  intptr_t Params, TSynchronizeOptions & Options)
{
  if (FLAGSET(Params, spSelectedOnly) && SynchronizeAllowSelectedOnly())
  {
    Options.Filter = new TStringList();
    Options.Filter->CaseSensitive = false;
    Options.Filter->Duplicates = dupAccept;

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
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::FullSynchronize(bool Source)
{
  TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  UnicodeString LocalDirectory = AnotherPanel->GetCurrentDirectory();
  UnicodeString RemoteDirectory = FTerminal->GetCurrentDirectory();

  bool SaveMode = !(GUIConfiguration->GetSynchronizeModeAuto() < 0);
  TTerminal::TSynchronizeMode Mode =
    (SaveMode ? (TTerminal::TSynchronizeMode)GUIConfiguration->GetSynchronizeModeAuto() :
     (Source ? TTerminal::smLocal : TTerminal::smRemote));
  intptr_t Params = GUIConfiguration->GetSynchronizeParams();
  bool SaveSettings = false;

  TGUICopyParamType CopyParam = GUIConfiguration->GetDefaultCopyParam();
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
    TRY_FINALLY (
    {
      WinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
      WinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
      FSynchronizationStart = Now();
      FSynchronizationCompare = true;
      TRY_FINALLY (
      {
        Checklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
          Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
          MAKE_CALLBACK(TWinSCPFileSystem::TerminalSynchronizeDirectory, this), &SynchronizeOptions);
      }
      ,
      {
        WinSCPPlugin()->ClearConsoleTitle();
        WinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
      }
      );

      if (Checklist && Checklist->GetCount() == 0)
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
        WinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
        WinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
        FSynchronizationStart = Now();
        FSynchronizationCompare = false;
        TRY_FINALLY (
        {
          FTerminal->SynchronizeApply(Checklist, LocalDirectory, RemoteDirectory,
            &CopyParam, Params | TTerminal::spNoConfirmation,
            MAKE_CALLBACK(TWinSCPFileSystem::TerminalSynchronizeDirectory, this));
        }
        ,
        {
          WinSCPPlugin()->ClearConsoleTitle();
          WinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
        }
        );
      }
    }
    ,
    {
      delete Checklist;
      if (UpdatePanel())
      {
        RedrawPanel();
      }
    }
    );
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalSynchronizeDirectory(
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  bool & Continue, bool Collect)
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    static const intptr_t ProgressWidth = 48;
    static UnicodeString ProgressTitle;
    static UnicodeString ProgressTitleCompare;
    static UnicodeString LocalLabel;
    static UnicodeString RemoteLabel;
    static UnicodeString StartTimeLabel;
    static UnicodeString TimeElapsedLabel;

    if (ProgressTitle.IsEmpty())
    {
      ProgressTitle = GetMsg(SYNCHRONIZE_PROGRESS_TITLE);
      ProgressTitleCompare = GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE);
      LocalLabel = GetMsg(SYNCHRONIZE_PROGRESS_LOCAL);
      RemoteLabel = GetMsg(SYNCHRONIZE_PROGRESS_REMOTE);
      StartTimeLabel = GetMsg(SYNCHRONIZE_PROGRESS_START_TIME);
      TimeElapsedLabel = GetMsg(SYNCHRONIZE_PROGRESS_ELAPSED);
    }

    UnicodeString Message;

    Message = LocalLabel + MinimizeName(LocalDirectory,
      ProgressWidth - LocalLabel.Length(), false);
    Message += ::StringOfChar(L' ', ProgressWidth - Message.Length()) + L"\n";
    Message += RemoteLabel + MinimizeName(RemoteDirectory,
      ProgressWidth - RemoteLabel.Length(), true) + L"\n";
    Message += StartTimeLabel + FSynchronizationStart.TimeString() + L"\n";
    Message += TimeElapsedLabel +
      FormatDateTimeSpan(GetConfiguration()->GetTimeFormat(), TDateTime(Now() - FSynchronizationStart)) + L"\n";

    WinSCPPlugin()->Message(0, (Collect ? ProgressTitleCompare : ProgressTitle), Message);

    if (WinSCPPlugin()->CheckForEsc() &&
        (MoreMessageDialog(GetMsg(CANCEL_OPERATION), NULL,
          qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      Continue = false;
    }
  }
}
//------------------------------------------------------------------------------
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
  TSynchronizeController Controller(
    MAKE_CALLBACK(TWinSCPFileSystem::DoSynchronize, this),
    MAKE_CALLBACK(TWinSCPFileSystem::DoSynchronizeInvalid, this),
    MAKE_CALLBACK(TWinSCPFileSystem::DoSynchronizeTooManyDirectories, this));
  assert(FSynchronizeController == NULL);
  FSynchronizeController = &Controller;

  TRY_FINALLY (
  {
    bool SaveSettings = false;
    TCopyParamType CopyParam = GUIConfiguration->GetDefaultCopyParam();
    intptr_t CopyParamAttrs = GetTerminal()->UsableCopyParamAttrs(0).Upload;
    uintptr_t Options =
      FLAGMASK(SynchronizeAllowSelectedOnly(), soAllowSelectedOnly);
    if (SynchronizeDialog(Params, &CopyParam,
        MAKE_CALLBACK(TSynchronizeController::StartStop, &Controller),
        SaveSettings, Options, CopyParamAttrs,
        MAKE_CALLBACK(TWinSCPFileSystem::GetSynchronizeOptions, this)) &&
        SaveSettings)
    {
      GUIConfiguration->SetSynchronizeParams(Params.Params | UnusedParams);
      GUIConfiguration->SetSynchronizeOptions(Params.Options);
    }
  }
  ,
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
  );
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronize(
  TSynchronizeController * /*Sender*/, const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, const TCopyParamType & CopyParam,
  const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options, bool Full)
{
  try
  {
    intptr_t PParams = Params.Params;
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
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    throw;
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, const UnicodeString & Directory,
  const UnicodeString & /* ErrorStr */)
{
  UnicodeString Message;
  if (!Directory.IsEmpty())
  {
    Message = FORMAT(GetMsg(WATCH_ERROR_DIRECTORY).c_str(), Directory.c_str());
  }
  else
  {
    Message = GetMsg(WATCH_ERROR_GENERAL);
  }

  MoreMessageDialog(Message, NULL, qtError, qaOK);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::DoSynchronizeTooManyDirectories(
  TSynchronizeController * /*Sender*/, intptr_t & MaxDirectories)
{
  if (MaxDirectories < GUIConfiguration->GetMaxWatchDirectories())
  {
    MaxDirectories = GUIConfiguration->GetMaxWatchDirectories();
  }
  else
  {
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    uintptr_t Result = MoreMessageDialog(
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::CustomCommandGetParamValue(
  const UnicodeString & AName, UnicodeString & Value)
{
  UnicodeString Name = AName;
  if (Name.IsEmpty())
  {
    Name = GetMsg(APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!WinSCPPlugin()->InputBox(GetMsg(APPLY_COMMAND_PARAM_TITLE),
        Name, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}
//------------------------------------------------------------------------------
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
      {
        std::auto_ptr<TStrings> FileListPtr;
        FileListPtr.reset(FileList);
        UnicodeString Target = FTerminal->GetCurrentDirectory();
        UnicodeString FileMask = L"*.*";
        if (RemoteTransferDialog(FileList, Target, FileMask, Move))
        {
          TRY_FINALLY (
          {
            if (Move)
            {
              GetTerminal()->MoveFiles(FileList, Target, FileMask);
            }
            else
            {
              GetTerminal()->CopyFiles(FileList, Target, FileMask);
            }
          }
          ,
          {
            GetPanelInfo()->ApplySelection();
            if (UpdatePanel())
            {
              RedrawPanel();
            }
          }
          );
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::RenameFile()
{
  TFarPanelItem * PanelItem = GetPanelInfo()->GetFocusedItem();
  assert(PanelItem != NULL);

  if (!PanelItem->GetIsParentDirectory())
  {
    RequireCapability(fcRename);

    TRemoteFile * File = static_cast<TRemoteFile *>(PanelItem->GetUserData());
    UnicodeString NewName = File->GetFileName();
    if (RenameFileDialog(File, NewName))
    {
      TRY_FINALLY (
      {
        GetTerminal()->RenameFile(File, NewName, true);
      }
      ,
      {
        if (UpdatePanel())
        {
          RedrawPanel();
        }
      }
      );
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::FileProperties()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  if (FileList)
  {
    assert(!FPanelItems);
    {
      std::auto_ptr<TStrings> FileListPtr;
      FileListPtr.reset(FileList);
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
        if (FTerminal->GetIsCapable(fcModeChanging))
        {
          Flags |= cpMode;
        }
        if (FTerminal->GetIsCapable(fcOwnerChanging))
        {
          Flags |= cpOwner;
        }
        if (FTerminal->GetIsCapable(fcGroupChanging))
        {
          Flags |= cpGroup;
        }

        TRemoteProperties NewProperties = CurrentProperties;
        if (PropertiesDialog(FileList, FTerminal->GetCurrentDirectory(),
            FTerminal->GetGroups(), FTerminal->GetUsers(), &NewProperties, Flags))
        {
          NewProperties = TRemoteProperties::ChangedProperties(CurrentProperties,
            NewProperties);
          TRY_FINALLY (
          {
            FTerminal->ChangeFilesProperties(FileList, &NewProperties);
          }
          ,
          {
            GetPanelInfo()->ApplySelection();
            if (UpdatePanel())
            {
              RedrawPanel();
            }
          }
          );
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::InsertTokenOnCommandLine(const UnicodeString & Token, bool Separate)
{
  UnicodeString Token2 = Token;
  if (!Token2.IsEmpty())
  {
    if (Token2.Pos(L' ') > 0)
    {
      Token2 = FORMAT(L"\"%s\"", Token2.c_str());
    }

    if (Separate)
    {
      Token2 += L" ";
    }

    FarControl(FCTL_INSERTCMDLINE, 0, reinterpret_cast<void *>(const_cast<wchar_t *>(Token2.c_str())));
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::InsertSessionNameOnCommandLine()
{
  TFarPanelItem * Focused = GetPanelInfo()->GetFocusedItem();

  if (Focused != NULL)
  {
    TSessionData * SessionData = reinterpret_cast<TSessionData *>(Focused->GetUserData());
    UnicodeString Name;
    if (SessionData != NULL)
    {
      Name = SessionData->GetName();
    }
    else
    {
      Name = ::UnixIncludeTrailingBackslash(FSessionsFolder);
      if (!Focused->GetIsParentDirectory())
      {
        Name = ::UnixIncludeTrailingBackslash(Name + Focused->GetFileName());
      }
    }
    InsertTokenOnCommandLine(Name, true);
  }
}
//------------------------------------------------------------------------------
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
        UnicodeString Path;
        if (Full)
        {
          // Get full address (with server address)
          UnicodeString SessionUrl = GetSessionUrl(FTerminal);
          Path = FORMAT(L"%s%s", SessionUrl.c_str(), File->GetFullFileName().c_str());
        }
        else
        {
          Path = File->GetFileName();
        }
        InsertTokenOnCommandLine(Path, true);
      }
    }
    else
    {
      InsertTokenOnCommandLine(::UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory()), true);
    }
  }
}
//------------------------------------------------------------------------------
// not used
void TWinSCPFileSystem::InsertPathOnCommandLine()
{
  InsertTokenOnCommandLine(FTerminal->GetCurrentDirectory(), false);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::CopyFullFileNamesToClipboard()
{
  TStrings * FileList = CreateSelectedFileList(osRemote);
  TStrings * FileNames = new TStringList();
  {
    std::auto_ptr<TStrings> FileListPtr;
    FileListPtr.reset(FileList);
    std::auto_ptr<TStrings> FileNamesPtr;
    FileNamesPtr.reset(FileNames);
    if (FileList != NULL)
    {
      for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
      {
        TRemoteFile * File = reinterpret_cast<TRemoteFile *>(FileList->Objects[Index]);
        if (File != NULL)
        {
          FileNames->Add(File->GetFileName());
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
        FileNames->Add(::UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory()));
      }
    }

    WinSCPPlugin()->FarCopyToClipboard(FileNames);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::GetSpaceAvailable(const UnicodeString & Path,
  TSpaceAvailable & ASpaceAvailable, bool & Close)
{
  // terminal can be already closed (e.g. dropped connection)
  if ((GetTerminal() != NULL) && GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    try
    {
      GetTerminal()->SpaceAvailable(Path, ASpaceAvailable);
    }
    catch (Exception & E)
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ShowInformation()
{
  const TSessionInfo & SessionInfo = GetTerminal()->GetSessionInfo();
  TFileSystemInfo FileSystemInfo = GetTerminal()->GetFileSystemInfo();
  TGetSpaceAvailableEvent OnGetSpaceAvailable;
  if (GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    OnGetSpaceAvailable = MAKE_CALLBACK(TWinSCPFileSystem::GetSpaceAvailable, this);
  }
  FileSystemInfoDialog(SessionInfo, FileSystemInfo, GetTerminal()->GetCurrentDirectory(),
    OnGetSpaceAvailable);
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::AreCachesEmpty()
{
  assert(Connected());
  return FTerminal->GetAreCachesEmpty();
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ClearCaches()
{
  assert(Connected());
  FTerminal->ClearCaches();
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::OpenSessionInPutty()
{
  assert(Connected());
  ::OpenSessionInPutty(GUIConfiguration->GetPuttyPath(), FTerminal->GetSessionData(),
    GUIConfiguration->GetPuttyPassword() ? GetTerminal()->GetPassword() : UnicodeString());
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::QueueShow(bool ClosingPlugin)
{
  assert(Connected());
  assert(FQueueStatus != NULL);
  QueueDialog(FQueueStatus, ClosingPlugin);
  ProcessQueue(true);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::OpenDirectory(bool Add)
{
  TBookmarkList * BookmarkList = new TBookmarkList();
  {
    std::auto_ptr<TBookmarkList> BookmarkListPtr;
    BookmarkListPtr.reset(BookmarkList);
    UnicodeString Directory = FTerminal->GetCurrentDirectory();
    UnicodeString SessionKey = FTerminal->GetSessionData()->GetSessionKey();
    TBookmarkList * CurrentBookmarkList;

    CurrentBookmarkList = FarConfiguration->GetBookmarks(SessionKey);
    if (CurrentBookmarkList != NULL)
    {
      BookmarkList->Assign(CurrentBookmarkList);
    }

    if (Add)
    {
      TBookmark * Bookmark = new TBookmark;
      Bookmark->SetRemote(Directory);
      Bookmark->SetName(Directory);
      BookmarkList->Add(Bookmark);
      FarConfiguration->SetBookmarks(SessionKey, BookmarkList);
    }

    bool Result = OpenDirectoryDialog(Add, Directory, BookmarkList);

    FarConfiguration->SetBookmarks(SessionKey, BookmarkList);

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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::HomeDirectory()
{
  FTerminal->HomeDirectory();
  if (UpdatePanel(true))
  {
    RedrawPanel();
  }
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::IsSynchronizedBrowsing()
{
  return FSynchronisingBrowse;
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ToggleSynchronizeBrowsing()
{
  FSynchronisingBrowse = !FSynchronisingBrowse;

  if (FarConfiguration->GetConfirmSynchronizedBrowsing())
  {
    UnicodeString Message = FSynchronisingBrowse ?
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
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeBrowsing(const UnicodeString & NewPath)
{
  bool Result;
  TFarPanelInfo * AnotherPanel = GetAnotherPanelInfo();
  UnicodeString OldPath = AnotherPanel->GetCurrentDirectory();
  // IncludeTrailingBackslash to expand C: to C:\.
  UnicodeString LocalPath = IncludeTrailingBackslash(NewPath);
  FarPanelDirectory fpd;
  memset(&fpd, 0, sizeof(fpd));
  fpd.StructSize = sizeof(fpd);
  fpd.Name = LocalPath.c_str();
  if (!FarControl(FCTL_SETPANELDIRECTORY, 0, &fpd, reinterpret_cast<HANDLE>(PANEL_PASSIVE)))
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
      // If FCTL_SETPANELDIR above fails, Far default current
      // directory to initial (?) one. So move this back to
      // previous directory.
      FarPanelDirectory fpd;
      memset(&fpd, 0, sizeof(fpd));
      fpd.StructSize = sizeof(fpd);
      fpd.Name = OldPath.c_str();
      FarControl(FCTL_SETPANELDIRECTORY, sizeof(fpd), &fpd, reinterpret_cast<HANDLE>(PANEL_PASSIVE));
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
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::SetDirectoryEx(const UnicodeString & Dir, int OpMode)
{
  if (!SessionList() && !Connected())
  {
    return false;
  }
  // FAR WORKAROUND
  // workaround to ignore "change to root directory" command issued by FAR,
  // before file is opened for viewing/editing from "find file" dialog
  // when plugin uses UNIX style paths
  else if ((OpMode & OPM_FIND) && (OpMode & OPM_SILENT) && (Dir == L"\\"))
  {
    if (FSavedFindFolder.IsEmpty())
    {
      return true;
    }
    else
    {
      bool Result = false;
      TRY_FINALLY (
      {
        // Result = SetDirectoryEx(Dir, OpMode);
        Result = SetDirectoryEx(FSavedFindFolder, OpMode);
      }
      ,
      {
        FSavedFindFolder = "";
      }
      );
      return Result;
    }
  }
  else
  {
    if ((OpMode & OPM_FIND) && FSavedFindFolder.IsEmpty())
    {
      FSavedFindFolder = FTerminal->GetCurrentDirectory();
    }

    if (SessionList())
    {
      FSessionsFolder = AbsolutePath(L"/" + FSessionsFolder, Dir);
      assert(FSessionsFolder[1] == L'/');
      FSessionsFolder.Delete(1, 1);
      FNewSessionsFolder = L"";
    }
    else
    {
      assert(!FNoProgress);
      bool Normal = FLAGCLEAR(OpMode, OPM_FIND | OPM_SILENT);
      UnicodeString PrevPath = FTerminal->GetCurrentDirectory();
      FNoProgress = !Normal;
      if (!FNoProgress)
      {
        WinSCPPlugin()->ShowConsoleTitle(GetMsg(CHANGING_DIRECTORY_TITLE));
      }
      FTerminal->SetExceptionOnFail(true);
      TRY_FINALLY (
      {
        if (Dir == L"\\")
        {
          FTerminal->ChangeDirectory(ROOTDIRECTORY);
        }
        else if ((Dir == PARENTDIRECTORY) && (FTerminal->GetCurrentDirectory() == ROOTDIRECTORY))
        {
          // ClosePanel();
          Disconnect();
        }
        else
        {
          FTerminal->ChangeDirectory(Dir);
        }
      }
      ,
      {
        if (FTerminal)
        {
          FTerminal->SetExceptionOnFail(false);
        }
        if (!FNoProgress)
        {
          WinSCPPlugin()->ClearConsoleTitle();
        }
        FNoProgress = false;
      }
      );

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
            UnicodeString RemotePath = ::UnixIncludeTrailingBackslash(FTerminal->GetCurrentDirectory());
            UnicodeString FullPrevPath = ::UnixIncludeTrailingBackslash(PrevPath);
            UnicodeString ALocalPath;
            if (RemotePath.SubString(1, FullPrevPath.Length()) == FullPrevPath)
            {
              ALocalPath = IncludeTrailingBackslash(AnotherPanel->GetCurrentDirectory()) +
                FromUnixPath(RemotePath.SubString(FullPrevPath.Length() + 1,
                  RemotePath.Length() - FullPrevPath.Length()));
            }
            else if (FullPrevPath.SubString(1, RemotePath.Length()) == RemotePath)
            {
              UnicodeString NewLocalPath;
              ALocalPath = ExcludeTrailingBackslash(AnotherPanel->GetCurrentDirectory());
              while (!::UnixComparePaths(FullPrevPath, RemotePath))
              {
                NewLocalPath = ExcludeTrailingBackslash(ExtractFileDir(ALocalPath));
                if (NewLocalPath == ALocalPath)
                {
                  Abort();
                }
                ALocalPath = NewLocalPath;
                FullPrevPath = ::UnixExtractFilePath(::UnixExcludeTrailingBackslash(FullPrevPath));
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
          catch(Exception & E)
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
//------------------------------------------------------------------------------
intptr_t TWinSCPFileSystem::MakeDirectoryEx(UnicodeString & Name, int OpMode)
{
  if (Connected())
  {
    assert(!(OpMode & OPM_SILENT) || !Name.IsEmpty());

    TRemoteProperties Properties = GUIConfiguration->GetNewDirectoryProperties();
    bool SaveSettings = false;

    if ((OpMode & OPM_SILENT) ||
        CreateDirectoryDialog(Name, &Properties, SaveSettings))
    {
      if (SaveSettings)
      {
        GUIConfiguration->SetNewDirectoryProperties(Properties);
      }

      WinSCPPlugin()->ShowConsoleTitle(GetMsg(CREATING_FOLDER));
      TRY_FINALLY (
      {
        FTerminal->CreateDirectory(Name, &Properties);
      }
      ,
      {
        WinSCPPlugin()->ClearConsoleTitle();
      }
      );
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
    assert(!(OpMode & OPM_SILENT) || !Name.IsEmpty());

    if (((OpMode & OPM_SILENT) ||
         WinSCPPlugin()->InputBox(GetMsg(CREATE_FOLDER_TITLE),
           StripHotkey(GetMsg(CREATE_FOLDER_PROMPT)),
           Name, 0, MAKE_SESSION_FOLDER_HISTORY)) &&
        !Name.IsEmpty())
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::DeleteSession(TSessionData * Data, void * /*Param*/)
{
  Data->Remove();
  StoredSessions->Remove(Data);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ProcessSessions(TObjectList * PanelItems,
  TProcessSessionEvent ProcessSession, void * Param)
{
  for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = static_cast<TFarPanelItem *>(PanelItems->GetItem(Index));
    assert(PanelItem);
    if (PanelItem->GetIsFile())
    {
      if (PanelItem->GetUserData() != NULL)
      {
        ProcessSession(static_cast<TSessionData *>(PanelItem->GetUserData()), Param);
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
      UnicodeString Folder = ::UnixIncludeTrailingBackslash(
        ::UnixIncludeTrailingBackslash(FSessionsFolder) + PanelItem->GetFileName());
      intptr_t Index2 = 0;
      while (Index2 < StoredSessions->GetCount())
      {
        TSessionData * Data = StoredSessions->GetSession(Index2);
        if (Data->GetName().SubString(1, Folder.Length()) == Folder)
        {
          ProcessSession(Data, Param);
          if ((Index2 < StoredSessions->GetCount()) && StoredSessions->GetSession(Index2) != Data)
          {
            Index2--;
          }
        }
        Index2++;
      }
      PanelItem->SetSelected(false);
    }
  }
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::DeleteFilesEx(TObjectList * PanelItems, int OpMode)
{
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osRemote);
    FPanelItems = PanelItems;
    TRY_FINALLY (
    {
      UnicodeString Query;
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
          static_cast<TFarPanelItem *>(PanelItems->GetItem(0))->GetFileName().c_str());
      }

      if ((OpMode & OPM_SILENT) || !FarConfiguration->GetConfirmDeleting() ||
          (MoreMessageDialog(Query, NULL, qtConfirmation, qaOK | qaCancel) == qaOK))
      {
        FTerminal->DeleteFiles(FFileList);
      }
    }
    ,
    {
      FPanelItems = NULL;
      SAFE_DESTROY(FFileList);
    }
    );
    return true;
  }
  else if (SessionList())
  {
    if ((OpMode & OPM_SILENT) || !FarConfiguration->GetConfirmDeleting() ||
        (MoreMessageDialog(GetMsg(DELETE_SESSIONS_CONFIRM), NULL, qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      ProcessSessions(PanelItems, MAKE_CALLBACK(TWinSCPFileSystem::DeleteSession, this), NULL);
    }
    return true;
  }
  else
  {
    return false;
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::QueueAddItem(TQueueItem * Item)
{
  FarConfiguration->CacheFarSettings();
  FQueue->AddItem(Item);
}
//------------------------------------------------------------------------------
struct TExportSessionParam
{
  UnicodeString DestPath;
};
//------------------------------------------------------------------------------
intptr_t TWinSCPFileSystem::GetFilesEx(TObjectList * PanelItems, bool Move,
  UnicodeString & DestPath, int OpMode)
{
  intptr_t Result;
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osRemote);
    TRY_FINALLY (
    {
      bool EditView = (OpMode & (OPM_EDIT | OPM_VIEW)) != 0;
      bool Confirmed =
        (OpMode & OPM_SILENT) &&
        (!EditView || FarConfiguration->GetEditorDownloadDefaultMode());

      TGUICopyParamType CopyParam = GUIConfiguration->GetDefaultCopyParam();
      if (EditView)
      {
        EditViewCopyParam(CopyParam);
      }

      // these parameters are known in advance
      intptr_t Params =
        FLAGMASK(Move, cpDelete);

      if (!Confirmed)
      {
        intptr_t CopyParamAttrs =
          GetTerminal()->UsableCopyParamAttrs(Params).Download |
          FLAGMASK(EditView, cpaNoExcludeMask);
        uintptr_t Options =
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
            DestPath, &CopyParam, Params, false));
          Confirmed = false;
        }
      }

      if (Confirmed)
      {
        if ((FFileList->GetCount() == 1) && (OpMode & OPM_EDIT))
        {
          FOriginalEditFile = IncludeTrailingBackslash(DestPath) +
            ::UnixExtractFileName(FFileList->GetString(0));
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
    ,
    {
      FPanelItems = NULL;
      SAFE_DESTROY(FFileList);
    }
    );
  }
  else if (SessionList())
  {
    UnicodeString Title = GetMsg(EXPORT_SESSION_TITLE);
    UnicodeString Prompt;
    if (PanelItems->GetCount() == 1)
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSION_PROMPT).c_str(),
        static_cast<TFarPanelItem *>(PanelItems->GetItem(0))->GetFileName().c_str());
    }
    else
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSIONS_PROMPT).c_str(), PanelItems->GetCount());
    }

    bool AResult = (OpMode & OPM_SILENT) ||
      WinSCPPlugin()->InputBox(Title, Prompt, DestPath, 0, L"Copy");
    if (AResult)
    {
      TExportSessionParam Param;
      Param.DestPath = DestPath;
      ProcessSessions(PanelItems, MAKE_CALLBACK(TWinSCPFileSystem::ExportSession, this), &Param);
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ExportSession(TSessionData * Data, void * AParam)
{
  TExportSessionParam & Param = *static_cast<TExportSessionParam *>(AParam);

  TSessionData * ExportData = new TSessionData(Data->GetName());
  std::auto_ptr<TSessionData> ExportDataPtr;
  ExportDataPtr.reset(ExportData);
  TSessionData * FactoryDefaults = new TSessionData(L"");
  std::auto_ptr<TSessionData> FactoryDefaultsPtr;
  FactoryDefaultsPtr.reset(FactoryDefaults);
  ExportData->Assign(Data);
  ExportData->SetModified(true);
  // TCopyParamType & CopyParam = GUIConfiguration->GetDefaultCopyParam();
  UnicodeString XmlFileName = IncludeTrailingBackslash(Param.DestPath) +
    ::ValidLocalFileName(::ExtractFilename(ExportData->GetName())) + L".netbox";
  THierarchicalStorage * ExportStorage = new TXmlStorage(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey());
  ExportStorage->Init();
  std::auto_ptr<THierarchicalStorage> ExportStoragePtr;
  ExportStoragePtr.reset(ExportStorage);
  ExportStorage->SetAccessMode(smReadWrite);
  {
    if (ExportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
    {
      ExportData->Save(ExportStorage, false, FactoryDefaults);
    }
  }
}
//------------------------------------------------------------------------------
intptr_t TWinSCPFileSystem::UploadFiles(bool Move, int OpMode, bool Edit,
  UnicodeString & DestPath)
{
  intptr_t Result = 1;
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
  intptr_t Params =
    FLAGMASK(Move, cpDelete);

  if (!Confirmed)
  {
    intptr_t CopyParamAttrs =
      GetTerminal()->UsableCopyParamAttrs(Params).Upload |
      FLAGMASK(Edit, (cpaNoExcludeMask | cpaNoClearArchive));
    // heuristics: do not ask for target directory when uploaded file
    // was downloaded in edit mode
    uintptr_t Options =
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
        DestPath, &CopyParam, Params, false));
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
    TRY_FINALLY (
    {
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(!Ask, cpNoConfirmation) |
        FLAGMASK(Edit, cpTemporary) |
        FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly)
        ;
      FTerminal->CopyToRemote(FFileList, DestPath, &CopyParam, Params);
    }
    ,
    {
      FNoProgressFinish = false;
    }
    );
  }
  else
  {
    Result = -1;
  }
  return Result;
}
//------------------------------------------------------------------------------
intptr_t TWinSCPFileSystem::PutFilesEx(TObjectList * PanelItems, bool Move, int OpMode)
{
  int Result;
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osLocal);
    TRY_FINALLY (
    {
      FPanelItems = PanelItems;

      // if file is saved under different name, FAR tries to upload original file,
      // but let's be robust and check for new name, in case it changes.
      // OPM_EDIT is set since 1.70 final, only.
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
          FFileList->SetString(0, FLastEditFile);

          FOriginalEditFile = L"";
          FLastEditFile = L"";

          UnicodeString CurrentDirectory = FTerminal->GetCurrentDirectory();
          Result = UploadFiles(Move, OpMode, true, CurrentDirectory);
          FTerminal->SetCurrentDirectory(CurrentDirectory);
        }
      }
      else
      {
        UnicodeString CurrentDirectory = FTerminal->GetCurrentDirectory();
        Result = UploadFiles(Move, OpMode, false, CurrentDirectory);
        FTerminal->SetCurrentDirectory(CurrentDirectory);
      }
    }
    ,
    {
      FPanelItems = NULL;
      SAFE_DESTROY(FFileList);
    }
    );
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
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::ImportSessions(TObjectList * PanelItems, bool /*Move*/,
  int OpMode)
{
  bool Result = (OpMode & OPM_SILENT) ||
    (MoreMessageDialog(GetMsg(IMPORT_SESSIONS_PROMPT), NULL,
      qtConfirmation, qaYes | qaNo) == qaYes);

  if (Result)
  {
    UnicodeString FileName;
    TFarPanelItem * PanelItem;
    for (intptr_t I = 0; I < PanelItems->GetCount(); ++I)
    {
      PanelItem = static_cast<TFarPanelItem *>(PanelItems->GetItem(I));
      bool AnyData = false;
      FileName = PanelItem->GetFileName();
      if (PanelItem->GetIsFile())
      {
        UnicodeString XmlFileName = ::IncludeTrailingBackslash(GetCurrentDir()) + FileName;
        std::auto_ptr<THierarchicalStorage> ImportStoragePtr(new TXmlStorage(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));
        ImportStoragePtr->Init();
        ImportStoragePtr->SetAccessMode(smRead);
        if (ImportStoragePtr->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false) &&
            ImportStoragePtr->HasSubKeys())
        {
          AnyData = true;
          StoredSessions->Load(ImportStoragePtr.get(), /* AsModified */ true, /* UseDefaults */ true);
          // modified only, explicit
          StoredSessions->Save(false, true);
        }
      }
      if (!AnyData)
      {
        throw Exception(FORMAT(GetMsg(IMPORT_SESSIONS_EMPTY).c_str(), FileName.c_str()));
      }
    }
  }
  return Result;
}
//------------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateFocusedFileList(
  TOperationSide Side, TFarPanelInfo * PanelInfo)
{
  if (PanelInfo == NULL)
  {
    PanelInfo = this->GetPanelInfo();
  }

  TStrings * Result;
  TFarPanelItem * PanelItem = PanelInfo->GetFocusedItem();
  if (PanelItem->GetIsParentDirectory())
  {
    Result = NULL;
  }
  else
  {
    Result = new TStringList();
    assert((Side == osLocal) || PanelItem->GetUserData());
    UnicodeString FileName = PanelItem->GetFileName();
    if (Side == osLocal)
    {
      FileName = IncludeTrailingBackslash(GetPanelInfo()->GetCurrentDirectory()) + FileName;
    }
    Result->AddObject(FileName, static_cast<TObject *>(PanelItem->GetUserData()));
  }
  return Result;
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
TStrings * TWinSCPFileSystem::CreateFileList(TObjectList * PanelItems,
  TOperationSide Side, bool SelectedOnly, const UnicodeString & Directory, bool FileNameOnly,
  TStrings * AFileList)
{
  TStrings * FileList = (AFileList == NULL ? new TStringList() : AFileList);
  try
  {
    UnicodeString FileName;
    TFarPanelItem * PanelItem;
    TObject * Data = NULL;
    for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
    {
      PanelItem = static_cast<TFarPanelItem *>(PanelItems->GetItem(Index));
      assert(PanelItem);
      if ((!SelectedOnly || PanelItem->GetSelected()) &&
          !PanelItem->GetIsParentDirectory())
      {
        FileName = PanelItem->GetFileName();
        if (Side == osRemote)
        {
          Data = static_cast<TRemoteFile *>(PanelItem->GetUserData());
          assert(Data);
        }
        if (Side == osLocal)
        {
          if (ExtractFilePath(FileName).IsEmpty())
          {
            if (!FileNameOnly)
            {
              UnicodeString Dir = Directory;
              if (Dir.IsEmpty())
              {
                Dir = GetCurrentDir();
              }
              FileName = IncludeTrailingBackslash(Dir) + FileName;
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::SaveSession()
{
  if (FTerminal->GetActive() && !FTerminal->GetSessionData()->GetName().IsEmpty())
  {
    FTerminal->GetSessionData()->SetRemoteDirectory(FTerminal->GetCurrentDirectory());

    TSessionData * Data;
    Data = static_cast<TSessionData *>(StoredSessions->FindByName(FTerminal->GetSessionData()->GetName()));
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
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::Connect(TSessionData * Data)
{
  bool Result = false;
  assert(!FTerminal);
  FTerminal = new TTerminal();
  FTerminal->Init(Data, GetConfiguration());
  try
  {
    FTerminal->SetOnQueryUser(MAKE_CALLBACK(TWinSCPFileSystem::TerminalQueryUser, this));
    FTerminal->SetOnPromptUser(MAKE_CALLBACK(TWinSCPFileSystem::TerminalPromptUser, this));
    FTerminal->SetOnDisplayBanner(MAKE_CALLBACK(TWinSCPFileSystem::TerminalDisplayBanner, this));
    FTerminal->SetOnShowExtendedException(MAKE_CALLBACK(TWinSCPFileSystem::TerminalShowExtendedException, this));
    FTerminal->SetOnChangeDirectory(MAKE_CALLBACK(TWinSCPFileSystem::TerminalChangeDirectory, this));
    FTerminal->SetOnReadDirectory(MAKE_CALLBACK(TWinSCPFileSystem::TerminalReadDirectory, this));
    FTerminal->SetOnStartReadDirectory(MAKE_CALLBACK(TWinSCPFileSystem::TerminalStartReadDirectory, this));
    FTerminal->SetOnReadDirectoryProgress(MAKE_CALLBACK(TWinSCPFileSystem::TerminalReadDirectoryProgress, this));
    FTerminal->SetOnInformation(MAKE_CALLBACK(TWinSCPFileSystem::TerminalInformation, this));
    FTerminal->SetOnFinished(MAKE_CALLBACK(TWinSCPFileSystem::OperationFinished, this));
    FTerminal->SetOnProgress(MAKE_CALLBACK(TWinSCPFileSystem::OperationProgress, this));
    FTerminal->SetOnDeleteLocalFile(MAKE_CALLBACK(TWinSCPFileSystem::TerminalDeleteLocalFile, this));
    FTerminal->SetOnCreateLocalFile(MAKE_CALLBACK(TWinSCPFileSystem::TerminalCreateLocalFile, this));
    FTerminal->SetOnGetLocalFileAttributes(MAKE_CALLBACK(TWinSCPFileSystem::TerminalGetLocalFileAttributes, this));
    FTerminal->SetOnSetLocalFileAttributes(MAKE_CALLBACK(TWinSCPFileSystem::TerminalSetLocalFileAttributes, this));
    FTerminal->SetOnMoveLocalFile(MAKE_CALLBACK(TWinSCPFileSystem::TerminalMoveLocalFile, this));
    FTerminal->SetOnRemoveLocalDirectory(MAKE_CALLBACK(TWinSCPFileSystem::TerminalRemoveLocalDirectory, this));
    FTerminal->SetOnCreateLocalDirectory(MAKE_CALLBACK(TWinSCPFileSystem::TerminalCreateLocalDirectory, this));
    FTerminal->SetOnCheckForEsc(MAKE_CALLBACK(TWinSCPFileSystem::TerminalCheckForEsc, this));
    ConnectTerminal(FTerminal);

    FTerminal->SetOnClose(MAKE_CALLBACK(TWinSCPFileSystem::TerminalClose, this));

    assert(FQueue == NULL);
    FQueue = new TTerminalQueue(FTerminal, GetConfiguration());
    FQueue->Init();
    FQueue->SetTransfersLimit(GUIConfiguration->GetQueueTransfersLimit());
    FQueue->SetOnQueryUser(MAKE_CALLBACK(TWinSCPFileSystem::TerminalQueryUser, this));
    FQueue->SetOnPromptUser(MAKE_CALLBACK(TWinSCPFileSystem::TerminalPromptUser, this));
    FQueue->SetOnShowExtendedException(MAKE_CALLBACK(TWinSCPFileSystem::TerminalShowExtendedException, this));
    FQueue->SetOnListUpdate(MAKE_CALLBACK(TWinSCPFileSystem::QueueListUpdate, this));
    FQueue->SetOnQueueItemUpdate(MAKE_CALLBACK(TWinSCPFileSystem::QueueItemUpdate, this));
    FQueue->SetOnEvent(MAKE_CALLBACK(TWinSCPFileSystem::QueueEvent, this));

    assert(FQueueStatus == NULL);
    FQueueStatus = FQueue->CreateStatus(NULL);

    // TODO: Create instance of TKeepaliveThread here, once its implementation
    // is complete

    Result = FTerminal->GetActive();
    if (!Result)
    {
      throw Exception(FORMAT(GetMsg(CANNOT_INIT_SESSION).c_str(), Data->GetSessionName().c_str()));
    }
  }
  catch(Exception &E)
  {
    FTerminal->ShowExtendedException(&E);
    SAFE_DESTROY(FTerminal);
    FTerminal = NULL;
    SAFE_DESTROY(FQueue);
    FQueue = NULL;
    SAFE_DESTROY(FQueueStatus);
    FQueueStatus = NULL;
  }

  if (FTerminal != NULL)
  {
    FSynchronisingBrowse = FTerminal->GetSessionData()->GetSynchronizeBrowsing();
  }
  return Result;
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::Disconnect()
{
  if (FTerminal && FTerminal->GetActive())
  {
    if (!FTerminal->GetSessionData()->GetName().IsEmpty())
    {
      FPrevSessionName = FTerminal->GetSessionData()->GetName();
    }
    SaveSession();
  }
  assert(FSynchronizeController == NULL);
  assert(!FAuthenticationSaveScreenHandle);
  assert(!FProgressSaveScreenHandle);
  assert(!FSynchronizationSaveScreenHandle);
  assert(!FFileList);
  assert(!FPanelItems);
  delete FQueue;
  FQueue = NULL;
  delete FQueueStatus;
  FQueueStatus = NULL;
  if (FTerminal != NULL)
  {
    FTerminal->GetSessionData()->SetSynchronizeBrowsing(FSynchronisingBrowse);
  }
  SAFE_DESTROY(FTerminal);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ConnectTerminal(TTerminal * Terminal)
{
  Terminal->Open();
}
//------------------------------------------------------------------------------
bool TWinSCPFileSystem::TerminalCheckForEsc()
{
  return WinSCPPlugin()->CheckForEsc();
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalClose(TObject * /*Sender*/)
{
  // Plugin closure is now invoked from HandleException
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::LogAuthentication(
  TTerminal * Terminal, const UnicodeString & Msg)
{
  assert(FAuthenticationLog != NULL);
  FAuthenticationLog->Add(Msg);
  TStringList * AuthenticationLogLines = new TStringList();
  {
    std::auto_ptr<TStringList> AuthenticationLogLinesPtr;
    AuthenticationLogLinesPtr.reset(AuthenticationLogLines);
    intptr_t Width = 42;
    intptr_t Height = 11;
    FarWrapText(::TrimRight(FAuthenticationLog->GetText()), AuthenticationLogLines, Width);
    intptr_t Count;
    UnicodeString Message;
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
      AuthenticationLogLines->SetString(0, AuthenticationLogLines->GetString(0) +
        ::StringOfChar(' ', Width - AuthenticationLogLines->GetString(0).Length()));
      Message = AnsiReplaceStr(AuthenticationLogLines->GetText(), L"\r", L"");
      Count = AuthenticationLogLines->GetCount();
    }

    Message += ::StringOfChar(L'\n', Height - Count);

    WinSCPPlugin()->Message(0, Terminal->GetSessionData()->GetSessionName(), Message);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & Str, bool /*Status*/, int Phase)
{
  if (Phase != 0)
  {
    if (GetTerminal() && (GetTerminal()->GetStatus() == ssOpening))
    {
      if (FAuthenticationLog == NULL)
      {
        FAuthenticationLog = new TStringList();
        WinSCPPlugin()->SaveScreen(FAuthenticationSaveScreenHandle);
        WinSCPPlugin()->ShowConsoleTitle(GetTerminal()->GetSessionData()->GetSessionName());
      }

      LogAuthentication(Terminal, Str);
      WinSCPPlugin()->UpdateConsoleTitle(Str);
    }
  }
  else
  {
    if (FAuthenticationLog != NULL)
    {
      WinSCPPlugin()->ClearConsoleTitle();
      WinSCPPlugin()->RestoreScreen(FAuthenticationSaveScreenHandle);
      SAFE_DESTROY(FAuthenticationLog);
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalChangeDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    UnicodeString Directory = FTerminal->GetCurrentDirectory();
    intptr_t Index = FPathHistory->IndexOf(Directory.c_str());
    if (Index >= 0)
    {
      FPathHistory->Delete(Index);
    }

    if (!FLastPath.IsEmpty())
    {
      FPathHistory->Add(FLastPath);
    }

    FLastPath = Directory;
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalStartReadDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    WinSCPPlugin()->ShowConsoleTitle(GetMsg(READING_DIRECTORY_TITLE));
  }
}
//------------------------------------------------------------------------------
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
    if (WinSCPPlugin()->CheckForEsc())
    {
      Cancel = true;
    }

    if (!FNoProgress)
    {
      WinSCPPlugin()->UpdateConsoleTitle(
        FORMAT(L"%s (%d)", GetMsg(READING_DIRECTORY_TITLE).c_str(), Progress));
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalReadDirectory(TObject * /*Sender*/,
  bool /*ReloadOnly*/)
{
  if (!FNoProgress)
  {
    WinSCPPlugin()->ClearConsoleTitle();
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalDeleteLocalFile(const UnicodeString & FileName,
  bool Alternative)
{
  bool ToRecycleBin = FLAGSET(WinSCPPlugin()->FarSystemSettings(), NBSS_DELETETORECYCLEBIN) != Alternative;
  if (ToRecycleBin || !WinSCPPlugin()->GetSystemFunctions())
  {
    if (!RecursiveDeleteFile(FileName, ToRecycleBin))
    {
      throw Exception(FORMAT(GetMsg(DELETE_LOCAL_FILE_ERROR).c_str(), FileName.c_str()));
    }
  }
  else
  {
    WinSCPPlugin()->DeleteLocalFile(FileName);
  }
}
//------------------------------------------------------------------------------
HANDLE TWinSCPFileSystem::TerminalCreateLocalFile(const UnicodeString & LocalFileName,
  DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (!WinSCPPlugin()->GetSystemFunctions())
  {
    return ::CreateFile(LocalFileName.c_str(), DesiredAccess, ShareMode, NULL, CreationDisposition, FlagsAndAttributes, 0);
  }
  else
  {
    return WinSCPPlugin()->CreateLocalFile(LocalFileName, DesiredAccess,
      ShareMode, CreationDisposition, FlagsAndAttributes);
  }
}
//------------------------------------------------------------------------------
DWORD TWinSCPFileSystem::TerminalGetLocalFileAttributes(const UnicodeString & LocalFileName)
{
  if (!WinSCPPlugin()->GetSystemFunctions())
  {
    return ::GetFileAttributes(LocalFileName.c_str());
  }
  else
  {
    return WinSCPPlugin()->GetLocalFileAttributes(LocalFileName);
  }
}
//------------------------------------------------------------------------------
BOOL TWinSCPFileSystem::TerminalSetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
{
  if (!WinSCPPlugin()->GetSystemFunctions())
  {
    return ::SetFileAttributes(LocalFileName.c_str(), FileAttributes);
  }
  else
  {
    return WinSCPPlugin()->SetLocalFileAttributes(LocalFileName, FileAttributes);
  }
}
//------------------------------------------------------------------------------
BOOL TWinSCPFileSystem::TerminalMoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
{
  if (!WinSCPPlugin()->GetSystemFunctions())
  {
    return ::MoveFileExW(LocalFileName.c_str(), NewLocalFileName.c_str(), Flags) != 0;
  }
  else
  {
    return WinSCPPlugin()->MoveLocalFile(LocalFileName, NewLocalFileName, Flags);
  }
}
//------------------------------------------------------------------------------
BOOL TWinSCPFileSystem::TerminalRemoveLocalDirectory(const UnicodeString & LocalDirName)
{
  if (!WinSCPPlugin()->GetSystemFunctions())
  {
    return ::RemoveDirectory(LocalDirName) != 0;
  }
  else
  {
    return WinSCPPlugin()->RemoveLocalDirectory(LocalDirName);
  }
}
//------------------------------------------------------------------------------
BOOL TWinSCPFileSystem::TerminalCreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (!WinSCPPlugin()->GetSystemFunctions())
  {
    return ::CreateDirectory(LocalDirName.c_str(), SecurityAttributes) != 0;
  }
  else
  {
    return WinSCPPlugin()->CreateLocalDirectory(LocalDirName, SecurityAttributes);
  }
}
//------------------------------------------------------------------------------
uintptr_t TWinSCPFileSystem::MoreMessageDialog(const UnicodeString & Str,
  TStrings * MoreMessages, TQueryType Type, uintptr_t Answers, const TMessageParams * Params)
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalQueryUser(TObject * /*Sender*/,
  const UnicodeString & Query, TStrings * MoreMessages, uintptr_t Answers,
  const TQueryParams * Params, uintptr_t & Answer, TQueryType Type, void * /*Arg*/)
{
  TMessageParams AParams;
  UnicodeString AQuery = Query;

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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
  TStrings * Prompts, TStrings * Results, bool & Result,
  void * /*Arg*/)
{
  if (Kind == pkPrompt)
  {
    assert(Instructions.IsEmpty());
    assert(Prompts->GetCount() == 1);
    assert((Prompts->Objects[0]) != NULL);
    UnicodeString AResult = Results->GetString(0);

    Result = WinSCPPlugin()->InputBox(Name, StripHotkey(Prompts->GetString(0)), AResult, FIB_NOUSELASTHISTORY);
    if (Result)
    {
      Results->SetString(0, AResult);
    }
  }
  else
  {
    Result = PasswordDialog(Terminal->GetSessionData(), Kind, Name, Instructions,
      Prompts, Results, GetTerminal()->GetStoredCredentialsTried());
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalDisplayBanner(
  TTerminal * /*Terminal*/, const UnicodeString & SessionName,
  const UnicodeString & Banner, bool & NeverShowAgain, intptr_t Options)
{
  BannerDialog(SessionName, Banner, NeverShowAgain, Options);
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::TerminalShowExtendedException(
  TTerminal * /*Terminal*/, Exception * E, void * /*Arg*/)
{
  WinSCPPlugin()->ShowExtendedException(E);
}
//------------------------------------------------------------------------------
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
    WinSCPPlugin()->SaveScreen(FProgressSaveScreenHandle);
    First = true;
  }

  // operation is finished (or terminated), so we hide progress form
  if (!ProgressData.InProgress && FProgressSaveScreenHandle)
  {
    WinSCPPlugin()->RestoreScreen(FProgressSaveScreenHandle);
    WinSCPPlugin()->ClearConsoleTitle();
  }
  else
  {
    ShowOperationProgress(ProgressData, First);
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::OperationFinished(TFileOperation Operation,
  TOperationSide Side, bool /*Temp*/, const UnicodeString & FileName, bool Success,
  TOnceDoneOperation & /*DisconnectWhenComplete*/)
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
      TObjectList * PanelItems = GetPanelInfo()->GetItems();
      for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
      {
        if ((static_cast<TFarPanelItem *>(PanelItems->GetItem(Index)))->GetFileName() == FileName)
        {
          PanelItem = static_cast<TFarPanelItem *>(PanelItems->GetItem(Index));
          break;
        }
      }
    }
    else
    {
      assert(FFileList);
      assert(FPanelItems->GetCount() == FFileList->GetCount());
      intptr_t Index = FFileList->IndexOf(FileName.c_str());
      assert(Index >= 0);
      PanelItem = static_cast<TFarPanelItem *>(FPanelItems->GetItem(Index));
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
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ShowOperationProgress(
  TFileOperationProgressType & ProgressData, bool First)
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  short percents = static_cast<short>(ProgressData.OverallProgress());
  if (Ticks - LastTicks > 500 || First)
  {
    LastTicks = Ticks;

    static const intptr_t ProgressWidth = 48;
    static const int Captions[] = {PROGRESS_COPY, PROGRESS_MOVE, PROGRESS_DELETE,
      PROGRESS_SETPROPERTIES, 0, 0, PROGRESS_CALCULATE_SIZE,
      PROGRESS_REMOTE_MOVE, PROGRESS_REMOTE_COPY, PROGRESS_GETPROPERTIES,
      PROGRESS_CALCULATE_CHECKSUM };
    static UnicodeString ProgressFileLabel;
    static UnicodeString TargetDirLabel;
    static UnicodeString StartTimeLabel;
    static UnicodeString TimeElapsedLabel;
    static UnicodeString BytesTransferedLabel;
    static UnicodeString CPSLabel;
    static UnicodeString TimeLeftLabel;

    if (ProgressFileLabel.IsEmpty())
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

    UnicodeString Message1;
    UnicodeString ProgressBar1;
    UnicodeString Message2;
    UnicodeString ProgressBar2;
    UnicodeString Title = GetMsg(Captions[static_cast<int>(ProgressData.Operation - 1)]);
    UnicodeString FileName = ProgressData.FileName;
    // for upload from temporary directory,
    // do not show source directory
    if (TransferOperation && (ProgressData.Side == osLocal) && ProgressData.Temp)
    {
      FileName = ExtractFileName(FileName, false);
    }
    Message1 = ProgressFileLabel + MinimizeName(FileName,
      ProgressWidth - ProgressFileLabel.Length(), ProgressData.Side == osRemote) + L"\n";
    // for downloads to temporary directory,
    // do not show target directory
    if (TransferOperation && !((ProgressData.Side == osRemote) && ProgressData.Temp))
    {
      Message1 += TargetDirLabel + MinimizeName(ProgressData.Directory,
                  ProgressWidth - TargetDirLabel.Length(), ProgressData.Side == osLocal) + L"\n";
    }
    ProgressBar1 = ProgressBar(ProgressData.OverallProgress(), ProgressWidth) + L"\n";
    if (TransferOperation)
    {
      Message2 = L"\1\n";
      UnicodeString StatusLine;
      UnicodeString Value;

      Value = FormatDateTimeSpan(GetConfiguration()->GetTimeFormat(), ProgressData.TimeElapsed());
      StatusLine = TimeElapsedLabel +
                   ::StringOfChar(L' ', ProgressWidth / 2 - 1 - TimeElapsedLabel.Length() - Value.Length()) +
                   Value + L"  ";

      UnicodeString LabelText;
      if (ProgressData.TotalSizeSet)
      {
        Value = FormatDateTimeSpan(GetConfiguration()->GetTimeFormat(), ProgressData.TotalTimeLeft());
        LabelText = TimeLeftLabel;
      }
      else
      {
        Value = ProgressData.StartTime.TimeString();
        LabelText = StartTimeLabel;
      }
      StatusLine = StatusLine + LabelText +
                   ::StringOfChar(' ', ProgressWidth - StatusLine.Length() -
                                  LabelText.Length() - Value.Length()) + Value;
      Message2 += StatusLine + L"\n";

      Value = FormatBytes(ProgressData.TotalTransfered);
      StatusLine = BytesTransferedLabel +
                   ::StringOfChar(' ', ProgressWidth / 2 - 1 - BytesTransferedLabel.Length() - Value.Length()) +
                   Value + L"  ";
      Value = FORMAT(L"%s/s", FormatBytes(ProgressData.CPS()).c_str());
      StatusLine = StatusLine + CPSLabel +
                   ::StringOfChar(' ', ProgressWidth - StatusLine.Length() -
                                  CPSLabel.Length() - Value.Length()) + Value;
      Message2 += StatusLine + L"\n";
      ProgressBar2 += ProgressBar(ProgressData.TransferProgress(), ProgressWidth) + L"\n";
    }
    UnicodeString Message =
      Message1 + ProgressBar1 + Message2 + ProgressBar2;
    WinSCPPlugin()->Message(0, Title, Message, NULL, NULL);

    if (First)
    {
      WinSCPPlugin()->ShowConsoleTitle(Title);
    }
    WinSCPPlugin()->UpdateConsoleTitleProgress(percents);

    if (WinSCPPlugin()->CheckForEsc())
    {
      CancelConfiguration(ProgressData);
    }
  }
  if (percents == 100)
  {
    WinSCPPlugin()->UpdateConsoleTitleProgress(percents);
  }
}
//------------------------------------------------------------------------------
UnicodeString TWinSCPFileSystem::ProgressBar(intptr_t Percentage, intptr_t Width)
{
  UnicodeString Result;
  // 0xB0 - 0x2591
  // 0xDB - 0x2588
  Result = ::StringOfChar(0x2588, (Width - 5) * (Percentage > 100 ? 100 : Percentage) / 100);
  Result += ::StringOfChar(0x2591, (Width - 5) - Result.Length());
  Result += FORMAT(L"%4d%%", Percentage > 100 ? 100 : Percentage);
  return Result;
}
//------------------------------------------------------------------------------
TTerminalQueueStatus * TWinSCPFileSystem::ProcessQueue(bool Hidden)
{
  TTerminalQueueStatus * Result = NULL;
  assert(FQueueStatus != NULL);
  FarPlugin->UpdateProgress(FQueueStatus->GetCount() > 0 ? TBPS_INDETERMINATE : TBPS_NOPROGRESS, 0);

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

    for (intptr_t Index = 0; Index < FQueueStatus->GetActiveCount(); ++Index)
    {
      TQueueItemProxy * QueueItem = FQueueStatus->GetItem(Index);
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::QueueListUpdate(TTerminalQueue * Queue)
{
  if (FQueue == Queue)
  {
    FQueueStatusInvalidated = true;
  }
}
//------------------------------------------------------------------------------
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
        (!QueueItem->GetInfo()->ModifiedLocal.IsEmpty());
      FRefreshRemoteDirectory = (QueueItem == NULL) ||
        (!QueueItem->GetInfo()->ModifiedRemote.IsEmpty());
    }

    if (QueueItem != NULL)
    {
      QueueItem->SetUserData(reinterpret_cast<void *>(1));
      FQueueItemInvalidated = true;
    }
  }
}
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::CancelConfiguration(TFileOperationProgressType & ProgressData)
{
  if (!ProgressData.Suspended)
  {
    ProgressData.Suspend();
    TRY_FINALLY (
    {
      TCancelStatus ACancel;
      uintptr_t Result;
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
      }
      switch (Result)
      {
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
    ,
    {
      ProgressData.Resume();
    }
    );
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::UploadFromEditor(bool NoReload,
  const UnicodeString & FileName, const UnicodeString & RealFileName,
  UnicodeString & DestPath)
{
  assert(FFileList == NULL);
  FFileList = new TStringList();
  assert(FTerminal->GetAutoReadDirectory());
  bool PrevAutoReadDirectory = FTerminal->GetAutoReadDirectory();
  if (NoReload)
  {
    FTerminal->SetAutoReadDirectory(false);
    if (::UnixComparePaths(DestPath, FTerminal->GetCurrentDirectory()))
    {
      FReloadDirectory = true;
    }
  }

  TRemoteFile * File = new TRemoteFile();
  File->SetFileName(RealFileName);
  TRY_FINALLY (
  {
    FFileList->AddObject(FileName, File);
    UploadFiles(false, 0, true, DestPath);
  }
  ,
  {
    FTerminal->SetAutoReadDirectory(PrevAutoReadDirectory);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(File);
  }
  );
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::UploadOnSave(bool NoReload)
{
  TFarEditorInfo * Info = WinSCPPlugin()->EditorInfo();
  if (Info != NULL)
  {
    std::auto_ptr<TFarEditorInfo> InfoPtr(Info);
    bool NativeEdit =
      (FLastEditorID >= 0) &&
      (FLastEditorID == Info->GetEditorID()) &&
      !FLastEditFile.IsEmpty();

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
        UnicodeString CurrentDirectory = FTerminal->GetCurrentDirectory();
        UploadFromEditor(NoReload, FLastEditFile, FLastEditFile, CurrentDirectory);
        FTerminal->SetCurrentDirectory(CurrentDirectory);
      }

      if (MultipleEdit)
      {
        UploadFromEditor(NoReload, Info->GetFileName(), I->second.FileTitle, I->second.Directory);
        // note that panel gets not refreshed upon switch to
        // panel view. but that's intentional
      }
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::ProcessEditorEvent(intptr_t Event, void * /*Param*/)
{
  // EE_REDRAW is the first for optimization
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
      TFarEditorInfo * Info = WinSCPPlugin()->EditorInfo();
      if (Info != NULL)
      {
        std::auto_ptr<TFarEditorInfo> InfoPtr(Info);
        TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
        if (it != FMultipleEdits.end())
        {
          UnicodeString FullFileName = ::UnixIncludeTrailingBackslash(it->second.Directory) +
            it->second.FileTitle;
          WinSCPPlugin()->FarEditorControl(ECTL_SETTITLE,
              FullFileName.Length(),
            static_cast<void *>(const_cast<wchar_t *>(FullFileName.c_str())));
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
      TFarEditorInfo * Info = WinSCPPlugin()->EditorInfo();
      if (Info != NULL)
      {
        {
          std::auto_ptr<TFarEditorInfo> InfoPtr;
          InfoPtr.reset(Info);
          if (!FLastEditFile.IsEmpty() &&
              AnsiSameText(FLastEditFile, Info->GetFileName()))
          {
            FLastEditorID = Info->GetEditorID();
            FEditorPendingSave = false;
          }

          if (!FLastMultipleEditFile.IsEmpty())
          {
            bool IsLastMultipleEditFile = AnsiSameText(::FromUnixPath(FLastMultipleEditFile), ::FromUnixPath(Info->GetFileName()));
            assert(IsLastMultipleEditFile);
            if (IsLastMultipleEditFile)
            {
              FLastMultipleEditFile = L"";

              TMultipleEdit MultipleEdit;
              MultipleEdit.FileName = ExtractFileName(Info->GetFileName(), false);
              MultipleEdit.FileTitle = FLastMultipleEditFileTitle;
              MultipleEdit.Directory = FLastMultipleEditDirectory;
              MultipleEdit.LocalFileName = Info->GetFileName();
              MultipleEdit.PendingSave = false;
              FMultipleEdits[Info->GetEditorID()] = MultipleEdit;
              if (FLastMultipleEditReadOnly)
              {
                EditorSetParameter Parameter;
                memset(&Parameter, 0, sizeof(Parameter));
                Parameter.StructSize = sizeof(EditorSetParameter);
                Parameter.Type = ESPT_LOCKMODE;
                Parameter.iParam = TRUE;
                WinSCPPlugin()->FarEditorControl(ECTL_SETPARAM, 0, &Parameter);
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

    TFarEditorInfo * Info = WinSCPPlugin()->EditorInfo();
    if (Info != NULL)
    {
      {
        std::auto_ptr<TFarEditorInfo> InfoPtr;
        InfoPtr.reset(Info);
        if (FLastEditorID == Info->GetEditorID())
        {
          FLastEditorID = -1;
        }

        TMultipleEdits::iterator I = FMultipleEdits.find(Info->GetEditorID());
        if (I != FMultipleEdits.end())
        {
          if (I->second.PendingSave)
          {
            UploadFromEditor(true, Info->GetFileName(), I->second.FileTitle, I->second.Directory);
            // reload panel content (if uploaded to current directory.
            // no need for RefreshPanel as panel is not visible yet.
            UpdatePanel();
          }

          if (Sysutils::DeleteFile(Info->GetFileName()))
          {
            // remove directory only if it is empty
            // (to avoid deleting another directory if user uses "save as")
            ::RemoveDir(ExcludeTrailingBackslash(ExtractFilePath(Info->GetFileName())));
          }

          FMultipleEdits.erase(I->first);
        }
      }
    }
  }
  else if (Event == EE_SAVE)
  {
    TFarEditorInfo * Info = WinSCPPlugin()->EditorInfo();
    if (Info != NULL)
    {
      {
        std::auto_ptr<TFarEditorInfo> InfoPtr;
        InfoPtr.reset(Info);
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
            UnicodeString FullFileName = ::UnixIncludeTrailingBackslash(I->second.Directory) +
                I->second.FileTitle;
            // note that we need to reset the title periodically (see EE_REDRAW)
            WinSCPPlugin()->FarEditorControl(ECTL_SETTITLE,
              FullFileName.Length(),
              static_cast<void *>(const_cast<wchar_t *>(FullFileName.c_str())));
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
//------------------------------------------------------------------------------
void TWinSCPFileSystem::EditViewCopyParam(TCopyParamType & CopyParam)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);
  CopyParam.SetReplaceInvalidChars(true);
  CopyParam.SetFileMask(L"");
  CopyParam.SetExcludeFileMask(TFileMasks());
}
//------------------------------------------------------------------------------
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
      std::auto_ptr<TStrings> FileListPtr;
      FileListPtr.reset(FileList);
      if (FileList->GetCount() == 1)
      {
        MultipleEdit(FTerminal->GetCurrentDirectory(), FileList->GetString(0),
          static_cast<TRemoteFile *>(FileList->Objects[0]));
      }
    }
  }
}
//------------------------------------------------------------------------------
void TWinSCPFileSystem::MultipleEdit(const UnicodeString & Directory,
  const UnicodeString & FileName, TRemoteFile * File)
{
  TEditHistory EditHistory;
  EditHistory.Directory = Directory;
  EditHistory.FileName = FileName;

  TEditHistories::iterator ih = rde::find(FEditHistories.begin(), FEditHistories.end(), EditHistory);
  if (ih != FEditHistories.end())
  {
    FEditHistories.erase(ih);
  }
  FEditHistories.push_back(EditHistory);

  UnicodeString FullFileName = ::UnixIncludeTrailingBackslash(Directory) + FileName;

  TRemoteFile * FileDuplicate = File->Duplicate();
  UnicodeString NewFileName = FullFileName; // ::UnixIncludeTrailingBackslash(GetFileNameHash(FullFileName)) + FileName;
  FileDuplicate->SetFileName(NewFileName);

  TMultipleEdits::iterator it = FMultipleEdits.begin();
  while (it != FMultipleEdits.end())
  {
    if (::UnixComparePaths(Directory, it->second.Directory) &&
        (NewFileName == it->second.FileName))
    {
      break;
    }
    ++it;
  }

  FLastMultipleEditReadOnly = false;
  bool Edit = true;
  if (it != FMultipleEdits.end())
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
    UnicodeString TempDir;
    TGUICopyParamType & CopyParam = GUIConfiguration->GetDefaultCopyParam();
    EditViewCopyParam(CopyParam);

    TStrings * FileList = new TStringList();
    assert(!FNoProgressFinish);
    FNoProgressFinish = true;
    TRY_FINALLY (
    {
      FileList->AddObject(FullFileName, FileDuplicate);
      TemporarilyDownloadFiles(FileList, CopyParam, TempDir);
    }
    ,
    {
      FNoProgressFinish = false;
      delete FileList;
      delete FileDuplicate;
    }
    );

    FLastMultipleEditFile = ::IncludeTrailingBackslash(TempDir) + NewFileName;
    FLastMultipleEditFileTitle = FileName;
    FLastMultipleEditDirectory = Directory;

    if (FarPlugin->Editor(FLastMultipleEditFile, FullFileName,
          EF_NONMODAL | EF_IMMEDIATERETURN | EF_DISABLEHISTORY))
    {
      // assert(FLastMultipleEditFile == L"");
    }
    FLastMultipleEditFile = L"";
    FLastMultipleEditFileTitle = L"";
  }
  else
  {
    assert(it != FMultipleEdits.end());

    intptr_t WindowCount = FarPlugin->FarAdvControl(ACTL_GETWINDOWCOUNT, 0);
    int Pos = 0;
    while (Pos < WindowCount)
    {
      WindowInfo Window = {0};
      Window.StructSize = sizeof(WindowInfo);
      Window.Pos = Pos;
      UnicodeString EditedFileName(1024, 0);
      Window.Name = const_cast<wchar_t *>(EditedFileName.c_str());
      Window.NameSize = (int)EditedFileName.GetLength();
      if (FarPlugin->FarAdvControl(ACTL_GETWINDOWINFO, 0, &Window) != 0)
      {
        if ((Window.Type == WTYPE_EDITOR) &&
            Window.Name && AnsiSameText(Window.Name, it->second.LocalFileName))
        {
          if (FarPlugin->FarAdvControl(ACTL_SETCURRENTWINDOW, Pos, NULL) != 0)
            FarPlugin->FarAdvControl(ACTL_COMMIT, 0);
          break;
        }
      }
      Pos++;
    }

    assert(Pos < WindowCount);
  }
}
//---------------------------------------------------------------------------------
bool TWinSCPFileSystem::IsEditHistoryEmpty()
{
  return FEditHistories.empty();
}
//---------------------------------------------------------------------------------
void TWinSCPFileSystem::EditHistory()
{
  TFarMenuItems * MenuItems = new TFarMenuItems();
  {
    std::auto_ptr<TFarMenuItems> MenuItemsPtr;
    MenuItemsPtr.reset(MenuItems);
    TEditHistories::const_iterator i = FEditHistories.begin();
    while (i != FEditHistories.end())
    {
      MenuItems->Add(MinimizeName(::UnixIncludeTrailingBackslash((*i).Directory) + (*i).FileName,
        WinSCPPlugin()->MaxMenuItemLength(), true));
      ++i;
    }

    MenuItems->Add(L"");
    MenuItems->SetItemFocused(MenuItems->GetCount() - 1);

    const FarKey BreakKeys[] = { { VK_F4, 0 }, { 0 } };
    
    intptr_t BreakCode = 0;
    intptr_t Result = WinSCPPlugin()->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
      GetMsg(MENU_EDIT_HISTORY), L"", MenuItems, BreakKeys, BreakCode);

    if ((Result >= 0) && (Result < static_cast<intptr_t>(FEditHistories.size())))
    {
      TRemoteFile * File;
      UnicodeString FullFileName =
        ::UnixIncludeTrailingBackslash(FEditHistories[Result].Directory) + FEditHistories[Result].FileName;
      FTerminal->ReadFile(FullFileName, File);
      {
        std::auto_ptr<TRemoteFile> FilePtr;
        FilePtr.reset(File);
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
//---------------------------------------------------------------------------------
bool TWinSCPFileSystem::IsLogging()
{
  return
    Connected() && FTerminal->GetLog()->GetLoggingToFile();
}
//---------------------------------------------------------------------------------
void TWinSCPFileSystem::ShowLog()
{
  assert(Connected() && FTerminal->GetLog()->GetLoggingToFile());
  WinSCPPlugin()->Viewer(FTerminal->GetLog()->GetCurrentFileName(), FTerminal->GetLog()->GetCurrentFileName(), VF_NONMODAL);
}
//------------------------------------------------------------------------------
UnicodeString TWinSCPFileSystem::GetFileNameHash(const UnicodeString & FileName)
{
  RawByteString Result;
  Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(FileName.c_str()), static_cast<int>(FileName.Length() * sizeof(wchar_t)),
    reinterpret_cast<unsigned char *>(const_cast<char *>(Result.c_str())));
  return BytesToHex(Result);
}
//---------------------------------------------------------------------------------
