
#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop

#include "TerminalManager.h"
#include <Authenticate.h>
#include "CustomScpExplorer.h"
#include "NonVisual.h"
#include "WinConfiguration.h"
#include "Tools.h"
#include <Common.h>
#include <CoreMain.h>
#include <GUITools.h>
#include <TextsWin.h>
#include <TextsCore.h>
#include <Progress.h>
#include <Exceptions.h>
#include <VCLCommon.h>
#include <WinApi.h>
#include <PuttyTools.h>
#include <HelpWin.h>
#include <System.IOUtils.hpp>
#include <StrUtils.hpp>

#pragma package(smart_init)

TTerminalManager * TTerminalManager::FInstance = nullptr;

TManagedTerminal::TManagedTerminal(TSessionData * SessionData,
  TConfiguration * Configuration) :
  TTerminal(SessionData, Configuration),
  LocalBrowser(false), LocalExplorerState(nullptr), RemoteExplorerState(nullptr), OtherLocalExplorerState(nullptr),
  ReopenStart(0), DirectoryLoaded(Now()), TerminalThread(nullptr), Disconnected(false), DisconnectedTemporarily(false)
{
  StateData = new TSessionData(L"");
  StateData->Assign(SessionData);
  StateData->LocalDirectory = StateData->LocalDirectoryExpanded;
}

TManagedTerminal::~TManagedTerminal()
{
  delete StateData;
  delete LocalExplorerState;
  delete OtherLocalExplorerState;
  delete RemoteExplorerState;
}


TTerminalManager * TTerminalManager::Instance(bool ForceCreation)
{
  if (!FInstance && ForceCreation)
  {
    FInstance = new TTerminalManager();
  }
  return FInstance;
}

void TTerminalManager::DestroyInstance()
{
  DebugAssert(FInstance);
  SAFE_DESTROY(FInstance);
}

TTerminalManager::TTerminalManager() :
  TTerminalList(Configuration)
{
  FQueueSection = new TCriticalSection();
  FActiveSession = nullptr;
  FTerminalWithFatalExceptionTimer = nullptr;
  FScpExplorer = nullptr;
  FDestroying = false;
  FTerminalPendingAction = tpNull;
  FDirectoryReadingStart = 0;
  FAuthenticateForm = nullptr;
  FTaskbarList = nullptr;
  FAuthenticating = 0;
  FMainThread = GetCurrentThreadId();
  FChangeSection = std::make_unique<TCriticalSection>();
  FPendingConfigurationChange = 0;
  FKeepAuthenticateForm = false;
  FUpdating = 0;
  FOpeningTerminal = NULL;

  FApplicationsEvents = std::make_unique<TApplicationEvents>(Application);
  FApplicationsEvents->OnException = ApplicationException;
  FApplicationsEvents->OnShowHint = ApplicationShowHint;
  FApplicationsEvents->OnMessage = ApplicationMessage;
  FApplicationsEvents->OnModalBegin = ApplicationModalBegin;
  FApplicationsEvents->OnModalEnd = ApplicationModalEnd;

  DebugAssert(WinConfiguration->OnMasterPasswordPrompt == nullptr);
  WinConfiguration->OnMasterPasswordPrompt = MasterPasswordPrompt;

  InitTaskbarButtonCreatedMessage();

  DebugAssert(Configuration && !Configuration->OnChange);
  Configuration->OnChange = ConfigurationChange;

  FMaxSessions = WinConfiguration->MaxSessions;

  FSessionList = new TStringList();
  FQueues = new TList();
  FTerminationMessages = new TStringList();
  std::unique_ptr<TSessionData> DummyData(std::make_unique<TSessionData>(L""));
  FLocalTerminal = CreateTerminal(DummyData.get());
  SetupTerminal(FLocalTerminal);
}

TTerminalManager::~TTerminalManager()
{
  FreeAll();

  DebugAssert(!ScpExplorer);

  DebugAssert(Configuration->OnChange == ConfigurationChange);
  Configuration->OnChange = nullptr;

  FApplicationsEvents.reset(nullptr);

  DebugAssert(WinConfiguration->OnMasterPasswordPrompt == MasterPasswordPrompt);
  WinConfiguration->OnMasterPasswordPrompt = nullptr;

  delete FLocalTerminal;
  delete FQueues;
  delete FTerminationMessages;
  delete FSessionList;
  CloseAuthenticateForm();
  delete FQueueSection;
  ReleaseTaskbarList();
}

void TTerminalManager::SetQueueConfiguration(TTerminalQueue * Queue)
{
  Queue->TransfersLimit = Configuration->QueueTransfersLimit;
  Queue->KeepDoneItemsFor =
    (GUIConfiguration->QueueKeepDoneItems ? GUIConfiguration->QueueKeepDoneItemsFor : 0);
}

TTerminalQueue * TTerminalManager::NewQueue(TTerminal * Terminal)
{
  TTerminalQueue * Queue = new TTerminalQueue(Terminal, Configuration);
  SetQueueConfiguration(Queue);
  Queue->Enabled = WinConfiguration->EnableQueueByDefault;
  Queue->OnQueryUser = TerminalQueryUser;
  Queue->OnPromptUser = TerminalPromptUser;
  Queue->OnShowExtendedException = TerminalShowExtendedException;
  Queue->OnEvent = QueueEvent;
  return Queue;
}

TManagedTerminal * TTerminalManager::CreateManagedTerminal(TSessionData * Data)
{
  TManagedTerminal * Result = new TManagedTerminal(Data, Configuration);
  Result->LocalBrowser = Data->IsLocalBrowser;
  return Result;
}

TTerminal * TTerminalManager::CreateTerminal(TSessionData * Data)
{
  return CreateManagedTerminal(Data);
}

TManagedTerminal * TTerminalManager::GetSession(int32_t Index)
{
  return DebugNotNull(dynamic_cast<TManagedTerminal *>(TTerminalList::Terminals[Index]));
}

void TTerminalManager::SetupTerminal(TTerminal * Terminal)
{
  Terminal->OnQueryUser = TerminalQueryUser;
  Terminal->OnPromptUser = TerminalPromptUser;
  Terminal->OnDisplayBanner = TerminalDisplayBanner;
  Terminal->OnShowExtendedException = TerminalShowExtendedException;
  Terminal->OnProgress = OperationProgress;
  Terminal->OnFinished = OperationFinished;
  Terminal->OnDeleteLocalFile = DeleteLocalFile;
  Terminal->OnReadDirectoryProgress = TerminalReadDirectoryProgress;
  Terminal->OnInformation = TerminalInformation;
  Terminal->OnCustomCommand = TerminalCustomCommand;
}

TManagedTerminal * TTerminalManager::DoNewSession(TSessionData * Data)
{
  if (Count >= FMaxSessions)
  {
    UnicodeString Msg = FMTLOAD(TOO_MANY_SESSIONS, (Count));
    if (MessageDialog(Msg, qtConfirmation, qaOK | qaCancel, HELP_TOO_MANY_SESSIONS) == qaCancel)
    {
      Abort();
    }
    FMaxSessions = FMaxSessions * 3 / 2; // increase limit before the next warning by 50%
  }
  TManagedTerminal * Session = DebugNotNull(dynamic_cast<TManagedTerminal *>(TTerminalList::NewTerminal(Data)));
  try
  {
    FQueues->Add(NewQueue(Session));
    FTerminationMessages->Add(L"");

    SetupTerminal(Session);
  }
  catch(...)
  {
    if (Session != nullptr)
    {
      FreeTerminal(Session);
    }
    throw;
  }

  return Session;
}

TTerminal * TTerminalManager::NewTerminal(TSessionData * Data)
{
  TTerminal * Terminal = DoNewSession(Data);
  DoSessionListChanged();
  return Terminal;
}

