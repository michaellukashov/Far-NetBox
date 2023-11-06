
#include <vcl.h>
#pragma hdrstop

#include <shlwapi.h>

#include <Common.h>
#include <Queue.h>
#include <Exceptions.h>
#include <CoreMain.h>
#include <TextsCore.h>
#include <TextsWin.h>
#include <HelpWin.h>
#include <HelpCore.h>
#include <Interface.h>
#include <VCLCommon.h>
__removed #include <Glyphs.h>
#include <PasTools.hpp>
#include <DateUtils.hpp>
__removed #include <Custom.h>
__removed #include <HistoryComboBox.hpp>

#include "WinInterface.h"
#include "GUITools.h"
__removed #include "JclDebug.hpp"
__removed #include "JclHookExcept.hpp"
#include <System.IOUtils.hpp>
#include <StrUtils.hpp>
__removed #include <WinApi.h>
#include "Tools.h"
//#include <Vcl.AppEvnts.hpp>
#if 0

#pragma package(smart_init)

#define WM_TRAY_ICON (WM_WINSCP_USER + 5)

TNotifyEvent GlobalOnMinimize = nullptr;

const IID IID_IListView_Win7 = {0xE5B16AF2, 0x3990, 0x4681, {0xA6, 0x09, 0x1F, 0x06, 0x0C, 0xD1, 0x42, 0x69}};

void FormHelp(TCustomForm * Form)
{
  InvokeHelp(Form->ActiveControl != nullptr ? Form->ActiveControl : Form);
}
#endif // #if 0

HINSTANCE HInstance = 0;

TMessageParams::TMessageParams(uint32_t AParams) noexcept
{
  Reset();
  Params = AParams;
}

TMessageParams::TMessageParams(const TQueryParams * AParams) noexcept
{
  Reset();
  Assign(AParams);
}

void TMessageParams::Assign(const TMessageParams *AParams)
{
  Reset();

  if (AParams != nullptr)
  {
    Aliases = AParams->Aliases;
    AliasesCount = AParams->AliasesCount;
    Timer = AParams->Timer;
    TimerEvent = AParams->TimerEvent;
    TimerMessage = AParams->TimerMessage;
    TimerAnswers = AParams->TimerAnswers;
    TimerQueryType = AParams->TimerQueryType;
    Timeout = AParams->Timeout;
    TimeoutAnswer = AParams->TimeoutAnswer;
    TimeoutResponse = AParams->TimeoutResponse;

    NeverAskAgainTitle = AParams->NeverAskAgainTitle;
    NeverAskAgainAnswer = AParams->NeverAskAgainAnswer;
    NeverAskAgainCheckedInitially = AParams->NeverAskAgainCheckedInitially;
    AllowHelp = AParams->AllowHelp;
    ImageName = AParams->ImageName;
    MoreMessagesUrl = AParams->MoreMessagesUrl;
    MoreMessagesSize = AParams->MoreMessagesSize;
    CustomCaption = AParams->CustomCaption;

    if (FLAGSET(AParams->Params, qpNeverAskAgainCheck))
    {
      Params |= mpNeverAskAgainCheck;
    }
    if (FLAGSET(AParams->Params, qpAllowContinueOnError))
    {
      Params |= mpAllowContinueOnError;
    }
  }
}

void TMessageParams::Assign(const TQueryParams * AParams)
{
  Reset();

  if (AParams != nullptr)
  {
    Aliases = AParams->Aliases;
    AliasesCount = AParams->AliasesCount;
    Timer = AParams->Timer;
    TimerEvent = AParams->TimerEvent;
    TimerMessage = AParams->TimerMessage;
    TimerAnswers = AParams->TimerAnswers;
    TimerQueryType = AParams->TimerQueryType;
    Timeout = AParams->Timeout;
    TimeoutAnswer = AParams->TimeoutAnswer;
    TimeoutResponse = AParams->TimeoutResponse;

    if (FLAGSET(AParams->Params, qpNeverAskAgainCheck))
    {
      Params |= mpNeverAskAgainCheck;
    }
    if (FLAGSET(AParams->Params, qpAllowContinueOnError))
    {
      Params |= mpAllowContinueOnError;
    }
  }
}

void TMessageParams::Reset()
{
  Params = 0;
  Aliases = nullptr;
  AliasesCount = 0;
  Timer = 0;
  TimerEvent = nullptr;
  TimerMessage = L"";
  TimerAnswers = 0;
  TimerQueryType = static_cast<TQueryType>(-1);
  Timeout = 0;
  TimeoutAnswer = 0;
  TimeoutResponse = 0;
  NeverAskAgainTitle = L"";
  NeverAskAgainAnswer = 0;
  NeverAskAgainCheckedInitially = false;
  AllowHelp = true;
  ImageName = L"";
  MoreMessagesUrl = L"";
  MoreMessagesSize = TSize();
  CustomCaption = L"";
}

