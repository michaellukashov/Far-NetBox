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

TCustomFarPlugin *CreateFarPlugin(HINSTANCE HInst)
{
  TCustomFarPlugin *Result = new TWinSCPPlugin(HInst);
  Result->Initialize();
  return Result;
}

void DestroyFarPlugin(TCustomFarPlugin *& Plugin)
{
  DebugAssert(FarPlugin);
  Plugin->Finalize();
  SAFE_DESTROY(Plugin);
}

TWinSCPPlugin::TWinSCPPlugin(HINSTANCE HInst) noexcept :
  TCustomFarPlugin(OBJECT_CLASS_TWinSCPPlugin, HInst),
  FInitialized(false)
{
}

TWinSCPPlugin::~TWinSCPPlugin() noexcept
{
  if (FInitialized)
  {
    GetFarConfiguration()->SetPlugin(nullptr);
    CoreFinalize();
  }
}

bool TWinSCPPlugin::HandlesFunction(THandlesFunction Function) const
{
  return (Function == hfProcessKey || Function == hfProcessEvent);
}

intptr_t TWinSCPPlugin::GetMinFarVersion() const
{
  return MAKEFARVERSION(2, 0, 1667);
}

void TWinSCPPlugin::SetStartupInfo(const struct PluginStartupInfo *Info)
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

void TWinSCPPlugin::GetPluginInfoEx(DWORD &Flags,
  TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
  TStrings *PluginConfigStrings, TStrings *CommandPrefixes)
{
  CoreInitializeOnce();
  Flags = PF_FULLCMDLINE;
  TFarConfiguration *FarConfiguration = GetFarConfiguration();
  if (FarConfiguration->GetDisksMenu())
  {
    DiskMenuStrings->AddObject(GetMsg(NB_PLUGIN_NAME),
      ToObj(FarConfiguration->GetDisksMenuHotKey()));
  }
  if (FarConfiguration->GetPluginsMenu())
  {
    PluginMenuStrings->Add(GetMsg(NB_PLUGIN_NAME));
  }
  if (FarConfiguration->GetPluginsMenuCommands())
  {
    PluginMenuStrings->Add(GetMsg(NB_MENU_COMMANDS));
  }
  PluginConfigStrings->Add(GetMsg(NB_PLUGIN_NAME));
  CommandPrefixes->SetCommaText(FarConfiguration->GetCommandPrefixes());
}

