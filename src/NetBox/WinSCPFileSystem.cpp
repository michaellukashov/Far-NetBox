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
#include <plugin.hpp>

TSessionPanelItem::TSessionPanelItem(const TSessionData *ASessionData) :
  TCustomFarPanelItem(OBJECT_CLASS_TSessionPanelItem)
{
  DebugAssert(ASessionData);
  FSessionData = ASessionData;
}

void TSessionPanelItem::SetPanelModes(TFarPanelModes *PanelModes)
{
  DebugAssert(FarPlugin);
  std::unique_ptr<TStrings> ColumnTitles(std::make_unique<TStringList>());
  ColumnTitles->Add(FarPlugin->GetMsg(NB_SESSION_NAME_COL_TITLE));
  for (int32_t Index = 0; Index < PANEL_MODES_COUNT; ++Index)
  {
    PanelModes->SetPanelMode(Index, L"N", L"0", ColumnTitles.get(), false, false, false);
  }
}

void TSessionPanelItem::SetKeyBarTitles(TFarKeyBarTitles *KeyBarTitles)
{
  KeyBarTitles->ClearKeyBarTitle(fsNone, 6);
  KeyBarTitles->SetKeyBarTitle(fsNone, 5, FarPlugin->GetMsg(NB_EXPORT_SESSION_KEYBAR));
  KeyBarTitles->ClearKeyBarTitle(fsShift, 1, 3);
  KeyBarTitles->SetKeyBarTitle(fsShift, 4, FarPlugin->GetMsg(NB_NEW_SESSION_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsShift, 5, FarPlugin->GetMsg(NB_COPY_SESSION_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsShift, 6, FarPlugin->GetMsg(NB_RENAME_SESSION_KEYBAR));
  KeyBarTitles->ClearKeyBarTitle(fsShift, 7, 8);
  KeyBarTitles->ClearKeyBarTitle(fsAlt, 6);
  KeyBarTitles->ClearKeyBarTitle(fsCtrl, 4, 11);
}

void TSessionPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString &AFileName, int64_t & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *&UserData, size_t & /*CustomColumnNumber*/)
{
  AFileName = base::UnixExtractFileName(FSessionData->GetName());
  UserData = nb::ToPtr(const_cast<TSessionData *>(FSessionData));
}

TSessionFolderPanelItem::TSessionFolderPanelItem(const UnicodeString Folder) :
  TCustomFarPanelItem(OBJECT_CLASS_TSessionFolderPanelItem),
  FFolder(Folder)
{
}

void TSessionFolderPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString &AFileName, int64_t & /*Size*/,
  uintptr_t &FileAttributes,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  AFileName = FFolder;
  FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
}

TRemoteFilePanelItem::TRemoteFilePanelItem(TRemoteFile *ARemoteFile) :
  TCustomFarPanelItem(OBJECT_CLASS_TRemoteFilePanelItem)
{
  DebugAssert(ARemoteFile);
  FRemoteFile = ARemoteFile;
}

void TRemoteFilePanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString &AFileName, int64_t &Size,
  uintptr_t &FileAttributes,
  TDateTime &LastWriteTime, TDateTime &LastAccess,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString &Owner, void *&UserData, size_t &CustomColumnNumber)
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