static bool IsPositiveAnswer(uint32_t Answer)
{
  return (Answer == qaYes) || (Answer == qaOK) || (Answer == qaYesToAll);
}

#if 0
static void NeverAskAgainCheckClick(void * /*Data*/, TObject * Sender)
{
  TCheckBox * CheckBox = dynamic_cast<TCheckBox *>(Sender);
  DebugAssert(CheckBox != nullptr);
  TForm * Dialog = dynamic_cast<TForm *>(CheckBox->Owner);
  DebugAssert(Dialog != nullptr);

  uint32_t PositiveAnswer = 0;

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
  TStrings * MoreMessages, TQueryType Type, uint32_t Answers, UnicodeString HelpKeyword,
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

  uint32_t TimeoutAnswer = (Params != nullptr) ? Params->TimeoutAnswer : 0;

  uint32_t ActualAnswers = Answers;
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
  uint32_t AliasesCount = (Params != nullptr) ? Params->AliasesCount : 0;

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

uint32_t ExecuteMessageDialog(TForm * Dialog, uint32_t Answers, const TMessageParams * Params)
{
  FlashOnBackground();
  uint32_t Answer = Dialog->ShowModal();
  // mrCancel is returned always when X button is pressed, despite
  // no Cancel button was on the dialog. Find valid "cancel" answer.
  // mrNone is returned when Windows session is closing (log off)
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
    uint32_t Result = 0;
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
  TMessageTimeout(TComponent * AOwner, uint32_t Timeout, TButton * Button, uint32_t Answer);

protected:
  uint32_t FOrigTimeout;
  uint32_t FTimeout;
  TButton * FButton;
  UnicodeString FOrigCaption;
  TPoint FOrigCursorPos;
  std::unique_ptr<TApplicationEvents> FApplicationEvents;
  uint32_t FAnswer;

  void DoTimer(TObject * Sender);
  void UpdateButton();
  void ApplicationMessage(TMsg & Msg, bool & Handled);
  void MouseMove();
  void Cancel();
};

TMessageTimeout::TMessageTimeout(TComponent * AOwner,
  uint32_t Timeout, TButton * Button, uint32_t Answer) :
  TTimer(AOwner), FOrigTimeout(Timeout), FTimeout(Timeout), FButton(Button), FAnswer(Answer)
{
  OnTimer = DoTimer;
  Interval = MSecsPerSec;
  FOrigCaption = FButton->Caption;
  FOrigCursorPos = Mouse->CursorPos;
  FApplicationEvents.reset(std::make_unique<TApplicationEvents>(Application));
  FApplicationEvents->OnMessage = ApplicationMessage;
  UpdateButton();
}

void TMessageTimeout::ApplicationMessage(TMsg & Msg, bool & DebugUsedArg(Handled))
{
  if (Msg.message == WM_MOUSEMOVE)
  {
    MouseMove();
  }
  else if ((Msg.message == WM_LBUTTONDOWN) || (Msg.message == WM_RBUTTONDOWN) ||
           (Msg.message == WM_KEYDOWN) || (Msg.message == WM_SYSKEYDOWN))
  {
    Cancel();
  }
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
    const uint32_t SuspendTime = 30 * MSecsPerSec;
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
    !Enabled ? FOrigCaption : FMTLOAD(TIMEOUT_BUTTON, FOrigCaption, int(FTimeout / MSecsPerSec));
}

void TMessageTimeout::DoTimer(TObject * /*Sender*/)
{
  if (FTimeout <= Interval)
  {
    DebugAssert(FButton != nullptr);

    // Needed particularly for "keep up to date" dialog, which does not close on the button click
    Enabled = false;
    TModalResult PrevModalResult = FButton->ModalResult;
    if (FAnswer != 0)
    {
      FButton->ModalResult = FAnswer;
    }
    try
    {
      FButton->Click();
    }
    __finally
    {
      FButton->ModalResult = PrevModalResult;
    }
  }
  else
  {
    FTimeout -= Interval;
    UpdateButton();
  }
}

void InitiateDialogTimeout(TForm * Dialog, uint32_t Timeout, TButton * Button, uint32_t Answer)
{
  TMessageTimeout * MessageTimeout = new TMessageTimeout(Application, Timeout, Button, Answer);
  MessageTimeout->Name = L"MessageTimeout";
  Dialog->InsertComponent(MessageTimeout);
}

class TPublicControl : public TControl
{
friend void MenuPopup(TObject * Sender, const TPoint & MousePos, bool & Handled);
};

// Merge with CreateMessageDialogEx
TForm * CreateMoreMessageDialogEx(const UnicodeString Message, TStrings * MoreMessages,
  TQueryType Type, uint32_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params)
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
      InitiateDialogTimeout(Dialog.get(), Params->Timeout, TimeoutButton, Params->TimeoutResponse);
    }
  }

  return Dialog.release();
}