bool TWinSCPPlugin::ConfigureEx(intptr_t /*Item*/)
{
  bool Change = false;

  std::unique_ptr<TFarMenuItems> MenuItems(std::make_unique<TFarMenuItems>());
  intptr_t MInterface = MenuItems->Add(GetMsg(NB_CONFIG_INTERFACE));
  intptr_t MConfirmations = MenuItems->Add(GetMsg(NB_CONFIG_CONFIRMATIONS));
  intptr_t MPanel = MenuItems->Add(GetMsg(NB_CONFIG_PANEL));
  intptr_t MTransfer = MenuItems->Add(GetMsg(NB_CONFIG_TRANSFER));
  intptr_t MBackground = MenuItems->Add(GetMsg(NB_CONFIG_BACKGROUND));
  intptr_t MEndurance = MenuItems->Add(GetMsg(NB_CONFIG_ENDURANCE));
  intptr_t MTransferEditor = MenuItems->Add(GetMsg(NB_CONFIG_TRANSFER_EDITOR));
  intptr_t MLogging = MenuItems->Add(GetMsg(NB_CONFIG_LOGGING));
  intptr_t MIntegration = MenuItems->Add(GetMsg(NB_CONFIG_INTEGRATION));
  MenuItems->AddSeparator();
  intptr_t MAbout = MenuItems->Add(GetMsg(NB_CONFIG_ABOUT));

  intptr_t Result;

  do
  {
    Result = Menu(FMENU_WRAPMODE, GetMsg(NB_PLUGIN_TITLE), L"", MenuItems.get());

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

intptr_t TWinSCPPlugin::ProcessEditorEventEx(intptr_t Event, void *Param)
{
  // for performance reasons, do not pass the event to file systems on redraw
  if ((Event != EE_REDRAW) || GetFarConfiguration()->GetEditorUploadOnSave() ||
    GetFarConfiguration()->GetEditorMultiple())
  {
    for (intptr_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
    {
      TWinSCPFileSystem *FileSystem = FOpenedPlugins->GetAs<TWinSCPFileSystem>(Index);
      FileSystem->ProcessEditorEvent(Event, Param);
    }
  }

  return 0;
}

intptr_t TWinSCPPlugin::ProcessEditorInputEx(const INPUT_RECORD *Rec)
{
  intptr_t Result = 0;
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

  return Result;
}

TCustomFarFileSystem *TWinSCPPlugin::OpenPluginEx(intptr_t OpenFrom, intptr_t Item)
{
  std::unique_ptr<TWinSCPFileSystem> FileSystem;
  CoreInitializeOnce();

  if ((OpenFrom == OPEN_PLUGINSMENU) &&
    (!GetFarConfiguration()->GetPluginsMenu() || (Item == 1)))
  {
    CommandsMenu(true);
  }
  else
  {
    FileSystem = std::make_unique<TWinSCPFileSystem>(this);
    FileSystem->Init(nullptr);

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
        const intptr_t P = CommandLine.Pos(L"\1");
        if (P > 0)
        {
          Directory = CommandLine.SubString(P + 1, CommandLine.Length() - P);
          CommandLine.SetLength(P - 1);
        }

        TWinSCPFileSystem *PanelSystem = dyn_cast<TWinSCPFileSystem>(GetPanelFileSystem());
        if (PanelSystem && PanelSystem->Connected() &&
          PanelSystem->GetTerminal()->GetSessionData()->GenerateSessionUrl(sufComplete) == CommandLine)
        {
          PanelSystem->SetDirectoryEx(Directory, OPM_SILENT);
          if (PanelSystem->UpdatePanel())
          {
            PanelSystem->RedrawPanel();
          }
          Abort();
        }
        // directory will be set by FAR itself
        Directory.Clear();
      }
      DebugAssert(StoredSessions);
      bool DefaultsOnly = false;
      std::unique_ptr<TOptions> Options(std::make_unique<TProgramParams>());
      ParseCommandLine(CommandLine, Options.get());
      std::unique_ptr<TSessionData> Session(StoredSessions->ParseUrl(CommandLine, Options.get(), DefaultsOnly));
      if (DefaultsOnly)
      {
        Abort();
      }
      if (!Session->GetCanLogin())
      {
        DebugAssert(false);
        Abort();
      }
      FileSystem->Connect(Session.get());
      if (!Directory.IsEmpty())
      {
        FileSystem->SetDirectoryEx(Directory, OPM_SILENT);
      }
    }
    else if (OpenFrom == OPEN_ANALYSE)
    {
      const wchar_t *XmlFileName = reinterpret_cast<const wchar_t *>(Item);
      std::unique_ptr<THierarchicalStorage> ImportStorage(std::make_unique<TXmlStorage>(XmlFileName, GetConfiguration()->GetStoredSessionsSubKey()));
      ImportStorage->Init();
      ImportStorage->SetAccessMode(smRead);
      if (!(ImportStorage->OpenSubKey(GetConfiguration()->GetStoredSessionsSubKey(), false) &&
          ImportStorage->HasSubKeys()))
      {
        DebugAssert(false);
        Abort();
      }
      const UnicodeString SessionName = ::PuttyUnMungeStr(ImportStorage->ReadStringRaw("Session", L""));
      std::unique_ptr<TSessionData> Session(std::make_unique<TSessionData>(SessionName));
      Session->Load(ImportStorage.get(), false);
      Session->SetModified(true);
      if (!Session->GetCanLogin())
      {
        DebugAssert(false);
        Abort();
      }
      FileSystem->Connect(Session.get());
    }
    else
    {
      DebugAssert(false);
    }
  }

  return FileSystem.release();
}

void TWinSCPPlugin::ParseCommandLine(UnicodeString &CommandLine,
  TOptions *Options)
{
  UnicodeString CmdLine = CommandLine;
  intptr_t Index = 1;
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
  intptr_t Pos = ::FirstDelimiter(Options->GetSwitchMarks(), CmdLine);
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
  TWinSCPFileSystem *WinSCPFileSystem;
  TWinSCPFileSystem *AnotherFileSystem;
  WinSCPFileSystem = dyn_cast<TWinSCPFileSystem>(GetPanelFileSystem());
  AnotherFileSystem = dyn_cast<TWinSCPFileSystem>(GetPanelFileSystem(true));
  bool FSConnected = (WinSCPFileSystem != nullptr) && WinSCPFileSystem->Connected();
  bool AnotherFSConnected = (AnotherFileSystem != nullptr) && AnotherFileSystem->Connected();
  bool FSVisible = FSConnected && FromFileSystem;
  bool AnyFSVisible = (FSConnected || AnotherFSConnected) && FromFileSystem;

  intptr_t MAttributes = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_ATTRIBUTES), FSVisible);
  intptr_t MLink = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_LINK), FSVisible);
  intptr_t MApplyCommand = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_APPLY_COMMAND), FSVisible);
  intptr_t MFullSynchronize = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_FULL_SYNCHRONIZE), AnyFSVisible);
  intptr_t MSynchronize = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_SYNCHRONIZE), AnyFSVisible);
  intptr_t MQueue = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_QUEUE), FSVisible);
  intptr_t MInformation = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_INFORMATION), FSVisible);
  intptr_t MLog = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_LOG), FSVisible);
  intptr_t MClearCaches = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_CLEAR_CACHES), FSVisible);
  intptr_t MPutty = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_PUTTY), FSVisible);
  intptr_t MEditHistory = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_EDIT_HISTORY), FSConnected);
  MenuItems->AddSeparator(FSConnected || FSVisible);
  intptr_t MAddBookmark = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_ADD_BOOKMARK), FSVisible);
  intptr_t MOpenDirectory = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_OPEN_DIRECTORY), FSVisible);
  intptr_t MHomeDirectory = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_HOME_DIRECTORY), FSVisible);
  intptr_t MSynchronizeBrowsing = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_SYNCHRONIZE_BROWSING), FSVisible);
  MenuItems->AddSeparator(FSVisible);
  intptr_t MPageant = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_PAGEANT), FromFileSystem);
  intptr_t MPuttygen = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_PUTTYGEN), FromFileSystem);
  MenuItems->AddSeparator(FromFileSystem);
  intptr_t MConfigure = MenuItems->Add(GetMsg(NB_MENU_COMMANDS_CONFIGURE));
  intptr_t MAbout = MenuItems->Add(GetMsg(NB_CONFIG_ABOUT));

  MenuItems->SetDisabled(MLog, !FSVisible || (WinSCPFileSystem && !WinSCPFileSystem->IsLogging()));
  MenuItems->SetDisabled(MClearCaches, !FSVisible || (WinSCPFileSystem && WinSCPFileSystem->AreCachesEmpty()));
  MenuItems->SetDisabled(MPutty, !FSVisible || !::SysUtulsFileExists(::ExpandEnvVars(ExtractProgram(GetFarConfiguration()->GetPuttyPath()))));
  MenuItems->SetDisabled(MEditHistory, !FSConnected || (WinSCPFileSystem && WinSCPFileSystem->IsEditHistoryEmpty()));
  MenuItems->SetChecked(MSynchronizeBrowsing, FSVisible && (WinSCPFileSystem && WinSCPFileSystem->IsSynchronizedBrowsing()));
  MenuItems->SetDisabled(MPageant, !::SysUtulsFileExists(::ExpandEnvVars(ExtractProgram(GetFarConfiguration()->GetPageantPath()))));
  MenuItems->SetDisabled(MPuttygen, !::SysUtulsFileExists(::ExpandEnvVars(ExtractProgram(GetFarConfiguration()->GetPuttygenPath()))));

  intptr_t Result = Menu(FMENU_WRAPMODE, GetMsg(NB_MENU_COMMANDS), L"", MenuItems.get());

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
      ConfigureEx(0);
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
      UnicodeString Path = (Result == MPageant) ?
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

