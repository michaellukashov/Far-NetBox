#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <CoreMain.h>
#include <Exceptions.h>
#include <Terminal.h>
#include <GUITools.h>
#include <ProgParams.h>
#include <MsgIDs.h>
#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarConfiguration.h"
#include "XmlStorage.h"

TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst)
{
  TCustomFarPlugin * Result = new TWinSCPPlugin(HInst);
  Result->Initialize();
  return Result;
}

void DestroyFarPlugin(TCustomFarPlugin *& Plugin)
{
  DebugAssert(FarPlugin);
  Plugin->Finalize();
  SAFE_DESTROY(Plugin);
}

static UnicodeString GetDbgPath(const char * Env) noexcept
{
  const char * Path = getenv(Env);
  if (Path)
  {
    UnicodeString Str;
    if (*Path == '~')
    {
      const char * Home = getenv("HOME");
      if (Home)
        Str = Home;
      else
      {
        const char * Temp = getenv("TEMP");
        if (Temp)
          Str = Temp;
      }
      Str += Path + 1;
    } else {
      Str = Path;
    }

    const UnicodeString DbgLogFileName = StripPathQuotes(::ExpandEnvironmentVariables(Str));
    return DbgLogFileName;
  }

  return UnicodeString();
}

TWinSCPPlugin::TWinSCPPlugin(HINSTANCE HInst) noexcept :
  TCustomFarPlugin(OBJECT_CLASS_TWinSCPPlugin, HInst)
{
#ifndef NDEBUG
  // setup debug handlers
  const UnicodeString DbgFileName = GetDbgPath("NETBOX_DBG");
  GetGlobals()->SetupDbgHandles(DbgFileName);
  // setup tinylog
  g_tinylog->level(tinylog::Utils::LEVEL_TRACE); // TODO: read from config file
  FILE * LogFile = base::LocalOpenFileForWriting("%TEMP%/netbox-dbglog.txt"); // TODO: read from config file
  if (LogFile)
  {
    g_tinylog->file(LogFile);
  }
  // TODO: icecream::ic.output(logFile);
  // IC();
#endif //ifndef NDEBUG
}

TWinSCPPlugin::~TWinSCPPlugin() noexcept
{
  // DEBUG_PRINTF("begin");
  if (FInitialized)
  {
    // GetFarConfiguration()->SetPlugin(nullptr);
    CoreFinalize();
    FInitialized = false;
  }
#ifndef NDEBUG
  g_tinylog->Close();
  SAFE_DESTROY_EX(tinylog::TinyLog, g_tinylog);
#endif //ifndef NDEBUG
  // DEBUG_PRINTF("begin");
}

bool TWinSCPPlugin::HandlesFunction(THandlesFunction Function) const
{
  return (Function == hfProcessKey || Function == hfProcessPanelEvent);
}

void TWinSCPPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    TCustomFarPlugin::SetStartupInfo(Info);
  }
  catch (Exception &E)
  {
    HandleException(&E);
  }
}

void TWinSCPPlugin::GetPluginInfoEx(PLUGIN_FLAGS & Flags,
  TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
  TStrings * PluginConfigStrings, TStrings * CommandPrefixes)
{
  CoreInitializeOnce();
  Flags = PF_FULLCMDLINE;
  const TFarConfiguration * FarConfiguration = GetFarConfiguration();
  if (FarConfiguration->GetDisksMenu())
  {
    DiskMenuStrings->AddObject(GetMsg(NB_PLUGIN_NAME),
      ToObj(nb::ToPtr(&DisksMenuGuid)));
  }
  if (FarConfiguration->GetPluginsMenu())
  {
    PluginMenuStrings->AddObject(GetMsg(NB_PLUGIN_NAME),
      ToObj(nb::ToPtr(&MenuGuid)));
  }
  if (FarConfiguration->GetPluginsMenuCommands())
  {
    PluginMenuStrings->AddObject(GetMsg(NB_MENU_COMMANDS),
      ToObj(nb::ToPtr(&MenuCommandsGuid)));
  }
  PluginConfigStrings->AddObject(GetMsg(NB_PLUGIN_NAME),
    ToObj(nb::ToPtr(&PluginConfigGuid)));
  CommandPrefixes->SetCommaText(FarConfiguration->GetCommandPrefixes());
}

