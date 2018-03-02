
#pragma once

#include <Classes.hpp>
#include <FileMasks.h>
//---------------------------------------------------------------------------
class TSessionData;
//---------------------------------------------------------------------------
#if 0
typedef void (__closure *TProcessMessagesEvent)();
#endif // #if 0
typedef nb::FastDelegate0<void> TProcessMessagesEvent;
//---------------------------------------------------------------------------
NB_CORE_EXPORT bool FindFile(UnicodeString &APath);
NB_CORE_EXPORT bool FindTool(const UnicodeString Name, UnicodeString &APath);
NB_CORE_EXPORT void ExecuteShellChecked(const UnicodeString APath, const UnicodeString Params,
  bool ChangeWorkingDirectory = false);
NB_CORE_EXPORT void ExecuteShellChecked(const UnicodeString Command);
NB_CORE_EXPORT bool ExecuteShell(const UnicodeString Path, const UnicodeString Params,
  HANDLE &Handle);
NB_CORE_EXPORT void ExecuteShellCheckedAndWait(const UnicodeString Command, TProcessMessagesEvent ProcessMessages);
NB_CORE_EXPORT bool CopyCommandToClipboard(const UnicodeString Command);
NB_CORE_EXPORT void OpenSessionInPutty(const UnicodeString PuttyPath,
  TSessionData *SessionData);
NB_CORE_EXPORT bool SpecialFolderLocation(intptr_t PathID, UnicodeString &APath);
NB_CORE_EXPORT UnicodeString UniqTempDir(const UnicodeString BaseDir,
  const UnicodeString Identity, bool Mask = false);
NB_CORE_EXPORT bool DeleteDirectory(const UnicodeString ADirName);
NB_CORE_EXPORT UnicodeString ItemsFormatString(const UnicodeString SingleItemFormat,
  const UnicodeString MultiItemsFormat, intptr_t Count, const UnicodeString FirstItem);
NB_CORE_EXPORT UnicodeString GetPersonalFolder();
NB_CORE_EXPORT UnicodeString ItemsFormatString(const UnicodeString SingleItemFormat,
  const UnicodeString MultiItemsFormat, const TStrings *Items);
NB_CORE_EXPORT UnicodeString FileNameFormatString(const UnicodeString SingleFileFormat,
  const UnicodeString MultiFilesFormat, const TStrings *AFiles, bool Remote);
//NB_CORE_EXPORT UnicodeString FormatDateTimeSpan(const UnicodeString TimeFormat, const TDateTime & DateTime);

#if 0

void AddSessionColorImage(TCustomImageList *ImageList, TColor Color, intptr_t MaskIndex);
void SetSubmenu(TTBXCustomItem *Item);
typedef intptr_t (*TCalculateWidth)(const UnicodeString Text, void *Arg);
void ApplyTabs(
  UnicodeString &Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void *CalculateWidthArg);
TPanel *CreateLabelPanel(TPanel *Parent, const UnicodeString Label);
void SelectScaledImageList(TImageList *ImageList);
void CopyImageList(TImageList *TargetList, TImageList *SourceList);
void LoadDialogImage(TImage *Image, const UnicodeString ImageName);
intptr_t DialogImageSize(TForm *Form);
intptr_t NormalizePixelsPerInch(intptr_t PixelsPerInch);
void HideComponentsPanel(TForm *Form);
namespace Webbrowserex {
class TWebBrowserEx;
}
using namespace Webbrowserex;
TWebBrowserEx *CreateBrowserViewer(TPanel *Parent, const UnicodeString LoadingLabel);
void SetBrowserDesignModeOff(TWebBrowserEx *WebBrowser);
void AddBrowserLinkHandler(TWebBrowserEx *WebBrowser,
  UnicodeString Url, TNotifyEvent Handler);
void NavigateBrowserToUrl(TWebBrowserEx *WebBrowser, const UnicodeString Url);
TComponent *FindComponentRecursively(TComponent *Root, const UnicodeString Name);