TManagedTerminal * TTerminalManager::NewLocalBrowser(const UnicodeString & LocalDirectory, const UnicodeString & OtherLocalDirectory)
{
  std::unique_ptr<TSessionData> SessionData(new TSessionData(UnicodeString()));
  SessionData->LocalDirectory = LocalDirectory;
  SessionData->OtherLocalDirectory = OtherLocalDirectory;
  TManagedTerminal * Result = NewManagedTerminal(SessionData.get());
  // Is true already, when LocalDirectory and OtherLocalDirectory are set
  Result->LocalBrowser = true;
  return Result;
}

void TTerminalManager::NewLocalSession(const UnicodeString & LocalDirectory, const UnicodeString & OtherLocalDirectory)
{
  ActiveSession = NewLocalBrowser(LocalDirectory, OtherLocalDirectory);
}

TManagedTerminal * TTerminalManager::NewManagedTerminal(TSessionData * Data)
{
  return DebugNotNull(dynamic_cast<TManagedTerminal *>(NewTerminal(Data)));
}

bool TTerminalManager::SupportedSession(TSessionData * Data)
{
  bool Result;
  // When main window exists already, ask it if it supports the session
  // (we cannot decide based on configuration,
  // as the user might have changed the interface in the preferences after the main window was created)
  // If not, assume based on configuration.
  if (ScpExplorer != nullptr)
  {
    Result = ScpExplorer->SupportedSession(Data);
  }
  else
  {
    Result =
      (WinConfiguration->Interface != ifExplorer) ||
      !Data->IsLocalBrowser;
  }
  return Result;
}

TManagedTerminal * TTerminalManager::NewSessions(TList * DataList)
{
  TManagedTerminal * Result = nullptr;
  for (int Index = 0; Index < DataList->Count; Index++)
  {
    TSessionData * Data = reinterpret_cast<TSessionData *>(DataList->Items[Index]);
    if (SupportedSession(Data))
    {
      TManagedTerminal * Session = DoNewSession(Data);
      // When opening workspace/folder, keep the sessions open, even if they fail to connect.
      // We cannot detect a folder here, so we "guess" it by a session set size.
      // After all, no one will have a folder with a one session only (while a workspace with one session is likely).
      // And when when opening a folder with a one session only, it's not that big problem, if we treat it the same way
      // as when opening the session only.
      // Also closing a workspace session will remove the session from the workspace.
      // While closing a folder session won't remove the session from the folder.
      Session->Permanent = Data->IsWorkspace || (DataList->Count > 1);
      if (Result == nullptr)
      {
        Result = Session;
      }
    }
  }
  DoSessionListChanged();
  return Result;
}

void TTerminalManager::FreeActiveTerminal()
{
  if (FTerminalPendingAction == tpNull)
  {
    DebugAssert(ActiveSession != nullptr);
    FreeTerminal(ActiveSession);
  }
  else
  {
    DebugAssert(FTerminalPendingAction == ::tpNone);
    FTerminalPendingAction = tpFree;
  }
}

void TTerminalManager::DoConnectTerminal(TTerminal * Terminal, bool Reopen, bool AdHoc)
{
  TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Terminal);
  // it must be managed terminal, unless it is secondary terminal (of managed terminal)
  DebugAssert((ManagedTerminal != nullptr) || (dynamic_cast<TSecondaryTerminal *>(Terminal) != nullptr));

  // particularly when we are reconnecting RemoteDirectory of managed terminal
  // hold the last used remote directory as opposite to session data, which holds
  // the default remote directory.
  // make sure the last used directory is used, but the default is preserved too
  UnicodeString OrigRemoteDirectory = Terminal->SessionData->RemoteDirectory;
  try
  {
    TValueRestorer<TTerminal *> OpeningTerminalRestorer(FOpeningTerminal);
    FOpeningTerminal = Terminal;
    TTerminalThread * TerminalThread = new TTerminalThread(Terminal);
    TerminalThread->AllowAbandon = (Terminal == FActiveTerminal);
    try
    {
      if (ManagedTerminal != nullptr)
      {
        Terminal->SessionData->RemoteDirectory = ManagedTerminal->StateData->RemoteDirectory;

        if ((double)ManagedTerminal->ReopenStart == 0)
        {
          ManagedTerminal->ReopenStart = Now();
        }

        ManagedTerminal->Disconnected = false;
        ManagedTerminal->DisconnectedTemporarily = false;
        DebugAssert(ManagedTerminal->TerminalThread == nullptr);
        ManagedTerminal->TerminalThread = TerminalThread;
      }

      TNotifyEvent OnIdle;
      ((TMethod*)&OnIdle)->Code = TerminalThreadIdle;
      TerminalThread->OnIdle = OnIdle;
      if (Reopen)
      {
        TerminalThread->TerminalReopen();
      }
      else
      {
        TerminalThread->TerminalOpen();
      }
    }
    __finally
    {
      TerminalThread->OnIdle = nullptr;
      if (!TerminalThread->Release())
      {
        if (!AdHoc && (DebugAlwaysTrue(Terminal == FActiveTerminal)))
        {
          // terminal was abandoned, must create a new one to replace it
          Terminal = ManagedTerminal = CreateManagedTerminal(new TSessionData(L""));
          SetupTerminal(Terminal);
          OwnsObjects = false;
          Items[ActiveSessionIndex] = Terminal;
          OwnsObjects = true;
          FActiveSession = ManagedTerminal;
          // Can be nullptr, when opening the first session from command-line
          if (FScpExplorer != nullptr)
          {
            FScpExplorer->ReplaceTerminal(ManagedTerminal);
          }
        }
        // Now we do not have any reference to an abandoned terminal, so we can safely allow the thread
        // to complete its task and destroy the terminal afterwards.
        TerminalThread->Terminate();

        // When abandoning cancelled terminal, DoInformation(Phase = 0) does not make it to TerminalInformation handler.
        if (DebugAlwaysTrue(FAuthenticating > 0))
        {
          FKeepAuthenticateForm = false;
          AuthenticatingDone();
        }
      }
      else
      {
        if (ManagedTerminal != nullptr)
        {
          ManagedTerminal->TerminalThread = nullptr;
        }
      }
    }
  }
  __finally
  {
    Terminal->SessionData->RemoteDirectory = OrigRemoteDirectory;
    if (Terminal->Active && (ManagedTerminal != nullptr))
    {
      ManagedTerminal->ReopenStart = 0;
      ManagedTerminal->Permanent = true;
    }
  }

  if (DebugAlwaysTrue(Terminal->Active) && !Reopen && GUIConfiguration->QueueBootstrap)
  {
    FindQueueForTerminal(Terminal)->AddItem(new TBootstrapQueueItem());
  }
}

void TTerminalManager::CloseAuthenticateForm()
{
  SAFE_DESTROY(FAuthenticateForm);
}

bool TTerminalManager::ConnectTerminal(TTerminal * Terminal)
{
  bool Result = true;
  // were it an active terminal, it would allow abandoning, what this API cannot deal with
  DebugAssert(Terminal != FActiveTerminal);
  try
  {
    DoConnectTerminal(Terminal, false, false);
  }
  catch (Exception & E)
  {
    ShowExtendedExceptionEx(Terminal, &E);
    Result = false;
  }
  return Result;
}

void TTerminalManager::TerminalThreadIdle(void * /*Data*/, TObject * /*Sender*/)
{
  Application->ProcessMessages();
}

bool TTerminalManager::ConnectActiveTerminalImpl(bool Reopen)
{
  ActiveTerminal->CollectUsage();

  TTerminalPendingAction Action;
  bool Result;
  do
  {
    Action = tpNull;
    Result = false;
    try
    {
      DebugAssert(!ActiveTerminal.empty());

      DoConnectTerminal(ActiveTerminal, Reopen, false);

      if (ScpExplorer)
      {
        DebugAssert(ActiveTerminal->Status == ssOpened);
        SessionReady();
      }

      WinConfiguration->ClearTemporaryLoginData();

      Result = true;
    }
    catch (Exception & E)
    {
      DebugAssert(FTerminalPendingAction == tpNull);
      FTerminalPendingAction = ::tpNone;
      try
      {
        DebugAssert(ActiveTerminal != nullptr);
        ActiveTerminal->ShowExtendedException(&E);
        Action = FTerminalPendingAction;
      }
      __finally
      {
        FTerminalPendingAction = tpNull;
      }
    }
  }
  while (Action == tpReconnect);

  if (Action == tpFree)
  {
    DisconnectActiveTerminalIfPermanentFreeOtherwise();
  }

  return Result;
}

