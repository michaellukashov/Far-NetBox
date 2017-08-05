
#include <vcl.h>
#pragma hdrstop

#if defined(_MSC_VER) && !defined(__clang__)
#include <shlobj.h>
#include <shellapi.h>
#endif // if defined(_MSC_VER) && !defined(__clang__)
#include <Common.h>

#include "GUITools.h"
#include "GUIConfiguration.h"
#include <TextsCore.h>
#include <CoreMain.h>
#include <SessionData.h>
#include <WinInterface.h>
#include <Interface.h>

#if 0
#include <TbxUtils.hpp>
#include <Math.hpp>
#include <WebBrowserEx.hpp>
#include <Tools.h>
#include "PngImageList.hpp"
#include <StrUtils.hpp>
#include <limits>
#include <Glyphs.h>
#include <PasTools.hpp>
#include <VCLCommon.h>
#include <Vcl.ScreenTips.hpp>

#include "Animations96.h"
#include "Animations120.h"
#include "Animations144.h"
#include "Animations192.h"

#pragma package(smart_init)
#endif // #if 0

extern const UnicodeString PageantTool = L"pageant.exe";
extern const UnicodeString PuttygenTool = L"puttygen.exe";

bool FindFile(UnicodeString & APath)
{
  bool Result = ::FileExists(ApiPath(APath));
  if (!Result)
  {
    UnicodeString ProgramFiles32 = ::IncludeTrailingBackslash(base::GetEnvVariable(L"ProgramFiles"));
    UnicodeString ProgramFiles64 = ::IncludeTrailingBackslash(base::GetEnvVariable(L"ProgramW6432"));
    if (!ProgramFiles32.IsEmpty() &&
      SameText(APath.SubString(1, ProgramFiles32.Length()), ProgramFiles32) &&
      !ProgramFiles64.IsEmpty())
    {
      UnicodeString Path64 =
        ProgramFiles64 + APath.SubString(ProgramFiles32.Length() + 1, APath.Length() - ProgramFiles32.Length());
      if (::FileExists(ApiPath(Path64)))
      {
        APath = Path64;
        Result = true;
      }
    }
  }

  if (!Result)
  {
    UnicodeString Paths = base::GetEnvVariable("PATH");
    if (Paths.Length() > 0)
    {
      UnicodeString NewPath = ::FileSearch(base::ExtractFileName(APath, false), Paths);
      Result = !NewPath.IsEmpty();
      if (Result)
      {
        APath = NewPath;
      }
    }
  }
  return Result;
}

void OpenSessionInPutty(UnicodeString PuttyPath,
  TSessionData * SessionData)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ::ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {

    Params = ::ExpandEnvironmentVariables(Params);
    UnicodeString Password = GetGUIConfiguration()->GetPuttyPassword() ? SessionData->GetPassword() : UnicodeString();
    TCustomCommandData Data(SessionData, SessionData->SessionGetUserName(), Password);
    TRemoteCustomCommand RemoteCustomCommand(Data, SessionData->GetRemoteDirectory());
    TWinInteractiveCustomCommand InteractiveCustomCommand(
      &RemoteCustomCommand, L"PuTTY", UnicodeString());

    UnicodeString Params2 =
      RemoteCustomCommand.Complete(InteractiveCustomCommand.Complete(Params, false), true);
    UnicodeString PuttyParams;

    if (!RemoteCustomCommand.IsSiteCommand(Params))
    {
      UnicodeString SessionName;
      TRegistryStorage * Storage = nullptr;
      TSessionData * ExportData = nullptr;
      TRegistryStorage * SourceStorage = nullptr;
      try__finally
      {
        SCOPE_EXIT
        {
          delete Storage;
          delete ExportData;
          delete SourceStorage;
        };
        Storage = new TRegistryStorage(GetConfiguration()->GetPuttySessionsKey());
        Storage->SetAccessMode(smReadWrite);
        // make it compatible with putty
        Storage->SetMungeStringValues(false);
        Storage->SetForceAnsi(true);
        if (Storage->OpenRootKey(true))
        {
          if (Storage->KeyExists(SessionData->GetStorageKey()))
          {
            SessionName = SessionData->GetSessionName();
          }
          else
          {
            SourceStorage = new TRegistryStorage(GetConfiguration()->GetPuttySessionsKey());
            SourceStorage->SetMungeStringValues(false);
            SourceStorage->SetForceAnsi(true);
            if (SourceStorage->OpenSubKey(StoredSessions->GetDefaultSettings()->GetName(), false) &&
              Storage->OpenSubKey(GetGUIConfiguration()->GetPuttySession(), true))
            {
              Storage->Copy(SourceStorage);
              Storage->CloseSubKey();
            }

            ExportData = new TSessionData(L"");
            ExportData->Assign(SessionData);
            ExportData->SetModified(true);
            ExportData->SetName(GetGUIConfiguration()->GetPuttySession());
            ExportData->SetWinTitle(SessionData->GetSessionName());
            ExportData->SetPassword(L"");

            if (SessionData->GetFSProtocol() == fsFTP)
            {
              if (GetGUIConfiguration()->GetTelnetForFtpInPutty())
              {
                ExportData->SetPuttyProtocol(PuttyTelnetProtocol);
                ExportData->SetPortNumber(TelnetPortNumber);
                // PuTTY  does not allow -pw for telnet
                Password = L"";
              }
              else
              {
                ExportData->SetPuttyProtocol(PuttySshProtocol);
                ExportData->SetPortNumber(SshPortNumber);
              }
            }

            ExportData->Save(Storage, true);
            SessionName = GetGUIConfiguration()->GetPuttySession();
          }
        }
      }
      __finally
      {
#if 0
        delete Storage;
        delete ExportData;
        delete SourceStorage;
#endif // #if 0
      };

      UnicodeString LoadSwitch = L"-load";
      intptr_t P = Params2.LowerCase().Pos(LoadSwitch + L" ");
      if ((P == 0) || ((P > 1) && (Params2[P - 1] != L' ')))
      {
        AddToList(PuttyParams, FORMAT(L"%s %s", LoadSwitch, EscapePuttyCommandParam(SessionName)), L" ");
      }
    }

    if (!Password.IsEmpty() && !RemoteCustomCommand.IsPasswordCommand(Params))
    {
      AddToList(PuttyParams, FORMAT("-pw %s", EscapePuttyCommandParam(Password)), L" ");
    }

    AddToList(PuttyParams, Params2, L" ");

    // PuTTY is started in its binary directory to allow relative paths in private key,
    // when opening PuTTY's own stored session.
    ExecuteShellChecked(Program, PuttyParams, true);
  }
  else
  {
    throw Exception(FMTLOAD(FILE_NOT_FOUND, Program));
  }
}

