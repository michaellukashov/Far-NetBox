
#include <vcl.h>
#pragma hdrstop

#include "WinInterface.h"
#include "VCLCommon.h"

#include <Common.h>
#include <TextsWin.h>
#include <RemoteFiles.h>
#include <GUITools.h>
#include <Tools.h>
#include <CustomWinConfiguration.h>
#include <CoreMain.h>

#include <Vcl.StdActns.hpp>
#include <PasswordEdit.hpp>
#include <FileCtrl.hpp>
#include <PathLabel.hpp>
#include <PasTools.hpp>
#include <StrUtils.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include <Math.hpp>
#include <TB2ExtItems.hpp>
#include <TBXExtItems.hpp>
#include <IEListView.hpp>
#include <WinApi.h>
#include <vssym32.h>

const UnicodeString LinkAppLabelMark(TraceInitStr(L" \x00BB"));

#pragma package(smart_init)

void FixListColumnWidth(TListView * TListView, int Index)
{
  if (Index < 0)
  {
    Index = TListView->Columns->Count + Index;
  }
  TListView->Column[Index]->Tag = 1;
}

static int GetColumnTextWidth(TListView * ListView, int ColumnPadding, const UnicodeString & Text)
{
  return
    ColumnPadding +
    ListView->Canvas->TextExtent(Text).Width;
}

