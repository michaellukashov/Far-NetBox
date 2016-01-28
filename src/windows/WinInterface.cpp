#include <Interface.h>
#include <FarTexts.h>

static bool IsPositiveAnswer(uintptr_t Answer)
{
  return (Answer == qaYes) || (Answer == qaOK) || (Answer == qaYesToAll);
}

static void NeverAskAgainCheckClick(void * /*Data*/, TObject * Sender)
{
  TCheckBox * CheckBox = dynamic_cast<TCheckBox *>(Sender);
  DebugAssert(CheckBox != nullptr);
  TForm * Dialog = dynamic_cast<TForm *>(CheckBox->Owner);
  DebugAssert(Dialog != nullptr);

  uintptr_t PositiveAnswer = 0;

  if (CheckBox->Checked)
  {
    if (CheckBox->Tag > 0)
    {
      PositiveAnswer = CheckBox->Tag;
    }
    else
    {
      for (int ii = 0; ii < Dialog->ControlCount; ii++)
      {
        TButton * Button = dynamic_cast<TButton *>(Dialog->Controls[ii]);
        if (Button != nullptr)
        {
          if (IsPositiveAnswer(Button->ModalResult))
          {
            PositiveAnswer = Button->ModalResult;
            break;
          }
        }
      }
    }

    DebugAssert(PositiveAnswer != 0);
  }

  for (int ii = 0; ii < Dialog->ControlCount; ii++)
  {
    TButton * Button = dynamic_cast<TButton *>(Dialog->Controls[ii]);
    if (Button != nullptr)
    {
      if ((Button->ModalResult != 0) && (Button->ModalResult != static_cast<int>(qaCancel)))
      {
        Button->Enabled = !CheckBox->Checked || (Button->ModalResult == static_cast<int>(PositiveAnswer));
      }

      if (Button->DropDownMenu != nullptr)
      {
        for (int iii = 0; iii < Button->DropDownMenu->Items->Count; iii++)
        {
          TMenuItem * Item = Button->DropDownMenu->Items->Items[iii];
          Item->Enabled = Item->Default || !CheckBox->Checked;
        }
      }
    }
  }
}

static TCheckBox * FindNeverAskAgainCheck(TForm * Dialog)
{
  return DebugNotNull(dynamic_cast<TCheckBox *>(Dialog->FindComponent(L"NeverAskAgainCheck")));
}

TForm * CreateMessageDialogEx(const UnicodeString Msg,
  TStrings * MoreMessages, TQueryType Type, uintptr_t Answers, UnicodeString HelpKeyword,
  const TMessageParams * Params, TButton *& TimeoutButton)
{
  TMsgDlgType DlgType;
  switch (Type) {
    case qtConfirmation: DlgType = mtConfirmation; break;
    case qtInformation: DlgType = mtInformation; break;
    case qtError: DlgType = mtError; break;
    case qtWarning: DlgType = mtWarning; break;
    default: DebugFail();
  }

  uintptr_t TimeoutAnswer = (Params != nullptr) ? Params->TimeoutAnswer : 0;

  uintptr_t ActualAnswers = Answers;
  if ((Params == nullptr) || Params->AllowHelp)
  {
    Answers = Answers | qaHelp;
  }

  if (IsInternalErrorHelpKeyword(HelpKeyword))
  {
    Answers = Answers | qaReport;
  }

  if ((MoreMessages != nullptr) && (MoreMessages->Count == 0))
  {
    MoreMessages = nullptr;
  }

  UnicodeString ImageName;
  UnicodeString MoreMessagesUrl;
  TSize MoreMessagesSize;
  UnicodeString CustomCaption;
  if (Params != nullptr)
  {
    ImageName = Params->ImageName;
    MoreMessagesUrl = Params->MoreMessagesUrl;
    MoreMessagesSize = Params->MoreMessagesSize;
    CustomCaption = Params->CustomCaption;
  }

  const TQueryButtonAlias * Aliases = (Params != nullptr) ? Params->Aliases : nullptr;
  uintptr_t AliasesCount = (Params != nullptr) ? Params->AliasesCount : 0;

  UnicodeString NeverAskAgainCaption;
  bool HasNeverAskAgain = (Params != nullptr) && FLAGSET(Params->Params, mpNeverAskAgainCheck);
  if (HasNeverAskAgain)
  {
    NeverAskAgainCaption =
      !Params->NeverAskAgainTitle.IsEmpty() ?
        (UnicodeString)Params->NeverAskAgainTitle :
        // qaOK | qaIgnore is used, when custom "non-answer" button is required
        LoadStr(((ActualAnswers == qaOK) || (ActualAnswers == (qaOK | qaIgnore))) ?
          NEVER_SHOW_AGAIN : NEVER_ASK_AGAIN);
  }

  TForm * Dialog = CreateMoreMessageDialog(Msg, MoreMessages, DlgType, Answers,
    Aliases, AliasesCount, TimeoutAnswer, &TimeoutButton, ImageName, NeverAskAgainCaption,
    MoreMessagesUrl, MoreMessagesSize, CustomCaption);

  try
  {
    if (HasNeverAskAgain && DebugAlwaysTrue(Params != nullptr))
    {
      TCheckBox * NeverAskAgainCheck = FindNeverAskAgainCheck(Dialog);
      NeverAskAgainCheck->Checked = Params->NeverAskAgainCheckedInitially;
      if (Params->NeverAskAgainAnswer > 0)
      {
        NeverAskAgainCheck->Tag = Params->NeverAskAgainAnswer;
      }
      TNotifyEvent OnClick;
      ((TMethod*)&OnClick)->Code = NeverAskAgainCheckClick;
      NeverAskAgainCheck->OnClick = OnClick;
    }

    Dialog->HelpKeyword = HelpKeyword;
    if (FLAGSET(Answers, qaHelp))
    {
      Dialog->BorderIcons = Dialog->BorderIcons << biHelp;
    }
    ResetSystemSettings(Dialog);
  }
  catch(...)
  {
    delete Dialog;
    throw;
  }
  return Dialog;
}

