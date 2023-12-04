#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <TextsWin.h>

#include "Bookmarks.h"
#include "FarConfiguration.h"
#include "FarPlugin.h"
#include "CoreMain.h"
#include "FarInterface.h"

constexpr const wchar_t * MessagePanelName = L"Panel";
constexpr const wchar_t * MainMessageLabelName = L"MainMessage";
constexpr const wchar_t * MessageLabelName = L"Message";
constexpr const wchar_t * YesButtonName = L"Yes";
constexpr const wchar_t * OKButtonName = L"OK";

void AnswerNameAndCaption(
  uint32_t Answer, UnicodeString & Name, UnicodeString & Caption)
{
  switch (Answer)
  {
    case qaYes:
      Caption = LoadStr(Vcl_Consts_SMsgDlgYes);
      Name = YesButtonName;
      break;

    case qaNo:
      Caption = LoadStr(Vcl_Consts_SMsgDlgNo);
      Name = L"No";
      break;

    case qaOK:
      Caption = LoadStr(Vcl_Consts_SMsgDlgOK);
      Name = OKButtonName;
      break;

    case qaCancel:
      Caption = LoadStr(Vcl_Consts_SMsgDlgCancel);
      Name = L"Cancel";
      break;

    case qaAbort:
      Caption = LoadStr(Vcl_Consts_SMsgDlgAbort);
      Name = L"Abort";
      break;

    case qaRetry:
      Caption = LoadStr(Vcl_Consts_SMsgDlgRetry);
      Name = L"Retry";
      break;

    case qaIgnore:
      Caption = LoadStr(Vcl_Consts_SMsgDlgIgnore);
      Name = L"Ignore";
      break;

    // Own variant to avoid accelerator conflict with "Abort" button.
    // Note that as of now, ALL_BUTTON is never actually used,
    // because qaAll is always aliased
    case qaAll:
      Caption = LoadStr(ALL_BUTTON);
      Name = L"All";
      break;

    case qaNoToAll:
      Caption = LoadStr(Vcl_Consts_SMsgDlgNoToAll);
      Name = L"NoToAll";
      break;

    // Own variant to avoid accelerator conflict with "Abort" button.
    case qaYesToAll:
      Caption = LoadStr(YES_TO_ALL_BUTTON);
      Name = L"YesToAll";
      break;

    case qaHelp:
      Caption = LoadStr(Vcl_Consts_SMsgDlgHelp);
      Name = L"Help";
      break;

    case qaSkip:
      Caption = LoadStr(SKIP_BUTTON);
      Name = L"Skip";
      break;

    case qaReport:
      Caption = LoadStr(REPORT_BUTTON);
      Name = L"Report";
      break;

    default:
      DebugFail();
      throw Exception(L"Undefined answer");
  }
}

