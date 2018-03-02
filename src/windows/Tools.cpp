#include <vcl.h>
#pragma hdrstop

#include <winhttp.h>
#include <Common.h>
#include <TextsCore.h>
#include <Exceptions.h>
//#include <FileMasks.h>
//#include <CoreMain.h>
#include <RemoteFiles.h>
#include <Interface.h>
#include <LibraryLoader.hpp>

#include "WinInterface.h"
//#include "GUITools.h"
#include "PuttyTools.h"
#include "Tools.h"
#include "WinConfiguration.h"
#if 0
#include <WinHelpViewer.hpp>
#include <PasTools.hpp>
#include <System.Win.ComObj.hpp>
#include <StrUtils.hpp>
#include <WinConfiguration.h>
#include <ProgParams.h>

// WORKAROUND
// VCL includes wininet.h (even with NO_WIN32_LEAN_AND_MEAN)
// and it cannot be combined with winhttp.h as of current Windows SDK.
// This is hack to allow that.
// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/8f468d9f-3f15-452c-803d-fc63ab3f684e/cannot-use-both-winineth-and-winhttph
#undef BOOLAPI
#undef SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
#undef SECURITY_FLAG_IGNORE_CERT_CN_INVALID

#define URL_COMPONENTS URL_COMPONENTS_ANOTHER
#define URL_COMPONENTSA URL_COMPONENTSA_ANOTHER
#define URL_COMPONENTSW URL_COMPONENTSW_ANOTHER

#define LPURL_COMPONENTS LPURL_COMPONENTS_ANOTHER
#define LPURL_COMPONENTSA LPURL_COMPONENTS_ANOTHER
#define LPURL_COMPONENTSW LPURL_COMPONENTS_ANOTHER

#define INTERNET_SCHEME INTERNET_SCHEME_ANOTHER
#define LPINTERNET_SCHEME LPINTERNET_SCHEME_ANOTHER

#define HTTP_VERSION_INFO HTTP_VERSION_INFO_ANOTHER
#define LPHTTP_VERSION_INFO LPHTTP_VERSION_INFO_ANOTHER

#include <winhttp.h>

#undef URL_COMPONENTS
#undef URL_COMPONENTSA
#undef URL_COMPONENTSW

#undef LPURL_COMPONENTS
#undef LPURL_COMPONENTSA
#undef LPURL_COMPONENTSW

#undef INTERNET_SCHEME
#undef LPINTERNET_SCHEME

#undef HTTP_VERSION_INFO
#undef LPHTTP_VERSION_INFO

#pragma package(smart_init)

TFontStyles IntToFontStyles(intptr_t Value)
{
  TFontStyles Result;
  for (int i = fsBold; i <= fsStrikeOut; i++)
  {
    if (value & 1)
    {
      Result << (TFontStyle)i;
    }
    value >>= 1;
  }
  return Result;
}

int FontStylesToInt(const TFontStyles value)
{
  int Result = 0;
  for (int i = fsStrikeOut; i >= fsBold; i--)
  {
    Result <<= 1;
    if (value.Contains((TFontStyle)i))
    {
      Result |= 1;
    }
  }
  return Result;
}

bool SameFont(TFont *Font1, TFont *Font2)
{
  // keep in sync with TFontConfiguration::operator!=
  return
    SameText(Font1->Name, Font2->Name) && (Font1->Height == Font2->Height) &&
    (Font1->Charset == Font2->Charset) && (Font1->Style == Font2->Style);
}

TColor GetWindowTextColor(TColor Color)
{
  return (Color == TColor(0)) ? clWindowText : Color;
}

TColor GetWindowColor(TColor Color)
{
  return (Color == TColor(0)) ? clWindow : Color;
}

TColor GetNonZeroColor(TColor Color)
{
  // 0,0,0 is "default color"
  if (Color == TColor(0))
  {
    // use near-black instead
    Color = TColor(RGB(1, 1, 1));
  }
  return Color;
}

void CenterFormOn(TForm *Form, TControl *CenterOn)
{
  TPoint ScreenPoint = CenterOn->ClientToScreen(TPoint(0, 0));
  Form->Left = ScreenPoint.x + (CenterOn->Width / 2) - (Form->Width / 2);
  Form->Top = ScreenPoint.y + (CenterOn->Height / 2) - (Form->Height / 2);
}

UnicodeString GetListViewStr(TListView *ListView)
{
  UnicodeString Result;
  for (int Index = 0; Index < ListView->Columns->Count; Index++)
  {
    AddToList(Result, IntToStr(ListView->Column[Index]->Width), L",");
  }
  // WORKAROUND
  // Adding an additional comma after the list,
  // to ensure that old versions that did not expect the pixels-per-inch part,
  // stop at the comma, otherwise they try to parse the
  // "last-column-width;pixels-per-inch" as integer and throw.
  // For the other instance of this hack, see TCustomListViewColProperties.GetParamsStr
  Result += L",;" + SavePixelsPerInch(ListView);
  return Result;
}

void LoadListViewStr(TListView *ListView, const UnicodeString ALayoutStr)
{
  UnicodeString LayoutStr = CutToChar(ALayoutStr, L';', true);
  int PixelsPerInch = LoadPixelsPerInch(CutToChar(ALayoutStr, L';', true), ListView);
  int Index = 0;
  while (!LayoutStr.IsEmpty() && (Index < ListView->Columns->Count))
  {
    int Width;
    if (TryStrToInt64(CutToChar(LayoutStr, L',', true), Width))
    {
      ListView->Column[Index]->Width = LoadDimension(Width, PixelsPerInch, ListView);
    }
    Index++;
  }
}