void AutoSizeListColumnsWidth(TListView * ListView, int ColumnToShrinkIndex)
{
  // Preallocate handle to precreate columns, otherwise our changes may get
  // overwritten once the handle is created.
  // This should actually get called only once the handle are allocated.
  // Otherwise we end up recreating the handles,
  // what may cause a flicker of the currently focused window title.
  ListView->HandleNeeded();

  SendMessage(ListView->Handle, WM_SETREDRAW, false, 0);

  try
  {
    int ColumnPadding = 2 * ScaleByTextHeightRunTime(ListView, 6);
    int ColumnShrinkMinWidth = ScaleByTextHeightRunTime(ListView, 60);

    int ResizableWidth = 0;
    int NonResizableWidth = 0;
    int LastResizable = -1;
    for (int Index = 0; Index < ListView->Columns->Count; Index++)
    {
      TListColumn * Column = ListView->Columns->Items[Index];

      int CaptionWidth = GetColumnTextWidth(ListView, ColumnPadding, Column->Caption);

      int OrigWidth = -1;
      bool NonResizable = (Column->Tag != 0);
      if (NonResizable) // optimization
      {
        OrigWidth = ListView_GetColumnWidth(ListView->Handle, Index);
      }

      int Width;
      // LVSCW_AUTOSIZE does not work for OwnerData list views
      if (!ListView->OwnerData)
      {
        ListView_SetColumnWidth(ListView->Handle, Index, LVSCW_AUTOSIZE);
        Width = ListView_GetColumnWidth(ListView->Handle, Index);
      }
      else
      {
        Width = 0;
        for (int ItemIndex = 0; ItemIndex < ListView->Items->Count; ItemIndex++)
        {
          TListItem * Item = ListView->Items->Item[ItemIndex];

          UnicodeString Text;
          if (Index == 0)
          {
            Text = Item->Caption;
          }
          // Particularly EditorListView3 on Preferences dialog does not have all subitems filled for internal editor
          else if (Index <= Item->SubItems->Count)
          {
            Text = Item->SubItems->Strings[Index - 1];
          }

          int TextWidth = GetColumnTextWidth(ListView, ColumnPadding, Text);
          if (TextWidth > Width)
          {
            Width = TextWidth;
          }
        }
      }
      Width = Max(Width, CaptionWidth);
      // Never shrink the non-resizable columns
      if (NonResizable && (Width < OrigWidth))
      {
        Width = OrigWidth;
      }
      Column->Width = Width;

      if (NonResizable)
      {
        NonResizableWidth += Width;
      }
      else
      {
        LastResizable = Index;
        ResizableWidth += Width;
      }
    }

    DebugAssert(LastResizable >= 0);

    int ClientWidth = ListView->ClientWidth;
    int RowCount = ListView->Items->Count;
    if ((ListView->VisibleRowCount < RowCount) &&
        (ListView->Width - ListView->ClientWidth < GetSystemMetricsForControl(ListView, SM_CXVSCROLL)))
    {
      ClientWidth -= GetSystemMetricsForControl(ListView, SM_CXVSCROLL);
    }

    if (DebugAlwaysTrue(NonResizableWidth < ClientWidth))
    {
      int Remaining = ClientWidth - NonResizableWidth;

      bool ProportionalResize = true;

      bool Shrinking = (Remaining < ResizableWidth);
      // If columns are too wide to fit and we have dedicated shrink column, shrink it
      if (Shrinking &&
          (ColumnToShrinkIndex >= 0))
      {
        TListColumn * ColumnToShrink = ListView->Columns->Items[ColumnToShrinkIndex];
        int ColumnToShrinkCaptionWidth = GetColumnTextWidth(ListView, ColumnPadding, ColumnToShrink->Caption);
        int ColumnToShrinkMinWidth = Max(ColumnShrinkMinWidth, ColumnToShrinkCaptionWidth);
        // This falls back to proportional shrinking when the shrink column would fall below min width.
        // Question is whether we should not shrink to min width instead.
        if ((ResizableWidth - ColumnToShrink->Width) < (Remaining - ColumnToShrinkMinWidth))
        {
          int ColumnToShrinkWidth = Remaining - (ResizableWidth - ColumnToShrink->Width);
          ColumnToShrink->Width = ColumnToShrinkWidth;
          ProportionalResize = false;
        }
      }

      if (ProportionalResize)
      {
        for (int Index = 0; Index <= LastResizable; Index++)
        {
          TListColumn * Column = ListView->Columns->Items[Index];

          if (Column->Tag == 0)
          {
            int Width = ListView_GetColumnWidth(ListView->Handle, Index);
            int AutoWidth = Width;
            if (Index < LastResizable)
            {
              Width = (Remaining * Width) / ResizableWidth;
            }
            else
            {
              Width = Remaining;
            }

            int CaptionWidth = GetColumnTextWidth(ListView, ColumnPadding, Column->Caption);
            Width = Max(Width, CaptionWidth);
            Width = Max(Width, Min(ColumnShrinkMinWidth, AutoWidth));
            Column->Width = Width;
            Remaining -= Min(Width, Remaining);
          }
        }

        DebugAssert(Remaining == 0);
      }
    }
  }
  __finally
  {
    SendMessage(ListView->Handle, WM_SETREDRAW, true, 0);
    // As recommended by documentation for WM_SETREDRAW.
    // Without this, session list on Import dialog stops working after the list is reloaded while visible
    // (i.e. when chaining import source)
    RedrawWindow(ListView->Handle, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
  }
}

static void SetParentColor(TControl * Control)
{
  TColor Color = clBtnFace;
  if (UseThemes)
  {
    bool OnTabSheet = false;
    TWinControl * Parent = Control->Parent;
    while ((Parent != nullptr) && !OnTabSheet)
    {
      TTabSheet * TabSheet = dynamic_cast<TTabSheet *>(Parent);
      OnTabSheet = (TabSheet != nullptr) && TabSheet->TabVisible;
      Parent = Parent->Parent;
    }

    if (OnTabSheet)
    {
      HTHEME Theme = OpenThemeData(nullptr, L"tab");
      if (Theme != nullptr)
      {
        COLORREF RGB;
        // XP with classic theme: Does not get past OpenThemeData, clBtnFace is exact color
        // XP with XP theme: not the exact color (probably same as clBtnFace), but close
        // Vista - ok
        // 2016 without desktop - ok
        // 7 with classic and high contrast themes: Do not get past OpenThemeData, clBtnFace is exact color
        // 7 with 7 and basic themes - ok
        // 10 with high contrast themes - ok (note the difference to 7 with high contract themes)
        // 10 - ok
        if (GetThemeColor(Theme, TABP_AEROWIZARDBODY, TIS_NORMAL, TMT_FILLCOLOR, &RGB) == S_OK)
        {
          Color = static_cast<TColor>(RGB);
        }
        CloseThemeData(Theme);
      }
    }
  }

  ((TEdit*)Control)->Color = Color;
}

void EnableControl(TControl * Control, bool Enable)
{
  if (Control->Enabled != Enable)
  {
    TWinControl * WinControl = dynamic_cast<TWinControl *>(Control);
    if ((WinControl != nullptr) &&
        (WinControl->ControlCount > 0))
    {
      for (int Index = 0; Index < WinControl->ControlCount; Index++)
      {
        EnableControl(WinControl->Controls[Index], Enable);
      }
    }
    Control->Enabled = Enable;
  }

  if ((dynamic_cast<TCustomEdit *>(Control) != nullptr) ||
      (dynamic_cast<TCustomComboBox *>(Control) != nullptr) ||
      (dynamic_cast<TCustomListView *>(Control) != nullptr) ||
      (dynamic_cast<TCustomTreeView *>(Control) != nullptr))
  {
    if (Enable)
    {
      ((TEdit*)Control)->Color = clWindow;
    }
    else
    {
      // This does not work for list view with
      // LVS_EX_DOUBLEBUFFER (TCustomDirView).
      // It automatically gets gray background.
      // Though on Windows 7, the control has to be disabled
      // only after it is showing already (see TCustomScpExplorerForm::UpdateControls())
      ((TEdit*)Control)->Color = clBtnFace;
    }
  }
};

void DoReadOnlyControl(TControl * Control, bool ReadOnly, bool Color);

void ReadOnlyAndEnabledControl(TControl * Control, bool ReadOnly, bool Enabled)
{
  // Change color only in only one of EnableControl and DoReadOnlyControl to prevent flicker
  if (ReadOnly)
  {
    DebugAssert(dynamic_cast<TWinControl *>(Control)->ControlCount == 0);
    // As EnableControl, but with no color change
    Control->Enabled = Enabled;
    DoReadOnlyControl(Control, ReadOnly, true);
  }
  else
  {
    DoReadOnlyControl(Control, ReadOnly, false);
    EnableControl(Control, Enabled);
  }
}

static void ReadOnlyEditContextPopup(void * /*Data*/, TObject * Sender, const TPoint & MousePos, bool & Handled)
{
  TEdit * Edit = static_cast<TEdit *>(Sender);
  if (Edit->ReadOnly)
  {
    MenuPopup(Sender, MousePos, Handled);
  }
}

void DoReadOnlyControl(TControl * Control, bool ReadOnly, bool Color)
{
  if (dynamic_cast<TCustomEdit *>(Control) != nullptr)
  {
    TEdit * Edit = static_cast<TEdit *>(Control);
    Edit->ReadOnly = ReadOnly;
    TMemo * Memo = dynamic_cast<TMemo *>(Control);
    if (ReadOnly)
    {
      if (Color)
      {
        SetParentColor(Control);
      }
      if (Memo != nullptr)
      {
        // Is true by default and makes the control swallow not only
        // returns but also escapes.
        // See also MemoKeyDown
        Memo->WantReturns = false;
      }

      if ((Edit->PopupMenu == nullptr) && (dynamic_cast<TPasswordEdit *>(Control) == nullptr))
      {
        std::unique_ptr<TPopupMenu> PopupMenu(new TPopupMenu(Edit));

        TMenuItem * Item;
        Item = new TMenuItem(PopupMenu.get());
        PopupMenu->Items->Add(Item);
        Item->Action = new TEditCopy(Item);
        Item->Caption = LoadStr(EDIT_COPY);
        Item->ShortCut = ShortCut(L'C', TShiftState() << ssCtrl);
        Item = new TMenuItem(PopupMenu.get());
        PopupMenu->Items->Add(Item);
        Item->Action = new TEditSelectAll(Item);
        Item->Caption = LoadStr(EDIT_SELECT_ALL);
        Item->ShortCut = ShortCut(L'A', TShiftState() << ssCtrl);

        Edit->PopupMenu = PopupMenu.release();
        Edit->OnContextPopup = MakeMethod<TContextPopupEvent>(nullptr, ReadOnlyEditContextPopup);
      }
    }
    else
    {
      if (Color)
      {
        Edit->Color = clWindow;
      }
      // not supported atm, we need to persist previous value of WantReturns
      DebugAssert(Memo == nullptr);

      if ((Edit->PopupMenu != nullptr) && (Edit->PopupMenu->Owner == Edit))
      {
        delete Edit->PopupMenu;
      }
    }
  }
  else if ((dynamic_cast<TCustomComboBox *>(Control) != nullptr) ||
           (dynamic_cast<TCustomTreeView *>(Control) != nullptr))
  {
    EnableControl(Control, !ReadOnly);
  }
  else
  {
    DebugFail();
  }
}

void ReadOnlyControl(TControl * Control, bool ReadOnly)
{
  DoReadOnlyControl(Control, ReadOnly, true);
}

int CalculateCheckBoxWidth(TControl * Control, const UnicodeString & Caption)
{
  return
    ScaleByTextHeight(Control, 13 + 3 + 8) + // checkbox, padding and buffer
    GetParentForm(Control)->Canvas->TextWidth(StripHotkey(Caption));
}

void AutoSizeCheckBox(TCheckBox * CheckBox)
{
  CheckBox->Width = CalculateCheckBoxWidth(CheckBox, CheckBox->Caption);
}

// Some of MainFormLike code can now be obsolete, thanks to Application->OnGetMainFormHandle.
static TForm * MainLikeForm = nullptr;

TForm * GetMainForm()
{
  TForm * Result;
  if (MainLikeForm != nullptr)
  {
    Result = MainLikeForm;
  }
  else
  {
    Result = Application->MainForm;
  }
  return Result;
}

bool IsMainFormHidden()
{
  bool Result = false;
  TForm * MainForm = GetMainForm();
  if (MainForm != nullptr)
  {
    Result =
      !MainForm->Visible ||
      (MainForm->Perform(WM_IS_HIDDEN, 0, 0) == 1);
  }
  // we do not expect this to return true when MainLikeForm is set
  DebugAssert(!Result || (MainLikeForm == nullptr));
  return Result;
}

bool IsMainFormLike(TCustomForm * Form)
{
  return
    (GetMainForm() == Form) ||
    // this particularly happens if error occurs while main
    // window is being shown (e.g. non existent local directory when opening
    // explorer)
    IsMainFormHidden();
}

UnicodeString FormatMainFormCaption(const UnicodeString & Caption, const UnicodeString & SessionName)
{
  UnicodeString Suffix = AppName;
  if (!SessionName.IsEmpty())
  {
    Suffix = SessionName + TitleSeparator + Suffix;
  }
  UnicodeString Result = Caption;
  if (Result.IsEmpty())
  {
    Result = Suffix;
  }
  else
  {
    Suffix = TitleSeparator + Suffix;
    if (!EndsStr(Suffix, Result))
    {
      Result += Suffix;
    }
  }
  return Result;
}

UnicodeString FormatFormCaption(
  TCustomForm * Form, const UnicodeString & Caption, const UnicodeString & SessionName)
{
  UnicodeString Result = Caption;
  if (IsMainFormLike(Form))
  {
    Result = FormatMainFormCaption(Result, SessionName);
  }
  return Result;
}

class TPublicWinControl : public TWinControl
{
friend TWndMethod ControlWndProc(TWinControl * Control);
friend void InstallPathWordBreakProc(TWinControl * Control);
};

TWndMethod ControlWndProc(TWinControl * Control)
{
  TPublicWinControl * PublicWinControl = static_cast<TPublicWinControl *>(Control);
  return &PublicWinControl->WndProc;
}

class TPublicControl : public TControl
{
friend void RealignControl(TControl * Control);
friend void DoFormWindowProc(TCustomForm * Form, TWndMethod WndProc, TMessage & Message);
friend TCanvas * CreateControlCanvas(TControl * Control);
};

class TPublicForm : public TForm
{
friend void ChangeFormPixelsPerInch(TForm * Form, int PixelsPerInch);
friend void ShowAsModal(TForm * Form, void *& Storage, bool BringToFront, bool TriggerModalStarted);
friend void HideAsModal(TForm * Form, void *& Storage);
friend void ShowFormNoActivate(TForm * Form);
};

void RealignControl(TControl * Control)
{
  TPublicControl * PublicControl = static_cast<TPublicControl *>(Control);
  PublicControl->RequestAlign();
}

static Forms::TMonitor * LastMonitor = nullptr;

static int GetTextHeightAtPixelsPerInch(TForm * Form, int PixelsPerInch)
{
  std::unique_ptr<TCanvas> Canvas(new TCanvas());
  Canvas->Handle = GetDC(0);
  Canvas->Font->Assign(Form->Font);
  Canvas->Font->PixelsPerInch = PixelsPerInch; // Must be set BEFORE size
  Canvas->Font->Size = Form->Font->Size;
  // VCLCOPY: this is what TCustomForm.GetTextHeight does
  return Canvas->TextHeight(L"0");
}

class TRescaleComponent : public TComponent
{
public:
  TRescaleComponent(TRescaleEvent AOnRescale, TObject * AToken, bool AOwnsToken) :
    TComponent(nullptr)
  {
    OnRescale = AOnRescale;
    Token = AToken;
    OwnsToken = AOwnsToken;
  }

  virtual ~TRescaleComponent()
  {
    if (OwnsToken)
    {
      delete Token;
    }
  }

  TRescaleEvent OnRescale;
  TObject * Token;
  bool OwnsToken;
};

void SetRescaleFunction(
  TComponent * Component, TRescaleEvent OnRescale, TObject * Token, bool OwnsToken)
{
  TRescaleComponent * RescaleComponent = new TRescaleComponent(OnRescale, Token, OwnsToken);
  RescaleComponent->Name = TRescaleComponent::QualifiedClassName();
  Component->InsertComponent(RescaleComponent);
}

static void ChangeControlScale(TControl * Control, int M, int D)
{
  // VCLCOPY This is what TCustomListView.ChangeScale does,
  // but it does that for initial loading only, when ScalingFlags includes sfWidth.
  // But ScalingFlags is reset in TControl.ChangeScale (seems like a bug).
  TCustomListView * CustomListView = dynamic_cast<TCustomListView *>(Control);
  if (CustomListView != nullptr)
  {
    TListView * ListView = reinterpret_cast<TListView *>(CustomListView);
    for (int Index = 0; Index < ListView->Columns->Count; Index++)
    {
      TListColumn * Column = ListView->Columns->Items[Index];
      if ((Column->Width != LVSCW_AUTOSIZE) &&
          (Column->Width != LVSCW_AUTOSIZE_USEHEADER))
      {
        Column->Width = MulDiv(Column->Width, M, D);
      }
    }
  }

  TCustomCombo * CustomCombo = dynamic_cast<TCustomCombo *>(Control);
  if (CustomCombo != nullptr)
  {
    // WORKAROUND
    // Some combo boxes (e.g. path box on Copy dialog) visualize selection after rescaling, even when not focused
    if (!CustomCombo->Focused())
    {
      CustomCombo->SelLength = 0;
    }
  }

  for (int Index = 0; Index < Control->ComponentCount; Index++)
  {
    TComponent * Component = Control->Components[Index];

    TControl * ChildControl = dynamic_cast<TControl *>(Component);
    if (ChildControl != nullptr)
    {
      ChangeControlScale(ChildControl, M, D);
    }

    TRescaleComponent * RescaleComponent =
      dynamic_cast<TRescaleComponent *>(Component->FindComponent(TRescaleComponent::QualifiedClassName()));
    if (RescaleComponent != nullptr)
    {
      RescaleComponent->OnRescale(Component, RescaleComponent->Token);
    }
  }

  Control->Perform(CM_DPICHANGED, M, D);
}

typedef std::pair<int, int> TRatio;
typedef std::map<TRatio, TRatio > TRatioMap;

class TFormRescaleComponent : public TComponent
{
public:
  TFormRescaleComponent() :
    TComponent(nullptr)
  {
    ImplicitRescaleAdded = false;
    Rescaling = false;
  }

  bool Rescaling;
  bool ImplicitRescaleAdded;
  TRatioMap RatioMap;
};

static TFormRescaleComponent * GetFormRescaleComponent(TForm * Form)
{
  TFormRescaleComponent * FormRescaleComponent =
    dynamic_cast<TFormRescaleComponent *>(Form->FindComponent(TFormRescaleComponent::QualifiedClassName()));
  if (FormRescaleComponent == nullptr)
  {
    FormRescaleComponent = new TFormRescaleComponent();
    FormRescaleComponent->Name = TFormRescaleComponent::QualifiedClassName();
    Form->InsertComponent(FormRescaleComponent);
  }
  return FormRescaleComponent;
}

void RecordFormImplicitRescale(TForm * Form)
{
  if (Form->Scaled)
  {
    TFormRescaleComponent * FormRescaleComponent = GetFormRescaleComponent(Form);
    if (!FormRescaleComponent->ImplicitRescaleAdded)
    {
      TRatio Ratio;
      GetFormScaleRatio(Form, Ratio.first, Ratio.second);
      TRatio RescaleKeyRatio(Form->PixelsPerInch, USER_DEFAULT_SCREEN_DPI);
      FormRescaleComponent->RatioMap[RescaleKeyRatio] = Ratio;
      FormRescaleComponent->ImplicitRescaleAdded = true;
    }
  }
}

static void GetFormRescaleRatio(TForm * Form, int PixelsPerInch, int & M, int & D)
{
  TFormRescaleComponent * FormRescaleComponent = GetFormRescaleComponent(Form);
  TRatio ReverseRescaleKeyRatio(Form->PixelsPerInch, PixelsPerInch);
  if (FormRescaleComponent->RatioMap.count(ReverseRescaleKeyRatio) > 0)
  {
    M = FormRescaleComponent->RatioMap[ReverseRescaleKeyRatio].second;
    D = FormRescaleComponent->RatioMap[ReverseRescaleKeyRatio].first;
  }
  else
  {
    M = GetTextHeightAtPixelsPerInch(Form, PixelsPerInch);
    D = GetTextHeightAtPixelsPerInch(Form, Form->PixelsPerInch);
  }


  TRatio RescaleKeyRatio(PixelsPerInch, Form->PixelsPerInch);
  FormRescaleComponent->RatioMap[RescaleKeyRatio] = TRatio(M, D);
}

static void ChangeFormPixelsPerInch(TForm * Form, int PixelsPerInch)
{
  RecordFormImplicitRescale(Form);

  TFormRescaleComponent * FormRescaleComponent = GetFormRescaleComponent(Form);

  if ((Form->PixelsPerInch != PixelsPerInch) && // optimization
      !FormRescaleComponent->Rescaling)
  {
    AppLogFmt(L"Scaling window %s", (Form->Caption));
    TAutoFlag RescalingFlag(FormRescaleComponent->Rescaling);

    int M, D;
    GetFormRescaleRatio(Form, PixelsPerInch, M, D);

    Form->PixelsPerInch = PixelsPerInch;
    TPublicForm * PublicCustomForm = static_cast<TPublicForm *>(Form);

    // WORKAROUND
    // TCustomForm.ChangeScale scales constraints only after rescaling the size,
    // so the unscaled constraints apply to the new size
    // (e.g. with TLoginDialog)
    std::unique_ptr<TSizeConstraints> Constraints(new TSizeConstraints(nullptr));
    Constraints->Assign(PublicCustomForm->Constraints);
    std::unique_ptr<TSizeConstraints> NoConstraints(std::make_unique<TSizeConstraints>(nullptr));
    PublicCustomForm->Constraints = NoConstraints.get();

    // Does not seem to have any effect
    PublicCustomForm->DisableAlign();
    try
    {
      int PrevFontHeight = Form->Font->Height;
      PublicCustomForm->ChangeScale(M, D);
      // Re-rescale by Height as it has higher granularity, hence lower risk of loosing precision due to rounding
      Form->Font->Height = MulDiv(PrevFontHeight, M, D);

      ChangeControlScale(Form, M, D);

      Constraints->MinWidth = MulDiv(Constraints->MinWidth, M, D);
      Constraints->MaxWidth = MulDiv(Constraints->MaxWidth, M, D);
      Constraints->MinHeight = MulDiv(Constraints->MinHeight, M, D);
      Constraints->MaxHeight = MulDiv(Constraints->MaxHeight, M, D);
      PublicCustomForm->Constraints = Constraints.get();
    }
    __finally
    {
      PublicCustomForm->EnableAlign();
    }

  }
}

static void FormShowingChanged(TForm * Form, TWndMethod WndProc, TMessage & Message)
{
  if (IsMainFormLike(Form))
  {
    if (Form->Showing)
    {
      if (Application->MainForm != Form)
      {
        MainLikeForm = Form;

        // When main form is hidden, no taskbar button for it is shown and
        // this window does not become main window, so there won't be no taskbar
        // icon created automatically (by VCL). So we force it manually here.
        // This particularly happen for all windows/dialogs of command-line
        // operations (e.g. /synchronize) after CreateScpExplorer() happens.

        // Also CM_SHOWINGCHANGED happens twice, and the WS_EX_APPWINDOW flag
        // from the first call is not preserved, re-applying on the second call too.

        // TODO: What about minimize to tray?

        int Style = GetWindowLong(Form->Handle, GWL_EXSTYLE);
        if (FLAGCLEAR(Style, WS_EX_APPWINDOW))
        {
          Style |= WS_EX_APPWINDOW;
          SetWindowLong(Form->Handle, GWL_EXSTYLE, Style);
        }
      }

      if (Form->Perform(WM_MANAGES_CAPTION, 0, 0) == 0)
      {
        Form->Caption = FormatFormCaption(Form, Form->Caption);
      }
      SendMessage(Form->Handle, WM_SETICON, ICON_BIG, reinterpret_cast<long>(Application->Icon->Handle));
    }
    else
    {
      if (MainLikeForm == Form)
      {
        // Do not bother with hiding the WS_EX_APPWINDOW flag
        // as the window is closing anyway.
        MainLikeForm = nullptr;
      }
    }
  }

  // Part of following code (but actually not all, TODO), has to happen
  // for all windows when VCL main window is hidden (particularly the last branch).
  // This is different from above branch, that should happen only for top-level visible window.
  if ((Application->MainForm == Form) ||
      // this particularly happens if error occurs while main
      // window is being shown (e.g. non existent local directory when opening
      // explorer)
      ((Application->MainForm != nullptr) && !Application->MainForm->Visible))
  {
    if (!Form->Showing)
    {
      // when closing main form, remember its monitor,
      // so that the next form is shown on the same one
      LastMonitor = Form->Monitor;
    }
    else if ((LastMonitor != nullptr) && (LastMonitor != Form->Monitor) &&
              Form->Showing)
    {
      // would actually always be poScreenCenter, see _SafeFormCreate
      if ((Form->Position == poMainFormCenter) ||
          (Form->Position == poOwnerFormCenter) ||
          (Form->Position == poScreenCenter))
      {
        // this would typically be a standalone message box (e.g. /UninstallCleanup or /Update)

        // If DPI changes (as the form moves to a monitor with a non-system DPI),
        // we have to re-center as the form size changed too.
        int PixelsPerInch;
        do
        {
          PixelsPerInch = Form->PixelsPerInch;
          // taken from TCustomForm::SetWindowToMonitor
          Form->SetBounds(LastMonitor->Left + ((LastMonitor->Width - Form->Width) / 2),
            LastMonitor->Top + ((LastMonitor->Height - Form->Height) / 2),
             Form->Width, Form->Height);
          Form->Position = poDesigned;
        }
        while (PixelsPerInch != Form->PixelsPerInch);
      }
      else if ((Form->Position != poDesigned) &&
               (Form->Position != poDefaultPosOnly))
      {
        // we do not expect any other positioning
        DebugFail();
      }
    }
    // otherwise it would not get centered
    else if ((Form->Position == poMainFormCenter) ||
             (Form->Position == poOwnerFormCenter))
    {
      Form->Position = poScreenCenter;
    }
  }

  if (Form->Showing)
  {
    // At least on single monitor setup, monitor DPI is 100% but system DPI is higher,
    // WM_DPICHANGED is not sent. But VCL scales the form using system DPI.
    // Also we have to do this always for implicitly placed forms (poDefaultPosOnly), like TEditorForm,
    // as they never get the WM_DPICHANGED.
    // Call this before WndProc below, i.e. before OnShow event (particularly important for the TEditorForm::FormShow).

    // GetControlPixelsPerInch would return Form.PixelsPerInch, but we want to get a new DPI of the form monitor.
    ChangeFormPixelsPerInch(Form, GetMonitorPixelsPerInch(GetMonitorFromControl(Form)));
  }

  bool WasFormCenter =
    (Form->Position == poMainFormCenter) ||
    (Form->Position == poOwnerFormCenter);
  WndProc(Message);
  // Make sure dialogs are shown on-screen even if center of the main window
  // is off-screen. Occurs e.g. if you move the main window so that
  // only window title is visible above taksbar.
  if (Form->Showing && WasFormCenter && (Form->Position == poDesigned))
  {
    TRect Rect;
    // Reading Form.Left/Form.Top instead here does not work, likely due to some
    // bug, when querying TProgressForm opened from TEditorForm (reloading remote file)
    GetWindowRect(Form->Handle, &Rect);

    int Left = Rect.Left;
    int Top = Rect.Top;
    TRect WorkArea = Form->Monitor->WorkareaRect;

    if (Left + Rect.Width() > WorkArea.Right)
    {
      Left = WorkArea.Right - Rect.Width();
    }
    if (Left < WorkArea.Left)
    {
      Left = WorkArea.Left;
    }
    if (Top + Rect.Height() > WorkArea.Bottom)
    {
      Top = WorkArea.Bottom - Rect.Height();
    }
    if (Top < WorkArea.Top)
    {
      Top = WorkArea.Top;
    }
    if ((Left != Rect.Left) ||
        (Top != Rect.Top))
    {
      SetWindowPos(Form->Handle, 0, Left, Top, Rect.Width(), Rect.Height(),
        SWP_NOZORDER + SWP_NOACTIVATE);
    }
  }
}

static TCustomForm * WindowPrintForm = nullptr;
static DWORD WindowPrintPrevClick = 0;
static unsigned int WindowPrintClickCount = 0;

void CountClicksForWindowPrint(TForm * Form)
{
  if ((WinConfiguration != nullptr) && WinConfiguration->AllowWindowPrint)
  {
    DWORD Tick = GetTickCount();
    if (WindowPrintForm != Form)
    {
      WindowPrintForm = Form;
      WindowPrintClickCount = 0;
    }
    if (WindowPrintPrevClick < Tick - 500)
    {
      WindowPrintClickCount = 0;
    }
    WindowPrintClickCount++;
    WindowPrintPrevClick = Tick;
    if (WindowPrintClickCount == 3)
    {
      WindowPrintClickCount = 0;

      TInstantOperationVisualizer Visualizer;

      // get the device context of the screen
      HDC ScreenDC = CreateDC(L"DISPLAY", nullptr, nullptr, nullptr);
      // and a device context to put it in
      HDC MemoryDC = CreateCompatibleDC(ScreenDC);

      try
      {
        bool Sizable = (Form->BorderStyle == bsSizeable);
        int Frame = GetSystemMetrics(Sizable ? SM_CXSIZEFRAME : SM_CXFIXEDFRAME) - 1;
        int Width = Form->Width - 2*Frame;
        int Height = Form->Height - Frame;

        // maybe worth checking these are positive values
        HBITMAP Bitmap = CreateCompatibleBitmap(ScreenDC, Width, Height);
        try
        {
          // get a new bitmap
          HBITMAP OldBitmap = static_cast<HBITMAP>(SelectObject(MemoryDC, Bitmap));

          BitBlt(MemoryDC, 0, 0, Width, Height, ScreenDC, Form->Left + Frame, Form->Top, SRCCOPY);
          Bitmap = static_cast<HBITMAP>(SelectObject(MemoryDC, OldBitmap));

          OpenClipboard(nullptr);
          try
          {
            EmptyClipboard();
            SetClipboardData(CF_BITMAP, Bitmap);
          }
          __finally
          {
            CloseClipboard();
          }
        }
        __finally
        {
          DeleteObject(Bitmap);
        }
      }
      __finally
      {
        DeleteDC(MemoryDC);
        DeleteDC(ScreenDC);
      }
    }
  }
}

inline void DoFormWindowProc(TCustomForm * Form, TWndMethod WndProc,
  TMessage & Message)
{
  TForm * AForm = dynamic_cast<TForm *>(Form);
  DebugAssert(AForm != nullptr);
  if ((Message.Msg == WM_SYSCOMMAND) &&
      (Message.WParam == SC_CONTEXTHELP))
  {
    FormHelp(Form);
    Message.Result = 1;
  }
  else if (Message.Msg == CM_SHOWINGCHANGED)
  {
    FormShowingChanged(AForm, WndProc, Message);
  }
  else if (Message.Msg == WM_SETICON)
  {
    // WORKAROUND: Swallow VCL attempt to clear the icon from TCustomForm.WMDestroy.
    // The clearing still happens to be visualised before the form is hidden.
    // On resizable forms, the icon gets replaced by the default application icon,
    // on the other forms, the icon disappears and window caption is shifted left.
    if (Message.LParam != 0)
    {
      WndProc(Message);
    }
  }
  else if (Message.Msg == WM_GETDPISCALEDSIZE)
  {
    WndProc(Message);
    int M, D;
    GetFormRescaleRatio(AForm, LOWORD(Message.WParam), M, D);
    SIZE & Size = *(reinterpret_cast<SIZE *>(Message.LParam));
    Size.cx = MulDiv(Size.cx, M, D);
    Size.cy = MulDiv(Size.cy, M, D);
    Message.Result = TRUE;
  }
  else if (Message.Msg == WM_DPICHANGED)
  {
    ChangeFormPixelsPerInch(AForm, LOWORD(Message.WParam));
    WndProc(Message);
  }
  else if ((Message.Msg == WM_LBUTTONDOWN) || (Message.Msg == WM_LBUTTONDBLCLK))
  {
    CountClicksForWindowPrint(AForm);
    WndProc(Message);
  }
  else
  {
    WndProc(Message);
  }
}

static void FormWindowProc(void * Data, TMessage & Message)
{
  TCustomForm * Form = static_cast<TCustomForm *>(Data);
  DoFormWindowProc(Form, ControlWndProc(Form), Message);
}

void InitializeSystemSettings()
{
}

void FinalizeSystemSettings()
{
}

#ifdef _DEBUG
void VerifyControl(TControl * Control)
{
  // If at this time the control has allocated persistence data that are used
  // for delayed handle recreation, we are at potential risk, as the future
  // de-persistence may overwrite meanwhile changed data.
  // For instance it may happen with list view that DPI scaling gets lost.
  // This for example happens when the list view has both design time
  // ReadOnly = true and some items set. We cannot usually explicitly
  // check for the presence of items as while the listview does not have
  // a handle allocated, item count querying does not work
  // (see also a check below)
  if (ControlHasRecreationPersistenceData(Control))
  {
    // Though if RTL bidi mode is set, the controls are recreated always,
    // as we cannot really prevent it. So we force creation here.
    DebugAssert(Application->BiDiMode != bdLeftToRight);
    TWinControl * WinControl = dynamic_cast<TWinControl *>(Control);
    // It must be TWinControl if ControlHasRecreationPersistenceData returned true
    if (DebugAlwaysTrue(WinControl != nullptr))
    {
      WinControl->HandleNeeded();
    }
  }

  TCustomListView * ListView = dynamic_cast<TCustomListView *>(Control);
  if (ListView != nullptr)
  {
    // As of now the HandleAllocated check is pointless as
    // ListView->Items->Count returns 0 when the handle is not allocated yet.
    // But we want to know if the implementation ever changes to allocate the handle
    // on the call. Because we do not want to allocate a handle here as
    // that would change the debug mode behavior from release behavior,
    // possibly hiding away some problems.
    DebugAssert(!ListView->HandleAllocated() || (ListView->Items->Count == 0));
  }
}
#endif

void ApplySystemSettingsOnControl(TControl * Control)
{
  #ifdef _DEBUG
  VerifyControl(Control);
  #endif

  // WORKAROUND
  // VCL does not scale status par panels (while for instance it does
  // scale list view headers). Remove this if they ever "fix" this.
  // Neither they scale the status bar size if UseSystemFont is true.
  // they claim it should scale with font size, but it does not,
  // probably because they eat WM_SIZE in TCustomStatusBar.WMSize.
  // (used to be issue 83599 on Embarcadero QC, when it was still online)
  // Check TCustomStatusBar.ChangeScale() for changes.
  // For TBX status bars, this is implemented in TTBXCustomStatusBar.ChangeScale
  TStatusBar * StatusBar = dynamic_cast<TStatusBar *>(Control);
  if (StatusBar != nullptr)
  {
    // We should have UseSystemFont and bottom alignment set for all status bars.
    if (DebugAlwaysTrue(StatusBar->UseSystemFont) &&
        DebugAlwaysTrue(StatusBar->Align == alBottom))
    {
      StatusBar->Height = ScaleByTextHeight(StatusBar, StatusBar->Height);
    }

    for (int Index = 0; Index < StatusBar->Panels->Count; Index++)
    {
      TStatusPanel * Panel = StatusBar->Panels->Items[Index];
      Panel->Width = ScaleByTextHeight(StatusBar, Panel->Width);
    }
  }

  TCustomListView * ListView = dynamic_cast<TCustomListView *>(Control);
  TCustomIEListView * IEListView = dynamic_cast<TCustomIEListView *>(Control);
  // For IEListView, this is (somewhat) handled in the TCustomListViewColProperties
  if ((ListView != nullptr) && (IEListView == nullptr))
  {
    TListView * PublicListView = reinterpret_cast<TListView *>(ListView);

    for (int Index = 0; Index < PublicListView->Columns->Count; Index++)
    {
      TListColumn * Column = ListView->Column[Index];
      Column->MaxWidth = ScaleByTextHeight(ListView, Column->MaxWidth);
      Column->MinWidth = ScaleByTextHeight(ListView, Column->MinWidth);
    }
  }

  // WORKAROUND for lack of public API for mimicking Explorer-style mouse selection
  // See https://stackoverflow.com/q/15750842/850848
  if (IEListView != nullptr)
  {
    // It should not be a problem to call the LVM_QUERYINTERFACE
    // on earlier versions of Windows. It should be noop.
    if (IsWin7())
    {
      IListView_Win7 * ListViewIntf = nullptr;
      SendMessage(IEListView->Handle, LVM_QUERYINTERFACE, reinterpret_cast<WPARAM>(&IID_IListView_Win7), reinterpret_cast<LPARAM>(&ListViewIntf));
      if (ListViewIntf != nullptr)
      {
        ListViewIntf->SetSelectionFlags(1, 1);
        ListViewIntf->Release();
      }
      else
      {
        DebugAssert(IsWine());
      }
    }
  }

  TWinControl * WinControl = dynamic_cast<TWinControl *>(Control);
  if (WinControl != nullptr)
  {
    for (int Index = 0; Index < WinControl->ControlCount; Index++)
    {
      ApplySystemSettingsOnControl(WinControl->Controls[Index]);
    }
  }
}

// Settings that must be set as soon as possible.
void UseSystemSettingsPre(TCustomForm * Control)
{
  LocalSystemSettings(Control);

  TWndMethod WindowProc;
  ((TMethod*)&WindowProc)->Data = Control;
  ((TMethod*)&WindowProc)->Code = FormWindowProc;
  Control->WindowProc = WindowProc;

  if (Control->HelpKeyword.IsEmpty())
  {
    // temporary help keyword to enable F1 key in all forms
    Control->HelpKeyword = L"start";
  }

  ApplySystemSettingsOnControl(Control);
};

static void FlipAnchors(TControl * Control)
{
  // WORKAROUND VCL flips the Align, but not the Anchors
  TAnchors Anchors = Control->Anchors;
  if (Anchors.Contains(akLeft) != Anchors.Contains(akRight))
  {
    if (Anchors.Contains(akLeft))
    {
      Anchors << akRight;
      Anchors >> akLeft;
    }
    else
    {
      Anchors << akLeft;
      Anchors >> akRight;
    }
  }
  Control->Anchors = Anchors;

  TWinControl * WinControl = dynamic_cast<TWinControl *>(Control);
  if (WinControl != nullptr)
  {
    for (int Index = 0; Index < WinControl->ControlCount; Index++)
    {
      FlipAnchors(WinControl->Controls[Index]);
    }
  }
}

// Settings that must be set only after whole form is constructed
void UseSystemSettingsPost(TCustomForm * Control)
{
  // When showing an early error message
  if (WinConfiguration != nullptr)
  {
    UnicodeString FlipStr = LoadStr(FLIP_CHILDREN);
    int FlipChildrenFlag =
      AdjustLocaleFlag(FlipStr, WinConfiguration->FlipChildrenOverride, false, true, false);
    if (static_cast<bool>(FlipChildrenFlag))
    {
      Control->FlipChildren(true);

      FlipAnchors(Control);
    }
  }
  ResetSystemSettings(Control);
};

void UseSystemSettings(TCustomForm * Control)
{
  UseSystemSettingsPre(Control);
  UseSystemSettingsPost(Control);
};

void ResetSystemSettings(TCustomForm * /*Control*/)
{
  // noop
}

struct TShowAsModalStorage
{
  bool TriggerModalFinished;
  void * FocusWindowList;
  HWND FocusActiveWindow;
  TFocusState FocusState;
  TCursor SaveCursor;
  int SaveCount;
};

void ShowAsModal(TForm * Form, void *& Storage, bool BringToFront, bool TriggerModalStarted)
{
  SetCorrectFormParent(Form);
  CancelDrag();
  if (GetCapture() != 0) SendMessage(GetCapture(), WM_CANCELMODE, 0, 0);
  ReleaseCapture();
  if (TriggerModalStarted)
  {
    Application->ModalStarted();
  }
  (static_cast<TPublicForm*>(Form))->FFormState << fsModal;

  TShowAsModalStorage * AStorage = new TShowAsModalStorage;

  AStorage->TriggerModalFinished = TriggerModalStarted;
  AStorage->FocusActiveWindow = GetActiveWindow();
  AStorage->FocusState = SaveFocusState();
  Screen->SaveFocusedList->Insert(0, Screen->FocusedForm);
  Screen->FocusedForm = Form;
  AStorage->SaveCursor = Screen->Cursor;
  // This is particularly used when displaying progress window
  // while downloading via temporary folder
  // (the mouse cursor is yet hidden at this point).
  // While the code was added to workaround the above problem,
  // it is taken from TCustomForm.ShowModal and
  // should have been here ever since.
  Screen->Cursor = crDefault;
  AStorage->SaveCount = Screen->CursorCount;
  AStorage->FocusWindowList = DisableTaskWindows(0);

  // VCLCOPY (TCustomForm::Show)
  Form->Visible = true;
  if (BringToFront)
  {
    Form->BringToFront();
  }

  SendMessage(Form->Handle, CM_ACTIVATE, 0, 0);

  Storage = AStorage;
}

void HideAsModal(TForm * Form, void *& Storage)
{
  DebugAssert((static_cast<TPublicForm*>(Form))->FFormState.Contains(fsModal));
  TShowAsModalStorage * AStorage = static_cast<TShowAsModalStorage *>(Storage);
  Storage = nullptr;

  SendMessage(Form->Handle, CM_DEACTIVATE, 0, 0);
  if (GetActiveWindow() != Form->Handle)
  {
    AStorage->FocusActiveWindow = 0;
  }
  Form->Hide();

  if (Screen->CursorCount == AStorage->SaveCount)
  {
    Screen->Cursor = AStorage->SaveCursor;
  }
  else
  {
    Screen->Cursor = crDefault;
  }

  EnableTaskWindows(AStorage->FocusWindowList);

  if (Screen->SaveFocusedList->Count > 0)
  {
    Screen->FocusedForm = static_cast<TCustomForm *>(Screen->SaveFocusedList->First());
    Screen->SaveFocusedList->Remove(Screen->FocusedForm);
  }
  else
  {
    Screen->FocusedForm = nullptr;
  }

  if (AStorage->FocusActiveWindow != 0)
  {
    SetActiveWindow(AStorage->FocusActiveWindow);
  }

  RestoreFocusState(AStorage->FocusState);

  (static_cast<TPublicForm*>(Form))->FFormState >> fsModal;

  if (AStorage->TriggerModalFinished)
  {
    Application->ModalFinished();
  }

  delete AStorage;
}

bool ReleaseAsModal(TForm * Form, void *& Storage)
{
  bool Result = (Storage != nullptr);
  if (Result)
  {
    HideAsModal(Form, Storage);
  }
  return Result;
}

bool SelectDirectory(UnicodeString & Path, const UnicodeString Prompt,
  bool PreserveFileName)
{
  bool Result;
  unsigned int ErrorMode;
  ErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

  try
  {
    UnicodeString Directory;
    UnicodeString FileName;
    // We do not have any real use for the PreserveFileName
    if (!PreserveFileName || DirectoryExists(ApiPath(Path)))
    {
      Directory = Path;
    }
    else
    {
      Directory = ExtractFilePath(Path);
      FileName = ExtractFileName(Path);
    }
    Result = SelectDirectory(Prompt, L"", Directory);
    if (Result)
    {
      Path = Directory;
      if (!FileName.IsEmpty())
      {
        Path = IncludeTrailingBackslash(Path) + FileName;
      }
    }
  }
  __finally
  {
    SetErrorMode(ErrorMode);
  }

  return Result;
}

void SelectDirectoryForEdit(THistoryComboBox * Edit)
{
  UnicodeString OriginalDirectory = ExpandEnvironmentVariables(Edit->Text);
  UnicodeString Directory = OriginalDirectory;
  if (SelectDirectory(Directory, LoadStr(SELECT_LOCAL_DIRECTORY), true) &&
      !SamePaths(OriginalDirectory, Directory))
  {
    Edit->Text = Directory;
    if (Edit->OnChange != nullptr)
    {
      Edit->OnChange(Edit);
    }
  }
}

bool ListViewAnyChecked(TListView * ListView, bool Checked)
{
  bool AnyChecked = false;
  for (int Index = 0; Index < ListView->Items->Count; Index++)
  {
    if (ListView->Items->Item[Index]->Checked == Checked)
    {
      AnyChecked = true;
      break;
    }
  }
  return AnyChecked;
}

void ListViewCheckAll(TListView * ListView,
  TListViewCheckAll CheckAll)
{
  bool Check;

  if (CheckAll == caToggle)
  {
    Check = ListViewAnyChecked(ListView, false);
  }
  else
  {
    Check = (CheckAll == caCheck);
  }

  for (int Index = 0; Index < ListView->Items->Count; Index++)
  {
    ListView->Items->Item[Index]->Checked = Check;
  }
}

void ComboAutoSwitchInitialize(TComboBox * ComboBox)
{
  int PrevIndex = ComboBox->ItemIndex;
  ComboBox->Items->BeginUpdate();
  try
  {
    ComboBox->Clear();
    ComboBox->Items->Add(LoadStr(AUTO_SWITCH_AUTO));
    ComboBox->Items->Add(LoadStr(AUTO_SWITCH_OFF));
    ComboBox->Items->Add(LoadStr(AUTO_SWITCH_ON));
  }
  __finally
  {
    ComboBox->Items->EndUpdate();
  }
  DebugAssert(PrevIndex < ComboBox->Items->Count);
  ComboBox->ItemIndex = PrevIndex;
}

void ComboAutoSwitchLoad(TComboBox * ComboBox, TAutoSwitch Value)
{
  ComboBox->ItemIndex = 2 - Value;
  if (ComboBox->ItemIndex < 0)
  {
    ComboBox->ItemIndex = 0;
  }
}

TAutoSwitch ComboAutoSwitchSave(TComboBox * ComboBox)
{
  return (TAutoSwitch)(2 - ComboBox->ItemIndex);
}

void CheckBoxAutoSwitchLoad(TCheckBox * CheckBox, TAutoSwitch Value)
{
  switch (Value)
  {
    case asOn:
      CheckBox->State = cbChecked;
      break;
    case asOff:
      CheckBox->State = cbUnchecked;
      break;
    default:
      CheckBox->State = cbGrayed;
      break;
  }
}

TAutoSwitch CheckBoxAutoSwitchSave(TCheckBox * CheckBox)
{
  switch (CheckBox->State)
  {
    case cbChecked:
      return asOn;
    case cbUnchecked:
      return asOff;
    default:
      return asAuto;
  }
}

static const wchar_t PathWordDelimiters[] = L"\\/ ;,.\r\n=";

static bool IsPathWordDelimiter(wchar_t Ch)
{
  return (wcschr(PathWordDelimiters, Ch) != nullptr);
}

// Windows algorithm is as follows (tested on W2k):
// right:
//   is_delimiter(current)
//     false:
//       right(left(current) + 1)
//     true:
//       right(right(current) + 1)
// left:
//   right(left(current) + 1)
int CALLBACK PathWordBreakProc(wchar_t * Ch, int Current, int Len, int Code)
{
  int Result;
  UnicodeString ACh(Ch, Len);
  if (Code == WB_ISDELIMITER)
  {
    // we return negacy of what WinAPI docs says
    Result = !IsPathWordDelimiter(ACh[Current + 1]);
  }
  else if (Code == WB_LEFT)
  {
    // skip consecutive delimiters
    while ((Current > 0) &&
           IsPathWordDelimiter(ACh[Current]))
    {
      Current--;
    }
    Result = ACh.SubString(1, Current - 1).LastDelimiter(PathWordDelimiters);
  }
  else if (Code == WB_RIGHT)
  {
    if (Current == 0)
    {
      // will be called again with Current == 1
      Result = 0;
    }
    else
    {
      const wchar_t * P = wcspbrk(ACh.c_str() + Current - 1, PathWordDelimiters);
      if (P == nullptr)
      {
        Result = Len;
      }
      else
      {
        Result = P - ACh.c_str() + 1;
        // skip consecutive delimiters
        while ((Result < Len) &&
               IsPathWordDelimiter(ACh[Result + 1]))
        {
          Result++;
        }
      }
    }
  }
  else
  {
    DebugFail();
    Result = 0;
  }
  return Result;
}

class TPathWordBreakProcComponent : public TComponent
{
public:
  TPathWordBreakProcComponent() :
    TComponent(nullptr)
  {
  }

  void PathWordBreakEditWindowProc(TMessage & Message);

  TWinControl * WinControl;
  TWndMethod PrevWindowProc;
};

void TPathWordBreakProcComponent::PathWordBreakEditWindowProc(TMessage & Message)
{
  bool Handled = false;
  if (Message.Msg == WM_CHAR)
  {
    // Ctrl+Backspace
    // Ctrl+Backspace is handled in WM_CHAR as any other char,
    // so we have to swallow it here to prevent it getting inserted to the text
    TWMChar & CharMessage = *reinterpret_cast<TWMChar *>(&Message);
    if (CharMessage.CharCode == '\x7F')
    {
      TCustomEdit * Edit = dynamic_cast<TCustomEdit *>(WinControl);
      TCustomComboBox * ComboBox = dynamic_cast<TCustomComboBox *>(WinControl);

      if (((Edit != nullptr) && (Edit->SelLength == 0)) ||
          ((ComboBox != nullptr) && (ComboBox->SelLength == 0)))
      {
        // See TCustomMaskEdit.SetCursor
        TKeyboardState KeyState;
        GetKeyboardState(KeyState);
        TKeyboardState NewKeyState;
        memset(NewKeyState, 0, sizeof(NewKeyState));
        NewKeyState[VK_CONTROL] = 0x81;
        NewKeyState[VK_SHIFT] = 0x81;
        SetKeyboardState(NewKeyState);

        SendMessage(WinControl->Handle, WM_KEYDOWN, VK_LEFT, 1);
        NewKeyState[VK_SHIFT] = 0;
        NewKeyState[VK_CONTROL] = 0;
        SetKeyboardState(NewKeyState);

        SendMessage(WinControl->Handle, WM_KEYDOWN, VK_DELETE, 1);
        SetKeyboardState(KeyState);
      }
      Message.Result = 1;
      Handled = true;
    }
  }

  if (!Handled)
  {
    if (PrevWindowProc != nullptr)
    {
      PrevWindowProc(Message);
    }
    else
    {
      ControlWndProc(WinControl)(Message);
    }
  }
}

class TPublicCustomCombo : public TCustomCombo
{
friend void InstallPathWordBreakProc(TWinControl * Control);
};

void InstallPathWordBreakProc(TWinControl * Control)
{
  // Since we are setting Application->ModalPopupMode = pmAuto,
  // this has to be called from OnShow, not from constructor anymore,
  // to have any effect

  HWND Wnd;
  if (dynamic_cast<TCustomCombo*>(Control) != nullptr)
  {
    TPublicCustomCombo * Combo =
      static_cast<TPublicCustomCombo *>(dynamic_cast<TCustomCombo *>(Control));
    Combo->HandleNeeded();
    Wnd = Combo->EditHandle;
  }
  else
  {
    Wnd = Control->Handle;
  }
  SendMessage(Wnd, EM_SETWORDBREAKPROC, 0, (LPARAM)(EDITWORDBREAKPROC)PathWordBreakProc);

  TPathWordBreakProcComponent * PathWordBreakProcComponent = new TPathWordBreakProcComponent();
  PathWordBreakProcComponent->Name = TPathWordBreakProcComponent::QualifiedClassName();
  Control->InsertComponent(PathWordBreakProcComponent);
  PathWordBreakProcComponent->WinControl = Control;
  // Have to remember the proc because of TTBEditItemViewer.EditWndProc
  PathWordBreakProcComponent->PrevWindowProc =
    // Test is probably redundant, it's there to limit impact of the change.
    ((Control->WindowProc != ControlWndProc(Control)) ? Control->WindowProc : TWndMethod());

  Control->WindowProc = PathWordBreakProcComponent->PathWordBreakEditWindowProc;
}

static void RemoveHiddenControlsFromOrder(TControl ** ControlsOrder, int & Count)
{
  int Shift = 0;
  for (int Index = 0; Index < Count; Index++)
  {
    if (ControlsOrder[Index]->Visible)
    {
      ControlsOrder[Index - Shift] = ControlsOrder[Index];
    }
    else
    {
      Shift++;
    }
  }
  Count -= Shift;
}

void SetVerticalControlsOrder(TControl ** ControlsOrder, int Count)
{
  RemoveHiddenControlsFromOrder(ControlsOrder, Count);

  if (Count > 0)
  {
    TWinControl * CommonParent = ControlsOrder[0]->Parent;
    CommonParent->DisableAlign();
    try
    {
      int Top = 0;
      for (int Index = 0; Index < Count; Index++)
      {
        DebugAssert(ControlsOrder[Index]->Parent == CommonParent);
        if ((Index == 0) || (Top > ControlsOrder[Index]->Top))
        {
          Top = ControlsOrder[Index]->Top;
        }
      }

      for (int Index = 0; Index < Count; Index++)
      {
        TControl * Control = ControlsOrder[Index];
        Control->Top = Top;
        if (((Control->Align == alTop) || (Control->Align == alBottom)) ||
            ((Index == Count - 1) || (ControlsOrder[Index + 1]->Align == alBottom)))
        {
          Top += Control->Height;
        }
      }
    }
    __finally
    {
      CommonParent->EnableAlign();
    }
  }
}

void SetHorizontalControlsOrder(TControl ** ControlsOrder, int Count)
{
  RemoveHiddenControlsFromOrder(ControlsOrder, Count);

  if (Count > 0)
  {
    TWinControl * CommonParent = ControlsOrder[0]->Parent;
    CommonParent->DisableAlign();
    try
    {
      int Left = 0;
      for (int Index = 0; Index < Count; Index++)
      {
        DebugAssert(ControlsOrder[Index]->Parent == CommonParent);
        if ((Index == 0) || (Left > ControlsOrder[Index]->Left))
        {
          Left = ControlsOrder[Index]->Left;
        }
      }

      for (int Index = 0; Index < Count; Index++)
      {
        TControl * Control = ControlsOrder[Index];
        Control->Left = Left;
        if (((Control->Align == alLeft) || (Control->Align == alRight)) ||
            ((Index == Count - 1) || (ControlsOrder[Index + 1]->Align == alRight)))
        {
          Left += Control->Width;
        }
        // vertical alignment has priority, so alBottom-aligned controls start
        // at the very left, even if there are any alLeft/alRight controls.
        // for the reason this code is not necessary in SetVerticalControlsOrder.
        // we could exit the loop as well here.
        if ((Index == Count - 1) || (ControlsOrder[Index + 1]->Align == alBottom))
        {
          Left = 0;
        }
      }
    }
    __finally
    {
      CommonParent->EnableAlign();
    }
  }
}

void MakeNextInTabOrder(TWinControl * Control, TWinControl * After)
{
  if (After->TabOrder > Control->TabOrder)
  {
    After->TabOrder = Control->TabOrder;
  }
  else if (After->TabOrder < Control->TabOrder - 1)
  {
    After->TabOrder = static_cast<TTabOrder>(Control->TabOrder - 1);
  }
}

void CutFormToDesktop(TForm * Form)
{
  DebugAssert(Form->Monitor != nullptr);
  TRect Workarea = Form->Monitor->WorkareaRect;
  if (Form->Top + Form->Height > Workarea.Bottom)
  {
    Form->Height = Workarea.Bottom - Form->Top;
  }
  if (Form->Left + Form->Width >= Workarea.Right)
  {
    Form->Width = Workarea.Right - Form->Left;
  }
}

void UpdateFormPosition(TCustomForm * Form, TPosition Position)
{
  if ((Position == poScreenCenter) ||
      (Position == poOwnerFormCenter) ||
      (Position == poMainFormCenter))
  {
    TCustomForm * CenterForm = nullptr;
    if ((Position == poOwnerFormCenter) ||
        (Position == poMainFormCenter))
    {
      CenterForm = Application->MainForm;
      if ((Position == poOwnerFormCenter) &&
          (dynamic_cast<TCustomForm*>(Form->Owner) != nullptr))
      {
        CenterForm = dynamic_cast<TCustomForm*>(Form->Owner);
      }
    }

    TRect Bounds = Form->BoundsRect;
    int X, Y;
    if (CenterForm != nullptr)
    {
      X = ((((TForm *)CenterForm)->Width - Bounds.Width()) / 2) +
        ((TForm *)CenterForm)->Left;
      Y = ((((TForm *)CenterForm)->Height - Bounds.Height()) / 2) +
        ((TForm *)CenterForm)->Top;
    }
    else
    {
      X = (Form->Monitor->Width - Bounds.Width()) / 2;
      Y = (Form->Monitor->Height - Bounds.Height()) / 2;
    }

    if (X < 0)
    {
      X = 0;
    }
    if (Y < 0)
    {
      Y = 0;
    }

    Form->SetBounds(X, Y, Bounds.Width(), Bounds.Height());
  }
}

void ResizeForm(TCustomForm * Form, int Width, int Height)
{
  // This has to be called only after DoFormWindowProc(CM_SHOWINGCHANGED),
  // so that a correct monitor is considered.
  // Note that we cannot use LastMonitor(), as ResizeForm is also called from
  // TConsoleDialog::DoAdjustWindow, where we need to use the actual monitor
  // (in case user moves the console window to a different monitor,
  // than where a main window is [no matter how unlikely that is])
  TRect WorkareaRect = Form->Monitor->WorkareaRect;
  if (Height > WorkareaRect.Height())
  {
    Height = WorkareaRect.Height();
  }
  if (Width > WorkareaRect.Width())
  {
    Width = WorkareaRect.Width();
  }
  if (Height < Form->Constraints->MinHeight)
  {
    Height = Form->Constraints->MinHeight;
  }
  if (Width < Form->Constraints->MinWidth)
  {
    Width = Form->Constraints->MinWidth;
  }
  TRect Bounds = Form->BoundsRect;
  int Top = Bounds.Top + ((Bounds.Height() - Height) / 2);
  int Left = Bounds.Left + ((Bounds.Width() - Width) / 2);
  if (Top + Height > WorkareaRect.Bottom)
  {
    Top = WorkareaRect.Bottom - Height;
  }
  if (Left + Width >= WorkareaRect.Right)
  {
    Left = WorkareaRect.Right - Width;
  }
  // WorkareaRect.Left is not 0, when secondary monitor is placed left of primary one.
  // Similarly for WorkareaRect.Top.
  if (Top < WorkareaRect.Top)
  {
    Top = WorkareaRect.Top;
  }
  if (Left < WorkareaRect.Left)
  {
    Left = WorkareaRect.Left;
  }
  Form->SetBounds(Left, Top, Width, Height);
  Bounds = Form->BoundsRect;
  // due to constraints, form can remain larger, make sure it is centered although
  Left = Bounds.Left + ((Width - Bounds.Width()) / 2);
  Top = Bounds.Top + ((Height - Bounds.Height()) / 2);
  Form->SetBounds(Left, Top, Width, Height);
}

TComponent * GetFormOwner()
{
  if (Screen->ActiveForm != nullptr)
  {
    return Screen->ActiveForm;
  }
  else
  {
    return Application;
  }
}

void SetCorrectFormParent(TForm * /*Form*/)
{
  // noop
  // remove
}

void InvokeHelp(TWinControl * Control)
{
  DebugAssert(Control != nullptr);

  HELPINFO HelpInfo;
  HelpInfo.cbSize = sizeof(HelpInfo);
  HelpInfo.iContextType = HELPINFO_WINDOW;
  HelpInfo.iCtrlId = 0;
  HelpInfo.hItemHandle = Control->Handle;
  HelpInfo.dwContextId = 0;
  HelpInfo.MousePos.x = 0;
  HelpInfo.MousePos.y = 0;
  SendMessage(Control->Handle, WM_HELP, nullptr, reinterpret_cast<long>(&HelpInfo));
}


static void FocusableLabelCanvas(TStaticText * StaticText,
  TCanvas ** ACanvas, TRect & R)
{
  TCanvas * Canvas = CreateControlCanvas(StaticText);
  try
  {
    R = StaticText->ClientRect;

    TSize TextSize;
    if (StaticText->AutoSize)
    {
      // We possibly could use the same code as in !AutoSize branch,
      // keeping this to avoid problems in existing code
      UnicodeString Caption = StaticText->Caption;
      bool AccelChar = false;
      if (StaticText->ShowAccelChar)
      {
        Caption = StripHotkey(Caption);
        AccelChar = (Caption != StaticText->Caption);
      }
      TextSize = Canvas->TextExtent(Caption);

      DebugAssert(StaticText->BorderStyle == sbsNone); // not taken into account
      if (AccelChar)
      {
        TextSize.cy += 2;
      }
    }
    else
    {
      TRect TextRect;
      SetRect(&TextRect, 0, 0, StaticText->Width, 0);
      DrawText(Canvas->Handle, StaticText->Caption.c_str(), -1, &TextRect,
        DT_CALCRECT | DT_WORDBREAK |
        StaticText->DrawTextBiDiModeFlagsReadingOnly());
      TextSize = TextRect.GetSize();
    }

    R.Bottom = R.Top + TextSize.cy;
    // Should call ChangeBiDiModeAlignment when UseRightToLeftAlignment(),
    // but the label seems to draw the text wrongly aligned, even though
    // the alignment is correctly flipped in TCustomStaticText.CreateParams
    switch (StaticText->Alignment)
    {
      case taLeftJustify:
        R.Right = R.Left + TextSize.cx;
        break;

      case taRightJustify:
        R.Left = Max(0, R.Right - TextSize.cx);
        break;

      case taCenter:
        {
          DebugFail(); // not used branch, possibly untested
          int Diff = R.Width() - TextSize.cx;
          R.Left += Diff / 2;
          R.Right -= Diff - (Diff / 2);
        }
        break;
    }
  }
  __finally
  {
    if (ACanvas == nullptr)
    {
      delete Canvas;
    }
  }

  if (ACanvas != nullptr)
  {
    *ACanvas = Canvas;
  }
}

static void FocusableLabelWindowProc(void * Data, TMessage & Message,
  bool & Clicked)
{
  Clicked = false;
  TStaticText * StaticText = static_cast<TStaticText *>(Data);
  if (Message.Msg == WM_LBUTTONDOWN)
  {
    StaticText->SetFocus();
    // in case the action takes long, make sure focus is shown immediately
    UpdateWindow(StaticText->Handle);
    Clicked = true;
    Message.Result = 1;
  }
  else if (Message.Msg == WM_RBUTTONDOWN)
  {
    StaticText->SetFocus();
    Message.Result = 1;
  }
  else if (Message.Msg == WM_CHAR)
  {
    if (reinterpret_cast<TWMChar &>(Message).CharCode == L' ')
    {
      Clicked = true;
      Message.Result = 1;
    }
    else
    {
      ControlWndProc(StaticText)(Message);
    }
  }
  else if (Message.Msg == CM_DIALOGCHAR)
  {
    if (StaticText->CanFocus() && StaticText->ShowAccelChar &&
        IsAccel(reinterpret_cast<TCMDialogChar &>(Message).CharCode, StaticText->Caption))
    {
      StaticText->SetFocus();
      // in case the action takes long, make sure focus is shown immediately
      UpdateWindow(StaticText->Handle);
      Clicked = true;
      Message.Result = 1;
    }
    else
    {
      ControlWndProc(StaticText)(Message);
    }
  }
  else
  {
    ControlWndProc(StaticText)(Message);
  }

  if (Message.Msg == WM_PAINT)
  {
    TRect R;
    TCanvas * Canvas;
    FocusableLabelCanvas(StaticText, &Canvas, R);
    try
    {
      if (StaticText->Focused())
      {
        Canvas->DrawFocusRect(R);
      }
      else if ((StaticText->Font->Color != LinkColor) && // LinkActionLabel and LinkLabel
               !EndsStr(LinkAppLabelMark, StaticText->Caption)) // LinkAppLabel
      {
        Canvas->Pen->Style = psDot;
        Canvas->Brush->Style = bsClear;
        if (!StaticText->Enabled)
        {
          Canvas->Pen->Color = clBtnHighlight;
          Canvas->MoveTo(R.Left + 1 + 1, R.Bottom);
          Canvas->LineTo(R.Right + 1, R.Bottom);
          Canvas->Pen->Color = clGrayText;
        }
        Canvas->MoveTo(R.Left, R.Bottom - 1);
        Canvas->LineTo(R.Right, R.Bottom - 1);
      }
    }
    __finally
    {
      delete Canvas;
    }
  }
  else if ((Message.Msg == WM_SETFOCUS) || (Message.Msg == WM_KILLFOCUS) ||
    (Message.Msg == CM_ENABLEDCHANGED))
  {
    StaticText->Invalidate();
  }
}

static THintWindow * PersistentHintWindow = nullptr;
static TControl * PersistentHintControl = nullptr;

void CancelPersistentHint()
{
  if (PersistentHintWindow != nullptr)
  {
    PersistentHintControl = nullptr;
    SAFE_DESTROY(PersistentHintWindow);
  }
}

void ShowPersistentHint(TControl * Control, TPoint HintPos)
{
  CancelPersistentHint();

  THintInfo HintInfo;
  HintInfo.HintControl = Control;
  HintInfo.HintPos = HintPos;
  HintInfo.HintMaxWidth = GetParentForm(Control)->Monitor->Width;
  HintInfo.HintColor = Application->HintColor;
  HintInfo.HintStr = GetShortHint(Control->Hint);
  HintInfo.HintData = nullptr;

  bool CanShow = true;
  if (Application->OnShowHint != nullptr)
  {
    Application->OnShowHint(HintInfo.HintStr, CanShow, HintInfo);
  }

  if (CanShow)
  {
    PersistentHintControl = Control;

    PersistentHintWindow = new TScreenTipHintWindow(Application);
    PersistentHintWindow->BiDiMode = Control->BiDiMode;
    PersistentHintWindow->Color = HintInfo.HintColor;

    TRect HintWinRect;
    if (HintInfo.HintMaxWidth < Control->Width)
    {
      HintInfo.HintMaxWidth = Control->Width;
    }
    HintWinRect = PersistentHintWindow->CalcHintRect(
      HintInfo.HintMaxWidth, HintInfo.HintStr, HintInfo.HintData);
    OffsetRect(HintWinRect, HintInfo.HintPos.x, HintInfo.HintPos.y);
    // TODO: right align window placement for UseRightToLeftAlignment, see Forms.pas

    PersistentHintWindow->ActivateHintData(HintWinRect, HintInfo.HintStr, HintInfo.HintData);
  }
}

static void HintLabelWindowProc(void * Data, TMessage & Message)
{
  bool Clicked = false;
  bool Cancel = false;

  TStaticText * StaticText = static_cast<TStaticText *>(Data);
  if (Message.Msg == CM_HINTSHOW)
  {
    TCMHintShow & HintShow = reinterpret_cast<TCMHintShow &>(Message);
    if (PersistentHintControl == StaticText)
    {
      // do not allow standard hint when persistent is already shown
      HintShow.Result = 1;
    }
    else
    {
      HintShow.HintInfo->HideTimeout = 100000; // never
    }
  }
  else if (Message.Msg == CN_KEYDOWN)
  {
    if ((reinterpret_cast<TWMKey &>(Message).CharCode == VK_ESCAPE) &&
        (PersistentHintControl == StaticText))
    {
      CancelPersistentHint();
      StaticText->Invalidate();
      Message.Result = 1;
    }
    else
    {
      FocusableLabelWindowProc(Data, Message, Clicked);
    }
  }
  else
  {
    FocusableLabelWindowProc(Data, Message, Clicked);
  }

  if (Message.Msg == CM_CANCELMODE)
  {
    TCMCancelMode & CancelMessage = (TCMCancelMode&)Message;
    if ((CancelMessage.Sender != StaticText) &&
        (CancelMessage.Sender != PersistentHintWindow))
    {
      Cancel = true;
    }
  }

  if ((Message.Msg == WM_DESTROY) || (Message.Msg == WM_KILLFOCUS))
  {
    Cancel = true;
  }

  if (Cancel && (PersistentHintControl == StaticText))
  {
    CancelPersistentHint();
  }

  if (Clicked && (PersistentHintControl != StaticText))
  {
    TRect R;
    TPoint HintPos;

    FocusableLabelCanvas(StaticText, nullptr, R);
    HintPos.y = R.Bottom - R.Top;
    HintPos.x = R.Left;

    ShowPersistentHint(StaticText, StaticText->ClientToScreen(HintPos));
  }
}

void HintLabel(TStaticText * StaticText, UnicodeString Hint)
{
  // Currently all are right-justified, when other alignment is used,
  // test respective branches in FocusableLabelCanvas.
  DebugAssert(StaticText->Alignment == taRightJustify);
  // With right-justify, it has to be off. We may not notice on original
  // English version, results will differ with translations only
  DebugAssert(!StaticText->AutoSize);
  StaticText->ParentFont = true;
  if (!Hint.IsEmpty())
  {
    StaticText->Hint = Hint;
  }
  StaticText->ShowHint = true;
  StaticText->Cursor = crHandPoint;

  TWndMethod WindowProc;
  ((TMethod*)&WindowProc)->Data = StaticText;
  ((TMethod*)&WindowProc)->Code = HintLabelWindowProc;
  StaticText->WindowProc = WindowProc;
}

static void ComboBoxFixWindowProc(void * Data, TMessage & Message)
{
  // it is TCustomComboxBox, but the properties are published only by TComboBox
  TComboBox * ComboBox = static_cast<TComboBox *>(Data);
  if (Message.Msg == WM_SIZE)
  {
    UnicodeString Text = ComboBox->Text;
    try
    {
      ControlWndProc(ComboBox)(Message);
    }
    __finally
    {
      // workaround for bug in combo box, that causes it to change text to any
      // item from drop down list which starts with current text,
      // after control is resized (unless the text is in drop down list as well)
      ComboBox->Text = Text;
      // hide selection, which is wrongly shown when form is resized, even when the box has not focus
      if (!ComboBox->Focused())
      {
        ComboBox->SelLength = 0;
      }
    }
  }
  else
  {
    ControlWndProc(ComboBox)(Message);
  }
}

void FixComboBoxResizeBug(TCustomComboBox * ComboBox)
{
  TWndMethod WindowProc;
  ((TMethod*)&WindowProc)->Data = ComboBox;
  ((TMethod*)&WindowProc)->Code = ComboBoxFixWindowProc;
  ComboBox->WindowProc = WindowProc;
}

static void LinkLabelClick(TStaticText * StaticText)
{
  if (StaticText->OnClick != nullptr)
  {
    StaticText->OnClick(StaticText);
  }
  else
  {
    OpenBrowser(StaticText->Caption);
  }
}

static void LinkLabelWindowProc(void * Data, TMessage & Message)
{
  bool Clicked = false;

  TStaticText * StaticText = static_cast<TStaticText *>(Data);
  if (Message.Msg == WM_CONTEXTMENU)
  {
    TWMContextMenu & ContextMenu = reinterpret_cast<TWMContextMenu &>(Message);

    if ((ContextMenu.Pos.x < 0) && (ContextMenu.Pos.y < 0))
    {
      TRect R;
      FocusableLabelCanvas(StaticText, nullptr, R);
      TPoint P = StaticText->ClientToScreen(TPoint(R.Left, R.Bottom));
      ContextMenu.Pos.x = static_cast<short>(P.x);
      ContextMenu.Pos.y = static_cast<short>(P.y);
    }
  }
  else if (Message.Msg == WM_KEYDOWN)
  {
    TWMKey & Key = reinterpret_cast<TWMKey &>(Message);
    if ((GetKeyState(VK_CONTROL) < 0) && (Key.CharCode == L'C'))
    {
      TInstantOperationVisualizer Visualizer;
      CopyToClipboard(StaticText->Caption);
      Message.Result = 1;
    }
    else
    {
      FocusableLabelWindowProc(Data, Message, Clicked);
    }
  }

  FocusableLabelWindowProc(Data, Message, Clicked);

  if (Message.Msg == WM_DESTROY)
  {
    delete StaticText->PopupMenu;
    DebugAssert(StaticText->PopupMenu == nullptr);
  }

  if (Clicked)
  {
    LinkLabelClick(StaticText);
  }
}

static void LinkLabelContextMenuClick(void * Data, TObject * Sender)
{
  TStaticText * StaticText = static_cast<TStaticText *>(Data);
  TMenuItem * MenuItem = dynamic_cast<TMenuItem *>(Sender);
  DebugAssert(MenuItem != nullptr);

  if (MenuItem->Tag == 0)
  {
    LinkLabelClick(StaticText);
  }
  else
  {
    TInstantOperationVisualizer Visualizer;
    CopyToClipboard(StaticText->Caption);
  }
}

static void DoLinkLabel(TStaticText * StaticText)
{
  StaticText->Transparent = false;
  StaticText->ParentFont = true;
  StaticText->Cursor = crHandPoint;

  TWndMethod WindowProc;
  ((TMethod*)&WindowProc)->Data = StaticText;
  ((TMethod*)&WindowProc)->Code = LinkLabelWindowProc;
  StaticText->WindowProc = WindowProc;
}

void LinkLabel(TStaticText * StaticText, UnicodeString Url,
  TNotifyEvent OnEnter)
{
  DoLinkLabel(StaticText);

  StaticText->Font->Style = StaticText->Font->Style << fsUnderline;

  reinterpret_cast<TButton*>(StaticText)->OnEnter = OnEnter;

  if (!Url.IsEmpty())
  {
    StaticText->Caption = Url;
  }

  bool IsUrl = IsHttpOrHttpsUrl(StaticText->Caption);
  if (IsUrl)
  {
    DebugAssert(StaticText->PopupMenu == nullptr);
    StaticText->PopupMenu = new TPopupMenu(StaticText);
    try
    {
      TNotifyEvent ContextMenuOnClick;
      ((TMethod*)&ContextMenuOnClick)->Data = StaticText;
      ((TMethod*)&ContextMenuOnClick)->Code = LinkLabelContextMenuClick;

      TMenuItem * Item;

      Item = new TMenuItem(StaticText->PopupMenu);
      Item->Caption = LoadStr(URL_LINK_OPEN);
      Item->Tag = 0;
      Item->ShortCut = ShortCut(L' ', TShiftState());
      Item->OnClick = ContextMenuOnClick;
      StaticText->PopupMenu->Items->Add(Item);

      Item = new TMenuItem(StaticText->PopupMenu);
      Item->Caption = LoadStr(EDIT_COPY);
      Item->Tag = 1;
      Item->ShortCut = ShortCut(L'C', TShiftState() << ssCtrl);
      Item->OnClick = ContextMenuOnClick;
      StaticText->PopupMenu->Items->Add(Item);
    }
    catch(...)
    {
      delete StaticText->PopupMenu;
      DebugAssert(StaticText->PopupMenu == nullptr);
      throw;
    }
  }

  StaticText->Font->Color = LinkColor;
}

void LinkActionLabel(TStaticText * StaticText)
{
  DoLinkLabel(StaticText);

  StaticText->Font->Color = LinkColor;
}

void LinkAppLabel(TStaticText * StaticText)
{
  DoLinkLabel(StaticText);

  StaticText->Caption = StaticText->Caption + LinkAppLabelMark;
}

static void HotTrackLabelMouseEnter(void * /*Data*/, TObject * Sender)
{
  reinterpret_cast<TLabel *>(Sender)->Font->Color = clBlue;
}

static void HotTrackLabelMouseLeave(void * /*Data*/, TObject * Sender)
{
  reinterpret_cast<TLabel *>(Sender)->ParentFont = true;
}

void HotTrackLabel(TLabel * Label)
{
  DebugAssert(Label->OnMouseEnter == nullptr);
  DebugAssert(Label->OnMouseLeave == nullptr);

  Label->OnMouseEnter = MakeMethod<TNotifyEvent>(nullptr, HotTrackLabelMouseEnter);
  Label->OnMouseLeave = MakeMethod<TNotifyEvent>(nullptr, HotTrackLabelMouseLeave);
}

void SetLabelHintPopup(TLabel * Label, const UnicodeString & Hint)
{
  Label->Caption = Hint;
  Label->Hint = Hint;
  TRect Rect(0, 0, Label->Width, 0);
  TScreenTipHintWindow::CalcHintTextRect(Label, Label->Canvas, Rect, Label->Caption);
  Label->ShowHint = (Rect.Bottom > Label->Height);
}

bool HasLabelHintPopup(TControl * Control, const UnicodeString & HintStr)
{
  TLabel * HintLabel = dynamic_cast<TLabel *>(Control);
  return (HintLabel != nullptr) && (GetShortHint(HintLabel->Caption) == HintStr);
}

Forms::TMonitor *  FormMonitor(TCustomForm * Form)
{
  Forms::TMonitor * Result;
  if ((Application->MainForm != nullptr) && (Application->MainForm != Form))
  {
    Result = Application->MainForm->Monitor;
  }
  else if (LastMonitor != nullptr)
  {
    Result = LastMonitor;
  }
  else
  {
    int i = 0;
    while ((i < Screen->MonitorCount) && !Screen->Monitors[i]->Primary)
    {
      i++;
    }
    DebugAssert(Screen->Monitors[i]->Primary);
    Result = Screen->Monitors[i];
  }
  return Result;
}

int GetLastMonitor()
{
  if (LastMonitor != nullptr)
  {
    return LastMonitor->MonitorNum;
  }
  else
  {
    return -1;
  }
}

void SetLastMonitor(int MonitorNum)
{
  if ((MonitorNum >= 0) && (MonitorNum < Screen->MonitorCount))
  {
    LastMonitor = Screen->Monitors[MonitorNum];
  }
  else
  {
    LastMonitor = nullptr;
  }
}

TForm * _SafeFormCreate(TMetaClass * FormClass, TComponent * Owner)
{
  TForm * Form;

  if (Owner == nullptr)
  {
    Owner = GetFormOwner();
  }

  // If there is no main form yet, make this one main.
  // This:
  // - Makes other forms (dialogs invoked from this one),
  // be placed on the same monitor (otherwise all new forms get placed
  // on primary monitor)
  // - Triggers MainForm-specific code in DoFormWindowProc.
  // - Shows button on taskbar
  if (Application->MainForm == nullptr)
  {
    Application->CreateForm(FormClass, &Form);
    DebugAssert(Application->MainForm == Form);
  }
  else
  {
    Form = dynamic_cast<TForm *>(Construct(FormClass, Owner));
    DebugAssert(Form != nullptr);
  }

  return Form;
}

bool SupportsSplitButton()
{
  return (Win32MajorVersion >= 6);
}

static TButton * FindStandardButton(TWinControl * Control, bool Default)
{
  TButton * Result = nullptr;
  int Index = 0;
  while ((Result == nullptr) && (Index < Control->ControlCount))
  {
    TControl * ChildControl = Control->Controls[Index];
    TButton * Button = dynamic_cast<TButton *>(ChildControl);
    if ((Button != nullptr) && (Default ? Button->Default : Button->Cancel))
    {
      Result = Button;
    }
    else
    {
      TWinControl * WinControl = dynamic_cast<TWinControl *>(ChildControl);
      if (WinControl != nullptr)
      {
        Result = FindStandardButton(WinControl, Default);
      }
    }
    Index++;
  }
  return Result;
}

TModalResult DefaultResult(TCustomForm * Form, TButton * DefaultButton)
{
  // The point of this is to avoid hardcoding mrOk when checking dialog results.
  // Previously we used != mrCancel instead, as mrCancel is more reliable,
  // being automatically used for Esc/X buttons (and hence kind of forced to be used
  // for Cancel buttons). But that failed to be reliable in the end, for
  // ModalResult being mrNone, when Windows session is being logged off.
  // We interpreted mrNone as OK, causing lots of troubles.
  TModalResult Result = mrNone;
  TButton * Button = FindStandardButton(Form, true);
  if (DebugAlwaysTrue(Button != nullptr))
  {
    Result = Button->ModalResult;
  }
  if (Result == mrNone)
  {
    DebugAssert((DefaultButton != nullptr) && (DefaultButton->ModalResult != mrNone));
    Result = DefaultButton->ModalResult;
  }
  else
  {
    // If default button fallback was provided,
    // make sure it is the default button we actually detected
    DebugAssert((DefaultButton == nullptr) || (Button == DefaultButton));
  }
  return Result;
}

void DefaultButton(TButton * Button, bool Default)
{
  // default property setter does not have guard for "the same value"
  if (Button->Default != Default)
  {
    Button->Default = Default;
  }
}

void MemoKeyDown(TObject * Sender, WORD & Key, TShiftState Shift)
{
  // Sender can be Form or Memo itself
  TControl * Control = dynamic_cast<TControl *>(Sender);
  if (DebugAlwaysTrue(Control != nullptr))
  {
    TCustomForm * Form = GetParentForm(Control);
    // Particularly when WantReturns is true,
    // memo swallows also Esc, so we have to handle it ourselves.
    // See also ReadOnlyControl.
    if ((Key == VK_ESCAPE) && Shift.Empty())
    {
      Form->ModalResult = mrCancel;
      Key = 0;
    }
    else if ((Key == VK_RETURN) && Shift.Contains(ssCtrl))
    {
      Form->ModalResult = DefaultResult(Form);
      Key = 0;
    }
  }
}

class TIconOwnerComponent : public TComponent
{
public:
  TIconOwnerComponent(TIcon * Icon) :
    TComponent(nullptr),
    FIcon(Icon)
  {
  }

private:
  std::unique_ptr<TIcon> FIcon;
};

static void FixFormIcon(TForm * Form, int Size, int WidthMetric, int HeightMetric)
{
  // Whole this code is to call ReadIcon from Vcl.Graphics.pas with correct size

  // Clone the icon data (whole .ico file content, that is originally loaded from .dfm)
  // to a new TIcon that does not have a size fixed yet (size cannot be changed after handle is allocated)
  std::unique_ptr<TMemoryStream> Stream(std::make_unique<TMemoryStream>());
  Form->Icon->SaveToStream(Stream.get());
  std::unique_ptr<TIcon> Icon(new TIcon());
  Stream->Position = 0;
  Icon->LoadFromStream(Stream.get());

  // Set desired size
  int Width = GetSystemMetricsForControl(Form, WidthMetric);
  int Height = GetSystemMetricsForControl(Form, HeightMetric);
  Icon->SetSize(Width, Height);

  // This calls TIcon::RequireHandle that retrieves the best icon for given size
  LPARAM LParam = reinterpret_cast<LPARAM>(Icon->Handle);
  SendMessage(Form->Handle, WM_SETICON, Size, LParam);

  // Make sure the icon is released
  TIconOwnerComponent * IconOwnerComponent = new TIconOwnerComponent(Icon.release());
  IconOwnerComponent->Name = TIconOwnerComponent::QualifiedClassName() + IntToStr(Size);
  Form->InsertComponent(IconOwnerComponent);
}

void FixFormIcons(TForm * Form)
{
  // VCL sets only ICON_BIG (so small icon is scaled down by OS from big icon),
  // and it uses a random (first?) size from the resource,
  // not the best size.
  FixFormIcon(Form, ICON_SMALL, SM_CXSMICON, SM_CYSMICON);
  FixFormIcon(Form, ICON_BIG, SM_CXICON, SM_CYICON);
  // We rely on VCL not calling WM_SETICON ever after
  // (what it would do, if CreateWnd is called again).
  // That would overwrite the ICON_BIG.
  // We might be able to make sure it uses a correct size by calling
  // TIcon.ReleaseHandle and setting a correct size.
}

class TDesktopFontManager : public TComponent
{
public:
  TDesktopFontManager();
  virtual ~TDesktopFontManager();

  void AddControl(TControl * Control);
  void Update();

protected:
  virtual void Notification(TComponent * AComponent, TOperation Operation);

private:
  HWND FWindowHandle;
  typedef std::set<TControl *> TControlSet;
  TControlSet FControls;

  void WndProc(TMessage & Message);
  void UpdateControl(TControl * Control);
};

TDesktopFontManager::TDesktopFontManager() : TComponent(nullptr)
{
  // Alternative is using Application->HookMainWindow
  FWindowHandle = AllocateHWnd(WndProc);
}

TDesktopFontManager::~TDesktopFontManager()
{
  DeallocateHWnd(FWindowHandle);
}

void TDesktopFontManager::Notification(TComponent * AComponent, TOperation Operation)
{
  if (DebugAlwaysTrue(Operation == opRemove))
  {
    TControl * Control = DebugNotNull(dynamic_cast<TControl *>(AComponent));
    FControls.erase(Control);
  }

  TComponent::Notification(AComponent, Operation);
}

void TDesktopFontManager::UpdateControl(TControl * Control)
{
  class TPublicControl : public TControl
  {
  public:
    __property Font;
  };

  TPublicControl * PublicControl = reinterpret_cast<TPublicControl *>(Control);
  std::unique_ptr<TFont> DesktopFont(new TFont());
  if (WinConfiguration->PanelFont.FontName.IsEmpty())
  {
    int PixelsPerInch = GetControlPixelsPerInch(Control);
    TLogFont LogFont;
    if (DebugAlwaysTrue(SystemParametersInfoForPixelsPerInch(SPI_GETICONTITLELOGFONT, sizeof(LogFont), &LogFont, 0, PixelsPerInch)))
    {
      DesktopFont->Handle = CreateFontIndirect(&LogFont);
    }
  }
  else
  {
    TWinConfiguration::RestoreFont(WinConfiguration->PanelFont, DesktopFont.get());
    DesktopFont->Height = ScaleByPixelsPerInchFromSystem(DesktopFont->Height, Control);
    DesktopFont->PixelsPerInch = PublicControl->Font->PixelsPerInch;
  }

  // Neither CreateFontIndirect nor RestoreFont set color, so we should have the default set by TFont constructor here.
  DebugAssert(DesktopFont->Color == clWindowText);
  if (!SameFont(DesktopFont.get(), PublicControl->Font) ||
      (DesktopFont->PixelsPerInch != PublicControl->Font->PixelsPerInch))
  {
    // Preserve color (particularly white color of file panel font in dark mode)
    DesktopFont->Color = PublicControl->Font->Color;

    PublicControl->Font->Assign(DesktopFont.get());
  }
}

void TDesktopFontManager::AddControl(TControl * Control)
{
  FControls.insert(Control);
  Control->FreeNotification(this);
  UpdateControl(Control);
}

void TDesktopFontManager::Update()
{
  TControlSet::iterator I = FControls.begin();
  while (I != FControls.end())
  {
    UpdateControl(*I);
    I++;
  }
}

void TDesktopFontManager::WndProc(TMessage & Message)
{
  if (Message.Msg == WM_WININICHANGE)
  {
    Update();
  }
  Message.Result = DefWindowProc(FWindowHandle, Message.Msg, Message.WParam, Message.LParam);
}

std::unique_ptr<TDesktopFontManager> DesktopFontManager(new TDesktopFontManager());

void UseDesktopFont(TControl * Control)
{
  TCustomStatusBar * StatusBar = dynamic_cast<TCustomStatusBar *>(Control);
  if (StatusBar != nullptr)
  {
    // prevent syncing to system font
    StatusBar->UseSystemFont = false;
  }

  DesktopFontManager->AddControl(Control);
}

void UpdateDesktopFont()
{
  DesktopFontManager->Update();
}

TShiftState AllKeyShiftStates()
{
  return TShiftState() << ssShift << ssAlt << ssCtrl;
}

static bool FormActivationHook(void * Data, TMessage & Message)
{
  bool Result = false;
  // Some dialogs, when application is restored from minimization,
  // do not get activated. So we do it explicitly here.
  // We cannot do this from TApplication::OnActivate because
  // TApplication.WndProc resets focus to the last active window afterwards.
  // So we override CM_ACTIVATE implementation here completely.
  if (Message.Msg == CM_ACTIVATE)
  {
    TCustomForm * Form = static_cast<TCustomForm *>(Data);
    if (Screen->FocusedForm == Form)
    {
      ::SetFocus(Form->Handle);
      // VCLCOPY
      if (Application->OnActivate != nullptr)
      {
        Application->OnActivate(Application);
      }
      Result = true;
    }
  }
  return Result;
}

void HookFormActivation(TCustomForm * Form)
{
  Application->HookMainWindow(MakeMethod<TWindowHook>(Form, FormActivationHook));
}

void UnhookFormActivation(TCustomForm * Form)
{
  Application->UnhookMainWindow(MakeMethod<TWindowHook>(Form, FormActivationHook));
}

void ShowFormNoActivate(TForm * Form)
{
  TPublicForm * PublicForm = static_cast<TPublicForm *>(Form);

  // This is same as SendToBack, except for added SWP_NOACTIVATE (VCLCOPY)
  SetWindowPos(PublicForm->WindowHandle, HWND_BOTTOM, 0, 0, 0, 0,
    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

  // This replaces TCustomForm::CMShowingChanged()
  // which calls ShowWindow(Handle, SW_SHOWNORMAL).

  ShowWindow(Form->Handle, SW_SHOWNOACTIVATE);

  // - so we have to call DoShow explicitly.
  PublicForm->DoShow();

  // - also we skip applying TForm::Position (VCLCOPY)
  if (DebugAlwaysTrue(Form->Position == poOwnerFormCenter))
  {
    TCustomForm * CenterForm = Application->MainForm;
    TCustomForm * OwnerForm = dynamic_cast<TCustomForm *>(Form->Owner);
    if (OwnerForm != nullptr)
    {
      CenterForm = OwnerForm;
    }
    int X, Y;
    if ((CenterForm != nullptr) && (CenterForm != Form))
    {
      TRect Bounds = CenterForm->BoundsRect;
      X = ((Bounds.Width() - Form->Width) / 2) + CenterForm->Left;
      Y = ((Bounds.Height() - Form->Height) / 2) + CenterForm->Top;
    }
    else
    {
      X = (Screen->Width - Form->Width) / 2;
      Y = (Screen->Height - Form->Height) / 2;
    }
    if (X < Screen->DesktopLeft)
    {
      X = Screen->DesktopLeft;
    }
    if (Y < Screen->DesktopTop)
    {
      Y = Screen->DesktopTop;
    }
    Form->SetBounds(X, Y, Form->Width, Form->Height);
    // We cannot call SetWindowToMonitor().
    // We cannot set FPosition = poDesigned, so workarea-checking code
    // in DoFormWindowProc is not triggered

    // If application is restored, dialog is not activated, do it manually.
    // Wait for application to be activated to activate ourself.
    HookFormActivation(Form);
  }
}

TPanel * CreateBlankPanel(TComponent * Owner)
{
  TPanel * Panel = new TPanel(Owner);
  Panel->BevelOuter = bvNone;
  Panel->BevelInner = bvNone; // default
  Panel->BevelKind = bkNone;
  return Panel;
}

bool IsButtonBeingClicked(TButtonControl * Button)
{
  class TPublicButtonControl : public TButtonControl
  {
  public:
    __property ClicksDisabled;
  };
  TPublicButtonControl * PublicButton = reinterpret_cast<TPublicButtonControl *>(Button);
  // HACK ClicksDisabled is set in TButtonControl.WndProc while changing focus as response to WM_LBUTTONDOWN.
  return PublicButton->ClicksDisabled;
}

// When using this in OnExit handers, it's still possible that the user does not actually click the
// CancelButton (for example, when the button is released out of the button).
// Then the validation is bypassed. Consequently, all dialogs that uses this must still
// gracefully handle submission with non-validated data.
bool IsCancelButtonBeingClicked(TControl * Control)
{
  TCustomForm * Form = GetParentForm(Control);
  TButtonControl * CancelButton = FindStandardButton(Form, false);
  // Find dialog has no Cancel button
  return (CancelButton != nullptr) && IsButtonBeingClicked(CancelButton);
}

TCanvas * CreateControlCanvas(TControl * Control)
{
  std::unique_ptr<TControlCanvas> Canvas(new TControlCanvas());
  Canvas->Control = Control;
  TPublicControl * PublicControl = static_cast<TPublicControl *>(Control);
  Canvas->Font = PublicControl->Font;
  return Canvas.release();
}

void AutoSizeButton(TButton * Button)
{
  std::unique_ptr<TCanvas> Canvas(CreateControlCanvas(Button));
  int MinWidth = Canvas->TextWidth(Button->Caption) + ScaleByTextHeight(Button, (2 * 8));
  if (Button->Width < MinWidth)
  {
    if (Button->Anchors.Contains(akRight))
    {
      Button->Left = Button->Left - (MinWidth - Button->Width);
    }
    Button->Width = MinWidth;
  }
}

void GiveTBItemPriority(TTBCustomItem * Item)
{
  DebugAssert(Item->GetTopComponent() != nullptr);
  TTBCustomToolbar * ToolbarComponent = dynamic_cast<TTBCustomToolbar *>(Item->GetTopComponent());
  if ((ToolbarComponent != nullptr) &&
      // Only for top-level buttons on custom command toolbar, not for submenus of the custom commands menu in the main menu
      (Item->Parent == ToolbarComponent->Items))
  {
    TTBItemViewer * Viewer = ToolbarComponent->View->Find(Item);
    ToolbarComponent->View->GivePriority(Viewer);
  }
}