uintptr_t ExecuteMessageDialog(TForm * Dialog, uintptr_t Answers, const TMessageParams * Params)
{
  FlashOnBackground();
  uintptr_t Answer = Dialog->ShowModal();
  // mrCancel is returned always when X button is pressed, despite
  // no Cancel button was on the dialog. Find valid "cancel" answer.
  // mrNone is retuned when Windows session is closing (log off)
  if ((Answer == mrCancel) || (Answer == mrNone))
  {
    Answer = CancelAnswer(Answers);
  }

  if ((Params != nullptr) && (Params->Params & mpNeverAskAgainCheck))
  {
    TCheckBox * NeverAskAgainCheck = FindNeverAskAgainCheck(Dialog);

    if (NeverAskAgainCheck->Checked)
    {
      bool PositiveAnswer =
        (Params->NeverAskAgainAnswer > 0) ?
          (Answer == Params->NeverAskAgainAnswer) :
          IsPositiveAnswer(Answer);
      if (PositiveAnswer)
      {
        Answer = qaNeverAskAgain;
      }
    }
  }

  return Answer;
}

class TMessageTimer : public TTimer
{
public:
  TQueryParamsTimerEvent Event;
  TForm * Dialog;

  TMessageTimer(TComponent * AOwner);

protected:
  void DoTimer(TObject * Sender);
};

TMessageTimer::TMessageTimer(TComponent * AOwner) : TTimer(AOwner)
{
  Event = nullptr;
  OnTimer = DoTimer;
  Dialog = nullptr;
}

void TMessageTimer::DoTimer(TObject * /*Sender*/)
{
  if (Event != nullptr)
  {
    uintptr_t Result = 0;
    Event(Result);
    if (Result != 0)
    {
      Dialog->ModalResult = Result;
    }
  }
}

class TMessageTimeout : public TTimer
{
public:
  TMessageTimeout(TComponent * AOwner, uintptr_t Timeout,
    TButton * Button);

  void MouseMove();
  void Cancel();

protected:
  uintptr_t FOrigTimeout;
  uintptr_t FTimeout;
  TButton * FButton;
  UnicodeString FOrigCaption;
  TPoint FOrigCursorPos;

  void DoTimer(TObject * Sender);
  void UpdateButton();
};

TMessageTimeout::TMessageTimeout(TComponent * AOwner,
  uintptr_t Timeout, TButton * Button) :
  TTimer(AOwner), FOrigTimeout(Timeout), FTimeout(Timeout), FButton(Button)
{
  OnTimer = DoTimer;
  Interval = MSecsPerSec;
  FOrigCaption = FButton->Caption;
  FOrigCursorPos = Mouse->CursorPos;
  UpdateButton();
}

void TMessageTimeout::MouseMove()
{
  TPoint CursorPos = Mouse->CursorPos;
  int Delta = std::max(std::abs(FOrigCursorPos.X - CursorPos.X), std::abs(FOrigCursorPos.Y - CursorPos.Y));

  int Threshold = 8;
  if (DebugAlwaysTrue(FButton != nullptr))
  {
    Threshold = ScaleByTextHeight(FButton, Threshold);
  }

  if (Delta > Threshold)
  {
    FOrigCursorPos = CursorPos;
    const uintptr_t SuspendTime = 30 * MSecsPerSec;
    FTimeout = std::max(FOrigTimeout, SuspendTime);
    UpdateButton();
  }
}

void TMessageTimeout::Cancel()
{
  Enabled = false;
  UpdateButton();
}

void TMessageTimeout::UpdateButton()
{
  DebugAssert(FButton != nullptr);
  FButton->Caption =
    !Enabled ? FOrigCaption : FMTLOAD(TIMEOUT_BUTTON, (FOrigCaption, int(FTimeout / MSecsPerSec)));
}

void TMessageTimeout::DoTimer(TObject * /*Sender*/)
{
  if (FTimeout <= Interval)
  {
    DebugAssert(FButton != nullptr);
    TForm * Dialog = dynamic_cast<TForm *>(FButton->Parent);
    DebugAssert(Dialog != nullptr);

    Dialog->ModalResult = FButton->ModalResult;
  }
  else
  {
    FTimeout -= Interval;
    UpdateButton();
  }
}
//---------------------------------------------------------------------
class TPublicControl : public TControl
{
friend void MenuPopup(TObject * Sender, const TPoint & MousePos, bool & Handled);
friend void SetTimeoutEvents(TControl * Control, TMessageTimeout * Timeout);
};
//---------------------------------------------------------------------
class TPublicWinControl : public TWinControl
{
friend void SetTimeoutEvents(TControl * Control, TMessageTimeout * Timeout);
};

static void MessageDialogMouseMove(void * Data, TObject * /*Sender*/,
  TShiftState /*Shift*/, int /*X*/, int /*Y*/)
{
  DebugAssert(Data != nullptr);
  TMessageTimeout * Timeout = static_cast<TMessageTimeout *>(Data);
  Timeout->MouseMove();
}

static void MessageDialogMouseDown(void * Data, TObject * /*Sender*/,
  TMouseButton /*Button*/, TShiftState /*Shift*/, int /*X*/, int /*Y*/)
{
  DebugAssert(Data != nullptr);
  TMessageTimeout * Timeout = static_cast<TMessageTimeout *>(Data);
  Timeout->Cancel();
}

static void MessageDialogKeyDownUp(void * Data, TObject * /*Sender*/,
  Word & /*Key*/, TShiftState /*Shift*/)
{
  DebugAssert(Data != nullptr);
  TMessageTimeout * Timeout = static_cast<TMessageTimeout *>(Data);
  Timeout->Cancel();
}