void TTerminalManager::DisconnectActiveTerminalIfPermanentFreeOtherwise()
{
  if (ActiveTerminal->Permanent)
  {
    DisconnectActiveTerminal();
  }
  else
  {
    FreeActiveTerminal();
  }
}

bool TTerminalManager::ConnectActiveTerminal()
{
  // add only stored sessions to the jump list,
  // ad-hoc session cannot be reproduced from just a session name
  if (StoredSessions->FindSame(ActiveTerminal->SessionData) != nullptr)
  {
    try
    {
      WinConfiguration->AddSessionToJumpList(ActiveTerminal->SessionData->SessionName);
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
    }
  }

  FAuthenticationCancelled = false;
  bool Result = ConnectActiveTerminalImpl(false);

  UnicodeString DateStamp = StandardDatestamp();
  if (Result)
  {
    if (Configuration->Usage->Get(L"OpenedSessionsFailedLastDate") == DateStamp)
    {
      Configuration->Usage->Inc(L"OpenedSessionsFailedRecovered");
    }
  }
  else
  {
    Configuration->Usage->Inc(L"OpenedSessionsFailed");
    Configuration->Usage->Set(L"OpenedSessionsFailedLastDate", DateStamp);
    if (FAuthenticationCancelled)
    {
      Configuration->Usage->Inc(L"OpenedSessionsFailedAfterCancel");
    }
  }

  if (Result && WinConfiguration->AutoOpenInPutty && CanOpenInPutty())
  {
    try
    {
      OpenInPutty();
    }
    catch(Exception & E)
    {
      ShowExtendedExceptionEx(nullptr, &E);
    }
  }

  return Result;
}

void TTerminalManager::DisconnectActiveTerminal()
{
  DebugAssert(ActiveTerminal);
  if (ActiveTerminal->Active)
  {
    ActiveTerminal->Close();
  }

  int Index = IndexOf(ActiveTerminal);

  TTerminalQueue * OldQueue;
  TTerminalQueue * NewQueue;
  OldQueue = reinterpret_cast<TTerminalQueue *>(FQueues->Items[Index]);
  NewQueue = this->NewQueue(ActiveTerminal);
  FQueues->Items[Index] = NewQueue;
  if (ScpExplorer != nullptr)
  {
    ScpExplorer->Queue = NewQueue;
  }
  delete OldQueue;

  ActiveTerminal->Disconnected = true;
  if (ScpExplorer != nullptr)
  {
    SessionReady(); // in case it was never connected
    ScpExplorer->TerminalDisconnected();
  }
  // disconnecting duplidate session removes need to distinguish the only connected session with short path
  DoSessionListChanged();
}

void TTerminalManager::ReconnectActiveTerminal()
{
  DebugAssert(ActiveTerminal);

  if (ScpExplorer)
  {
    if (ScpExplorer->Terminal == ActiveTerminal)
    {
      ScpExplorer->UpdateSession(ActiveTerminal);
    }
  }

  try
  {
    if (FTerminalPendingAction == tpNull)
    {
      ConnectActiveTerminalImpl(true);
    }
    else
    {
      FTerminalPendingAction = tpReconnect;
    }
  }
  catch(...)
  {
    DisconnectActiveTerminal();
    throw;
  }
}

void TTerminalManager::FreeAll()
{
  FDestroying = true;
  try
  {
    while (Count)
    {
      FreeTerminal(Sessions[0]);
    }
  }
  __finally
  {
    FDestroying = false;
  }
}

void TTerminalManager::FreeTerminal(TTerminal * Terminal)
{
  try
  {
    TManagedTerminal * ManagedSession = DebugNotNull(dynamic_cast<TManagedTerminal *>(Terminal));
    // we want the Login dialog to open on auto-workspace name,
    // as set in TCustomScpExplorerForm::FormClose
    if ((!FDestroying || !WinConfiguration->AutoSaveWorkspace) && !ManagedSession->LocalBrowser)
    {
      if (StoredSessions->FindSame(Terminal->SessionData) != nullptr)
      {
        WinConfiguration->LastStoredSession = Terminal->SessionData->Name;
      }
    }

    if (ScpExplorer != nullptr)
    {
      ScpExplorer->TerminalRemoved(Terminal);
    }

    if (Terminal->Active)
    {
      Terminal->Close();
    }
  }
  __finally
  {
    int Index = IndexOf(Terminal);
    Extract(Terminal);

    TTerminalQueue * Queue;
    Queue = reinterpret_cast<TTerminalQueue *>(FQueues->Items[Index]);
    FQueues->Delete(Index);
    FTerminationMessages->Delete(Index);

    if ((ActiveSession != nullptr) && (Terminal == ActiveSession))
    {
      TManagedTerminal * NewActiveTerminal;
      bool LastTerminalClosed = false;

      if (FDestroying)
      {
        NewActiveTerminal = nullptr;
      }
      else
      {
        if (Count > 0)
        {
          NewActiveTerminal = Sessions[Index < Count ? Index : Index - 1];
          if (!NewActiveTerminal->Active && !NewActiveTerminal->Disconnected)
          {
            NewActiveTerminal->Disconnected = true;
            NewActiveTerminal->DisconnectedTemporarily = true;
          }
        }
        else
        {
          NewActiveTerminal = nullptr;
          LastTerminalClosed = true;
          if (ScpExplorer != nullptr)
          {
            TAutoNestingCounter UpdatingCounter(FUpdating); // prevent tab flicker
            NewActiveTerminal = ScpExplorer->GetReplacementForLastSession();
          }
        }
      }
      DoSetActiveSession(NewActiveTerminal, false, LastTerminalClosed);
    }
    else
    {
      SaveTerminal(Terminal);
    }

    // only now all references to/from queue (particularly events to explorer)
    // are cleared
    delete Queue;
    delete Terminal;

    DoSessionListChanged();
  }
}

void TTerminalManager::UpdateScpExplorer(TManagedTerminal * Session, TTerminalQueue * Queue)
{
  FScpExplorer->ManagedSession = Session;
  FScpExplorer->Queue = Queue;
}

void TTerminalManager::UpdateScpExplorer()
{
  UpdateScpExplorer(ActiveSession, ActiveQueue);
}

void TTerminalManager::SetScpExplorer(TCustomScpExplorerForm * value)
{
  if (ScpExplorer != value)
  {
    // changing explorer is not supported yet
    DebugAssert(!ScpExplorer || !value);
    FScpExplorer = value;
    if (FScpExplorer)
    {
      UpdateScpExplorer();
      UpdateTaskbarList();
    }
  }
}

TManagedTerminal * TTerminalManager::GetActiveTerminal()
{
  TManagedTerminal * Result;
  if ((ActiveSession != nullptr) && !ActiveSession->LocalBrowser)
  {
    Result = ActiveSession;
  }
  else
  {
    Result = nullptr;
  }
  return Result;
}

void TTerminalManager::SetActiveSession(TManagedTerminal * value)
{
  DoSetActiveSession(value, false, false);
}

void TTerminalManager::SetActiveTerminalWithAutoReconnect(TManagedTerminal * value)
{
  DoSetActiveSession(value, true, false);
}

