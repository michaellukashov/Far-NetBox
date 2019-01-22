
#pragma once

#include <Classes.hpp>
#include <FileMasks.h>
#include <DirectoryMonitor.hpp>
//---------------------------------------------------------------------------
class TSessionData;
//---------------------------------------------------------------------------
#if 0
typedef void (__closure *TProcessMessagesEvent)();
#endif // #if 0
using TProcessMessagesEvent = nb::FastDelegate0<void>;
//---------------------------------------------------------------------------
NB_CORE_EXPORT bool FindFile(UnicodeString &APath);
NB_CORE_EXPORT bool FindTool(UnicodeString Name, UnicodeString &APath);
NB_CORE_EXPORT void ExecuteTool(UnicodeString Name);
NB_CORE_EXPORT void ExecuteShellChecked(UnicodeString APath, UnicodeString Params,
  bool ChangeWorkingDirectory = false);
NB_CORE_EXPORT void ExecuteShellChecked(UnicodeString Command);
NB_CORE_EXPORT bool ExecuteShell(UnicodeString Path, UnicodeString Params,
  HANDLE &Handle);
NB_CORE_EXPORT void ExecuteShellCheckedAndWait(UnicodeString Command, TProcessMessagesEvent ProcessMessages);
__removed TObjectList * StartCreationDirectoryMonitorsOnEachDrive(uintptr_t Filter, TFileChangedEvent OnChanged);
NB_CORE_EXPORT bool CopyCommandToClipboard(UnicodeString ACommand);
extern bool DontCopyCommandToClipboard;
NB_CORE_EXPORT void OpenSessionInPutty(UnicodeString PuttyPath,
  TSessionData *SessionData);
NB_CORE_EXPORT bool SpecialFolderLocation(intptr_t PathID, UnicodeString &APath);
NB_CORE_EXPORT UnicodeString UniqTempDir(UnicodeString BaseDir,
  UnicodeString Identity, bool Mask = false);
NB_CORE_EXPORT bool DeleteDirectory(UnicodeString ADirName);

#if 0
int GetSessionColorImage(TCustomImageList * ImageList, TColor Color, int MaskIndex);
void RegenerateSessionColorsImageList(TCustomImageList * ImageList, int MaskIndex);
void __fastcall SetSubmenu(TTBXCustomItem * Item);
typedef int __fastcall (*TCalculateWidth)(UnicodeString Text, void * Arg);
void __fastcall ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg);
TPanel * __fastcall CreateLabelPanel(TPanel * Parent, UnicodeString & Label);
void __fastcall SelectScaledImageList(TImageList * ImageList);
void __fastcall CopyImageList(TImageList * TargetList, TImageList * SourceList);
void __fastcall LoadDialogImage(TImage * Image, UnicodeString & ImageName);
int __fastcall DialogImageSize(TForm * Form);
int __fastcall NormalizePixelsPerInch(int PixelsPerInch);
void __fastcall HideComponentsPanel(TForm * Form);
UnicodeString FormatIncrementalSearchStatus(UnicodeString & Text, bool HaveNext);
namespace Webbrowserex
{
  class TWebBrowserEx;
}
using namespace Webbrowserex;
TWebBrowserEx * __fastcall CreateBrowserViewer(TPanel * Parent, UnicodeString & LoadingLabel);
void __fastcall SetBrowserDesignModeOff(TWebBrowserEx * WebBrowser);
void __fastcall AddBrowserLinkHandler(TWebBrowserEx * WebBrowser,
  UnicodeString & Url, TNotifyEvent Handler);
void __fastcall NavigateBrowserToUrl(TWebBrowserEx * WebBrowser, UnicodeString & Url);
void ReadyBrowserForStreaming(TWebBrowserEx * WebBrowser);
void WaitBrowserToIdle(TWebBrowserEx * WebBrowser);
void HideBrowserScrollbars(TWebBrowserEx * WebBrowser);
UnicodeString GenerateAppHtmlPage(TFont * Font, TPanel * Parent, UnicodeString & Body, bool Seamless);
void LoadBrowserDocument(TWebBrowserEx * WebBrowser, UnicodeString & Document);
void __fastcall GetInstrutionsTheme(
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont);
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont);
#endif // #if 0
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TLocalCustomCommand : public TFileCustomCommand
{
public:
  TLocalCustomCommand() noexcept;
  explicit TLocalCustomCommand(
    const TCustomCommandData &Data, UnicodeString RemotePath, UnicodeString LocalPath) noexcept;
  explicit TLocalCustomCommand(
    const TCustomCommandData &Data, UnicodeString RemotePath, UnicodeString LocalPath,
    UnicodeString AFileName, UnicodeString LocalFileName,
    UnicodeString FileList) noexcept;
  virtual ~TLocalCustomCommand() noexcept = default;

  virtual bool IsFileCommand(UnicodeString Command) const;
  bool HasLocalFileName(UnicodeString Command) const;

protected:
  virtual intptr_t PatternLen(UnicodeString Command, intptr_t Index) const;
  virtual bool PatternReplacement(intptr_t Index, UnicodeString Pattern,
    UnicodeString &Replacement, bool &Delimit) const;
  virtual void DelimitReplacement(UnicodeString &Replacement, wchar_t Quote);

private:
  UnicodeString FLocalPath;
  UnicodeString FLocalFileName;
};
//---------------------------------------------------------------------------
#if 0
namespace Pngimagelist {
class TPngImageList;
class TPngImageCollectionItem;
}
using namespace Pngimagelist;
//---------------------------------------------------------------------------
TPngImageList * __fastcall GetAnimationsImages(TControl * Control);
TImageList * __fastcall GetButtonImages(TControl * Control);
TPngImageList * __fastcall GetDialogImages(TControl * Control);
void __fastcall ReleaseImagesModules();
//---------------------------------------------------------------------------
class TFrameAnimation
{
public:
  __fastcall TFrameAnimation();
  void __fastcall Init(TPaintBox * PaintBox, UnicodeString & Name);
  void __fastcall Start();
  void __fastcall Stop();

private:
  UnicodeString FName;
  TPaintBox * FPaintBox;
  TPngImageList * FImageList;
  int FFirstFrame;
  int FFirstLoopFrame;
  int FLastFrame;
  int FCurrentFrame;
  DWORD FNextFrameTick;
  TTimer * FTimer;
  bool FPainted;

