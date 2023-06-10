
#pragma once

#include <Classes.hpp>
#include <FileMasks.h>
#include <DirectoryMonitor.hpp>

class TSessionData;

#if 0
typedef void (__closure *TProcessMessagesEvent)();
#endif // #if 0
using TProcessMessagesEvent = nb::FastDelegate0<void>;

void GUIFinalize();
NB_CORE_EXPORT bool FindFile(UnicodeString & APath);
NB_CORE_EXPORT bool FindTool(const UnicodeString & AName, UnicodeString & APath);
NB_CORE_EXPORT void ExecuteTool(const UnicodeString & AName);
NB_CORE_EXPORT void ExecuteShellChecked(const UnicodeString APath, const UnicodeString AParams,
  bool ChangeWorkingDirectory = false);
NB_CORE_EXPORT void ExecuteShellChecked(const UnicodeString ACommand);
NB_CORE_EXPORT bool ExecuteShell(const UnicodeString APath, const UnicodeString AParams,
  HANDLE & Handle);
NB_CORE_EXPORT void ExecuteShellCheckedAndWait(const UnicodeString ACommand, TProcessMessagesEvent ProcessMessages);
__removed TObjectList * StartCreationDirectoryMonitorsOnEachDrive(uint32_t Filter, TFileChangedEvent OnChanged);
extern bool DontCopyCommandToClipboard;
bool CopyCommandToClipboard(const UnicodeString & Command);
bool DoesSessionExistInPutty(const UnicodeString & StorageKey);
bool ExportSessionToPutty(TSessionData * SessionData, bool ReuseExisting, const UnicodeString & SessionName);
void OpenSessionInPutty(TSessionData * SessionData);
NB_CORE_EXPORT void OpenSessionInPutty(const UnicodeString APuttyPath,
  TSessionData * SessionData);
NB_CORE_EXPORT bool SpecialFolderLocation(int32_t APathID, UnicodeString & APath);
NB_CORE_EXPORT UnicodeString UniqTempDir(const UnicodeString ABaseDir,
  const UnicodeString Identity, bool Mask = false);
NB_CORE_EXPORT bool DeleteDirectory(const UnicodeString ADirName);

#if 0

int GetSessionColorImage(TCustomImageList * ImageList, TColor Color, int MaskIndex);
void RegenerateSessionColorsImageList(TCustomImageList * ImageList, int MaskIndex);
void SetSubmenu(TTBXCustomItem * Item, bool Enable);
typedef int (*TCalculateWidth)(UnicodeString Text, void * Arg);
void ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg);
TPanel * CreateLabelPanel(TPanel * Parent, const UnicodeString & Label);
void SelectScaledImageList(TImageList * ImageList);
void CopyImageList(TImageList * TargetList, TImageList * SourceList);
void LoadDialogImage(TImage * Image, const UnicodeString & ImageName);
int DialogImageSize(TForm * Form);
int NormalizePixelsPerInch(int PixelsPerInch);
void HideComponentsPanel(TForm * Form);
UnicodeString FormatIncrementalSearchStatus(const UnicodeString & Text, bool HaveNext);
namespace Webbrowserex
{
  class TWebBrowserEx;
}
using namespace Webbrowserex;
TWebBrowserEx * CreateBrowserViewer(TPanel * Parent, const UnicodeString & LoadingLabel);
void SetBrowserDesignModeOff(TWebBrowserEx * WebBrowser);
void AddBrowserLinkHandler(TWebBrowserEx * WebBrowser,
  const UnicodeString & Url, TNotifyEvent Handler);