void TRemoteFilePanelItem::TranslateColumnTypes(UnicodeString &AColumnTypes,
  TStrings *ColumnTitles)
{
  UnicodeString ColumnTypes = AColumnTypes;
  AColumnTypes.Clear();
  UnicodeString Title;
  while (!ColumnTypes.IsEmpty())
  {
    UnicodeString Column = CutToChar(ColumnTypes, ',', false);
    if (Column == L"G")
    {
      Column = L"C0";
      Title = FarPlugin->GetMsg(NB_GROUP_COL_TITLE);
    }
    else if (Column == L"R")
    {
      Column = L"C1";
      Title = FarPlugin->GetMsg(NB_RIGHTS_COL_TITLE);
    }
    else if (Column == L"RO")
    {
      Column = L"C2";
      Title = FarPlugin->GetMsg(NB_RIGHTS_OCTAL_COL_TITLE);
    }
    else if (Column == L"L")
    {
      Column = L"C3";
      Title = FarPlugin->GetMsg(NB_LINK_TO_COL_TITLE);
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

void TRemoteFilePanelItem::SetPanelModes(TFarPanelModes *PanelModes)
{
  DebugAssert(FarPlugin);
  std::unique_ptr<TStrings> ColumnTitles(std::make_unique<TStringList>());
  TFarConfiguration *FarConfiguration = GetFarConfiguration();
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

void TRemoteFilePanelItem::SetKeyBarTitles(TFarKeyBarTitles *KeyBarTitles)
{
  KeyBarTitles->ClearKeyBarTitle(fsShift, 1, 3); // archive commands
  KeyBarTitles->SetKeyBarTitle(fsShift, 5, FarPlugin->GetMsg(NB_COPY_TO_FILE_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsShift, 6, FarPlugin->GetMsg(NB_MOVE_TO_FILE_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsAltShift, 12,
    FarPlugin->GetMsg(NB_OPEN_DIRECTORY_KEYBAR));
  KeyBarTitles->SetKeyBarTitle(fsAltShift, 6,
    FarPlugin->GetMsg(NB_RENAME_FILE_KEYBAR));
}

class TFarInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TFarInteractiveCustomCommand(TCustomFarPlugin *Plugin,
    TCustomCommand *ChildCustomCommand);

protected:
  virtual void Prompt(int32_t Index, const UnicodeString APrompt,
    UnicodeString &Value) const override;

private:
  TCustomFarPlugin *FPlugin;
};

TFarInteractiveCustomCommand::TFarInteractiveCustomCommand(
  TCustomFarPlugin *Plugin, TCustomCommand *ChildCustomCommand) :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FPlugin = Plugin;
}

void TFarInteractiveCustomCommand::Prompt(int32_t /*Index*/, const UnicodeString APrompt,
  UnicodeString &Value) const
{
  UnicodeString Prompt = APrompt;
  if (Prompt.IsEmpty())
  {
    Prompt = FPlugin->GetMsg(NB_APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!FPlugin->InputBox(FPlugin->GetMsg(NB_APPLY_COMMAND_PARAM_TITLE),
      Prompt, Value, 0, APPLY_COMMAND_PARAM_HISTORY))
  {
    Abort();
  }
}

// Attempt to allow keepalives from background thread.
// Not finished nor used.
NB_DEFINE_CLASS_ID(TKeepAliveThread);
class TKeepAliveThread : public TSimpleThread
{
public:
  explicit TKeepAliveThread(TWinSCPFileSystem *FileSystem, const TDateTime &Interval) noexcept;
  virtual ~TKeepAliveThread() = default;

  void InitKeepaliveThread();
  virtual void Execute() override;
  virtual void Terminate() override;

private:
  TWinSCPFileSystem *FFileSystem{nullptr};
  TDateTime FInterval;
  HANDLE FEvent{INVALID_HANDLE_VALUE};
};

TKeepAliveThread::TKeepAliveThread(TWinSCPFileSystem *FileSystem,
  const TDateTime &Interval) noexcept :
  TSimpleThread(OBJECT_CLASS_TKeepAliveThread),
  FFileSystem(FileSystem),
  FInterval(Interval)
{
}

void TKeepAliveThread::InitKeepaliveThread()
{
  TSimpleThread::InitSimpleThread();
  FEvent = ::CreateEvent(nullptr, false, false, nullptr);
  Start();
}

void TKeepAliveThread::Terminate()
{
  // TCompThread::Terminate();
  ::SetEvent(FEvent);
}

void TKeepAliveThread::Execute()
{
  while (!IsFinished())
  {
    if ((::WaitForSingleObject(FEvent, nb::ToDWord(
            nb::ToDouble(FInterval) * MSecsPerDay)) != WAIT_FAILED) &&
      !IsFinished())
    {
      FFileSystem->KeepaliveThreadCallback();
    }
  }
  SAFE_CLOSE_HANDLE(FEvent);
}

TWinSCPFileSystem::TWinSCPFileSystem(TCustomFarPlugin *APlugin) noexcept :
  TCustomFarFileSystem(OBJECT_CLASS_TWinSCPFileSystem, APlugin),
  FPathHistory(std::make_unique<TStringList>())
{
}

void TWinSCPFileSystem::Init(TSecureShell * /*SecureShell*/)
{
  TCustomFarFileSystem::Init();
}

TWinSCPFileSystem::~TWinSCPFileSystem() noexcept
{
  Disconnect();
  // SAFE_DESTROY(FPathHistory);
}

void TWinSCPFileSystem::HandleException(Exception *E, OPERATION_MODES OpMode)
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
  TGuard Guard(FCriticalSection); nb::used(Guard);

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

const TWinSCPPlugin *TWinSCPFileSystem::GetWinSCPPlugin() const
{
  return dyn_cast<const TWinSCPPlugin>(FPlugin);
}

TWinSCPPlugin *TWinSCPFileSystem::GetWinSCPPlugin()
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
        (MoreMessageDialog(GetMsg(NB_PENDING_QUEUE_ITEMS), nullptr, qtWarning,
            qaOK | qaCancel) == qaOK))
      {
        QueueShow(true);
      }
    }
  }
}

void TWinSCPFileSystem::GetOpenPanelInfoEx(OPENPANELINFO_FLAGS &Flags,
  UnicodeString &HostFile, UnicodeString &CurDir, UnicodeString &AFormat,
  UnicodeString &PanelTitle, TFarPanelModes *PanelModes, intptr_t & /*StartPanelMode*/,
  OPENPANELINFO_SORTMODES & /*StartSortMode*/, bool & /*StartSortOrder*/, TFarKeyBarTitles *KeyBarTitles,
  UnicodeString &ShortcutData)
{
  if (!IsSessionList())
  {
    Flags = !OPIF_DISABLEFILTER | !OPIF_DISABLESORTGROUPS | !OPIF_DISABLEHIGHLIGHTING |
      OPIF_SHOWPRESERVECASE | OPIF_COMPAREFATTIME | OPIF_SHORTCUT;

    // When slash is added to the end of path, windows style paths
    // (vandyke: c:/windows/system) are displayed correctly on command-line, but
    // leaved subdirectory is not focused, when entering parent directory.
    HostFile = GetSessionData()->GetHostName(); // GenerateSessionUrl(sufHostKey); // GetSessionData()->GetSessionName();
    CurDir = FTerminal->RemoteGetCurrentDirectory();
    UnicodeString SessionName = GetSessionData()->GetLocalName();
    AFormat = FORMAT("netbox:%s", SessionName);
    UnicodeString HostName = GetSessionData()->GetHostNameExpanded();
    UnicodeString Url = GetSessionData()->GenerateSessionUrl(sufComplete);
    if (GetFarConfiguration()->GetSessionNameInTitle())
    {
      PanelTitle = FORMAT(" %s:%s ", SessionName, CurDir);
    }
    else
    {
      PanelTitle = FORMAT(" %s:%s ", HostName, CurDir);
    }
    ShortcutData = FORMAT("netbox:%s\1%s", Url, CurDir);

    TRemoteFilePanelItem::SetPanelModes(PanelModes);
    TRemoteFilePanelItem::SetKeyBarTitles(KeyBarTitles);
  }
  else
  {
    CurDir = FSessionsFolder;
    AFormat = "netbox";
    Flags = !OPIF_DISABLESORTGROUPS | !OPIF_DISABLEHIGHLIGHTING | OPIF_USEATTRHIGHLIGHTING |
      OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE | OPIF_SHORTCUT;

    PanelTitle = FORMAT(" %s [/%s]", GetMsg(NB_STORED_SESSION_TITLE), FSessionsFolder);

    TSessionPanelItem::SetPanelModes(PanelModes);
    TSessionPanelItem::SetKeyBarTitles(KeyBarTitles);
  }
}

bool TWinSCPFileSystem::GetFindDataEx(TObjectList *PanelItems, OPERATION_MODES OpMode)
{
  bool Result;
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

      TCustomFileSystem *FileSystem = GetTerminal()->GetFileSystem();
      bool ResolveSymlinks = GetSessionData()->GetResolveSymlinks();
      for (int32_t Index = 0; Index < GetTerminal()->GetFiles()->GetCount(); ++Index)
      {
        TRemoteFile *File = GetTerminal()->GetFiles()->GetFile(Index);
        DebugAssert(File);
        if (ResolveSymlinks && File->GetIsSymLink())
        {
          if (FarPlugin->CheckForEsc())
            break;
          // Check what kind of symlink this is
          const UnicodeString LinkFileName = File->GetLinkTo();
          if (!LinkFileName.IsEmpty())
          {
            TRemoteFile *LinkFile = nullptr;
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
    bool SessionList = true;
    std::unique_ptr<THierarchicalStorage> Storage(GetConfiguration()->CreateScpStorage(SessionList));
    if (Storage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), False))
    {
      StoredSessions->Load(Storage.get());
    }
    UnicodeString Folder = FSessionsFolder;
    if (!FSessionsFolder.IsEmpty())
    {
      Folder = base::UnixIncludeTrailingBackslash(FSessionsFolder);
    }

    std::unique_ptr<TStringList> ChildPaths(std::make_unique<TStringList>());
    ChildPaths->SetCaseSensitive(false);
    for (int32_t Index = 0; Index < StoredSessions->GetCount(); ++Index)
    {
      const TSessionData *Data = StoredSessions->GetSession(Index);
      UnicodeString SessionName = Data->GetName();
      if (SessionName.SubString(1, Folder.Length()) == Folder)
      {
        UnicodeString Name = SessionName.SubString(
            Folder.Length() + 1, SessionName.Length() - Folder.Length());
        int32_t Slash = Name.Pos(L'/');
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
      PanelItems->Add(new THintPanelItem(GetMsg(NB_NEW_SESSION_HINT)));
    }

    TWinSCPFileSystem *OppositeFileSystem =
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
      const TSessionData *PrevSession = StoredSessions->GetSessionByName(FPrevSessionName);
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

void TWinSCPFileSystem::DuplicateOrRenameSession(TSessionData *Data,
  bool Duplicate)
{
  DebugAssert(Data);
  UnicodeString Name = Data->GetName();
  if (GetWinSCPPlugin()->InputBox(GetMsg(Duplicate ? NB_DUPLICATE_SESSION_TITLE : NB_RENAME_SESSION_TITLE),
      GetMsg(Duplicate ? NB_DUPLICATE_SESSION_PROMPT : NB_RENAME_SESSION_PROMPT),
      Name, 0) &&
    !Name.IsEmpty() && (Name != Data->GetName()))
  {
    Name = ReplaceChar(Name, L'\\', L'/');
    TNamedObject *EData = StoredSessions->FindByName(Name);
    if ((EData != nullptr) && (EData != Data))
    {
      throw Exception(FORMAT(GetMsg(NB_SESSION_ALREADY_EXISTS_ERROR), Name));
    }
    else
    {
      TSessionData *NData = StoredSessions->NewSession(Name, Data);
      FSessionsFolder = ::ExcludeTrailingBackslash(base::UnixExtractFilePath(Name));

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

void TWinSCPFileSystem::FocusSession(const TSessionData *Data)
{
  TFarPanelInfo **PanelInfo = GetPanelInfo();
  const TFarPanelItem *SessionItem = PanelInfo && *PanelInfo ? (*PanelInfo)->FindUserData(Data) : nullptr;
  if (SessionItem != nullptr)
  {
    (*PanelInfo)->SetFocusedItem(SessionItem);
  }
}

void TWinSCPFileSystem::EditConnectSession(TSessionData *Data, bool Edit)
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

void TWinSCPFileSystem::EditConnectSession(TSessionData *Data, bool Edit, bool NewData, bool FillInConnect)
{
  TSessionData *OrigData = Data;
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
        TSessionData *SelectSession = nullptr;
        if (NewData)
        {
          // UnicodeString Name =
          //    IncludeTrailingBackslash(FSessionsFolder) + Data->GetSessionName();
          UnicodeString Name;
          if (!FSessionsFolder.IsEmpty())
          {
            Name = base::UnixIncludeTrailingBackslash(FSessionsFolder);
          }
          if (Data)
            Name += Data->GetSessionName();
          if (GetWinSCPPlugin()->InputBox(GetMsg(NB_NEW_SESSION_NAME_TITLE),
              GetMsg(NB_NEW_SESSION_NAME_PROMPT), Name, 0) &&
            !Name.IsEmpty())
          {
            if (StoredSessions->FindByName(Name))
            {
              throw Exception(FORMAT(GetMsg(NB_SESSION_ALREADY_EXISTS_ERROR), Name));
            }
            else
            {
              SelectSession = StoredSessions->NewSession(Name, Data);
              FSessionsFolder = ::ExcludeTrailingBackslash(base::UnixExtractFilePath(Name));
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
      TFarPanelInfo **PanelInfo = GetPanelInfo();
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
  const UnicodeString & AddedLine, TCaptureOutputType OutputEvent)
{
  if (OutputEvent == cotExitCode)
    return;
  if (FOutputLog)
  {
    GetWinSCPPlugin()->FarWriteConsole(AddedLine + L"\n");
  }
  if (FCapturedLog != nullptr)
  {
    FCapturedLog->Add(AddedLine);
  }
}

void TWinSCPFileSystem::RequireLocalPanel(TFarPanelInfo *Panel, const UnicodeString Message)
{
  if (Panel->GetIsPlugin() || (Panel->GetType() != ptFile))
  {
    throw Exception(Message);
  }
}

void TWinSCPFileSystem::RequireCapability(int32_t Capability)
{
  if (!FTerminal->GetIsCapable(static_cast<TFSCapability>(Capability)))
  {
    throw Exception(FORMAT(GetMsg(NB_OPERATION_NOT_SUPPORTED),
        FTerminal->GetFileSystemInfo().ProtocolName));
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
      TMessageParams Params(nullptr);
      Params.Params = qpNeverAskAgainCheck;
      uint32_t Answer = MoreMessageDialog(
          FORMAT(GetMsg(NB_PERFORM_ON_COMMAND_SESSION),
            FTerminal->GetFileSystemInfo().ProtocolName,
            FTerminal->GetFileSystemInfo().ProtocolName),
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

bool TWinSCPFileSystem::ExecuteCommand(const UnicodeString Command)
{
  if (FTerminal->AllowedAnyCommand(Command) &&
    EnsureCommandSessionFallback(fcAnyCommand))
  {
    FTerminal->BeginTransaction();
    {
      SCOPE_EXIT
      {
        if (FTerminal->InTransaction())
          FTerminal->EndTransaction();
        if (FTerminal->GetActive())
        {
          UpdatePanel();
        }
        else
        {
          RedrawPanel();
          RedrawPanel(true);
        }
      };
      FarControl(FCTL_SETCMDLINE, 0, reinterpret_cast<void *>(L""));
      TWinSCPPlugin *WinSCPPlugin = GetWinSCPPlugin();
      WinSCPPlugin->ShowConsoleTitle(Command);
      {
        SCOPE_EXIT
        {
          //WinSCPPlugin->ScrollTerminalScreen(1);
          WinSCPPlugin->SaveTerminalScreen();
          WinSCPPlugin->ClearConsoleTitle();
        };
        WinSCPPlugin->ShowTerminalScreen(Command);

        FOutputLog = true;
        FTerminal->AnyCommand(Command, nb::bind(&TWinSCPFileSystem::TerminalCaptureLog, this));
      }
    }
  }
  return true;
}

bool TWinSCPFileSystem::ProcessKeyEx(int32_t Key, uint32_t ControlState)
{
  bool Handled = false;

  TFarPanelInfo *const *PanelInfo = GetPanelInfo();
  const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;

  if ((Key == 'W') && (ControlState & SHIFTMASK) &&
    (ControlState & ALTMASK))
  {
    GetWinSCPPlugin()->CommandsMenu(true);
    Handled = true;
  }
  else if (IsSessionList())
  {
    TSessionData *Data = nullptr;
    if ((Focused != nullptr) && Focused->GetIsFile() && Focused->GetUserData())
    {
      Data = cast_to<TSessionData>(Focused->GetUserData());
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
      RemoteCreateLink();
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

void TWinSCPFileSystem::RemoteCreateLink()
{
  RequireCapability(fcResolveSymlink);
  RequireCapability(fcSymbolicLink);

  bool Edit = false;
  TRemoteFile *File = nullptr;
  UnicodeString FileName;
  UnicodeString PointTo;
  bool SymbolicLink = true;

  TFarPanelInfo *const *PanelInfo = GetPanelInfo();
  const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
  if (Focused && Focused->GetUserData())
  {
    File = cast_to<TRemoteFile>(Focused->GetUserData());

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
      int32_t Params = dfNoRecursive;
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
      GetTerminal()->RemoteCreateLink(FileName, PointTo, SymbolicLink);
    if (UpdatePanel())
    {
      RedrawPanel();
    }
  }
}

void TWinSCPFileSystem::TemporarilyDownloadFiles(TStrings *AFileList, TCopyParamType &CopyParam, UnicodeString &TempDir)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);

  TempDir = GetWinSCPPlugin()->GetTemporaryDir();
  if (TempDir.IsEmpty() || !::SysUtulsForceDirectories(ApiPath(TempDir)))
  {
    throw Exception(FMTLOAD(NB_CREATE_TEMP_DIR_ERROR, TempDir));
  }

  FTerminal->SetExceptionOnFail(true);
  {
    SCOPE_EXIT
    {
      FTerminal->SetExceptionOnFail(false);
    };
    try
    {
      FTerminal->CopyToLocal(AFileList, TempDir, &CopyParam, cpTemporary, nullptr);
    }
    catch (...)
    {
      try
      {
        RecursiveDeleteFile(::ExcludeTrailingBackslash(TempDir), false);
      }
      catch (...)
      {
        DEBUG_PRINTF("TWinSCPFileSystem::TemporarilyDownloadFiles: error during RecursiveDeleteFile");
      }
      throw;
    }
  }
}

void TWinSCPFileSystem::ApplyCommand()
{
  TFarPanelInfo **PanelInfo = this->GetPanelInfo();
  if (PanelInfo && *PanelInfo && (*PanelInfo)->GetSelectedCount(true) == 0)
  {
    MoreMessageDialog(GetMsg(MSG_NO_FILES_SELECTED), nullptr,
      qtInformation, qaOK);
    return;
  }

  std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote, PanelInfo));
  if (FileList != nullptr)
  {
    TFarConfiguration *FarConfiguration = GetFarConfiguration();
    int32_t Params = FarConfiguration->GetApplyCommandParams();
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
          TRemoteCustomCommand RemoteCustomCommand(Data, GetTerminal()->RemoteGetCurrentDirectory());
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
              FCapturedLog = std::make_unique<TStringList>();
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
                  GetWinSCPPlugin()->FarCopyToClipboard(FCapturedLog.get());
                  FCapturedLog.reset();
                }
              };
              if (FLAGSET(Params, ccShowResults))
              {
                GetWinSCPPlugin()->ShowTerminalScreen(Command);
              }

              FTerminal->CustomCommandOnFiles(Command, Params, FileList.get(), OutputEvent);
            }
          }
        }
      }
      else
      {
        TCustomCommandData Data1(GetTerminal());
        TLocalCustomCommand LocalCustomCommand(Data1, GetTerminal()->RemoteGetCurrentDirectory(), L"");
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
            TFarPanelInfo **AnotherPanel = GetAnotherPanelInfo();
            RequireLocalPanel(*AnotherPanel, GetMsg(NB_APPLY_COMMAND_LOCAL_PATH_REQUIRED));

            LocalFileList.reset(CreateSelectedFileList(osLocal, AnotherPanel));

            if (FileListCommand)
            {
              if ((LocalFileList == nullptr) || (LocalFileList->GetCount() != 1))
              {
                throw Exception(GetMsg(NB_CUSTOM_COMMAND_SELECTED_UNMATCH1));
              }
            }
            else
            {
              if ((LocalFileList == nullptr) ||
                ((LocalFileList->GetCount() != 1) &&
                  (FileList->GetCount() != 1) &&
                  (LocalFileList->GetCount() != FileList->GetCount())))
              {
                throw Exception(GetMsg(NB_CUSTOM_COMMAND_SELECTED_UNMATCH));
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
            RemoteFileList = std::make_unique<TStringList>();

            TMakeLocalFileListParams MakeFileListParam;
            MakeFileListParam.FileList = RemoteFileList.get();
            MakeFileListParam.IncludeDirs = FLAGSET(Params, ccApplyToDirectories);
            MakeFileListParam.Recursive =
              FLAGSET(Params, ccRecursive) && !FileListCommand;

            ProcessLocalDirectory(TempDir, nb::bind(&TTerminal::MakeLocalFileList, FTerminal), &MakeFileListParam);

            TFileOperationProgressType Progress(nb::bind(&TWinSCPFileSystem::OperationProgress, this), nb::bind(&TWinSCPFileSystem::OperationFinished, this));

            Progress.Start(foCustomCommand, osRemote, nb::ToInt32(FileListCommand ? 1 : FileList->GetCount()));
            {
              SCOPE_EXIT
              {
                Progress.Stop();
              };
              if (FileListCommand)
              {
                UnicodeString LocalFile;
                UnicodeString FileList2 = base::MakeFileList(RemoteFileList.get());

                if (LocalFileCommand)
                {
                  DebugAssert(LocalFileList->GetCount() == 1);
                  LocalFile = LocalFileList->GetString(0);
                }

                TCustomCommandData Data2(FTerminal);
                TLocalCustomCommand CustomCommand(Data2,
                  GetTerminal()->RemoteGetCurrentDirectory(), L"", L"", LocalFile, FileList2);
                ExecuteShellCheckedAndWait(CustomCommand.Complete(Command, true),
                  TProcessMessagesEvent());
              }
              else if (LocalFileCommand)
              {
                if (LocalFileList->GetCount() == 1)
                {
                  UnicodeString LocalFile = LocalFileList->GetString(0);

                  for (int32_t Index = 0; Index < RemoteFileList->GetCount(); ++Index)
                  {
                    UnicodeString FileName = RemoteFileList->GetString(Index);
                    TCustomCommandData Data3(FTerminal);
                    TLocalCustomCommand CustomCommand(Data3,
                      GetTerminal()->RemoteGetCurrentDirectory(), FileName, L"", LocalFile, L"");
                    ExecuteShellCheckedAndWait(
                      CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                  }
                }
                else if (RemoteFileList->GetCount() == 1)
                {
                  UnicodeString FileName = RemoteFileList->GetString(0);

                  for (int32_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
                  {
                    TCustomCommandData Data4(FTerminal);
                    TLocalCustomCommand CustomCommand(
                      Data4, GetTerminal()->RemoteGetCurrentDirectory(),
                      L"", FileName, LocalFileList->GetString(Index), L"");
                    ExecuteShellCheckedAndWait(
                      CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                  }
                }
                else
                {
                  if (LocalFileList->GetCount() != RemoteFileList->GetCount())
                  {
                    throw Exception(GetMsg(NB_CUSTOM_COMMAND_PAIRS_DOWNLOAD_FAILED));
                  }

                  for (int32_t Index = 0; Index < LocalFileList->GetCount(); ++Index)
                  {
                    UnicodeString FileName = RemoteFileList->GetString(Index);
                    TCustomCommandData Data5(FTerminal);
                    TLocalCustomCommand CustomCommand(
                      Data5, GetTerminal()->RemoteGetCurrentDirectory(),
                      L"", FileName, LocalFileList->GetString(Index), L"");
                    ExecuteShellCheckedAndWait(
                      CustomCommand.Complete(Command, true), TProcessMessagesEvent());
                  }
                }
              }
              else
              {
                for (int32_t Index = 0; Index < RemoteFileList->GetCount(); ++Index)
                {
                  TCustomCommandData Data6(FTerminal);
                  TLocalCustomCommand CustomCommand(Data6,
                    GetTerminal()->RemoteGetCurrentDirectory(), L"", RemoteFileList->GetString(Index), L"", L"");
                  ExecuteShellCheckedAndWait(
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

void TWinSCPFileSystem::Synchronize(UnicodeString LocalDirectory,
  UnicodeString RemoteDirectory, TTerminal::TSynchronizeMode Mode,
  const TCopyParamType &CopyParam, int32_t Params, TSynchronizeChecklist **AChecklist,
  TSynchronizeOptions *Options)
{
  TSynchronizeChecklist *Checklist = nullptr;
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
    GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
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
    GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_SYNCHRONIZE_PROGRESS_TITLE));
    FSynchronizationStart = Now();
    FSynchronizationCompare = false;
    {
      SCOPE_EXIT
      {
        GetWinSCPPlugin()->ClearConsoleTitle();
        GetWinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
      };
      FTerminal->SynchronizeApply(Checklist, &CopyParam,
                                  Params | TTerminal::spNoConfirmation,
                                  nb::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this),
                                  nullptr, nullptr, nullptr, nullptr);
//        LocalDirectory, RemoteDirectory,
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
  int32_t Params, TSynchronizeOptions &Options)
{
  if (FLAGSET(Params, TTerminal::spSelectedOnly) && SynchronizeAllowSelectedOnly())
  {
    Options.Filter = new TStringList();
    Options.Filter->SetCaseSensitive(false);
    Options.Filter->SetDuplicates(dupAccept);

    TFarPanelInfo **PanelInfo = GetPanelInfo();
    if (PanelInfo && *PanelInfo && (*PanelInfo)->GetSelectedCount() > 0)
    {
      CreateFileList((*PanelInfo)->GetItems(), osRemote, true, "", true, Options.Filter);
    }
    if ((*GetAnotherPanelInfo())->GetSelectedCount() > 0)
    {
      CreateFileList((*GetAnotherPanelInfo())->GetItems(), osLocal, true, "", true, Options.Filter);
    }
    Options.Filter->Sort();
  }
}

void TWinSCPFileSystem::FullSynchronize(bool Source)
{
  TFarPanelInfo **AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(*AnotherPanel, GetMsg(NB_SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  UnicodeString LocalDirectory = (*AnotherPanel)->GetCurrDirectory();
  UnicodeString RemoteDirectory = FTerminal->RemoteGetCurrentDirectory();

  bool SaveMode = !(GetGUIConfiguration()->GetSynchronizeModeAuto() < 0);
  TTerminal::TSynchronizeMode Mode =
    SaveMode ? static_cast<TTerminal::TSynchronizeMode>(GetGUIConfiguration()->GetSynchronizeModeAuto()) :
    (Source ? TTerminal::smLocal : TTerminal::smRemote);
  int32_t Params = GetGUIConfiguration()->GetSynchronizeParams();
  bool SaveSettings = false;

  TGUICopyParamType CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  TUsableCopyParamAttrs CopyParamAttrs = GetTerminal()->UsableCopyParamAttrs(0);
  int32_t Options =
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
      GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_SYNCHRONIZE_PROGRESS_COMPARE_TITLE));
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
        MoreMessageDialog(GetMsg(NB_COMPARE_NO_DIFFERENCES), nullptr,
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
        GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_SYNCHRONIZE_PROGRESS_TITLE));
        FSynchronizationStart = Now();
        FSynchronizationCompare = false;
        {
          SCOPE_EXIT
          {
            GetWinSCPPlugin()->ClearConsoleTitle();
            GetWinSCPPlugin()->RestoreScreen(FSynchronizationSaveScreenHandle);
          };
          FTerminal->SynchronizeApply(Checklist.get(), &CopyParam,
                                      Params | TTerminal::spNoConfirmation,
                                      nb::bind(&TWinSCPFileSystem::TerminalSynchronizeDirectory, this),
                                      nullptr, nullptr, nullptr, nullptr);
//            LocalDirectory, RemoteDirectory,
        }
      }
    }
  }
}

void TWinSCPFileSystem::TerminalSynchronizeDirectory(
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  bool &Continue, bool Collect, const TSynchronizeOptions * SynchronizeOptions)
{
  static uint32_t LastTicks;
  uint32_t Ticks = ::GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    static const int32_t ProgressWidth = 48;
    static UnicodeString ProgressTitle;
    static UnicodeString ProgressTitleCompare;
    static UnicodeString LocalLabel;
    static UnicodeString RemoteLabel;
    static UnicodeString StartTimeLabel;
    static UnicodeString TimeElapsedLabel;

    if (ProgressTitle.IsEmpty())
    {
      ProgressTitle = GetMsg(NB_SYNCHRONIZE_PROGRESS_TITLE);
      ProgressTitleCompare = GetMsg(NB_SYNCHRONIZE_PROGRESS_COMPARE_TITLE);
      LocalLabel = GetMsg(NB_SYNCHRONIZE_PROGRESS_LOCAL);
      RemoteLabel = GetMsg(NB_SYNCHRONIZE_PROGRESS_REMOTE);
      StartTimeLabel = GetMsg(NB_SYNCHRONIZE_PROGRESS_START_TIME);
      TimeElapsedLabel = GetMsg(NB_SYNCHRONIZE_PROGRESS_ELAPSED);
    }

    UnicodeString Message = LocalLabel + base::MinimizeName(LocalDirectory,
        ProgressWidth - LocalLabel.Length(), false);
    Message += ::StringOfChar(L' ', ProgressWidth - Message.Length()) + L"\n";
    Message += RemoteLabel + base::MinimizeName(RemoteDirectory,
        ProgressWidth - RemoteLabel.Length(), true) + L"\n";
    Message += StartTimeLabel + FSynchronizationStart.GetTimeString(false) + L"\n";
    Message += TimeElapsedLabel +
      FormatDateTimeSpan(GetConfiguration()->TimeFormat(), TDateTime(Now() - FSynchronizationStart)) + L"\n";

    GetWinSCPPlugin()->Message(0, (Collect ? ProgressTitleCompare : ProgressTitle), Message);

    if (GetWinSCPPlugin()->CheckForEsc() &&
      (MoreMessageDialog(GetMsg(NB_CANCEL_OPERATION), nullptr,
          qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      Continue = false;
    }
  }
}

void TWinSCPFileSystem::Synchronize()
{
  TFarPanelInfo **AnotherPanel = GetAnotherPanelInfo();
  RequireLocalPanel(*AnotherPanel, GetMsg(NB_SYNCHRONIZE_LOCAL_PATH_REQUIRED));

  TSynchronizeParamType Params;
  Params.LocalDirectory = (*AnotherPanel)->GetCurrDirectory();
  Params.RemoteDirectory = FTerminal->RemoteGetCurrentDirectory();
  int32_t UnusedParams = (GetGUIConfiguration()->GetSynchronizeParams() &
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
    uint32_t Options =
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
  TSynchronizeController * /*Sender*/, UnicodeString LocalDirectory,
  UnicodeString RemoteDirectory, const TCopyParamType &CopyParam,
  const TSynchronizeParamType &Params, TSynchronizeChecklist **Checklist,
  TSynchronizeOptions *Options, bool Full)
{
  try
  {
    int32_t PParams = Params.Params;
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
  catch (Exception &E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    throw;
  }
}

void TWinSCPFileSystem::DoSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, UnicodeString Directory,
  UnicodeString /*ErrorStr*/)
{
  UnicodeString Message;
  if (!Directory.IsEmpty())
  {
    Message = FORMAT(GetMsg(NB_WATCH_ERROR_DIRECTORY), Directory);
  }
  else
  {
    Message = GetMsg(NB_WATCH_ERROR_GENERAL);
  }

  MoreMessageDialog(Message, nullptr, qtError, qaOK);
}

void TWinSCPFileSystem::DoSynchronizeTooManyDirectories(
  TSynchronizeController * /*Sender*/, int32_t &MaxDirectories)
{
  if (MaxDirectories < GetGUIConfiguration()->GetMaxWatchDirectories())
  {
    MaxDirectories = GetGUIConfiguration()->GetMaxWatchDirectories();
  }
  else
  {
    TMessageParams Params(nullptr);
    Params.Params = qpNeverAskAgainCheck;
    uint32_t Result = MoreMessageDialog(
        FORMAT(GetMsg(NB_TOO_MANY_WATCH_DIRECTORIES), MaxDirectories, MaxDirectories), nullptr,
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
  const UnicodeString AName, UnicodeString &Value)
{
  UnicodeString Name = AName;
  if (Name.IsEmpty())
  {
    Name = GetMsg(NB_APPLY_COMMAND_PARAM_PROMPT);
  }
  if (!GetWinSCPPlugin()->InputBox(GetMsg(NB_APPLY_COMMAND_PARAM_TITLE),
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
    if (FileList)
    {
      DebugAssert(!FPanelItems);
      UnicodeString Target = FTerminal->RemoteGetCurrentDirectory();
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
          GetTerminal()->TerminalMoveFiles(FileList.get(), Target, FileMask, false);
        }
        else
        {
          GetTerminal()->TerminalCopyFiles(FileList.get(), Target, FileMask, false); //TODO: use option DontOverwrite
        }
      }
    }
  }
}

void TWinSCPFileSystem::RenameFile()
{
  TFarPanelInfo *const *PanelInfo = GetPanelInfo();
  const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
  DebugAssert(Focused != nullptr);

  if (Focused && !Focused->GetIsParentDirectory())
  {
    RequireCapability(fcRename);

    TRemoteFile *File = cast_to<TRemoteFile>(Focused->GetUserData());
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
      GetTerminal()->TerminalRenameFile(File, NewName);
    }
  }
}

void TWinSCPFileSystem::FileProperties()
{
  std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote));
  if (FileList)
  {
    DebugAssert(!FPanelItems);

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
      TRemoteProperties CurrentProperties = TRemoteProperties::CommonProperties(FileList.get());

      int32_t Flags = 0;
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
      if (PropertiesDialog(FileList.get(), FTerminal->RemoteGetCurrentDirectory(),
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

void TWinSCPFileSystem::InsertTokenOnCommandLine(const UnicodeString Token, bool Separate)
{
  UnicodeString Token2 = Token;
  if (!Token2.IsEmpty())
  {
    if (Token2.Pos(L' ') > 0)
    {
      Token2 = FORMAT("\"%s\"", Token2);
    }

    if (Separate)
    {
      Token2 += L" ";
    }

    FarControl(FCTL_INSERTCMDLINE, 0, nb::ToPtr(ToWChar(Token2)));
  }
}

void TWinSCPFileSystem::InsertSessionNameOnCommandLine()
{
  TFarPanelInfo *const *PanelInfo = GetPanelInfo();
  const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;

  if (Focused != nullptr)
  {
    TSessionData *SessionData = cast_to<TSessionData>(Focused->GetUserData());
    UnicodeString Name;
    if (SessionData != nullptr)
    {
      Name = SessionData->GetName();
    }
    else
    {
      Name = base::UnixIncludeTrailingBackslash(FSessionsFolder);
      if (!Focused->GetIsParentDirectory())
      {
        Name = base::UnixIncludeTrailingBackslash(Name + Focused->GetFileName());
      }
    }
    InsertTokenOnCommandLine(Name, true);
  }
}

void TWinSCPFileSystem::InsertFileNameOnCommandLine(bool Full)
{
  TFarPanelInfo *const *PanelInfo = GetPanelInfo();
  const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;

  if (Focused != nullptr)
  {
    if (!Focused->GetIsParentDirectory())
    {
      const TRemoteFile *File = reinterpret_cast<const TRemoteFile *>(Focused->GetUserData());
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
      InsertTokenOnCommandLine(base::UnixIncludeTrailingBackslash(FTerminal->RemoteGetCurrentDirectory()), true);
    }
  }
}

UnicodeString TWinSCPFileSystem::GetFullFilePath(const TRemoteFile *AFile) const
{
  UnicodeString SessionUrl = GetSessionUrl(FTerminal, true);
  UnicodeString Result = FORMAT("%s%s", SessionUrl, AFile->GetFullFileName());
  return Result;
}

// not used
void TWinSCPFileSystem::InsertPathOnCommandLine()
{
  InsertTokenOnCommandLine(FTerminal->RemoteGetCurrentDirectory(), false);
}

void TWinSCPFileSystem::CopyFullFileNamesToClipboard()
{
  std::unique_ptr<TStrings> FileList(CreateSelectedFileList(osRemote));
  std::unique_ptr<TStrings> FileNames(std::make_unique<TStringList>());
  if (FileList != nullptr)
  {
    for (int32_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      const TRemoteFile *File = reinterpret_cast<const TRemoteFile *>(FileList->GetObj(Index));
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
    TFarPanelInfo *const *PanelInfo = GetPanelInfo();
    const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
    if (PanelInfo && *PanelInfo && ((*PanelInfo)->GetSelectedCount() == 0) &&
      Focused && Focused->GetIsParentDirectory())
    {
      FileNames->Add(base::UnixIncludeTrailingBackslash(FTerminal->RemoteGetCurrentDirectory()));
    }
  }

  GetWinSCPPlugin()->FarCopyToClipboard(FileNames.get());
}

void TWinSCPFileSystem::GetSpaceAvailable(const UnicodeString APath,
  TSpaceAvailable &ASpaceAvailable, bool &Close)
{
  // terminal can be already closed (e.g. dropped connection)
  if ((GetTerminal() != nullptr) && GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    try
    {
      GetTerminal()->SpaceAvailable(APath, ASpaceAvailable);
    }
    catch (Exception &E)
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
  const TSessionInfo &SessionInfo = GetTerminal()->GetSessionInfo();
  TFileSystemInfo FileSystemInfo = GetTerminal()->GetFileSystemInfo();
  TGetSpaceAvailableEvent OnGetSpaceAvailable;
  if (GetTerminal()->GetIsCapable(fcCheckingSpaceAvailable))
  {
    OnGetSpaceAvailable = nb::bind(&TWinSCPFileSystem::GetSpaceAvailable, this);
  }
  FileSystemInfoDialog(SessionInfo, FileSystemInfo, GetTerminal()->RemoteGetCurrentDirectory(),
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
  ::OpenSessionInPutty(GetSessionData());
}

void TWinSCPFileSystem::QueueShow(bool ClosingPlugin)
{
  DebugAssert(Connected());
  TTerminalQueueStatus *QueueStatus = GetQueueStatus();
  DebugAssert(QueueStatus != nullptr);
  QueueDialog(QueueStatus, ClosingPlugin);
  ProcessQueue(true);
}

void TWinSCPFileSystem::OpenDirectory(bool Add)
{
  std::unique_ptr<TBookmarkList> BookmarkList(std::make_unique<TBookmarkList>());
  UnicodeString Directory = FTerminal->RemoteGetCurrentDirectory();
  UnicodeString SessionKey = GetSessionData()->GetSessionKey();

  TBookmarkList *CurrentBookmarkList = GetFarConfiguration()->GetBookmarks(SessionKey);
  if (CurrentBookmarkList != nullptr)
  {
    BookmarkList->Assign(CurrentBookmarkList);
  }

  if (Add)
  {
    TBookmark *Bookmark = new TBookmark();
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
      GetMsg(NB_SYNCHRONIZE_BROWSING_ON) : GetMsg(NB_SYNCHRONIZE_BROWSING_OFF);
    TMessageParams Params(nullptr);
    Params.Params = qpNeverAskAgainCheck;
    if (MoreMessageDialog(Message, nullptr, qtInformation, qaOK, &Params) ==
      qaNeverAskAgain)
    {
      GetFarConfiguration()->SetConfirmSynchronizedBrowsing(false);
    }
  }
}

bool TWinSCPFileSystem::SynchronizeBrowsing(const UnicodeString NewPath)
{
  bool Result;
  TFarPanelInfo **AnotherPanel = GetAnotherPanelInfo();
  UnicodeString OldPath = AnotherPanel && *AnotherPanel ? (*AnotherPanel)->GetCurrDirectory() : "";
  // IncludeTrailingBackslash to expand C: to C:\.
  UnicodeString LocalPath = ::IncludeTrailingBackslash(NewPath);
  FarPanelDirectory fpd;
  nb::ClearStruct(fpd);
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
    if (AnotherPanel && *AnotherPanel && !::SamePaths((*AnotherPanel)->GetCurrDirectory(), NewPath))
    {
      // FAR WORKAROUND
      // If FCTL_SETPANELDIR above fails, Far default current
      // directory to initial (?) one. So move this back to
      // previous directory.
      nb::ClearStruct(fpd);
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

bool TWinSCPFileSystem::SetDirectoryEx(UnicodeString Dir, OPERATION_MODES OpMode)
{
  if (!IsSessionList() && !Connected())
  {
    return false;
  }
  // FAR WORKAROUND
  // workaround to ignore "change to root directory" command issued by FAR,
  // before file is opened for viewing/editing from "find file" dialog
  // when plugin uses UNIX style paths
  if ((OpMode & OPM_FIND) && (OpMode & OPM_SILENT) && (Dir == L"\\"))
  {
    if (FSavedFindFolder.IsEmpty())
    {
      return true;
    }
    bool Result;
    SCOPE_EXIT
    {
      // Result = SetDirectoryEx(Dir, OpMode);
      FSavedFindFolder.Clear();
    };
    Result = SetDirectoryEx(FSavedFindFolder, OpMode);
    return Result;
  }
  if ((OpMode & OPM_FIND) && FSavedFindFolder.IsEmpty() && FTerminal)
  {
    FSavedFindFolder = FTerminal->RemoteGetCurrentDirectory();
  }

  if (IsSessionList())
  {
    FSessionsFolder = base::AbsolutePath(ROOTDIRECTORY + FSessionsFolder, Dir);
    DebugAssert(FSessionsFolder[1] == L'/');
    FSessionsFolder.Delete(1, 1);
    FNewSessionsFolder.Clear();
  }
  else
  {
    DebugAssert(!FNoProgress);
    bool Normal = FLAGCLEAR(OpMode, OPM_FIND | OPM_SILENT);
    UnicodeString PrevPath = FTerminal ? FTerminal->RemoteGetCurrentDirectory() : "";
    FNoProgress = !Normal;
    if (!FNoProgress)
    {
      GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_CHANGING_DIRECTORY_TITLE));
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
      else if ((Dir == PARENTDIRECTORY) && (FTerminal->RemoteGetCurrentDirectory() == ROOTDIRECTORY))
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
      (PrevPath != FTerminal->RemoteGetCurrentDirectory()))
    {
      TFarPanelInfo **AnotherPanel = GetAnotherPanelInfo();
      if (AnotherPanel && *AnotherPanel && ((*AnotherPanel)->GetIsPlugin() || ((*AnotherPanel)->GetType() != ptFile)))
      {
        MoreMessageDialog(GetMsg(NB_SYNCHRONIZE_LOCAL_PATH_REQUIRED), nullptr, qtError, qaOK);
      }
      else if (AnotherPanel && *AnotherPanel)
      {
        try
        {
          UnicodeString RemotePath = base::UnixIncludeTrailingBackslash(FTerminal->RemoteGetCurrentDirectory());
          UnicodeString FullPrevPath = base::UnixIncludeTrailingBackslash(PrevPath);
          UnicodeString LocalPath;
          if (RemotePath.SubString(1, FullPrevPath.Length()) == FullPrevPath && AnotherPanel)
          {
            LocalPath = IncludeTrailingBackslash((*AnotherPanel)->GetCurrDirectory()) +
              base::FromUnixPath(RemotePath.SubString(FullPrevPath.Length() + 1,
                  RemotePath.Length() - FullPrevPath.Length()));
          }
          else if (FullPrevPath.SubString(1, RemotePath.Length()) == RemotePath && AnotherPanel)
          {
            LocalPath = ExcludeTrailingBackslash((*AnotherPanel)->GetCurrDirectory());
            while (!base::UnixSamePath(FullPrevPath, RemotePath))
            {
              UnicodeString NewLocalPath = ExcludeTrailingBackslash(ExtractFileDir(LocalPath));
              if (NewLocalPath == LocalPath)
              {
                Abort();
              }
              LocalPath = NewLocalPath;
              FullPrevPath = base::UnixExtractFilePath(base::UnixExcludeTrailingBackslash(FullPrevPath));
            }
          }
          else
          {
            Abort();
          }

          if (!SynchronizeBrowsing(LocalPath))
          {
            if (MoreMessageDialog(FORMAT(GetMsg(NB_SYNC_DIR_BROWSE_CREATE), LocalPath),
                nullptr, qtInformation, qaYes | qaNo) == qaYes)
            {
              if (!::SysUtulsForceDirectories(ApiPath(LocalPath)))
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
        catch (Exception &E)
        {
          FSynchronisingBrowse = false;
          GetWinSCPPlugin()->ShowExtendedException(&E);
          MoreMessageDialog(GetMsg(NB_SYNC_DIR_BROWSE_ERROR), nullptr, qtInformation, qaOK);
        }
      }
    }
  }

  return true;
}

int32_t TWinSCPFileSystem::MakeDirectoryEx(UnicodeString &Name, OPERATION_MODES OpMode)
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

      GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_CREATING_FOLDER));
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
        GetWinSCPPlugin()->InputBox(GetMsg(NB_CREATE_FOLDER_TITLE),
          ::StripHotkey(GetMsg(NB_CREATE_FOLDER_PROMPT)),
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

void TWinSCPFileSystem::DeleteSession(TSessionData *Data, void * /*AParam*/)
{
  Data->Remove();
  StoredSessions->Remove(Data);
}

void TWinSCPFileSystem::ProcessSessions(TObjectList *PanelItems,
  TProcessSessionEvent ProcessSession, void *AParam)
{
  for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
  {
    TFarPanelItem *PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
    DebugAssert(PanelItem);
    if (PanelItem->GetIsFile())
    {
      if (PanelItem->GetUserData() != nullptr)
      {
        ProcessSession(cast_to<TSessionData>(PanelItem->GetUserData()), AParam);
        PanelItem->SetSelected(false);
      }
      else
      {
        UnicodeString Msg = GetMsg(NB_NEW_SESSION_HINT);
        DebugAssert(PanelItem->GetFileName() == Msg);
      }
    }
    else
    {
      DebugAssert(PanelItem->GetUserData() == nullptr);
      UnicodeString Folder = base::UnixIncludeTrailingBackslash(
          base::UnixIncludeTrailingBackslash(FSessionsFolder) + PanelItem->GetFileName());
      int32_t Index2 = 0;
      while (Index2 < StoredSessions->GetCount())
      {
        TSessionData *Data = StoredSessions->GetSession(Index2);
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

bool TWinSCPFileSystem::DeleteFilesEx(TObjectList *PanelItems, OPERATION_MODES OpMode)
{
  if (Connected())
  {
    FFileList.reset(CreateFileList(PanelItems, osRemote));
    FPanelItems = PanelItems;
    SCOPE_EXIT
    {
      FPanelItems = nullptr;
      FFileList.reset();
    };
    UnicodeString Query;
    bool Recycle = GetSessionData()->GetDeleteToRecycleBin() &&
      !FTerminal->IsRecycledFile(FFileList->GetString(0));  //-V522
    if (PanelItems->GetCount() > 1)
    {
      Query = FORMAT(GetMsg(Recycle ? NB_RECYCLE_FILES_CONFIRM : NB_DELETE_FILES_CONFIRM),
          PanelItems->GetCount());
    }
    else
    {
      Query = FORMAT(GetMsg(Recycle ? NB_RECYCLE_FILE_CONFIRM : NB_DELETE_FILE_CONFIRM),
          PanelItems->GetAs<TFarPanelItem>(0)->GetFileName());
    }

    if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
      (MoreMessageDialog(Query, nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      FTerminal->RemoteDeleteFiles(FFileList.get());
    }
    return true;
  }
  else if (IsSessionList())
  {
    if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
      (MoreMessageDialog(GetMsg(NB_DELETE_SESSIONS_CONFIRM), nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
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

void TWinSCPFileSystem::QueueAddItem(TQueueItem *Item)
{
  GetFarConfiguration()->CacheFarSettings();
  GetQueue()->AddItem(Item);
}

struct TExportSessionParam
{
  UnicodeString DestPath;
};

int32_t TWinSCPFileSystem::GetFilesEx(TObjectList *PanelItems, bool Move,
  UnicodeString &DestPath, OPERATION_MODES OpMode)
{
  int32_t Result = -1;
  if (Connected())
  {
    FFileList.reset(CreateFileList(PanelItems, osRemote));
    SCOPE_EXIT
    {
      FPanelItems = nullptr;
      FFileList.reset();
    };
    Result = GetFilesRemote(PanelItems, Move, DestPath, OpMode);
  }
  else if (IsSessionList())
  {
    UnicodeString Title = GetMsg(NB_EXPORT_SESSION_TITLE);
    UnicodeString Prompt;
    if (PanelItems->GetCount() == 1)
    {
      Prompt = FORMAT(GetMsg(NB_EXPORT_SESSION_PROMPT),
          PanelItems->GetAs<TFarPanelItem>(0)->GetFileName());
    }
    else
    {
      Prompt = FORMAT(GetMsg(NB_EXPORT_SESSIONS_PROMPT), PanelItems->GetCount());
    }

    bool AResult = (OpMode & OPM_SILENT) ||
      GetWinSCPPlugin()->InputBox(Title, Prompt, DestPath, 0, "Copy");
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

int32_t TWinSCPFileSystem::GetFilesRemote(TObjectList *PanelItems, bool Move,
  UnicodeString &DestPath, OPERATION_MODES OpMode)
{
  int32_t Result = -1;
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
  int32_t Params =
    FLAGMASK(Move, cpDelete);

  if (!Confirmed)
  {
    int32_t CopyParamAttrs =
      GetTerminal()->UsableCopyParamAttrs(Params).Download;
    ; // FLAGMASK(EditView, cpaNoExcludeMask);

    uint32_t Options =
      FLAGMASK(EditView, coTempTransfer | coDisableNewerOnly);
    Confirmed = CopyDialog(false, Move, FFileList.get(),
        Options, CopyParamAttrs,
        DestPath, &CopyParam);

    if (Confirmed && !EditView && CopyParam.GetQueue())
    {
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(CopyParam.GetQueueNoConfirmation(), cpNoConfirmation) |
        FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly);
      QueueAddItem(new TDownloadQueueItem(FTerminal, FFileList.get(),
          DestPath, &CopyParam, Params, false, false));
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
    FTerminal->CopyToLocal(FFileList.get(), DestPath, &CopyParam, Params, nullptr);
    Result = 1;
  }
  return Result;
}

TTerminalQueue *TWinSCPFileSystem::GetQueue()
{
  if (FQueue == nullptr)
  {
    FQueue = new TTerminalQueue(FTerminal, GetConfiguration());
    FQueue->InitTerminalQueue();
    FQueue->SetTransfersLimit(GetGUIConfiguration()->QueueTransfersLimit());
    FQueue->SetOnQueryUser(nb::bind(&TWinSCPFileSystem::TerminalQueryUser, this));
    FQueue->SetOnPromptUser(nb::bind(&TWinSCPFileSystem::TerminalPromptUser, this));
    FQueue->SetOnShowExtendedException(nb::bind(&TWinSCPFileSystem::TerminalShowExtendedException, this));
    FQueue->SetOnListUpdate(nb::bind(&TWinSCPFileSystem::QueueListUpdate, this));
    FQueue->SetOnQueueItemUpdate(nb::bind(&TWinSCPFileSystem::QueueItemUpdate, this));
    FQueue->SetOnEvent(nb::bind(&TWinSCPFileSystem::QueueEvent, this));
  }
  return FQueue;
}

TTerminalQueueStatus *TWinSCPFileSystem::GetQueueStatus()
{
  if (FQueueStatus == nullptr)
  {
    TTerminalQueueStatus *Current = nullptr;
    FQueueStatus = GetQueue()->CreateStatus(Current);
  }
  return FQueueStatus;
}

void TWinSCPFileSystem::ExportSession(TSessionData *Data, void *AParam)
{
  TExportSessionParam &Param = *static_cast<TExportSessionParam *>(AParam);

  std::unique_ptr<TSessionData> ExportData(std::make_unique<TSessionData>(Data->GetName()));
  std::unique_ptr<TSessionData> FactoryDefaults(std::make_unique<TSessionData>(""));
  ExportData->Assign(Data);
  ExportData->SetModified(true);
  UnicodeString XmlFileName = ::IncludeTrailingBackslash(Param.DestPath) +
    ::ValidLocalFileName(::ExtractFilename(ExportData->GetName())) + ".netbox";
  std::unique_ptr<THierarchicalStorage> ExportStorage(std::make_unique<TXmlStorage>(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));
  ExportStorage->Init();
  ExportStorage->SetAccessMode(smReadWrite);
  if (ExportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), true))
  {
    ExportData->Save(ExportStorage.get(), false, FactoryDefaults.get());
  }
}

int32_t TWinSCPFileSystem::UploadFiles(bool Move, OPERATION_MODES OpMode, bool Edit,
  UnicodeString &DestPath)
{
  int32_t Result = 1;
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
  int32_t Params =
    FLAGMASK(Move, cpDelete);

  if (!Confirmed)
  {
    int32_t CopyParamAttrs =
      GetTerminal()->UsableCopyParamAttrs(Params).Upload |
      FLAGMASK(Edit, cpaNoClearArchive);
    // heuristics: do not ask for target directory when uploaded file
    // was downloaded in edit mode
    uint32_t Options =
      FLAGMASK(Edit, coTempTransfer) |
      FLAGMASK(Edit || !GetTerminal()->GetIsCapable(fcNewerOnlyUpload), coDisableNewerOnly);
    Confirmed = CopyDialog(true, Move, FFileList.get(),
        Options, CopyParamAttrs,
        DestPath, &CopyParam);

    if (Confirmed && !Edit && CopyParam.GetQueue())
    {
      // these parameters are known only after transfer dialog
      Params |=
        FLAGMASK(CopyParam.GetQueueNoConfirmation(), cpNoConfirmation) |
        FLAGMASK(CopyParam.GetNewerOnly(), cpNewerOnly);
      QueueAddItem(new TUploadQueueItem(FTerminal, FFileList.get(),
          DestPath, &CopyParam, Params, false, false));
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
      FTerminal->CopyToRemote(FFileList.get(), DestPath, &CopyParam, Params, nullptr);
    }
  }
  else
  {
    Result = -1;
  }
  return Result;
}

int32_t TWinSCPFileSystem::PutFilesEx(TObjectList *PanelItems, bool Move, OPERATION_MODES OpMode)
{
  int32_t Result;
  if (Connected())
  {
    FFileList.reset(CreateFileList(PanelItems, osLocal));
    SCOPE_EXIT
    {
      FPanelItems = nullptr;
    };
    FPanelItems = PanelItems;

    // if file is saved under different name, FAR tries to upload original file,
    // but let's be robust and check for new name, in case it changes.
    // OPM_EDIT is set since 1.70 final, only.
    // When comparing, beware that one path may be long path and the other short
    // (since 1.70 alpha 6, DestPath in GetFiles is short path,
    // while current path in PutFiles is long path)
    if (FLAGCLEAR(OpMode, OPM_SILENT) && (FFileList->GetCount() == 1) && //-V522
      (IsPathToSameFile(FFileList->GetString(0), FOriginalEditFile) || //-V522
        IsPathToSameFile(FFileList->GetString(0), FLastEditFile)))
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

        UnicodeString CurrentDirectory = FTerminal->RemoteGetCurrentDirectory();
        Result = UploadFiles(Move, OpMode, true, CurrentDirectory);
        FTerminal->TerminalSetCurrentDirectory(CurrentDirectory);
      }
    }
    else
    {
      UnicodeString CurrentDirectory = FTerminal->RemoteGetCurrentDirectory();
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
  UpdatePanel();
  return Result;
}

bool TWinSCPFileSystem::ImportSessions(TObjectList *PanelItems, bool /*Move*/,
  OPERATION_MODES OpMode)
{
  bool Result = (OpMode & OPM_SILENT) ||
    (MoreMessageDialog(GetMsg(NB_IMPORT_SESSIONS_PROMPT), nullptr,
        qtConfirmation, qaYes | qaNo) == qaYes);

  if (Result)
  {
    UnicodeString FileName;
    for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
    {
      TFarPanelItem *PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
      bool AnyData = false;
      FileName = PanelItem->GetFileName();
      if (PanelItem->GetIsFile())
      {
        UnicodeString XmlFileName = ::IncludeTrailingBackslash(::GetCurrentDir()) + FileName;
        std::unique_ptr<THierarchicalStorage> ImportStorage(std::make_unique<TXmlStorage>(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));
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
        throw Exception(FORMAT(GetMsg(NB_IMPORT_SESSIONS_EMPTY), FileName));
      }
    }
  }
  return Result;
}

TStrings *TWinSCPFileSystem::CreateFocusedFileList(TOperationSide Side, TFarPanelInfo **APanelInfo)
{
  if (!APanelInfo || !*APanelInfo)
  {
    APanelInfo = this->GetPanelInfo();
  }

  TStrings *Result = nullptr;
  const TFarPanelItem *Focused = APanelInfo && *APanelInfo ? (*APanelInfo)->GetFocusedItem() : nullptr;
  if (Focused && !Focused->GetIsParentDirectory())
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

TStrings *TWinSCPFileSystem::CreateSelectedFileList(TOperationSide Side, TFarPanelInfo **APanelInfo)
{
  TFarPanelInfo **PanelInfo = APanelInfo;
  if (PanelInfo == nullptr)
  {
    PanelInfo = this->GetPanelInfo();
  }

  TStrings *Result;
  if (PanelInfo && *PanelInfo && (*PanelInfo)->GetSelectedCount() > 0)
  {
    UnicodeString CurrDirectory = Connected() ? FTerminal->RemoteGetCurrentDirectory() : (*PanelInfo)->GetCurrDirectory();
    Result = CreateFileList((*PanelInfo)->GetItems(), Side, true, CurrDirectory);
  }
  else
  {
    Result = CreateFocusedFileList(Side, PanelInfo);
  }
  return Result;
}

TStrings *TWinSCPFileSystem::CreateFileList(TObjectList *PanelItems,
  TOperationSide Side, bool SelectedOnly, const UnicodeString Directory, bool FileNameOnly,
  TStrings *AFileList)
{
  std::unique_ptr<TStrings> FileList(AFileList == nullptr ? new TStringList() : AFileList);
  FileList->SetDuplicates(dupAccept);

  TFarPanelItem *PanelItem;
  TObject *Data = nullptr;
  for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
  {
    PanelItem = PanelItems->GetAs<TFarPanelItem>(Index);
    DebugAssert(PanelItem);
    if ((!SelectedOnly || PanelItem->GetSelected()) &&
      !PanelItem->GetIsParentDirectory())
    {
      UnicodeString FileName = PanelItem->GetFileName();
      if (Side == osRemote)
      {
        Data = cast_to<TRemoteFile>(PanelItem->GetUserData());
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
    GetSessionData()->SetRemoteDirectory(FTerminal->RemoteGetCurrentDirectory());

    TSessionData *Data = dyn_cast<TSessionData>(StoredSessions->FindByName(GetSessionData()->GetName()));
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

bool TWinSCPFileSystem::Connect(TSessionData *Data)
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
      throw Exception(FORMAT(GetMsg(NB_CANNOT_INIT_SESSION), Data->GetSessionName()));
    }
  }
  catch (Exception &E)
  {
    EFatal *Fatal = dyn_cast<EFatal>(&E);
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

void TWinSCPFileSystem::ConnectTerminal(TTerminal *Terminal)
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
  TTerminal *Terminal, const UnicodeString Msg)
{
  DebugAssert(FAuthenticationLog != nullptr);
  if (!FAuthenticationLog)
    return;
  FAuthenticationLog->Add(Msg);
  std::unique_ptr<TStringList> AuthenticationLogLines(std::make_unique<TStringList>());
  int32_t Width = 42;
  int32_t Height = 11;
  FarWrapText(::TrimRight(FAuthenticationLog->GetText()), AuthenticationLogLines.get(), Width);
  int32_t Count;
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
  TTerminal *Terminal, UnicodeString AStr, bool /*Status*/, int32_t Phase, const UnicodeString & Additional)
{
  if (Phase != 0)
  {
    TSessionStatus sts = GetTerminal() ? GetTerminal()->GetStatus() : ssClosed;
    if (sts == ssOpening || sts == ssOpened)
    {
      bool new_log = FAuthenticationLog == nullptr;
      if (new_log)
      {
        FAuthenticationLog = std::make_unique<TStringList>();
        GetWinSCPPlugin()->SaveScreen(FAuthenticationSaveScreenHandle);
        GetWinSCPPlugin()->ShowConsoleTitle(GetTerminal()->GetSessionData()->GetSessionName());
      }

      LogAuthentication(Terminal, AStr);
      if (new_log && sts == ssOpened) goto do_restore;
    }
  }
  else
  {
    if (FAuthenticationLog != nullptr)
    {
do_restore:
      GetWinSCPPlugin()->ClearConsoleTitle();
      GetWinSCPPlugin()->RestoreScreen(FAuthenticationSaveScreenHandle);
      FAuthenticationLog.reset();
    }
  }
}

void TWinSCPFileSystem::TerminalChangeDirectory(TObject * /*Sender*/)
{
  if (!FNoProgress)
  {
    UnicodeString Directory = FTerminal->RemoteGetCurrentDirectory();
    int32_t Index = FPathHistory->IndexOf(Directory);
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
    GetWinSCPPlugin()->ShowConsoleTitle(GetMsg(NB_READING_DIRECTORY_TITLE));
  }
}

void TWinSCPFileSystem::TerminalReadDirectoryProgress(
  TObject * /*Sender*/, int32_t Progress, int32_t /*ResolvedLinks*/, bool &Cancel)
{
  if (Progress < 0)
  {
    if (!FNoProgress && (Progress == -2))
    {
      MoreMessageDialog(GetMsg(NB_DIRECTORY_READING_CANCELLED), nullptr,
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
        FORMAT("%s (%d)", GetMsg(NB_READING_DIRECTORY_TITLE), Progress));
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

void TWinSCPFileSystem::TerminalDeleteLocalFile(UnicodeString AFileName, bool Alternative, int32_t& Deleted)
{
  bool ToRecycleBin = FLAGSET(GetWinSCPPlugin()->GetFarSystemSettings(), NBSS_DELETETORECYCLEBIN) != Alternative;
  if (ToRecycleBin || !GetWinSCPPlugin()->GetSystemFunctions())
  {
    if (!RecursiveDeleteFile(AFileName, ToRecycleBin))
    {
      throw Exception(FORMAT(GetMsg(NB_DELETE_LOCAL_FILE_ERROR), AFileName));
    }
  }
  else
  {
    GetWinSCPPlugin()->DeleteLocalFile(AFileName);
  }
}

HANDLE TWinSCPFileSystem::TerminalCreateLocalFile(const UnicodeString ALocalFileName,
  DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::CreateFile(ApiPath(ALocalFileName).c_str(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, nullptr);
  }
  else
  {
    return GetWinSCPPlugin()->CreateLocalFile(ALocalFileName, DesiredAccess,
        ShareMode, CreationDisposition, FlagsAndAttributes);
  }
}

DWORD TWinSCPFileSystem::TerminalGetLocalFileAttributes(const UnicodeString ALocalFileName) const
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::FileGetAttrFix(ALocalFileName);
  }
  else
  {
    return GetWinSCPPlugin()->GetLocalFileAttributes(ALocalFileName);
  }
}

bool TWinSCPFileSystem::TerminalSetLocalFileAttributes(const UnicodeString ALocalFileName, DWORD FileAttributes)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::SetFileAttributesW(ApiPath(ALocalFileName).c_str(), FileAttributes) != FALSE;
  }
  else
  {
    return GetWinSCPPlugin()->SetLocalFileAttributes(ALocalFileName, FileAttributes);
  }
}

bool TWinSCPFileSystem::TerminalMoveLocalFile(const UnicodeString ALocalFileName, const UnicodeString ANewLocalFileName, DWORD Flags)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::MoveFileExW(ApiPath(ALocalFileName).c_str(), ApiPath(ANewLocalFileName).c_str(), Flags) != 0;
  }
  else
  {
    return GetWinSCPPlugin()->MoveLocalFile(ALocalFileName, ANewLocalFileName, Flags);
  }
}

bool TWinSCPFileSystem::TerminalRemoveLocalDirectory(const UnicodeString ALocalDirName)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::RemoveDirectory(ApiPath(ALocalDirName).c_str()) != 0;
  }
  else
  {
    return GetWinSCPPlugin()->RemoveLocalDirectory(ALocalDirName);
  }
}
bool TWinSCPFileSystem::TerminalCreateLocalDirectory(const UnicodeString ALocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  if (!GetWinSCPPlugin()->GetSystemFunctions())
  {
    return ::CreateDirectory(ApiPath(ALocalDirName).c_str(), SecurityAttributes) != 0;
  }
  else
  {
    return GetWinSCPPlugin()->CreateLocalDirectory(ALocalDirName, SecurityAttributes);
  }
}

uint32_t TWinSCPFileSystem::MoreMessageDialog(const UnicodeString Str,
  TStrings *MoreMessages, TQueryType Type, uint32_t Answers, const TMessageParams *AParams)
{
  TMessageParams Params(nullptr);

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
  const UnicodeString AQuery, TStrings *MoreMessages, uint32_t Answers,
  const TQueryParams *AParams, uint32_t &Answer, TQueryType Type, void * /*Arg*/)
{
  TMessageParams Params(nullptr);
  UnicodeString Query = AQuery;

  if (AParams != nullptr)
  {
    if (AParams->Params & qpFatalAbort)
    {
      Query = FORMAT(GetMsg(NB_WARN_FATAL_ERROR), Query);
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

void TWinSCPFileSystem::TerminalPromptUser(TTerminal *Terminal,
  TPromptKind Kind, const UnicodeString AName, const UnicodeString AInstructions,
  TStrings *Prompts, TStrings *Results, bool &AResult,
  void * /*Arg*/)
{
  if (Kind == pkPrompt)
  {
    DebugAssert(AInstructions.IsEmpty());
    DebugAssert(Prompts->GetCount() == 1);
    DebugAssert(Prompts->GetObj(0) != nullptr);
    UnicodeString Result = Results->GetString(0);

    AResult = GetWinSCPPlugin()->InputBox(AName, ::StripHotkey(Prompts->GetString(0)), Result, FIB_NOUSELASTHISTORY);
    if (AResult)
    {
      Results->SetString(0, Result);
    }
  }
  else
  {
    AResult = PasswordDialog(Terminal->GetSessionData(), Kind, AName, AInstructions,
        Prompts, Results, GetTerminal()->GetStoredCredentialsTried());
  }
}

void TWinSCPFileSystem::TerminalDisplayBanner(
  TTerminal * /*Terminal*/, const UnicodeString ASessionName,
  const UnicodeString ABanner, bool &NeverShowAgain, int32_t Options, uint32_t & /*Params*/)
{
  BannerDialog(ASessionName, ABanner, NeverShowAgain, Options);
}

void TWinSCPFileSystem::TerminalShowExtendedException(
  TTerminal * /*Terminal*/, Exception *E, void * /*Arg*/)
{
  GetWinSCPPlugin()->ShowExtendedException(E);
}

void TWinSCPFileSystem::OperationProgress(
  TFileOperationProgressType &ProgressData)
{
  if (FNoProgress)
  {
    return;
  }

  bool First = false;
  if (ProgressData.GetInProgress() && !FProgressSaveScreenHandle)
  {
    GetWinSCPPlugin()->SaveScreen(FProgressSaveScreenHandle);
    First = true;
  }

  // operation is finished (or terminated), so we hide progress form
  if (!ProgressData.GetInProgress() && FProgressSaveScreenHandle)
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
  TOperationSide Side, bool /*Temp*/, const UnicodeString AFileName, bool Success,
  TOnceDoneOperation & /*DisconnectWhenComplete*/)
{
  DebugUsedParam(Side);

  if ((Operation != foCalculateSize) &&
    (Operation != foGetProperties) &&
    (Operation != foCalculateChecksum) &&
    (FSynchronizationSaveScreenHandle == nullptr) &&
    !FNoProgress && !FNoProgressFinish)
  {
    TFarPanelItem *PanelItem = nullptr;

    if (!FPanelItems)
    {
      TObjectList *PanelItems = (*GetPanelInfo())->GetItems();
      for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
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
      int32_t Index = FFileList->IndexOf(AFileName);
      DebugAssert(Index >= 0);
      PanelItem = cast_to<TFarPanelItem>(FPanelItems->GetItem(Index));
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
  TFileOperationProgressType &ProgressData, bool Force)
{
  static uint32_t LastTicks;
  uint32_t Ticks = ::GetTickCount();
  short percents = static_cast<short>(ProgressData.OverallProgress());
  if (Ticks - LastTicks > 500 || Force)
  {
    LastTicks = Ticks;

    static const int32_t ProgressWidth = 48;
    static const int Captions[] = {NB_PROGRESS_COPY, NB_PROGRESS_MOVE, NB_PROGRESS_DELETE,
        NB_PROGRESS_SETPROPERTIES, 0, 0, NB_PROGRESS_CALCULATE_SIZE,
        NB_PROGRESS_REMOTE_MOVE, NB_PROGRESS_REMOTE_COPY, NB_PROGRESS_GETPROPERTIES,
        NB_PROGRESS_CALCULATE_CHECKSUM
      };
    static UnicodeString ProgressFileLabel;
    static UnicodeString TargetDirLabel;
    static UnicodeString StartTimeLabel;
    static UnicodeString TimeElapsedLabel;
    static UnicodeString BytesTransferredLabel;
    static UnicodeString CPSLabel;
    static UnicodeString TimeLeftLabel;

    if (ProgressFileLabel.IsEmpty())
    {
      ProgressFileLabel = GetMsg(NB_PROGRESS_FILE_LABEL);
      TargetDirLabel = GetMsg(NB_TARGET_DIR_LABEL);
      StartTimeLabel = GetMsg(NB_START_TIME_LABEL);
      TimeElapsedLabel = GetMsg(NB_TIME_ELAPSED_LABEL);
      BytesTransferredLabel = GetMsg(NB_BYTES_TRANSFERED_LABEL);
      CPSLabel = GetMsg(NB_CPS_LABEL);
      TimeLeftLabel = GetMsg(NB_TIME_LEFT_LABEL);
    }

    bool TransferOperation =
      ((ProgressData.GetOperation() == foCopy) || (ProgressData.GetOperation() == foMove));

    UnicodeString ProgressBarCurrentFile;
    UnicodeString Message2;
    UnicodeString Title = GetMsg(Captions[nb::ToInt(ProgressData.GetOperation() - 1)]);
    UnicodeString FileName = ProgressData.GetFileName();
    // for upload from temporary directory,
    // do not show source directory
    if (TransferOperation && (ProgressData.GetSide() == osLocal) && ProgressData.GetTemp())
    {
      FileName = base::ExtractFileName(FileName, false);
    }
    UnicodeString Message1 = ProgressFileLabel + base::MinimizeName(FileName,
        ProgressWidth - ProgressFileLabel.Length(), ProgressData.GetSide() == osRemote) + L"\n";
    // for downloads to temporary directory,
    // do not show target directory
    if (TransferOperation && !((ProgressData.GetSide() == osRemote) && ProgressData.GetTemp()))
    {
      Message1 += TargetDirLabel + base::MinimizeName(ProgressData.GetDirectory(),
          ProgressWidth - TargetDirLabel.Length(), ProgressData.GetSide() == osLocal) + L"\n";
    }
    UnicodeString ProgressBarTotal = ProgressBar(ProgressData.OverallProgress(), ProgressWidth) + L"\n";
    if (TransferOperation)
    {
      Message2 = L"\1\n";

      UnicodeString Value = FormatDateTimeSpan(GetConfiguration()->TimeFormat(), ProgressData.TimeElapsed());
      UnicodeString StatusLine = TimeElapsedLabel +
        StringOfChar(L' ', ProgressWidth / 2 - 1 - TimeElapsedLabel.Length() - Value.Length()) +
        Value + L"  ";

      UnicodeString LabelText;
      if (ProgressData.GetTotalSizeSet())
      {
        Value = FormatDateTimeSpan(GetConfiguration()->TimeFormat(), ProgressData.TotalTimeLeft());
        LabelText = TimeLeftLabel;
      }
      else
      {
        Value = ProgressData.GetStartTime().GetTimeString(true);
        LabelText = StartTimeLabel;
      }
      StatusLine = StatusLine + LabelText +
        StringOfChar(' ', ProgressWidth - StatusLine.Length() -
          LabelText.Length() - Value.Length()) + Value;
      Message2 += StatusLine + L"\n";

      Value = base::FormatBytes(ProgressData.GetTotalTransferred());
      StatusLine = BytesTransferredLabel +
        StringOfChar(' ', ProgressWidth / 2 - 1 - BytesTransferredLabel.Length() - Value.Length()) +
        Value + L"  ";
      Value = FORMAT("%s/s", base::FormatBytes(ProgressData.CPS()));
      StatusLine = StatusLine + CPSLabel +
        StringOfChar(' ', ProgressWidth - StatusLine.Length() -
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

UnicodeString TWinSCPFileSystem::ProgressBar(int32_t Percentage, int32_t Width)
{
  // 0xB0 - 0x2591
  // 0xDB - 0x2588
  UnicodeString Result = ::StringOfChar(0x2588, (Width - 5) * (Percentage > 100 ? 100 : Percentage) / 100);
  Result += ::StringOfChar(0x2591, (Width - 5) - Result.Length());
  Result += FORMAT("%4d%%", Percentage > 100 ? 100 : Percentage);
  return Result;
}

TTerminalQueueStatus *TWinSCPFileSystem::ProcessQueue(bool Hidden)
{
  TTerminalQueueStatus *Result = nullptr;
  if (FQueue == nullptr)
    return Result;

  TTerminalQueueStatus *QueueStatus = GetQueueStatus();
  DebugAssert(QueueStatus != nullptr);
  FarPlugin->UpdateProgress(QueueStatus->GetCount() > 0 ? TBPS_INDETERMINATE : TBPS_NOPROGRESS, 0);

  if (FQueueStatusInvalidated || FQueueItemInvalidated)
  {
    if (FQueueStatusInvalidated)
    {
      TGuard Guard(FQueueStatusSection); nb::used(Guard);

      FQueueStatusInvalidated = false;

      if (FQueue != nullptr)
        FQueueStatus = FQueue->CreateStatus(FQueueStatus);
      Result = FQueueStatus;
    }

    FQueueItemInvalidated = false;

    for (int32_t Index = 0; Index < FQueueStatus->GetActiveCount(); ++Index)
    {
      TQueueItemProxy *QueueItem = FQueueStatus->GetItem(Index);
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
      TGuard Guard(FQueueStatusSection); nb::used(Guard);
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

void TWinSCPFileSystem::QueueListUpdate(TTerminalQueue *Queue)
{
  if (GetQueue() == Queue)
  {
    FQueueStatusInvalidated = true;
  }
}

void TWinSCPFileSystem::QueueItemUpdate(TTerminalQueue *Queue,
  TQueueItem *Item)
{
  if (GetQueue() == Queue)
  {
    TGuard Guard(FQueueStatusSection); nb::used(Guard);

    TTerminalQueueStatus *QueueStatus = GetQueueStatus();
    DebugAssert(QueueStatus != nullptr);

    TQueueItemProxy *QueueItem = QueueStatus->FindByQueueItem(Item);

    if ((Item->GetStatus() == TQueueItem::qsDone) && (GetTerminal() != nullptr))
    {
      FRefreshLocalDirectory = (QueueItem == nullptr) ||
        (!QueueItem->GetInfo()->ModifiedLocal.IsEmpty());
      FRefreshRemoteDirectory = (QueueItem == nullptr) ||
        (!QueueItem->GetInfo()->ModifiedRemote.IsEmpty());
    }

    if (QueueItem != nullptr)
    {
      QueueItem->SetUserData(nb::ToPtr(1LL));
      FQueueItemInvalidated = true;
    }
  }
}

void TWinSCPFileSystem::QueueEvent(TTerminalQueue *Queue,
  TQueueEvent Event)
{
  TGuard Guard(FQueueStatusSection); nb::used(Guard);
  if (Queue == GetQueue())
  {
    FQueueEventPending = true;
    FQueueEvent = Event;
  }
}

void TWinSCPFileSystem::CancelConfiguration(TFileOperationProgressType &ProgressData)
{
  if (!ProgressData.GetSuspended())
  {
    ProgressData.Suspend();
    SCOPE_EXIT
    {
      ProgressData.Resume();
    };
    TCancelStatus ACancel;
    uintptr_t Result;
    if (ProgressData.GetTransferringFile() &&
      (ProgressData.TimeExpected() > GetGUIConfiguration()->GetIgnoreCancelBeforeFinish()))
    {
      Result = MoreMessageDialog(GetMsg(NB_CANCEL_OPERATION_FATAL2), nullptr,
          qtWarning, qaYes | qaNo | qaCancel);
    }
    else
    {
      Result = MoreMessageDialog(GetMsg(NB_CANCEL_OPERATION), nullptr,
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

    if (ACancel > ProgressData.GetCancel())
    {
      ProgressData.SetCancel(ACancel);
    }
  }
}

void TWinSCPFileSystem::UploadFromEditor(bool NoReload,
  const UnicodeString AFileName, const UnicodeString RealFileName,
  UnicodeString &DestPath)
{
  DebugAssert(FFileList == nullptr);
  FFileList = std::make_unique<TStringList>();
  DebugAssert(FTerminal->GetAutoReadDirectory());
  bool PrevAutoReadDirectory = FTerminal->GetAutoReadDirectory();
  if (NoReload)
  {
    FTerminal->SetAutoReadDirectory(false);
    if (base::UnixSamePath(DestPath, FTerminal->RemoteGetCurrentDirectory()))
    {
      FReloadDirectory = true;
    }
  }

  std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>());
  File->SetFileName(RealFileName);
  SCOPE_EXIT
  {
    FTerminal->SetAutoReadDirectory(PrevAutoReadDirectory);
    FFileList.reset();
  };
  FFileList->AddObject(AFileName, File.get()); //-V522
  UploadFiles(false, 0, true, DestPath);
}

void TWinSCPFileSystem::UploadOnSave(bool NoReload)
{
  std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
  if (Info != nullptr)
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
        UnicodeString CurrentDirectory = FTerminal->RemoteGetCurrentDirectory();
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

void TWinSCPFileSystem::ProcessEditorEvent(int32_t Event, void * /*Param*/)
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
      if (Info != nullptr)
      {
        TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
        if (it != FMultipleEdits.end())
        {
          UnicodeString FullFileName = base::UnixIncludeTrailingBackslash(it->second.Directory) +
            it->second.FileTitle;
          GetWinSCPPlugin()->FarEditorControl(ECTL_SETTITLE,
            FullFileName.Length(),
            nb::ToPtr(ToWChar(FullFileName)));
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
      if (Info != nullptr)
      {
        if (!FLastEditFile.IsEmpty() &&
          ::AnsiSameText(FLastEditFile, Info->GetFileName()))
        {
          FLastEditorID = Info->GetEditorID();
          FEditorPendingSave = false;
        }

        if (!FLastMultipleEditFile.IsEmpty())
        {
          bool IsLastMultipleEditFile = ::AnsiSameText(base::FromUnixPath(FLastMultipleEditFile), base::FromUnixPath(Info->GetFileName()));
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
              nb::ClearStruct(Parameter);
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
    if (Info != nullptr)
    {
      if (FLastEditorID == Info->GetEditorID())
      {
        FLastEditorID = -1;
      }

      TMultipleEdits::iterator it = FMultipleEdits.find(Info->GetEditorID());
      if (it != FMultipleEdits.end())
      {
        TMultipleEdit &ed = it->second;
        if (ed.PendingSave)
        {
          UploadFromEditor(true, Info->GetFileName(), ed.FileTitle, ed.Directory);
          // reload panel content (if uploaded to current directory.
          // no need for RefreshPanel as panel is not visible yet.
          UpdatePanel();
        }

        if (::SysUtulsRemoveFile(Info->GetFileName()))
        {
          // remove directory only if it is empty
          // (to avoid deleting another directory if user uses "save as")
          ::SysUtulsRemoveDir(::ExcludeTrailingBackslash(::ExtractFilePath(Info->GetFileName())));
        }

        FMultipleEdits.erase(it->first);
      }
    }
  }
  else if (Event == EE_SAVE)
  {
    std::unique_ptr<TFarEditorInfo> Info(GetWinSCPPlugin()->EditorInfo());
    if (Info != nullptr)
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
          UnicodeString FullFileName = base::UnixIncludeTrailingBackslash(it->second.Directory) +
            it->second.FileTitle;
          // note that we need to reset the title periodically (see EE_REDRAW)
          GetWinSCPPlugin()->FarEditorControl(ECTL_SETTITLE,
            FullFileName.Length(),
            nb::ToPtr(ToWChar(FullFileName)));
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

void TWinSCPFileSystem::EditViewCopyParam(TCopyParamType &CopyParam)
{
  CopyParam.SetFileNameCase(ncNoChange);
  CopyParam.SetPreserveReadOnly(false);
  CopyParam.SetResumeSupport(rsOff);
  CopyParam.SetReplaceInvalidChars(true);
  CopyParam.SetFileMask(L"");
}

void TWinSCPFileSystem::MultipleEdit()
{
  TFarPanelInfo *const *PanelInfo = GetPanelInfo();
  const TFarPanelItem *Focused = PanelInfo && *PanelInfo ? (*PanelInfo)->GetFocusedItem() : nullptr;
  if ((Focused != nullptr) && Focused->GetIsFile() &&
    (Focused->GetUserData() != nullptr))
  {
    std::unique_ptr<TStrings> FileList(CreateFocusedFileList(osRemote));
    DebugAssert((FileList == nullptr) || (FileList->GetCount() == 1));

    if ((FileList != nullptr) && (FileList->GetCount() == 1))
    {
      MultipleEdit(FTerminal->RemoteGetCurrentDirectory(), FileList->GetString(0),
        FileList->GetAs<TRemoteFile>(0));
    }
  }
}

void TWinSCPFileSystem::MultipleEdit(const UnicodeString Directory,
  UnicodeString AFileName, TRemoteFile *AFile)
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

  UnicodeString FullFileName = base::UnixIncludeTrailingBackslash(Directory) + AFileName;

  std::unique_ptr<TRemoteFile> FileDuplicate(AFile ? AFile->Duplicate() : new TRemoteFile());
  UnicodeString NewFileName = AFileName; // FullFileName;
  FileDuplicate->SetFileName(NewFileName);

  TMultipleEdits::iterator it_e = FMultipleEdits.begin();
  while (it_e != FMultipleEdits.end())
  {
    const TMultipleEdit &ed = it_e->second;
    if (base::UnixSamePath(Directory, ed.Directory) &&
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
    TMessageParams Params(nullptr);
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaYes;
    Aliases[0].Alias = GetMsg(NB_EDITOR_CURRENT);
    Aliases[1].Button = qaNo;
    Aliases[1].Alias = GetMsg(NB_EDITOR_NEW_INSTANCE);
    Aliases[2].Button = qaOK;
    Aliases[2].Alias = GetMsg(NB_EDITOR_NEW_INSTANCE_RO);
    Params.Aliases = Aliases;
    Params.AliasesCount = _countof(Aliases);
    switch (MoreMessageDialog(FORMAT(GetMsg(NB_EDITOR_ALREADY_LOADED), FullFileName),
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
    int32_t Pos = 0;
    while (Pos < WindowCount)
    {
      WindowInfo Window;
      nb::ClearStruct(Window);
      Window.StructSize = sizeof(WindowInfo);
      Window.Pos = Pos;
      UnicodeString EditedFileName(1024, 0);
      Window.Name = ToWChar(EditedFileName);
      Window.NameSize = nb::ToIntPtr(EditedFileName.GetLength());
      if (FarPlugin->FarAdvControl(ACTL_GETWINDOWINFO, 0, &Window) != 0)
      {
        if ((Window.Type == WTYPE_EDITOR) &&
          Window.Name && AnsiSameText(Window.Name, it_e->second.LocalFileName))
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
    TGUICopyParamType &CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
    EditViewCopyParam(CopyParam);

    std::unique_ptr<TStrings> FileList(std::make_unique<TStringList>());
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
  std::unique_ptr<TFarMenuItems> MenuItems(std::make_unique<TFarMenuItems>());
  TEditHistories::const_iterator it = FEditHistories.begin();
  while (it != FEditHistories.end())
  {
    MenuItems->Add(base::MinimizeName(base::UnixIncludeTrailingBackslash(it->Directory) + it->FileName,
        GetWinSCPPlugin()->MaxMenuItemLength(), true));
    ++it;
  }

  MenuItems->Add(L"");
  MenuItems->SetItemFocused(MenuItems->GetCount() - 1);

  const FarKey BreakKeys[] = {{ VK_F4, 0 }, { 0 }};

  intptr_t BreakCode = 0;
  int32_t Result = GetWinSCPPlugin()->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
      GetMsg(NB_MENU_EDIT_HISTORY), L"", MenuItems.get(), BreakKeys, BreakCode);

  if ((Result >= 0) && (Result < nb::ToInt32(FEditHistories.size())))
  {
    TRemoteFile *File = nullptr;
    const TEditHistory &EditHistory = FEditHistories[Result];
    UnicodeString FullFileName =
      base::UnixIncludeTrailingBackslash(EditHistory.Directory) + EditHistory.FileName;
    File = FTerminal->ReadFile(FullFileName);
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
  return Connected() && FTerminal->GetLog()->LogToFile();
}

void TWinSCPFileSystem::ShowLog()
{
  DebugAssert(Connected() && FTerminal->GetLog()->LogToFile());
  const TSessionLog *Log = FTerminal->GetLog();
  GetWinSCPPlugin()->Viewer(Log->GetLogFileName(), Log->GetLogFileName(), VF_NONMODAL);
}

UnicodeString TWinSCPFileSystem::GetFileNameHash(const UnicodeString AFileName) const
{
  RawByteString Result;
  char *Buf = Result.SetLength(16);
  md5checksum(
    reinterpret_cast<const char *>(AFileName.c_str()), nb::ToInt(AFileName.Length() * sizeof(wchar_t)),
    reinterpret_cast<uint8_t *>(Buf));
  return BytesToHex(Result);
}