void SetTimeoutEvents(TControl * Control, TMessageTimeout * Timeout)
{
  TPublicControl * PublicControl = reinterpret_cast<TPublicControl *>(Control);
  DebugAssert(PublicControl->OnMouseMove == nullptr);
  PublicControl->OnMouseMove = MakeMethod<TMouseMoveEvent>(Timeout, MessageDialogMouseMove);
  DebugAssert(PublicControl->OnMouseDown == nullptr);
  PublicControl->OnMouseDown = MakeMethod<TMouseEvent>(Timeout, MessageDialogMouseDown);

  TWinControl * WinControl = dynamic_cast<TWinControl *>(Control);
  if (WinControl != nullptr)
  {
    TPublicWinControl * PublicWinControl = reinterpret_cast<TPublicWinControl *>(Control);
    DebugAssert(PublicWinControl->OnKeyDown == nullptr);
    PublicWinControl->OnKeyDown = MakeMethod<TKeyEvent>(Timeout, MessageDialogKeyDownUp);
    DebugAssert(PublicWinControl->OnKeyUp == nullptr);
    PublicWinControl->OnKeyUp = MakeMethod<TKeyEvent>(Timeout, MessageDialogKeyDownUp);

    for (int Index = 0; Index < WinControl->ControlCount; Index++)
    {
      SetTimeoutEvents(WinControl->Controls[Index], Timeout);
    }
  }
}

// Merge with CreateMessageDialogEx
TForm * CreateMoreMessageDialogEx(const UnicodeString Message, TStrings * MoreMessages,
  TQueryType Type, uintptr_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params)
{
  std::unique_ptr<TForm> Dialog;
  UnicodeString AMessage = Message;
  TMessageTimer * Timer = nullptr;

  if ((Params != nullptr) && (Params->Timer > 0))
  {
    Timer = new TMessageTimer(Application);
    Timer->Interval = Params->Timer;
    Timer->Event = Params->TimerEvent;
    if (Params->TimerAnswers > 0)
    {
      Answers = Params->TimerAnswers;
    }
    if (Params->TimerQueryType >= 0)
    {
      Type = Params->TimerQueryType;
    }
    if (!Params->TimerMessage.IsEmpty())
    {
      AMessage = Params->TimerMessage;
    }
    Timer->Name = L"MessageTimer";
  }

  TButton * TimeoutButton = nullptr;
  Dialog.reset(
    CreateMessageDialogEx(
      AMessage, MoreMessages, Type, Answers, HelpKeyword, Params, TimeoutButton));

  if (Timer != nullptr)
  {
    Timer->Dialog = Dialog.get();
    Dialog->InsertComponent(Timer);
  }

  if (Params != nullptr)
  {
    if (Params->Timeout > 0)
    {
      TMessageTimeout * Timeout = new TMessageTimeout(Application, Params->Timeout, TimeoutButton);
      SetTimeoutEvents(Dialog.get(), Timeout);
      Timeout->Name = L"MessageTimeout";
      Dialog->InsertComponent(Timeout);
    }
  }

  return Dialog.release();
}

uintptr_t MoreMessageDialog(const UnicodeString Message, TStrings * MoreMessages,
  TQueryType Type, uintptr_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params)
{
  std::unique_ptr<TForm> Dialog(CreateMoreMessageDialogEx(Message, MoreMessages, Type, Answers, HelpKeyword, Params));
  uintptr_t Result = ExecuteMessageDialog(Dialog.get(), Answers, Params);
  return Result;
}

uintptr_t MessageDialog(const UnicodeString Msg, TQueryType Type,
  uintptr_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params)
{
  return MoreMessageDialog(Msg, nullptr, Type, Answers, HelpKeyword, Params);
}

uintptr_t SimpleErrorDialog(const UnicodeString Msg, const UnicodeString MoreMessages)
{
  uintptr_t Result;
  TStrings * More = nullptr;
  try
  {
    if (!MoreMessages.IsEmpty())
    {
      More = TextToStringList(MoreMessages);
    }
    Result = MoreMessageDialog(Msg, More, qtError, qaOK, HELP_NONE);
  }
  __finally
  {
    delete More;
  }
  return Result;
}

static TStrings * StackInfoListToStrings(
  TJclStackInfoList * StackInfoList)
{
  std::unique_ptr<TStrings> StackTrace(new TStringList());
  StackInfoList->AddToStrings(StackTrace.get(), true, false, true, true);
  for (int Index = 0; Index < StackTrace->Count; Index++)
  {
    UnicodeString Frame = StackTrace->Strings[Index];
    // get rid of declarations "flags" that are included in .map
    Frame = ReplaceStr(Frame, L"", L"");
    Frame = ReplaceStr(Frame, L"__linkproc__ ", L"");
    if (DebugAlwaysTrue(!Frame.IsEmpty() && (Frame[1] == L'(')))
    {
      int Start = Frame.Pos(L"[");
      int End = Frame.Pos(L"]");
      if (DebugAlwaysTrue((Start > 1) && (End > Start) && (Frame[Start - 1] == L' ')))
      {
        // remove absolute address
        Frame.Delete(Start - 1, End - Start + 2);
      }
    }
    StackTrace->Strings[Index] = Frame;
  }
  return StackTrace.release();
}

static TCriticalSection StackTraceCriticalSection;
typedef rde::map<DWORD, TStrings *> TStackTraceMap;
static TStackTraceMap StackTraceMap;

bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages)
{
  bool Result = false;

  TGuard Guard(StackTraceCriticalSection);

  DWORD Id = ::GetCurrentThreadId();
  TStackTraceMap::iterator Iterator = StackTraceMap.find(Id);
  if (Iterator != StackTraceMap.end())
  {
    std::unique_ptr<TStrings> OwnedMoreMessages;
    if (MoreMessages == nullptr)
    {
      OwnedMoreMessages.reset(new TStringList());
      MoreMessages = OwnedMoreMessages.get();
      Result = true;
    }
    if (!MoreMessages->GetText().IsEmpty())
    {
      MoreMessages->SetText(MoreMessages->GetText() + "\n");
    }
    MoreMessages->SetText(MoreMessages->GetText() + LoadStr(MSG_STACK_TRACE) + "\n");
    MoreMessages->AddStrings(Iterator->second);

    delete Iterator->second;
    StackTraceMap.erase(Id);

    OwnedMoreMessages.release();
  }
  return Result;
}

