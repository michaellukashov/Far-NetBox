
#pragma once

#include <Classes.hpp>
#include <FileMasks.h>
#include <DirectoryMonitor.hpp>

class TSessionData;

#if 0
typedef void (__closure *TProcessMessagesEvent)();
#endif // #if 0
using TProcessMessagesEvent = nb::FastDelegate0<void>;

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
void SetSubmenu(TTBXCustomItem * Item);
typedef int (*TCalculateWidth)(UnicodeString Text, void * Arg);
void ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg);
TPanel * CreateLabelPanel(TPanel * Parent, UnicodeString & Label);
void SelectScaledImageList(TImageList * ImageList);
void CopyImageList(TImageList * TargetList, TImageList * SourceList);
void LoadDialogImage(TImage * Image, UnicodeString & ImageName);
int DialogImageSize(TForm * Form);
int NormalizePixelsPerInch(int PixelsPerInch);
void HideComponentsPanel(TForm * Form);
UnicodeString FormatIncrementalSearchStatus(UnicodeString & Text, bool HaveNext);
namespace Webbrowserex
{
  class TWebBrowserEx;
}
using namespace Webbrowserex;
TWebBrowserEx * CreateBrowserViewer(TPanel * Parent, UnicodeString & LoadingLabel);
void SetBrowserDesignModeOff(TWebBrowserEx * WebBrowser);
void AddBrowserLinkHandler(TWebBrowserEx * WebBrowser,
  UnicodeString & Url, TNotifyEvent Handler);
void NavigateBrowserToUrl(TWebBrowserEx * WebBrowser, UnicodeString & Url);
void ReadyBrowserForStreaming(TWebBrowserEx * WebBrowser);
void WaitBrowserToIdle(TWebBrowserEx * WebBrowser);
void HideBrowserScrollbars(TWebBrowserEx * WebBrowser);
UnicodeString GenerateAppHtmlPage(TFont * Font, TPanel * Parent, UnicodeString & Body, bool Seamless);
void LoadBrowserDocument(TWebBrowserEx * WebBrowser, UnicodeString & Document);
void GetInstrutionsTheme(
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont);
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont);
#endif // #if 0

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
  virtual ~TLocalCustomCommand() = default;

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

#if 0
namespace Pngimagelist {
class TPngImageList;
class TPngImageCollectionItem;
}
using namespace Pngimagelist;

TPngImageList * GetAnimationsImages(TControl * Control);
TImageList * GetButtonImages(TControl * Control);
TPngImageList * GetDialogImages(TControl * Control);
void ReleaseImagesModules();

class TFrameAnimation
{
public:
  TFrameAnimation();
  void Init(TPaintBox * PaintBox, UnicodeString & Name);
  void Start();
  void Stop();

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

  void DoInit();
  void PaintBoxPaint(TObject * Sender);
  void CalculateNextFrameTick();
  TPngImageCollectionItem * GetCurrentImage();
  void Animate();
  void Timer(TObject * Sender);
  void Repaint();
  void Rescale();
  static void PaintBoxRescale(TComponent * Sender, TObject * Token);
};

class TScreenTipHintWindow : public THintWindow
{
public:
  TScreenTipHintWindow(TComponent * Owner);
  virtual TRect CalcHintRect(int MaxWidth, UnicodeString AHint, void * AData);
  virtual void ActivateHintData(const TRect & Rect, UnicodeString AHint, void * AData);

  static void CalcHintTextRect(TControl * Control, TCanvas * Canvas, TRect & Rect, UnicodeString & Hint);

protected:
  virtual void Paint();
  virtual void Dispatch(void * AMessage);

private:
  bool FParentPainting;
  int FMargin;
  UnicodeString FShortHint;
  UnicodeString FLongHint;
  TControl * FHintControl;
  bool FHintPopup;
  std::unique_ptr<TFont> FScaledHintFont;

  UnicodeString GetLongHintIfAny(UnicodeString & AHint);
  static int GetTextFlags(TControl * Control);

  bool UseBoldShortHint(TControl * HintControl);
  int GetMargin(TControl * HintControl, UnicodeString & Hint);
  TFont * GetFont(TControl * HintControl, UnicodeString & Hint);
  TControl * GetHintControl(void * Data);
  void SplitHint(
    TControl * HintControl, UnicodeString & Hint, UnicodeString & ShortHint, UnicodeString & LongHint);
  void SplitHint(
    TControl * HintControl, UnicodeString & Hint, UnicodeString & ShortHint, UnicodeString & LongHint);
};

// Newer version rich edit that supports "Friendly name hyperlinks" and
// allows wider range of Unicode characters: https://stackoverflow.com/q/47433656/850848
class TNewRichEdit : public TRichEdit
{
public:
  virtual TNewRichEdit(TComponent * AOwner);

protected:
  virtual void CreateParams(TCreateParams & Params);
  virtual void CreateWnd();
  virtual void DestroyWnd();

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

#if 0
// Based on:
// https://stackoverflow.com/q/6912424/850848
NB_CORE_EXPORT extern UnicodeString PageantTool;
NB_CORE_EXPORT extern UnicodeString PuttygenTool;
// https://stackoverflow.com/q/4685863/850848
class TUIStateAwareLabel : public TLabel
{
protected:
  DYNAMIC void DoDrawText(TRect & Rect, int Flags);
};
// FindComponentClass takes parameter by reference and as such it cannot be implemented in
// an inline method without a compiler warning, which we cannot suppress in a macro.
// And having the implementation in a real code (not macro) also allows us to debug the code.
void FindComponentClass(
  void * Data, TReader * Reader, UnicodeString ClassName, TComponentClass & ComponentClass);
#define INTERFACE_HOOK_CUSTOM(PARENT) \
  protected: \
    virtual void ReadState(TReader * Reader) \
    { \
      Reader->OnFindComponentClass = MakeMethod<TFindComponentClassEvent>(nullptr, FindComponentClass); \
      PARENT::ReadState(Reader); \
    }
#define INTERFACE_HOOK INTERFACE_HOOK_CUSTOM(TForm)


#endif // #if 0