bool TWinSCPPlugin::ConfigureEx(const GUID * /* Item */)
{
  bool Change = false;

  std::unique_ptr<TFarMenuItems> MenuItems(std::make_unique<TFarMenuItems>());
  const int32_t MInterface = MenuItems->AddString(GetMsg(NB_CONFIG_INTERFACE));
  const int32_t MConfirmations = MenuItems->AddString(GetMsg(NB_CONFIG_CONFIRMATIONS));
  const int32_t MPanel = MenuItems->AddString(GetMsg(NB_CONFIG_PANEL));
  const int32_t MTransfer = MenuItems->AddString(GetMsg(NB_CONFIG_TRANSFER));
  const int32_t MBackground = MenuItems->AddString(GetMsg(NB_CONFIG_BACKGROUND));
  const int32_t MEndurance = MenuItems->AddString(GetMsg(NB_CONFIG_ENDURANCE));
  const int32_t MTransferEditor = MenuItems->AddString(GetMsg(NB_CONFIG_TRANSFER_EDITOR));
  const int32_t MLogging = MenuItems->AddString(GetMsg(NB_CONFIG_LOGGING));
  const int32_t MIntegration = MenuItems->AddString(GetMsg(NB_CONFIG_INTEGRATION));
  MenuItems->AddSeparator();
  const int32_t MAbout = MenuItems->AddString(GetMsg(NB_CONFIG_ABOUT));

  intptr_t Result;

  do
  {
    Result = Menu(FMENU_WRAPMODE, GetMsg(NB_PLUGIN_TITLE), "", MenuItems.get());

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

    if (Change)
    {
      // only modified, implicit
      GetConfiguration()->DoSave(false, false);
    }
  }
  while (Result >= 0);

  return Change;
}