uintptr_t ExceptionMessageDialog(Exception * E, TQueryType Type,
  const UnicodeString MessageFormat, uintptr_t Answers, UnicodeString HelpKeyword,
  const TMessageParams * Params)
{
  TStrings * MoreMessages = nullptr;
  ExtException * EE = dynamic_cast<ExtException *>(E);
  if (EE != nullptr)
  {
    MoreMessages = EE->MoreMessages;
  }

  UnicodeString Message;
  // this is always called from within ExceptionMessage check,
  // so it should never fail here
  DebugCheck(ExceptionMessageFormatted(E, Message));

  HelpKeyword = MergeHelpKeyword(HelpKeyword, GetExceptionHelpKeyword(E));

  std::unique_ptr<TStrings> OwnedMoreMessages;
  if (AppendExceptionStackTraceAndForget(MoreMessages))
  {
    OwnedMoreMessages.reset(MoreMessages);
  }

  return MoreMessageDialog(
    FORMAT(MessageFormat.IsEmpty() ? UnicodeString(L"%s") : MessageFormat, (Message)),
    MoreMessages, Type, Answers, HelpKeyword, Params);
}

uintptr_t FatalExceptionMessageDialog(Exception * E, TQueryType Type,
  int SessionReopenTimeout, const UnicodeString MessageFormat, uintptr_t Answers,
  UnicodeString HelpKeyword, const TMessageParams * Params)
{
  DebugAssert(FLAGCLEAR(Answers, qaRetry));
  Answers |= qaRetry;

  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);

  TMessageParams AParams;
  if (Params != nullptr)
  {
    AParams = *Params;
  }
  DebugAssert(AParams.Timeout == 0);
  // the condition is de facto excess
  if (SessionReopenTimeout > 0)
  {
    AParams.Timeout = SessionReopenTimeout;
    AParams.TimeoutAnswer = qaRetry;
  }
  DebugAssert(AParams.Aliases == nullptr);
  AParams.Aliases = Aliases;
  AParams.AliasesCount = LENOF(Aliases);

  return ExceptionMessageDialog(E, Type, MessageFormat, Answers, HelpKeyword, &AParams);
}

static void DoExceptNotify(TObject * ExceptObj, void * ExceptAddr,
  bool OSException, void * BaseOfStack)
{
  if (ExceptObj != nullptr)
  {
    Exception * E = dynamic_cast<Exception *>(ExceptObj);
    if ((E != nullptr) && IsInternalException(E)) // optimization
    {
      DoExceptionStackTrace(ExceptObj, ExceptAddr, OSException, BaseOfStack);

      TJclStackInfoList * StackInfoList = JclLastExceptStackList();

      if (DebugAlwaysTrue(StackInfoList != nullptr))
      {
        std::unique_ptr<TStrings> StackTrace(StackInfoListToStrings(StackInfoList));

        DWORD ThreadID = GetCurrentThreadId();

        TGuard Guard(StackTraceCriticalSection.get());

        TStackTraceMap::iterator Iterator = StackTraceMap.find(ThreadID);
        if (Iterator != StackTraceMap.end())
        {
          Iterator->second->Add(L"");
          Iterator->second->AddStrings(StackTrace.get());
        }
        else
        {
          StackTraceMap.insert(std::make_pair(ThreadID, StackTrace.release()));
        }

        // this chains so that JclLastExceptStackList() returns nullptr the next time
        // for the current thread
        delete StackInfoList;
      }
    }
  }
}

void * BusyStart()
{
  void * Token = reinterpret_cast<void *>(Screen->Cursor);
  Screen->Cursor = crHourGlass;
  return Token;
}

void BusyEnd(void * Token)
{
  Screen->Cursor = reinterpret_cast<TCursor>(Token);
}


static DWORD MainThread = 0;
static TDateTime LastGUIUpdate = 0;
static double GUIUpdateIntervalFrac = static_cast<double>(OneSecond/1000*GUIUpdateInterval);  // 1/5 sec
static bool NoGUI = false;

void SetNoGUI()
{
  NoGUI = true;
}

bool ProcessGUI(bool Force)
{
  DebugAssert(MainThread != 0);
  bool Result = false;
  // Calling ProcessMessages in Azure WebJob causes access violation in VCL.
  // As we do not really need to call it in scripting/.NET, just skip it.
  if ((MainThread == GetCurrentThreadId()) && !NoGUI)
  {
    TDateTime N = Now();
    if (Force ||
        (double(N) - double(LastGUIUpdate) > GUIUpdateIntervalFrac))
    {
      LastGUIUpdate = N;
      Application->ProcessMessages();
      Result = true;
    }
  }
  return Result;
}

void CopyParamListButton(TButton * Button)
{
  if (!SupportsSplitButton())
  {
    MenuButton(Button);
  }
}

const int cpiDefault = -1;
const int cpiConfigure = -2;
const int cpiCustom = -3;
const int cpiSaveSettings = -4;