bool FindTool(UnicodeString Name, UnicodeString & APath)
{
  UnicodeString AppPath = ::IncludeTrailingBackslash(::ExtractFilePath(GetConfiguration()->ModuleFileName()));
  APath = AppPath + Name;
  bool Result = true;
  if (!::FileExists(ApiPath(APath)))
  {
    APath = AppPath + L"PuTTY\\" + Name;
    if (!::FileExists(ApiPath(APath)))
    {
      APath = Name;
      if (!FindFile(APath))
      {
        Result = false;
      }
    }
  }
  return Result;
}

bool CopyCommandToClipboard(UnicodeString Command)
{
  bool Result = false; // UseAlternativeFunction() && IsKeyPressed(VK_CONTROL);
  if (Result)
  {
    TInstantOperationVisualizer Visualizer;
    CopyToClipboard(Command);
  }
  return Result;
}

static bool DoExecuteShell(const UnicodeString APath, const UnicodeString Params,
  bool ChangeWorkingDirectory, HANDLE * Handle)
{
  bool Result = CopyCommandToClipboard(FormatCommand(APath, Params));

  if (!Result)
  {
    UnicodeString Directory = ::ExtractFilePath(APath);

    TShellExecuteInfoW ExecuteInfo;
    ClearStruct(ExecuteInfo);
    ExecuteInfo.cbSize = sizeof(ExecuteInfo);
    ExecuteInfo.fMask =
      SEE_MASK_FLAG_NO_UI |
      FLAGMASK((Handle != nullptr), SEE_MASK_NOCLOSEPROCESS);
    ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(nullptr));
    ExecuteInfo.lpFile = ToWChar(APath);
    ExecuteInfo.lpParameters = ToWChar(Params);
    ExecuteInfo.lpDirectory = (ChangeWorkingDirectory ? Directory.c_str() : nullptr);
    ExecuteInfo.nShow = SW_SHOW;

    Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
    if (Result)
    {
      if (Handle != nullptr)
      {
        *Handle = ExecuteInfo.hProcess;
      }
    }
  }
  return Result;
}

void ExecuteShellChecked(const UnicodeString APath, const UnicodeString Params, bool ChangeWorkingDirectory)
{
  if (!DoExecuteShell(APath, Params, ChangeWorkingDirectory, nullptr))
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, APath));
  }
}

void ExecuteShellChecked(const UnicodeString Command)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  ExecuteShellChecked(Program, Params);
}

bool ExecuteShell(const UnicodeString Path, const UnicodeString Params,
  HANDLE & Handle)
{
  return DoExecuteShell(Path, Params, false, &Handle);
}

void ExecuteShellCheckedAndWait(HINSTANCE Handle, const UnicodeString Command,
  TProcessMessagesEvent ProcessMessages)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  HANDLE ProcessHandle;
  bool Result = DoExecuteShell(Program, Params, false, &ProcessHandle);
  if (!Result)
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, Program));
  }
  if (ProcessMessages != nullptr)
  {
    unsigned long WaitResult;
    do
    {
      // Same as in ExecuteProcessAndReadOutput
      WaitResult = WaitForSingleObject(ProcessHandle, 200);
      if (WaitResult == WAIT_FAILED)
      {
        throw Exception(LoadStr(DOCUMENT_WAIT_ERROR));
      }
      ProcessMessages();
    }
    while (WaitResult == WAIT_TIMEOUT);
  }
  else
  {
    WaitForSingleObject(ProcessHandle, INFINITE);
  }
}

bool SpecialFolderLocation(intptr_t PathID, UnicodeString & APath)
{
#if defined(_MSC_VER) && !defined(__clang__)
  LPITEMIDLIST Pidl;
  wchar_t Buf[MAX_PATH];
  if (::SHGetSpecialFolderLocation(nullptr, static_cast<int>(PathID), &Pidl) == NO_ERROR &&
    ::SHGetPathFromIDList(Pidl, Buf))
  {
    APath = UnicodeString(Buf);
    return true;
  }
#endif // if defined(_MSC_VER) && !defined(__clang__)
  return false;
}

UnicodeString ItemsFormatString(UnicodeString SingleItemFormat,
  UnicodeString MultiItemsFormat, intptr_t Count, UnicodeString FirstItem)
{
  UnicodeString Result;
  if (Count == 1)
  {
    Result = FORMAT(SingleItemFormat, FirstItem);
  }
  else
  {
    Result = FORMAT(MultiItemsFormat, Count);
  }
  return Result;
}

UnicodeString ItemsFormatString(UnicodeString SingleItemFormat,
  UnicodeString MultiItemsFormat, const TStrings * Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
    Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : UnicodeString()));
}