int32_t TWinSCPPlugin::ProcessEditorEventEx(const struct ProcessEditorEventInfo * Info)
{
  // for performance reasons, do not pass the event to file systems on redraw
  if ((Info->Event != EE_REDRAW) || GetFarConfiguration()->GetEditorUploadOnSave() ||
    GetFarConfiguration()->GetEditorMultiple())
  {
    for (int32_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
    {
      TWinSCPFileSystem * FileSystem = FOpenedPlugins->GetAs<TWinSCPFileSystem>(Index);
      Ensures(FileSystem);
      FileSystem->ProcessEditorEvent(Info->Event, Info->Param);
    }
  }

  return 0;
}

int32_t TWinSCPPlugin::ProcessEditorInputEx(const INPUT_RECORD * Rec)
{
  int32_t Result = 0;
  if ((Rec->EventType == KEY_EVENT) &&
    Rec->Event.KeyEvent.bKeyDown &&
    (Rec->Event.KeyEvent.uChar.AsciiChar == 'W') &&
    (FLAGSET(Rec->Event.KeyEvent.dwControlKeyState, ALTMASK)) &&
    (FLAGSET(Rec->Event.KeyEvent.dwControlKeyState, SHIFTMASK)))
  {
    CommandsMenu(false);
    Result = 1;
  }

  return Result;
}

TCustomFarFileSystem * TWinSCPPlugin::OpenPluginEx(OPENFROM OpenFrom, intptr_t Item)
{
  std::unique_ptr<TWinSCPFileSystem> FileSystem;
  bool Success = true;
  CoreInitializeOnce();
  // DEBUG_PRINTF("OpenFrom: %d", (int)OpenFrom);

  if ((OpenFrom == OPEN_PLUGINSMENU) &&
    (!GetFarConfiguration()->GetPluginsMenu() || (Item == 1)))
  {
    CommandsMenu(true);
  }
  else
  {
    FileSystem = std::make_unique<TWinSCPFileSystem>(this);
    FileSystem->InitWinSCPFileSystem(nullptr);

    if (OpenFrom == OPEN_LEFTDISKMENU || OpenFrom == OPEN_RIGHTDISKMENU ||
      OpenFrom == OPEN_PLUGINSMENU ||
      OpenFrom == OPEN_FINDLIST)
    {
      // nothing
    }
    else if (OpenFrom == OPEN_SHORTCUT || OpenFrom == OPEN_COMMANDLINE)
    {
      UnicodeString Directory;
      UnicodeString CommandLine;
      FAROPENSHORTCUTFLAGS Flags = FOSF_NONE;
      if (OpenFrom == OPEN_SHORTCUT)
      {
        const OpenShortcutInfo * Info = reinterpret_cast<OpenShortcutInfo *>(Item);
        CommandLine = Info->ShortcutData;
        Flags = Info->Flags;
      }
      else
      {
        const OpenCommandLineInfo * Info = reinterpret_cast<OpenCommandLineInfo *>(Item);
        CommandLine = Info->CommandLine;
      }
      // DEBUG_PRINTF("CommandLine: %s", CommandLine);
      if (OpenFrom == OPEN_SHORTCUT)
      {
        const int32_t P = CommandLine.Pos(SHORTCUT_DELIMITER);
        if (P > 0)
        {
          Directory = CommandLine.SubString(P + 1, CommandLine.Length() - P);
          CommandLine.SetLength(P - 1);
        }
        // DEBUG_PRINTF("Directory: %s", Directory);
        // DEBUG_PRINTF("CommandLine: %s", CommandLine);

        const bool Another = !(Flags & FOSF_ACTIVE);
        TWinSCPFileSystem * PanelSystem = rtti::dyn_cast_or_null<TWinSCPFileSystem>(GetPanelFileSystem());

        if (PanelSystem && PanelSystem->Connected() &&
          PanelSystem->GetTerminal()->GetSessionData()->GenerateSessionUrl(sufComplete) == CommandLine)
        {
          PanelSystem->SetDirectoryEx(Directory, OPM_SILENT);
          if (PanelSystem->UpdatePanel(false, Another))
          {
            PanelSystem->RedrawPanel(Another);
          }
          Abort();
        }
        // directory will be set by FAR itself
        Directory.Clear();
      }
      DebugAssert(GetStoredSessions());
      bool DefaultsOnly = false;
      std::unique_ptr<TOptions> Options(std::make_unique<TProgramParams>());
      ParseCommandLine(CommandLine, Options.get());
      constexpr int32_t ParseUrlFlags = pufAllowStoredSiteWithProtocol;
      std::unique_ptr<TSessionData> Session(GetStoredSessions()->ParseUrl(CommandLine, Options.get(), DefaultsOnly, nullptr, nullptr, nullptr, ParseUrlFlags));
      if (!DefaultsOnly)
      {
        if (!Session->GetCanLogin())
        {
          DebugAssert(false);
          Abort();
        }
        FileSystem->SetConnectedDirectly();
        Success = FileSystem->Connect(Session.get());
        if (Success && !Directory.IsEmpty())
        {
          FileSystem->SetDirectoryEx(Directory, OPM_SILENT);
        }
      }
    }
    else if (OpenFrom == OPEN_ANALYSE)
    {
      const OpenAnalyseInfo * Info = reinterpret_cast<OpenAnalyseInfo *>(Item);
      const wchar_t * XmlFileName = Info->Info->FileName;
      std::unique_ptr<THierarchicalStorage> ImportStorage(std::make_unique<TXmlStorage>(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));

      ImportStorage->Init();
      ImportStorage->SetAccessMode(smRead);
      if (!(ImportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false) &&
          ImportStorage->HasSubKeys()))
      {
        DebugAssert(false);
        Abort();
      }
      const UnicodeString SessionName = ::PuttyUnMungeStr(ImportStorage->ReadStringRaw("Session", ""));
      std::unique_ptr<TSessionData> Session(std::make_unique<TSessionData>(SessionName));
      Session->Load(ImportStorage.get(), false);
      Session->SetModified(true);
      if (!Session->GetCanLogin())
      {
        DebugAssert(false);
        Abort();
      }
      FileSystem->SetConnectedDirectly();
      Success = FileSystem->Connect(Session.get());
    }
    else
    {
      DebugAssert(false);
    }
  }
  if (!Success)
  {
    FileSystem.reset(nullptr);
  }
  return FileSystem.release();
}