void CopyParamListPopup(TRect Rect, TPopupMenu * Menu,
  const TCopyParamType & Param, UnicodeString Preset, TNotifyEvent OnClick,
  int Options, int CopyParamAttrs, bool SaveSettings)
{
  Menu->Items->Clear();

  TMenuItem * CustomizeItem = nullptr;
  TMenuItem * Item;

  if (FLAGSET(Options, cplCustomize))
  {
    Item = new TMenuItem(Menu);
    Item->Caption = LoadStr(COPY_PARAM_CUSTOM);
    Item->Tag = cpiCustom;
    Item->Default = FLAGSET(Options, cplCustomizeDefault);
    Item->OnClick = OnClick;
    Menu->Items->Add(Item);
    CustomizeItem = Item;
  }

  if (FLAGSET(Options, cplSaveSettings))
  {
    Item = new TMenuItem(Menu);
    Item->Caption = LoadStr(COPY_PARAM_SAVE_SETTINGS);
    Item->Tag = cpiSaveSettings;
    Item->Checked = SaveSettings;
    Item->OnClick = OnClick;
    Menu->Items->Add(Item);
  }

  Item = new TMenuItem(Menu);
  Item->Caption = LoadStr(COPY_PARAM_PRESET_HEADER);
  Item->Visible = false;
  Item->Enabled = false;
  Menu->Items->Add(Item);

  bool AnyChecked = false;
  Item = new TMenuItem(Menu);
  Item->Caption = LoadStr(COPY_PARAM_DEFAULT);
  Item->Tag = cpiDefault;
  Item->Checked =
    Preset.IsEmpty() && (GUIConfiguration->CopyParamPreset[L""] == Param);
  AnyChecked = AnyChecked || Item->Checked;
  Item->OnClick = OnClick;
  Menu->Items->Add(Item);

  TCopyParamType DefaultParam;
  const TCopyParamList * CopyParamList = GUIConfiguration->CopyParamList;
  for (int i = 0; i < CopyParamList->Count; i++)
  {
    UnicodeString Name = CopyParamList->Names[i];
    TCopyParamType AParam = GUIConfiguration->CopyParamPreset[Name];
    if (AParam.AnyUsableCopyParam(CopyParamAttrs) ||
        // This makes "Binary" preset visible,
        // as long as we care about transfer mode
        ((AParam == DefaultParam) &&
         FLAGCLEAR(CopyParamAttrs, cpaIncludeMaskOnly) &&
         FLAGCLEAR(CopyParamAttrs, cpaNoTransferMode)))
    {
      Item = new TMenuItem(Menu);
      Item->Caption = Name;
      Item->Tag = i;
      Item->Checked =
        (Preset == Name) && (AParam == Param);
      AnyChecked = AnyChecked || Item->Checked;
      Item->OnClick = OnClick;
      Menu->Items->Add(Item);
    }
  }

  if (CustomizeItem != nullptr)
  {
    CustomizeItem->Checked = !AnyChecked;
  }

  Item = new TMenuItem(Menu);
  Item->Caption = L"-";
  Menu->Items->Add(Item);

  Item = new TMenuItem(Menu);
  Item->Caption = LoadStr(COPY_PARAM_CONFIGURE);
  Item->Tag = cpiConfigure;
  Item->OnClick = OnClick;
  Menu->Items->Add(Item);

  MenuPopup(Menu, Rect, nullptr);
}

bool CopyParamListPopupClick(TObject * Sender,
  TCopyParamType & Param, UnicodeString & Preset, int CopyParamAttrs,
  bool * SaveSettings)
{
  TComponent * Item = dynamic_cast<TComponent *>(Sender);
  DebugAssert(Item != nullptr);
  DebugAssert((Item->Tag >= cpiSaveSettings) && (Item->Tag < GUIConfiguration->CopyParamList->Count));

  bool Result;
  if (Item->Tag == cpiConfigure)
  {
    bool MatchedPreset = (GUIConfiguration->CopyParamPreset[Preset] == Param);
    DoPreferencesDialog(pmPresets);
    Result = (MatchedPreset && GUIConfiguration->HasCopyParamPreset[Preset]);
    if (Result)
    {
      // For cast, see a comment below
      Param = TCopyParamType(GUIConfiguration->CopyParamPreset[Preset]);
    }
  }
  else if (Item->Tag == cpiCustom)
  {
    Result = DoCopyParamCustomDialog(Param, CopyParamAttrs);
  }
  else if (Item->Tag == cpiSaveSettings)
  {
    if (DebugAlwaysTrue(SaveSettings != nullptr))
    {
      *SaveSettings = !*SaveSettings;
    }
    Result = false;
  }
  else
  {
    Preset = (Item->Tag >= 0) ?
      GUIConfiguration->CopyParamList->Names[Item->Tag] : UnicodeString();
    // The cast strips away the "queue" properties of the TGUICopyParamType
    // that are not configurable in presets
    Param = TCopyParamType(GUIConfiguration->CopyParamPreset[Preset]);
    Result = true;
  }
  return Result;
}

TWinInteractiveCustomCommand::TWinInteractiveCustomCommand(
  TCustomCommand * ChildCustomCommand, const UnicodeString CustomCommandName) :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FCustomCommandName = StripHotkey(CustomCommandName);
}

void TWinInteractiveCustomCommand::Prompt(
  const UnicodeString & Prompt, UnicodeString & Value)
{
  UnicodeString APrompt = Prompt;
  if (APrompt.IsEmpty())
  {
    APrompt = FMTLOAD(CUSTOM_COMMANDS_PARAM_PROMPT, (FCustomCommandName));
  }
  std::unique_ptr<TStrings> History(CloneStrings(CustomWinConfiguration->History[L"CustomCommandParam"]));
  if (InputDialog(FMTLOAD(CUSTOM_COMMANDS_PARAM_TITLE, (FCustomCommandName)),
        APrompt, Value, HELP_CUSTOM_COMMAND_PARAM, History.get()))
  {
    CustomWinConfiguration->History[L"CustomCommandParam"] = History.get();
  }
  else
  {
    Abort();
  }
}

