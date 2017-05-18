#include <vcl.h>
#pragma hdrstop

#include "WinSCPFileSystem.h"
#include "WinSCPPlugin.h"
#include "FarConfiguration.h"
#include "FarUtils.h"
#include <Common.h>
#include <MsgIDs.h>
#include <Exceptions.h>
#include <SessionData.h>
#include <CoreMain.h>
#include <ScpFileSystem.h>
#include <Bookmarks.h>
#include <GUITools.h>
#include <Sysutils.hpp>
#include "guid.h"

#include "PuttyIntf.h"
#include "XmlStorage.h"

TSessionPanelItem::TSessionPanelItem(const TSessionData * ASessionData) :
  TCustomFarPanelItem(OBJECT_CLASS_TSessionPanelItem)
{
  DebugAssert(ASessionData);
  FSessionData = ASessionData;
}

void TSessionPanelItem::SetPanelModes(TFarPanelModes * PanelModes)
{
  DebugAssert(FarPlugin);
  std::unique_ptr<TStrings> ColumnTitles(new TStringList());
  ColumnTitles->Add(FarPlugin->GetMsg(SESSION_NAME_COL_TITLE));
  for (intptr_t Index = 0; Index < PANEL_MODES_COUNT; ++Index)
  {
    PanelModes->SetPanelMode(Index, L"N", L"0", ColumnTitles.get(), false, false, false);
  }
}

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

void TSessionPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & AFileName, int64_t & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& UserData, size_t & /*CustomColumnNumber*/)
{
  AFileName = base::UnixExtractFileName(FSessionData->GetName());
  UserData = static_cast<void *>(const_cast<TSessionData *>(FSessionData));
}

TSessionFolderPanelItem::TSessionFolderPanelItem(const UnicodeString & Folder) :
  TCustomFarPanelItem(OBJECT_CLASS_TSessionFolderPanelItem),
  FFolder(Folder)
{
}

void TSessionFolderPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & AFileName, int64_t & /*Size*/,
  uintptr_t & FileAttributes,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  AFileName = FFolder;
  FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
}

TRemoteFilePanelItem::TRemoteFilePanelItem(TRemoteFile * ARemoteFile) :
  TCustomFarPanelItem(OBJECT_CLASS_TRemoteFilePanelItem)
{
  DebugAssert(ARemoteFile);
  FRemoteFile = ARemoteFile;
}

void TRemoteFilePanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & AFileName, int64_t & Size,
  uintptr_t & FileAttributes,
  TDateTime & LastWriteTime, TDateTime & LastAccess,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & Owner, void *& UserData, size_t & CustomColumnNumber)
{
  AFileName = FRemoteFile->GetFileName();
  Size = FRemoteFile->GetSize();
  if (Size < 0)
    Size = 0;
  if (FRemoteFile->GetIsDirectory())
    Size = 0;
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

UnicodeString TRemoteFilePanelItem::GetCustomColumnData(size_t Column)
{
  switch (Column)
  {
  case 0: return FRemoteFile->GetFileGroup().GetName();
  case 1: return FRemoteFile->GetRightsStr();
  case 2: return FRemoteFile->GetRights()->GetOctal();
  case 3: return FRemoteFile->GetLinkTo();
  default:
    DebugFail();
    return UnicodeString();
  }
}

void TRemoteFilePanelItem::TranslateColumnTypes(UnicodeString & AColumnTypes,
  TStrings * ColumnTitles)
{
  UnicodeString ColumnTypes = AColumnTypes;
  AColumnTypes.Clear();
  UnicodeString Column;
  UnicodeString Title;
  while (!ColumnTypes.IsEmpty())
  {
    Column = CutToChar(ColumnTypes, ',', false);
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
      Title.Clear();
    }
    AColumnTypes += (AColumnTypes.IsEmpty() ? L"" : L",") + Column;
    if (ColumnTitles)
    {
      ColumnTitles->Add(Title);
    }
  }
}

void TRemoteFilePanelItem::SetPanelModes(TFarPanelModes * PanelModes)
{
  DebugAssert(FarPlugin);
  std::unique_ptr<TStrings> ColumnTitles(new TStringList());
  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  if (FarConfiguration->GetCustomPanelModeDetailed())
  {
    UnicodeString ColumnTypes = FarConfiguration->GetColumnTypesDetailed();
    UnicodeString StatusColumnTypes = FarConfiguration->GetStatusColumnTypesDetailed();

    TranslateColumnTypes(ColumnTypes, ColumnTitles.get());
    TranslateColumnTypes(StatusColumnTypes, nullptr);

    PanelModes->SetPanelMode(5 /*detailed*/,
      ColumnTypes, FarConfiguration->GetColumnWidthsDetailed(),
      ColumnTitles.get(), FarConfiguration->GetFullScreenDetailed(), false, true, false,
      StatusColumnTypes, FarConfiguration->GetStatusColumnWidthsDetailed());
  }
}

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

class TFarInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TFarInteractiveCustomCommand(TCustomFarPlugin * Plugin,
    TCustomCommand * ChildCustomCommand);

protected:
  virtual void Prompt(const UnicodeString & APrompt,
    UnicodeString & Value) const;

private:
  TCustomFarPlugin * FPlugin;
};

TFarInteractiveCustomCommand::TFarInteractiveCustomCommand(
  TCustomFarPlugin * Plugin, TCustomCommand * ChildCustomCommand) :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FPlugin = Plugin;
}