uint32_t MoreMessageDialog(const UnicodeString Message, TStrings * MoreMessages,
  TQueryType Type, uint32_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params)
{
  std::unique_ptr<TForm> Dialog(CreateMoreMessageDialogEx(Message, MoreMessages, Type, Answers, HelpKeyword, Params));
  uint32_t Result = ExecuteMessageDialog(Dialog.get(), Answers, Params);
  return Result;
}

uint32_t MessageDialog(const UnicodeString Msg, TQueryType Type,
  uint32_t Answers, UnicodeString HelpKeyword, const TMessageParams * Params)
{
  return MoreMessageDialog(Msg, nullptr, Type, Answers, HelpKeyword, Params);
}

uint32_t SimpleErrorDialog(const UnicodeString Msg, const UnicodeString MoreMessages)
{
  uint32_t Result;
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
  std::unique_ptr<TStrings> StackTrace(std::make_unique<TStringList>());
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
#endif // #if 0

static std::unique_ptr<TCriticalSection> StackTraceCriticalSection(TraceInitPtr(new TCriticalSection()));
typedef rde::map<DWORD, TStrings *> TStackTraceMap;
static TStackTraceMap StackTraceMap;

UnicodeString GetExceptionDebugInfo()
{
  UnicodeString Result;
#if 0
  TGuard Guard(StackTraceCriticalSection.get()); nb::used(Guard);
  TStackTraceMap::iterator Iterator = StackTraceMap.find(GetCurrentThreadId());
  if (Iterator != StackTraceMap.end())
  {
    TStrings * StackTrace = Iterator->second;
    for (int Index = 0; Index < StackTrace->Count; Index++)
    {
      UnicodeString Frame = StackTrace->Strings[Index];
      // The last line might be empty
      if (!Frame.IsEmpty())
      {
        int P = Frame.Pos(L")");
        if (DebugAlwaysTrue(P > 0))
        {
          UnicodeString Symbol = Frame.SubString(P + 1, Frame.Length() - P).Trim();

          if ((Symbol != L"KERNELBASE.dll.RaiseException") &&
              (Symbol != L"Jclhookexcept::JclAddExceptNotifier") &&
              (Symbol != L"_ReThrowException") &&
              (Symbol != L"____ExceptionHandler") &&
              (Symbol != L"__ExceptionHandler") &&
              (Symbol != L"___doGlobalUnwind") &&
              (Symbol != L"_ThrowExceptionLDTC"))
          {
            AddToList(Result, Symbol, L";");
          }
        }
      }
    }
  }
#endif // #if 0
  return Result;
}

bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages)
{
  bool Result = false;
#if 0

  TGuard Guard(StackTraceCriticalSection.get()); nb::used(Guard);

  TStackTraceMap::iterator Iterator = StackTraceMap.find(GetCurrentThreadId());
  if (Iterator != StackTraceMap.end())
  {
    std::unique_ptr<TStrings> OwnedMoreMessages;
    if (MoreMessages == nullptr)
    {
      OwnedMoreMessages = std::make_unique<TStringList>();
      MoreMessages = OwnedMoreMessages.get();
      Result = true;
    }
    if (!MoreMessages->Text.IsEmpty())
    {
      MoreMessages->Text = MoreMessages->Text + "\n";
    }
    MoreMessages->Text = MoreMessages->Text + LoadStr(STACK_TRACE) + "\n";
    MoreMessages->AddStrings(Iterator->second);

    delete Iterator->second;
    StackTraceMap.erase(Iterator);

    OwnedMoreMessages.release();
  }
#endif // #if 0
  return Result;
}

uint32_t ExceptionMessageDialog(Exception * /*E*/, TQueryType /*Type*/,
  const UnicodeString & /*MessageFormat*/, uint32_t /*Answers*/, const UnicodeString & /*HelpKeyword*/,
  const TMessageParams * /*Params*/)
{
#if 0
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
  if (!MessageFormat.IsEmpty())
  {
    Message = FORMAT(MessageFormat, UnformatMessage(Message));
  }

  HelpKeyword = MergeHelpKeyword(HelpKeyword, GetExceptionHelpKeyword(E));

  std::unique_ptr<TStrings> OwnedMoreMessages;
  if (AppendExceptionStackTraceAndForget(MoreMessages))
  {
    OwnedMoreMessages.reset(MoreMessages);
  }

  return MoreMessageDialog(
    Message, MoreMessages, Type, Answers, HelpKeyword, Params);
#endif // #if 0
  return 0;
}