  void __fastcall DoInit();
  void __fastcall PaintBoxPaint(TObject * Sender);
  void __fastcall CalculateNextFrameTick();
  TPngImageCollectionItem * __fastcall GetCurrentImage();
  void __fastcall Animate();
  void __fastcall Timer(TObject * Sender);
  void __fastcall Repaint();
  void __fastcall Rescale();
  static void __fastcall PaintBoxRescale(TComponent * Sender, TObject * Token);
};
//---------------------------------------------------------------------------
class TScreenTipHintWindow : public THintWindow
{
public:
  __fastcall TScreenTipHintWindow(TComponent * Owner);
  virtual TRect __fastcall CalcHintRect(int MaxWidth, UnicodeString AHint, void * AData);
  virtual void __fastcall ActivateHintData(const TRect & Rect, UnicodeString AHint, void * AData);

  static void __fastcall CalcHintTextRect(TControl * Control, TCanvas * Canvas, TRect & Rect, UnicodeString & Hint);

protected:
  virtual void __fastcall Paint();
  virtual void __fastcall Dispatch(void * AMessage);

private:
  bool FParentPainting;
  int FMargin;
  UnicodeString FShortHint;
  UnicodeString FLongHint;
  TControl * FHintControl;
  bool FHintPopup;
  std::unique_ptr<TFont> FScaledHintFont;

  UnicodeString __fastcall GetLongHintIfAny(UnicodeString & AHint);
  static int __fastcall GetTextFlags(TControl * Control);

  bool __fastcall UseBoldShortHint(TControl * HintControl);
  int __fastcall GetMargin(TControl * HintControl, UnicodeString & Hint);
  TFont * __fastcall GetFont(TControl * HintControl, UnicodeString & Hint);
  TControl * __fastcall GetHintControl(void * Data);
  void __fastcall SplitHint(
    TControl * HintControl, UnicodeString & Hint, UnicodeString & ShortHint, UnicodeString & LongHint);
  void __fastcall SplitHint(
    TControl * HintControl, UnicodeString & Hint, UnicodeString & ShortHint, UnicodeString & LongHint);
};
//---------------------------------------------------------------------------
// Newer version rich edit that supports "Friendly name hyperlinks" and
// allows wider range of Unicode characters: https://stackoverflow.com/q/47433656/850848
class TNewRichEdit : public TRichEdit
{
public:
  virtual __fastcall TNewRichEdit(TComponent * AOwner);

protected:
  virtual void __fastcall CreateParams(TCreateParams & Params);
  virtual void __fastcall CreateWnd();
  virtual void __fastcall DestroyWnd();

private:
  HINSTANCE FLibrary;
};
#endif // #if 0

NB_CORE_EXPORT UnicodeString ItemsFormatString(UnicodeString SingleItemFormat,
  UnicodeString MultiItemsFormat, intptr_t Count, UnicodeString FirstItem);
NB_CORE_EXPORT UnicodeString GetPersonalFolder();
NB_CORE_EXPORT UnicodeString ItemsFormatString(UnicodeString SingleItemFormat,
  UnicodeString MultiItemsFormat, const TStrings *Items);
NB_CORE_EXPORT UnicodeString FileNameFormatString(UnicodeString SingleFileFormat,
  UnicodeString MultiFilesFormat, const TStrings *AFiles, bool Remote);
NB_CORE_EXPORT UnicodeString FileNameFormatString(UnicodeString SingleFileFormat,
  UnicodeString MultiFilesFormat, const TStrings *AFiles, bool Remote);
//---------------------------------------------------------------------------
#if 0
// Based on:
// https://stackoverflow.com/q/6912424/850848
NB_CORE_EXPORT extern UnicodeString PageantTool;
NB_CORE_EXPORT extern UnicodeString PuttygenTool;
// https://stackoverflow.com/q/4685863/850848
class TUIStateAwareLabel : public TLabel
{
protected:
  DYNAMIC void __fastcall DoDrawText(TRect & Rect, int Flags);
};
// FindComponentClass takes parameter by reference and as such it cannot be implemented in
// an inline method without a compiler warning, which we cannot suppress in a macro.
// And having the implementation in a real code (not macro) also allows us to debug the code.
void __fastcall FindComponentClass(
  void * Data, TReader * Reader, UnicodeString ClassName, TComponentClass & ComponentClass);
#define INTERFACE_HOOK_CUSTOM(PARENT) \
  protected: \
    virtual void __fastcall ReadState(TReader * Reader) \
    { \
      Reader->OnFindComponentClass = MakeMethod<TFindComponentClassEvent>(NULL, FindComponentClass); \
      PARENT::ReadState(Reader); \
    }
#define INTERFACE_HOOK INTERFACE_HOOK_CUSTOM(TForm)
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#endif // #if 0