void TFarInteractiveCustomCommand::Prompt(const UnicodeString & APrompt,
  UnicodeString & Value) const
{
  UnicodeString Prompt = APrompt;
  if (Prompt.IsEmpty())
  {
    Prompt = FPlugin->GetMsg(APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!FPlugin->InputBox(FPlugin->GetMsg(APPLY_COMMAND_PARAM_TITLE),
        Prompt, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}

// Attempt to allow keepalives from background thread.
// Not finished nor used.
class TKeepaliveThread : public TSimpleThread
{
public:
  explicit TKeepaliveThread(TWinSCPFileSystem * FileSystem, const TDateTime & Interval);

  virtual ~TKeepaliveThread()
  {
  }

  virtual void Init();
  virtual void Execute();
  virtual void Terminate();

private:
  TWinSCPFileSystem * FFileSystem;
  TDateTime FInterval;
  HANDLE FEvent;
};

TKeepaliveThread::TKeepaliveThread(TWinSCPFileSystem * FileSystem,
  const TDateTime & Interval) :
  TSimpleThread(OBJECT_CLASS_TKeepAliveThread),
  FFileSystem(FileSystem),
  FInterval(Interval),
  FEvent(nullptr)
{
}

void TKeepaliveThread::Init()
{
  TSimpleThread::Init();
  FEvent = ::CreateEvent(nullptr, false, false, nullptr);
  Start();
}

void TKeepaliveThread::Terminate()
{
  // TCompThread::Terminate();
  ::SetEvent(FEvent);
}

void TKeepaliveThread::Execute()
{
  while (!IsFinished())
  {
    if ((::WaitForSingleObject(FEvent, static_cast<DWORD>(
         ToDouble(FInterval) * MSecsPerDay)) != WAIT_FAILED) &&
        !IsFinished())
    {
      FFileSystem->KeepaliveThreadCallback();
    }
  }
  SAFE_CLOSE_HANDLE(FEvent);
}

TWinSCPFileSystem::TWinSCPFileSystem(TCustomFarPlugin * APlugin) :
  TCustomFarFileSystem(OBJECT_CLASS_TWinSCPFileSystem, APlugin),
  FTerminal(nullptr),
  FQueue(nullptr),
  FQueueStatus(nullptr),
  FQueueEvent(qeEmpty),
  FProgressSaveScreenHandle(nullptr),
  FSynchronizationSaveScreenHandle(nullptr),
  FAuthenticationSaveScreenHandle(nullptr),
  FSynchronizationCompare(false),
  FFileList(nullptr),
  FPanelItems(nullptr),
  FSavedFindFolder(L""),
  FQueueStatusInvalidated(false),
  FQueueItemInvalidated(false),
  FRefreshLocalDirectory(false),
  FRefreshRemoteDirectory(false),
  FQueueEventPending(false),
  FNoProgress(false),
  FNoProgressFinish(false),
  FKeepaliveThread(nullptr),
  FSynchronisingBrowse(false),
  FSynchronizeController(nullptr),
  FCapturedLog(nullptr),
  FAuthenticationLog(nullptr),
  FLastEditorID(-1),
  FLoadingSessionList(false),
  FPathHistory(new TStringList()),
  FCurrentDirectoryWasChanged(false),

  FReloadDirectory(false),
  FLastMultipleEditReadOnly(false),
  FEditorPendingSave(false),
  FOutputLog(false)
{
}

void TWinSCPFileSystem::Init(TSecureShell * /*SecureShell*/)
{
  TCustomFarFileSystem::Init();
}

TWinSCPFileSystem::~TWinSCPFileSystem()
{
  Disconnect();
  SAFE_DESTROY(FPathHistory);
}

void TWinSCPFileSystem::HandleException(Exception * E, OPERATION_MODES OpMode)
{
  if ((GetTerminal() != nullptr) && isa<EFatal>(E))
  {
    bool Reopen = GetTerminal()->QueryReopen(E, 0, nullptr);
    if (Reopen)
    {
      UpdatePanel();
    }
    else
    {
      if (!FClosed)
      {
        ClosePanel();
      }
      //GetTerminal()->ShowExtendedException(E);
    }
  }
  else
  {
    TCustomFarFileSystem::HandleException(E, OpMode);
  }
}

void TWinSCPFileSystem::KeepaliveThreadCallback()
{
  TGuard Guard(FCriticalSection);

  if (Connected())
  {
    FTerminal->Idle();
  }
}

bool TWinSCPFileSystem::IsSessionList() const
{
  return (FTerminal == nullptr);
}

bool TWinSCPFileSystem::Connected() const
{
  // Check for active added to avoid "disconnected" message popup repeatedly
  // from "idle"
  return !IsSessionList() && FTerminal->GetActive();
}

const TWinSCPPlugin * TWinSCPFileSystem::GetWinSCPPlugin() const
{
  return dyn_cast<const TWinSCPPlugin>(FPlugin);
}

TWinSCPPlugin * TWinSCPFileSystem::GetWinSCPPlugin()
{
  return dyn_cast<TWinSCPPlugin>(FPlugin);
}

void TWinSCPFileSystem::Close()
{
  SCOPE_EXIT
  {
    TCustomFarFileSystem::Close();
  };
  SAFE_DESTROY(FKeepaliveThread);

  if (Connected())
  {
    if (FQueue != nullptr)
    {
      if (!FQueue->GetIsEmpty() &&
          (MoreMessageDialog(GetMsg(PENDING_QUEUE_ITEMS), nullptr, qtWarning,
             qaOK | qaCancel) == qaOK))
      {
        QueueShow(true);
      }
    }
  }
}

void TWinSCPFileSystem::GetOpenPanelInfoEx(OPENPANELINFO_FLAGS &Flags,
  UnicodeString & HostFile, UnicodeString & CurDir, UnicodeString & AFormat,
  UnicodeString & PanelTitle, TFarPanelModes * PanelModes, intptr_t & /*StartPanelMode*/,
  OPENPANELINFO_SORTMODES & /*StartSortMode*/, bool & /*StartSortOrder*/, TFarKeyBarTitles * KeyBarTitles,
  UnicodeString & ShortcutData)
{
  if (!IsSessionList())
  {
    Flags = !OPIF_DISABLEFILTER | !OPIF_DISABLESORTGROUPS | !OPIF_DISABLEHIGHLIGHTING |
      OPIF_SHOWPRESERVECASE | OPIF_COMPAREFATTIME | OPIF_SHORTCUT;

    // When slash is added to the end of path, windows style paths
    // (vandyke: c:/windows/system) are displayed correctly on command-line, but
    // leaved subdirectory is not focused, when entering parent directory.
    HostFile = GetSessionData()->GetHostName(); // GenerateSessionUrl(sufHostKey); // GetSessionData()->GetSessionName();
    CurDir = FTerminal->GetCurrDirectory();
    AFormat = FORMAT(L"netbox:%s", GetSessionData()->GetSessionName().c_str());
    UnicodeString HostName = GetSessionData()->GetHostNameExpanded(); // GetSessionData()->GetSessionName();
    UnicodeString Url = GetSessionData()->GenerateSessionUrl(sufComplete);
    if (GetFarConfiguration()->GetHostNameInTitle())
    {
      PanelTitle = FORMAT(L" %s:%s ", HostName.c_str(), CurDir.c_str());
    }
    else
    {
      PanelTitle = FORMAT(L" %s ", CurDir.c_str());
    }
    ShortcutData = FORMAT(L"netbox:%s\1%s", Url.c_str(), CurDir.c_str());

    TRemoteFilePanelItem::SetPanelModes(PanelModes);
    TRemoteFilePanelItem::SetKeyBarTitles(KeyBarTitles);
  }
  else
  {
    CurDir = FSessionsFolder;
    AFormat = L"netbox";
    Flags = !OPIF_DISABLESORTGROUPS | !OPIF_DISABLEHIGHLIGHTING | OPIF_USEATTRHIGHLIGHTING |
      OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE | OPIF_SHORTCUT;

    PanelTitle = FORMAT(L" %s [/%s]", GetMsg(NB_STORED_SESSION_TITLE).c_str(), FSessionsFolder.c_str());

    TSessionPanelItem::SetPanelModes(PanelModes);
    TSessionPanelItem::SetKeyBarTitles(KeyBarTitles);
  }
}

bool TWinSCPFileSystem::GetFindDataEx(TObjectList * PanelItems, OPERATION_MODES OpMode)
{
  bool Result = false;
  if (Connected())
  {
    DebugAssert(!FNoProgress);
    // OPM_FIND is used also for calculation of directory size (F3, quick view).
    // However directory is usually read from SetDirectory, so FNoProgress
    // seems to have no effect here.
    // Do not know if OPM_SILENT is even used.
    FNoProgress = FLAGSET(OpMode, OPM_FIND) || FLAGSET(OpMode, OPM_SILENT);
    {
      SCOPE_EXIT
      {
        FNoProgress = false;
      };
      if (FReloadDirectory && FTerminal->GetActive())
      {
        FReloadDirectory = false;
        FTerminal->ReloadDirectory();
      }

      TCustomFileSystem * FileSystem = GetTerminal()->GetFileSystem();
      bool ResolveSymlinks = GetSessionData()->GetResolveSymlinks();
      for (intptr_t Index = 0; Index < GetTerminal()->GetFiles()->GetCount(); ++Index)
      {
        TRemoteFile * File = GetTerminal()->GetFiles()->GetFile(Index);
        DebugAssert(File);
        if (ResolveSymlinks && File->GetIsSymLink())
        {
          if (FarPlugin->CheckForEsc())
            break;
          // Check what kind of symlink this is
          const UnicodeString LinkFileName = File->GetLinkTo();
          if (!LinkFileName.IsEmpty())
          {
            TRemoteFile * LinkFile = nullptr;
            try
            {
              FileSystem->ReadFile(LinkFileName, LinkFile);
            }
            catch (const Exception & /*E*/)
            {
              LinkFile = nullptr;
            }
            if ((LinkFile != nullptr) && LinkFile->GetIsDirectory())
            {
              File->SetType(FILETYPE_DIRECTORY);
            }
            SAFE_DESTROY(LinkFile);
          }
        }
        PanelItems->Add(new TRemoteFilePanelItem(File));
      }
    }
    Result = true;
  }
  else if (IsSessionList())
  {
    Result = true;
    DebugAssert(StoredSessions);
    StoredSessions->Load();
    UnicodeString Folder = FSessionsFolder;
    if (!FSessionsFolder.IsEmpty())
    {
      Folder = core::UnixIncludeTrailingBackslash(FSessionsFolder);
    }

    std::unique_ptr<TStringList> ChildPaths(new TStringList());
    ChildPaths->SetCaseSensitive(false);
    for (intptr_t Index = 0; Index < StoredSessions->GetCount(); ++Index)
    {
      const TSessionData * Data = StoredSessions->GetSession(Index);
      UnicodeString SessionName = Data->GetName();
      if (SessionName.SubString(1, Folder.Length()) == Folder)
      {
        UnicodeString Name = SessionName.SubString(
          Folder.Length() + 1, SessionName.Length() - Folder.Length());
        intptr_t Slash = Name.Pos(L'/');
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

    if (!FNewSessionsFolder.IsEmpty())
    {
      PanelItems->Add(new TSessionFolderPanelItem(FNewSessionsFolder));
    }
    if (PanelItems->GetCount() == 0)
    {
      PanelItems->Add(new THintPanelItem(GetMsg(NEW_SESSION_HINT)));
    }

    TWinSCPFileSystem * OppositeFileSystem =
      dyn_cast<TWinSCPFileSystem>(GetOppositeFileSystem());
    if ((OppositeFileSystem != nullptr) && !OppositeFileSystem->Connected() &&
      !OppositeFileSystem->FLoadingSessionList)
    {
      FLoadingSessionList = true;
      {
        SCOPE_EXIT
        {
          FLoadingSessionList = false;
        };
        UpdatePanel(false, true);
        RedrawPanel(true);
      }
    }
    if (!FPrevSessionName.IsEmpty())
    {
      const TSessionData * PrevSession = StoredSessions->GetSessionByName(FPrevSessionName);
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

void TWinSCPFileSystem::DuplicateOrRenameSession(TSessionData * Data,
  bool Duplicate)
{
  DebugAssert(Data);
  UnicodeString Name = Data->GetName();
  if (GetWinSCPPlugin()->InputBox(GetMsg(Duplicate ? DUPLICATE_SESSION_TITLE : RENAME_SESSION_TITLE),
        GetMsg(Duplicate ? DUPLICATE_SESSION_PROMPT : RENAME_SESSION_PROMPT),
        Name, 0) &&
      !Name.IsEmpty() && (Name != Data->GetName()))
  {
    Name = ReplaceChar(Name, L'\\', L'/');
    TNamedObject * EData = StoredSessions->FindByName(Name);
    if ((EData != nullptr) && (EData != Data))
    {
      throw Exception(FORMAT(GetMsg(SESSION_ALREADY_EXISTS_ERROR).c_str(), Name.c_str()));
    }
    else
    {
      TSessionData * NData = StoredSessions->NewSession(Name, Data);
      FSessionsFolder = ::ExcludeTrailingBackslash(core::UnixExtractFilePath(Name));

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
      StoredSessions->Save(/*All*/true, /*Explicit*/true);

      if (UpdatePanel())
      {
        RedrawPanel();

        FocusSession(NData);
      }
    }
  }
}

void TWinSCPFileSystem::FocusSession(const TSessionData * Data)
{
  TFarPanelInfo ** PanelInfo = GetPanelInfo();
  const TFarPanelItem * SessionItem = PanelInfo && *PanelInfo ? (*PanelInfo)->FindUserData(Data) : nullptr;
  if (SessionItem != nullptr)
  {
    (*PanelInfo)->SetFocusedItem(SessionItem);
  }
}

void TWinSCPFileSystem::EditConnectSession(TSessionData * Data, bool Edit)
{
  bool NewData = !Data;
  bool FillInConnect = !Edit && Data && !Data->GetCanLogin();
  if (NewData || FillInConnect)
  {
    Data = new TSessionData(L"");
  }

  SCOPE_EXIT
  {
    if (NewData || FillInConnect)
    {
      SAFE_DESTROY(Data);
    }
  };
  EditConnectSession(Data, Edit, NewData, FillInConnect);
}

void TWinSCPFileSystem::EditConnectSession(TSessionData * Data, bool Edit, bool NewData, bool FillInConnect)
{
  TSessionData * OrigData = Data;
  if (FillInConnect && Data)
  {
    Data->Assign(OrigData);
    Data->SetName(L"");
  }

  TSessionActionEnum Action;
  if (Edit || FillInConnect)
  {
    Action = (FillInConnect ? saConnect : (OrigData == nullptr ? saAdd : saEdit));
    if (SessionDialog(Data, Action))
    {
      if ((!NewData && !FillInConnect) || (Action != saConnect))
      {
        TSessionData * SelectSession = nullptr;
        if (NewData)
        {
          // UnicodeString Name =
          //    IncludeTrailingBackslash(FSessionsFolder) + Data->GetSessionName();
          UnicodeString Name;
          if (!FSessionsFolder.IsEmpty())
          {
            Name = core::UnixIncludeTrailingBackslash(FSessionsFolder);
          }
          if (Data)
            Name += Data->GetSessionName();
          if (GetWinSCPPlugin()->InputBox(GetMsg(NEW_SESSION_NAME_TITLE),
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
              FSessionsFolder = ::ExcludeTrailingBackslash(core::UnixExtractFilePath(Name));
            }
          }
        }
        else if (FillInConnect && OrigData)
        {
          UnicodeString OrigName = OrigData->GetName();
          OrigData->Assign(Data);
          OrigData->SetName(OrigName);
        }

        // modified only, explicit
        StoredSessions->Save(false, true);
        if (UpdatePanel())
        {
          if (SelectSession != nullptr)
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
      TFarPanelInfo ** PanelInfo = GetPanelInfo();
      if (PanelInfo && *PanelInfo && (*PanelInfo)->GetItemCount())
      {
        (*PanelInfo)->SetFocusedIndex(0);
      }
    }
  }
}

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
      if (FQueue != nullptr)
      {
        FQueue->Idle();
      }
      ProcessQueue(true);
    }
    else if ((Event == FE_GOTFOCUS) || (Event == FE_KILLFOCUS))
    {
      Result = true;
    }
    else if (Event == FE_REDRAW)
    {
      if (FCurrentDirectoryWasChanged)
      {
        UpdatePanel();
        FCurrentDirectoryWasChanged = false;
      }
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

void TWinSCPFileSystem::TerminalCaptureLog(
  const UnicodeString & AddedLine, TCaptureOutputType /*OutputEvent*/)
{
  if (FOutputLog)
  {
    GetWinSCPPlugin()->FarWriteConsole(AddedLine + L"\n");
  }
  if (FCapturedLog != nullptr)
  {
    FCapturedLog->Add(AddedLine);
  }
}

void TWinSCPFileSystem::RequireLocalPanel(TFarPanelInfo * Panel, const UnicodeString & Message)
{
  if (Panel->GetIsPlugin() || (Panel->GetType() != ptFile))
  {
    throw Exception(Message);
  }
}

void TWinSCPFileSystem::RequireCapability(intptr_t Capability)
{
  if (!FTerminal->GetIsCapable(static_cast<TFSCapability>(Capability)))
  {
    throw Exception(FORMAT(GetMsg(OPERATION_NOT_SUPPORTED).c_str(),
      FTerminal->GetFileSystemInfo().ProtocolName.c_str()));
  }
}

bool TWinSCPFileSystem::EnsureCommandSessionFallback(TFSCapability Capability)
{
  bool Result = FTerminal->GetIsCapable(Capability) ||
    FTerminal->GetCommandSessionOpened();

  if (!Result)
  {
    if (!GetGUIConfiguration()->GetConfirmCommandSession())
    {
      Result = true;
    }
    else
    {
      TMessageParams Params(0);
      Params.Params = qpNeverAskAgainCheck;
      uintptr_t Answer = MoreMessageDialog(
        FORMAT(GetMsg(PERFORM_ON_COMMAND_SESSION).c_str(),
          FTerminal->GetFileSystemInfo().ProtocolName.c_str(),
          FTerminal->GetFileSystemInfo().ProtocolName.c_str()),
        nullptr,
        qtConfirmation, qaOK | qaCancel, &Params);
      if (Answer == qaNeverAskAgain)
      {
        GetGUIConfiguration()->SetConfirmCommandSession(false);
        GetGUIConfiguration()->DoSave(false, false); // modified, implicit
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

bool TWinSCPFileSystem::ExecuteCommand(const UnicodeString & Command)
{
  if (FTerminal->AllowedAnyCommand(Command) &&
    EnsureCommandSessionFallback(fcAnyCommand))
  {
    FTerminal->BeginTransaction();
    {
      SCOPE_EXIT
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
      };
      FarControl(FCTL_SETCMDLINE, 0, reinterpret_cast<void *>(L""));
      TWinSCPPlugin * WinSCPPlugin =  GetWinSCPPlugin();
      WinSCPPlugin->ShowConsoleTitle(Command);
      {
        SCOPE_EXIT
        {
          //WinSCPPlugin->ScrollTerminalScreen(1);
          WinSCPPlugin->SaveTerminalScreen();
          WinSCPPlugin->ClearConsoleTitle();
        };
        WinSCPPlugin->ShowTerminalScreen();

        FOutputLog = true;
        FTerminal->AnyCommand(Command, nb::bind(&TWinSCPFileSystem::TerminalCaptureLog, this));
      }
    }
  }
  return true;
}

bool TWinSCPFileSystem::ProcessKeyEx(intptr_t Key, uintptr_t ControlState)
{
  bool Handled = false;

  TFarPanelInfo * const * PanelInfo = GetPanelInfo();
  const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;

  if ((Key == 'W') && (ControlState & SHIFTMASK) &&
    (ControlState & ALTMASK))
  {
    GetWinSCPPlugin()->CommandsMenu(true);
    Handled = true;
  }
  else if (IsSessionList())
  {
    TSessionData * Data = nullptr;
    if ((Focused != nullptr) && Focused->GetIsFile() && Focused->GetUserData())
    {
      Data = get_as<TSessionData>(Focused->GetUserData());
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

    if (Key == VK_RETURN && (ControlState == 0 || ControlState & ENHANCED_KEY) && Data)
    {
      EditConnectSession(Data, false);
      Handled = true;
    }

    if (Key == VK_F4 && (ControlState == 0))
    {
      if ((Data != nullptr) || (StoredSessions->GetCount() == 0))
      {
        EditConnectSession(Data, true);
      }
      Handled = true;
    }

    if (Key == VK_F4 && (ControlState & SHIFTMASK))
    {
      EditConnectSession(nullptr, true);
      Handled = true;
    }

    if (((Key == VK_F5) || (Key == VK_F6)) &&
      (ControlState & SHIFTMASK))
    {
      if (Data != nullptr)
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

    if ((Key == VK_INSERT) && (((ControlState & ALTMASK) && (ControlState & SHIFTMASK)) ||
      (((ControlState & CTRLMASK) && (ControlState & ALTMASK)))))
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
      GetFarConfiguration()->GetEditorMultiple())
    {
      MultipleEdit();
      Handled = true;
    }
  }
  return Handled;
}

void TWinSCPFileSystem::CreateLink()
{
  RequireCapability(fcResolveSymlink);
  RequireCapability(fcSymbolicLink);

  bool Edit = false;
  TRemoteFile * File = nullptr;
  UnicodeString FileName;
  UnicodeString PointTo;
  bool SymbolicLink = true;

  TFarPanelInfo * const * PanelInfo = GetPanelInfo();
  const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
  if (Focused && Focused->GetUserData())
  {
    File = get_as<TRemoteFile>(Focused->GetUserData());

    if (File)
    {
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
  }

  if (LinkDialog(FileName, PointTo, SymbolicLink, Edit,
        GetTerminal()->GetIsCapable(fcHardLink)))
  {
    if (Edit)
    {
      DebugAssert(!File || (File->GetFileName() == FileName));
      intptr_t Params = dfNoRecursive;
      GetTerminal()->SetExceptionOnFail(true);
      {
        SCOPE_EXIT
        {
          GetTerminal()->SetExceptionOnFail(false);
        };
        GetTerminal()->RemoteDeleteFile(L"", File, &Params);
      }
    }
    if (File)
      GetTerminal()->CreateLink(FileName, PointTo, SymbolicLink, File->GetIsDirectory());
    if (UpdatePanel())
    {
      RedrawPanel();
    }
  }
}

void TWinSCPFileSystem::TemporarilyDownloadFiles(TStrings * AFileList, TCopyParamType & CopyParam, UnicodeString & TempDir)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);

  TempDir = GetWinSCPPlugin()->GetTemporaryDir();
  if (TempDir.IsEmpty() || !::ForceDirectories(ApiPath(TempDir)))
  {
    throw Exception(FMTLOAD(CREATE_TEMP_DIR_ERROR, TempDir.c_str()));
  }

  FTerminal->SetExceptionOnFail(true);
  {
    SCOPE_EXIT
    {
      FTerminal->SetExceptionOnFail(false);
    };
    try
    {
      FTerminal->CopyToLocal(AFileList, TempDir, &CopyParam, cpTemporary);
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

void TWinSCPFileSystem::ApplyCommand()
{
  TFarPanelInfo ** PanelInfo = this->GetPanelInfo();
  if (PanelInfo && *PanelInfo && (*PanelInfo)->GetSelectedCount(true) == 0)
  {
    MoreMessageDialog(GetMsg(MSG_NO_FILES_SELECTED), nullptr,
      qtInformation, qaOK);
    return;
  }

  std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote, PanelInfo));
  if (FileList.get() != nullptr)
  {
    TFarConfiguration * FarConfiguration = GetFarConfiguration();
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
          TRemoteCustomCommand RemoteCustomCommand(Data, GetTerminal()->GetCurrDirectory());
          TFarInteractiveCustomCommand InteractiveCustomCommand(
            GetWinSCPPlugin(), &RemoteCustomCommand);

          Command = InteractiveCustomCommand.Complete(Command, false);

          {
            SCOPE_EXIT
            {
              (*GetPanelInfo())->ApplySelection();
              if (UpdatePanel())
              {
                RedrawPanel();
              }
            };
            TCaptureOutputEvent OutputEvent = nullptr;
            FOutputLog = false;
            if (FLAGSET(Params, ccShowResults))
            {
              DebugAssert(!FNoProgress);
              FNoProgress = true;
              FOutputLog = true;
              OutputEvent = nb::bind(&TWinSCPFileSystem::TerminalCaptureLog, this);
            }

            if (FLAGSET(Params, ccCopyResults))
            {
              DebugAssert(FCapturedLog == nullptr);
              FCapturedLog = new TStringList();
              OutputEvent = nb::bind(&TWinSCPFileSystem::TerminalCaptureLog, this);
            }
            {
              SCOPE_EXIT
              {
                if (FLAGSET(Params, ccShowResults))
                {
                  FNoProgress = false;
                  GetWinSCPPlugin()->ScrollTerminalScreen(1);
                  GetWinSCPPlugin()->SaveTerminalScreen();
                }

                if (FLAGSET(Params, ccCopyResults))
                {
                  GetWinSCPPlugin()->FarCopyToClipboard(FCapturedLog);
                  SAFE_DESTROY(FCapturedLog);
                }
              };
              if (FLAGSET(Params, ccShowResults))
              {
                GetWinSCPPlugin()->ShowTerminalScreen();
              }

              FTerminal->CustomCommandOnFiles(Command, Params, FileList.get(), OutputEvent);
            }
          }
        }
      }
      else
      {
        TCustomCommandData Data1(GetTerminal());
        TLocalCustomCommand LocalCustomCommand(Data1, GetTerminal()->GetCurrDirectory(), L"");
        TFarInteractiveCustomCommand InteractiveCustomCommand(GetWinSCPPlugin(),
            &LocalCustomCommand);

        Command = InteractiveCustomCommand.Complete(Command, false);

        std::unique_ptr<TStrings> LocalFileList;
        std::unique_ptr<TStrings> RemoteFileList;
        {
          bool FileListCommand = LocalCustomCommand.IsFileListCommand(Command);
          bool LocalFileCommand = LocalCustomCommand.HasLocalFileName(Command);

          if (LocalFileCommand)
          {
            TFarPanelInfo ** AnotherPanel = GetAnotherPanelInfo();
            RequireLocalPanel(*AnotherPanel, GetMsg(APPLY_COMMAND_LOCAL_PATH_REQUIRED));

            LocalFileList.reset(CreateSelectedFileList(osLocal, AnotherPanel));

            if (FileListCommand)
            {
              if ((LocalFileList.get() == nullptr) || (LocalFileList->GetCount() != 1))
              {
                throw Exception(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH1));
              }
            }
            else
            {
              if ((LocalFileList.get() == nullptr) ||
                ((LocalFileList->GetCount() != 1) &&
                  (FileList->GetCount() != 1) &&
                  (LocalFileList->GetCount() != FileList->GetCount())))
              {
                throw Exception(GetMsg(CUSTOM_COMMAND_SELECTED_UNMATCH));
              }
            }
          }

          UnicodeString TempDir;

          TemporarilyDownloadFiles(FileList.get(), GetGUIConfiguration()->GetDefaultCopyParam(), TempDir);

          {
            SCOPE_EXIT
            {
              RecursiveDeleteFile(::ExcludeTrailingBackslash(TempDir), false);
            };
            RemoteFileList.reset(new TStringList());

            TMakeLocalFileListParams MakeFileListParam;
            MakeFileListParam.FileList = RemoteFileList.get();
            MakeFileListParam.IncludeDirs = FLAGSET(Params, ccApplyToDirectories);
            MakeFileListParam.Recursive =
              FLAGSET(Params, ccRecursive) && !FileListCommand;

            ProcessLocalDirectory(TempDir, nb::bind(&TTerminal::MakeLocalFileList, FTerminal), &MakeFileListParam);

            TFileOperationProgressType Progress(nb::bind(&TWinSCPFileSystem::OperationProgress, this), nb::bind(&TWinSCPFileSystem::OperationFinished, this));

            Progress.Start(foCustomCommand, osRemote, static_cast<intptr_t>(FileListCommand ? 1 : FileList->GetCount()));
            {
              SCOPE_EXIT
              {
                Progress.Stop();
              };
              if (FileListCommand)
              {
                UnicodeString LocalFile;
                UnicodeString FileList2 = core::MakeFileList(RemoteFileList.get());

                if (LocalFileCommand)
                {
                  DebugAssert(LocalFileList->GetCount() == 1);
                  LocalFile = LocalFileList->GetString(0);
                }

                TCustomCommandData Data2(FTerminal);
                TLocalCustomCommand CustomCommand(Data2,
                  GetTerminal()->GetCurrDirectory(), L"", L"", LocalFile, FileList2);
                ExecuteShellAndWait(GetWinSCPPlugin()->GetHandle(), CustomCommand.Complete(Command, true),
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
                      GetTerminal()->GetCurrDirectory(), FileName, L"", LocalFile, L"");
                    ExecuteShellAndWait(GetWinSCPPlugin()->GetHandle(),
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
                      Data4, GetTerminal()->GetCurrDirectory(),
                      L"", FileName, LocalFileList->GetString(Index), L"");
                    ExecuteShellAndWait(GetWinSCPPlugin()->GetHandle(),
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
                      Data5, GetTerminal()->GetCurrDirectory(),
                      L"", FileName, LocalFileList->GetString(Index), L"");
                    ExecuteShellAndWait(GetWinSCPPlugin()->GetHandle(),
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
                    GetTerminal()->GetCurrDirectory(), L"", RemoteFileList->GetString(Index), L"", L"");
                  ExecuteShellAndWait(GetWinSCPPlugin()->GetHandle(),
                    CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                }
              }
            }
          }
        }
      }
    }
  }
}

void TWinSCPFileSystem::Synchronize(const UnicodeString & LocalDirectory,
  const UnicodeString & RemoteDirectory, TTerminal::TSynchronizeMode Mode,
  const TCopyParamType & CopyParam, intptr_t Params, TSynchronizeChecklist ** AChecklist,
  TSynchronizeOptions * Options)
{
  TSynchronizeChecklist * Checklist = nullptr;
  {
    SCOPE_EXIT
    {
      if (AChecklist == nullptr)
      {
        SAFE_DESTROY(Checklist);
      }
      else
      {
        *AChecklist = Checklist;
      }
    };
    GetWinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
    GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = true;
    {
      SCOPE_EXIT
      {
        GetWinSCPPlugin()->ClearConsoleTitle();
        GetWinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
      };
      {
        Checklist = FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
          Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
          nb::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this), Options);
      }
    }

    GetWinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
    GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = false;
    {
      SCOPE_EXIT
      {
        GetWinSCPPlugin()->ClearConsoleTitle();
        GetWinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
      };
      FTerminal->SynchronizeApply(Checklist, LocalDirectory, RemoteDirectory,
        &CopyParam, Params | TTerminal::spNoConfirmation,
        nb::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this));
    }
  }
}

bool TWinSCPFileSystem::SynchronizeAllowSelectedOnly()
{
  return
    ((*GetPanelInfo())->GetSelectedCount() > 0) ||
    ((*GetAnotherPanelInfo())->GetSelectedCount() > 0);
}

void TWinSCPFileSystem::GetSynchronizeOptions(
  intptr_t Params, TSynchronizeOptions & Options)
{
  if (FLAGSET(Params, spSelectedOnly) && SynchronizeAllowSelectedOnly())
  {
    Options.Filter = new TStringList();
    Options.Filter->SetCaseSensitive(false);
    Options.Filter->SetDuplicates(dupAccept);

    TFarPanelInfo ** PanelInfo = GetPanelInfo();
    if (PanelInfo && *PanelInfo && (*PanelInfo)->GetSelectedCount() > 0)
    {
      CreateFileList((*PanelInfo)->GetItems(), osRemote, true, L"", true, Options.Filter);
    }
    if ((*GetAnotherPanelInfo())->GetSelectedCount() > 0)
    {
      CreateFileList((*GetAnotherPanelInfo())->GetItems(), osLocal, true, L"", true, Options.Filter);
    }
    Options.Filter->Sort();
  }
}

void TWinSCPFileSystem::FullSynchronize(bool Source)
{
  TFarPanelInfo ** AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(*AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  UnicodeString LocalDirectory = (*AnotherPanel)->GetCurrDirectory();
  UnicodeString RemoteDirectory = FTerminal->GetCurrDirectory();

  bool SaveMode = !(GetGUIConfiguration()->GetSynchronizeModeAuto() < 0);
  TTerminal::TSynchronizeMode Mode =
    SaveMode ? static_cast<TTerminal::TSynchronizeMode>(GetGUIConfiguration()->GetSynchronizeModeAuto()) :
      (Source ? TTerminal::smLocal : TTerminal::smRemote);
  intptr_t Params = GetGUIConfiguration()->GetSynchronizeParams();
  bool SaveSettings = false;

  TGUICopyParamType CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  TUsableCopyParamAttrs CopyParamAttrs = GetTerminal()->UsableCopyParamAttrs(0);
  intptr_t Options =
    FLAGMASK(!FTerminal->GetIsCapable(fcTimestampChanging), fsoDisableTimestamp) |
    FLAGMASK(SynchronizeAllowSelectedOnly(), fsoAllowSelectedOnly);
  if (FullSynchronizeDialog(Mode, Params, LocalDirectory, RemoteDirectory,
      &CopyParam, SaveSettings, SaveMode, Options, CopyParamAttrs))
  {
    TSynchronizeOptions SynchronizeOptions;
    GetSynchronizeOptions(Params, SynchronizeOptions);

    if (SaveSettings)
    {
      GetGUIConfiguration()->SetSynchronizeParams(Params);
      if (SaveMode)
      {
        GetGUIConfiguration()->SetSynchronizeModeAuto(Mode);
      }
    }

    std::unique_ptr<TSynchronizeChecklist> Checklist;
    {
      SCOPE_EXIT
      {
        if (UpdatePanel())
        {
          RedrawPanel();
        }
      };
      GetWinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
      GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
      FSynchronizationStart = Now();
      FSynchronizationCompare = true;
      {
        SCOPE_EXIT
        {
          GetWinSCPPlugin()->ClearConsoleTitle();
          GetWinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
        };
        Checklist.reset(FTerminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
          Mode, &CopyParam, Params | TTerminal::spNoConfirmation,
          nb::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this), &SynchronizeOptions));
      }

      if (Checklist.get() && Checklist->GetCount() == 0)
      {
        MoreMessageDialog(GetMsg(COMPARE_NO_DIFFERENCES), nullptr,
           qtInformation, qaOK);
      }
      else if (FLAGCLEAR(Params, TTerminal::spPreviewChanges) ||
               SynchronizeChecklistDialog(Checklist.get(), Mode, Params,
                 LocalDirectory, RemoteDirectory))
      {
        if (FLAGSET(Params, TTerminal::spPreviewChanges))
        {
          FSynchronizationStart = Now();
        }
        GetWinSCPPlugin()->SaveScreen(FSynchronizationSaveScreenHandle);
        GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(SYNCHRONIZE_PROGRESS_TITLE));
        FSynchronizationStart = Now();
        FSynchronizationCompare = false;
        {
          SCOPE_EXIT
          {
            GetWinSCPPlugin()->ClearConsoleTitle();
            GetWinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
          };
          FTerminal->SynchronizeApply(Checklist.get(), LocalDirectory, RemoteDirectory,
            &CopyParam, Params | TTerminal::spNoConfirmation,
            nb::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this));
        }
      }
    }
  }
}

void TWinSCPFileSystem::TerminalSynchronizeDirectory(
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  bool & Continue, bool Collect)
{
  static uint32_t LastTicks;
  uint32_t Ticks = ::GetTickCount();
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

    Message = LocalLabel + core::MinimizeName(LocalDirectory,
      ProgressWidth - LocalLabel.Length(), false);
    Message += ::StringOfChar(L' ', ProgressWidth - Message.Length()) + L"\n";
    Message += RemoteLabel + core::MinimizeName(RemoteDirectory,
      ProgressWidth - RemoteLabel.Length(), true) + L"\n";
    Message += StartTimeLabel + FSynchronizationStart.TimeString(false) + L"\n";
    Message += TimeElapsedLabel +
      FormatDateTimeSpan(GetConfiguration()->GetTimeFormat(), TDateTime(Now() - FSynchronizationStart)) + L"\n";

    GetWinSCPPlugin()->Message(0, (Collect ? ProgressTitleCompare : ProgressTitle), Message);

    if (GetWinSCPPlugin()->CheckForEsc() &&
        (MoreMessageDialog(GetMsg(CANCEL_OPERATION), nullptr,
          qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      Continue = false;
    }
  }
}

void TWinSCPFileSystem::Synchronize()
{
  TFarPanelInfo ** AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(*AnotherPanel, GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  TSynchronizeParamType Params;
  Params.LocalDirectory = (*AnotherPanel)->GetCurrDirectory();
  Params.RemoteDirectory = FTerminal->GetCurrDirectory();
  intptr_t UnusedParams = (GetGUIConfiguration()->GetSynchronizeParams() &
    (TTerminal::spPreviewChanges | TTerminal::spTimestamp |
      TTerminal::spNotByTime | TTerminal::spBySize));
  Params.Params = GetGUIConfiguration()->GetSynchronizeParams() & ~UnusedParams;
  Params.Options = GetGUIConfiguration()->GetSynchronizeOptions();
  TSynchronizeController Controller(
    nb::bind(&TWinSCPFileSystem::DoSynchronize, this),
    nb::bind(&TWinSCPFileSystem::DoSynchronizeInvalid, this),
    nb::bind(&TWinSCPFileSystem::DoSynchronizeTooManyDirectories, this));
  DebugAssert(FSynchronizeController == nullptr);
  FSynchronizeController = &Controller;

  {
    SCOPE_EXIT
    {
      FSynchronizeController = nullptr;
      // plugin might have been closed during some synchronization already
      if (!FClosed)
      {
        if (UpdatePanel())
        {
          RedrawPanel();
        }
      }
    };
    bool SaveSettings = false;
    TCopyParamType CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
    DWORD CopyParamAttrs = GetTerminal()->UsableCopyParamAttrs(0).Upload;
    uintptr_t Options =
      FLAGMASK(SynchronizeAllowSelectedOnly(), soAllowSelectedOnly);
    if (SynchronizeDialog(Params, &CopyParam,
        nb::bind(&TSynchronizeController::StartStop, &Controller),
        SaveSettings, Options, CopyParamAttrs,
        nb::bind(&TWinSCPFileSystem::GetSynchronizeOptions, this)) &&
        SaveSettings)
    {
      GetGUIConfiguration()->SetSynchronizeParams(Params.Params | UnusedParams);
      GetGUIConfiguration()->SetSynchronizeOptions(Params.Options);
    }
  }
}

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
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    throw;
  }
}

void TWinSCPFileSystem::DoSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, const UnicodeString & Directory,
  const UnicodeString & /*ErrorStr*/)
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

  MoreMessageDialog(Message, nullptr, qtError, qaOK);
}

void TWinSCPFileSystem::DoSynchronizeTooManyDirectories(
  TSynchronizeController * /*Sender*/, intptr_t & MaxDirectories)
{
  if (MaxDirectories < GetGUIConfiguration()->GetMaxWatchDirectories())
  {
    MaxDirectories = GetGUIConfiguration()->GetMaxWatchDirectories();
  }
  else
  {
    TMessageParams Params(0);
    Params.Params = qpNeverAskAgainCheck;
    uintptr_t Result = MoreMessageDialog(
      FORMAT(GetMsg(TOO_MANY_WATCH_DIRECTORIES).c_str(), MaxDirectories, MaxDirectories), nullptr,
      qtConfirmation, qaYes | qaNo, &Params);

    if ((Result == qaYes) || (Result == qaNeverAskAgain))
    {
      MaxDirectories *= 2;
      if (Result == qaNeverAskAgain)
      {
        GetGUIConfiguration()->SetMaxWatchDirectories(MaxDirectories);
      }
    }
    else
    {
      Abort();
    }
  }
}

void TWinSCPFileSystem::CustomCommandGetParamValue(
  const UnicodeString & AName, UnicodeString & Value)
{
  UnicodeString Name = AName;
  if (Name.IsEmpty())
  {
    Name = GetMsg(APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!GetWinSCPPlugin()->InputBox(GetMsg(APPLY_COMMAND_PARAM_TITLE),
        Name, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}

void TWinSCPFileSystem::TransferFiles(bool Move)
{
  if (Move)
  {
    RequireCapability(fcRemoteMove);
  }

  if (Move || EnsureCommandSessionFallback(fcRemoteCopy))
  {
    std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote));
    if (FileList.get())
    {
      DebugAssert(!FPanelItems);
      UnicodeString Target = FTerminal->GetCurrDirectory();
      UnicodeString FileMask = L"*.*";
      if (FileList->GetCount() == 1)
        FileMask = base::UnixExtractFileName(FileList->GetString(0));
      if (RemoteTransferDialog(FileList.get(), Target, FileMask, Move))
      {
        SCOPE_EXIT
        {
          (*GetPanelInfo())->ApplySelection();
          if (UpdatePanel())
          {
            RedrawPanel();
          }
        };
        if (Move)
        {
          GetTerminal()->MoveFiles(FileList.get(), Target, FileMask);
        }
        else
        {
          GetTerminal()->CopyFiles(FileList.get(), Target, FileMask);
        }
      }
    }
  }
}

void TWinSCPFileSystem::RenameFile()
{
  TFarPanelInfo * const * PanelInfo = GetPanelInfo();
  const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
  DebugAssert(Focused != nullptr);

  if (Focused && !Focused->GetIsParentDirectory())
  {
    RequireCapability(fcRename);

    TRemoteFile * File = get_as<TRemoteFile>(Focused->GetUserData());
    UnicodeString NewName = File->GetFileName();
    if (RenameFileDialog(File, NewName))
    {
      SCOPE_EXIT
      {
        if (UpdatePanel())
        {
          RedrawPanel();
        }
      };
      GetTerminal()->TerminalRenameFile(File, NewName, true);
    }
  }
}

void TWinSCPFileSystem::FileProperties()
{
  std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote));
  if (FileList.get())
  {
    DebugAssert(!FPanelItems);
    TRemoteProperties CurrentProperties;

    bool Cont = true;
    if (!GetTerminal()->LoadFilesProperties(FileList.get()))
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
      CurrentProperties = TRemoteProperties::CommonProperties(FileList.get());

      intptr_t Flags = 0;
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
      if (PropertiesDialog(FileList.get(), FTerminal->GetCurrDirectory(),
          FTerminal->GetGroups(), FTerminal->GetUsers(), &NewProperties, Flags))
      {
        NewProperties = TRemoteProperties::ChangedProperties(CurrentProperties,
          NewProperties);
        SCOPE_EXIT
        {
          (*GetPanelInfo())->ApplySelection();
          if (UpdatePanel())
          {
            RedrawPanel();
          }
        };
        FTerminal->ChangeFilesProperties(FileList.get(), &NewProperties);
      }
    }
  }
}

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