void RestoreForm(const UnicodeString Data, TForm *Form, bool PositionOnly)
{
  DebugAssert(Form);
  if (!Data.IsEmpty())
  {
    Forms::TMonitor *Monitor = FormMonitor(Form);

    UnicodeString LeftStr = CutToChar(Data, L';', true);
    UnicodeString TopStr = CutToChar(Data, L';', true);
    UnicodeString RightStr = CutToChar(Data, L';', true);
    UnicodeString BottomStr = CutToChar(Data, L';', true);
    TWindowState State = (TWindowState)StrToIntDef(CutToChar(Data, L';', true), (int)wsNormal);
    int PixelsPerInch = LoadPixelsPerInch(CutToChar(Data, L';', true), Form);

    TRect Bounds = Form->BoundsRect;
    int Left = StrToDimensionDef(LeftStr, PixelsPerInch, Form, Bounds.Left);
    int Top = StrToDimensionDef(TopStr, PixelsPerInch, Form, Bounds.Top);
    bool DefaultPos = (Left == -1) && (Top == -1);
    if (!DefaultPos)
    {
      Bounds.Left = Left;
      Bounds.Top = Top;
    }
    else
    {
      Bounds.Left = 0;
      Bounds.Top = 0;
    }
    Bounds.Right = StrToDimensionDef(RightStr, PixelsPerInch, Form, Bounds.Right);
    Bounds.Bottom = StrToDimensionDef(BottomStr, PixelsPerInch, Form, Bounds.Bottom);
    Form->WindowState = State;
    if (State == wsNormal)
    {
      // move to the target monitor
      OffsetRect(Bounds, Monitor->Left, Monitor->Top);

      // reduce window size to that of monitor size
      // (this does not cut window into monitor!)
      if (Bounds.Width() > Monitor->WorkareaRect.Width())
      {
        Bounds.Right -= (Bounds.Width() - Monitor->WorkareaRect.Width());
      }
      if (Bounds.Height() > Monitor->WorkareaRect.Height())
      {
        Bounds.Bottom -= (Bounds.Height() - Monitor->WorkareaRect.Height());
      }

      if (DefaultPos ||
        ((Bounds.Left < Monitor->Left) ||
          (Bounds.Left > Monitor->Left + Monitor->WorkareaRect.Width() - 20) ||
          (Bounds.Top < Monitor->Top) ||
          (Bounds.Top > Monitor->Top + Monitor->WorkareaRect.Height() - 20)))
      {
        bool ExplicitPlacing = !Monitor->Primary;
        if (!ExplicitPlacing)
        {
          TPosition Position;
          if ((Application->MainForm == nullptr) || (Application->MainForm == Form))
          {
            Position = poDefaultPosOnly;
          }
          else
          {
            Position = poOwnerFormCenter;
          }

          // If handle is allocated already, changing Position reallocates it, what brings lot of nasty side effects.
          if (Form->HandleAllocated() && (Form->Position != Position))
          {
            ExplicitPlacing = true;
          }
          else
          {
            Form->Width = Bounds.Width();
            Form->Height = Bounds.Height();
          }
        }

        if (ExplicitPlacing)
        {
          // when positioning on non-primary monitor, we need
          // to handle that ourselves, so place window to center
          if (!PositionOnly)
          {
            Form->SetBounds(Monitor->Left + ((Monitor->Width - Bounds.Width()) / 2),
              Monitor->Top + ((Monitor->Height - Bounds.Height()) / 2),
              Bounds.Width(), Bounds.Height());
          }
          Form->Position = poDesigned;
        }
      }
      else
      {
        Form->Position = poDesigned;
        if (!PositionOnly)
        {
          Form->BoundsRect = Bounds;
        }
      }
    }
    else if (State == wsMaximized)
    {
      Form->Position = poDesigned;

      if (!PositionOnly)
      {
        Bounds = Form->BoundsRect;
        OffsetRect(Bounds, Monitor->Left, Monitor->Top);
        Form->BoundsRect = Bounds;
      }
    }
  }
  else if (Form->Position == poDesigned)
  {
    Form->Position = poDefaultPosOnly;
  }
}

UnicodeString StoreForm(TCustomForm *Form)
{
  DebugAssert(Form);
  TRect Bounds = Form->BoundsRect;
  OffsetRect(Bounds, -Form->Monitor->Left, -Form->Monitor->Top);
  UnicodeString Result =
    FORMAT(L"%d;%d;%d;%d;%d;%s", (SaveDimension(Bounds.Left), SaveDimension(Bounds.Top),
      SaveDimension(Bounds.Right), SaveDimension(Bounds.Bottom),
      // we do not want WinSCP to start minimized next time (we cannot handle that anyway).
      // note that WindowState is wsNormal when window in minimized for some reason.
      // actually it is wsMinimized only when minimized by MSVDM
      (int)(Form->WindowState == wsMinimized ? wsNormal : Form->WindowState),
      SavePixelsPerInch(Form)));
  return Result;
}

void RestoreFormSize(const UnicodeString Data, TForm *Form)
{
  // This has to be called only after DoFormWindowProc(CM_SHOWINGCHANGED).
  // See comment in ResizeForm.
  UnicodeString WidthStr = CutToChar(Data, L',', true);
  UnicodeString HeightStr = CutToChar(Data, L',', true);
  int PixelsPerInch = LoadPixelsPerInch(CutToChar(Data, L',', true), Form);

  int Width = StrToDimensionDef(WidthStr, PixelsPerInch, Form, Form->Width);
  int Height = StrToDimensionDef(HeightStr, PixelsPerInch, Form, Form->Height);
  ResizeForm(Form, Width, Height);
}

UnicodeString StoreFormSize(TForm *Form)
{
  return FORMAT(L"%d,%d,%s", (Form->Width, Form->Height, SavePixelsPerInch(Form)));
}