void TWinInteractiveCustomCommand::Execute(
  const UnicodeString & Command, UnicodeString & Value)
{
  // inspired by
  // http://forum.codecall.net/topic/72472-execute-a-console-program-and-capture-its-output/
  HANDLE StdOutOutput;
  HANDLE StdOutInput;
  HANDLE StdInOutput;
  HANDLE StdInInput;
  SECURITY_ATTRIBUTES SecurityAttributes;
  SecurityAttributes.nLength = sizeof(SecurityAttributes);
  SecurityAttributes.lpSecurityDescriptor = nullptr;
  SecurityAttributes.bInheritHandle = TRUE;
  try
  {
    if (!CreatePipe(&StdOutOutput, &StdOutInput, &SecurityAttributes, 0))
    {
      throw Exception(FMTLOAD(SHELL_PATTERN_ERROR, (Command, L"out")));
    }
    else if (!CreatePipe(&StdInOutput, &StdInInput, &SecurityAttributes, 0))
    {
      throw Exception(FMTLOAD(SHELL_PATTERN_ERROR, (Command, L"in")));
    }
    else
    {
      STARTUPINFO StartupInfo;
      PROCESS_INFORMATION ProcessInformation;

      FillMemory(&StartupInfo, sizeof(StartupInfo), 0);
      StartupInfo.cb = sizeof(StartupInfo);
      StartupInfo.wShowWindow = SW_HIDE;
      StartupInfo.hStdInput = StdInOutput;
      StartupInfo.hStdOutput = StdOutInput;
      StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

      if (!CreateProcess(nullptr, Command.c_str(), &SecurityAttributes, &SecurityAttributes,
            TRUE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &StartupInfo, &ProcessInformation))
      {
        throw Exception(FMTLOAD(SHELL_PATTERN_ERROR, (Command, L"process")));
      }
      else
      {
        try
        {
          // wait until the console program terminated
          bool Running = true;
          while (Running)
          {
            switch (WaitForSingleObject(ProcessInformation.hProcess, 200))
            {
              case WAIT_TIMEOUT:
                Application->ProcessMessages();
                break;

              case WAIT_OBJECT_0:
                Running = false;
                break;

              default:
                throw Exception(FMTLOAD(SHELL_PATTERN_ERROR, (Command, L"wait")));
            }
          }

          char Buffer[1024];
          unsigned long Read;
          while (PeekNamedPipe(StdOutOutput, nullptr, 0, nullptr, &Read, nullptr) &&
                 (Read > 0))

          {
            if (!ReadFile(StdOutOutput, &Buffer, Read, &Read, nullptr))
            {
              throw Exception(FMTLOAD(SHELL_PATTERN_ERROR, (Command, L"read")));
            }
            else if (Read > 0)
            {
              Value += AnsiToString(Buffer, Read);
            }
          }

          // trim trailing cr/lf
          Value = TrimRight(Value);
        }
        __finally
        {
          CloseHandle(ProcessInformation.hProcess);
          CloseHandle(ProcessInformation.hThread);
        }
      }
    }
  }
  __finally
  {
    if (StdOutOutput != INVALID_HANDLE_VALUE)
    {
      CloseHandle(StdOutOutput);
    }
    if (StdOutInput != INVALID_HANDLE_VALUE)
    {
      CloseHandle(StdOutInput);
    }
    if (StdInOutput != INVALID_HANDLE_VALUE)
    {
      CloseHandle(StdInOutput);
    }
    if (StdInInput != INVALID_HANDLE_VALUE)
    {
      CloseHandle(StdInInput);
    }
  }
}

void MenuPopup(TPopupMenu * Menu, TButton * Button)
{
  MenuPopup(Menu, CalculatePopupRect(Button), Button);
}

void MenuPopup(TObject * Sender, const TPoint & MousePos, bool & Handled)
{
  TControl * Control = dynamic_cast<TControl *>(Sender);
  DebugAssert(Control != nullptr);
  TPoint Point;
  if ((MousePos.x == -1) && (MousePos.y == -1))
  {
    Point = Control->ClientToScreen(TPoint(0, 0));
  }
  else
  {
    Point = Control->ClientToScreen(MousePos);
  }
  TPopupMenu * PopupMenu = (reinterpret_cast<TPublicControl *>(Control))->PopupMenu;
  DebugAssert(PopupMenu != nullptr);
  TRect Rect(Point, Point);
  MenuPopup(PopupMenu, Rect, Control);
  Handled = true;
}

TComponent * GetPopupComponent(TObject * Sender)
{
  TComponent * Item = dynamic_cast<TComponent *>(Sender);
  DebugAssert(Item != nullptr);
  TPopupMenu * PopupMenu = dynamic_cast<TPopupMenu *>(Item->Owner);
  DebugAssert(PopupMenu != nullptr);
  DebugAssert(PopupMenu->PopupComponent != nullptr);
  return PopupMenu->PopupComponent;
}

void MenuButton(TButton * Button)
{
  Button->Images = GlyphsModule->ButtonImages;
  Button->ImageIndex = 0;
  Button->DisabledImageIndex = 1;
  Button->ImageAlignment = iaRight;
}

TRect CalculatePopupRect(TButton * Button)
{
  TPoint UpPoint = Button->ClientToScreen(TPoint(0, 0));
  TPoint DownPoint = Button->ClientToScreen(TPoint(Button->Width, Button->Height));
  TRect Rect(UpPoint, DownPoint);
  // With themes enabled, button are rendered 1 pixel smaller than their actual size
  int Offset = UseThemes() ? -1 : 0;
  Rect.Inflate(Offset, Offset);
  return Rect;
}

TRect CalculatePopupRect(TControl * Control, TPoint MousePos)
{
  MousePos = Control->ClientToScreen(MousePos);
  TRect Rect(MousePos, MousePos);
  return Rect;
}

void FixButtonImage(TButton * Button)
{
  // with themes enabled, button image is by default drawn too high
  if (UseThemes())
  {
    Button->ImageMargins->Top = 1;
  }
}

void CenterButtonImage(TButton * Button)
{
  // with themes disabled, the text seems to be drawn over the icon,
  // so that the padding spaces hide away most of the icon
  if (UseThemes())
  {
    Button->ImageAlignment = iaCenter;
    int ImageWidth = Button->Images->Width;

    std::unique_ptr<TControlCanvas> Canvas(new TControlCanvas());
    Canvas->Control = Button;

    UnicodeString Caption = Button->Caption;
    UnicodeString Padding;
    while (Canvas->TextWidth(Padding) < ImageWidth)
    {
      Padding += L" ";
    }
    if (Button->IsRightToLeft())
    {
      Caption = Caption + Padding;
    }
    else
    {
      Caption = Padding + Caption;
    }
    Button->Caption = Caption;

    int CaptionWidth = Canvas->TextWidth(Caption);
    // The margins seem to extend the area over which the image is centered,
    // so we have to set it to a double of desired padding.
    // The original formula is - 2 * ((CaptionWidth / 2) - (ImageWidth / 2) + ScaleByTextHeight(Button, 2))
    // the one below is equivalent, but with reduced rouding.
    // Without the change, the rouding caused the space between icon and caption too
    // small on 200% zoom.
    // Note that (CaptionWidth / 2) - (ImageWidth / 2)
    // is approximatelly same as half of caption width before padding.
    Button->ImageMargins->Left = -(CaptionWidth - ImageWidth + ScaleByTextHeight(Button, 4));
  }
  else
  {
    // at least do not draw it so near to the edge
    Button->ImageMargins->Left = 1;
  }
}