void TWinSCPFileSystem::InsertSessionNameOnCommandLine()
{
  TFarPanelInfo * const * PanelInfo = GetPanelInfo();
  const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;

  if (Focused != nullptr)
  {
    TSessionData * SessionData = get_as<TSessionData>(Focused->GetUserData());
    UnicodeString Name;
    if (SessionData != nullptr)
    {
      Name = SessionData->GetName();
    }
    else
    {
      Name = core::UnixIncludeTrailingBackslash(FSessionsFolder);
      if (!Focused->GetIsParentDirectory())
      {
        Name = core::UnixIncludeTrailingBackslash(Name + Focused->GetFileName());
      }
    }
    InsertTokenOnCommandLine(Name, true);
  }
}

void TWinSCPFileSystem::InsertFileNameOnCommandLine(bool Full)
{
  TFarPanelInfo * const * PanelInfo = GetPanelInfo();
  const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;

  if (Focused != nullptr)
  {
    if (!Focused->GetIsParentDirectory())
    {
      const TRemoteFile * File = reinterpret_cast<const TRemoteFile *>(Focused->GetUserData());
      if (File != nullptr)
      {
        UnicodeString Path;
        if (Full)
        {
          // Get full address (with server address)
          Path = GetFullFilePath(File);
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
      InsertTokenOnCommandLine(core::UnixIncludeTrailingBackslash(FTerminal->GetCurrDirectory()), true);
    }
  }
}

UnicodeString TWinSCPFileSystem::GetFullFilePath(const TRemoteFile * AFile) const
{
  UnicodeString SessionUrl = GetSessionUrl(FTerminal, true);
  UnicodeString Result = FORMAT(L"%s%s", SessionUrl.c_str(), AFile->GetFullFileName().c_str());
  return Result;
}

// not used
void TWinSCPFileSystem::InsertPathOnCommandLine()
{
  InsertTokenOnCommandLine(FTerminal->GetCurrDirectory(), false);
}

void TWinSCPFileSystem::CopyFullFileNamesToClipboard()
{
  std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote));
  std::unique_ptr<TStrings> FileNames(new TStringList());
  if (FileList.get() != nullptr)
  {
    for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      const TRemoteFile * File = reinterpret_cast<const TRemoteFile *>(FileList->GetObj(Index));
      if (File != nullptr)
      {
        FileNames->Add(GetFullFilePath(File));
      }
      else
      {
        DebugAssert(false);
      }
    }
  }
  else
  {
    TFarPanelInfo * const * PanelInfo = GetPanelInfo();
    const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
    if (PanelInfo && *PanelInfo && ((*PanelInfo)->GetSelectedCount() == 0) &&
      Focused->GetIsParentDirectory())
    {
      FileNames->Add(core::UnixIncludeTrailingBackslash(FTerminal->GetCurrDirectory()));
    }
  }

  GetWinSCPPlugin()->FarCopyToClipboard(FileNames.get());
}