void TTerminalManager::DoSetActiveSession(TManagedTerminal * value, bool AutoReconnect, bool LastTerminalClosed)
{
  if (ActiveSession != value)
  {
    if (NonVisualDataModule != nullptr)
    {
      NonVisualDataModule->StartBusy();
    }
    void * Focus = nullptr;
    try
    {
      // here used to be call to TCustomScpExporer::UpdateSessionData (now UpdateSession)
      // but it seems to be duplicate to call from TCustomScpExporer::SessionChanging

      TManagedTerminal * PActiveSession = ActiveSession;
      FActiveSession = value;
      if (ScpExplorer)
      {
        Focus = ScpExplorer->SaveFocus();
        if ((ActiveSession != nullptr) &&
            ((ActiveSession->Status == ssOpened) || ActiveSession->Disconnected || ActiveSession->LocalBrowser))
        {
          SessionReady();
        }
        else
        {
          UpdateScpExplorer(nullptr, nullptr);
        }
      }
      UpdateAppTitle();

      if (PActiveSession != nullptr)
      {
        if (PActiveSession->DisconnectedTemporarily && DebugAlwaysTrue(PActiveSession->Disconnected))
        {
          PActiveSession->Disconnected = false;
          PActiveSession->DisconnectedTemporarily = false;
        }

        if (!PActiveSession->Active)
        {
          SaveTerminal(PActiveSession);
        }
      }

      if (ActiveSession != nullptr)
      {
        int Index = ActiveSessionIndex;
        if (!ActiveSession->Active &&
            !FTerminationMessages->Strings[Index].IsEmpty() &&
            DebugAlwaysTrue(!ActiveSession->LocalBrowser))
        {
          UnicodeString Message = FTerminationMessages->Strings[Index];
          FTerminationMessages->Strings[Index] = L"";
          if (AutoReconnect)
          {
            ReconnectActiveTerminal();
          }
          else
          {
            Exception * E = new ESshFatal(nullptr, Message);
            try
            {
              // finally show pending terminal message,
              // this gives user also possibility to reconnect
              ActiveTerminal->ShowExtendedException(E);
            }
            __finally
            {
              delete E;
            }
          }
        }

        // LastTerminalClosed is true only for a replacement local session,
        // and it should never happen that it fails to be activated
        if (LastTerminalClosed && DebugAlwaysFalse(value != ActiveSession))
        {
          LastTerminalClosed = false; // just in case
        }
      }
      else
      {
        LastTerminalClosed = true;
      }

      if (LastTerminalClosed && !Updating && (ScpExplorer != nullptr))
      {
        ScpExplorer->LastTerminalClosed();
      }

      if ((ActiveSession != nullptr) &&
          !ActiveSession->Active && !ActiveSession->Disconnected && !ActiveSession->LocalBrowser)
      {
        ConnectActiveTerminal();
      }
    }
    __finally
    {
      if (NonVisualDataModule != nullptr)
      {
        NonVisualDataModule->EndBusy();
      }
      if ((Focus != nullptr) && DebugAlwaysTrue(ScpExplorer != nullptr))
      {
        ScpExplorer->RestoreFocus(Focus);
      }
    }
  }
}

void TTerminalManager::QueueStatusUpdated()
{
  UpdateAppTitle();
}

bool TTerminalManager::ShouldDisplayQueueStatusOnAppTitle()
{
  bool Result = IsApplicationMinimized();
  if (!Result && (ScpExplorer != nullptr))
  {
    HWND Window = GetActiveWindow();
    Window = GetAncestor(Window, GA_ROOTOWNER);
    Result = (ScpExplorer->Handle != Window);
  }
  return Result;
}

UnicodeString TTerminalManager::FormatFormCaptionWithSession(TCustomForm * Form, const UnicodeString & Caption)
{
  return FormatFormCaption(Form, Caption, GetActiveSessionAppTitle());
}

UnicodeString TTerminalManager::GetAppProgressTitle()
{
  UnicodeString Result;
  UnicodeString QueueProgressTitle;
  UnicodeString ProgressTitle = !FProgressTitle.IsEmpty() ? FProgressTitle : ScpExplorer->GetProgressTitle();
  if (!FForegroundProgressTitle.IsEmpty())
  {
    Result = FForegroundProgressTitle;
  }
  else if (!ProgressTitle.IsEmpty() && !ForegroundTask())
  {
    Result = ProgressTitle;
  }
  else if (ShouldDisplayQueueStatusOnAppTitle() &&
           !(QueueProgressTitle = ScpExplorer->GetQueueProgressTitle()).IsEmpty())
  {
    Result = QueueProgressTitle;
  }

  return Result;
}

void TTerminalManager::UpdateAppTitle()
{
  if (ScpExplorer)
  {
    TForm * MainForm = GetMainForm();
    if (MainForm != ScpExplorer)
    {
      // triggers caption update for some forms
      MainForm->Perform(WM_MANAGES_CAPTION, 0, 0);
    }

    UnicodeString NewTitle = FormatMainFormCaption(GetActiveSessionAppTitle());

    UnicodeString ProgressTitle = GetAppProgressTitle();
    if (!ProgressTitle.IsEmpty())
    {
      NewTitle = ProgressTitle + TitleSeparator + NewTitle;
    }
    else if (ScpExplorer != nullptr)
    {
      UnicodeString Path = ScpExplorer->PathForCaption();
      if (!Path.IsEmpty())
      {
        NewTitle = Path + TitleSeparator + NewTitle;
      }
    }

    // Not updating MainForm here, as for all other possible main forms, this code is actually not what we want.
    // And they all update their title on their own (some using GetAppProgressTitle()).
    ScpExplorer->Caption = NewTitle;
    ScpExplorer->ApplicationTitleChanged();
  }
}

void TTerminalManager::SaveTerminal(TTerminal * Terminal)
{
  TSessionData * Data = StoredSessions->FindSame(Terminal->SessionData);
  if (Data != nullptr)
  {
    TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Terminal);
    DebugAssert(ManagedTerminal != nullptr);

    bool Changed = false;
    if (Terminal->SessionData->UpdateDirectories)
    {
      Data->CopyDirectoriesStateData(ManagedTerminal->StateData);
      Changed = true;
    }

    if (Changed)
    {
      // modified only, implicit
      StoredSessions->Save(false, false);
    }
  }
}

void TTerminalManager::HandleException(Exception * E)
{
  // can be null for example when exception is thrown on login dialog
  if (ActiveTerminal != nullptr)
  {
    ActiveTerminal->ShowExtendedException(E);
  }
  else
  {
    ShowExtendedException(E);
  }
}

void TTerminalManager::ApplicationException(TObject * /*Sender*/,
  Exception * E)
{
  HandleException(E);
}

void TTerminalManager::ApplicationShowHint(UnicodeString & HintStr,
  bool & /*CanShow*/, THintInfo & HintInfo)
{
  HintInfo.HintData = HintInfo.HintControl;
  if (HasLabelHintPopup(HintInfo.HintControl, HintStr))
  {
    // Hack for transfer setting labels.
    // Should be converted to something like HintLabel()
    HintInfo.HideTimeout = 100000; // "almost" never
  }
  else if (dynamic_cast<TProgressBar *>(HintInfo.HintControl) != nullptr)
  {
    // Hint is forcibly hidden in TProgressForm::FormHide
    HintInfo.HideTimeout = 100000; // "almost" never
    HintInfo.ReshowTimeout = 500; // updated each 0.5s
  }
  else
  {
    int HintMaxWidth = 300;

    TControl * ScaleControl = HintInfo.HintControl;
    if (DebugAlwaysFalse(HintInfo.HintControl == nullptr) ||
        (GetParentForm(HintInfo.HintControl) == nullptr))
    {
      ScaleControl = ScpExplorer;
    }
    HintMaxWidth = ScaleByTextHeight(ScaleControl, HintMaxWidth);

    HintInfo.HintMaxWidth = HintMaxWidth;
  }
}