void NavigateBrowserToUrl(TWebBrowserEx * WebBrowser, const UnicodeString & Url);
void ReadyBrowserForStreaming(TWebBrowserEx * WebBrowser);
void WaitBrowserToIdle(TWebBrowserEx * WebBrowser);
void HideBrowserScrollbars(TWebBrowserEx * WebBrowser);
bool CopyTextFromBrowser(TWebBrowserEx * WebBrowser, UnicodeString & Text);
UnicodeString GenerateAppHtmlPage(TFont * Font, TPanel * Parent, const UnicodeString & Body, bool Seamless);
void LoadBrowserDocument(TWebBrowserEx * WebBrowser, const UnicodeString & Document);
TComponent * FindComponentRecursively(TComponent * Root, const UnicodeString & Name);
void GetInstrutionsTheme(
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont);
bool CanShowTimeEstimate(TDateTime StartTime);

#endif // #if 0

class NB_CORE_EXPORT TLocalCustomCommand : public TFileCustomCommand
{
public:
  TLocalCustomCommand() noexcept;
  explicit TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath) noexcept;
  explicit TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath,
    const UnicodeString & AFileName, const UnicodeString & LocalFileName,
    const UnicodeString & FileList) noexcept;
  virtual ~TLocalCustomCommand() = default;

  virtual bool IsFileCommand(const UnicodeString & ACommand) const;
  bool HasLocalFileName(const UnicodeString & ACommand) const;

protected:
  virtual int32_t PatternLen(const UnicodeString & Command, int32_t Index) const;
  virtual bool PatternReplacement(int32_t Index, const UnicodeString & Pattern,
    const UnicodeString & Replacement, bool & Delimit) const;
  virtual void DelimitReplacement(UnicodeString & Replacement, wchar_t Quote);

private:
  UnicodeString FLocalPath;
  UnicodeString FLocalFileName;
};

#if 0

namespace Pngimagelist
{
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
  void Init(TPaintBox * PaintBox, const UnicodeString & Name);
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
  virtual TRect CalcHintRect(int MaxWidth, const UnicodeString AHint, void * AData);
  virtual void ActivateHintData(const TRect & Rect, const UnicodeString AHint, void * AData);

  static void CalcHintTextRect(TControl * Control, TCanvas * Canvas, TRect & Rect, const UnicodeString & Hint);

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

  UnicodeString GetLongHintIfAny(const UnicodeString & AHint);
  static int GetTextFlags(TControl * Control);
  bool IsPathLabel(TControl * HintControl);
  bool UseBoldShortHint(TControl * HintControl);
  int GetMargin(TControl * HintControl, const UnicodeString & Hint);
  TFont * GetFont(TControl * HintControl, const UnicodeString & Hint);
  TControl * GetHintControl(void * Data);
  void SplitHint(
    TControl * HintControl, const UnicodeString & Hint, UnicodeString & ShortHint, UnicodeString & LongHint);
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
  UnicodeString MultiItemsFormat, int32_t Count, UnicodeString FirstItem);
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
// https://stackoverflow.com/q/4685863/850848
class TUIStateAwareLabel : public TLabel
{
public:
  virtual TUIStateAwareLabel(TComponent * AOwner);
  virtual void Dispatch(void * AMessage);

protected:
  DYNAMIC void DoDrawText(TRect & Rect, int Flags);
};
// FindComponentClass takes parameter by reference and as such it cannot be implemented in
// an inline method without a compiler warning, which we cannot suppress in a macro.
// And having the implementation in a real code (not macro) also allows us to debug the code.
void FindComponentClass(
  void * Data, TReader * Reader, const UnicodeString ClassName, TComponentClass & ComponentClass);
#define INTERFACE_HOOK_CUSTOM(PARENT) \
  protected: \
    virtual void ReadState(TReader * Reader) \
    { \
      Reader->OnFindComponentClass = MakeMethod<TFindComponentClassEvent>(nullptr, FindComponentClass); \
      PARENT::ReadState(Reader); \
    }
#define INTERFACE_HOOK INTERFACE_HOOK_CUSTOM(TForm)


#endif // #if 0

NB_CORE_EXPORT extern const UnicodeString PageantTool;
NB_CORE_EXPORT extern const UnicodeString PuttygenTool;