void TWinSCPFileSystem::GetSpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & ASpaceAvailable, bool & Close)
{
  // terminal can be already closed (e.g. dropped connection)
  if ((GetTerminal() != nullptr) && GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    try
    {
      GetTerminal()->SpaceAvailable(APath, ASpaceAvailable);
    }
    catch (Exception & E)
    {
      if (!GetTerminal()->GetActive())
      {
        Close = true;
      }
      DEBUG_PRINTF("before HandleException");
      HandleException(&E);
    }
  }
}

void TWinSCPFileSystem::ShowInformation()
{
  const TSessionInfo & SessionInfo = GetTerminal()->GetSessionInfo();
  TFileSystemInfo FileSystemInfo = GetTerminal()->GetFileSystemInfo();
  TGetSpaceAvailableEvent OnGetSpaceAvailable;
  if (GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    OnGetSpaceAvailable = nb::bind(&TWinSCPFileSystem::GetSpaceAvailable, this);
  }
  FileSystemInfoDialog(SessionInfo, FileSystemInfo, GetTerminal()->GetCurrDirectory(),
    OnGetSpaceAvailable);
}

bool TWinSCPFileSystem::AreCachesEmpty() const
{
  DebugAssert(Connected());
  return FTerminal->GetAreCachesEmpty();
}

void TWinSCPFileSystem::ClearCaches()
{
  DebugAssert(Connected());
  FTerminal->ClearCaches();
}

void TWinSCPFileSystem::OpenSessionInPutty()
{
  DebugAssert(Connected());
  ::OpenSessionInPutty(GetGUIConfiguration()->GetPuttyPath(), GetSessionData());
}

void TWinSCPFileSystem::QueueShow(bool ClosingPlugin)
{
  DebugAssert(Connected());
  TTerminalQueueStatus * QueueStatus = GetQueueStatus();
  DebugAssert(QueueStatus != nullptr);
  QueueDialog(QueueStatus, ClosingPlugin);
  ProcessQueue(true);
}

void TWinSCPFileSystem::OpenDirectory(bool Add)
{
  std::unique_ptr<TBookmarkList> BookmarkList(new TBookmarkList());
  UnicodeString Directory = FTerminal->GetCurrDirectory();
  UnicodeString SessionKey = GetSessionData()->GetSessionKey();
  TBookmarkList * CurrentBookmarkList;

  CurrentBookmarkList = GetFarConfiguration()->GetBookmarks(SessionKey);
  if (CurrentBookmarkList != nullptr)
  {
    BookmarkList->Assign(CurrentBookmarkList);
  }

  if (Add)
  {
    TBookmark * Bookmark = new TBookmark();
    Bookmark->SetRemote(Directory);
    Bookmark->SetName(Directory);
    BookmarkList->Add(Bookmark);
    GetFarConfiguration()->SetBookmarks(SessionKey, BookmarkList.get());
  }

  bool Result = OpenDirectoryDialog(Add, Directory, BookmarkList.get());

  GetFarConfiguration()->SetBookmarks(SessionKey, BookmarkList.get());
  GetConfiguration()->Save();

  if (Result)
  {
    FTerminal->RemoteChangeDirectory(Directory);
    if (UpdatePanel(true))
    {
      RedrawPanel();
    }
  }
}

void TWinSCPFileSystem::HomeDirectory()
{
  FTerminal->HomeDirectory();
  if (UpdatePanel(true))
  {
    RedrawPanel();
  }
}

bool TWinSCPFileSystem::IsSynchronizedBrowsing() const
{
  return FSynchronisingBrowse;
}

void TWinSCPFileSystem::ToggleSynchronizeBrowsing()
{
  FSynchronisingBrowse = !FSynchronisingBrowse;

  if (GetFarConfiguration()->GetConfirmSynchronizedBrowsing())
  {
    UnicodeString Message = FSynchronisingBrowse ?
      GetMsg(SYNCHRONIZE_BROWSING_ON) : GetMsg(SYNCHRONIZE_BROWSING_OFF);
    TMessageParams Params(0);
    Params.Params = qpNeverAskAgainCheck;
    if (MoreMessageDialog(Message, nullptr, qtInformation, qaOK, &Params) ==
      qaNeverAskAgain)
    {
      GetFarConfiguration()->SetConfirmSynchronizedBrowsing(false);
    }
  }
}

