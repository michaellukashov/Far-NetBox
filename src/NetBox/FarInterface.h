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
  UnicodeString ImageName, const UnicodeString NeverAskAgainCaption,
  const UnicodeString MoreMessagesUrl, int MoreMessagesSize,
  const UnicodeString CustomCaption);
TFarDialog *CreateMoreMessageDialogEx(const UnicodeString Message, TStrings *MoreMessages,
  TQueryType Type, uint32_t Answers, const UnicodeString HelpKeyword, const TMessageParams *Params);
uintptr_t ExecuteMessageDialog(TFarDialog *Dialog, uint32_t Answers, const TMessageParams *Params);
#if 0
void InsertPanelToMessageDialog(TFarDialog * Form, TPanel * Panel);
#endif //#if 0

//from windows/GUITools.h
void ValidateMaskEdit(TFarComboBox *Edit);
void ValidateMaskEdit(TFarEdit *Edit);