bool TTerminalManager::HandleMouseWheel(WPARAM WParam, LPARAM LParam)
{
  // WORKAROUND This is no longer necessary on Windows 10 (except for WM_WANTS_MOUSEWHEEL_INACTIVE part)
  bool Result = false;
  if (Application->Active)
  {
    TPoint Point(LOWORD(LParam), HIWORD(LParam));
    TWinControl * Control = FindVCLWindow(Point);
    if (Control != nullptr)
    {
      TCustomForm * Form = GetParentForm(Control);
      // Only case we expect the parent form to be NULL is on the Find/Replace dialog,
      // which is owned by VCL's internal TRedirectorWindow.
      DebugAssert((Form != nullptr) || (Control->ClassName() == L"TRedirectorWindow"));
      if (Form != nullptr)
      {
        // Send it only to windows we tested it with.
        // Though we should sooner or later remove this test and pass it to all our windows.
        if (Form->Active && (Form->Perform(WM_WANTS_MOUSEWHEEL, 0, 0) == 1))
        {
          SendMessage(Control->Handle, WM_MOUSEWHEEL, WParam, LParam);
          Result = true;
        }
        else if (!Form->Active && (Form->Perform(WM_WANTS_MOUSEWHEEL_INACTIVE, 0, 0) == 1))
        {
          TWinControl * Control2;
          // FindVCLWindow stops on window level, when the window is not active? or when there's a modal window over it?
          // (but in any case, when we have operation running on top of Synchronization checklist).
          // WORKAROUND: The while loop does what AllLevels parameter of ControlAtPos should do, but it's broken.
          // Based on (now removed) Embarcadero QC 82143.
          while ((Control2 = dynamic_cast<TWinControl *>(Control->ControlAtPos(Control->ScreenToClient(Point), false, true))) != nullptr)
          {
            Control = Control2;
          }
          SendMessage(Control->Handle, WM_MOUSEWHEEL, WParam, LParam);
          Result = true;
        }
      }
    }
  }
  return Result;
}

void TTerminalManager::ApplicationMessage(TMsg & Msg, bool & Handled)
{
  if (Msg.message == FTaskbarButtonCreatedMessage)
  {
    CreateTaskbarList();
  }
  if (Msg.message == WM_MOUSEWHEEL)
  {
    Handled = HandleMouseWheel(Msg.wParam, Msg.lParam);
  }
}

void TTerminalManager::ApplicationModalBegin(TObject * /*Sender*/)
{
  InterfaceStartDontMeasure();
  NonVisualDataModule->StartBusy();
  if (ScpExplorer != nullptr)
  {
    ScpExplorer->SuspendWindowLock();
  }
}

void TTerminalManager::ApplicationModalEnd(TObject * /*Sender*/)
{
  NonVisualDataModule->EndBusy();
  if (ScpExplorer != nullptr)
  {
    ScpExplorer->ResumeWindowLock();
  }
}

void TTerminalManager::InitTaskbarButtonCreatedMessage()
{
  // XE6 VCL already handles TaskbarButtonCreated, but does not call ChangeWindowMessageFilterEx.
  // So we keep our implementation.
  // See also https://stackoverflow.com/q/14614823/850848#14618587
  FTaskbarButtonCreatedMessage = RegisterWindowMessage(L"TaskbarButtonCreated");

  HINSTANCE User32Library = LoadLibrary(L"user32.dll");
  ChangeWindowMessageFilterExProc ChangeWindowMessageFilterEx =
    (ChangeWindowMessageFilterExProc)GetProcAddress(User32Library, "ChangeWindowMessageFilterEx");

  if (ChangeWindowMessageFilterEx != nullptr)
  {
    // without this we won't get TaskbarButtonCreated, when app is running elevated
    ChangeWindowMessageFilterEx(
      Application->Handle, FTaskbarButtonCreatedMessage, MSGFLT_ALLOW, nullptr);
  }
}

void TTerminalManager::CreateTaskbarList()
{

  ReleaseTaskbarList();

  if(SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_ALL,
        IID_ITaskbarList3, (void **) &FTaskbarList)))
  {
    if (ScpExplorer != nullptr)
    {
      UpdateTaskbarList();
    }
  }
}

void TTerminalManager::ReleaseTaskbarList()
{
  if (FTaskbarList != nullptr)
  {
    FTaskbarList->Release();
  }
}

void TTerminalManager::UpdateTaskbarList()
{
  ScpExplorer->UpdateTaskbarList(FTaskbarList);
}

void TTerminalManager::DeleteLocalFile(const UnicodeString & FileName, bool Alternative, int32_t & Deleted)
{
  Deleted = RecursiveDeleteFileChecked(FileName, (WinConfiguration->DeleteToRecycleBin != Alternative));
}

void TTerminalManager::TerminalQueryUser(TObject * Sender,
  const UnicodeString & Query, TStrings * MoreMessages, uint32_t Answers,
  const TQueryParams * Params, uint32_t & Answer, TQueryType Type, void * /*Arg*/)
{
  UnicodeString HelpKeyword;
  TMessageParams MessageParams(Params);
  UnicodeString AQuery = Query;

  if (Params != nullptr)
  {
    HelpKeyword = Params->HelpKeyword;

    if (FLAGSET(Params->Params, qpFatalAbort))
    {
      AQuery = FMTLOAD(WARN_FATAL_ERROR, (AQuery));

      if (!MessageParams.TimerMessage.IsEmpty())
      {
        MessageParams.TimerMessage = FMTLOAD(WARN_FATAL_ERROR, (MessageParams.TimerMessage));
      }
    }
  }

  if (ScpExplorer)
  {
    Answer = ScpExplorer->MoreMessageDialog(AQuery, MoreMessages, Type, Answers,
      HelpKeyword, &MessageParams, dynamic_cast<TTerminal *>(Sender));
  }
  else
  {
    Answer = MoreMessageDialog(AQuery, MoreMessages, Type, Answers, HelpKeyword,
      &MessageParams);
  }
}

void TTerminalManager::AuthenticateFormCancel(TObject * Sender)
{
  TAuthenticateForm * Form = dynamic_cast<TAuthenticateForm *>(Sender);
  DebugAssert(Form != nullptr);
  TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Form->Terminal);
  // will be null e.g. for background transfers
  if (ManagedTerminal != nullptr)
  {
    TTerminalThread * TerminalThread = ManagedTerminal->TerminalThread;
    // can be NULL for reconnects from transfers
    if ((TerminalThread != nullptr) && !TerminalThread->Cancelling)
    {
      Form->Log(LoadStr(AUTH_CANCELLING));
      TerminalThread->Cancel();
    }
  }
  FAuthenticationCancelled = true;
}

TAuthenticateForm * TTerminalManager::MakeAuthenticateForm(
  TTerminal * Terminal)
{
  TAuthenticateForm * Dialog = SafeFormCreate<TAuthenticateForm>();
  Dialog->Init(Terminal);
  DebugAssert(Dialog->OnCancel == nullptr);
  Dialog->OnCancel = AuthenticateFormCancel;
  return Dialog;
}

void TTerminalManager::FileNameInputDialogInitializeRenameBaseName(
  TObject * /*Sender*/, TInputDialogData * Data)
{
  EditSelectBaseName(Data->Edit->Handle);
}

void TTerminalManager::TerminalPromptUser(
  TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
  TStrings * Prompts, TStrings * Results, bool & Result, void * /*Arg*/)
{
  if (((Kind == pkPrompt) || (Kind == pkFileName)) && (FAuthenticateForm == nullptr) &&
      (Terminal->Status != ssOpening))
  {
    DebugAssert(Instructions.IsEmpty());
    DebugAssert(Prompts->Count == 1);
    DebugAssert(FLAGSET(int(Prompts->Objects[0]), pupEcho));
    UnicodeString AResult = Results->Strings[0];

    TInputDialogInitialize InputDialogInitialize = nullptr;
    if ((Kind == pkFileName) && !WinConfiguration->RenameWholeName)
    {
      InputDialogInitialize = FileNameInputDialogInitializeRenameBaseName;
    }

    Result = InputDialog(Name, Prompts->Strings[0], AResult, L"", nullptr, false, InputDialogInitialize);
    if (Result)
    {
      Results->Strings[0] = AResult;
    }
  }
  else
  {
    TAuthenticateForm * AuthenticateForm = FAuthenticateForm;
    if (AuthenticateForm == nullptr)
    {
      AuthenticateForm = MakeAuthenticateForm(Terminal);
    }

    try
    {
      Result = AuthenticateForm->PromptUser(Kind, Name, Instructions, Prompts, Results,
        (FAuthenticateForm != nullptr), Terminal->StoredCredentialsTried);
    }
    __finally
    {
      if (FAuthenticateForm == nullptr)
      {
        delete AuthenticateForm;
      }
    }
  }
}