bool TWinSCPFileSystem::SynchronizeBrowsing(const UnicodeString & NewPath)
{
  bool Result;
  TFarPanelInfo ** AnotherPanel = GetAnotherPanelInfo();
  UnicodeString OldPath = AnotherPanel && *AnotherPanel ? (*AnotherPanel)->GetCurrDirectory() : L"";
  // IncludeTrailingBackslash to expand C: to C:\.
  UnicodeString LocalPath = ::IncludeTrailingBackslash(NewPath);
  FarPanelDirectory fpd;
  ClearStruct(fpd);
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
    if (AnotherPanel && *AnotherPanel && !ComparePaths((*AnotherPanel)->GetCurrDirectory(), NewPath))
    {
      // FAR WORKAROUND
      // If FCTL_SETPANELDIR above fails, Far default current
      // directory to initial (?) one. So move this back to
      // previous directory.
      FarPanelDirectory fpd;
      ClearStruct(fpd);
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

bool TWinSCPFileSystem::SetDirectoryEx(const UnicodeString & Dir, OPERATION_MODES OpMode)
{
  if (!IsSessionList() && !Connected())
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
      SCOPE_EXIT
      {
        // Result = SetDirectoryEx(Dir, OpMode);
        FSavedFindFolder.Clear();
      };
      Result = SetDirectoryEx(FSavedFindFolder, OpMode);
      return Result;
    }
  }
  else
  {
    if ((OpMode & OPM_FIND) && FSavedFindFolder.IsEmpty() && FTerminal)
    {
      FSavedFindFolder = FTerminal->GetCurrDirectory();
    }

    if (IsSessionList())
    {
      FSessionsFolder = core::AbsolutePath(ROOTDIRECTORY + FSessionsFolder, Dir);
      DebugAssert(FSessionsFolder[1] == L'/');
      FSessionsFolder.Delete(1, 1);
      FNewSessionsFolder.Clear();
    }
    else
    {
      DebugAssert(!FNoProgress);
      bool Normal = FLAGCLEAR(OpMode, OPM_FIND | OPM_SILENT);
      UnicodeString PrevPath = FTerminal ? FTerminal->GetCurrDirectory() : L"";
      FNoProgress = !Normal;
      if (!FNoProgress)
      {
        GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(CHANGING_DIRECTORY_TITLE));
      }
      if (FTerminal)
      {
        FTerminal->SetExceptionOnFail(true);
      }
      {
        SCOPE_EXIT
        {
          if (FTerminal)
          {
            FTerminal->SetExceptionOnFail(false);
          }
          if (!FNoProgress)
          {
            GetWinSCPPlugin()->ClearConsoleTitle();
          }
          FNoProgress = false;
        };
        if (Dir == L"\\")
        {
          FTerminal->RemoteChangeDirectory(ROOTDIRECTORY);
        }
        else if ((Dir == PARENTDIRECTORY) && (FTerminal->GetCurrDirectory() == ROOTDIRECTORY))
        {
          // ClosePanel();
          Disconnect();
        }
        else
        {
          FTerminal->RemoteChangeDirectory(Dir);
          FCurrentDirectoryWasChanged = true;
        }
      }

      if (FTerminal && Normal && FSynchronisingBrowse &&
        (PrevPath != FTerminal->GetCurrDirectory()))
      {
        TFarPanelInfo ** AnotherPanel = GetAnotherPanelInfo();
        if (AnotherPanel && *AnotherPanel && ((*AnotherPanel)->GetIsPlugin() || ((*AnotherPanel)->GetType() != ptFile)))
        {
          MoreMessageDialog(GetMsg(SYNCHRONIZE_LOCAL_PATH_REQUIRED), nullptr, qtError, qaOK);
        }
        else if (AnotherPanel && *AnotherPanel)
        {
          try
          {
            UnicodeString RemotePath = core::UnixIncludeTrailingBackslash(FTerminal->GetCurrDirectory());
            UnicodeString FullPrevPath = core::UnixIncludeTrailingBackslash(PrevPath);
            UnicodeString LocalPath;
            if (RemotePath.SubString(1, FullPrevPath.Length()) == FullPrevPath && AnotherPanel)
            {
              LocalPath = ::IncludeTrailingBackslash((*AnotherPanel)->GetCurrDirectory()) +
                core::FromUnixPath(RemotePath.SubString(FullPrevPath.Length() + 1,
                  RemotePath.Length() - FullPrevPath.Length()));
            }
            else if (FullPrevPath.SubString(1, RemotePath.Length()) == RemotePath && AnotherPanel)
            {
              UnicodeString NewLocalPath;
              LocalPath = ::ExcludeTrailingBackslash((*AnotherPanel)->GetCurrDirectory());
              while (!core::UnixSamePath(FullPrevPath, RemotePath))
              {
                NewLocalPath = ::ExcludeTrailingBackslash(::ExtractFileDir(LocalPath));
                if (NewLocalPath == LocalPath)
                {
                  Abort();
                }
                LocalPath = NewLocalPath;
                FullPrevPath = core::UnixExtractFilePath(core::UnixExcludeTrailingBackslash(FullPrevPath));
              }
            }
            else
            {
              Abort();
            }

            if (!SynchronizeBrowsing(LocalPath))
            {
              if (MoreMessageDialog(FORMAT(GetMsg(SYNC_DIR_BROWSE_CREATE).c_str(), LocalPath.c_str()),
                    nullptr, qtInformation, qaYes | qaNo) == qaYes)
              {
                if (!::ForceDirectories(ApiPath(LocalPath)))
                {
                  ::RaiseLastOSError();
                }
                else
                {
                  if (!SynchronizeBrowsing(LocalPath))
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
          catch (Exception & E)
          {
            FSynchronisingBrowse = false;
            GetWinSCPPlugin()->ShowExtendedException(&E);
            MoreMessageDialog(GetMsg(SYNC_DIR_BROWSE_ERROR), nullptr, qtInformation, qaOK);
          }
        }
      }
    }

    return true;
  }
}

intptr_t TWinSCPFileSystem::MakeDirectoryEx(UnicodeString & Name, OPERATION_MODES OpMode)
{
  if (Connected())
  {
    DebugAssert(!(OpMode & OPM_SILENT) || !Name.IsEmpty());

    TRemoteProperties Properties = GetGUIConfiguration()->GetNewDirectoryProperties();
    bool SaveSettings = false;

    if ((OpMode & OPM_SILENT) ||
      CreateDirectoryDialog(Name, &Properties, SaveSettings))
    {
      if (SaveSettings)
      {
        GetGUIConfiguration()->SetNewDirectoryProperties(Properties);
      }

      GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(CREATING_FOLDER));
      SCOPE_EXIT
      {
        GetWinSCPPlugin()->ClearConsoleTitle();
      };
      FTerminal->RemoteCreateDirectory(Name, &Properties);
      return 1;
    }
    else
    {
      Name.Clear();
      return -1;
    }
  }
  else if (IsSessionList())
  {
    DebugAssert(!(OpMode & OPM_SILENT) || !Name.IsEmpty());

    if (((OpMode & OPM_SILENT) ||
         GetWinSCPPlugin()->InputBox(GetMsg(CREATE_FOLDER_TITLE),
           ::StripHotkey(GetMsg(CREATE_FOLDER_PROMPT)),
           Name, 0, MAKE_SESSION_FOLDER_HISTORY)) &&
        !Name.IsEmpty())
    {
      TSessionData::ValidateName(Name);
      FNewSessionsFolder = Name;
      return 1;
    }
    else
    {
      Name.Clear();
      return -1;
    }
  }
  else
  {
    Name.Clear();
    return -1;
  }
}

void TWinSCPFileSystem::DeleteSession(TSessionData * Data, void * /*AParam*/)
{
  Data->Remove();
  StoredSessions->Remove(Data);
}

void TWinSCPFileSystem::ProcessSessions(TObjectList * PanelItems,
  TProcessSessionEvent ProcessSession, void * AParam)
{
  for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
    DebugAssert(PanelItem);
    if (PanelItem->GetIsFile())
    {
      if (PanelItem->GetUserData() != nullptr)
      {
        ProcessSession(get_as<TSessionData>(PanelItem->GetUserData()), AParam);
        PanelItem->SetSelected(false);
      }
      else
      {
        UnicodeString Msg = GetMsg(NEW_SESSION_HINT);
        DebugAssert(PanelItem->GetFileName() == Msg);
      }
    }
    else
    {
      DebugAssert(PanelItem->GetUserData() == nullptr);
      UnicodeString Folder = core::UnixIncludeTrailingBackslash(
        core::UnixIncludeTrailingBackslash(FSessionsFolder) + PanelItem->GetFileName());
      intptr_t Index2 = 0;
      while (Index2 < StoredSessions->GetCount())
      {
        TSessionData * Data = StoredSessions->GetSession(Index2);
        if (Data->GetName().SubString(1, Folder.Length()) == Folder)
        {
          ProcessSession(Data, AParam);
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

bool TWinSCPFileSystem::DeleteFilesEx(TObjectList * PanelItems, OPERATION_MODES OpMode)
{
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osRemote);
    FPanelItems = PanelItems;
    SCOPE_EXIT
    {
      FPanelItems = nullptr;
      SAFE_DESTROY(FFileList);
    };
    UnicodeString Query;
    bool Recycle = GetSessionData()->GetDeleteToRecycleBin() &&
      !FTerminal->IsRecycledFile(FFileList->GetString(0));  //-V522
    if (PanelItems->GetCount() > 1)
    {
      Query = FORMAT(GetMsg(Recycle ? RECYCLE_FILES_CONFIRM : DELETE_FILES_CONFIRM).c_str(),
        PanelItems->GetCount());
    }
    else
    {
      Query = FORMAT(GetMsg(Recycle ? RECYCLE_FILE_CONFIRM : DELETE_FILE_CONFIRM).c_str(),
        PanelItems->GetAs<TFarPanelItem>(0)->GetFileName().c_str());
    }

    if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
      (MoreMessageDialog(Query, nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      FTerminal->RemoteDeleteFiles(FFileList);
    }
    return true;
  }
  else if (IsSessionList())
  {
    if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
      (MoreMessageDialog(GetMsg(DELETE_SESSIONS_CONFIRM), nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      ProcessSessions(PanelItems, nb::bind(&TWinSCPFileSystem::DeleteSession, this), nullptr);
    }
    return true;
  }
  else
  {
    return false;
  }
}

void TWinSCPFileSystem::QueueAddItem(TQueueItem * Item)
{
  GetFarConfiguration()->CacheFarSettings();
  GetQueue()->AddItem(Item);
}

struct TExportSessionParam
{
  UnicodeString DestPath;
};

intptr_t TWinSCPFileSystem::GetFilesEx(TObjectList * PanelItems, bool Move,
  UnicodeString & DestPath, OPERATION_MODES OpMode)
{
  intptr_t Result = -1;
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osRemote);
    SCOPE_EXIT
    {
      FPanelItems = nullptr;
      SAFE_DESTROY(FFileList);
    };
    Result = GetFilesRemote(PanelItems, Move, DestPath, OpMode);
  }
  else if (IsSessionList())
  {
    UnicodeString Title = GetMsg(EXPORT_SESSION_TITLE);
    UnicodeString Prompt;
    if (PanelItems->GetCount() == 1)
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSION_PROMPT).c_str(),
        PanelItems->GetAs<TFarPanelItem>(0)->GetFileName().c_str());
    }
    else
    {
      Prompt = FORMAT(GetMsg(EXPORT_SESSIONS_PROMPT).c_str(), PanelItems->GetCount());
    }

    bool AResult = (OpMode & OPM_SILENT) ||
      GetWinSCPPlugin()->InputBox(Title, Prompt, DestPath, 0, L"Copy");
    if (AResult)
    {
      TExportSessionParam Param;
      Param.DestPath = DestPath;
      ProcessSessions(PanelItems, nb::bind(&TWinSCPFileSystem::ExportSession, this), &Param);
      Result = 1;
    }
  }
  return Result;
}

intptr_t TWinSCPFileSystem::GetFilesRemote(TObjectList * PanelItems, bool Move,
  UnicodeString & DestPath, OPERATION_MODES OpMode)
{
  intptr_t Result = -1;
  bool EditView = (OpMode & (OPM_EDIT | OPM_VIEW)) != 0;
  bool Confirmed =
    (OpMode & OPM_SILENT) &&
    (!EditView || GetFarConfiguration()->GetEditorDownloadDefaultMode());

  TGUICopyParamType CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
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
      GetTerminal()->UsableCopyParamAttrs(Params).Download;
      ; // FLAGMASK(EditView, cpaNoExcludeMask);

    uintptr_t Options =
      FLAGMASK(EditView, coTempTransfer | coDisableNewerOnly);
    Confirmed = CopyDialog(false, Move, FFileList,
      Options, CopyParamAttrs,
      DestPath, &CopyParam);

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
      FOriginalEditFile = ::IncludeTrailingBackslash(DestPath) +
        base::UnixExtractFileName(FFileList->GetString(0));
      FLastEditFile = FOriginalEditFile;
      FLastEditCopyParam = CopyParam;
      FLastEditorID = -1;
    }
    else
    {
      FOriginalEditFile.Clear();
      FLastEditFile.Clear();
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
  return Result;
}

TTerminalQueue * TWinSCPFileSystem::GetQueue()
{
  if (FQueue == nullptr)
  {
    FQueue = new TTerminalQueue(FTerminal, GetConfiguration());
    FQueue->Init();
    FQueue->SetTransfersLimit(GetGUIConfiguration()->GetQueueTransfersLimit());
    FQueue->SetOnQueryUser(nb::bind(&TWinSCPFileSystem::TerminalQueryUser, this));
    FQueue->SetOnPromptUser(nb::bind(&TWinSCPFileSystem::TerminalPromptUser, this));
    FQueue->SetOnShowExtendedException(nb::bind(&TWinSCPFileSystem::TerminalShowExtendedException, this));
    FQueue->SetOnListUpdate(nb::bind(&TWinSCPFileSystem::QueueListUpdate, this));
    FQueue->SetOnQueueItemUpdate(nb::bind(&TWinSCPFileSystem::QueueItemUpdate, this));
    FQueue->SetOnEvent(nb::bind(&TWinSCPFileSystem::QueueEvent, this));
  }
  return FQueue;
}

TTerminalQueueStatus * TWinSCPFileSystem::GetQueueStatus()
{
  if (FQueueStatus == nullptr)
  {
    FQueueStatus = GetQueue()->CreateStatus(nullptr);
  }
  return FQueueStatus;
}

void TWinSCPFileSystem::ExportSession(TSessionData * Data, void * AParam)
{
  TExportSessionParam & Param = *static_cast<TExportSessionParam *>(AParam);

  std::unique_ptr<TSessionData> ExportData(new TSessionData(Data->GetName()));
  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  ExportData->Assign(Data);
  ExportData->SetModified(true);
  UnicodeString XmlFileName = ::IncludeTrailingBackslash(Param.DestPath) +
    ::ValidLocalFileName(::ExtractFilename(ExportData->GetName())) + L".netbox";
  std::unique_ptr<THierarchicalStorage> ExportStorage(new TXmlStorage(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));
  ExportStorage->Init();
  ExportStorage->SetAccessMode(smReadWrite);
  if (ExportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
  {
    ExportData->Save(ExportStorage.get(), false, FactoryDefaults.get());
  }
}

intptr_t TWinSCPFileSystem::UploadFiles(bool Move, OPERATION_MODES OpMode, bool Edit,
  UnicodeString & DestPath)
{
  intptr_t Result = 1;
  bool Confirmed = (OpMode & OPM_SILENT);
  bool Ask = !Confirmed;

  TGUICopyParamType CopyParam;

  if (Edit)
  {
    CopyParam = FLastEditCopyParam;
    Confirmed = GetFarConfiguration()->GetEditorUploadSameOptions();
    Ask = false;
  }
  else
  {
    CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  }

  // these parameters are known in advance
  intptr_t Params =
    FLAGMASK(Move, cpDelete);

  if (!Confirmed)
  {
    intptr_t CopyParamAttrs =
      GetTerminal()->UsableCopyParamAttrs(Params).Upload |
      FLAGMASK(Edit, cpaNoClearArchive);
    // heuristics: do not ask for target directory when uploaded file
    // was downloaded in edit mode
    uintptr_t Options =
      FLAGMASK(Edit, coTempTransfer) |
      FLAGMASK(Edit || !GetTerminal()->GetIsCapable(fcNewerOnlyUpload), coDisableNewerOnly);
    Confirmed = CopyDialog(true, Move, FFileList,
      Options, CopyParamAttrs,
      DestPath, &CopyParam);

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
    DebugAssert(!FNoProgressFinish);
    // it does not make sense to unselect file being uploaded from editor,
    // moreover we may upload the file under name that does not exist in
    // remote panel
    FNoProgressFinish = Edit;
    {
      SCOPE_EXIT
      {
        FNoProgressFinish = false;
      };
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(!Ask, cpNoConfirmation) |
        FLAGMASK(Edit, cpTemporary) |
        FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly);
      FTerminal->CopyToRemote(FFileList, DestPath, &CopyParam, Params);
    }
  }
  else
  {
    Result = -1;
  }
  return Result;
}

intptr_t TWinSCPFileSystem::PutFilesEx(TObjectList * PanelItems, bool Move, OPERATION_MODES OpMode)
{
  intptr_t Result;
  if (Connected())
  {
    FFileList = CreateFileList(PanelItems, osLocal);
    SCOPE_EXIT
    {
      FPanelItems = nullptr;
      SAFE_DESTROY(FFileList);
    };
    FPanelItems = PanelItems;

    // if file is saved under different name, FAR tries to upload original file,
    // but let's be robust and check for new name, in case it changes.
    // OPM_EDIT is set since 1.70 final, only.
    // When comparing, beware that one path may be long path and the other short
    // (since 1.70 alpha 6, DestPath in GetFiles is short path,
    // while current path in PutFiles is long path)
    if (FLAGCLEAR(OpMode, OPM_SILENT) && (FFileList->GetCount() == 1) && //-V522
      (CompareFileName(FFileList->GetString(0), FOriginalEditFile) || //-V522
        CompareFileName(FFileList->GetString(0), FLastEditFile)))
    {
      // editor should be closed already
      DebugAssert(FLastEditorID < 0);

      /*if (GetFarConfiguration()->GetEditorUploadOnSave())
      {
        // already uploaded from EE_REDRAW
        Result = -1;
      }
      else*/
      {
        // just in case file was saved under different name
        FFileList->SetString(0, FLastEditFile);

        FOriginalEditFile.Clear();
        FLastEditFile.Clear();

        UnicodeString CurrentDirectory = FTerminal->GetCurrDirectory();
        Result = UploadFiles(Move, OpMode, true, CurrentDirectory);
        FTerminal->TerminalSetCurrentDirectory(CurrentDirectory);
      }
    }
    else
    {
      UnicodeString CurrentDirectory = FTerminal->GetCurrDirectory();
      Result = UploadFiles(Move, OpMode, false, CurrentDirectory);
      FTerminal->TerminalSetCurrentDirectory(CurrentDirectory);
    }
  }
  else if (IsSessionList())
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

bool TWinSCPFileSystem::ImportSessions(TObjectList * PanelItems, bool /*Move*/,
  OPERATION_MODES OpMode)
{
  bool Result = (OpMode & OPM_SILENT) ||
    (MoreMessageDialog(GetMsg(IMPORT_SESSIONS_PROMPT), nullptr,
      qtConfirmation, qaYes | qaNo) == qaYes);

  if (Result)
  {
    UnicodeString FileName;
    TFarPanelItem * PanelItem;
    for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
    {
      PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
      bool AnyData = false;
      FileName = PanelItem->GetFileName();
      if (PanelItem->GetIsFile())
      {
        UnicodeString XmlFileName = ::IncludeTrailingBackslash(::GetCurrentDir()) + FileName;
        std::unique_ptr<THierarchicalStorage> ImportStorage(new TXmlStorage(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));
        ImportStorage->Init();
        ImportStorage->SetAccessMode(smRead);
        if (ImportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false) &&
          ImportStorage->HasSubKeys())
        {
          AnyData = true;
          StoredSessions->Load(ImportStorage.get(), /*AsModified*/ true, /*UseDefaults*/ true);
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

TStrings * TWinSCPFileSystem::CreateFocusedFileList(TOperationSide Side, TFarPanelInfo ** APanelInfo)
{
  if (!APanelInfo || !*APanelInfo)
  {
    APanelInfo = this->GetPanelInfo();
  }

  TStrings * Result = nullptr;
  const TFarPanelItem * Focused = APanelInfo && *APanelInfo ? (*APanelInfo)->GetFocusedItem() : nullptr;
  if (!Focused->GetIsParentDirectory())
  {
    Result = new TStringList();
    DebugAssert((Side == osLocal) || Focused->GetUserData());
    UnicodeString FileName = Focused->GetFileName();
    if (Side == osLocal)
    {
      FileName = ::IncludeTrailingBackslash((*APanelInfo)->GetCurrDirectory()) + FileName;
    }
    Result->AddObject(FileName, as_object(Focused->GetUserData()));
  }
  return Result;
}

TStrings * TWinSCPFileSystem::CreateSelectedFileList(TOperationSide Side, TFarPanelInfo ** APanelInfo)
{
  TFarPanelInfo ** PanelInfo = APanelInfo;
  if (PanelInfo == nullptr)
  {
    PanelInfo = this->GetPanelInfo();
  }

  TStrings * Result;
  if (PanelInfo && *PanelInfo && (*PanelInfo)->GetSelectedCount() > 0)
  {
    UnicodeString CurrDirectory = Connected() ? FTerminal->GetCurrDirectory() : (*PanelInfo)->GetCurrDirectory();
    Result = CreateFileList((*PanelInfo)->GetItems(), Side, true, CurrDirectory);
  }
  else
  {
    Result = CreateFocusedFileList(Side, PanelInfo);
  }
  return Result;
}

TStrings * TWinSCPFileSystem::CreateFileList(TObjectList * PanelItems,
  TOperationSide Side, bool SelectedOnly, const UnicodeString & Directory, bool FileNameOnly,
  TStrings * AFileList)
{
  std::unique_ptr<TStrings> FileList(AFileList == nullptr ? new TStringList() : AFileList);
  FileList->SetDuplicates(dupAccept);

  UnicodeString FileName;
  TFarPanelItem * PanelItem;
  TObject * Data = nullptr;
  for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
  {
    PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
    DebugAssert(PanelItem);
    if ((!SelectedOnly || PanelItem->GetSelected()) &&
      !PanelItem->GetIsParentDirectory())
    {
      FileName = PanelItem->GetFileName();
      if (Side == osRemote)
      {
        Data = get_as<TRemoteFile>(PanelItem->GetUserData());
        DebugAssert(Data);
      }
      if (Side == osLocal)
      {
        if (::ExtractFilePath(FileName).IsEmpty())
        {
          if (!FileNameOnly)
          {
            UnicodeString Dir = Directory;
            if (Dir.IsEmpty())
            {
              Dir = ::GetCurrentDir();
            }
            FileName = ::IncludeTrailingBackslash(Dir) + FileName;
          }
        }
        else
        {
          if (FileNameOnly)
          {
            FileName = base::ExtractFileName(FileName, false);
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
  return FileList.release();
}

void TWinSCPFileSystem::SaveSession()
{
  if (FTerminal->GetActive() && !GetSessionData()->GetName().IsEmpty())
  {
    GetSessionData()->SetRemoteDirectory(FTerminal->GetCurrDirectory());

    TSessionData * Data = dyn_cast<TSessionData>(StoredSessions->FindByName(GetSessionData()->GetName()));
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

bool TWinSCPFileSystem::Connect(TSessionData * Data)
{
  bool Result = false;
  DebugAssert(!FTerminal);
  FTerminal = new TTerminal();
  FTerminal->Init(Data, GetConfiguration());
  try
  {
    FTerminal->SetOnQueryUser(nb::bind(&TWinSCPFileSystem::TerminalQueryUser, this));
    FTerminal->SetOnPromptUser(nb::bind(&TWinSCPFileSystem::TerminalPromptUser, this));
    FTerminal->SetOnDisplayBanner(nb::bind(&TWinSCPFileSystem::TerminalDisplayBanner, this));
    FTerminal->SetOnShowExtendedException(nb::bind(&TWinSCPFileSystem::TerminalShowExtendedException, this));
    FTerminal->SetOnChangeDirectory(nb::bind(&TWinSCPFileSystem::TerminalChangeDirectory, this));
    FTerminal->SetOnReadDirectory(nb::bind(&TWinSCPFileSystem::TerminalReadDirectory, this));
    FTerminal->SetOnStartReadDirectory(nb::bind(&TWinSCPFileSystem::TerminalStartReadDirectory, this));
    FTerminal->SetOnReadDirectoryProgress(nb::bind(&TWinSCPFileSystem::TerminalReadDirectoryProgress, this));
    FTerminal->SetOnInformation(nb::bind(&TWinSCPFileSystem::TerminalInformation, this));
    FTerminal->SetOnFinished(nb::bind(&TWinSCPFileSystem::OperationFinished, this));
    FTerminal->SetOnProgress(nb::bind(&TWinSCPFileSystem::OperationProgress, this));
    FTerminal->SetOnDeleteLocalFile(nb::bind(&TWinSCPFileSystem::TerminalDeleteLocalFile, this));
    FTerminal->SetOnCreateLocalFile(nb::bind(&TWinSCPFileSystem::TerminalCreateLocalFile, this));
    FTerminal->SetOnGetLocalFileAttributes(nb::bind(&TWinSCPFileSystem::TerminalGetLocalFileAttributes, this));
    FTerminal->SetOnSetLocalFileAttributes(nb::bind(&TWinSCPFileSystem::TerminalSetLocalFileAttributes, this));
    FTerminal->SetOnMoveLocalFile(nb::bind(&TWinSCPFileSystem::TerminalMoveLocalFile, this));
    FTerminal->SetOnRemoveLocalDirectory(nb::bind(&TWinSCPFileSystem::TerminalRemoveLocalDirectory, this));
    FTerminal->SetOnCreateLocalDirectory(nb::bind(&TWinSCPFileSystem::TerminalCreateLocalDirectory, this));
    FTerminal->SetOnCheckForEsc(nb::bind(&TWinSCPFileSystem::TerminalCheckForEsc, this));
    ConnectTerminal(FTerminal);

    FTerminal->SetOnClose(nb::bind(&TWinSCPFileSystem::TerminalClose, this));

    DebugAssert(FQueue == nullptr);
    DebugAssert(FQueueStatus == nullptr);

    TODO("Create instance of TKeepaliveThread here, once its implementation is complete");

    Result = FTerminal->GetActive();
    if (!Result)
    {
      throw Exception(FORMAT(GetMsg(CANNOT_INIT_SESSION).c_str(), Data->GetSessionName().c_str()));
    }
  }
  catch (Exception & E)
  {
    EFatal * Fatal = dyn_cast<EFatal>(&E);
    if ((Fatal == nullptr) || !Fatal->GetReopenQueried())
    {
      FTerminal->ShowExtendedException(&E);
    }
    SAFE_DESTROY(FTerminal);
    SAFE_DESTROY(FQueue);
    SAFE_DESTROY(FQueueStatus);
  }

  if (FTerminal != nullptr)
  {
    FSynchronisingBrowse = GetSessionData()->GetSynchronizeBrowsing();
  }
  return Result;
}

void TWinSCPFileSystem::Disconnect()
{
  if (FTerminal && FTerminal->GetActive())
  {
    if (!GetSessionData()->GetName().IsEmpty())
    {
      FPrevSessionName = GetSessionData()->GetName();
    }
    SaveSession();
  }
  DebugAssert(FSynchronizeController == nullptr);
  DebugAssert(!FAuthenticationSaveScreenHandle);
  DebugAssert(!FProgressSaveScreenHandle);
  DebugAssert(!FSynchronizationSaveScreenHandle);
  DebugAssert(!FFileList);
  DebugAssert(!FPanelItems);
  SAFE_DESTROY(FQueue);
  SAFE_DESTROY(FQueueStatus);
  if (FTerminal != nullptr)
  {
    GetSessionData()->SetSynchronizeBrowsing(FSynchronisingBrowse);
  }
  SAFE_DESTROY(FTerminal);
}

void TWinSCPFileSystem::ConnectTerminal(TTerminal * Terminal)
{
  Terminal->Open();
}

bool TWinSCPFileSystem::TerminalCheckForEsc()
{
  return GetWinSCPPlugin()->CheckForEsc();
}

void TWinSCPFileSystem::TerminalClose(TObject * /*Sender*/)
{
  // Plugin closure is now invoked from HandleException
}

void TWinSCPFileSystem::LogAuthentication(
  TTerminal * Terminal, const UnicodeString & Msg)
{
  DebugAssert(FAuthenticationLog != nullptr);
  FAuthenticationLog->Add(Msg);
  std::unique_ptr<TStringList> AuthenticationLogLines(new TStringList());
  intptr_t Width = 42;
  intptr_t Height = 11;
  FarWrapText(::TrimRight(FAuthenticationLog->GetText()), AuthenticationLogLines.get(), Width);
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
    Message = ::AnsiReplaceStr(AuthenticationLogLines->GetText(), L"\r", L"");
    Count = AuthenticationLogLines->GetCount();
  }

  Message += ::StringOfChar(L'\n', Height - Count);

  GetWinSCPPlugin()->Message(0, Terminal->GetSessionData()->GetSessionName(), Message);
}

void TWinSCPFileSystem::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & Str, bool /*Status*/, intptr_t Phase)
{
  if (Phase != 0)
  {
    if (GetTerminal() && (GetTerminal()->GetStatus() == ssOpening))
    {
      if (FAuthenticationLog == nullptr)
      {
        FAuthenticationLog = new TStringList();
        GetWinSCPPlugin()->SaveScreen(FAuthenticationSaveScreenHandle);
        GetWinSCPPlugin()->ShowConsoleTitle(GetTerminal()->GetSessionData()->GetSessionName());
      }

      LogAuthentication(Terminal, Str);
      GetWinSCPPlugin()->UpdateConsoleTitle(Str);
    }
  }
  else
  {
    if (FAuthenticationLog != nullptr)
    {
      GetWinSCPPlugin()->ClearConsoleTitle();
      GetWinSCPPlugin()->RestoreScreen(FAuthenticationSaveScreenHandle);
      SAFE_DESTROY(FAuthenticationLog);
    }
  }
}

void TWinSCPFileSystem::TerminalChangeDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    UnicodeString Directory = FTerminal->GetCurrDirectory();
    intptr_t Index = FPathHistory->IndexOf(Directory);
    if (Index >= 0)
    {
      FPathHistory->Delete(Index);
    }

    if (!FLastPath.IsEmpty())
    {
      FPathHistory->Add(FLastPath);
    }

    FLastPath = Directory;
    SaveSession(); // To save changed directory
  }
}

void TWinSCPFileSystem::TerminalStartReadDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(READING_DIRECTORY_TITLE));
  }
}

void TWinSCPFileSystem::TerminalReadDirectoryProgress(
  TObject * /*Sender*/, intptr_t Progress, intptr_t /*ResolvedLinks*/, bool & Cancel)
{
  if (Progress < 0)
  {
    if (!FNoProgress && (Progress == -2))
    {
      MoreMessageDialog(GetMsg(DIRECTORY_READING_CANCELLED), nullptr,
         qtWarning, qaOK);
    }
  }
  else
  {
    if (GetWinSCPPlugin()->CheckForEsc())
    {
      Cancel = true;
    }

    if (!FNoProgress)
    {
      GetWinSCPPlugin()->UpdateConsoleTitle(
        FORMAT(L"%s (%d)", GetMsg(READING_DIRECTORY_TITLE).c_str(), Progress));
    }
  }
}

void TWinSCPFileSystem::TerminalReadDirectory(TObject * /*Sender*/,
  bool /*ReloadOnly*/)
{
  if (!FNoProgress)
  {
    GetWinSCPPlugin()->ClearConsoleTitle();
  }
}

void TWinSCPFileSystem::TerminalDeleteLocalFile(const UnicodeString & AFileName,
  bool Alternative)
{
  bool ToRecycleBin = FLAGSET(GetWinSCPPlugin()->GetFarSystemSettings(), NBSS_DELETETORECYCLEBIN) != Alternative;
  if (ToRecycleBin || !GetWinSCPPlugin()->GetSystemFunctions())
  {
    if (!RecursiveDeleteFile(AFileName, ToRecycleBin))
    {
      throw Exception(FORMAT(GetMsg(DELETE_LOCAL_FILE_ERROR).c_str(), AFileName.c_str()));
    }
  }
  else
  {
    GetWinSCPPlugin()->DeleteLocalFile(AFileName);
  }
}

HANDLE TWinSCPFileSystem::TerminalCreateLocalFile(const UnicodeString & LocalFileName,
  DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::CreateFile(ApiPath(LocalFileName).c_str(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, nullptr);
  }
  else
  {
    return GetWinSCPPlugin()->CreateLocalFile(LocalFileName, DesiredAccess,
      ShareMode, CreationDisposition, FlagsAndAttributes);
  }
}

inline DWORD TWinSCPFileSystem::TerminalGetLocalFileAttributes(const UnicodeString & LocalFileName) const
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::FileGetAttrFix(LocalFileName);
  }
  else
  {
    return GetWinSCPPlugin()->GetLocalFileAttributes(LocalFileName);
  }
}

inline BOOL TWinSCPFileSystem::TerminalSetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::SetFileAttributes(ApiPath(LocalFileName).c_str(), FileAttributes);
  }
  else
  {
    return GetWinSCPPlugin()->SetLocalFileAttributes(LocalFileName, FileAttributes);
  }
}

BOOL TWinSCPFileSystem::TerminalMoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::MoveFileExW(ApiPath(LocalFileName).c_str(), ApiPath(NewLocalFileName).c_str(), Flags) != 0;
  }
  else
  {
    return GetWinSCPPlugin()->MoveLocalFile(LocalFileName, NewLocalFileName, Flags);
  }
}