#endif // #if 0
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TLocalCustomCommand : public TFileCustomCommand
{
public:
  TLocalCustomCommand();
  explicit TLocalCustomCommand(
    const TCustomCommandData &Data, const UnicodeString RemotePath, const UnicodeString LocalPath);
  explicit TLocalCustomCommand(
    const TCustomCommandData &Data, const UnicodeString RemotePath, const UnicodeString LocalPath,
    const UnicodeString AFileName, const UnicodeString LocalFileName,
    const UnicodeString FileList);
  virtual ~TLocalCustomCommand() {}

  virtual bool IsFileCommand(const UnicodeString Command) const;
  bool HasLocalFileName(const UnicodeString Command) const;

protected:
  virtual intptr_t PatternLen(const UnicodeString Command, intptr_t Index) const;
  virtual bool PatternReplacement(intptr_t Index, const UnicodeString Pattern,
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
TPngImageList *GetAnimationsImages(TControl *Control);
TImageList *GetButtonImages(TControl *Control);
TPngImageList *GetDialogImages(TControl *Control);
void ReleaseImagesModules();
//---------------------------------------------------------------------------
class TFrameAnimation
{
public:
  TFrameAnimation();
  void Init(TPaintBox *PaintBox, const UnicodeString Name);
  void Start();
  void Stop();

private:
  UnicodeString FName;
  TPaintBox *FPaintBox;
  TPngImageList *FImageList;
  intptr_t FFirstFrame;
  intptr_t FFirstLoopFrame;
  intptr_t FLastFrame;
  intptr_t FCurrentFrame;
  DWORD FNextFrameTick;
  TTimer *FTimer;
  bool FPainted;

  void DoInit();
  void PaintBoxPaint(TObject *Sender);
  void CalculateNextFrameTick();
  TPngImageCollectionItem *GetCurrentImage();
  void Animate();
  void Timer(TObject *Sender);
  void Repaint();
  void Rescale();
  static void PaintBoxRescale(TComponent *Sender, TObject *Token);
};
//---------------------------------------------------------------------------
class TScreenTipHintWindow : public THintWindow
{
public:
  TScreenTipHintWindow(TComponent *Owner);
  virtual TRect CalcHintRect(intptr_t MaxWidth, const UnicodeString AHint, void *AData);
  virtual void ActivateHintData(const TRect &Rect, const UnicodeString AHint, void *AData);

  static void CalcHintTextRect(TControl *Control, TCanvas *Canvas, TRect &Rect, const UnicodeString Hint);

protected:
  virtual void Paint();
  virtual void Dispatch(void *AMessage);

private:
  bool FParentPainting;
  intptr_t FMargin;
  UnicodeString FShortHint;
  UnicodeString FLongHint;
  TControl *FHintControl;
  bool FHintPopup;
  std::unique_ptr<TFont> FScaledHintFont;

  UnicodeString GetLongHintIfAny(const UnicodeString AHint);
  static intptr_t GetTextFlags(TControl *Control);
  bool IsHintPopup(TControl *HintControl, const UnicodeString Hint);
  bool IsPathLabel(TControl *HintControl);
  bool UseBoldShortHint(TControl *HintControl);
  intptr_t GetMargin(TControl *HintControl, const UnicodeString Hint);
  TFont *GetFont(TControl *HintControl, const UnicodeString Hint);
  TControl *GetHintControl(void *Data);
};
//---------------------------------------------------------------------------
// Newer version rich edit that supports "Friendly name hyperlinks" and
// allows wider range of Unicode characters: https://stackoverflow.com/q/47433656/850848
class TNewRichEdit : public TRichEdit
{
public:
  virtual TNewRichEdit(TComponent * AOwner);

protected:
  virtual void CreateParams(TCreateParams & Params);
  virtual void DestroyWnd();

private:
  HINSTANCE FLibrary;
};
#endif // #if 0
//---------------------------------------------------------------------------
NB_CORE_EXPORT extern const UnicodeString PageantTool;
NB_CORE_EXPORT extern const UnicodeString PuttygenTool;
//---------------------------------------------------------------------------