UnicodeString FileNameFormatString(UnicodeString SingleFileFormat,
  UnicodeString MultiFilesFormat, const TStrings * AFiles, bool Remote)
{
  DebugAssert(AFiles != nullptr);
  UnicodeString Item;
  if (AFiles && (AFiles->GetCount() > 0))
  {
    Item = Remote ? base::UnixExtractFileName(AFiles->GetString(0)) :
      base::ExtractFileName(AFiles->GetString(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    AFiles ? AFiles->GetCount() : 0, Item);
}

UnicodeString UniqTempDir(const UnicodeString BaseDir, const UnicodeString Identity,
  bool Mask)
{
  DebugAssert(!BaseDir.IsEmpty());
  UnicodeString TempDir;
  do
  {
    TempDir = ::IncludeTrailingBackslash(BaseDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
      TempDir += ::IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
    }
  }
  while (!Mask && ::DirectoryExists(ApiPath(TempDir)));

  return TempDir;
}

bool DeleteDirectory(const UnicodeString ADirName)
{
  TSearchRecChecked sr;
  bool retval = true;
  if (FindFirstUnchecked(ADirName + L"\\*", faAnyFile, sr) == 0) // VCL Function
  {
    if (FLAGSET(sr.Attr, faDirectory))
    {
      if (sr.Name != L"." && sr.Name != L"..")
        retval = ::DeleteDirectory(ADirName + L"\\" + sr.Name);
    }
    else
    {
      retval = ::RemoveFile(ApiPath(ADirName + L"\\" + sr.Name));
    }

    if (retval)
    {
      while (FindNextChecked(sr) == 0)
      { // VCL Function
        if (FLAGSET(sr.Attr, faDirectory))
        {
          if (sr.Name != L"." && sr.Name != L"..")
            retval = DeleteDirectory(ADirName + L"\\" + sr.Name);
        }
        else
        {
          retval = ::RemoveFile(ApiPath(ADirName + L"\\" + sr.Name));
        }

        if (!retval) break;
      }
    }
  }
  base::FindClose(sr);
  if (retval) retval = RemoveDir(ApiPath(ADirName)); // VCL function
  return retval;
}

#if 0

void AddSessionColorImage(
  TCustomImageList * ImageList, TColor Color, intptr_t MaskIndex)
{

  // This overly complex drawing is here to support color button on SiteAdvanced
  // dialog. There we use plain TImageList, instead of TPngImageList,
  // TButton does not work with transparent images
  // (not even TBitmap with Transparent = true)
  std::unique_ptr<TBitmap> MaskBitmap(new TBitmap());
  ImageList->GetBitmap(MaskIndex, MaskBitmap.get());

  std::unique_ptr<TPngImage> MaskImage(new TPngImage());
  MaskImage->Assign(MaskBitmap.get());

  std::unique_ptr<TPngImage> ColorImage(new TPngImage(COLOR_RGB, 16, ImageList->Width, ImageList->Height));

  TColor MaskTransparentColor = MaskImage->Pixels[0][0];
  TColor TransparentColor = MaskTransparentColor;
  // Expecting that the color to be replaced is in the centre of the image (HACK)
  TColor MaskColor = MaskImage->Pixels[ImageList->Width / 2][ImageList->Height / 2];

  for (intptr_t Y = 0; Y < ImageList->Height; Y++)
  {
    for (intptr_t X = 0; X < ImageList->Width; X++)
    {
      TColor SourceColor = MaskImage->Pixels[X][Y];
      TColor DestColor;
      // this branch is pointless as long as MaskTransparentColor and
      // TransparentColor are the same
      if (SourceColor == MaskTransparentColor)
      {
        DestColor = TransparentColor;
      }
      else if (SourceColor == MaskColor)
      {
        DestColor = Color;
      }
      else
      {
        DestColor = SourceColor;
      }
      ColorImage->Pixels[X][Y] = DestColor;
    }
  }

  std::unique_ptr<TBitmap> Bitmap(new TBitmap());
  Bitmap->SetSize(ImageList->Width, ImageList->Height);
  ColorImage->AssignTo(Bitmap.get());

  ImageList->AddMasked(Bitmap.get(), TransparentColor);
}

void SetSubmenu(TTBXCustomItem * Item)
{
  class TTBXPublicItem : public TTBXCustomItem
  {
  public:
    __property ItemStyle;
  };
  TTBXPublicItem * PublicItem = reinterpret_cast<TTBXPublicItem *>(Item);
  DebugAssert(PublicItem != nullptr);
  // See TTBItemViewer.IsPtInButtonPart (called from TTBItemViewer.MouseDown)
  PublicItem->ItemStyle = PublicItem->ItemStyle << tbisSubmenu;
}

bool IsEligibleForApplyingTabs(
  UnicodeString Line, intptr_t & TabPos, UnicodeString & Start, UnicodeString & Remaining)
{
  bool Result = false;
  TabPos = Line.Pos(L"\t");
  if (TabPos > 0)
  {
    Remaining = Line.SubString(TabPos + 1, Line.Length() - TabPos);
    // WORKAROUND
    // Some translations still use obsolete hack of consecutive tabs to aling the contents.
    // Delete these, so that the following check does not fail on this
    while (Remaining.SubString(1, 1) == L"\t")
    {
      Remaining.Delete(1, 1);
    }

    // We do not have, not support, mutiple tabs on a single line
    if (DebugAlwaysTrue(Remaining.Pos(L"\t") == 0))
    {
      Start = Line.SubString(1, TabPos - 1);
      // WORKAROUND
      // Previously we padded the string before tab with spaces,
      // to aling the contents across multiple lines
      Start = Start.TrimRight();
      // at least two normal spaces for separation
      Start += L"  ";
      Result = true;
    }
  }
  return Result;
}

static intptr_t CalculateWidthByLength(UnicodeString Text, void * /*Arg*/)
{
  return Text.Length();
}

void ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg)
{
  if (CalculateWidth == nullptr)
  {
    DebugAssert(CalculateWidthArg == nullptr);
    CalculateWidth = CalculateWidthByLength;
  }

  std::unique_ptr<TStringList> Lines(TextToStringList(Text));

  intptr_t MaxWidth = -1;
  for (intptr_t Index = 0; Index < Lines->Count; Index++)
  {
    UnicodeString Line = Lines->Strings[Index];
    intptr_t TabPos;
    UnicodeString Start;
    UnicodeString Remaining;
    if (IsEligibleForApplyingTabs(Line, TabPos, Start, Remaining))
    {
      intptr_t Width = CalculateWidth(Start, CalculateWidthArg);
      MaxWidth = Max(MaxWidth, Width);
    }
  }

  // Optimization and also to prevent potential regression for texts without tabs
  if (MaxWidth >= 0)
  {
    for (intptr_t Index = 0; Index < Lines->Count; Index++)
    {
      UnicodeString Line = Lines->Strings[Index];
      intptr_t TabPos;
      UnicodeString Start;
      UnicodeString Remaining;
      if (IsEligibleForApplyingTabs(Line, TabPos, Start, Remaining))
      {
        intptr_t Width;
        while ((Width = CalculateWidth(Start, CalculateWidthArg)) < MaxWidth)
        {
          intptr_t Wider = CalculateWidth(Start + Padding, CalculateWidthArg);
          // If padded string is wider than max width by more pixels
          // than non-padded string is shorter than max width
          if ((Wider > MaxWidth) && ((Wider - MaxWidth) > (MaxWidth - Width)))
          {
            break;
          }
          Start += Padding;
        }
        Lines->Strings[Index] = Start + Remaining;
      }
    }

    Text = Lines->Text;
    // remove trailing newline
    Text = Text.TrimRight();
  }
}

static void DoSelectScaledImageList(TImageList * ImageList)
{
  TImageList * MatchingList = nullptr;
  intptr_t MachingPixelsPerInch = 0;
  intptr_t PixelsPerInch = GetComponentPixelsPerInch(ImageList);

  for (intptr_t Index = 0; Index < ImageList->Owner->ComponentCount; Index++)
  {
    TImageList * OtherList = dynamic_cast<TImageList *>(ImageList->Owner->Components[Index]);

    if ((OtherList != nullptr) &&
        (OtherList != ImageList) &&
        ::StartsStr(ImageList->Name, OtherList->Name))
    {
      UnicodeString OtherListPixelsPerInchStr =
        OtherList->Name.SubString(
          ImageList->Name.Length() + 1, OtherList->Name.Length() - ImageList->Name.Length());
      intptr_t OtherListPixelsPerInch = StrToInt(OtherListPixelsPerInchStr);
      if ((OtherListPixelsPerInch <= PixelsPerInch) &&
          ((MatchingList == nullptr) ||
           (MachingPixelsPerInch < OtherListPixelsPerInch)))
      {
        MatchingList = OtherList;
        MachingPixelsPerInch = OtherListPixelsPerInch;
      }
    }
  }

  if (MatchingList != nullptr)
  {
    UnicodeString ImageListBackupName = ImageList->Name + IntToStr(USER_DEFAULT_SCREEN_DPI);

    if (ImageList->Owner->FindComponent(ImageListBackupName) == nullptr)
    {
      TImageList * ImageListBackup;
      TPngImageList * PngImageList = dynamic_cast<TPngImageList *>(ImageList);
      if (PngImageList != nullptr)
      {
        ImageListBackup = new TPngImageList(ImageList->Owner);
      }
      else
      {
        ImageListBackup = new TImageList(ImageList->Owner);
      }

      ImageListBackup->Name = ImageListBackupName;
      ImageList->Owner->InsertComponent(ImageListBackup);
      CopyImageList(ImageListBackup, ImageList);
    }

    CopyImageList(ImageList, MatchingList);
  }
}

static void ImageListRescale(TComponent * Sender, TObject * /*Token*/)
{
  TImageList * ImageList = DebugNotNull(dynamic_cast<TImageList *>(Sender));
  DoSelectScaledImageList(ImageList);
}

void SelectScaledImageList(TImageList * ImageList)
{
  DoSelectScaledImageList(ImageList);

  SetRescaleFunction(ImageList, ImageListRescale);
}

void CopyImageList(TImageList * TargetList, TImageList * SourceList)
{
  // Maybe this is not necessary, once the TPngImageList::Assign was fixed
  TPngImageList * PngTargetList = dynamic_cast<TPngImageList *>(TargetList);
  TPngImageList * PngSourceList = dynamic_cast<TPngImageList *>(SourceList);

  TargetList->Clear();
  TargetList->Height = SourceList->Height;
  TargetList->Width = SourceList->Width;

  if ((PngTargetList != nullptr) && (PngSourceList != nullptr))
  {
    // AddImages won't copy the names and we need them for
    // LoadDialogImage and TFrameAnimation
    PngTargetList->PngImages->Assign(PngSourceList->PngImages);
  }
  else
  {
    TargetList->AddImages(SourceList);
  }
}

static bool DoLoadDialogImage(TImage * Image, UnicodeString ImageName)
{
  bool Result = false;
  if (GlyphsModule != nullptr)
  {
    TPngImageList * DialogImages = GetDialogImages(Image);

    intptr_t Index;
    for (Index = 0; Index < DialogImages->PngImages->Count; Index++)
    {
      TPngImageCollectionItem * PngItem = DialogImages->PngImages->Items[Index];
      if (SameText(PngItem->Name, ImageName))
      {
        Image->Picture->Assign(PngItem->PngImage);
        break;
      }
    }

    DebugAssert(Index < DialogImages->PngImages->Count);
    Result = true;
  }
  // When showing an exception from wWinMain, the images are released already.
  // Non-existence of the glyphs module is just a good indication of that.
  // We expect errors only.
  else if (ImageName == L"Error")
  {
    Image->Picture->Icon->Handle = LoadIcon(0, IDI_HAND);
  }
  // For showing an information about trace files
  else if (DebugAlwaysTrue(ImageName == L"Information"))
  {
    Image->Picture->Icon->Handle = LoadIcon(0, IDI_APPLICATION);
  }
  return Result;
}

class TDialogImageName : public TObject
{
public:
  UnicodeString ImageName;
};

static void DialogImageRescale(TComponent * Sender, TObject * Token)
{
  TImage * Image = DebugNotNull(dynamic_cast<TImage *>(Sender));
  TDialogImageName * DialogImageName = DebugNotNull(dynamic_cast<TDialogImageName *>(Token));
  DoLoadDialogImage(Image, DialogImageName->ImageName);
}

void LoadDialogImage(TImage * Image, UnicodeString ImageName)
{
  if (DoLoadDialogImage(Image, ImageName))
  {
    TDialogImageName * DialogImageName = new TDialogImageName();
    DialogImageName->ImageName = ImageName;
    SetRescaleFunction(Image, DialogImageRescale, DialogImageName, true);
  }
}

intptr_t DialogImageSize(TForm * Form)
{
  return ScaleByPixelsPerInch(32, Form);
}

void HideComponentsPanel(TForm * Form)
{
  TComponent * Component = DebugNotNull(Form->FindComponent(L"ComponentsPanel"));
  TPanel * Panel = DebugNotNull(dynamic_cast<TPanel *>(Component));
  DebugAssert(Panel->Align == alBottom);
  intptr_t Offset = Panel->Height;
  Panel->Visible = false;
  Panel->Height = 0;
  Form->Height -= Offset;

  for (intptr_t Index = 0; Index < Form->ControlCount; Index++)
  {
    TControl * Control = Form->Controls[Index];

    // Shift back bottom-anchored controls
    // (needed for toolbar panel on Progress window and buttons on Preferences dialog),
    if ((Control->Align == alNone) &&
        Control->Anchors.Contains(akBottom) &&
        !Control->Anchors.Contains(akTop))
    {
      Control->Top += Offset;
    }

    // Resize back all-anchored controls
    // (needed for main panel on Preferences dialog),
    if (Control->Anchors.Contains(akBottom) &&
        Control->Anchors.Contains(akTop))
    {
      Control->Height += Offset;
    }
  }
}

class TBrowserViewer : public TWebBrowserEx
{
public:
  virtual TBrowserViewer(TComponent* AOwner);

  void AddLinkHandler(
    UnicodeString Url, TNotifyEvent Handler);
  void NavigateToUrl(UnicodeString Url);

  TControl * LoadingPanel;

protected:
  DYNAMIC void DoContextPopup(const TPointptr_t & MousePos, bool & Handled);
  void DocumentComplete(
    TObject * Sender, const _di_IDispatch Disp, const OleVariant & URL);
  void BeforeNavigate2(
    TObject * Sender, const _di_IDispatch Disp, const OleVariant & URL,
    const OleVariant & Flags, const OleVariant & TargetFrameName,
    const OleVariant & PostData, const OleVariant & Headers, WordBool & Cancel);

  bool FComplete;
  std::map<UnicodeString, TNotifyEvent> FHandlers;
};

TBrowserViewer::TBrowserViewer(TComponent* AOwner) :
  TWebBrowserEx(AOwner)
{
  FComplete = false;

  OnDocumentComplete = DocumentComplete;
  OnBeforeNavigate2 = BeforeNavigate2;
  LoadingPanel = nullptr;
}

void TBrowserViewer::AddLinkHandler(
  UnicodeString Url, TNotifyEvent Handler)
{
  FHandlers.insert(std::make_pair(Url, Handler));
}

void TBrowserViewer::DoContextPopup(const TPointptr_t & MousePos, bool & Handled)
{
  // suppress built-in context menu
  Handled = true;
  TWebBrowserEx::DoContextPopup(MousePos, Handled);
}

void TBrowserViewer::DocumentComplete(
    TObject * /*Sender*/, const _di_IDispatch /*Disp*/, const OleVariant & /*URL*/)
{
  SetBrowserDesignModeOff(this);

  FComplete = true;

  if (LoadingPanel != nullptr)
  {
    LoadingPanel->Visible = false;
  }
}

void TBrowserViewer::BeforeNavigate2(
  TObject * /*Sender*/, const _di_IDispatch /*Disp*/, const OleVariant & AURL,
  const OleVariant & /*Flags*/, const OleVariant & /*TargetFrameName*/,
  const OleVariant & /*PostData*/, const OleVariant & /*Headers*/, WordBool & Cancel)
{
  // If OnDocumentComplete was not called yet, is has to be our initial message URL,
  // opened using TWebBrowserEx::Navigate(), allow it.
  // Otherwise it's user navigating, block that and open link
  // in an external browser, possibly adding campaign parameters on the way.
  if (FComplete)
  {
    Cancel = 1;

    UnicodeString URL = AURL;
    if (FHandlers.count(URL) > 0)
    {
      FHandlers[URL](this);
    }
    else
    {
      OpenBrowser(URL);
    }
  }
}

void TBrowserViewer::NavigateToUrl(UnicodeString Url)
{
  FComplete = false;
  Navigate(Url.c_str());
}

TPanel * CreateLabelPanel(TPanel * Parent, UnicodeString Label)
{
  TPanel * Result = CreateBlankPanel(Parent);
  Result->Parent = Parent;
  Result->Align = alClient;
  Result->Caption = Label;
  return Result;
}

TWebBrowserEx * CreateBrowserViewer(TPanel * Parent, UnicodeString LoadingLabel)
{
  TBrowserViewer * Result = new TBrowserViewer(Parent);
  // TWebBrowserEx has its own unrelated Name and Parent properties.
  // The name is used in DownloadUpdate().
  static_cast<TWinControl *>(Result)->Name = L"BrowserViewer";
  static_cast<TWinControl *>(Result)->Parent = Parent;
  Result->Align = alClient;
  Result->ControlBorder = cbNone;

  Result->LoadingPanel = CreateLabelPanel(Parent, LoadingLabel);

  return Result;
}

void SetBrowserDesignModeOff(TWebBrowserEx * WebBrowser)
{
  if (DebugAlwaysTrue(WebBrowser->Document2 != nullptr))
  {
    WebBrowser->Document2->designMode = L"Off";
  }
}

void AddBrowserLinkHandler(TWebBrowserEx * WebBrowser,
  UnicodeString Url, TNotifyEvent Handler)
{
  TBrowserViewer * BrowserViewer = dynamic_cast<TBrowserViewer *>(WebBrowser);
  if (DebugAlwaysTrue(BrowserViewer != nullptr))
  {
    BrowserViewer->AddLinkHandler(Url, Handler);
  }
}

void NavigateBrowserToUrl(TWebBrowserEx * WebBrowser, UnicodeString Url)
{
  TBrowserViewer * BrowserViewer = dynamic_cast<TBrowserViewer *>(WebBrowser);
  if (DebugAlwaysTrue(BrowserViewer != nullptr))
  {
    BrowserViewer->NavigateToUrl(Url);
  }
}

TComponent * FindComponentRecursively(TComponent * Root, UnicodeString Name)
{
  for (intptr_t Index = 0; Index < Root->ComponentCount; Index++)
  {
    TComponent * Component = Root->Components[Index];
    if (CompareText(Component->Name, Name) == 0)
    {
      return Component;
    }

    Component = FindComponentRecursively(Component, Name);
    if (Component != nullptr)
    {
      return Component;
    }
  }
  return nullptr;
}

#endif // #if 0

TLocalCustomCommand::TLocalCustomCommand()
{
}

TLocalCustomCommand::TLocalCustomCommand(
  const TCustomCommandData & Data, UnicodeString RemotePath, UnicodeString LocalPath) :
  TFileCustomCommand(Data, RemotePath)
{
  FLocalPath = LocalPath;
}

TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  UnicodeString RemotePath, UnicodeString LocalPath, UnicodeString FileName,
  UnicodeString LocalFileName, UnicodeString FileList) :
  TFileCustomCommand(Data, RemotePath, FileName, FileList)
{
  FLocalPath = LocalPath;
  FLocalFileName = LocalFileName;
}