void TWinSCPPlugin::ParseCommandLine(UnicodeString & CommandLine,
  const TOptions * Options)
{
  UnicodeString CmdLine = CommandLine;
  int32_t Index = 1;
  // Skip session name
  {
    while ((Index < CmdLine.Length()) && (CmdLine[Index] == L' '))
      ++Index;
    if (Index >= CmdLine.Length())
      return;
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
  CmdLine = CmdLine.SubString(Index);
  // Parse params
  const int32_t Pos = ::FirstDelimiter(Options->GetSwitchMarks(), CmdLine);
  UnicodeString CommandLineParams;
  if (Pos > 0)
    CommandLineParams = CmdLine.SubString(Pos);
  if (!CommandLineParams.IsEmpty())
  {
    TODO("implement Options->ParseParams(CommandLineParams)");
    // ThrowNotImplemented(3015);
    CommandLine = CommandLine.SubString(1, CommandLine.Length() - CommandLineParams.Length()).Trim();
  }
}

void TWinSCPPlugin::CommandsMenu(bool FromFileSystem)
{
  std::unique_ptr<TFarMenuItems> MenuItems(std::make_unique<TFarMenuItems>());
  TWinSCPFileSystem * WinSCPFileSystem = rtti::dyn_cast_or_null<TWinSCPFileSystem>(GetPanelFileSystem());
  TWinSCPFileSystem * AnotherFileSystem = rtti::dyn_cast_or_null<TWinSCPFileSystem>(GetPanelFileSystem(true));
  const bool FSConnected = (WinSCPFileSystem != nullptr) && WinSCPFileSystem->Connected();
  const bool AnotherFSConnected = (AnotherFileSystem != nullptr) && AnotherFileSystem->Connected();
  const bool FSVisible = FSConnected && FromFileSystem;
  const bool AnyFSVisible = (FSConnected || AnotherFSConnected) && FromFileSystem;

  const int32_t MAttributes = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_ATTRIBUTES), FSVisible);
  const int32_t MLink = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_LINK), FSVisible);
  const int32_t MApplyCommand = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_APPLY_COMMAND), FSVisible);
  const int32_t MFullSynchronize = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_FULL_SYNCHRONIZE), AnyFSVisible);
  const int32_t MSynchronize = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_SYNCHRONIZE), AnyFSVisible);
  const int32_t MQueue = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_QUEUE), FSVisible);
  const int32_t MInformation = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_INFORMATION), FSVisible);
  const int32_t MLog = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_LOG), FSVisible);
  const int32_t MClearCaches = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_CLEAR_CACHES), FSVisible);
  const int32_t MPutty = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_PUTTY), FSVisible);
  const int32_t MEditHistory = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_EDIT_HISTORY), FSConnected);
  MenuItems->AddSeparator(FSConnected || FSVisible);
  const int32_t MAddBookmark = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_ADD_BOOKMARK), FSVisible);
  const int32_t MOpenDirectory = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_OPEN_DIRECTORY), FSVisible);
  const int32_t MHomeDirectory = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_HOME_DIRECTORY), FSVisible);
  const int32_t MSynchronizeBrowsing = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_SYNCHRONIZE_BROWSING), FSVisible);
  MenuItems->AddSeparator(FSVisible);
  const int32_t MPageant = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_PAGEANT), FromFileSystem);
  const int32_t MPuttygen = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_PUTTYGEN), FromFileSystem);
  MenuItems->AddSeparator(FromFileSystem);
  const int32_t MConfigure = MenuItems->AddString(GetMsg(NB_MENU_COMMANDS_CONFIGURE));
  const int32_t MAbout = MenuItems->AddString(GetMsg(NB_CONFIG_ABOUT));

  MenuItems->SetDisabled(MLog, !FSVisible || (WinSCPFileSystem && !WinSCPFileSystem->IsLogging()));
  MenuItems->SetDisabled(MClearCaches, !FSVisible || (WinSCPFileSystem && WinSCPFileSystem->AreCachesEmpty()));
  MenuItems->SetDisabled(MPutty, !FSVisible || !base::FileExists(::ExpandEnvVars(ExtractProgram(GetFarConfiguration()->GetPuttyPath()))));
  MenuItems->SetDisabled(MEditHistory, !FSConnected || (WinSCPFileSystem && WinSCPFileSystem->IsEditHistoryEmpty()));
  MenuItems->SetChecked(MSynchronizeBrowsing, FSVisible && (WinSCPFileSystem && WinSCPFileSystem->IsSynchronizedBrowsing()));
  MenuItems->SetDisabled(MPageant, !base::FileExists(::ExpandEnvVars(ExtractProgram(GetFarConfiguration()->GetPageantPath()))));
  MenuItems->SetDisabled(MPuttygen, !base::FileExists(::ExpandEnvVars(ExtractProgram(GetFarConfiguration()->GetPuttygenPath()))));

  const intptr_t Result = Menu(FMENU_WRAPMODE, GetMsg(NB_MENU_COMMANDS), "", MenuItems.get());

  if (Result >= 0)
  {
    if ((Result == MLog) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->ShowLog();
    }
    else if ((Result == MAttributes) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->FileProperties();
    }
    else if ((Result == MLink) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->RemoteCreateLink();
    }
    else if ((Result == MApplyCommand) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->ApplyCommand();
    }
    else if (Result == MFullSynchronize)
    {
      if (WinSCPFileSystem != nullptr)
      {
        WinSCPFileSystem->FullSynchronize(true);
      }
      else
      {
        DebugAssert(AnotherFileSystem != nullptr);
        if (AnotherFileSystem)
          AnotherFileSystem->FullSynchronize(false);
      }
    }
    else if (Result == MSynchronize)
    {
      if (WinSCPFileSystem != nullptr)
      {
        WinSCPFileSystem->Synchronize();
      }
      else
      {
        DebugAssert(AnotherFileSystem != nullptr);
        if (AnotherFileSystem)
          AnotherFileSystem->Synchronize();
      }
    }
    else if ((Result == MQueue) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->QueueShow(false);
    }
    else if ((Result == MAddBookmark || Result == MOpenDirectory) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->OpenDirectory(Result == MAddBookmark);
    }
    else if (Result == MHomeDirectory && WinSCPFileSystem)
    {
      WinSCPFileSystem->HomeDirectory();
    }
    else if (Result == MConfigure)
    {
      ConfigureEx(nullptr);
    }
    else if (Result == MAbout)
    {
      AboutDialog();
    }
    else if ((Result == MPutty) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->OpenSessionInPutty();
    }
    else if ((Result == MEditHistory) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->EditHistory();
    }
    else if (Result == MPageant || Result == MPuttygen)
    {
      const UnicodeString Path = (Result == MPageant) ?
        GetFarConfiguration()->GetPageantPath() : GetFarConfiguration()->GetPuttygenPath();
      UnicodeString Program, Params, Dir;
      SplitCommand(::ExpandEnvVars(Path), Program, Params, Dir);
      ::ExecuteShellChecked(Program, Params);
    }
    else if ((Result == MClearCaches) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->ClearCaches();
    }
    else if ((Result == MSynchronizeBrowsing) && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem != nullptr);
      WinSCPFileSystem->ToggleSynchronizeBrowsing();
    }
    else if (Result == MInformation && WinSCPFileSystem)
    {
      DebugAssert(WinSCPFileSystem);
      WinSCPFileSystem->ShowInformation();
    }
    else
    {
      DebugAssert(false);
    }
  }
}