uint32_t FatalExceptionMessageDialog(
  Exception * E, TQueryType Type, const UnicodeString & MessageFormat, uint32_t Answers,
  const UnicodeString & HelpKeyword, const TMessageParams * Params)
{
#if 0
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
  AParams.Aliases = Aliases;
  AParams.AliasesCount = LENOF(Aliases);

  return ExceptionMessageDialog(E, Type, MessageFormat, Answers, HelpKeyword, &AParams);
#endif // #if 0
  ThrowNotImplemented(3017);
  return 0;
}


static void DoExceptNotify(TObject * ExceptObj, void * ExceptAddr,
  bool OSException, void * BaseOfStack)
{
#if 0
  if (ExceptObj != nullptr)
  {
    Exception * E = dynamic_cast<Exception *>(ExceptObj);
    if ((E != nullptr) && IsInternalException(E))
    {
      DoExceptionStackTrace(ExceptObj, ExceptAddr, OSException, BaseOfStack);

      TJclStackInfoList * StackInfoList = JclLastExceptStackList();

      if (DebugAlwaysTrue(StackInfoList != nullptr))
      {
        std::unique_ptr<TStrings> StackTrace(StackInfoListToStrings(StackInfoList));

        DWORD ThreadID = GetCurrentThreadId();

        TGuard Guard(StackTraceCriticalSection.get()); nb::used(Guard);

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
#endif // #if 0
  ThrowNotImplemented(3016);
}

void * BusyStart()
{
  void * Token = nullptr; // reinterpret_cast<void *>(Screen->Cursor);
  __removed Screen->Cursor = crHourGlass;
  return Token;
}

void BusyEnd(void * Token)
{
  __removed Screen->Cursor = reinterpret_cast<TCursor>(Token);
}


static DWORD MainThread = 0;
static TDateTime LastGUIUpdate{};
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
#if 0
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
#endif // #if 0
  return Result;
}

void SystemRequired()
{

}

#if 0

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
const int cpiGenerateCode = -5;
const int cpiSavePreset = -6;

void CopyParamListPopup(TRect Rect, TPopupMenu * Menu,
  const TCopyParamType & Param, UnicodeString Preset, TNotifyEvent OnClick,
  int Options, int CopyParamAttrs, bool SaveSettings)
{
  Menu->Items->Clear();

  TMenuItem * Item;

  Item = new TMenuItem(Menu);
  Item->Caption = LoadStr(COPY_PARAM_CUSTOM);
  Item->Tag = cpiCustom;
  Item->Default = FLAGSET(Options, cplCustomizeDefault);
  Item->OnClick = OnClick;
  Menu->Items->Add(Item);
  TMenuItem * CustomizeItem = Item;

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
  Item->Caption = LoadStr(COPY_PARAM_SAVE_PRESET);
  Item->Tag = cpiSavePreset;
  Item->OnClick = OnClick;
  Menu->Items->Add(Item);

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

  CustomizeItem->Checked = !AnyChecked;

  Item = new TMenuItem(Menu);
  Item->Caption = L"-";
  Menu->Items->Add(Item);

  Item = new TMenuItem(Menu);
  Item->Caption = LoadStr(COPY_PARAM_CONFIGURE);
  Item->Tag = cpiConfigure;
  Item->OnClick = OnClick;
  Menu->Items->Add(Item);

  if (FLAGSET(Options, cplGenerateCode))
  {
    Item = new TMenuItem(Menu);
    Item->Caption = L"-";
    Menu->Items->Add(Item);

    Item = new TMenuItem(Menu);
    Item->Caption = LoadStr(COPY_PARAM_GENERATE_CODE);
    Item->Tag = cpiGenerateCode;
    Item->OnClick = OnClick;
    Menu->Items->Add(Item);
  }


  MenuPopup(Menu, Rect, nullptr);
}

int CopyParamListPopupClick(TObject * Sender,
  TCopyParamType & Param, UnicodeString & Preset, int CopyParamAttrs,
  bool * SaveSettings)
{
  TComponent * Item = dynamic_cast<TComponent *>(Sender);
  DebugAssert(Item != nullptr);
  DebugAssert((Item->Tag >= cpiSavePreset) && (Item->Tag < GUIConfiguration->CopyParamList->Count));

  int Result;
  if (Item->Tag == cpiConfigure)
  {
    bool MatchedPreset = (GUIConfiguration->CopyParamPreset[Preset] == Param);
    DoPreferencesDialog(pmPresets);
    Result = (MatchedPreset && GUIConfiguration->HasCopyParamPreset[Preset]) ? 1 : 0;
    if (Result > 0)
    {
      // For cast, see a comment below
      Param = TCopyParamType(GUIConfiguration->CopyParamPreset[Preset]);
    }
  }
  else if (Item->Tag == cpiCustom)
  {
    Result = DoCopyParamCustomDialog(Param, CopyParamAttrs) ? 1 : 0;
  }
  else if (Item->Tag == cpiSaveSettings)
  {
    if (DebugAlwaysTrue(SaveSettings != nullptr))
    {
      *SaveSettings = !*SaveSettings;
    }
    Result = 0;
  }
  else if (Item->Tag == cpiSavePreset)
  {
    std::unique_ptr<TCopyParamList> CopyParamList(new TCopyParamList());
    *CopyParamList = *GUIConfiguration->CopyParamList;
    int Index = -1;
    if (DoCopyParamPresetDialog(CopyParamList.get(), Index, cpmAdd, nullptr, Param))
    {
      GUIConfiguration->CopyParamList = CopyParamList.get();
      // If saved unmodified, then make this the selected preset
      if (*CopyParamList->CopyParams[Index] == Param)
      {
        Preset = CopyParamList->Names[Index];
      }
    }
    Result = 0;
  }
  else if (Item->Tag == cpiGenerateCode)
  {
    Result = -cplGenerateCode;
  }
  else if (Item->Tag == cpiSavePreset)
  {
    std::unique_ptr<TCopyParamList> CopyParamList(new TCopyParamList());
    *CopyParamList = *GUIConfiguration->CopyParamList;
    int Index = -1;
    if (DoCopyParamPresetDialog(CopyParamList.get(), Index, cpmAdd, nullptr, Param))
    {
      GUIConfiguration->CopyParamList = CopyParamList.get();
      // If saved unmodified, then make this the selected preset
      if (*CopyParamList->CopyParams[Index] == Param)
      {
        Preset = CopyParamList->Names[Index];
      }
    }
    Result = 0;
  }
  else
  {
    Preset = (Item->Tag >= 0) ?
      GUIConfiguration->CopyParamList->Names[Item->Tag] : UnicodeString();
    // The cast strips away the "queue" properties of the TGUICopyParamType
    // that are not configurable in presets
    Param = TCopyParamType(GUIConfiguration->CopyParamPreset[Preset]);
    Result = 1;
  }
  return Result;
}


class TCustomCommandPromptsDialog : public TCustomDialog
{
public:
  TCustomCommandPromptsDialog(
    const UnicodeString & CustomCommandName, const UnicodeString & HelpKeyword,
    const TUnicodeStringVector & Prompts, const TUnicodeStringVector & Defaults);

  bool Execute(TUnicodeStringVector & Values);

private:
  UnicodeString HistoryKey(int Index);

  std::vector<THistoryComboBox *> FEdits;
  TUnicodeStringVector FPrompts;
  UnicodeString FCustomCommandName;
};

TCustomCommandPromptsDialog::TCustomCommandPromptsDialog(
    const UnicodeString & CustomCommandName, const UnicodeString & HelpKeyword,
    const TUnicodeStringVector & Prompts, const TUnicodeStringVector & Defaults) :
  TCustomDialog(HelpKeyword)
{

  FCustomCommandName = CustomCommandName;
  Caption = FMTLOAD(CUSTOM_COMMANDS_PARAMS_TITLE, FCustomCommandName);

  FPrompts = Prompts;
  DebugAssert(FPrompts.size() == Defaults.size());
  for (size_t Index = 0; Index < FPrompts.size(); Index++)
  {
    UnicodeString Prompt = FPrompts[Index];
    if (Prompt.IsEmpty())
    {
      Prompt = LoadStr(CUSTOM_COMMANDS_PARAM_PROMPT2);
    }
    THistoryComboBox * ComboBox = new THistoryComboBox(this);
    ComboBox->AutoComplete = false;
    AddComboBox(ComboBox, CreateLabel(Prompt));
    ComboBox->Items = CustomWinConfiguration->History[HistoryKey(Index)];
    ComboBox->Text = Defaults[Index];
    FEdits.push_back(ComboBox);
  }
}

UnicodeString TCustomCommandPromptsDialog::HistoryKey(int Index)
{
  UnicodeString Result = FPrompts[Index];
  if (Result.IsEmpty())
  {
    Result = IntToStr(Index);
  }
  Result = FORMAT(L"%s_%s", FCustomCommandName, Result);
  Result = CustomWinConfiguration->GetValidHistoryKey(Result);
  return L"CustomCommandParam_" + Result;
}

bool TCustomCommandPromptsDialog::Execute(TUnicodeStringVector & Values)
{

  bool Result = TCustomDialog::Execute();

  if (Result)
  {
    for (size_t Index = 0; Index < FEdits.size(); Index++)
    {
      Values.push_back(FEdits[Index]->Text);
      FEdits[Index]->SaveToHistory();
      CustomWinConfiguration->History[HistoryKey(Index)] = FEdits[Index]->Items;
    }
  }

  return Result;
}
#endif // #if 0


TWinInteractiveCustomCommand::TWinInteractiveCustomCommand(
  TCustomCommand * ChildCustomCommand, const UnicodeString CustomCommandName, const UnicodeString HelpKeyword) noexcept :
  TInteractiveCustomCommand(ChildCustomCommand)
{
  FCustomCommandName = StripEllipsis(StripHotkey(CustomCommandName));
  FHelpKeyword = HelpKeyword;
}

void TWinInteractiveCustomCommand::PatternHint(int32_t Index, const UnicodeString & Pattern)
{
  if (IsPromptPattern(Pattern))
  {
    UnicodeString Prompt;
    UnicodeString Default;
    bool Delimit = false;
    ParsePromptPattern(Pattern, Prompt, Default, Delimit);
#if 0
    FIndexes.insert(std::make_pair(Index, FPrompts.size()));
    FPrompts.push_back(Prompt);
    FDefaults.push_back(Default);
#endif // #if 0
  }
}

void TWinInteractiveCustomCommand::Prompt(
  int32_t /*Index*/, const UnicodeString & /*Prompt*/, UnicodeString &/*Value*/) const
{
#if 0
  if (DebugAlwaysTrue(FIndexes.find(Index) != FIndexes.end()))
  {
    size_t PromptIndex = FIndexes[Index];
    if (FValues.empty())
    {
      UnicodeString HelpKeyword = FHelpKeyword;
      if (HelpKeyword.IsEmpty())
      {
        HelpKeyword = HELP_CUSTOM_COMMAND_PARAM;
      }
      std::unique_ptr<TCustomCommandPromptsDialog> Dialog(
        new TCustomCommandPromptsDialog(FCustomCommandName, HelpKeyword, FPrompts, FDefaults));
      if (!Dialog->Execute(FValues))
      {
        Abort();
      }
    }

    if (DebugAlwaysTrue(FValues.size() == FPrompts.size()) &&
        DebugAlwaysTrue(PromptIndex < FValues.size()))
    {
      Value = FValues[PromptIndex];
    }
  }
#endif // #if 0
}

void TWinInteractiveCustomCommand::Execute(
  const UnicodeString & Command, UnicodeString & Value) const
{
  DWORD DummyExitCode;
  ThrowNotImplemented(3019);
//  ExecuteProcessAndReadOutput(Command, Value, DummyExitCode, false);
  // trim trailing cr/lf
  Value = TrimRight(Value);
}
#if 0

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

static void SetMenuButtonImages(TButton * Button)
{
  Button->Images = GetButtonImages(Button);
}

static void MenuButtonRescale(TComponent * Sender, TObject * /*Token*/)
{
  TButton * Button = DebugNotNull(dynamic_cast<TButton *>(Sender));
  SetMenuButtonImages(Button);
}

void MenuButton(TButton * Button)
{
  SetMenuButtonImages(Button);
  Button->ImageIndex = 0;
  Button->DisabledImageIndex = 1;
  Button->ImageAlignment = iaRight;
  SetRescaleFunction(Button, MenuButtonRescale);
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

    std::unique_ptr<TCanvas> Canvas(CreateControlCanvas(Button));

    UnicodeString Caption;
    // Centering unlinks the caption from the action
    TAction * Action = dynamic_cast<TAction *>(Button->Action);
    if (Action != NULL)
    {
      Caption = Action->Caption;
    }
    else
    {
      Caption = Button->Caption.Trim();
    }
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
    // Without the change, the rounding caused the space between icon and caption too
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

bool MinimizedToTray = false;

static void DoApplicationMinimizeRestore(bool Minimize)
{
  TForm * MainForm = Application->MainForm;
  TForm * MainLikeForm = GetMainForm();
  bool MinimizeToTray;
  if (Minimize)
  {
    MinimizeToTray = WinConfiguration->MinimizeToTray;
  }
  else
  {
    // Use tray restore code, even if minimization to tray is not on anymore when restoring.
    // This is particularly used for ad-hoc minimize to tray from "keep up to date" dialog.
    MinimizeToTray = MinimizedToTray;
  }
  MinimizedToTray = false; // reset in any case, even if we somehow got restored from tray without invoking this code
  if ((MainLikeForm != MainForm) && !MinimizeToTray)
  {
    static TWindowState PreviousWindowState = wsNormal;
    if (Minimize)
    {
      PreviousWindowState = MainLikeForm->WindowState;
      // This works correctly with child windows thanks to Application->OnGetMainFormHandle
      MainLikeForm->WindowState = wsMinimized;
    }
    else
    {
      MainLikeForm->WindowState = PreviousWindowState;
    }
  }
  else
  {
    // What is described below should not ever happen, except when minimizing to tray,
    // as we capture command-line operation above.
    // Had we called TApplication::Minimize, it would hide all non-MainForm windows, including MainLineForm,
    // so it actually also hides taskbar button, what we do not want.
    // WORKAROUND
    // When main window is hidden (command-line operation),
    // we do not want it to be shown by TApplication.Restore,
    // so we temporarily detach it from an application.
    // Probably not really necessary for minimizing phase,
    // but we do it for consistency anyway.
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
        MinimizedToTray = WinConfiguration->MinimizeToTray;
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
  uint32_t Cmd = (SysCommand.CmdType & 0xFFF0);
  bool Result = (Cmd == SC_MINIMIZE);
  if (Result)
  {
    ApplicationMinimize();
    SysCommand.Result = 1;
  }
  return Result;
}

void ClickToolbarItem(TTBCustomItem * Item, bool PositionCursor)
{
  TTBCustomItem * TopItem = Item;
  while (TopItem->Parent != nullptr)
  {
    TopItem = TopItem->Parent;
  }
  TTBCustomToolbar * Toolbar = dynamic_cast<TTBCustomToolbar *>(TopItem->ParentComponent);
  DebugAssert(Toolbar != nullptr);
  TTBItemViewer * Viewer = Toolbar->View->Find(Item);
  DebugAssert(Viewer != nullptr);

  int X = Viewer->BoundsRect.Left + (Viewer->BoundsRect.Width() / 2);
  int Y = Viewer->BoundsRect.Top + (Viewer->BoundsRect.Height() / 2);

  if (PositionCursor)
  {
    Mouse->CursorPos = Toolbar->ClientToScreen(TPoint(X, Y));
  }

  PostMessage(Toolbar->Handle, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(X, Y));
}


UnicodeString DumpCallstackEventName(int ProcessId)
{
  return FORMAT(DUMPCALLSTACK_EVENT, ProcessId);
}

UnicodeString DumpCallstackFileName(int ProcessId)
{
  UnicodeString FileName = FORMAT(L"%s.txt", DumpCallstackEventName(ProcessId));
  UnicodeString Result = TPath::Combine(SystemTemporaryDirectory(), FileName);
  return Result;
}

void CheckConfigurationForceSave()
{
  if (UseAlternativeFunction() && Configuration->Persistent &&
      (Configuration->Storage == stIniFile) && Sysutils::FileExists(ApiPath(Configuration->IniFileStorageName)) &&
      !Configuration->ForceSave)
  {
    int Attr = GetFileAttributes(ApiPath(Configuration->IniFileStorageName).c_str());
    if (FLAGSET(Attr, FILE_ATTRIBUTE_READONLY))
    {
      UnicodeString Message = FMTLOAD(READONLY_INI_FILE_OVERWRITE, Configuration->IniFileStorageName);
      if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel, HELP_READONLY_INI_FILE) == qaOK)
      {
        Configuration->ForceSave = true;
      }
    }
  }
}

class TCallstackThread : public TSignalThread
{
public:
  TCallstackThread();

protected:
  virtual void ProcessEvent();

private:
  static HANDLE DoCreateEvent();
};

TCallstackThread::TCallstackThread() :
  TSignalThread(true, DoCreateEvent())
{
}

void TCallstackThread::ProcessEvent()
{
  try
  {
    UnicodeString Path = DumpCallstackFileName(GetCurrentProcessId());
    std::unique_ptr<TStrings> StackStrings;
    HANDLE MainThreadHandle = reinterpret_cast<HANDLE>(MainThreadID);
    if (SuspendThread(MainThreadHandle) < 0)
    {
      RaiseLastOSError();
    }
    try
    {
      TJclStackInfoList * StackInfoList = JclCreateThreadStackTraceFromID(true, MainThreadID);
      if (StackInfoList == nullptr)
      {
        RaiseLastOSError();
      }
      StackStrings.reset(StackInfoListToStrings(StackInfoList));
    }
    __finally
    {
      if (ResumeThread(MainThreadHandle) < 0)
      {
        RaiseLastOSError();
      }
    }
    TFile::WriteAllText(Path, StackStrings->Text);
  }
  catch (...)
  {
  }
}

HANDLE TCallstackThread::DoCreateEvent()
{
  UnicodeString Name = DumpCallstackEventName(GetCurrentProcessId());
  return CreateEvent(nullptr, false, false, Name.c_str());
}


std::unique_ptr<TCallstackThread> CallstackThread;

static void AppGetMainFormHandle(void * /*Data*/, HWND & Handle)
{
  TForm * MainForm = GetMainForm();
  // This, among other, causes minimizing of the top-level non-MainForm minimize other child windows.
  // Like clicking "Minimize" on Progress window over Synchronization progress window over Synchronization checklist window.
  // Would also have a lot of other effects (hopefully possitive) and may render lot of existing MainFormLike code obsolete.
  if ((MainForm != nullptr) && IsMainFormLike(MainForm) && MainForm->HandleAllocated())
  {
    Handle = MainForm->Handle;
  }
}
#endif // #if 0

void WinInitialize()
{
#if 0
  if (JclHookExceptions())
  {
    JclStackTrackingOptions << stAllModules;
    JclAddExceptNotifier(DoExceptNotify, npFirstChain);
    CallstackThread = std::make_unique<TCallstackThread>();
    CallstackThread->Start();
  }
#endif // #if 0

  __removed SetErrorMode(SEM_FAILCRITICALERRORS);
  __removed OnApiPath = ApiPath;
  MainThread = GetCurrentThreadId();
  __removed Application->OnGetMainFormHandle = MakeMethod<TGetHandleEvent>(nullptr, AppGetMainFormHandle);

}

void WinFinalize()
{
#if 0
  CallstackThread.reset(nullptr);
  JclRemoveExceptNotifier(DoExceptNotify);
#endif // #if 0
}
#if 0

::TTrayIcon::TTrayIcon(uint32_t Id)
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
  const UnicodeString & Str, TQueryType QueryType, uint32_t Timeout,
  TNotifyEvent OnBalloonClick, TObject * BalloonUserData)
{
  if (Timeout > 30000)
  {
    // this is probably system limit, do not try more, especially for
    // the timeout-driven hiding of the tray icon (for Win2k)
    Timeout = 30000;
  }
  FTrayIcon->uFlags |= NIF_INFO;
  AppLogFmt("Tray popup balloon: %s - %s", (Title, Str));
  Title = Title + TitleSeparator + AppNameString();
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

bool ::TTrayIcon::Notify(uint32_t Message)
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
    uint32_t Max = LENOF(FTrayIcon->szTip);
    StrPLCopy(FTrayIcon->szTip, value, Max - 1);
    Update();
  }
}
#endif // #if 0