intptr_t TLocalCustomCommand::PatternLen(UnicodeString Command, intptr_t Index) const
{
  intptr_t Len;
  if ((Index < Command.Length()) && (Command[Index + 1] == L'\\'))
  {
    Len = 2;
  }
  else if ((Index < Command.Length()) && (Command[Index + 1] == L'^'))
  {
    Len = 3;
  }
  else
  {
    Len = TFileCustomCommand::PatternLen(Command, Index);
  }
  return Len;
}

bool TLocalCustomCommand::PatternReplacement(
  intptr_t Index, UnicodeString Pattern, UnicodeString & Replacement, bool & Delimit) const
{
  bool Result;
  if (Pattern == L"!\\")
  {
    // When used as "!\" in an argument to PowerShell, the trailing \ would escpae the ",
    // so we exclude it
    Replacement = ::ExcludeTrailingBackslash(FLocalPath);
    Result = true;
  }
  else if (Pattern == L"!^!")
  {
    Replacement = FLocalFileName;
    Result = true;
  }
  else
  {
    Result = TFileCustomCommand::PatternReplacement(Index, Pattern, Replacement, Delimit);
  }
  return Result;
}

void TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}

bool TLocalCustomCommand::HasLocalFileName(UnicodeString Command) const
{
  return FindPattern(Command, L'^');
}