BOOL TWinSCPFileSystem::TerminalRemoveLocalDirectory(const UnicodeString & LocalDirName)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::RemoveDirectory(ApiPath(LocalDirName).c_str()) != 0;
  }
  else
  {
    return GetWinSCPPlugin()->RemoveLocalDirectory(LocalDirName);
  }
}
BOOL TWinSCPFileSystem::TerminalCreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::CreateDirectory(ApiPath(LocalDirName).c_str(), SecurityAttributes) != 0;
  }
  else
  {
    return GetWinSCPPlugin()->CreateLocalDirectory(LocalDirName, SecurityAttributes);
  }
}

uintptr_t TWinSCPFileSystem::MoreMessageDialog(const UnicodeString & Str,
  TStrings * MoreMessages, TQueryType Type, uintptr_t Answers, const TMessageParams * AParams)
{
  TMessageParams Params(0);

  //if ((FProgressSaveScreenHandle != 0) ||
  //    (FSynchronizationSaveScreenHandle != 0))
  {
    if (AParams != nullptr)
    {
      Params.Assign(AParams);
    }
    AParams = &Params;
    Params.Flags |= FMSG_WARNING;
  }

  return GetWinSCPPlugin()->MoreMessageDialog(Str, MoreMessages, Type,
    Answers, &Params);
}

