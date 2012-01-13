//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarConfiguration.h"
#include "FarTexts.h"
#include "FarDialog.h"
#include "plugin.hpp"
#include <Common.h>
#include <CoreMain.h>
#include <Exceptions.h>
#include <Terminal.h>
#include <GUITools.h>
//---------------------------------------------------------------------------

TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst)
{
  return new TWinSCPPlugin(HInst);
}

//---------------------------------------------------------------------------
TMessageParams::TMessageParams()
{
  Flags = 0;
  Aliases = NULL;
  AliasesCount = 0;
  Params = 0;
  Timer = 0;
  TimerEvent = NULL;
  TimerAnswers = 0;
  Timeout = 0;
  TimeoutAnswer = 0;
}
//---------------------------------------------------------------------------
TWinSCPPlugin::TWinSCPPlugin(HINSTANCE HInst) :
    TCustomFarPlugin(HInst)
{
  FInitialized = false;
  Self = this;
  CreateMutex(NULL, false, L"NetBoxFar");
}
//---------------------------------------------------------------------------
TWinSCPPlugin::~TWinSCPPlugin()
{
  if (FInitialized)
  {
    FarConfiguration->SetPlugin(NULL);
    CoreFinalize();
  }
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::HandlesFunction(THandlesFunction Function)
{
  return (Function == hfProcessKey || Function == hfProcessEvent);
}
//---------------------------------------------------------------------------
int TWinSCPPlugin::GetMinFarVersion()
{
  return FAR20; // FAR170BETA5;
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    TCustomFarPlugin::SetStartupInfo(Info);
    assert(!FInitialized);
    CoreInitialize();
    FInitialized = true;
  }
  catch (const std::exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::GetPluginInfoEx(long unsigned & Flags,
  TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
  TStrings * PluginConfigStrings, TStrings * CommandPrefixes)
{
  Flags = PF_FULLCMDLINE;
  if (FarConfiguration->GetDisksMenu())
  {
    DiskMenuStrings->AddObject(GetMsg(PLUGIN_NAME),
      (TObject *)FarConfiguration->GetDisksMenuHotKey());
  }
  if (FarConfiguration->GetPluginsMenu())
  {
    PluginMenuStrings->Add(GetMsg(PLUGIN_NAME));
  }
  if (FarConfiguration->GetPluginsMenuCommands())
  {
    PluginMenuStrings->Add(GetMsg(MENU_COMMANDS));
  }
  PluginConfigStrings->Add(GetMsg(PLUGIN_NAME));
  CommandPrefixes->SetCommaText(FarConfiguration->GetCommandPrefixes());
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::ImportSessions(const std::wstring &RegistryStorageKey,
    int &imported)
{
    // DEBUG_PRINTF(L"begin");
    imported = 0;
    THierarchicalStorage *ImportStorage = new TRegistryStorage(RegistryStorageKey);
    THierarchicalStorage *ExportStorage = new TRegistryStorage(Configuration->GetRegistryStorageKey());
    ExportStorage->SetAccessMode(smReadWrite);
    TSessionData *FactoryDefaults = new TSessionData(L"");
    BOOST_SCOPE_EXIT ( (&FactoryDefaults) (&ImportStorage) (&ExportStorage) )
    {
        delete FactoryDefaults;
        delete ImportStorage;
        delete ExportStorage;
    } BOOST_SCOPE_EXIT_END

    int failed = 0;
    if (ImportStorage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), /* CanCreate */ false) &&
        ExportStorage->OpenSubKey(Configuration->GetStoredSessionsSubKey(), /* CanCreate */ true)
       )
    {
        TStrings* SubKeyNames = new TStringList();
        BOOST_SCOPE_EXIT ( (&SubKeyNames) )
        {
            delete SubKeyNames;
        } BOOST_SCOPE_EXIT_END
        ImportStorage->GetSubKeyNames(SubKeyNames);
        // DEBUG_PRINTF(L"SubKeyNames->GetCount = %d", SubKeyNames->GetCount());
        for (int i = 0; i < SubKeyNames->GetCount(); i++)
        {
            // DEBUG_PRINTF(L"SubKeyNames->GetString(%d) = %s", i, SubKeyNames->GetString(i).c_str());
            TSessionData *ExportData = new TSessionData(SubKeyNames->GetString(i));
            BOOST_SCOPE_EXIT ( (&ExportData) )
            {
                delete ExportData;
            } BOOST_SCOPE_EXIT_END
            if (!ExportData->HasSessionName())
                continue;
            ExportData->Load(ImportStorage);
            ExportData->SetModified(true);
            try
            {
                ExportData->Save(ExportStorage, /* PuttyExport = */ false, FactoryDefaults);
                imported++;
            }
            catch (const std::exception &E)
            {
                failed++;
            }
        }
    }

    // DEBUG_PRINTF(L"end, imported = %d, failed = %d", imported, failed);
    return true;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::ImportSessions()
{
    // DEBUG_PRINTF(L"begin");
    const std::wstring SessionsKeys[] = 
    {
        L"Software\\Martin Prikryl\\WinSCP 2", // WinSCP 1.6.2 sessions
        L"Software\\Michael Lukashov\\FarNetBox", // NetBox 2.0.0 sessions
        L"",
    };
    int all_imported = 0;
    for (int i = 0; !SessionsKeys[i].empty(); i++)
    {
        std::wstring RegistryStorageKey = SessionsKeys[i];
        // DEBUG_PRINTF(L"RegistryStorageKey = %s", RegistryStorageKey.c_str());
        try
        {
          int imported = 0;
          ImportSessions(RegistryStorageKey, imported);
          all_imported += imported;
        }
        catch (const std::exception &E)
        {
            ShowExtendedException(&E);
        }
    }
    MoreMessageDialog(FORMAT(GetMsg(IMPORTED_SESSIONS_INFO).c_str(), all_imported),
        /* MoreMessages */ NULL, qtInformation, qaOK);
    TWinSCPFileSystem * PanelSystem = NULL;
    PanelSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem());
    if (PanelSystem && PanelSystem->SessionList())
    {
        // PanelSystem->RedrawPanel(/* Another */ false);
        PanelSystem->UpdatePanel(/* ClearSelection */ true, /* Another */ false);
    }
    // DEBUG_PRINTF(L"end, all_imported = %d", all_imported);
    return false;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::ConfigureEx(int /*Item*/)
{
  // DEBUG_PRINTF(L"begin");
  bool Change = false;

  TFarMenuItems * MenuItems = new TFarMenuItems();
  {
      BOOST_SCOPE_EXIT ( (&MenuItems) )
      {
        delete MenuItems;
      } BOOST_SCOPE_EXIT_END
    int MInterface = MenuItems->Add(GetMsg(CONFIG_INTERFACE));
    int MConfirmations = MenuItems->Add(GetMsg(CONFIG_CONFIRMATIONS));
    int MPanel = MenuItems->Add(GetMsg(CONFIG_PANEL));
    int MTransfer = MenuItems->Add(GetMsg(CONFIG_TRANSFER));
    int MBackground = MenuItems->Add(GetMsg(CONFIG_BACKGROUND));
    int MEndurance = MenuItems->Add(GetMsg(CONFIG_ENDURANCE));
    int MTransferEditor = MenuItems->Add(GetMsg(CONFIG_TRANSFER_EDITOR));
    int MLogging = MenuItems->Add(GetMsg(CONFIG_LOGGING));
    int MIntegration = MenuItems->Add(GetMsg(CONFIG_INTEGRATION));
    MenuItems->AddSeparator();
    int MAbout = MenuItems->Add(GetMsg(CONFIG_ABOUT));

    int Result;

    do
    {
      Result = Menu(FMENU_WRAPMODE, GetMsg(PLUGIN_TITLE), L"", MenuItems);
      // DEBUG_PRINTF(L"Result = %d", Result);

      if (Result >= 0)
      {
        if (Result == MInterface)
        {
          if (ConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MTransfer)
        {
          if (TransferConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MBackground)
        {
          if (QueueConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MEndurance)
        {
          if (EnduranceConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MPanel)
        {
          if (PanelConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MTransferEditor)
        {
          if (TransferEditorConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MConfirmations)
        {
          if (ConfirmationsConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MLogging)
        {
          if (LoggingConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MIntegration)
        {
          if (IntegrationConfigurationDialog())
          {
            Change = true;
          }
        }
        else if (Result == MAbout)
        {
          AboutDialog();
        }
      }
    }
    while (Result >= 0);
  }
  // DEBUG_PRINTF(L"end");
  return Change;
}
//---------------------------------------------------------------------------
int TWinSCPPlugin::ProcessEditorEventEx(int Event, void * Param)
{
  // for performance reasons, do not pass the event to file systems on redraw
  if ((Event != EE_REDRAW) || FarConfiguration->GetEditorUploadOnSave() ||
      FarConfiguration->GetEditorMultiple())
  {
    TWinSCPFileSystem * FileSystem;
    for (size_t Index = 0; Index < FOpenedPlugins->GetCount(); Index++)
    {
      FileSystem = dynamic_cast<TWinSCPFileSystem *>(FOpenedPlugins->GetItem(Index));
      FileSystem->ProcessEditorEvent(Event, Param);
    }
  }

  return 0;
}
//---------------------------------------------------------------------------
int TWinSCPPlugin::ProcessEditorInputEx(const INPUT_RECORD * Rec)
{
  int Result;
  if ((Rec->EventType == KEY_EVENT) &&
      Rec->Event.KeyEvent.bKeyDown &&
      (Rec->Event.KeyEvent.uChar.AsciiChar == 'W') &&
      (FLAGSET(Rec->Event.KeyEvent.dwControlKeyState, LEFT_ALT_PRESSED) ||
       FLAGSET(Rec->Event.KeyEvent.dwControlKeyState, RIGHT_ALT_PRESSED)) &&
      FLAGSET(Rec->Event.KeyEvent.dwControlKeyState, SHIFT_PRESSED))
  {
    CommandsMenu(false);
    Result = 1;
  }
  else
  {
    Result = 0;
  }

  return Result;
}
//---------------------------------------------------------------------------
TCustomFarFileSystem * TWinSCPPlugin::OpenPluginEx(int OpenFrom, int Item)
{
  TWinSCPFileSystem * FileSystem = NULL;
  try
  {
    if ((OpenFrom == OPEN_PLUGINSMENU) &&
        (!FarConfiguration->GetPluginsMenu() || (Item == 1)))
    {
      CommandsMenu(true);
    }
    else
    {
      FileSystem = new TWinSCPFileSystem(this);
      FileSystem->Init(NULL);

      if (OpenFrom == OPEN_DISKMENU || OpenFrom == OPEN_PLUGINSMENU ||
          OpenFrom == OPEN_FINDLIST)
      {
        // nothing
      }
      else if (OpenFrom == OPEN_SHORTCUT || OpenFrom == OPEN_COMMANDLINE)
      {
        std::wstring Directory;
        std::wstring Name = reinterpret_cast<wchar_t *>(Item);
        if (OpenFrom == OPEN_SHORTCUT)
        {
          size_t P = Name.find(L"\1");
          if (P != std::wstring::npos)
          {
            Directory = Name.substr(P + 1, Name.size() - P);
            Name.resize(P - 1);
          }

          TWinSCPFileSystem * PanelSystem;
          PanelSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem());
          if (PanelSystem && PanelSystem->Connected() &&
              PanelSystem->GetTerminal()->GetSessionData()->GetSessionUrl() == Name)
          {
            PanelSystem->SetDirectoryEx(Directory, OPM_SILENT);
            if (PanelSystem->UpdatePanel())
            {
              PanelSystem->RedrawPanel();
            }
            Abort();
          }
          // directory will be set by FAR itself
          Directory = L"";
        }
        assert(StoredSessions);
        bool DefaultsOnly;
        TSessionData * Session = StoredSessions->ParseUrl(Name, NULL, DefaultsOnly);
        if (DefaultsOnly)
        {
          Abort();
        }
        if (!Session->GetCanLogin())
        {
          assert(false);
          Abort();
        }
        FileSystem->Connect(Session);
        if (!Directory.empty())
        {
          FileSystem->SetDirectoryEx(Directory, OPM_SILENT);
        }
      }
      else
      {
        assert(false);
      }
    }
  }
  catch (...)
  {
    delete FileSystem;
    throw;
  }

  return FileSystem;
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::CommandsMenu(bool FromFileSystem)
{
  TFarMenuItems * MenuItems = new TFarMenuItems();
  {
      BOOST_SCOPE_EXIT ( (&MenuItems) )
      {
        delete MenuItems;
      } BOOST_SCOPE_EXIT_END
    TWinSCPFileSystem * FileSystem;
    TWinSCPFileSystem * AnotherFileSystem;
    FileSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem());
    AnotherFileSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem(true));
    bool FSConnected = (FileSystem != NULL) && FileSystem->Connected();
    bool AnotherFSConnected = (AnotherFileSystem != NULL) && AnotherFileSystem->Connected();
    bool FSVisible = FSConnected && FromFileSystem;
    bool AnyFSVisible = (FSConnected || AnotherFSConnected) && FromFileSystem;

    int MAttributes = MenuItems->Add(GetMsg(MENU_COMMANDS_ATTRIBUTES), FSVisible);
    int MLink = MenuItems->Add(GetMsg(MENU_COMMANDS_LINK), FSVisible);
    int MApplyCommand = MenuItems->Add(GetMsg(MENU_COMMANDS_APPLY_COMMAND), FSVisible);
    int MFullSynchronize = MenuItems->Add(GetMsg(MENU_COMMANDS_FULL_SYNCHRONIZE), AnyFSVisible);
    int MSynchronize = MenuItems->Add(GetMsg(MENU_COMMANDS_SYNCHRONIZE), AnyFSVisible);
    int MQueue = MenuItems->Add(GetMsg(MENU_COMMANDS_QUEUE), FSVisible);
    int MInformation = MenuItems->Add(GetMsg(MENU_COMMANDS_INFORMATION), FSVisible);
    int MLog = MenuItems->Add(GetMsg(MENU_COMMANDS_LOG), FSVisible);
    int MClearCaches = MenuItems->Add(GetMsg(MENU_COMMANDS_CLEAR_CACHES), FSVisible);
    int MPutty = MenuItems->Add(GetMsg(MENU_COMMANDS_PUTTY), FSVisible);
    int MEditHistory = MenuItems->Add(GetMsg(MENU_COMMANDS_EDIT_HISTORY), FSConnected);
    MenuItems->AddSeparator(FSConnected || FSVisible);
    int MAddBookmark = MenuItems->Add(GetMsg(MENU_COMMANDS_ADD_BOOKMARK), FSVisible);
    int MOpenDirectory = MenuItems->Add(GetMsg(MENU_COMMANDS_OPEN_DIRECTORY), FSVisible);
    int MHomeDirectory = MenuItems->Add(GetMsg(MENU_COMMANDS_HOME_DIRECTORY), FSVisible);
    int MSynchronizeBrowsing = MenuItems->Add(GetMsg(MENU_COMMANDS_SYNCHRONIZE_BROWSING), FSVisible);
    MenuItems->AddSeparator(FSVisible);
    int MPageant = MenuItems->Add(GetMsg(MENU_COMMANDS_PAGEANT), FromFileSystem);
    int MPuttygen = MenuItems->Add(GetMsg(MENU_COMMANDS_PUTTYGEN), FromFileSystem);
    MenuItems->AddSeparator(FromFileSystem);
    int MImportSessions = MenuItems->Add(GetMsg(MENU_COMMANDS_IMPORT_SESSIONS));
    int MConfigure = MenuItems->Add(GetMsg(MENU_COMMANDS_CONFIGURE));
    int MAbout = MenuItems->Add(GetMsg(CONFIG_ABOUT));

    MenuItems->SetDisabled(MLog, !FSVisible || (FileSystem && !FileSystem->IsLogging()));
    MenuItems->SetDisabled(MClearCaches, !FSVisible || (FileSystem && FileSystem->AreCachesEmpty()));
    MenuItems->SetDisabled(MPutty, !FSVisible || !FileExistsEx(ExpandEnvironmentVariables(ExtractProgram(FarConfiguration->GetPuttyPath()))));
    MenuItems->SetDisabled(MEditHistory, !FSConnected || (FileSystem && FileSystem->IsEditHistoryEmpty()));
    MenuItems->SetChecked(MSynchronizeBrowsing, FSVisible && (FileSystem && FileSystem->IsSynchronizedBrowsing()));
    MenuItems->SetDisabled(MPageant, !FileExistsEx(ExpandEnvironmentVariables(ExtractProgram(FarConfiguration->GetPageantPath()))));
    MenuItems->SetDisabled(MPuttygen, !FileExistsEx(ExpandEnvironmentVariables(ExtractProgram(FarConfiguration->GetPuttygenPath()))));

    int Result = Menu(FMENU_WRAPMODE, GetMsg(MENU_COMMANDS), L"", MenuItems);

    if (Result >= 0)
    {
      if ((Result == MLog) && FileSystem)
      {
        FileSystem->ShowLog();
      }
      else if ((Result == MAttributes) && FileSystem)
      {
        FileSystem->FileProperties();
      }
      else if ((Result == MLink) && FileSystem)
      {
        FileSystem->CreateLink();
      }
      else if ((Result == MApplyCommand) && FileSystem)
      {
        FileSystem->ApplyCommand();
      }
      else if (Result == MFullSynchronize)
      {
        if (FileSystem != NULL)
        {
          FileSystem->FullSynchronize(true);
        }
        else
        {
          assert(AnotherFileSystem != NULL);
          AnotherFileSystem->FullSynchronize(false);
        }
      }
      else if (Result == MSynchronize)
      {
        if (FileSystem != NULL)
        {
          FileSystem->Synchronize();
        }
        else
        {
          assert(AnotherFileSystem != NULL);
          AnotherFileSystem->Synchronize();
        }
      }
      else if ((Result == MQueue) && FileSystem)
      {
        FileSystem->QueueShow(false);
      }
      else if ((Result == MAddBookmark || Result == MOpenDirectory) && FileSystem)
      {
        FileSystem->OpenDirectory(Result == MAddBookmark);
      }
      else if (Result == MHomeDirectory && FileSystem)
      {
        FileSystem->HomeDirectory();
      }
      else if (Result == MImportSessions)
      {
        ImportSessions();
      }
      else if (Result == MConfigure)
      {
        ConfigureEx(0);
      }
      else if (Result == MAbout)
      {
        AboutDialog();
      }
      else if ((Result == MPutty) && FileSystem)
      {
        FileSystem->OpenSessionInPutty();
      }
      else if ((Result == MEditHistory) && FileSystem)
      {
        FileSystem->EditHistory();
      }
      else if (Result == MPageant || Result == MPuttygen)
      {
        std::wstring Path = (Result == MPageant) ?
          FarConfiguration->GetPageantPath() : FarConfiguration->GetPuttygenPath();
        std::wstring Program, Params, Dir;
        SplitCommand(Path, Program, Params, Dir);
        ExecuteShell(Program, Params);
      }
      else if ((Result == MClearCaches) && FileSystem)
      {
        FileSystem->ClearCaches();
      }
      else if ((Result == MSynchronizeBrowsing) && FileSystem)
      {
        FileSystem->ToggleSynchronizeBrowsing();
      }
      else if ((Result == MInformation) && FileSystem)
      {
        FileSystem->ShowInformation();
      }
      else
      {
        assert(false);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::ShowExtendedException(const std::exception * E)
{
  // if (strlen(E->what()) > 0)
  {
    if (::InheritsFrom<std::exception, std::exception>(E))
    {
      if (!::InheritsFrom<std::exception, EAbort>(E))
      {
        TQueryType Type;
        Type = (::InheritsFrom<std::exception, ESshTerminate>(E)) ?
          qtInformation : qtError;

        TStrings *MoreMessages = NULL;
        if (::InheritsFrom<std::exception, ExtException>(E))
        {
          MoreMessages = dynamic_cast<const ExtException *>(E)->GetMoreMessages();
        }

        std::wstring Message = ::TranslateExceptionMessage(E);
        MoreMessageDialog(Message, MoreMessages, Type, qaOK);
      }
    }
    else
    {
      // ShowException(ExceptObject(), ExceptAddr());
      DEBUG_PRINTF(L"ShowException");
    }
  }
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::OldFar()
{
  throw ExtException(FORMAT(GetMsg(OLD_FAR).c_str(), FormatFarVersion(GetMinFarVersion()).c_str()));
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::HandleException(const std::exception * E, int OpMode)
{
  if (((OpMode & OPM_FIND) == 0) || ::InheritsFrom<std::exception, EFatal>(E))
  {
    ShowExtendedException(E);
  }
}
//---------------------------------------------------------------------------
struct TFarMessageData
{
  TFarMessageData()
  {
    Params = NULL;
    ButtonCount = 0;
  }

  const TMessageParams * Params;
  int Buttons[15 + 1];
  int ButtonCount;
};
//---------------------------------------------------------------------------
void TWinSCPPlugin::MessageClick(void * Token, int Result, bool & Close)
{
  TFarMessageData & Data = *static_cast<TFarMessageData *>(Token);

  assert(Result >= 0 && Result < Data.ButtonCount);

  if ((Data.Params != NULL) && (Data.Params->Aliases != NULL))
  {
    for (unsigned int i = 0; i < Data.Params->AliasesCount; i++)
    {
      if ((static_cast<int>(Data.Params->Aliases[i].Button) == Data.Buttons[Result]) &&
          (!Data.Params->Aliases[i].OnClick.empty()))
      {
        Data.Params->Aliases[i].OnClick(NULL);
        Close = false;
        break;
      }
    }
  }
}
//---------------------------------------------------------------------------
int TWinSCPPlugin::MoreMessageDialog(const std::wstring &Str,
  TStrings * MoreMessages, TQueryType Type, int Answers,
  const TMessageParams * Params)
{
  int Result;
  std::wstring str = Str;
  TStrings * ButtonLabels = new TStringList();
  {
      BOOST_SCOPE_EXIT ( (&ButtonLabels) )
      {
        delete ButtonLabels;
      } BOOST_SCOPE_EXIT_END
    unsigned int Flags = 0;

    if (Params != NULL)
    {
      Flags = Params->Flags;
    }

    int TitleId = 0;
    switch (Type) {
      case qtConfirmation: TitleId = MSG_TITLE_CONFIRMATION; break;
      case qtInformation: TitleId = MSG_TITLE_INFORMATION; break;
      case qtError: TitleId = MSG_TITLE_ERROR; Flags |= FMSG_WARNING; break;
      case qtWarning: TitleId = MSG_TITLE_WARNING; Flags |= FMSG_WARNING; break;
      default: assert(false);
    }
    TFarMessageData Data;
    Data.Params = Params;

    // make sure to do the check on full answers, not on reduced "timer answers"
    if (((Answers & qaAbort) && (Answers & qaRetry)) ||
        (GetTopDialog() != NULL))
    {
      // use warning colors for abort/retry confirmation dialog
      Flags |= FMSG_WARNING;
    }

    if (Params != NULL)
    {
      if (Params->Timer > 0)
      {
        if (Params->TimerAnswers > 0)
        {
          Answers = Params->TimerAnswers;
        }
        if (!Params->TimerMessage.empty())
        {
          str = Params->TimerMessage;
        }
      }
    }

    int AAnswers = Answers;
    bool NeverAskAgainCheck = (Params != NULL) && FLAGSET(Params->Params, qpNeverAskAgainCheck);
    bool NeverAskAgainPending = NeverAskAgainCheck;
    int TimeoutButton = 0;

    #define ADD_BUTTON_EX(TYPE, CANNEVERASK) \
      if (AAnswers & qa ## TYPE) \
      { \
        ButtonLabels->Add(GetMsg(MSG_BUTTON_ ## TYPE)); \
        Data.Buttons[Data.ButtonCount] = qa ## TYPE; \
        Data.ButtonCount++; \
        AAnswers -= qa ## TYPE; \
        if ((Params != NULL) && (Params->Timeout != 0) && \
            (Params->TimeoutAnswer == qa ## TYPE)) \
        { \
          TimeoutButton = ButtonLabels->GetCount() - 1; \
        } \
        if (NeverAskAgainPending && CANNEVERASK) \
        { \
          ButtonLabels->PutObject(ButtonLabels->GetCount() - 1, (TObject*)true); \
          NeverAskAgainPending = false; \
        } \
      }
    #define ADD_BUTTON(TYPE) ADD_BUTTON_EX(TYPE, false)
    #pragma warning(push)
    #pragma warning(disable: 4127)
    ADD_BUTTON_EX(Yes, true);
    ADD_BUTTON(No);
    ADD_BUTTON_EX(OK, true);
    ADD_BUTTON(Cancel);
    ADD_BUTTON(Abort);
    ADD_BUTTON(Retry);
    ADD_BUTTON(Ignore);
    ADD_BUTTON(Skip);
    ADD_BUTTON(All);
    ADD_BUTTON(NoToAll);
    ADD_BUTTON_EX(YesToAll, true);
    ADD_BUTTON(Help);
    #pragma warning(pop)
    #undef ADD_BUTTON
    #undef ADD_BUTTON_EX

    USEDPARAM(AAnswers);
    assert(!AAnswers);
    USEDPARAM(NeverAskAgainPending);
    assert(!NeverAskAgainPending);

    if ((Params != NULL) && (Params->Aliases != NULL))
    {
      for (int bi = 0; bi < Data.ButtonCount; bi++)
      {
        for (unsigned int ai = 0; ai < Params->AliasesCount; ai++)
        {
          if (static_cast<int>(Params->Aliases[ai].Button) == Data.Buttons[bi])
          {
            ButtonLabels->PutString(bi, Params->Aliases[ai].Alias);
            break;
          }
        }
      }
    }

    #define MORE_BUTTON_ID -2
    TFarMessageParams FarParams;

    if (NeverAskAgainCheck)
    {
      FarParams.CheckBoxLabel =
        (Answers == qaOK) ? GetMsg(MSG_CHECK_NEVER_SHOW_AGAIN) :
          GetMsg(MSG_CHECK_NEVER_ASK_AGAIN);
    }

    if (Params != NULL)
    {
      if (Params->Timer > 0)
      {
        FarParams.Timer = Params->Timer;
        FarParams.TimerEvent = Params->TimerEvent;
      }

      if (Params->Timeout > 0)
      {
        FarParams.Timeout = Params->Timeout;
        FarParams.TimeoutButton = TimeoutButton;
        FarParams.TimeoutStr = GetMsg(MSG_BUTTON_TIMEOUT);
      }
    }

    FarParams.Token = &Data;
    farmessageclick_slot_type slot = boost::bind(&TWinSCPPlugin::MessageClick, this, _1, _2, _3);
    FarParams.ClickEvent = &slot;

    std::wstring DialogStr = str;
    if (MoreMessages && (MoreMessages->GetCount() > 0))
    {
      FarParams.MoreMessages = MoreMessages;
    }
    else
    {
      FarParams.MoreMessages = NULL;
    }

    Result = Message(Flags, GetMsg(TitleId), DialogStr, ButtonLabels, &FarParams);
    if (FarParams.TimerAnswer > 0)
    {
      Result = FarParams.TimerAnswer;
    }
    else if (Result < 0)
    {
      Result = CancelAnswer(Answers);
    }
    else
    {
      assert(Result >= 0 && Result < Data.ButtonCount);
      Result = Data.Buttons[Result];
    }

    if (FarParams.CheckBox)
    {
      assert(NeverAskAgainCheck);
      Result = qaNeverAskAgain;
    }
  }
  return Result;
}