bool TLocalCustomCommand::IsFileCommand(UnicodeString Command) const
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}

#if 0

typedef std::set<TDataModule *> TImagesModules;
static TImagesModules ImagesModules;
static std::map<int, TPngImageList *> AnimationsImages;
static std::map<int, TImageList *> ButtonImages;
static std::map<int, TPngImageList *> DialogImages;

intptr_t NormalizePixelsPerInch(intptr_t PixelsPerInch)
{
  if (PixelsPerInch >= 192)
  {
    PixelsPerInch = 192;
  }
  else if (PixelsPerInch >= 144)
  {
    PixelsPerInch = 144;
  }
  else if (PixelsPerInch >= 120)
  {
    PixelsPerInch = 120;
  }
  else
  {
    PixelsPerInch = 96;
  }
  return PixelsPerInch;
}

static intptr_t NeedImagesModule(TControl * Control)
{
  intptr_t PixelsPerInch = NormalizePixelsPerInch(GetControlPixelsPerInch(Control));

  if (AnimationsImages.find(PixelsPerInch) == AnimationsImages.end())
  {
    TDataModule * ImagesModule;
    HANDLE ResourceModule = GUIConfiguration->ChangeToDefaultResourceModule();
    try
    {
      if (PixelsPerInch == 192)
      {
        ImagesModule = new TAnimations192Module(Application);
      }
      else if (PixelsPerInch == 144)
      {
        ImagesModule = new TAnimations144Module(Application);
      }
      else if (PixelsPerInch == 120)
      {
        ImagesModule = new TAnimations120Module(Application);
      }
      else
      {
        DebugAssert(PixelsPerInch == 96);
        ImagesModule = new TAnimations96Module(Application);
      }

      ImagesModules.insert(ImagesModule);

      TPngImageList * AAnimationImages =
        DebugNotNull(dynamic_cast<TPngImageList *>(ImagesModule->FindComponent(L"AnimationImages")));
      AnimationsImages.insert(std::make_pair(PixelsPerInch, AAnimationImages));

      TImageList * AButtonImages =
        DebugNotNull(dynamic_cast<TImageList *>(ImagesModule->FindComponent(L"ButtonImages")));
      ButtonImages.insert(std::make_pair(PixelsPerInch, AButtonImages));

      TPngImageList * ADialogImages =
        DebugNotNull(dynamic_cast<TPngImageList *>(ImagesModule->FindComponent(L"DialogImages")));
      DialogImages.insert(std::make_pair(PixelsPerInch, ADialogImages));
    }
    __finally
    {
      GUIConfiguration->ChangeResourceModule(ResourceModule);
    }
  }

  return PixelsPerInch;
}

