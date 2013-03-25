//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <CoreMain.h>
#include <Exceptions.h>
#include <Terminal.h>
#include <GUITools.h>
#include <ProgParams.h>
#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarConfiguration.h"
#include "FarTexts.h"
#include "FarDialog.h"
#include "plugin.hpp"
#include "XmlStorage.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
  TCustomFarPlugin(HInst),
  FInitialized(false)
{
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
intptr_t TWinSCPPlugin::GetMinFarVersion()
{
  return MAKEFARVERSION(2, 0, 1667);
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    TCustomFarPlugin::SetStartupInfo(Info);
  }
  catch(Exception & E)
  {
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::GetPluginInfoEx(DWORD & Flags,
  TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
  TStrings * PluginConfigStrings, TStrings * CommandPrefixes)
{
  Flags = PF_FULLCMDLINE;
  if (FarConfiguration->GetDisksMenu())
  {
    DiskMenuStrings->AddObject(GetMsg(PLUGIN_NAME),
      reinterpret_cast<TObject *>((size_t)FarConfiguration->GetDisksMenuHotKey()));
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
bool TWinSCPPlugin::ConfigureEx(intptr_t /*Item*/)
{
  bool Change = false;

  TFarMenuItems * MenuItems = new TFarMenuItems();
  {
    std::auto_ptr<TFarMenuItems> MenuItemsPtr;
    MenuItemsPtr.reset(MenuItems);
    intptr_t MInterface = MenuItems->Add(GetMsg(CONFIG_INTERFACE));
    intptr_t MConfirmations = MenuItems->Add(GetMsg(CONFIG_CONFIRMATIONS));
    intptr_t MPanel = MenuItems->Add(GetMsg(CONFIG_PANEL));
    intptr_t MTransfer = MenuItems->Add(GetMsg(CONFIG_TRANSFER));
    intptr_t MBackground = MenuItems->Add(GetMsg(CONFIG_BACKGROUND));
    intptr_t MEndurance = MenuItems->Add(GetMsg(CONFIG_ENDURANCE));
    intptr_t MTransferEditor = MenuItems->Add(GetMsg(CONFIG_TRANSFER_EDITOR));
    intptr_t MLogging = MenuItems->Add(GetMsg(CONFIG_LOGGING));
    intptr_t MIntegration = MenuItems->Add(GetMsg(CONFIG_INTEGRATION));
    MenuItems->AddSeparator();
    intptr_t MAbout = MenuItems->Add(GetMsg(CONFIG_ABOUT));

    intptr_t Result = 0;

    do
    {
      Result = Menu(FMENU_WRAPMODE, GetMsg(PLUGIN_TITLE), L"", MenuItems);

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

  return Change;
}
//---------------------------------------------------------------------------
intptr_t TWinSCPPlugin::ProcessEditorEventEx(intptr_t Event, void * Param)
{
  // for performance reasons, do not pass the event to file systems on redraw
  if ((Event != EE_REDRAW) || FarConfiguration->GetEditorUploadOnSave() ||
      FarConfiguration->GetEditorMultiple())
  {
    TWinSCPFileSystem * FileSystem = NULL;
    for (intptr_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
    {
      FileSystem = dynamic_cast<TWinSCPFileSystem *>(FOpenedPlugins->GetItem(Index));
      FileSystem->ProcessEditorEvent(Event, Param);
    }
  }

  return 0;
}
//---------------------------------------------------------------------------
intptr_t TWinSCPPlugin::ProcessEditorInputEx(const INPUT_RECORD * Rec)
{
  intptr_t Result;
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
TCustomFarFileSystem * TWinSCPPlugin::OpenPluginEx(intptr_t OpenFrom, intptr_t Item)
{
  TWinSCPFileSystem * FileSystem = NULL;
  try
  {
    if (!FInitialized)
    {
      CoreInitialize();
      CleanupConfiguration();
      FInitialized = true;
    }

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
        UnicodeString Directory;
        UnicodeString CommandLine = reinterpret_cast<wchar_t *>(Item);
        if (OpenFrom == OPEN_SHORTCUT)
        {
          intptr_t P = CommandLine.Pos(L"\1");
          if (P > 0)
          {
            Directory = CommandLine.SubString(P + 1, CommandLine.Length() - P);
            CommandLine.SetLength(P - 1);
          }

          TWinSCPFileSystem * PanelSystem;
          PanelSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem());
          if (PanelSystem && PanelSystem->Connected() &&
              PanelSystem->GetTerminal()->GetSessionData()->GetSessionUrl() == CommandLine)
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
        bool DefaultsOnly = false;
        TOptions * Options = new TProgramParams();
        ParseCommandLine(CommandLine, Options);
        TSessionData * Session = StoredSessions->ParseUrl(CommandLine, Options, DefaultsOnly);
        TRY_FINALLY (
        {
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
          if (!Directory.IsEmpty())
          {
            FileSystem->SetDirectoryEx(Directory, OPM_SILENT);
          }
        }
        ,
        {
          delete Options;
          delete Session;
        }
        );
      }
      else if (OpenFrom == OPEN_ANALYSE)
      {
        const wchar_t * XmlFileName = reinterpret_cast<const wchar_t *>(Item);
        TSessionData * Session = NULL;
        THierarchicalStorage * ImportStorage = NULL;
        TRY_FINALLY (
        {
          ImportStorage = new TXmlStorage(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey());
          ImportStorage->Init();
          ImportStorage->SetAccessMode(smRead);
          if (!(ImportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false) &&
                ImportStorage->HasSubKeys()))
          {
            assert(false);
            Abort();
          }
          UnicodeString SessionName = ::PuttyUnMungeStr(ImportStorage->ReadStringRaw(L"Session", L""));
          Session = new TSessionData(SessionName);
          Session->Load(ImportStorage);
          Session->SetModified(true);
          if (!Session->GetCanLogin())
          {
            assert(false);
            Abort();
          }
          FileSystem->Connect(Session);
        }
        ,
        {
          delete ImportStorage;
          delete Session;
        }
        );
      }
      else
      {
        assert(false);
      }
    }
  }
  catch(...)
  {
    delete FileSystem;
    throw;
  }

  return FileSystem;
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::ParseCommandLine(UnicodeString & CommandLine,
  TOptions * Options)
{
  // UnicodeString CommandLineParams;
  UnicodeString CmdLine = CommandLine;
  // intptr_t Pos = FirstDelimiter(Opt->GetSwitchMarks(), CmdLine);
  intptr_t Index = 1;
  // Skip session name
  {
    while ((Index < CmdLine.Length()) && (CmdLine[Index] == L' '))
     ++Index;
    if (CmdLine[Index] == L'"')
    {
      ++Index;
      while ((Index < CmdLine.Length()) && (CmdLine[Index] != L'"'))
        ++Index;
      ++Index;
    }
    while ((Index < CmdLine.Length()) && (CmdLine[Index] != L' '))
      ++Index;
  }
  CmdLine = CmdLine.SubString(Index, -1);
  // Parse params
  intptr_t Pos = FirstDelimiter(Options->GetSwitchMarks(), CmdLine);
  UnicodeString CommandLineParams;
  if (Pos > 0)
    CommandLineParams = CmdLine.SubString(Pos, -1);
  // DEBUG_PRINTF(L"CommandLineParams = %s", CommandLineParams.c_str());
  if (!CommandLineParams.IsEmpty())
  {
    Options->ParseParams(CommandLineParams);
    CommandLine = CommandLine.SubString(1, CommandLine.Length() - CommandLineParams.Length()).Trim();
  }
}
//---------------------------------------------------------------------------
void TWinSCPPlugin::CommandsMenu(bool FromFileSystem)
{
  TFarMenuItems * MenuItems = new TFarMenuItems();
  {
    std::auto_ptr<TFarMenuItems> MenuItemsPtr;
    MenuItemsPtr.reset(MenuItems);
    TWinSCPFileSystem * FileSystem;
    TWinSCPFileSystem * AnotherFileSystem;
    FileSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem());
    AnotherFileSystem = dynamic_cast<TWinSCPFileSystem *>(GetPanelFileSystem(true));
    bool FSConnected = (FileSystem != NULL) && FileSystem->Connected();
    bool AnotherFSConnected = (AnotherFileSystem != NULL) && AnotherFileSystem->Connected();
    bool FSVisible = FSConnected && FromFileSystem;
    bool AnyFSVisible = (FSConnected || AnotherFSConnected) && FromFileSystem;

    intptr_t MAttributes = MenuItems->Add(GetMsg(MENU_COMMANDS_ATTRIBUTES), FSVisible);
    intptr_t MLink = MenuItems->Add(GetMsg(MENU_COMMANDS_LINK), FSVisible);
    intptr_t MApplyCommand = MenuItems->Add(GetMsg(MENU_COMMANDS_APPLY_COMMAND), FSVisible);
    intptr_t MFullSynchronize = MenuItems->Add(GetMsg(MENU_COMMANDS_FULL_SYNCHRONIZE), AnyFSVisible);
    intptr_t MSynchronize = MenuItems->Add(GetMsg(MENU_COMMANDS_SYNCHRONIZE), AnyFSVisible);
    intptr_t MQueue = MenuItems->Add(GetMsg(MENU_COMMANDS_QUEUE), FSVisible);
    intptr_t MInformation = MenuItems->Add(GetMsg(MENU_COMMANDS_INFORMATION), FSVisible);
    intptr_t MLog = MenuItems->Add(GetMsg(MENU_COMMANDS_LOG), FSVisible);
    intptr_t MClearCaches = MenuItems->Add(GetMsg(MENU_COMMANDS_CLEAR_CACHES), FSVisible);
    intptr_t MPutty = MenuItems->Add(GetMsg(MENU_COMMANDS_PUTTY), FSVisible);
    intptr_t MEditHistory = MenuItems->Add(GetMsg(MENU_COMMANDS_EDIT_HISTORY), FSConnected);
    MenuItems->AddSeparator(FSConnected || FSVisible);
    intptr_t MAddBookmark = MenuItems->Add(GetMsg(MENU_COMMANDS_ADD_BOOKMARK), FSVisible);
    intptr_t MOpenDirectory = MenuItems->Add(GetMsg(MENU_COMMANDS_OPEN_DIRECTORY), FSVisible);
    intptr_t MHomeDirectory = MenuItems->Add(GetMsg(MENU_COMMANDS_HOME_DIRECTORY), FSVisible);
    intptr_t MSynchronizeBrowsing = MenuItems->Add(GetMsg(MENU_COMMANDS_SYNCHRONIZE_BROWSING), FSVisible);
    MenuItems->AddSeparator(FSVisible);
    intptr_t MPageant = MenuItems->Add(GetMsg(MENU_COMMANDS_PAGEANT), FromFileSystem);
    intptr_t MPuttygen = MenuItems->Add(GetMsg(MENU_COMMANDS_PUTTYGEN), FromFileSystem);
    MenuItems->AddSeparator(FromFileSystem);
    intptr_t MConfigure = MenuItems->Add(GetMsg(MENU_COMMANDS_CONFIGURE));
    intptr_t MAbout = MenuItems->Add(GetMsg(CONFIG_ABOUT));

    MenuItems->SetDisabled(MLog, !FSVisible || (FileSystem && !FileSystem->IsLogging()));
    MenuItems->SetDisabled(MClearCaches, !FSVisible || (FileSystem && FileSystem->AreCachesEmpty()));
    MenuItems->SetDisabled(MPutty, !FSVisible || !FileExistsEx(ExpandEnvironmentVariables(ExtractProgram(FarConfiguration->GetPuttyPath()))));
    MenuItems->SetDisabled(MEditHistory, !FSConnected || (FileSystem && FileSystem->IsEditHistoryEmpty()));
    MenuItems->SetChecked(MSynchronizeBrowsing, FSVisible && (FileSystem && FileSystem->IsSynchronizedBrowsing()));
    MenuItems->SetDisabled(MPageant, !FileExistsEx(ExpandEnvironmentVariables(ExtractProgram(FarConfiguration->GetPageantPath()))));
    MenuItems->SetDisabled(MPuttygen, !FileExistsEx(ExpandEnvironmentVariables(ExtractProgram(FarConfiguration->GetPuttygenPath()))));

    intptr_t Result = Menu(FMENU_WRAPMODE, GetMsg(MENU_COMMANDS), L"", MenuItems);

    if (Result >= 0)
    {
      if ((Result == MLog) && FileSystem)
      {
        assert(FileSystem);
        FileSystem->ShowLog();
      }
      else if ((Result == MAttributes) && FileSystem)
      {
        assert(FileSystem);
        FileSystem->FileProperties();
      }
      else if ((Result == MLink) && FileSystem)
      {
        assert(FileSystem);
        FileSystem->CreateLink();
      }
      else if ((Result == MApplyCommand) && FileSystem)
      {
        assert(FileSystem);
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
        assert(FileSystem);
        FileSystem->QueueShow(false);
      }
      else if ((Result == MAddBookmark || Result == MOpenDirectory) && FileSystem)
      {
        assert(FileSystem);
        FileSystem->OpenDirectory(Result == MAddBookmark);
      }
      else if (Result == MHomeDirectory && FileSystem)
      {
        FileSystem->HomeDirectory();
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
        assert(FileSystem);
        FileSystem->OpenSessionInPutty();
      }
      else if ((Result == MEditHistory) && FileSystem)
      {
        assert(FileSystem);
        FileSystem->EditHistory();
      }
      else if (Result == MPageant || Result == MPuttygen)
      {
        UnicodeString Path = (Result == MPageant) ?
          FarConfiguration->GetPageantPath() : FarConfiguration->GetPuttygenPath();
        UnicodeString Program, Params, Dir;
        SplitCommand(ExpandEnvironmentVariables(Path), Program, Params, Dir);
        ExecuteShell(Program, Params);
      }
      else if ((Result == MClearCaches) && FileSystem)
      {
        assert(FileSystem);
        FileSystem->ClearCaches();
      }
      else if ((Result == MSynchronizeBrowsing) && FileSystem)
      {
        assert(FileSystem != NULL);
        FileSystem->ToggleSynchronizeBrowsing();
      }
      else if (Result == MInformation && FileSystem)
      {
        assert(FileSystem);
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
void TWinSCPPlugin::ShowExtendedException(Exception * E)
{
  if (!E->Message.IsEmpty())
  {
    if (E->InheritsFrom<Exception>())
    {
      if (!E->InheritsFrom<EAbort>())
      {
        TQueryType Type;
        Type = (E->InheritsFrom<ESshTerminate>()) ?
          qtInformation : qtError;

        TStrings * MoreMessages = NULL;
        if (E->InheritsFrom<ExtException>())
        {
          MoreMessages = dynamic_cast<ExtException *>(E)->GetMoreMessages();
        }

        UnicodeString Message = TranslateExceptionMessage(E);
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
void TWinSCPPlugin::HandleException(Exception * E, int OpMode)
{
  if (((OpMode & OPM_FIND) == 0) || ::InheritsFrom<std::exception, EFatal>(E))
  {
    ShowExtendedException(E);
  }
}
//---------------------------------------------------------------------------
struct TFarMessageData : public TObject
{
  TFarMessageData()
  {
    Params = NULL;
    memset(Buttons, 0, sizeof(Buttons));
    ButtonCount = 0;
  }

  const TMessageParams * Params;
  uintptr_t Buttons[15 + 1];
  uintptr_t ButtonCount;
};
//---------------------------------------------------------------------------
void TWinSCPPlugin::MessageClick(void * Token, uintptr_t Result, bool & Close)
{
  TFarMessageData & Data = *static_cast<TFarMessageData *>(Token);

  assert(Result != -1 && Result < Data.ButtonCount);

  if ((Data.Params != NULL) && (Data.Params->Aliases != NULL))
  {
    for (uintptr_t I = 0; I < Data.Params->AliasesCount; I++)
    {
      if ((Data.Params->Aliases[I].Button == Data.Buttons[Result]) &&
          (Data.Params->Aliases[I].OnClick))
      {
        Data.Params->Aliases[I].OnClick(NULL);
        Close = false;
        break;
      }
    }
  }
}
//---------------------------------------------------------------------------
uintptr_t TWinSCPPlugin::MoreMessageDialog(const UnicodeString & Str,
  TStrings * MoreMessages, TQueryType Type, uintptr_t Answers,
  const TMessageParams * Params)
{
  uintptr_t Result = 0;
  UnicodeString DialogStr = Str;
  TStrings * ButtonLabels = new TStringList();
  {
    std::auto_ptr<TStrings> ButtonLabelsPtr;
    ButtonLabelsPtr.reset(ButtonLabels);
    uintptr_t Flags = 0;

    if (Params != NULL)
    {
      Flags = Params->Flags;
    }

    intptr_t TitleId = 0;
    switch (Type)
    {
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
        if (!Params->TimerMessage.IsEmpty())
        {
          DialogStr = Params->TimerMessage;
        }
      }
    }

    uintptr_t AAnswers = Answers;
    bool NeverAskAgainCheck = (Params != NULL) && FLAGSET(Params->Params, qpNeverAskAgainCheck);
    bool NeverAskAgainPending = NeverAskAgainCheck;
    uintptr_t TimeoutButton = 0;

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
          ButtonLabels->Objects(ButtonLabels->GetCount() - 1, reinterpret_cast<TObject *>((size_t)true)); \
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
      for (uintptr_t bi = 0; bi < Data.ButtonCount; bi++)
      {
        for (uintptr_t ai = 0; ai < Params->AliasesCount; ai++)
        {
          if (Params->Aliases[ai].Button == Data.Buttons[bi])
          {
            ButtonLabels->SetString(bi, Params->Aliases[ai].Alias);
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
        FarParams.Timer = static_cast<unsigned int>(Params->Timer);
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
    FarParams.ClickEvent = MAKE_CALLBACK(TWinSCPPlugin::MessageClick, this);

    if (MoreMessages && (MoreMessages->GetCount() > 0))
    {
      FarParams.MoreMessages = MoreMessages;
    }
    else
    {
      FarParams.MoreMessages = NULL;
    }

    Result = Message(static_cast<DWORD>(Flags), GetMsg(TitleId), DialogStr, ButtonLabels, &FarParams);
    if (FarParams.TimerAnswer > 0)
    {
      Result = FarParams.TimerAnswer;
    }
    else if (Result == NPOS)
    {
      Result = CancelAnswer(Answers);
    }
    else
    {
      assert(Result != -1 && Result < Data.ButtonCount);
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
//---------------------------------------------------------------------------
void TWinSCPPlugin::CleanupConfiguration()
{
  // Check if key Configuration\Version exists
  THierarchicalStorage * Storage = FarConfiguration->CreateScpStorage(false);
  TRY_FINALLY (
  {
    Storage->SetAccessMode(smReadWrite);
    if (Storage->OpenSubKey(FarConfiguration->GetConfigurationSubKey(), false))
    {
      if (!Storage->ValueExists(L"Version"))
      {
        Storage->DeleteSubKey(L"CDCache");
      }
      else
      {
        UnicodeString Version = Storage->ReadString(L"Version", L"");
        if (::StrToVersionNumber(Version) < MAKEVERSIONNUMBER(2,1,19))
        {
          Storage->DeleteSubKey(L"CDCache");
        }
      }
      Storage->WriteStringRaw(L"Version", ::VersionNumberToStr(::GetCurrentVersionNumber()));
      Storage->CloseSubKey();
    }
  }
  ,
  {
    delete Storage;
  }
  );
}
//---------------------------------------------------------------------------