int AdjustLocaleFlag(const UnicodeString & S, TLocaleFlagOverride LocaleFlagOverride, bool Recommended, int On, int Off)
{
  int Result = !S.IsEmpty() && StrToInt(S);
  switch (LocaleFlagOverride)
  {
    default:
    case lfoLanguageIfRecommended:
      if (!Recommended)
      {
        Result = Off;
      }
      break;

    case lfoLanguage:
      // noop = as configured in locale
      break;

    case lfoAlways:
      Result = On;
      break;

    case lfoNever:
      Result = Off;
      break;
  }
  return Result;
}

void SetGlobalMinimizeHandler(TCustomForm * /*Form*/, TNotifyEvent OnMinimize)
{
  if (GlobalOnMinimize == nullptr)
  {
    GlobalOnMinimize = OnMinimize;
  }
}

void ClearGlobalMinimizeHandler(TNotifyEvent OnMinimize)
{
  if (GlobalOnMinimize == OnMinimize)
  {
    GlobalOnMinimize = nullptr;
  }
}

void CallGlobalMinimizeHandler(TObject * Sender)
{
  Configuration->Usage->Inc(L"OperationMinimizations");
  if (DebugAlwaysTrue(GlobalOnMinimize != nullptr))
  {
    GlobalOnMinimize(Sender);
  }
}

static void DoApplicationMinimizeRestore(bool Minimize)
{
  // WORKAROUND
  // When main window is hidden (command-line operation),
  // we do not want it to be shown by TApplication.Restore,
  // so we temporarily detach it from an application.
  // Probably not really necessary for minimizing phase,
  // but we do it for consistency anyway.
  TForm * MainForm = Application->MainForm;
  bool RestoreMainForm = false;
  if (DebugAlwaysTrue(MainForm != nullptr) &&
      !MainForm->Visible)
  {
    SetAppMainForm(nullptr);
    RestoreMainForm = true;
  }
  try
  {
    if (Minimize)
    {
      Application->Minimize();
    }
    else
    {
      Application->Restore();
    }
  }
  __finally
  {
    if (RestoreMainForm)
    {
      SetAppMainForm(MainForm);
    }
  }
}

void ApplicationMinimize()
{
  DoApplicationMinimizeRestore(true);
}

void ApplicationRestore()
{
  DoApplicationMinimizeRestore(false);
}

bool IsApplicationMinimized()
{
  // VCL help recommends handling Application->OnMinimize/OnRestore
  // for tracking state, but OnRestore is actually not called
  // (OnMinimize is), when app is minimized from e.g. Progress window
  bool AppMinimized = IsIconic(Application->Handle);
  bool MainFormMinimized = IsIconic(Application->MainFormHandle);
  return AppMinimized || MainFormMinimized;
}

bool HandleMinimizeSysCommand(TMessage & Message)
{
  TWMSysCommand & SysCommand = reinterpret_cast<TWMSysCommand &>(Message);
  uintptr_t Cmd = (SysCommand.CmdType & 0xFFF0);
  bool Result = (Cmd == SC_MINIMIZE);
  if (Result)
  {
    ApplicationMinimize();
    SysCommand.Result = 1;
  }
  return Result;
}

void WinInitialize()
{
  if (JclHookExceptions())
  {
    JclStackTrackingOptions << stAllModules;
    JclAddExceptNotifier(DoExceptNotify, npFirstChain);
  }

  SetErrorMode(SEM_FAILCRITICALERRORS);
  OnApiPath = ApiPath;
  MainThread = GetCurrentThreadId();

#pragma warn -8111
#pragma warn .8111

}

void WinFinalize()
{
  JclRemoveExceptNotifier(DoExceptNotify);
}

::TTrayIcon::TTrayIcon(uintptr_t Id)
{
  FVisible = false;
  FOnClick = nullptr;
  FOnBalloonClick = nullptr;
  FBalloonUserData = nullptr;

  FTrayIcon = new NOTIFYICONDATA;
  memset(FTrayIcon, 0, sizeof(*FTrayIcon));
  FTrayIcon->cbSize = sizeof(*FTrayIcon);
  FTrayIcon->uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

  // LoadIconMetric is available from Windows Vista only
  HMODULE ComCtl32Dll = GetModuleHandle(comctl32);
  if (DebugAlwaysTrue(ComCtl32Dll))
  {
    typedef HRESULT WINAPI (* TLoadIconMetric)(HINSTANCE hinst, PCWSTR pszName, int lims, __out HICON *phico);
    TLoadIconMetric LoadIconMetric = (TLoadIconMetric)GetProcAddress(ComCtl32Dll, "LoadIconMetric");
    if (LoadIconMetric != nullptr)
    {
      // Prefer not to use Application->Icon->Handle as that shows 32x32 scaled down to 16x16 for some reason
      LoadIconMetric(MainInstance, L"MAINICON", LIM_SMALL, &FTrayIcon->hIcon);
    }
  }

  if (FTrayIcon->hIcon == 0)
  {
    FTrayIcon->hIcon = Application->Icon->Handle;
  }

  FTrayIcon->uID = Id;
  FTrayIcon->hWnd = AllocateHWnd(WndProc);
  FTrayIcon->uCallbackMessage = WM_TRAY_ICON;

  FTaskbarCreatedMsg = RegisterWindowMessage(L"TaskbarCreated");
}

::TTrayIcon::~TTrayIcon()
{
  // make sure we hide icon even in case it was shown just to pop up the balloon
  // (in which case Visible == false)
  CancelBalloon();
  Visible = false;
  DeallocateHWnd(FTrayIcon->hWnd);
  delete FTrayIcon;
}