static void ExecuteProcessAndReadOutput(const
  UnicodeString &Command, const UnicodeString HelpKeyword, UnicodeString &Output)
{
  if (!CopyCommandToClipboard(Command))
  {
    SECURITY_ATTRIBUTES SecurityAttributes;
    ZeroMemory(&SecurityAttributes, sizeof(SecurityAttributes));
    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.bInheritHandle = TRUE;
    SecurityAttributes.lpSecurityDescriptor = nullptr;

    HANDLE PipeRead = INVALID_HANDLE_VALUE;
    HANDLE PipeWrite = INVALID_HANDLE_VALUE;

    if (!CreatePipe(&PipeRead, &PipeWrite, &SecurityAttributes, 0) ||
      !SetHandleInformation(PipeRead, HANDLE_FLAG_INHERIT, 0))
    {
      throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, Command));
    }

    PROCESS_INFORMATION ProcessInformation;
    ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));

    try
    {
      try
      {
        STARTUPINFO StartupInfo;
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(STARTUPINFO);
        StartupInfo.hStdError = PipeWrite;
        StartupInfo.hStdOutput = PipeWrite;
        StartupInfo.wShowWindow = SW_HIDE;
        StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

        if (!CreateProcess(nullptr, Command.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &StartupInfo, &ProcessInformation))
        {
          throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, Command));
        }
      }
      __finally
      {
        // If we do not close the handle here, the ReadFile below would get stuck once the app finishes writting,
        // as it still sees that someone "can" write to the pipe.
        CloseHandle(PipeWrite);
      }

      char Buffer[4096];
      DWORD BytesRead;
      while (ReadFile(PipeRead, Buffer, sizeof(Buffer), &BytesRead, nullptr))
      {
        Output += UnicodeString(UTF8String(Buffer, BytesRead));
        // Same as in ExecuteShellCheckedAndWait
        Sleep(200);
        Application->ProcessMessages();
      }

      DWORD ExitCode;
      if (DebugAlwaysTrue(GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode)) &&
        (ExitCode != 0))
      {
        UnicodeString Buf = Output;
        UnicodeString Buf2;
        if (ExtractMainInstructions(Buf, Buf2))
        {
          throw ExtException(Output, UnicodeString(), HelpKeyword);
        }
        else
        {
          throw ExtException(MainInstructions(FMTLOAD(COMMAND_FAILED_CODEONLY, (ToInt(ExitCode)))), Output, HelpKeyword);
        }
      }
    }
    __finally
    {
      CloseHandle(ProcessInformation.hProcess);
      CloseHandle(ProcessInformation.hThread);
    }
  }
}

void ExecuteProcessChecked(
  UnicodeString Command, const UnicodeString HelpKeyword, UnicodeString *Output)
{
  if (Output == nullptr)
  {
    ExecuteShellChecked(Command);
  }
  else
  {
    ExecuteProcessAndReadOutput(Command, HelpKeyword, *Output);
  }
}

void ExecuteProcessCheckedAndWait(
  UnicodeString Command, const UnicodeString HelpKeyword, UnicodeString *Output)
{
  if (Output == nullptr)
  {
    ExecuteShellCheckedAndWait(Command, &Application->ProcessMessages);
  }
  else
  {
    ExecuteProcessAndReadOutput(Command, HelpKeyword, *Output);
  }
}

bool IsKeyPressed(int VirtualKey)
{
  return FLAGSET(GetAsyncKeyState(VirtualKey), 0x8000);
}

bool UseAlternativeFunction()
{
  return IsKeyPressed(VK_SHIFT);
}

bool OpenInNewWindow()
{
  return UseAlternativeFunction();
}

void ExecuteNewInstance(const UnicodeString Param)
{
  UnicodeString Arg = Param;
  if (!Arg.IsEmpty())
  {
    Arg = FORMAT(L"\"%s\" %s", Arg, TProgramParams::FormatSwitch(NEWINSTANCE_SWICH));
  }

  ExecuteShellChecked(Application->ExeName, Arg);
}

IShellLink *CreateDesktopShortCut(const UnicodeString Name,
  UnicodeString File, const UnicodeString Params, const UnicodeString Description,
  int SpecialFolder, int IconIndex, bool Return)
{
  IShellLink *pLink = nullptr;

  if (SpecialFolder < 0)
  {
    SpecialFolder = CSIDL_DESKTOPDIRECTORY;
  }

  try
  {
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
          IID_IShellLink, (void **) &pLink)))
    {
      try
      {
        pLink->SetPath(File.c_str());
        pLink->SetDescription(Description.c_str());
        pLink->SetArguments(Params.c_str());
        pLink->SetShowCmd(SW_SHOW);
        // Explicitly setting icon file,
        // without this icons are not shown at least in Windows 7 jumplist
        pLink->SetIconLocation(File.c_str(), IconIndex);

        IPersistFile *pPersistFile;
        if (!Return &&
          SUCCEEDED(pLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile)))
        {
          try
          {
            LPMALLOC      ShellMalloc;
            LPITEMIDLIST  DesktopPidl;
            wchar_t DesktopDir[MAX_PATH];

            OleCheck(SHGetMalloc(&ShellMalloc));

            try
            {
              OleCheck(SHGetSpecialFolderLocation(nullptr, SpecialFolder, &DesktopPidl));

              OleCheck(SHGetPathFromIDList(DesktopPidl, DesktopDir));
            }
            __finally
            {
              ShellMalloc->Free(DesktopPidl);
              ShellMalloc->Release();
            }

            WideString strShortCutLocation(DesktopDir);
            // Name can contain even path (e.g. to create quick launch icon)
            strShortCutLocation += UnicodeString(L"\\") + Name + L".lnk";
            OleCheck(pPersistFile->Save(strShortCutLocation.c_bstr(), TRUE));
          }
          __finally
          {
            pPersistFile->Release();
          }
        }

        // this is necessary for Windows 7 taskbar jump list links
        IPropertyStore *PropertyStore;
        if (SUCCEEDED(pLink->QueryInterface(IID_IPropertyStore, (void **)&PropertyStore)))
        {
          PROPVARIANT Prop;
          Prop.vt = VT_LPWSTR;
          Prop.pwszVal = Name.c_str();
          PropertyStore->SetValue(PKEY_Title, Prop);
          PropertyStore->Commit();
          PropertyStore->Release();
        }
      }
      catch(...)
      {
        pLink->Release();
        throw;
      }

      if (!Return)
      {
        pLink->Release();
        pLink = nullptr;
      }
    }
  }
  catch(Exception &E)
  {
    throw ExtException(&E, LoadStr(CREATE_SHORTCUT_ERROR));
  }

  return pLink;
}