TPngImageList * GetAnimationsImages(TControl * Control)
{
  intptr_t PixelsPerInch = NeedImagesModule(Control);
  return DebugNotNull(AnimationsImages[PixelsPerInch]);
}

TImageList * GetButtonImages(TControl * Control)
{
  intptr_t PixelsPerInch = NeedImagesModule(Control);
  return DebugNotNull(ButtonImages[PixelsPerInch]);
}

TPngImageList * GetDialogImages(TControl * Control)
{
  intptr_t PixelsPerInch = NeedImagesModule(Control);
  return DebugNotNull(DialogImages[PixelsPerInch]);
}

void ReleaseImagesModules()
{

  TImagesModules::iterator i = ImagesModules.begin();
  while (i != ImagesModules.end())
  {
    delete (*i);
    i++;
  }
  ImagesModules.clear();
}

TFrameAnimation::TFrameAnimation()
{
  FFirstFrame = -1;
}

void TFrameAnimation::Init(TPaintBox * PaintBox, UnicodeString Name)
{
  FName = Name;
  FPaintBox = PaintBox;

  FPaintBox->ControlStyle = FPaintBox->ControlStyle << csOpaque;
  DebugAssert((FPaintBox->OnPaintptr_t == nullptr) || (FPaintBox->OnPaintptr_t == PaintBoxPaint));
  FPaintBox->OnPaintptr_t = PaintBoxPaint;
  SetRescaleFunction(FPaintBox, PaintBoxRescale, reinterpret_cast<TObject *>(this));

  DoInit();
}

void TFrameAnimation::DoInit()
{
  FImageList = GetAnimationsImages(FPaintBox);
  FFirstFrame = -1;
  FFirstLoopFrame = -1;
  FPaintBox->Width = FImageList->Width;
  FPaintBox->Height = FImageList->Height;

  if (!FName.IsEmpty())
  {
    intptr_t Frame = 0;
    while (Frame < FImageList->PngImages->Count)
    {
      UnicodeString FrameData = FImageList->PngImages->Items[Frame]->Name;
      UnicodeString FrameName;
      FrameName = CutToChar(FrameData, L'_', false);

      if (SameText(FName, FrameName))
      {
        intptr_t FrameIndex = StrToInt(CutToChar(FrameData, L'_', false));
        if (FFirstFrame < 0)
        {
          FFirstFrame = Frame;
        }
        if ((FFirstLoopFrame < 0) && (FrameIndex > 0))
        {
          FFirstLoopFrame = Frame;
        }
        FLastFrame = Frame;
      }
      else
      {
        if (FFirstFrame >= 0)
        {
          // optimization
          break;
        }
      }
      Frame++;
    }

    DebugAssert(FFirstFrame >= 0);
    DebugAssert(FFirstLoopFrame >= 0);
  }

  Stop();
}