void TWinSCPPlugin::ShowExtendedException(Exception *E)
{
  if (E && !E->Message.IsEmpty())
  {
    if (isa<EAbort>(E))
    {
      TQueryType Type = isa<ESshTerminate>(E) ? qtInformation : qtError;

      TStrings *MoreMessages = nullptr;
      if (isa<ExtException>(E))
      {
        MoreMessages = dyn_cast<ExtException>(E)->GetMoreMessages();
      }
      UnicodeString Message = TranslateExceptionMessage(E);
      MoreMessageDialog(Message, MoreMessages, Type, qaOK);
    }
  }
}

void TWinSCPPlugin::HandleException(Exception *E, int OpMode)
{
  if (((OpMode & OPM_FIND) == 0) || isa<EFatal>(E))
  {
    ShowExtendedException(E);
  }
}

NB_DEFINE_CLASS_ID(TFarMessageData);
struct TFarMessageData : public TObject
{
  NB_DISABLE_COPY(TFarMessageData)
public:
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TFarMessageData); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TFarMessageData) || TObject::is(Kind); }
public:
  TFarMessageData() :
    TObject(OBJECT_CLASS_TFarMessageData),
    Params(nullptr),
    ButtonCount(0)
  {
    nb::ClearArray(Buttons);
  }

  const TMessageParams *Params;
  uint32_t Buttons[15 + 1];
  uintptr_t ButtonCount;
};