void ::TTrayIcon::PopupBalloon(UnicodeString Title,
  const UnicodeString & Str, TQueryType QueryType, uintptr_t Timeout,
  TNotifyEvent OnBalloonClick, TObject * BalloonUserData)
{
  if (Timeout > 30000)
  {
    // this is probably system limit, do not try more, especially for
    // the timeout-driven hiding of the tray icon (for Win2k)
    Timeout = 30000;
  }
  FTrayIcon->uFlags |= NIF_INFO;
  Title = FORMAT(L"%s - %s", (Title, AppNameString()));
  StrPLCopy(FTrayIcon->szInfoTitle, Title, LENOF(FTrayIcon->szInfoTitle) - 1);
  UnicodeString Info = Str;
  // When szInfo is empty, balloon is not shown
  // (or actually it means the balloon should be deleted, if any)
  if (Info.IsEmpty())
  {
    Info = L" ";
  }
  StrPLCopy(FTrayIcon->szInfo, Info, LENOF(FTrayIcon->szInfo) - 1);
  FTrayIcon->uTimeout = Timeout;
  switch (QueryType)
  {
    case qtError:
      FTrayIcon->dwInfoFlags = NIIF_ERROR;
      break;

    case qtInformation:
    case qtConfirmation:
      FTrayIcon->dwInfoFlags = NIIF_INFO;
      break;

    case qtWarning:
    default:
      FTrayIcon->dwInfoFlags = NIIF_WARNING;
      break;
  }

  KillTimer(FTrayIcon->hWnd, 1);
  if (Visible)
  {
    Update();
  }
  else
  {
    Notify(NIM_ADD);
  }

  FOnBalloonClick = OnBalloonClick;
  delete FBalloonUserData;
  FBalloonUserData = BalloonUserData;

  // Clearing the flag ensures that subsequent updates does not hide the baloon
  // unless CancelBalloon is called explicitly
  FTrayIcon->uFlags = FTrayIcon->uFlags & ~NIF_INFO;
}

void ::TTrayIcon::BalloonCancelled()
{
  FOnBalloonClick = nullptr;
  delete FBalloonUserData;
  FBalloonUserData = nullptr;
}

void ::TTrayIcon::CancelBalloon()
{
  KillTimer(FTrayIcon->hWnd, 1);
  if (Visible)
  {
    FTrayIcon->uFlags |= NIF_INFO;
    FTrayIcon->szInfo[0] = L'\0';
    Update();
    FTrayIcon->uFlags = FTrayIcon->uFlags & ~NIF_INFO;
  }
  else
  {
    Notify(NIM_DELETE);
  }

  BalloonCancelled();
}

bool ::TTrayIcon::Notify(uintptr_t Message)
{
  bool Result = SUCCEEDED(Shell_NotifyIcon(Message, (NOTIFYICONDATA*)FTrayIcon));
  if (Result && (Message == NIM_ADD))
  {
    UINT Timeout = FTrayIcon->uTimeout;
    try
    {
      FTrayIcon->uVersion = NOTIFYICON_VERSION;
      Result = SUCCEEDED(Shell_NotifyIcon(NIM_SETVERSION, (NOTIFYICONDATA*)FTrayIcon));
    }
    __finally
    {
      FTrayIcon->uTimeout = Timeout;
    }
  }
  return Result;
}

void ::TTrayIcon::Update()
{
  if (Visible)
  {
    Notify(NIM_MODIFY);
  }
}

void ::TTrayIcon::SetVisible(bool value)
{
  if (Visible != value)
  {
    if (value)
    {
      FVisible = Notify(NIM_ADD);
    }
    else
    {
      FVisible = false;
      KillTimer(FTrayIcon->hWnd, 1);
      Notify(NIM_DELETE);
      BalloonCancelled();
    }
  }
}

void ::TTrayIcon::WndProc(TMessage & Message)
{
  try
  {
    if (Message.Msg == WM_TRAY_ICON)
    {
      DebugAssert(Message.WParam == 0);
      switch (Message.LParam)
      {
        // old shell32
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        // new shell32:
        case WM_CONTEXTMENU:
          if (OnClick != nullptr)
          {
            OnClick(nullptr);
          }
          Message.Result = true;
          break;
      }

      if (Message.LParam == NIN_BALLOONUSERCLICK)
      {
        if (FOnBalloonClick != nullptr)
        {
          // prevent the user data from being freed by possible call
          // to CancelBalloon or PopupBalloon during call to OnBalloonClick
          std::unique_ptr<TObject> UserData(FBalloonUserData);
          FBalloonUserData = nullptr;
          FOnBalloonClick(UserData.get());
        }
        else if (OnClick != nullptr)
        {
          OnClick(nullptr);
        }
      }

      switch (Message.LParam)
      {
        case NIN_BALLOONHIDE:
        case NIN_BALLOONTIMEOUT:
        case NIN_BALLOONUSERCLICK:
          KillTimer(FTrayIcon->hWnd, 1);
          // if icon was shown just to display balloon, hide it with the balloon
          if (!Visible)
          {
            Notify(NIM_DELETE);
          }
          BalloonCancelled();
          break;
      }
    }
    else if (Message.Msg == WM_TIMER)
    {
      // sanity check
      Notify(NIM_DELETE);
      BalloonCancelled();
    }
    else if (Message.Msg == FTaskbarCreatedMsg)
    {
      if (Visible)
      {
        // force recreation
        Visible = false;
        Visible = true;
      }
    }
    else
    {
      Message.Result = DefWindowProc(FTrayIcon->hWnd, Message.Msg, Message.WParam, Message.LParam);
    }
  }
  catch(Exception & E)
  {
    Application->HandleException(&E);
  }
}

UnicodeString ::TTrayIcon::GetHint()
{
  return FTrayIcon->szTip;
}

void ::TTrayIcon::SetHint(UnicodeString value)
{
  if (Hint != value)
  {
    uintptr_t Max = LENOF(FTrayIcon->szTip);
    StrPLCopy(FTrayIcon->szTip, value, Max - 1);
    Update();
  }
}