IShellLink *CreateDesktopSessionShortCut(
  UnicodeString SessionName, const UnicodeString Name,
  UnicodeString AdditionalParams, int SpecialFolder, int IconIndex,
  bool Return)
{
  bool DefaultsOnly;
  UnicodeString InfoTip;

  bool IsFolder = StoredSessions->IsFolder(SessionName);
  bool IsWorkspace = StoredSessions->IsWorkspace(SessionName);

  if (IsFolder || IsWorkspace)
  {
    InfoTip = FMTLOAD(
        (IsFolder ? SHORTCUT_INFO_TIP_FOLDER : SHORTCUT_INFO_TIP_WORKSPACE),
        SessionName);

    if (Name.IsEmpty())
    {
      // no slashes in filename
      Name = UnixExtractFileName(SessionName);
    }
  }
  else
  {
    // this should not be done for workspaces and folders
    TSessionData *SessionData =
      StoredSessions->ParseUrl(EncodeUrlString(SessionName), nullptr, DefaultsOnly);
    InfoTip =
      FMTLOAD(SHORTCUT_INFO_TIP, SessionName, SessionData->InfoTip);
    if (Name.IsEmpty())
    {
      // no slashes in filename
      Name = SessionData->LocalName;
    }
    delete SessionData;
  }

  return
    CreateDesktopShortCut(ValidLocalFileName(Name), Application->ExeName,
      FORMAT(L"\"%s\"%s%s", EncodeUrlString(SessionName), (AdditionalParams.IsEmpty() ? L"" : L" "), AdditionalParams),
      InfoTip, SpecialFolder, IconIndex, Return);
}

template<class TEditControl>
void ValidateMaskEditT(const UnicodeString Mask, TEditControl *Edit, int ForceDirectoryMasks)
{
  DebugAssert(Edit != nullptr);
  TFileMasks Masks(ForceDirectoryMasks);
  try
  {
    Masks = Mask;
  }
  catch(EFileMasksException &E)
  {
    ShowExtendedException(&E);
    Edit->SetFocus();
    // This does not work for TEdit and TMemo (descendants of TCustomEdit) anymore,
    // as it re-selects whole text on exception in TCustomEdit.CMExit
    Edit->SelStart = E.ErrorStart - 1;
    Edit->SelLength = E.ErrorLen;
    Abort();
  }
}

void ValidateMaskEdit(TComboBox *Edit)
{
  ValidateMaskEditT(Edit->Text, Edit, -1);
}

void ValidateMaskEdit(TEdit *Edit)
{
  ValidateMaskEditT(Edit->Text, Edit, -1);
}

void ValidateMaskEdit(TMemo *Edit, bool Directory)
{
  UnicodeString Mask = TFileMasks::ComposeMaskStr(GetUnwrappedMemoLines(Edit), Directory);
  ValidateMaskEditT(Mask, Edit, Directory ? 1 : 0);
}

TStrings *GetUnwrappedMemoLines(TMemo *Memo)
{
  // This removes soft linebreakes when text in memo wraps
  // (Memo->Lines includes soft linebreaks, while Memo->Text does not)
  return TextToStringList(Memo->Text);
}

void ExitActiveControl(TForm *Form)
{
  if (Form->ActiveControl != nullptr)
  {
    TNotifyEvent OnExit = ((TEdit *)Form->ActiveControl)->OnExit;
    if (OnExit != nullptr)
    {
      OnExit(Form->ActiveControl);
    }
  }
}

bool IsWinSCPUrl(const UnicodeString Url)
{
  UnicodeString HomePageUrl = LoadStr(HOMEPAGE_URL);
  UnicodeString HttpHomePageUrl = ChangeUrlProtocol(HomePageUrl, HttpProtocol);
  DebugAssert(HomePageUrl != HttpHomePageUrl);
  return
    StartsText(HomePageUrl, Url) ||
    StartsText(HttpHomePageUrl, Url);
}

UnicodeString SecureUrl(const UnicodeString Url)
{
  UnicodeString Result = Url;
  if (IsWinSCPUrl(Url) && IsHttpUrl(Url))
  {
    Result = ChangeUrlProtocol(Result, HttpsProtocol);
  }
  return Result;
}