void TWinSCPPlugin::MessageClick(void *Token, uintptr_t Result, bool &Close)
{
  DebugAssert(Token);
  TFarMessageData &Data = *get_as<TFarMessageData>(Token);

  DebugAssert(Result != nb::ToUIntPtr(-1) && Result < Data.ButtonCount);

  if ((Data.Params != nullptr) && (Data.Params->Aliases != nullptr))
  {
    for (uintptr_t Index = 0; Index < Data.Params->AliasesCount; ++Index)
    {
      const TQueryButtonAlias &Alias = Data.Params->Aliases[Index];
      if ((Alias.Button == Data.Buttons[Result]) &&
        (Alias.OnSubmit))
      {
        uint32_t Answer = 0;
        Alias.OnSubmit(nullptr, Answer);
        Close = false;
        break;
      }
    }
  }
}

uintptr_t TWinSCPPlugin::MoreMessageDialog(const UnicodeString Str,
  TStrings *MoreMessages, TQueryType Type, uintptr_t Answers,
  const TMessageParams *Params)
{
  uintptr_t Result;
  UnicodeString DialogStr = Str;
  std::unique_ptr<TStrings> ButtonLabels(std::make_unique<TStringList>());
  uintptr_t Flags = 0;

  if (Params != nullptr)
  {
    Flags = Params->Flags;
  }

  intptr_t TitleId = 0;
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

  uintptr_t AAnswers = Answers;
  bool NeverAskAgainCheck = (Params != nullptr) && FLAGSET(Params->Params, qpNeverAskAgainCheck);
  bool NeverAskAgainPending = NeverAskAgainCheck;
  uintptr_t TimeoutButton = 0;

#define ADD_BUTTON_EX(TYPE, CANNEVERASK) \
    if (AAnswers & qa ## TYPE) \
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
      if (NeverAskAgainPending && CANNEVERASK) \
      { \
        ButtonLabels->SetObj(ButtonLabels->GetCount() - 1, ToObj(true)); \
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

  DebugUsedParam(AAnswers);
  DebugAssert(!AAnswers);
  DebugUsedParam(NeverAskAgainPending);
  DebugAssert(!NeverAskAgainPending);

  uintptr_t DefaultButtonIndex = 0;
  if ((Params != nullptr) && (Params->Aliases != nullptr))
  {
    for (uintptr_t bi = 0; bi < Data.ButtonCount; bi++)
    {
      for (uintptr_t ai = 0; ai < Params->AliasesCount; ai++)
      {
        if (Params->Aliases[ai].Button == Data.Buttons[bi] &&
          !Params->Aliases[ai].Alias.IsEmpty())
        {
          ButtonLabels->SetString(bi, Params->Aliases[ai].Alias);
          if (Params->Aliases[ai].Default)
            DefaultButtonIndex = bi;
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

  Result = Message(nb::ToDWord(Flags), GetMsg(TitleId), DialogStr, ButtonLabels.get(), &FarParams);
  if (FarParams.TimerAnswer > 0)
  {
    Result = FarParams.TimerAnswer;
  }
  else if (Result == static_cast<uintptr_t>(-1))
  {
    Result = CancelAnswer(Answers);
  }
  else
  {
    DebugAssert(Result != nb::ToUIntPtr(-1) && Result < Data.ButtonCount);
    Result = Data.Buttons[Result];
  }

  if (FarParams.CheckBox)
  {
    DebugAssert(NeverAskAgainCheck);
    Result = qaNeverAskAgain;
  }

  return Result;
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
      Storage->DeleteSubKey("CDCache");
    }
    else
    {
      const UnicodeString Version = Storage->ReadString("Version", L"");
      if (::StrToVersionNumber(Version) < MAKEVERSIONNUMBER(2, 1, 19))
      {
        Storage->DeleteSubKey("CDCache");
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

