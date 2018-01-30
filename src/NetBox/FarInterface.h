//from windows/WinInterface.h
#pragma once
#include <Classes.hpp>
#include <Interface.h>
#include <GUIConfiguration.h>
#include <FarDialog.h>

enum TMsgDlgType
{
  mtConfirmation,
  mtInformation,
  mtError,
  mtWarning,
};

// forms\MessageDlg.cpp
void AnswerNameAndCaption(
  uint32_t Answer, UnicodeString &Name, UnicodeString &Caption);
TFarDialog *CreateMoreMessageDialog(const UnicodeString Msg,
  TStrings *MoreMessages, TMsgDlgType DlgType, uint32_t Answers,
  const TQueryButtonAlias *Aliases, uintptr_t AliasesCount,
  uintptr_t TimeoutAnswer, TFarButton **TimeoutButton,
  UnicodeString ImageName, UnicodeString NeverAskAgainCaption,
  UnicodeString MoreMessagesUrl, int MoreMessagesSize,
  UnicodeString CustomCaption);
TFarDialog *CreateMoreMessageDialogEx(const UnicodeString Message, TStrings *MoreMessages,
  TQueryType Type, uint32_t Answers, const UnicodeString HelpKeyword, const TMessageParams *Params);
uintptr_t ExecuteMessageDialog(TFarDialog *Dialog, uint32_t Answers, const TMessageParams *Params);
/*void InsertPanelToMessageDialog(TFarDialog * Form, TPanel * Panel);
void NavigateMessageDialogToUrl(TFarDialog * Form, UnicodeString Url);*/

//from windows/GUITools.h
void ValidateMaskEdit(TFarComboBox *Edit);
void ValidateMaskEdit(TFarEdit *Edit);