void TTerminalManager::TerminalDisplayBanner(
  TTerminal * Terminal, const UnicodeString & SessionName,
  const UnicodeString & Banner, bool & NeverShowAgain, int32_t Options, uint32_t & Params)
{
  DebugAssert(FAuthenticateForm != nullptr);
  TAuthenticateForm * AuthenticateForm = FAuthenticateForm;
  if (AuthenticateForm == nullptr)
  {
    AuthenticateForm = MakeAuthenticateForm(Terminal);
  }

  try
  {
    AuthenticateForm->Banner(Banner, NeverShowAgain, Options, Params);
  }
  __finally
  {
    if (FAuthenticateForm == nullptr)
    {
      delete AuthenticateForm;
    }
  }
}

void TTerminalManager::TerminalShowExtendedException(
  TTerminal * Terminal, Exception * E, void * /*Arg*/)
{
  if (ScpExplorer)
  {
    ScpExplorer->ShowExtendedException(Terminal, E);
  }
  else
  {
    ShowExtendedExceptionEx(Terminal, E);
  }
}

static TDateTime DirectoryReadingProgressDelay(0, 0, 1, 500);

void TTerminalManager::TerminalReadDirectoryProgress(
  TObject * Sender, int32_t Progress, int32_t ResolvedLinks, bool & Cancel)
{
  DebugAlwaysTrue((Sender == FOpeningTerminal) == (FAuthenticateForm != NULL));
  if (Progress == 0)
  {
    if ((ScpExplorer != nullptr) && (Sender != FOpeningTerminal))
    {
      // See also TCustomScpExplorerForm::RemoteDirViewBusy
      ScpExplorer->LockWindow();
    }
    FDirectoryReadingStart = Now();
    if (!FProgressTitle.IsEmpty() || !FForegroundProgressTitle.IsEmpty())
    {
      FProgressTitle = L"";
      FForegroundProgressTitle = L"";
      UpdateAppTitle();
    }

    // Reset "was ESC ever pressed?" state
    GetAsyncKeyState(VK_ESCAPE);
  }
  else if (Progress < 0)
  {
    if (Progress == -2)
    {
      // cancelled
      if (ScpExplorer != nullptr)
      {
        ScpExplorer->ReadDirectoryCancelled();
      }
    }
    else
    {
      if ((ScpExplorer != nullptr) && (Sender != FOpeningTerminal))
      {
        ScpExplorer->UnlockWindow();
      }

      FProgressTitle = L"";
      FForegroundProgressTitle = L"";
      UpdateAppTitle();
    }
  }
  else
  {
    // If the least significant bit is set,
    // the key was pressed after the previous call to GetAsyncKeyState.
    int KeyState = GetAsyncKeyState(VK_ESCAPE);
    if (FLAGSET(KeyState, 0x01))
    {
      Cancel = true;
    }

    if ((Now() - FDirectoryReadingStart) >= DirectoryReadingProgressDelay)
    {
      // 4 is arbitrary number
      FForegroundProgressTitle =
        FMTLOAD(ResolvedLinks >= 4 ? DIRECTORY_READING_AND_RESOLVING_PROGRESS : DIRECTORY_READING_PROGRESS,
          (Progress));
      UpdateAppTitle();
    }
  }
}

void TTerminalManager::TerminalCustomCommand(
  TTerminal * /*Terminal*/, const UnicodeString & Command, bool & Handled)
{
  // Implementation has to be thread-safe
  Handled = CopyCommandToClipboard(Command);
}

void TTerminalManager::AuthenticatingDone()
{
  FAuthenticating--;
  if (FAuthenticating == 0)
  {
    BusyEnd(FBusyToken);
    FBusyToken = nullptr;
  }
  if (!FKeepAuthenticateForm)
  {
    CloseAuthenticateForm();
  }
}

void TTerminalManager::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & Str, bool DebugUsedArg(Status), int32_t Phase, const UnicodeString & Additional)
{
  if (ScpExplorer != nullptr)
  {
    ScpExplorer->TerminalConnecting();
  }
  if (Phase == 1)
  {
    if (FAuthenticating == 0)
    {
      FBusyToken = BusyStart();
    }
    FAuthenticating++;
  }
  else if (Phase == 0)
  {
    DebugAssert(FAuthenticating > 0);
    AuthenticatingDone();
  }
  else
  {
    if (FAuthenticating > 0)
    {
      bool ShowPending = false;
      if (FAuthenticateForm == nullptr)
      {
        FAuthenticateForm = MakeAuthenticateForm(Terminal);
        ShowPending = true;
      }
      FAuthenticateForm->Log(Str, Additional);
      if (ShowPending)
      {
        FAuthenticateForm->ShowAsModal();
      }
    }
  }
}

void TTerminalManager::OperationFinished(::TFileOperation Operation,
  TOperationSide Side, bool Temp, const UnicodeString & FileName, bool Success,
  TOnceDoneOperation & OnceDoneOperation)
{
  DebugAssert(ScpExplorer);
  ScpExplorer->OperationFinished(Operation, Side, Temp, FileName, Success,
    OnceDoneOperation);
}

void TTerminalManager::OperationProgress(
  TFileOperationProgressType & ProgressData)
{
  UpdateAppTitle();
  DebugAssert(ScpExplorer);
  ScpExplorer->OperationProgress(ProgressData);
}

void TTerminalManager::QueueEvent(TTerminalQueue * Queue, TQueueEvent Event)
{
  const TGuard Guard(FQueueSection);
  FQueueEvents.push_back(std::make_pair(Queue, Event));
}

void TTerminalManager::DoConfigurationChange()
{
  DebugAssert(Configuration);
  DebugAssert(Configuration == WinConfiguration);

  TTerminalQueue * Queue;
  for (int Index = 0; Index < Count; Index++)
  {
    DebugAssert(Sessions[Index]->Log);
    Sessions[Index]->Log->ReflectSettings();
    Sessions[Index]->ActionLog->ReflectSettings();
    Queue = reinterpret_cast<TTerminalQueue *>(FQueues->Items[Index]);
    SetQueueConfiguration(Queue);
  }

  if (ScpExplorer)
  {
    ScpExplorer->ConfigurationChanged();
  }
}

void TTerminalManager::ConfigurationChange(TObject * /*Sender*/)
{
  if (FMainThread == GetCurrentThreadId())
  {
    DoConfigurationChange();
  }
  else
  {
    const TGuard Guard(FChangeSection.get());
    FPendingConfigurationChange++;
  }
}

void TTerminalManager::SessionReady()
{
  UpdateScpExplorer();
  ScpExplorer->SessionReady();
}

TStrings * TTerminalManager::GetSessionList()
{
  FSessionList->Clear();
  for (int i = 0; i < Count; i++)
  {
    TManagedTerminal * Terminal = Sessions[i];
    UnicodeString Name = GetSessionTitle(Terminal, true);
    FSessionList->AddObject(Name, Terminal);
  }
  return FSessionList;
}

int TTerminalManager::GetActiveSessionIndex() const
{
  return (ActiveSession != nullptr) ? IndexOf(ActiveSession) : -1;
}

void TTerminalManager::SetActiveSessionIndex(int32_t value)
{
  ActiveSession = Sessions[value];
}