void TWinSCPPlugin::ShowExtendedException(Exception * E)
{
  if (E && !E->Message.IsEmpty())
  {
    const TQueryType Type = rtti::isa<ETerminate>(E) ? qtInformation : qtError;

    TStrings * MoreMessages = nullptr;
    if (rtti::isa<ExtException>(E))
    {
      MoreMessages = rtti::dyn_cast_or_null<ExtException>(E)->GetMoreMessages();
    }
    const UnicodeString Message = TranslateExceptionMessage(E);
    MoreMessageDialog(Message, MoreMessages, Type, qaOK);
  }
}

void TWinSCPPlugin::HandleException(Exception * E, OPERATION_MODES OpMode)
{
  if (((OpMode & OPM_FIND) == 0) || rtti::isa<EFatal>(E))
  {
    ShowExtendedException(E);
  }
}

struct TFarMessageData final : public TObject
{
  NB_DISABLE_COPY(TFarMessageData)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TFarMessageData); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarMessageData) || TObject::is(Kind); }
public:
  TFarMessageData() noexcept : TObject(OBJECT_CLASS_TFarMessageData)
  {
    nb::ClearArray(Buttons);
  }

  const TMessageParams * Params{nullptr};
  uint32_t Buttons[15 + 1]{};
  uint32_t ButtonCount{0};
};