void TWinSCPFileSystem::TerminalQueryUser(TObject * /*Sender*/,
  const UnicodeString & AQuery, TStrings * MoreMessages, uintptr_t Answers,
  const TQueryParams * AParams, uintptr_t & Answer, TQueryType Type, void * /*Arg*/)
{
  TMessageParams Params(0);
  UnicodeString Query = AQuery;

  if (AParams != nullptr)
  {
    if (AParams->Params & qpFatalAbort)
    {
      Query = FORMAT(GetMsg(WARN_FATAL_ERROR).c_str(), Query.c_str());
    }

    Params.Aliases = AParams->Aliases;
    Params.AliasesCount = AParams->AliasesCount;
    Params.Params = AParams->Params & (qpNeverAskAgainCheck | qpAllowContinueOnError);
    Params.Timer = AParams->Timer;
    Params.TimerEvent = AParams->TimerEvent;
    Params.TimerMessage = AParams->TimerMessage;
    Params.TimerAnswers = AParams->TimerAnswers;
    Params.Timeout = AParams->Timeout;
    Params.TimeoutAnswer = AParams->TimeoutAnswer;
  }

  Answer = MoreMessageDialog(Query, MoreMessages, Type, Answers, &Params);
}

void TWinSCPFileSystem::TerminalPromptUser(TTerminal * Terminal,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
  TStrings * Prompts, TStrings * Results, bool & AResult,
  void * /*Arg*/)
{
  if (Kind == pkPrompt)
  {
    DebugAssert(Instructions.IsEmpty());
    DebugAssert(Prompts->GetCount() == 1);
    DebugAssert(Prompts->GetObj(0) != nullptr);
    UnicodeString Result = Results->GetString(0);

    AResult = GetWinSCPPlugin()->InputBox(Name, ::StripHotkey(Prompts->GetString(0)), Result, FIB_NOUSELASTHISTORY);
    if (AResult)
    {
      Results->SetString(0, Result);
    }
  }
  else
  {
    AResult = PasswordDialog(Terminal->GetSessionData(), Kind, Name, Instructions,
      Prompts, Results, GetTerminal()->GetStoredCredentialsTried());
  }
}

void TWinSCPFileSystem::TerminalDisplayBanner(
  TTerminal * /*Terminal*/, const UnicodeString & SessionName,
  const UnicodeString & Banner, bool & NeverShowAgain, intptr_t Options)
{
  BannerDialog(SessionName, Banner, NeverShowAgain, Options);
}

void TWinSCPFileSystem::TerminalShowExtendedException(
  TTerminal * /*Terminal*/, Exception * E, void * /*Arg*/)
{
  GetWinSCPPlugin()->ShowExtendedException(E);
}

void TWinSCPFileSystem::OperationProgress(
  TFileOperationProgressType & ProgressData)
{
  if (FNoProgress)
  {
    return;
  }

  bool First = false;
  if (ProgressData.InProgress && !FProgressSaveScreenHandle)
  {
    GetWinSCPPlugin()->SaveScreen(FProgressSaveScreenHandle);
    First = true;
  }

  // operation is finished (or terminated), so we hide progress form
  if (!ProgressData.InProgress && FProgressSaveScreenHandle)
  {
    GetWinSCPPlugin()->RestoreScreen(FProgressSaveScreenHandle);
    GetWinSCPPlugin()->ClearConsoleTitle();
  }
  else
  {
    ShowOperationProgress(ProgressData, First);
  }
}

void TWinSCPFileSystem::OperationFinished(TFileOperation Operation,
  TOperationSide Side, bool /*Temp*/, const UnicodeString & AFileName, bool Success,
  TOnceDoneOperation & /*DisconnectWhenComplete*/)
{
  DebugUsedParam(Side);

  if ((Operation != foCalculateSize) &&
    (Operation != foGetProperties) &&
    (Operation != foCalculateChecksum) &&
    (FSynchronizationSaveScreenHandle == nullptr) &&
    !FNoProgress && !FNoProgressFinish)
  {
    TFarPanelItem * PanelItem = nullptr;

    if (!FPanelItems)
    {
      TObjectList * PanelItems = (*GetPanelInfo())->GetItems();
      for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
      {
        if (PanelItems->GetAs<TFarPanelItem>(Index)->GetFileName() == AFileName)
        {
          PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
          break;
        }
      }
    }
    else
    {
      DebugAssert(FFileList);
      DebugAssert(FPanelItems->GetCount() == FFileList->GetCount());
      intptr_t Index = FFileList->IndexOf(AFileName);
      DebugAssert(Index >= 0);
      PanelItem = get_as<TFarPanelItem>(FPanelItems->GetItem(Index));
    }

    DebugAssert(PanelItem && PanelItem->GetFileName() ==
      ((Side == osLocal) ? base::ExtractFileName(AFileName, false) : AFileName));
    if (Success && PanelItem)
    {
      PanelItem->SetSelected(false);
    }
  }

  if (Success && (FSynchronizeController != nullptr))
  {
    if (Operation == foCopy)
    {
      DebugAssert(Side == osLocal);
      FSynchronizeController->LogOperation(soUpload, AFileName);
    }
    else if (Operation == foDelete)
    {
      DebugAssert(Side == osRemote);
      FSynchronizeController->LogOperation(soDelete, AFileName);
    }
  }
}

void TWinSCPFileSystem::ShowOperationProgress(
  TFileOperationProgressType & ProgressData, bool Force)
{
  static uint32_t LastTicks;
  uint32_t Ticks = ::GetTickCount();
  short percents = static_cast<short>(ProgressData.OverallProgress());
  if (Ticks - LastTicks > 500 || Force)
  {
    LastTicks = Ticks;

    static const intptr_t ProgressWidth = 48;
    static const int Captions[] = {PROGRESS_COPY, PROGRESS_MOVE, PROGRESS_DELETE,
      PROGRESS_SETPROPERTIES, 0, 0, PROGRESS_CALCULATE_SIZE,
      PROGRESS_REMOTE_MOVE, PROGRESS_REMOTE_COPY, PROGRESS_GETPROPERTIES,
      PROGRESS_CALCULATE_CHECKSUM};
    static UnicodeString ProgressFileLabel;
    static UnicodeString TargetDirLabel;
    static UnicodeString StartTimeLabel;
    static UnicodeString TimeElapsedLabel;
    static UnicodeString BytesTransferredLabel;
    static UnicodeString CPSLabel;
    static UnicodeString TimeLeftLabel;

    if (ProgressFileLabel.IsEmpty())
    {
      ProgressFileLabel = GetMsg(PROGRESS_FILE_LABEL);
      TargetDirLabel = GetMsg(TARGET_DIR_LABEL);
      StartTimeLabel = GetMsg(START_TIME_LABEL);
      TimeElapsedLabel = GetMsg(TIME_ELAPSED_LABEL);
      BytesTransferredLabel = GetMsg(BYTES_TRANSFERED_LABEL);
      CPSLabel = GetMsg(CPS_LABEL);
      TimeLeftLabel = GetMsg(TIME_LEFT_LABEL);
    }

    bool TransferOperation =
      ((ProgressData.Operation == foCopy) || (ProgressData.Operation == foMove));

    UnicodeString Message1;
    UnicodeString ProgressBarCurrentFile;
    UnicodeString Message2;
    UnicodeString ProgressBarTotal;
    UnicodeString Title = GetMsg(Captions[static_cast<int>(ProgressData.Operation - 1)]);
    UnicodeString FileName = ProgressData.FileName;
    // for upload from temporary directory,
    // do not show source directory
    if (TransferOperation && (ProgressData.Side == osLocal) && ProgressData.Temp)
    {
      FileName = base::ExtractFileName(FileName, false);
    }
    Message1 = ProgressFileLabel + core::MinimizeName(FileName,
      ProgressWidth - ProgressFileLabel.Length(), ProgressData.Side == osRemote) + L"\n";
    // for downloads to temporary directory,
    // do not show target directory
    if (TransferOperation && !((ProgressData.Side == osRemote) && ProgressData.Temp))
    {
      Message1 += TargetDirLabel + core::MinimizeName(ProgressData.Directory,
                  ProgressWidth - TargetDirLabel.Length(), ProgressData.Side == osLocal) + L"\n";
    }
    ProgressBarTotal = ProgressBar(ProgressData.OverallProgress(), ProgressWidth) + L"\n";
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
        Value = ProgressData.StartTime.TimeString(true);
        LabelText = StartTimeLabel;
      }
      StatusLine = StatusLine + LabelText +
                   ::StringOfChar(' ', ProgressWidth - StatusLine.Length() -
                                  LabelText.Length() - Value.Length()) + Value;
      Message2 += StatusLine + L"\n";

      Value = FormatBytes(ProgressData.TotalTransfered);
      StatusLine = BytesTransferredLabel +
                   ::StringOfChar(' ', ProgressWidth / 2 - 1 - BytesTransferredLabel.Length() - Value.Length()) +
                   Value + L"  ";
      Value = FORMAT(L"%s/s", FormatBytes(ProgressData.CPS()).c_str());
      StatusLine = StatusLine + CPSLabel +
                   ::StringOfChar(' ', ProgressWidth - StatusLine.Length() -
                                  CPSLabel.Length() - Value.Length()) + Value;
      Message2 += StatusLine + L"\n";
      ProgressBarCurrentFile = ProgressBar(ProgressData.TransferProgress(), ProgressWidth) + L"\n";
    }
    UnicodeString Message =
      Message1 + ProgressBarCurrentFile + Message2 + ProgressBarTotal;
    GetWinSCPPlugin()->Message(0, Title, Message, nullptr, nullptr);

    if (Force)
    {
      GetWinSCPPlugin()->ShowConsoleTitle(Title);
    }
    GetWinSCPPlugin()->UpdateConsoleTitleProgress(percents);

    if (GetWinSCPPlugin()->CheckForEsc())
    {
      CancelConfiguration(ProgressData);
    }
  }
  if (percents == 100)
  {
    GetWinSCPPlugin()->UpdateConsoleTitleProgress(percents);
  }
}

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

TTerminalQueueStatus * TWinSCPFileSystem::ProcessQueue(bool Hidden)
{
  TTerminalQueueStatus * Result = nullptr;
  if (FQueue == nullptr)
    return Result;

  TTerminalQueueStatus * QueueStatus = GetQueueStatus();
  DebugAssert(QueueStatus != nullptr);
  FarPlugin->UpdateProgress(QueueStatus->GetCount() > 0 ? TBPS_INDETERMINATE : TBPS_NOPROGRESS, 0);

  if (FQueueStatusInvalidated || FQueueItemInvalidated)
  {
    if (FQueueStatusInvalidated)
    {
      TGuard Guard(FQueueStatusSection);

      FQueueStatusInvalidated = false;

      if (FQueue != nullptr)
        FQueueStatus = FQueue->CreateStatus(FQueueStatus);
      Result = FQueueStatus;
    }

    FQueueItemInvalidated = false;

    for (intptr_t Index = 0; Index < FQueueStatus->GetActiveCount(); ++Index)
    {
      TQueueItemProxy * QueueItem = FQueueStatus->GetItem(Index);
      if (QueueItem->GetUserData() != nullptr)
      {
        QueueItem->Update();
        Result = FQueueStatus;
      }

      if (GetGUIConfiguration()->GetQueueAutoPopup() &&
        TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
      {
        QueueItem->ProcessUserAction();
      }
    }
  }

  if (FRefreshRemoteDirectory)
  {
    if ((GetTerminal() != nullptr) && GetTerminal()->GetActive())
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
    if (GetOppositeFileSystem() == nullptr)
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
      if (Hidden && GetFarConfiguration()->GetQueueBeep())
      {
        ::MessageBeep(MB_OK);
      }
      break;

    case qePendingUserAction:
      if (Hidden && !GetGUIConfiguration()->GetQueueAutoPopup() && GetFarConfiguration()->GetQueueBeep())
      {
        // MB_ICONQUESTION would be more appropriate, but in default Windows Sound
        // schema it has no sound associated
        ::MessageBeep(MB_OK);
      }
      break;
    }
  }

  return Result;
}

void TWinSCPFileSystem::QueueListUpdate(TTerminalQueue * Queue)
{
  if (GetQueue() == Queue)
  {
    FQueueStatusInvalidated = true;
  }
}

void TWinSCPFileSystem::QueueItemUpdate(TTerminalQueue * Queue,
  TQueueItem * Item)
{
  if (GetQueue() == Queue)
  {
    TGuard Guard(FQueueStatusSection);

    TTerminalQueueStatus * QueueStatus = GetQueueStatus();
    DebugAssert(QueueStatus != nullptr);

    TQueueItemProxy * QueueItem = QueueStatus->FindByQueueItem(Item);

    if ((Item->GetStatus() == TQueueItem::qsDone) && (GetTerminal() != nullptr))
    {
      FRefreshLocalDirectory = (QueueItem == nullptr) ||
        (!QueueItem->GetInfo()->ModifiedLocal.IsEmpty());
      FRefreshRemoteDirectory = (QueueItem == nullptr) ||
        (!QueueItem->GetInfo()->ModifiedRemote.IsEmpty());
    }

    if (QueueItem != nullptr)
    {
      QueueItem->SetUserData(ToPtr(1LL));
      FQueueItemInvalidated = true;
    }
  }
}

void TWinSCPFileSystem::QueueEvent(TTerminalQueue * Queue,
  TQueueEvent Event)
{
  TGuard Guard(FQueueStatusSection);
  if (Queue == GetQueue())
  {
    FQueueEventPending = true;
    FQueueEvent = Event;
  }
}

void TWinSCPFileSystem::CancelConfiguration(TFileOperationProgressType & ProgressData)
{
  if (!ProgressData.Suspended)
  {
    ProgressData.Suspend();
    SCOPE_EXIT
    {
      ProgressData.Resume();
    };
    TCancelStatus ACancel;
    uintptr_t Result = 0;
    if (ProgressData.TransferingFile &&
      (ProgressData.TimeExpected() > GetGUIConfiguration()->GetIgnoreCancelBeforeFinish()))
    {
      Result = MoreMessageDialog(GetMsg(CANCEL_OPERATION_FATAL), nullptr,
                                 qtWarning, qaYes | qaNo | qaCancel);
    }
    else
    {
      Result = MoreMessageDialog(GetMsg(CANCEL_OPERATION), nullptr,
                                 qtConfirmation, qaOK | qaCancel);
    }
    switch (Result)
    {
    case qaYes:
      ACancel = csCancelTransfer;
      break;
    case qaOK:
      ACancel = csCancel;
      break;
    case qaNo:
      ACancel = csContinue;
      break;
    default:
      ACancel = csContinue;
      break;
    }

    if (ACancel > ProgressData.Cancel)
    {
      ProgressData.Cancel = ACancel;
    }
  }
}

