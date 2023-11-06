
#pragma once

#include <comctrls.hpp>
#include <WinInterface.h>
__removed #include <HelpIntfs.hpp>
#include <stdio.h>
#include <SessionData.h>
__removed #include <Vcl.Graphics.hpp>

#if 0
void CenterFormOn(TForm * Form, TControl * CenterOn);
void ExecuteProcessAndReadOutput(const UnicodeString & Command, UnicodeString & Output, DWORD & ExitCode, bool ReadStdErr);
void ExecuteProcessChecked(
  const UnicodeString & Command, const UnicodeString & HelpKeyword, UnicodeString * Output);
void ExecuteProcessCheckedAndWait(
  const UnicodeString & Command, const UnicodeString & HelpKeyword, UnicodeString * Output);
bool IsKeyPressed(int VirtualKey);
bool UseAlternativeFunction();
bool OpenInNewWindow();
void ExecuteSelf(const UnicodeString & Params);
void ExecuteNewInstance(const UnicodeString & Param, const UnicodeString & AdditionalParams = UnicodeString());
IShellLink * CreateAppDesktopShortCut(
  const UnicodeString & Name, const UnicodeString & Params, const UnicodeString & Description,
  int SpecialFolder = -1, int IconIndex = 0, bool Return = false);
IShellLink * CreateDesktopSessionShortCut(
  const UnicodeString & SessionName, UnicodeString Name,
  const UnicodeString & AdditionalParams,
  int SpecialFolder = -1, int IconIndex = SITE_ICON, bool Return = false);
UnicodeString GetListViewStr(TListView * ListView);
void LoadListViewStr(TListView * ListView, UnicodeString LayoutStr);
void RestoreForm(UnicodeString Data, TForm * Form, bool PositionOnly = false);
UnicodeString StoreForm(TCustomForm * Form);
void RestoreFormSize(UnicodeString Data, TForm * Form);
UnicodeString StoreFormSize(TForm * Form);
TFontStyles IntToFontStyles(int value);
int FontStylesToInt(const TFontStyles value);
bool SameFont(TFont * Font1, TFont * Font2);
TColor GetWindowTextColor(TColor BackgroundColor, TColor Color = static_cast<TColor>(0));
TColor GetWindowColor(TColor Color = static_cast<TColor>(0));
TColor GetBtnFaceColor();
TColor GetNonZeroColor(TColor Color);
void ValidateMask(const UnicodeString & Mask, int ForceDirectoryMasks = -1);
void ValidateMaskEdit(TComboBox * Edit);
void ValidateMaskEdit(TEdit * Edit);
void ValidateMaskEdit(TMemo * Edit, bool Directory);
bool IsWinSCPUrl(const UnicodeString & Url);
UnicodeString SecureUrl(const UnicodeString & Url);
void OpenBrowser(UnicodeString URL);
void OpenFileInExplorer(const UnicodeString & Path);
void OpenFolderInExplorer(const UnicodeString & Path);
void ShowHelp(const UnicodeString & HelpKeyword);
#endif // #if 0
NB_CORE_EXPORT bool IsFormatInClipboard(unsigned int Format);
NB_CORE_EXPORT bool NonEmptyTextFromClipboard(UnicodeString & Text);
NB_CORE_EXPORT HANDLE OpenTextFromClipboard(const wchar_t *& Text);
NB_CORE_EXPORT void CloseTextFromClipboard(HANDLE Handle);
#if 0
void ExitActiveControl(TForm * Form);
UnicodeString ReadResource(const UnicodeString ResName);
bool DumpResourceToFile(const UnicodeString ResName,
  const UnicodeString FileName);
void BrowseForExecutable(TEdit * Control, UnicodeString Title,
  UnicodeString Filter, bool FileNameCommand, bool Escape);
void BrowseForExecutable(TComboBox * Control, UnicodeString Title,
  UnicodeString Filter, bool FileNameCommand, bool Escape);
bool FontDialog(TFont * Font);
#endif // #if 0

NB_CORE_EXPORT bool SaveDialog(const UnicodeString & Title, const UnicodeString & Filter,
  const UnicodeString & DefaultExt, UnicodeString & FileName);
NB_CORE_EXPORT bool AutodetectProxy(UnicodeString & HostName, int32_t & PortNumber);
NB_CORE_EXPORT bool IsWin64();
NB_CORE_EXPORT void CopyToClipboard(const UnicodeString & Text);
NB_CORE_EXPORT void CopyToClipboard(TStrings * Strings);
NB_CORE_EXPORT void ShutDownWindows();
NB_CORE_EXPORT void SuspendWindows();
NB_CORE_EXPORT void EditSelectBaseName(HWND Edit);
NB_CORE_EXPORT UnicodeString GetConvertedKeyFileName(const UnicodeString & FileName);
struct TPrivateKey;
UnicodeString AddMatchingKeyCertificate(TPrivateKey * PrivateKey, const UnicodeString & FileName);
NB_CORE_EXPORT void VerifyAndConvertKey(UnicodeString & FileName, bool CanIgnore);
NB_CORE_EXPORT void VerifyKey(const UnicodeString & FileName);
NB_CORE_EXPORT void VerifyCertificate(const UnicodeString & FileName);
#if 0
NB_CORE_EXPORT TStrings * GetUnwrappedMemoLines(TMemo * Memo);
NB_CORE_EXPORT bool DetectSystemExternalEditor(
  bool AllowDefaultEditor,
  UnicodeString & Executable, UnicodeString & ExecutableDescription,
  UnicodeString & UsageState, bool & TryNextTime);

#define IUNKNOWN \
  virtual HRESULT __stdcall QueryInterface(const GUID& IID, void **Obj) \
  { \
    return TInterfacedObject::QueryInterface(IID, (void *)Obj); \
  } \
  \
  virtual ULONG __stdcall AddRef() \
  { \
    return TInterfacedObject::_AddRef(); \
  } \
  \
  virtual ULONG __stdcall Release() \
  { \
    return TInterfacedObject::_Release(); \
  }

void InitializeCustomHelp(ICustomHelpViewer * HelpViewer);
void FinalizeCustomHelp();

#endif // #if 0