void TWinSCPPlugin::MessageClick(void * Token, int32_t Result, bool & Close)
{
  DebugAssert(Token);
  const TFarMessageData & Data = *static_cast<TFarMessageData *>(Token);

  DebugAssert((Result != -1) && (Result < nb::ToInt32(Data.ButtonCount)));

  if ((Data.Params != nullptr) && (Data.Params->Aliases != nullptr))
  {
    for (uint32_t Index = 0; Index < Data.Params->AliasesCount; ++Index)
    {
      const TQueryButtonAlias & Alias = Data.Params->Aliases[Index];
      if ((Result >= 0) && (Result < nb::ToInt32(Data.ButtonCount)) && (Alias.Button == Data.Buttons[Result]) &&
        (Alias.OnSubmit))
      {
        uint32_t Answer{0};
        Alias.OnSubmit(nullptr, Answer);
        Close = false;
        break;
      }
    }
  }
}

uint32_t TWinSCPPlugin::MoreMessageDialog(const UnicodeString & Str,
  TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
  const TMessageParams * Params)
{
  uint32_t Result;
  UnicodeString DialogStr = Str;
  std::unique_ptr<TStrings> ButtonLabels(std::make_unique<TStringList>());
  uint32_t Flags = 0;

  if (Params != nullptr)
  {
    Flags = Params->Flags;
  }

  int32_t TitleId = 0;
  switch (Type)
  {
  case qtConfirmation:
    TitleId = MSG_TITLE_CONFIRMATION;
    break;
  case qtInformation:
    TitleId = MSG_TITLE_INFORMATION;
    break;
  case qtError:
    TitleId = MSG_TITLE_ERROR;
    Flags |= FMSG_WARNING;
    break;
  case qtWarning:
    TitleId = MSG_TITLE_WARNING;
    Flags |= FMSG_WARNING;
    break;
  default:
    DebugAssert(false);
  }
  TFarMessageData Data;
  Data.Params = Params;

  // make sure to do the check on full answers, not on reduced "timer answers"
  if (((Answers & qaAbort) && (Answers & qaRetry)) ||
    (GetTopDialog() != nullptr))
  {
    // use warning colors for abort/retry confirmation dialog
    Flags |= FMSG_WARNING;
  }

  if (Params != nullptr)
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

  uint32_t AAnswers = Answers;
  bool NeverAskAgainCheck = (Params != nullptr) && FLAGSET(Params->Params, mpNeverAskAgainCheck);
  bool NeverAskAgainPending = NeverAskAgainCheck;
  int32_t TimeoutButton = 0;

#define ADD_BUTTON_EX(TYPE, CANNEVERASK) \
  do { if (AAnswers & qa ## TYPE) \
  { \
    ButtonLabels->Add(GetMsg(MSG_BUTTON_ ## TYPE)); \
    Data.Buttons[Data.ButtonCount] = qa ## TYPE; \
    Data.ButtonCount++; \
    AAnswers -= qa ## TYPE; \
    if ((Params != nullptr) && (Params->Timeout != 0) && \
        (Params->TimeoutAnswer == qa ## TYPE)) \
    { \
      TimeoutButton = ButtonLabels->GetCount() - 1; \
    } \
    if (NeverAskAgainPending && (CANNEVERASK)) \
    { \
      ButtonLabels->SetObject(ButtonLabels->GetCount() - 1, ToObj(true)); \
      NeverAskAgainPending = false; \
    } \
  } } while (0)
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

  DebugUsedParam(AAnswers);
  DebugAssert(!AAnswers);
  DebugUsedParam(NeverAskAgainPending);
  DebugAssert(!NeverAskAgainPending);

  int32_t DefaultButtonIndex = 0;
  if ((Params != nullptr) && (Params->Aliases != nullptr))
  {
    for (uint32_t bi = 0; bi < Data.ButtonCount; bi++)
    {
      for (uint32_t ai = 0; ai < Params->AliasesCount; ai++)
      {
        if (Params->Aliases[ai].Button == Data.Buttons[bi] &&
          !Params->Aliases[ai].Alias.IsEmpty())
        {
          ButtonLabels->SetString(nb::ToInt32(bi), Params->Aliases[ai].Alias);
          if (Params->Aliases[ai].Default)
            DefaultButtonIndex = nb::ToInt32(bi);
          break;
        }
      }
    }
  }

  constexpr const int32_t MORE_BUTTON_ID = -2;
  TFarMessageParams FarParams{};

  if (NeverAskAgainCheck)
  {
    FarParams.CheckBoxLabel =
      (Answers == qaOK) ? GetMsg(MSG_CHECK_NEVER_SHOW_AGAIN) :
      GetMsg(MSG_CHECK_NEVER_ASK_AGAIN);
  }

  if (Params != nullptr)
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
  FarParams.DefaultButton = DefaultButtonIndex;
  FarParams.ClickEvent = nb::bind(&TWinSCPPlugin::MessageClick, this);

  if (MoreMessages && (MoreMessages->GetCount() > 0))
  {
    FarParams.MoreMessages = MoreMessages;
  }
  else
  {
    FarParams.MoreMessages = nullptr;
  }

  Result = Message(Flags, GetMsg(TitleId), DialogStr, ButtonLabels.get(), &FarParams);
  if (FarParams.TimerAnswer > 0)
  {
    Result = FarParams.TimerAnswer;
  }
  else if (Result == nb::ToUInt32(nb::NPOS))
  {
    Result = CancelAnswer(Answers);
  }
  else
  {
    DebugAssert(Result != static_cast<uint32_t>(-1) && Result < Data.ButtonCount);
    Result = Data.Buttons[Result];
  }

  if (FarParams.CheckBox)
  {
    DebugAssert(NeverAskAgainCheck);
    Result = qaNeverAskAgain;
  }

  return Result;
}

void TWinSCPPlugin::DeleteLocalFile(const UnicodeString & LocalFileName)
{
  GetSystemFunctions()->DeleteFile(LocalFileName.c_str());
}

HANDLE TWinSCPPlugin::CreateLocalFile(const UnicodeString & LocalFileName,
  DWORD DesiredAccess, DWORD ShareMode, DWORD CreationDisposition, DWORD FlagsAndAttributes)
{
  return GetSystemFunctions()->CreateFile(LocalFileName.c_str(), DesiredAccess,
    ShareMode, nullptr, CreationDisposition, FlagsAndAttributes, nullptr);
}

DWORD TWinSCPPlugin::GetLocalFileAttributes(const UnicodeString & LocalFileName) const
{
  return GetSystemFunctions()->GetFileAttributes(LocalFileName.c_str());
}

bool TWinSCPPlugin::SetLocalFileAttributes(const UnicodeString & LocalFileName, DWORD FileAttributes)
{
  return GetSystemFunctions()->SetFileAttributes(LocalFileName.c_str(), FileAttributes) != FALSE;
}

bool TWinSCPPlugin::MoveLocalFile(const UnicodeString & LocalFileName, const UnicodeString & NewLocalFileName, DWORD Flags)
{
  return GetSystemFunctions()->MoveFileEx(LocalFileName.c_str(), NewLocalFileName.c_str(), Flags) != FALSE;
}

bool TWinSCPPlugin::RemoveLocalDirectory(const UnicodeString & LocalDirName)
{
  return GetSystemFunctions()->RemoveDirectory(LocalDirName.c_str()) != FALSE;
}

bool TWinSCPPlugin::CreateLocalDirectory(const UnicodeString & LocalDirName, LPSECURITY_ATTRIBUTES SecurityAttributes)
{
  return GetSystemFunctions()->CreateDirectory(LocalDirName.c_str(), SecurityAttributes) != FALSE;
}

void TWinSCPPlugin::CleanupConfiguration()
{
  // Check if key Configuration\Version exists
  std::unique_ptr<THierarchicalStorage> Storage(GetFarConfiguration()->CreateConfigStorage());
  Storage->SetAccessMode(smReadWrite);
  if (Storage->OpenSubKey(GetFarConfiguration()->GetConfigurationSubKey(), false))
  {
    if (!Storage->ValueExists("Version"))
    {
      Storage->RecursiveDeleteSubKey("CDCache");
    }
    else
    {
      const UnicodeString Version = Storage->ReadString("Version", "");
      if (::StrToVersionNumber(Version) < MAKEVERSIONNUMBER(2, 1, 19))
      {
        Storage->RecursiveDeleteSubKey("CDCache");
      }
    }
    Storage->WriteStringRaw("Version", ::VersionNumberToStr(::GetCurrentVersionNumber()));
    Storage->CloseSubKey();
  }
}

void TWinSCPPlugin::CoreInitializeOnce()
{
  if (!FInitialized)
  {
    CoreInitialize();
    CleanupConfiguration();
    FInitialized = true;
  }
}