bool InputDialog(const UnicodeString ACaption,
  const UnicodeString APrompt, UnicodeString &Value, UnicodeString HelpKeyword,
  TStrings *History, bool PathInput,
  TInputDialogInitializeEvent OnInitialize, bool Echo, int32_t Width)
{
  bool Result = GetGlobals()->InputDialog(ACaption, APrompt, Value, HelpKeyword,
    History, PathInput, OnInitialize, Echo);
  return Result;
}

uint32_t MessageDialog(const UnicodeString & Msg, TQueryType Type,
  uint32_t Answers, const UnicodeString & HelpKeyword, const TMessageParams * Params)
{
  DebugUsedParam(HelpKeyword);
  uint32_t Result = GetGlobals()->MoreMessageDialog(Msg, nullptr, Type, Answers, Params);
  return Result;
}

uint32_t MessageDialog(int32_t Ident, TQueryType Type,
  uint32_t Answers, const UnicodeString & HelpKeyword, const TMessageParams * Params)
{
  DebugUsedParam(HelpKeyword);
  UnicodeString Msg = LoadStr(Ident);
  uint32_t Result = GetGlobals()->MoreMessageDialog(Msg, nullptr, Type, Answers, Params);
  return Result;
}

uint32_t SimpleErrorDialog(const UnicodeString & Msg, const UnicodeString & /*MoreMessages*/)
{
  uint32_t Answers = qaOK;
  uint32_t Result = GetGlobals()->MoreMessageDialog(Msg, nullptr, qtError, Answers, nullptr);
  return Result;
}

uint32_t MoreMessageDialog(const UnicodeString & Message,
  TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
  const UnicodeString & HelpKeyword, const TMessageParams * Params)
{
  DebugUsedParam(HelpKeyword);
  uint32_t Result = GetGlobals()->MoreMessageDialog(Message, MoreMessages, Type, Answers, Params);
  return Result;
}