void TFrameAnimation::PaintBoxRescale(TComponent * /*Sender*/, TObject * Token)
{
  TFrameAnimation * FrameAnimation = reinterpret_cast<TFrameAnimation *>(Token);
  FrameAnimation->Rescale();
}

void TFrameAnimation::Rescale()
{
  bool Started = (FTimer != nullptr) && FTimer->Enabled;
  DoInit();
  if (Started)
  {
    Start();
  }
}

void TFrameAnimation::Start()
{
  if (FFirstFrame >= 0)
  {
    FNextFrameTick = GetTickCount();
    CalculateNextFrameTick();

    if (FTimer == nullptr)
    {
      FTimer = new TTimer(GetParentForm(FPaintBox));
      FTimer->Interval = static_cast<int>(GUIUpdateInterval);
      FTimer->OnTimer = Timer;
    }
    else
    {
      // reset timer
      FTimer->Enabled = false;
      FTimer->Enabled = true;
    }
  }
}

void TFrameAnimation::Timer(TObject * /*Sender*/)
{
  Animate();
}

void TFrameAnimation::PaintBoxPaint(TObject * Sender)
{
  if (FFirstFrame >= 0)
  {
    // Double-buffered drawing to prevent flicker (as the images are transparent)
    DebugUsedParam(Sender);
    DebugAssert(FPaintBox == Sender);
    DebugAssert(FPaintBox->ControlStyle.Contains(csOpaque));
    std::unique_ptr<TBitmap> Bitmap(new TBitmap());
    Bitmap->SetSize(FPaintBox->Width, FPaintBox->Height);
    Bitmap->Canvas->Brush->Color = FPaintBox->Color;
    TRect Rect(0, 0, Bitmap->Width, Bitmap->Height);
    Bitmap->Canvas->FillRect(Rect);
    TGraphic * Graphic = GetCurrentImage()->PngImage;
    DebugAssert(Graphic->Width == FPaintBox->Width);
    DebugAssert(Graphic->Height == FPaintBox->Height);
    Bitmap->Canvas->Draw(0, 0, Graphic);
    FPaintBox->Canvas->Draw(0, 0, Bitmap.get());
  }
  FPainted = true;
}

void TFrameAnimation::Repaint()
{
  FPainted = false;
  // If the form is not showing yet, the Paint() is not even called
  FPaintBox->Repaint();
  if (!FPainted)
  {
    // Paintptr_t later, alternativelly we may keep trying Repaint() in Animate().
    // See also a hack in TAuthenticateForm::Log.
    FPaintBox->Invalidate();
  }
}

void TFrameAnimation::Stop()
{
  FNextFrameTick = std::numeric_limits<DWORD>::max();
  FCurrentFrame = FFirstFrame;
  Repaint();

  if (FTimer != nullptr)
  {
    FTimer->Enabled = false;
  }
}

void TFrameAnimation::Animate()
{
  if (FFirstFrame >= 0)
  {
    // UPGRADE: Use GetTickCount64() when we stop supporting Windows XP.
    DWORD TickCount = GetTickCount();

    // Keep in sync with an opposite condition at the end of the loop.
    // We may skip some frames if we got stalled for a while
    while (TickCount >= FNextFrameTick)
    {
      if (FCurrentFrame >= FLastFrame)
      {
        FCurrentFrame = FFirstLoopFrame;
      }
      else
      {
        FCurrentFrame++;
      }

      CalculateNextFrameTick();

      Repaint();
    }
  }
}

TPngImageCollectionItem * TFrameAnimation::GetCurrentImage()
{
  return FImageList->PngImages->Items[FCurrentFrame];
}

void TFrameAnimation::CalculateNextFrameTick()
{
  TPngImageCollectionItem * ImageItem = GetCurrentImage();
  UnicodeString Duration = ImageItem->Name;
  CutToChar(Duration, L'_', false);
  // skip index (is not really used)
  CutToChar(Duration, L'_', false);
  // This should overflow, when tick count wraps.
  FNextFrameTick += StrToInt(Duration) * 10;
}


// Hints use:
// - Cleanup list tooltip (multi line)
// - Combo edit button
// - Transfer settings label (multi line, follows label size and font)
// - HintLabel (hintptr_t and persistent hint, multipline)
// - status bar hints

TScreenTipHintWindow::TScreenTipHintWindow(TComponent * Owner) :
  THintWindow(Owner)
{
  FParentPainting = false;
}

intptr_t TScreenTipHintWindow::GetTextFlags(TControl * Control)
{
  return DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | Control->DrawTextBiDiModeFlagsReadingOnly();
}

bool TScreenTipHintWindow::UseBoldShortHint(TControl * HintControl)
{
  return
    (dynamic_cast<TTBCustomDockableWindow *>(HintControl) != nullptr) ||
    (dynamic_cast<TTBPopupWindow *>(HintControl) != nullptr);
}

bool TScreenTipHintWindow::IsPathLabel(TControl * HintControl)
{
  return (dynamic_cast<TPathLabel *>(HintControl) != nullptr);
}

bool TScreenTipHintWindow::IsHintPopup(TControl * HintControl, UnicodeString Hint)
{
  TLabel * HintLabel = dynamic_cast<TLabel *>(HintControl);
  return (HintLabel != nullptr) && HasLabelHintPopup(HintLabel, Hint);
}

intptr_t TScreenTipHintWindow::GetMargin(TControl * HintControl, UnicodeString Hint)
{
  intptr_t Result;

  if (IsHintPopup(HintControl, Hint) || IsPathLabel(HintControl))
  {
    Result = 3;
  }
  else
  {
    Result = 6;
  }

  Result = ScaleByTextHeight(HintControl, Result);

  return Result;
}