void OpenBrowser(const UnicodeString URL)
{
  if (IsWinSCPUrl(URL))
  {
    DebugAssert(!IsHttpUrl(URL));
    URL = CampaignUrl(URL);
  }
  ShellExecute(Application->Handle, L"open", URL.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void OpenFolderInExplorer(const UnicodeString Path)
{
  if ((int)ShellExecute(Application->Handle, L"explore",
      (wchar_t *)Path.data(), nullptr, nullptr, SW_SHOWNORMAL) <= 32)
  {
    throw Exception(FMTLOAD(EXPLORE_LOCAL_DIR_ERROR, Path));
  }
}

void OpenFileInExplorer(const UnicodeString Path)
{
  PCIDLIST_ABSOLUTE Folder = ILCreateFromPathW(ApiPath(Path).c_str());
  SHOpenFolderAndSelectItems(Folder, 0, nullptr, 0);
}

void ShowHelp(const UnicodeString AHelpKeyword)
{
  // see also AppendUrlParams
  UnicodeString HelpKeyword = AHelpKeyword;
  const wchar_t FragmentSeparator = L'#';
  UnicodeString HelpPath = CutToChar(HelpKeyword, FragmentSeparator, false);
  UnicodeString HelpUrl = FMTLOAD(DOCUMENTATION_KEYWORD_URL2, HelpPath, Configuration->ProductVersion, GUIConfiguration->AppliedLocaleHex);
  AddToList(HelpUrl, HelpKeyword, FragmentSeparator);
  OpenBrowser(HelpUrl);
}

bool IsFormatInClipboard(unsigned int Format)
{
  bool Result = OpenClipboard(0);
  if (Result)
  {
    Result = IsClipboardFormatAvailable(Format);
    CloseClipboard();
  }
  return Result;
}
#endif // #if 0

HANDLE OpenTextFromClipboard(const wchar_t *&Text)
{
  HANDLE Result = nullptr;
  if (OpenClipboard(0))
  {
    // Check also for CF_TEXT?
    Result = GetClipboardData(CF_UNICODETEXT);
    if (Result != nullptr)
    {
      Text = static_cast<const wchar_t *>(GlobalLock(Result));
    }
    else
    {
      CloseClipboard();
    }
  }
  return Result;
}

void CloseTextFromClipboard(HANDLE Handle)
{
  if (Handle != nullptr)
  {
    GlobalUnlock(Handle);
  }
  CloseClipboard();
}

bool TextFromClipboard(UnicodeString &Text, bool Trim)
{
  const wchar_t *AText = nullptr;
  HANDLE Handle = OpenTextFromClipboard(AText);
  bool Result = (Handle != nullptr);
  if (Result)
  {
    Text = AText;
    if (Trim)
    {
      Text = Text.Trim();
    }
    CloseTextFromClipboard(Handle);
  }
  return Result;
}

bool NonEmptyTextFromClipboard(UnicodeString &Text)
{
  return
    TextFromClipboard(Text, true) &&
    !Text.IsEmpty();
}

static bool GetResource(
  const UnicodeString ResName, void *&Content, unsigned long &Size)
{
  HRSRC Resource = FindResourceEx(GetGlobals()->GetInstanceHandle(), RT_RCDATA, ResName.c_str(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
  bool Result = (Resource != nullptr);
  if (Result)
  {
    Size = SizeofResource(GetGlobals()->GetInstanceHandle(), Resource);
    if (!Size)
    {
      throw Exception(FORMAT("Cannot get size of resource %s", ResName));
    }

    Content = LoadResource(GetGlobals()->GetInstanceHandle(), Resource);
    if (!Content)
    {
      throw Exception(FORMAT("Cannot read resource %s", ResName));
    }

    Content = LockResource(Content);
    if (!Content)
    {
      throw Exception(FORMAT("Cannot lock resource %s", ResName));
    }
  }

  return Result;
}

bool DumpResourceToFile(const UnicodeString ResName,
  const UnicodeString FileName)
{
  void *Content;
  unsigned long Size;
  bool Result = GetResource(ResName, Content, Size);

  if (Result)
  {
    FILE *f = _wfopen(ApiPath(FileName).c_str(), L"wb");
    if (!f)
    {
      throw Exception(FORMAT(L"Cannot create file %s", FileName));
    }
    if (fwrite(Content, 1, Size, f) != Size)
    {
      throw Exception(FORMAT(L"Cannot write to file %s", FileName));
    }
    fclose(f);
  }

  return Result;
}

UnicodeString ReadResource(const UnicodeString ResName)
{
  void *Content;
  unsigned long Size;
  UnicodeString Result;

  if (GetResource(ResName, Content, Size))
  {
    Result = UnicodeString(UTF8String(static_cast<char *>(Content), Size));
  }

  return Result;
}

#if 0
template <class T>
void BrowseForExecutableT(T *Control, const UnicodeString Title,
  UnicodeString Filter, bool FileNameCommand, bool Escape)
{
  UnicodeString Executable, Program, Params, Dir;
  Executable = Control->Text;
  if (FileNameCommand)
  {
    ReformatFileNameCommand(Executable);
  }
  SplitCommand(Executable, Program, Params, Dir);

  TOpenDialog *FileDialog = new TOpenDialog(Application);
  try
  {
    if (Escape)
    {
      Program = ReplaceStr(Program, L"\\\\", L"\\");
    }
    UnicodeString ExpandedProgram = ExpandEnvironmentVariables(Program);
    FileDialog->FileName = ExpandedProgram;
    UnicodeString InitialDir = ::ExtractFilePath(ExpandedProgram);
    if (!InitialDir.IsEmpty())
    {
      FileDialog->InitialDir = InitialDir;
    }
    FileDialog->Filter = Filter;
    FileDialog->Title = Title;

    if (FileDialog->Execute())
    {
      TNotifyEvent PrevOnChange = Control->OnChange;
      Control->OnChange = nullptr;
      try
      {
        // preserve unexpanded file, if the destination has not changed actually
        if (!IsPathToSameFile(ExpandedProgram, FileDialog->FileName))
        {
          Program = FileDialog->FileName;
          if (Escape)
          {
            Program = ReplaceStr(Program, L"\\", L"\\\\");
          }
        }
        Control->Text = FormatCommand(Program, Params);
      }
      __finally
      {
        Control->OnChange = PrevOnChange;
      }

      if (Control->OnExit != nullptr)
      {
        Control->OnExit(Control);
      }
    }
  }
  __finally
  {
    delete FileDialog;
  }
}

void BrowseForExecutable(TEdit *Control, const UnicodeString Title,
  UnicodeString Filter, bool FileNameCommand, bool Escape)
{
  BrowseForExecutableT(Control, Title, Filter, FileNameCommand, Escape);
}

void BrowseForExecutable(TComboBox *Control, const UnicodeString Title,
  UnicodeString Filter, bool FileNameCommand, bool Escape)
{
  BrowseForExecutableT(Control, Title, Filter, FileNameCommand, Escape);
}

bool FontDialog(TFont *Font)
{
  bool Result;
  TFontDialog *Dialog = new TFontDialog(Application);
  try
  {
    Dialog->Device = fdScreen;
    Dialog->Options = TFontDialogOptions() << fdForceFontExist;
    Dialog->Font = Font;
    Result = Dialog->Execute();
    if (Result)
    {
      Font->Assign(Dialog->Font);
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}

bool SaveDialog(const UnicodeString Title, const UnicodeString Filter,
  UnicodeString DefaultExt, UnicodeString &FileName)
{
  bool Result;
#if 0
  TFileSaveDialog *Dialog = new TFileSaveDialog(Application);
  try
  {
    Dialog->Title = Title;
    FilterToFileTypes(Filter, Dialog->FileTypes);
    Dialog->DefaultExtension = DefaultExt;
    Dialog->FileName = FileName;
    UnicodeString DefaultFolder = ::ExtractFilePath(FileName);
    if (!DefaultFolder.IsEmpty())
    {
      Dialog->DefaultFolder = DefaultFolder;
    }
    Dialog->Options = Dialog->Options << fdoOverWritePrompt << fdoForceFileSystem <<
      fdoPathMustExist << fdoNoReadOnlyReturn;
    Result = Dialog->Execute();
    if (Result)
    {
      FileName = Dialog->FileName;
    }
  }
  __finally
  {
    delete Dialog;
  }
#else
  TSaveDialog *Dialog = new TSaveDialog(Application);
  try
  {
    Dialog->Title = Title;
    Dialog->Filter = Filter;
    Dialog->DefaultExt = DefaultExt;
    Dialog->FileName = FileName;
    UnicodeString InitialDir = ::ExtractFilePath(FileName);
    if (!InitialDir.IsEmpty())
    {
      Dialog->InitialDir = InitialDir;
    }
    Dialog->Options = Dialog->Options << ofOverwritePrompt << ofPathMustExist <<
      ofNoReadOnlyReturn;
    Result = Dialog->Execute();
    if (Result)
    {
      FileName = Dialog->FileName;
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
#endif // #if 0
}
#endif // #if 0

bool SaveDialog(const UnicodeString ATitle, const UnicodeString Filter,
  UnicodeString ADefaultExt, UnicodeString &AFileName)
{
  bool Result = false;
  DebugUsedParam(Filter);
  DebugUsedParam(ADefaultExt);

  Result = InputDialog(ATitle, L""/*LoadStr(LOGIN_PRIVATE_KEY)*/, AFileName, L"", nullptr, true, nullptr, true);
  return Result;
}

#if 0
// implemented in FarInterface.cpp
void CopyToClipboard(const UnicodeString Text)
{
  HANDLE Data;
  void *DataPtr;

  if (OpenClipboard(0))
  {
    try__finally
    {
      SCOPE_EXIT
      {
        CloseClipboard();
      };
      size_t Size = (Text.Length() + 1) * sizeof(wchar_t);
      Data = GlobalAlloc(GMEM_MOVEABLE + GMEM_DDESHARE, Size);
      try
      {
        SCOPE_EXIT
        {
          GlobalUnlock(Data);
        };
        DataPtr = GlobalLock(Data);
        try__finally
        {
          SCOPE_EXIT
          {
            GlobalUnlock(Data);
          };
          memcpy(DataPtr, Text.c_str(), Size);
          EmptyClipboard();
          SetClipboardData(CF_UNICODETEXT, Data);
        }
        __finally
        {
          GlobalUnlock(Data);
        };
      }
      catch (...)
      {
        GlobalFree(Data);
        throw;
      }
    }
    __finally
    {
      CloseClipboard();
    };
  }
  else
  {
    throw Exception(SCannotOpenClipboard);
  }
}
#endif // #if 0

void CopyToClipboard(TStrings *Strings)
{
  if (Strings->GetCount() > 0)
  {
    CopyToClipboard(StringsToText(Strings));
  }
}

bool IsWin64()
{
  static int Result = -1;
  if (Result < 0)
  {
    Result = 0;
    BOOL Wow64Process = FALSE;
    if (::IsWow64Process(::GetCurrentProcess(), &Wow64Process))
    {
      if (Wow64Process)
      {
        Result = 1;
      }
    }
  }

  return (Result > 0);
}

static void AcquireShutDownPrivileges()
{
  HANDLE Token;
  // Get a token for this process.
  Win32Check(FALSE != ::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token));

  TOKEN_PRIVILEGES Priv;
  ZeroMemory(&Priv, sizeof(Priv));
  // Get the LUID for the shutdown privilege.
  // For hibernate/suspend, you need the same:
  // https://stackoverflow.com/q/959589/850848
  Win32Check(FALSE != ::LookupPrivilegeValue(nullptr, SE_SHUTDOWN_NAME, &Priv.Privileges[0].Luid));

  Priv.PrivilegeCount = 1;  // one privilege to set
  Priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  // Get the shutdown privilege for this process.
  Win32Check(FALSE != ::AdjustTokenPrivileges(Token, FALSE, &Priv, 0, static_cast<PTOKEN_PRIVILEGES>(nullptr), nullptr));
}

void ShutDownWindows()
{
  AcquireShutDownPrivileges();

  // Shut down the system and force all applications to close.
  Win32Check(FALSE != ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF,
      SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED));
}

#if 0

void SuspendWindows()
{
  AcquireShutDownPrivileges();

  // https://msdn.microsoft.com/en-us/library/windows/desktop/aa373201.aspx
  Win32Check(FALSE != SetSuspendState(false, false, false));
}

void EditSelectBaseName(HWND Edit)
{
  UnicodeString Text;
  Text.SetLength(GetWindowTextLength(Edit) + 1);
  GetWindowText(Edit, Text.c_str(), Text.Length());

  int P = Text.LastDelimiter(L".");
  if (P > 0)
  {
    // SendMessage does not work, if edit control is not fully
    // initialized yet
    PostMessage(Edit, EM_SETSEL, 0, P - 1);
  }
}

#endif // #if 0

static void ConvertKey(UnicodeString &FileName, TKeyType Type)
{
  UnicodeString Passphrase;

  UnicodeString Comment;
  if (IsKeyEncrypted(Type, FileName, Comment))
  {
    if (!InputDialog(
        LoadStr(PASSPHRASE_TITLE),
        FORMAT(LoadStr(PROMPT_KEY_PASSPHRASE), Comment),
        Passphrase, HELP_NONE, nullptr, false, nullptr, false))
    {
      Abort();
    }
  }

  TPrivateKey *PrivateKey = LoadKey(Type, FileName, Passphrase);

  try__finally
  {
    SCOPE_EXIT
    {
      FreeKey(PrivateKey);
    };
    FileName = ::ChangeFileExt(FileName, ".ppk", L'\\');

    if (!SaveDialog(LoadStr(CONVERTKEY_SAVE_TITLE), LoadStr(CONVERTKEY_SAVE_FILTER), L"ppk", FileName))
    {
      Abort();
    }

    SaveKey(ktSSH2, FileName, Passphrase, PrivateKey);

    MessageDialog(MainInstructions(FMTLOAD(CONVERTKEY_SAVED, FileName)), qtInformation, qaOK);
  }
  __finally__removed
  ({
    FreeKey(PrivateKey);
  })
}

static void DoVerifyKey(
  UnicodeString AFileName, TSshProt SshProt, bool Convert)
{
  if (!AFileName.Trim().IsEmpty())
  {
    UnicodeString FileName = ::ExpandEnvironmentVariables(AFileName);
    TKeyType Type = GetKeyType(FileName);
    // reason _wfopen failed
    int Error = errno;
    UnicodeString Message;
    UnicodeString HelpKeyword; // = HELP_LOGIN_KEY_TYPE;
    UnicodeString PuttygenPath;
    std::unique_ptr<TStrings> MoreMessages;
    switch (Type)
    {
    case ktOpenSSHPEM:
    case ktOpenSSHNew:
    case ktSSHCom:
    {
      UnicodeString TypeName = ((Type == ktOpenSSHPEM) || (Type == ktOpenSSHNew)) ? L"OpenSSH" : L"ssh.com";
      Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName, TypeName);

      if (Convert)
      {
        // Configuration->Usage->Inc(L"PrivateKeyConvertSuggestionsNative");
        UnicodeString ConvertMessage = FMTLOAD(KEY_TYPE_CONVERT3, TypeName, RemoveMainInstructionsTag(Message));
        Message = UnicodeString();
        if (MoreMessageDialog(ConvertMessage, nullptr, qtConfirmation, qaOK | qaCancel, HelpKeyword) == qaOK)
        {
          ConvertKey(FileName, Type);
          // Configuration->Usage->Inc(L"PrivateKeyConverted");
        }
        else
        {
          Abort();
        }
      }
      else
      {
        HelpKeyword = ""; // HELP_KEY_TYPE_UNSUPPORTED;
      }
    }
    break;

    case ktSSH1:
    case ktSSH2:
      if ((Type == ktSSH1) != (SshProt == ssh1only))
      {
        Message =
          MainInstructions(
            FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
              AFileName, Type == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2"));
      }
      break;

    case ktSSH1Public:
    case ktSSH2PublicRFC4716:
    case ktSSH2PublicOpenSSH:
      // noop
      // Do not even bother checking SSH protocol version
      break;

    case ktUnopenable:
      Message = MainInstructions(FMTLOAD(KEY_TYPE_UNOPENABLE, AFileName));
      if (Error != ERROR_SUCCESS)
      {
        MoreMessages.reset(TextToStringList(SysErrorMessageForError(Error)));
      }
      break;

    default:
      DebugFail();
    // fallthru
    case ktUnknown:
      Message = MainInstructions(FMTLOAD(KEY_TYPE_UNKNOWN2, AFileName));
      break;
    }

    if (!Message.IsEmpty())
    {
      // Configuration->Usage->Inc(L"PrivateKeySelectErrors");
      if (MoreMessageDialog(Message, MoreMessages.get(), qtWarning, qaIgnore | qaAbort, HelpKeyword) == qaAbort)
      {
        Abort();
      }
    }
  }
}

void VerifyAndConvertKey(const UnicodeString AFileName, TSshProt SshProt)
{
  DoVerifyKey(AFileName, SshProt, true);
}

void VerifyKey(const UnicodeString FileName, TSshProt SshProt)
{
  DoVerifyKey(FileName, SshProt, false);
}

void VerifyCertificate(const UnicodeString FileName)
{
  if (!FileName.Trim().IsEmpty())
  {
    try
    {
      CheckCertificate(FileName);
    }
    catch (Exception &E)
    {
      if (ExceptionMessageDialog(&E, qtWarning, L"", qaIgnore | qaAbort) == qaAbort)
      {
        Abort();
      }
    }
  }
}

bool DetectSystemExternalEditor(
  bool /*AllowDefaultEditor*/,
  UnicodeString & /*Executable*/, UnicodeString & /*ExecutableDescription*/,
  UnicodeString & /*UsageState*/, bool & /*TryNextTime*/)
{
  bool Result = false;
#if 0
  UnicodeString TempName = ::ExcludeTrailingBackslash(WinConfiguration->TemporaryDir()) + L".txt";
  if (FileExists(ApiPath(TempName)))
  {
    TryNextTime = true;
    UsageState = "F";
  }
  else
  {
    unsigned int File = FileCreate(ApiPath(TempName));
    if (File == (unsigned int)INVALID_HANDLE_VALUE)
    {
      TryNextTime = true;
      UsageState = "F";
    }
    else
    {
      FileClose(File);

      try
      {
        wchar_t ExecutableBuf[MAX_PATH];
        if (!SUCCEEDED(FindExecutable(TempName.c_str(), nullptr, ExecutableBuf)))
        {
          UsageState = "N";
        }
        else
        {
          Executable = ExecutableBuf;
          if (Executable.IsEmpty() ||
            !FileExists(ApiPath(Executable)))
          {
            UsageState = "N";
          }
          else
          {
            UnicodeString ExecutableName = ExtractFileName(Executable);
            if (!AllowDefaultEditor &&
              SameText(ExecutableName, TEditorPreferences::GetDefaultExternalEditor()))
            {
              UsageState = "P";
              Executable = L"";
            }
            else if (SameText(ExecutableName, "openwith.exe"))
            {
              UsageState = "W";
              Executable = L"";
            }
            else
            {
              try
              {
                ExecutableDescription = Configuration->GetFileDescription(Executable);
              }
              catch(...)
              {
              }

              if (ExecutableDescription.IsEmpty())
              {
                ExecutableDescription = ExecutableName;
              }

              Result = true;
            }
          }
        }
      }
      __finally
      {
        DeleteFile(ApiPath(TempName));
      }
    }
  }
#endif // #if 0
  return Result;
}

static bool GetProxyUrlFromIE(UnicodeString &Proxy)
{
  bool Result = false;
  // Code from http://gentoo.osuosl.org/distfiles/cl331.zip/io/
  WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IEProxyInfo;
  ClearStruct(IEProxyInfo);
  TLibraryLoader LibraryLoader(L"winhttp.dll", true);
  if (LibraryLoader.Loaded())
  {
    typedef BOOL (WINAPI * FWinHttpGetIEProxyConfigForCurrentUser)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *);
    FWinHttpGetIEProxyConfigForCurrentUser GetIEProxyConfig = reinterpret_cast<FWinHttpGetIEProxyConfigForCurrentUser>(
        LibraryLoader.GetProcAddress("WinHttpGetIEProxyConfigForCurrentUser"));
    if (GetIEProxyConfig && GetIEProxyConfig(&IEProxyInfo))
    {
      if (IEProxyInfo.lpszProxy != nullptr)
      {
        UnicodeString IEProxy = IEProxyInfo.lpszProxy;
        Proxy = L"";
        while (Proxy.IsEmpty() && !IEProxy.IsEmpty())
        {
          UnicodeString Str = CutToChar(IEProxy, L';', true);
          if (Str.Pos(L"=") == 0)
          {
            Proxy = Str;
          }
          else
          {
            UnicodeString Protocol = CutToChar(Str, L'=', true);
            if (SameText(Protocol, L"http"))
            {
              Proxy = Str;
            }
          }
        }

        GlobalFree(IEProxyInfo.lpszProxy);
        Result = true;
      }
      if (IEProxyInfo.lpszAutoConfigUrl != nullptr)
      {
        GlobalFree(IEProxyInfo.lpszAutoConfigUrl);
      }
      if (IEProxyInfo.lpszProxyBypass != nullptr)
      {
        GlobalFree(IEProxyInfo.lpszProxyBypass);
      }
    }
  }
  return Result;
}

bool AutodetectProxy(UnicodeString &AHostName, intptr_t &APortNumber)
{
  bool Result = false;

  /* First we try for proxy info direct from the registry if
     it's available. */
  UnicodeString Proxy;
  WINHTTP_PROXY_INFO ProxyInfo;
  //memset(&ProxyInfo, 0, sizeof(ProxyInfo));
  ClearStruct(ProxyInfo);
  // if (WinHttpGetDefaultProxyConfiguration(&ProxyInfo))
  TLibraryLoader LibraryLoader(L"winhttp.dll", true);
  if (LibraryLoader.Loaded())
  {
    typedef BOOL (WINAPI * FWinHttpGetDefaultProxyConfiguration)(WINHTTP_PROXY_INFO *);
    FWinHttpGetDefaultProxyConfiguration GetDefaultProxyConfiguration = reinterpret_cast<FWinHttpGetDefaultProxyConfiguration>(
        LibraryLoader.GetProcAddress("WinHttpGetDefaultProxyConfiguration"));
    if (GetDefaultProxyConfiguration)
    {
      if (GetDefaultProxyConfiguration(&ProxyInfo))
      {
        if (ProxyInfo.lpszProxy != nullptr)
        {
          Proxy = ProxyInfo.lpszProxy;
          GlobalFree(ProxyInfo.lpszProxy);
          Result = true;
        }
        if (ProxyInfo.lpszProxyBypass != nullptr)
        {
          GlobalFree(ProxyInfo.lpszProxyBypass);
        }
      }
    }
  }

  /* The next fallback is to get the proxy info from MSIE.  This is also
     usually much quicker than WinHttpGetProxyForUrl(), although sometimes
     it seems to fall back to that, based on the longish delay involved.
     Another issue with this is that it won't work in a service process
     that isn't impersonating an interactive user (since there isn't a
     current user), but in that case we just fall back to
     WinHttpGetProxyForUrl() */
  if (!Result)
  {
    Result = GetProxyUrlFromIE(Proxy);
  }

  if (Result)
  {
    if (Proxy.Trim().IsEmpty())
    {
      Result = false;
    }
    else
    {
      AHostName = CutToChar(Proxy, L':', true);
      APortNumber = StrToIntDef(Proxy, ProxyPortNumber);
    }
  }

  // We can also use WinHttpGetProxyForUrl, but it is lengthy
  // See the source address of the code for example

  return Result;
}


#if 0
class TWinHelpTester : public TInterfacedObject, public IWinHelpTester
{
public:
  virtual bool CanShowALink(const UnicodeString ALink, const UnicodeString FileName);
  virtual bool CanShowTopic(const UnicodeString Topic, const UnicodeString FileName);
  virtual bool CanShowContext(const int Context, const UnicodeString FileName);
  virtual TStringList *GetHelpStrings(const UnicodeString ALink);
  virtual UnicodeString GetHelpPath();
  virtual UnicodeString GetDefaultHelpFile();

  IUNKNOWN
};

class TCustomHelpSelector : public TInterfacedObject, public IHelpSelector
{
public:
  TCustomHelpSelector(const UnicodeString Name);

  virtual int SelectKeyword(TStrings *Keywords);
  virtual int TableOfContents(TStrings *Contents);

  IUNKNOWN

private:
  UnicodeString FName;
};

void AssignHelpSelector(IHelpSelector *HelpSelector)
{
  _di_IHelpSystem HelpSystem;
  if (GetHelpSystem(HelpSystem))
  {
    HelpSystem->AssignHelpSelector(HelpSelector);
  }
}

void InitializeCustomHelp(ICustomHelpViewer *HelpViewer)
{
  _di_IHelpManager HelpManager;
  RegisterViewer(HelpViewer, HelpManager);

  // Register dummy tester that disables win help
  WinHelpTester = new TWinHelpTester();

  AssignHelpSelector(new TCustomHelpSelector(HelpViewer->GetViewerName()));
}

void FinalizeCustomHelp()
{
  AssignHelpSelector(nullptr);
}


bool TWinHelpTester::CanShowALink(const UnicodeString ALink,
  const UnicodeString FileName)
{
  return !Application->HelpFile.IsEmpty();
}

bool TWinHelpTester::CanShowTopic(const UnicodeString Topic,
  const UnicodeString FileName)
{
  DebugFail();
  return !Application->HelpFile.IsEmpty();
}

bool TWinHelpTester::CanShowContext(const int /*Context*/,
  const UnicodeString FileName)
{
  DebugFail();
  return !Application->HelpFile.IsEmpty();
}

TStringList *TWinHelpTester::GetHelpStrings(const UnicodeString ALink)
{
  DebugFail();
  TStringList *Result = new TStringList();
  Result->Add(ViewerName + L": " + ALink);
  return Result;
}

UnicodeString TWinHelpTester::GetHelpPath()
{
  // never called on windows anyway
  return ::ExtractFilePath(Application->HelpFile);
}

UnicodeString TWinHelpTester::GetDefaultHelpFile()
{
  return Application->HelpFile;
}


TCustomHelpSelector::TCustomHelpSelector(const UnicodeString Name) :
  FName(Name)
{
}

int TCustomHelpSelector::SelectKeyword(TStrings * /*Keywords*/)
{
  DebugFail();
  return 0;
}

int TCustomHelpSelector::TableOfContents(TStrings *Contents)
{
  return Contents->IndexOf(FName);
}
#endif // #if 0