void TWinSCPFileSystem::UploadFromEditor(bool NoReload,
  const UnicodeString & AFileName, const UnicodeString & RealFileName,
  UnicodeString & DestPath)
{
  DebugAssert(FFileList == nullptr);
  FFileList = new TStringList();
  DebugAssert(FTerminal->GetAutoReadDirectory());
  bool PrevAutoReadDirectory = FTerminal->GetAutoReadDirectory();
  if (NoReload)
  {
    FTerminal->SetAutoReadDirectory(false);
    if (core::UnixSamePath(DestPath, FTerminal->GetCurrDirectory()))
    {
      FReloadDirectory = true;
    }
  }

  std::unique_ptr<TRemoteFile> File(new TRemoteFile());
  File->SetFileName(RealFileName);
  SCOPE_EXIT
  {
    FTerminal->SetAutoReadDirectory(PrevAutoReadDirectory);
    SAFE_DESTROY(FFileList);
  };
  FFileList->AddObject(AFileName, File.get()); //-V522
  UploadFiles(false, 0, true, DestPath);
}

void TWinSCPFileSystem::UploadOnSave(bool NoReload)
{
  std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
  if (Info.get() != nullptr)
  {
    bool NativeEdit =
      (FLastEditorID >= 0) &&
      (FLastEditorID == Info->GetEditorID()) &&
      !FLastEditFile.IsEmpty();

    TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
    bool MultipleEdit = (it != FMultipleEdits.end());

    if (NativeEdit || MultipleEdit)
    {
      // make sure this is reset before any dialog is shown as it may cause recursion
      FEditorPendingSave = false;

      if (NativeEdit)
      {
        DebugAssert(FLastEditFile == Info->GetFileName());
        // always upload under the most recent name
        UnicodeString CurrentDirectory = FTerminal->GetCurrDirectory();
        UploadFromEditor(NoReload, FLastEditFile, FLastEditFile, CurrentDirectory);
        FTerminal->TerminalSetCurrentDirectory(CurrentDirectory);
      }

      if (MultipleEdit)
      {
        UploadFromEditor(NoReload, Info->GetFileName(), it->second.FileTitle, it->second.Directory);
        // note that panel gets not refreshed upon switch to
        // panel view. but that's intentional
      }
    }
  }
}

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
    static uint32_t LastTicks = 0;
    uint32_t Ticks = ::GetTickCount();
    if ((LastTicks == 0) || (Ticks - LastTicks > 500))
    {
      LastTicks = Ticks;
      std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
      if (Info.get() != nullptr)
      {
        TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
        if (it != FMultipleEdits.end())
        {
          UnicodeString FullFileName = core::UnixIncludeTrailingBackslash(it->second.Directory) +
            it->second.FileTitle;
          GetWinSCPPlugin()->FarEditorControl(ECTL_SETTITLE,
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
      std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
      if (Info.get() != nullptr)
      {
        if (!FLastEditFile.IsEmpty() &&
          ::AnsiSameText(FLastEditFile, Info->GetFileName()))
        {
          FLastEditorID = Info->GetEditorID();
          FEditorPendingSave = false;
        }

        if (!FLastMultipleEditFile.IsEmpty())
        {
          bool IsLastMultipleEditFile = ::AnsiSameText(core::FromUnixPath(FLastMultipleEditFile), core::FromUnixPath(Info->GetFileName()));
          DebugAssert(IsLastMultipleEditFile);
          if (IsLastMultipleEditFile)
          {
            FLastMultipleEditFile.Clear();

            TMultipleEdit MultipleEdit;
            MultipleEdit.FileName = base::ExtractFileName(Info->GetFileName(), false);
            MultipleEdit.FileTitle = FLastMultipleEditFileTitle;
            MultipleEdit.Directory = FLastMultipleEditDirectory;
            MultipleEdit.LocalFileName = Info->GetFileName();
            MultipleEdit.PendingSave = false;
            FMultipleEdits[Info->GetEditorID()] = MultipleEdit;
            if (FLastMultipleEditReadOnly)
            {
              EditorSetParameter Parameter;
              ClearStruct(Parameter);
              Parameter.StructSize = sizeof(EditorSetParameter);
              Parameter.Type = ESPT_LOCKMODE;
              Parameter.iParam = TRUE;
              GetWinSCPPlugin()->FarEditorControl(ECTL_SETPARAM, 0, &Parameter);
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
      DebugAssert(false); // should not happen, but let's be robust
      UploadOnSave(false);
    }

    std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
    if (Info.get() != nullptr)
    {
      if (FLastEditorID == Info->GetEditorID())
      {
        FLastEditorID = -1;
      }

      TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
      if (it != FMultipleEdits.end())
      {
        TMultipleEdit & ed = it->second;
        if (ed.PendingSave)
        {
          UploadFromEditor(true, Info->GetFileName(), ed.FileTitle, ed.Directory);
          // reload panel content (if uploaded to current directory.
          // no need for RefreshPanel as panel is not visible yet.
          UpdatePanel();
        }

        if (::RemoveFile(Info->GetFileName()))
        {
          // remove directory only if it is empty
          // (to avoid deleting another directory if user uses "save as")
          ::RemoveDir(::ExcludeTrailingBackslash(::ExtractFilePath(Info->GetFileName())));
        }

        FMultipleEdits.erase(it->first);
      }
    }
  }
  else if (Event == EE_SAVE)
  {
    std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
    if (Info.get() != nullptr)
    {
      if ((FLastEditorID >= 0) && (FLastEditorID == Info->GetEditorID()))
      {
        // if the file is saved under different name ("save as"), we upload
        // the file back under that name
        FLastEditFile = Info->GetFileName();

        if (GetFarConfiguration()->GetEditorUploadOnSave())
        {
          FEditorPendingSave = true;
        }
      }

      TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
      if (it != FMultipleEdits.end())
      {
        if (it->second.LocalFileName != Info->GetFileName())
        {
          // update file name (after "save as")
          it->second.LocalFileName = Info->GetFileName();
          it->second.FileName = base::ExtractFileName(Info->GetFileName(), true);
          // update editor title
          UnicodeString FullFileName = core::UnixIncludeTrailingBackslash(it->second.Directory) +
            it->second.FileTitle;
          // note that we need to reset the title periodically (see EE_REDRAW)
          GetWinSCPPlugin()->FarEditorControl(ECTL_SETTITLE,
              FullFileName.Length(),
            static_cast<void *>(const_cast<wchar_t *>(FullFileName.c_str())));
        }

        if (GetFarConfiguration()->GetEditorUploadOnSave())
        {
          FEditorPendingSave = true;
        }
        else
        {
          it->second.PendingSave = true;
        }
      }
    }
  }
}

void TWinSCPFileSystem::EditViewCopyParam(TCopyParamType & CopyParam)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);
  CopyParam.SetReplaceInvalidChars(true);
  CopyParam.SetFileMask(L"");
}

void TWinSCPFileSystem::MultipleEdit()
{
  TFarPanelInfo * const * PanelInfo = GetPanelInfo();
  const TFarPanelItem * Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
  if ((Focused != nullptr) && Focused->GetIsFile() &&
    (Focused->GetUserData() != nullptr))
  {
    std::unique_ptr<TStrings> FileList(CreateFocusedFileList(osRemote));
    DebugAssert((FileList.get() == nullptr) || (FileList->GetCount() == 1));

    if ((FileList.get() != nullptr) && (FileList->GetCount() == 1))
    {
      MultipleEdit(FTerminal->GetCurrDirectory(), FileList->GetString(0),
        FileList->GetAs<TRemoteFile>(0));
    }
  }
}

void TWinSCPFileSystem::MultipleEdit(const UnicodeString & Directory,
  const UnicodeString & AFileName, TRemoteFile * AFile)
{
  DebugAssert(AFile);
  TEditHistory EditHistory;
  EditHistory.Directory = Directory;
  EditHistory.FileName = AFileName;

  TEditHistories::iterator it_h = rde::find(FEditHistories.begin(), FEditHistories.end(), EditHistory);
  if (it_h != FEditHistories.end())
  {
    FEditHistories.erase(it_h);
  }
  FEditHistories.push_back(EditHistory);

  UnicodeString FullFileName = core::UnixIncludeTrailingBackslash(Directory) + AFileName;

  std::unique_ptr<TRemoteFile> FileDuplicate(AFile ? AFile->Duplicate() : nullptr);
  UnicodeString NewFileName = AFileName; // FullFileName;
  FileDuplicate->SetFileName(NewFileName);

  TMultipleEdits::iterator it_e = FMultipleEdits.begin();
  while (it_e != FMultipleEdits.end())
  {
    const TMultipleEdit & ed = it_e->second;
    if (core::UnixSamePath(Directory, ed.Directory) &&
      (NewFileName == ed.FileName))
    {
      break;
    }
    ++it_e;
  }

  FLastMultipleEditReadOnly = false;
  bool EditCurrent = false;
  if (it_e != FMultipleEdits.end())
  {
    TMessageParams Params(0);
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaYes;
    Aliases[0].Alias = GetMsg(EDITOR_CURRENT);
    Aliases[1].Button = qaNo;
    Aliases[1].Alias = GetMsg(EDITOR_NEW_INSTANCE);
    Aliases[2].Button = qaOK;
    Aliases[2].Alias = GetMsg(EDITOR_NEW_INSTANCE_RO);
    Params.Aliases = Aliases;
    Params.AliasesCount = _countof(Aliases);
    switch (MoreMessageDialog(FORMAT(GetMsg(EDITOR_ALREADY_LOADED).c_str(), FullFileName.c_str()),
          nullptr, qtConfirmation, qaYes | qaNo | qaOK | qaCancel, &Params))
    {
    case qaYes:
      EditCurrent = true;
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

  if (EditCurrent)
  {
    DebugAssert(it_e != FMultipleEdits.end());

    intptr_t WindowCount = FarPlugin->FarAdvControl(ACTL_GETWINDOWCOUNT, 0);
    intptr_t Pos = 0;
    while (Pos < WindowCount)
    {
      WindowInfo Window;
      ClearStruct(Window);
      Window.StructSize = sizeof(WindowInfo);
      Window.Pos = (int)Pos;
      UnicodeString EditedFileName(1024, 0);
      Window.Name = const_cast<wchar_t *>(EditedFileName.c_str());
      Window.NameSize = static_cast<int>(EditedFileName.GetLength());
      if (FarPlugin->FarAdvControl(ACTL_GETWINDOWINFO, 0, &Window) != 0)
      {
        if ((Window.Type == WTYPE_EDITOR) &&
            Window.Name && ::AnsiSameText(Window.Name, it_e->second.LocalFileName))
        {
          // Switch to current editor.
          if (FarPlugin->FarAdvControl(ACTL_SETCURRENTWINDOW, Pos, nullptr) != 0)
          {
            FarPlugin->FarAdvControl(ACTL_COMMIT, 0);
          }
          break;
        }
      }
      Pos++;
    }

    DebugAssert(Pos < WindowCount);
  }
  else
  {
    UnicodeString TempDir;
    TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
    EditViewCopyParam(CopyParam);

    std::unique_ptr<TStrings> FileList(new TStringList());
    DebugAssert(!FNoProgressFinish);
    FNoProgressFinish = true;
    {
      SCOPE_EXIT
      {
        FNoProgressFinish = false;
      };
      FileList->AddObject(FullFileName, FileDuplicate.get());
      TemporarilyDownloadFiles(FileList.get(), CopyParam, TempDir);
    }
    UnicodeString ValidLocalFileName = CopyParam.ValidLocalFileName(NewFileName);
    FLastMultipleEditFile = ::IncludeTrailingBackslash(TempDir) + ValidLocalFileName;
    FLastMultipleEditFileTitle = AFileName;
    FLastMultipleEditDirectory = Directory;

    if (FarPlugin->Editor(FLastMultipleEditFile, FullFileName,
          EF_NONMODAL | EF_IMMEDIATERETURN | EF_DISABLEHISTORY))
    {
      // DebugAssert(FLastMultipleEditFile.IsEmpty());
    }
    FLastMultipleEditFile.Clear();
    FLastMultipleEditFileTitle.Clear();
  }
}

bool TWinSCPFileSystem::IsEditHistoryEmpty() const
{
  return FEditHistories.empty();
}

void TWinSCPFileSystem::EditHistory()
{
  std::unique_ptr<TFarMenuItems> MenuItems(new TFarMenuItems());
  TEditHistories::const_iterator it = FEditHistories.begin();
  while (it != FEditHistories.end())
  {
    MenuItems->Add(core::MinimizeName(core::UnixIncludeTrailingBackslash(it->Directory) + it->FileName,
      GetWinSCPPlugin()->MaxMenuItemLength(), true));
    ++it;
  }

  MenuItems->Add(L"");
  MenuItems->SetItemFocused(MenuItems->GetCount() - 1);

  const FarKey BreakKeys[] = {{ VK_F4, 0 }, { 0 }};

  intptr_t BreakCode = 0;
  intptr_t Result = GetWinSCPPlugin()->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
    GetMsg(MENU_EDIT_HISTORY), L"", MenuItems.get(), BreakKeys, BreakCode);

  if ((Result >= 0) && (Result < static_cast<intptr_t>(FEditHistories.size())))
  {
    TRemoteFile * File = nullptr;
    const TEditHistory & EditHistory = FEditHistories[Result];
    UnicodeString FullFileName =
      core::UnixIncludeTrailingBackslash(EditHistory.Directory) + EditHistory.FileName;
    FTerminal->ReadFile(FullFileName, File);
    std::unique_ptr<TRemoteFile> FilePtr(File);
    DebugAssert(FilePtr.get());
    if (File && !File->GetHaveFullFileName())
    {
      File->SetFullFileName(FullFileName);
    }
    MultipleEdit(EditHistory.Directory, EditHistory.FileName, File);
  }
}

bool TWinSCPFileSystem::IsLogging() const
{
  return Connected() && FTerminal->GetLog()->GetLoggingToFile();
}

void TWinSCPFileSystem::ShowLog()
{
  DebugAssert(Connected() && FTerminal->GetLog()->GetLoggingToFile());
  const TSessionLog * Log = FTerminal->GetLog();
  GetWinSCPPlugin()->Viewer(Log->GetCurrentFileName(), Log->GetCurrentFileName(), VF_NONMODAL);
}

UnicodeString TWinSCPFileSystem::GetFileNameHash(const UnicodeString & AFileName) const
{
  RawByteString Result;
  Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(AFileName.c_str()), static_cast<int>(AFileName.Length() * sizeof(wchar_t)),
    reinterpret_cast<uint8_t *>(const_cast<char *>(Result.c_str())));
  return BytesToHex(Result);
}