UnicodeString TTerminalManager::GetPathForSessionTabName(const UnicodeString & Path)
{
  UnicodeString Result = Path;
  const int MaxPathLen = 16;
  if ((WinConfiguration->SessionTabNameFormat == stnfShortPathTrunc) &&
      (Result.Length() > MaxPathLen))
  {
    Result = Result.SubString(1, MaxPathLen - 2) + Ellipsis;
  }
  return Result;
}

UnicodeString TTerminalManager::GetSessionTitle(TManagedTerminal * Session, bool Unique)
{
  UnicodeString Result;
  if (!Session->LocalBrowser)
  {
    Result = Session->SessionData->SessionName;
    if (Unique &&
        (WinConfiguration->SessionTabNameFormat != stnfNone))
    {
      int Index = IndexOf(Session);
      // not for background transfer sessions and disconnected sessions
      if ((Index >= 0) && Session->Active)
      {
        for (int Index2 = 0; Index2 < Count; Index2++)
        {
          UnicodeString Name = Sessions[Index2]->SessionData->SessionName;
          if ((Sessions[Index2] != Session) &&
              Sessions[Index2]->Active &&
              SameText(Name, Result))
          {
            UnicodeString Path = ExtractShortName(Session->CurrentDirectory, true);
            if (!Path.IsEmpty())
            {
              Path = GetPathForSessionTabName(Path);
              Result = FORMAT(L"%s (%s)", Result, Path);
            }
            break;
          }
        }
      }
    }
  }
  else
  {
    // should happen only when closing
    if (ScpExplorer != nullptr)
    {
      Result = ScpExplorer->GetLocalBrowserSessionTitle(Session);
    }
  }
  return Result;
}

UnicodeString TTerminalManager::GetActiveSessionAppTitle()
{
  UnicodeString Result;
  if ((ActiveSession != nullptr) && !ActiveSession->LocalBrowser)
  {
    Result = GetSessionTitle(ActiveSession, false);
  }
  return Result;
}

TTerminalQueue * TTerminalManager::GetActiveQueue()
{
  TTerminalQueue * Result = nullptr;
  if (ActiveSession != nullptr)
  {
    Result = reinterpret_cast<TTerminalQueue *>(FQueues->Items[ActiveSessionIndex]);
  }
  return Result;
}

void TTerminalManager::CycleTerminals(bool Forward)
{
  if (Count > 0)
  {
    int Index = ActiveSessionIndex;
    Index += Forward ? 1 : -1;
    if (Index < 0)
    {
      Index = Count-1;
    }
    else if (Index >= Count)
    {
      Index = 0;
    }
    ActiveSessionIndex = Index;
  }
}

bool TTerminalManager::CanOpenInPutty()
{
  return (ActiveTerminal != nullptr) && !GUIConfiguration->PuttyPath.Trim().IsEmpty();
}

void TTerminalManager::OpenInPutty()
{
  Configuration->Usage->Inc(L"OpenInPutty");

  TSessionData * Data = nullptr;
  try
  {
    // Is NULL on the first session when called from ConnectActiveTerminal()
    // due to WinConfiguration->AutoOpenInPutty
    if (ScpExplorer != nullptr)
    {
      Data = ScpExplorer->CloneCurrentSessionData();
    }
    else
    {
      Data = new TSessionData(L"");
      DebugAssert(ActiveTerminal != nullptr);
      Data->Assign(ActiveTerminal->SessionData);
      ActiveTerminal->UpdateSessionCredentials(Data);
    }

    if (ActiveTerminal->TunnelLocalPortNumber != 0)
    {
      Data->ConfigureTunnel(ActiveTerminal->TunnelLocalPortNumber);
    }

    OpenSessionInPutty(Data);
  }
  __finally
  {
    delete Data;
  }
}

void TTerminalManager::NewSession(
  const UnicodeString & SessionUrl, bool ReloadSessions, TForm * LinkedForm, bool ReplaceExisting)
{
  if (ReloadSessions)
  {
    StoredSessions->Reload();
  }

  std::unique_ptr<TObjectList> DataList(std::make_unique<TObjectList>());

  bool Retry;
  do
  {
    Retry = false;
    if (!DataList) // first round
    {
      DataList.reset(new TObjectList());
      UnicodeString DownloadFile; // unused
      GetLoginData(SessionUrl, nullptr, DataList.get(), DownloadFile, true, LinkedForm);
    }
    else
    {
      if (!DoLoginDialog(DataList.get(), LinkedForm))
      {
        Abort(); // As GetLoginData would do
      }
    }

    if (DataList->Count > 0)
    {
      if (ReplaceExisting)
      {
        // Tested for only the implicit Commanders' local browser
        DebugAssert((Count == 0) || ((Count == 1) && Sessions[0]->LocalBrowser));
        TAutoNestingCounter UpdatingCounter(FUpdating); // prevent tab flicker
        FreeAll();
      }
      TManagedTerminal * ANewSession = NewSessions(DataList.get());
      bool AdHoc = (DataList->Count == 1) && (StoredSessions->FindSame(reinterpret_cast<TSessionData *>(DataList->Items[0])) == nullptr);
      bool CanRetry = SessionUrl.IsEmpty() && AdHoc;
      bool ShowLoginWhenNoSession = WinConfiguration->ShowLoginWhenNoSession;
      if (CanRetry)
      {
        // we will show our own login dialog, so prevent opening an empty one
        WinConfiguration->ShowLoginWhenNoSession = false;
      }
      try
      {
        ActiveSession = ANewSession;
      }
      __finally
      {
        if (CanRetry) // do not reset the value, unless really needed, as it can theoretically be changed meanwhile by the user
        {
          WinConfiguration->ShowLoginWhenNoSession = ShowLoginWhenNoSession;
        }
      }
      Retry = CanRetry && (ActiveSession != ANewSession);
    }
  }
  while (Retry);
}

void TTerminalManager::Idle(bool SkipCurrentTerminal)
{

  if (FPendingConfigurationChange > 0) // optimization
  {
    bool Changed = false;

    {
      const TGuard Guard(FChangeSection.get());
      if (DebugAlwaysTrue(FPendingConfigurationChange > 0))
      {
        FPendingConfigurationChange--;
        Changed = true;
      }
    }

    if (Changed)
    {
      DoConfigurationChange();
    }
  }

  for (int Index = 0; Index < Count; Index++)
  {
    TManagedTerminal * Terminal = Sessions[Index];
    try
    {
      if (!SkipCurrentTerminal || (Terminal != ActiveTerminal))
      {
        // make sure Idle is called on the thread that runs the terminal
        if (Terminal->TerminalThread != nullptr)
        {
          Terminal->TerminalThread->Idle();
        }
        else
        {
          if (Terminal->Active)
          {
            Terminal->Idle();
          }
        }

        if (Terminal->Active)
        {
          DebugAssert(Index < FQueues->Count);
          if (Index < FQueues->Count)
          {
            reinterpret_cast<TTerminalQueue *>(FQueues->Items[Index])->Idle();
          }
        }
      }
    }
    catch(Exception & E)
    {
      if (Terminal == ActiveTerminal)
      {
        // throw further, so that the exception is handled in proper place
        // (particularly in broken-transfer reconnect handler, bug 72)
        throw;
      }
      else
      {
        // we may not have inactive terminal, unless there is a explorer,
        // also Idle is called from explorer anyway
        DebugAssert(ScpExplorer != nullptr);
        if (ScpExplorer != nullptr)
        {
          ScpExplorer->InactiveTerminalException(Terminal, &E);
        }

        if (!Terminal->Active)
        {
          // if session is lost, save the error message and rethrow it
          // once the terminal gets activated
          FTerminationMessages->Strings[Index] = E.Message;
        }
      }
    }
  }

  TTerminalQueue * QueueWithEvent;
  TQueueEvent QueueEvent;

  do
  {
    QueueWithEvent = nullptr;

    {
      const TGuard Guard(FQueueSection);

      if (!FQueueEvents.empty())
      {
        QueueWithEvent = FQueueEvents[0].first;
        QueueEvent = FQueueEvents[0].second;
        FQueueEvents.erase(FQueueEvents.begin());
      }
    }

    if (QueueWithEvent != nullptr)
    {
      int Index = FQueues->IndexOf(QueueWithEvent);
      // the session may not exist anymore
      if (Index >= 0)
      {
        TManagedTerminal * Terminal = Sessions[Index];
        // we can hardly have a queue event without explorer
        DebugAssert(ScpExplorer != nullptr);
        if (ScpExplorer != nullptr)
        {
          ScpExplorer->QueueEvent(Terminal, QueueWithEvent, QueueEvent);
        }
      }
    }
  }
  while (QueueWithEvent != nullptr);
}

