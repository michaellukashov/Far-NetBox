//---------------------------------------------------------------------------
#pragma once

#ifndef VCLCommonH
#define VCLCommonH
//---------------------------------------------------------------------------
#include "Common.h"
#include "Configuration.h"
#include "Exceptions.h"
#include <ComCtrls.hpp>
#if 0
//---------------------------------------------------------------------------
const TColor LinkColor = clBlue;
//---------------------------------------------------------------------------
void FixListColumnWidth(TListView * TListView, int Index);
void AutoSizeListColumnsWidth(TListView * ListView, int ColumnToShrinkIndex = -1);
void EnableControl(TControl* Control, bool Enable);
void ReadOnlyControl(TControl * Control, bool ReadOnly = true);
void ReadOnlyAndEnabledControl(TControl * Control, bool ReadOnly, bool Enabled);
void InitializeSystemSettings();
void FinalizeSystemSettings();
void LocalSystemSettings(TCustomForm * Control);
void UseSystemSettingsPre(TCustomForm * Control);
void UseSystemSettingsPost(TCustomForm * Control);
void UseSystemSettings(TCustomForm * Control);
void ResetSystemSettings(TCustomForm * Control);
void LinkLabel(TStaticText * StaticText, const UnicodeString Url = L"",
  TNotifyEvent OnEnter = nullptr);
void LinkActionLabel(TStaticText * StaticText);
void LinkAppLabel(TStaticText * StaticText);
void HintLabel(TStaticText * StaticText, const UnicodeString Hint = L"");
void HotTrackLabel(TLabel * Label);
void SetLabelHintPopup(TLabel * Label, const UnicodeString Hint);
bool HasLabelHintPopup(TLabel * Label, const UnicodeString HintStr);
void FixComboBoxResizeBug(TCustomComboBox * ComboBox);
void ShowAsModal(TForm * Form, void *& Storage, bool BringToFront = true);
void HideAsModal(TForm * Form, void *& Storage);
bool ReleaseAsModal(TForm * Form, void *& Storage);
bool SelectDirectory(UnicodeString &Path, const UnicodeString Prompt,
  bool PreserveFileName);
enum TListViewCheckAll { caCheck, caUncheck, caToggle };
bool ListViewAnyChecked(TListView * ListView, bool Checked = true);
void ListViewCheckAll(TListView * ListView,
  TListViewCheckAll CheckAll);
void ComboAutoSwitchInitialize(TComboBox * ComboBox);
void ComboAutoSwitchLoad(TComboBox * ComboBox, TAutoSwitch Value);
TAutoSwitch ComboAutoSwitchSave(TComboBox * ComboBox);
void CheckBoxAutoSwitchLoad(TCheckBox * CheckBox, TAutoSwitch Value);
TAutoSwitch CheckBoxAutoSwitchSave(TCheckBox * CheckBox);
void InstallPathWordBreakProc(TWinControl * Control);
void SetVerticalControlsOrder(TControl ** ControlsOrder, int Count);
void SetHorizontalControlsOrder(TControl ** ControlsOrder, int Count);
void MakeNextInTabOrder(TWinControl * Control, TWinControl * After);
void CutFormToDesktop(TForm * Form);
void UpdateFormPosition(TCustomForm * Form, TPosition Position);
void ResizeForm(TCustomForm * Form, int Width, int Height);
TComponent * GetFormOwner();
TForm * GetMainForm();
void SetCorrectFormParent(TForm * Form);
void InvokeHelp(TWinControl * Control);
void FixFormIcons(TForm * Form);
Forms::TMonitor *  FormMonitor(TCustomForm * Form);
int GetLastMonitor();
void SetLastMonitor(int MonitorNum);
TForm * _SafeFormCreate(TMetaClass * FormClass, TComponent * Owner);
template<class FormType>
FormType * SafeFormCreate(TComponent * Owner = nullptr)
{
  return dynamic_cast<FormType *>(_SafeFormCreate(__classid(FormType), Owner));
}
bool SupportsSplitButton();
TModalResult DefaultResult(TCustomForm * Form, TButton * DefaultButton = nullptr);
void DefaultButton(TButton * Button, bool Default);
void MemoKeyDown(TObject * Sender, WORD & Key, TShiftState Shift);
void UseDesktopFont(TControl * Control);
void UpdateDesktopFont();
UnicodeString FormatFormCaption(TCustomForm * Form, const UnicodeString Caption);
UnicodeString FormatMainFormCaption(const UnicodeString Caption);
TShiftState AllKeyShiftStates();
void RealignControl(TControl * Control);
void HookFormActivation(TCustomForm * Form);
void UnhookFormActivation(TCustomForm * Form);
void ShowFormNoActivate(TForm * Form);
TPanel * CreateBlankPanel(TComponent * Owner);
typedef void (*TRescaleEvent)(TComponent * Sender, TObject * Token);
void SetRescaleFunction(
  TComponent * Component, TRescaleEvent OnRescale, TObject * Token = nullptr, bool OwnsToken = false);
void RecordFormImplicitRescale(TForm * Form);
//---------------------------------------------------------------------------

#endif // #if 0

#endif  // VCLCommonH