TFont * TScreenTipHintWindow::GetFont(TControl * HintControl, UnicodeString Hint)
{
  TFont * Result;
  if (IsHintPopup(HintControl, Hint) || IsPathLabel(HintControl))
  {
    Result = reinterpret_cast<TLabel *>(dynamic_cast<TCustomLabel *>(HintControl))->Font;
  }
  else
  {
    FScaledHintFont.reset(new TFont());
    FScaledHintFont->Assign(Screen->HintFont);
    FScaledHintFont->Size = ScaleByPixelsPerInchFromSystem(FScaledHintFont->Size, HintControl);
    Result = FScaledHintFont.get();
  }
  return Result;
}

void TScreenTipHintWindow::CalcHintTextRect(TControl * Control, TCanvas * Canvas, TRect & Rect, UnicodeString Hint)
{
  const intptr_t Flags = DT_CALCRECT | GetTextFlags(Control);
  DrawText(Canvas->Handle, Hint.c_str(), -1, &Rect, Flags);
}

TRect TScreenTipHintWindow::CalcHintRect(intptr_t MaxWidth, const UnicodeString AHint, void * AData)
{
  TControl * HintControl = GetHintControl(AData);
  intptr_t Margin = GetMargin(HintControl, AHint);
  const UnicodeString ShortHint = GetShortHint(AHint);
  const UnicodeString LongHint = GetLongHintIfAny(AHint);

  Canvas->Font->Assign(GetFont(HintControl, AHint));

  const intptr_t ScreenTipTextOnlyWidth = ScaleByTextHeight(HintControl, cScreenTipTextOnlyWidth);

  if (!LongHint.IsEmpty())
  {
    // double-margin on the right
    MaxWidth = ScreenTipTextOnlyWidth - (3 * Margin);
  }

  // Multi line short hints can be twice as wide, to not break the individual lines unless really necessary.
  // (login site tree, clean up dialog list, preferences custom command list, persistent hint, etc).
  // And they also can be twice as wide, to not break the individual lines unless really necessary.
  if (ShortHint.Pos(L"\n") > 0)
  {
    MaxWidth *= 2;
  }

  bool HintPopup = IsHintPopup(HintControl, AHint);
  if (HintPopup)
  {
    MaxWidth = HintControl->Width;
  }

  if (UseBoldShortHint(HintControl))
  {
    Canvas->Font->Style = TFontStyles() << fsBold;
  }
  TRect ShortRect(0, 0, MaxWidth, 0);
  CalcHintTextRect(this, Canvas, ShortRect, ShortHint);
  Canvas->Font->Style = TFontStyles();

  TRect Result;

  if (LongHint.IsEmpty())
  {
    Result = ShortRect;

    if (HintPopup)
    {
      Result.Right = MaxWidth + 2 * Margin;
    }
    else
    {
      Result.Right += 3 * Margin;
    }

    Result.Bottom += 2 * Margin;
  }
  else
  {
    const intptr_t LongIndentation = Margin * 3 / 2;
    TRect LongRect(0, 0, MaxWidth - LongIndentation, 0);
    CalcHintTextRect(this, Canvas, LongRect, LongHint);

    Result.Right = ScreenTipTextOnlyWidth;
    Result.Bottom = Margin + ShortRect.Height() + (Margin / 3 * 5) + LongRect.Height() + Margin;
  }

  // VCLCOPY: To counter the increase in THintWindow::ActivateHintData
  Result.Bottom -= 4;

  return Result;
}

void TScreenTipHintWindow::ActivateHintData(const TRect & ARect, const UnicodeString AHint, void * AData)
{
  FShortHint = GetShortHint(AHint);
  FLongHint = GetLongHintIfAny(AHint);
  FHintControl = GetHintControl(AData);
  FMargin = GetMargin(FHintControl, AHint);
  FHintPopup = IsHintPopup(FHintControl, AHint);

  Canvas->Font->Assign(GetFont(FHintControl, AHint));

  TRect Rect = ARect;

  if (FHintPopup)
  {
    Rect.SetLocation(FHintControl->ClientToScreen(TPoint(-FMargin, -FMargin)));
  }
  if (IsPathLabel(FHintControl))
  {
    Rect.Offset(-FMargin, -FMargin);
  }

  THintWindow::ActivateHintData(Rect, FShortHint, AData);
}

TControl * TScreenTipHintWindow::GetHintControl(void * Data)
{
  return reinterpret_cast<TControl *>(DebugNotNull(Data));
}

UnicodeString TScreenTipHintWindow::GetLongHintIfAny(UnicodeString AHint)
{
  UnicodeString Result;
  intptr_t P = Pos(L"|", AHint);
  if (P > 0)
  {
    Result = GetLongHint(AHint);
  }
  return Result;
}

void TScreenTipHintWindow::Dispatch(void * AMessage)
{
  TMessage * Message = static_cast<TMessage*>(AMessage);
  switch (Message->Msg)
  {
    case WM_GETTEXTLENGTH:
      if (FParentPainting)
      {
        // make THintWindow::Paint() not paint the Caption
        Message->Result = 0;
      }
      else
      {
        THintWindow::Dispatch(AMessage);
      }
      break;

    default:
      THintWindow::Dispatch(AMessage);
      break;
  }
}

void TScreenTipHintWindow::Paint()
{
  // paint frame/background
  {
    TAutoFlag ParentPaintingFlag(FParentPainting);
    THintWindow::Paint();
  }

  const intptr_t Flags = GetTextFlags(this);
  const intptr_t Margin = FMargin - 1; // 1 = border

  TRect Rect = ClientRect;
  Rect.Inflate(-Margin, -Margin);
  if (!FHintPopup)
  {
    Rect.Right -= FMargin;
  }
  if (UseBoldShortHint(FHintControl))
  {
    Canvas->Font->Style = TFontStyles() << fsBold;
  }
  DrawText(Canvas->Handle, FShortHint.c_str(), -1, &Rect, Flags);
  TRect ShortRect = Rect;
  DrawText(Canvas->Handle, FShortHint.c_str(), -1, &ShortRect, DT_CALCRECT | Flags);
  Canvas->Font->Style = TFontStyles();

  if (!FLongHint.IsEmpty())
  {
    Rect.Left += FMargin * 3 / 2;
    Rect.Top += ShortRect.Height() + (FMargin / 3 * 5);
    DrawText(Canvas->Handle, FLongHint.c_str(), -1, &Rect, Flags);
  }
}

#endif // #if 0