void TTerminalManager::MasterPasswordPrompt()
{
  if (GetCurrentThreadId() == MainThreadID)
  {
    if (!DoMasterPasswordDialog())
    {
      Abort();
    }
  }
  else
  {
    // this can happen only when we keep cancelling all master password prompts
    // as long as the sessing finally connects (session password has to be
    // explicitly typed in), and background transfer is started
    Abort();
  }
}

void TTerminalManager::Move(TTerminal * Source, TTerminal * Target)
{
  int SourceIndex = IndexOf(Source);
  int TargetIndex = IndexOf(Target);
  TTerminalList::Move(SourceIndex, TargetIndex);
  FQueues->Move(SourceIndex, TargetIndex);
  DoSessionListChanged();
  // when there are indexed sessions with the same name,
  // the index may change when reordering the sessions
  UpdateAppTitle();
}

void TTerminalManager::DoSessionListChanged()
{
  if ((FScpExplorer != nullptr) && !Updating)
  {
    FScpExplorer->SessionListChanged();
  }
}

void TTerminalManager::SaveWorkspace(TList * DataList)
{
  for (int Index = 0; Index < Count; Index++)
  {
    TManagedTerminal * ManagedTerminal = Sessions[Index];
    TSessionData * Data = StoredSessions->SaveWorkspaceData(ManagedTerminal->StateData, Index);
    if (ManagedTerminal->Active)
    {
      ManagedTerminal->UpdateSessionCredentials(Data);
    }
    DataList->Add(Data);
  }
}

bool TTerminalManager::IsActiveTerminalForSite(TTerminal * Terminal, TSessionData * Data)
{
  bool Result = Terminal->Active;
  if (Result)
  {
    std::unique_ptr<TSessionData> TerminalData(Terminal->SessionData->Clone());
    Terminal->UpdateSessionCredentials(TerminalData.get());
    Result = TerminalData->IsSameSite(Data);
  }
  return Result;
}

TManagedTerminal * TTerminalManager::FindActiveTerminalForSite(TSessionData * Data)
{
  TManagedTerminal * Result = nullptr;
  for (int Index = 0; (Result == nullptr) && (Index < Count); Index++)
  {
    TManagedTerminal * Terminal = Sessions[Index];
    if (IsActiveTerminalForSite(Terminal, Data))
    {
      Result = Terminal;
    }
  }
  return Result;
}

TTerminalQueue * TTerminalManager::FindQueueForTerminal(TTerminal * Terminal)
{
  int Index = IndexOf(Terminal);
  return reinterpret_cast<TTerminalQueue *>(FQueues->Items[Index]);
}

bool TTerminalManager::UploadPublicKey(
  TTerminal * Terminal, TSessionData * Data, UnicodeString & FileName)
{
  std::unique_ptr<TOpenDialog> OpenDialog(std::make_unique<TOpenDialog>(Application));
  OpenDialog->Title = LoadStr(LOGIN_PUBLIC_KEY_TITLE);
  OpenDialog->Filter = LoadStr(LOGIN_PUBLIC_KEY_FILTER);
  OpenDialog->DefaultExt = PuttyKeyExt;
  OpenDialog->FileName = FileName;

  bool Result = OpenDialog->Execute();
  if (Result)
  {
    Configuration->Usage->Inc(L"PublicKeyInstallation");
    FileName = OpenDialog->FileName;

    VerifyAndConvertKey(FileName, false);

    bool AdHocTerminal = (Terminal == nullptr);
    std::unique_ptr<TTerminal> TerminalOwner;
    if (AdHocTerminal)
    {
      DebugAssert(Data != nullptr);

      TAutoFlag KeepAuthenticateFormFlag(FKeepAuthenticateForm);
      try
      {
        TerminalOwner.reset(CreateTerminal(Data));
        Terminal = TerminalOwner.get();
        SetupTerminal(Terminal);
        Terminal->OnProgress = nullptr;
        Terminal->OnFinished = nullptr;
        DoConnectTerminal(Terminal, false, true);
      }
      catch (Exception & E)
      {
        CloseAuthenticateForm();
        throw;
      }
    }

    UnicodeString Installed;
    try
    {
      UnicodeString SshImplementation = Terminal->GetSessionInfo().SshImplementation;
      UnicodeString NotOpenSSHMessage = FMTLOAD(LOGIN_NOT_OPENSSH, (SshImplementation));
      if (IsOpenSSH(SshImplementation) ||
          (MessageDialog(NotOpenSSHMessage, qtConfirmation, qaOK | qaCancel, HELP_LOGIN_AUTHORIZED_KEYS) == qaOK))
      {
        // Ad-hoc terminal
        if (FAuthenticateForm != nullptr)
        {
          UnicodeString Comment;
          bool UnusedHasCertificate;
          GetPublicKeyLine(FileName, Comment, UnusedHasCertificate);
          FAuthenticateForm->Log(FMTLOAD(LOGIN_PUBLIC_KEY_UPLOAD, (Comment)));
        }

        Installed = Terminal->UploadPublicKey(FileName);
      }
    }
    __finally
    {
      CloseAuthenticateForm(); // When uploading from Login dialog
    }

    if (!Installed.IsEmpty())
    {
      Terminal->LogEvent("Public key installation done.");
      if (AdHocTerminal)
      {
        TerminalOwner.reset(nullptr);
      }
      else
      {
        Terminal->Log->AddSeparator();
      }

      MessageDialog(Installed, qtInformation, qaOK, HELP_LOGIN_AUTHORIZED_KEYS);
    }
  }

  return Result;
}

bool TTerminalManager::IsUpdating()
{
  return (FUpdating > 0);
}

bool TTerminalManager::HookFatalExceptionMessageDialog(TMessageParams & Params)
{
  bool Result =
    DebugAlwaysTrue(ActiveTerminal != nullptr) &&
    DebugAlwaysTrue(Params.Timer == 0) &&
    DebugAlwaysTrue(Params.TimerEvent == nullptr) &&
    DebugAlwaysTrue(FTerminalWithFatalExceptionTimer == nullptr);
  if (Result)
  {
    Params.Timer = MSecsPerSec / 4;
    Params.TimerEvent = TerminalFatalExceptionTimer;
    FTerminalWithFatalExceptionTimer = ActiveTerminal;
    FTerminalReconnnecteScheduled = false;
  }
  return Result;
}

void TTerminalManager::UnhookFatalExceptionMessageDialog()
{
  DebugAssert(FTerminalWithFatalExceptionTimer != nullptr);
  FTerminalWithFatalExceptionTimer = nullptr;
}

bool TTerminalManager::ScheduleTerminalReconnnect(TTerminal * Terminal)
{
  bool Result = (FTerminalWithFatalExceptionTimer == Terminal);
  if (Result)
  {
    FTerminalReconnnecteScheduled = true;
  }
  return Result;
}

void TTerminalManager::TerminalFatalExceptionTimer(uint32_t & Result)
{
  if (FTerminalReconnnecteScheduled)
  {
    Result = qaRetry;
    FTerminalReconnnecteScheduled = false;
  }
}
