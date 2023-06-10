
#ifndef CustomWinConfigurationH
#define CustomWinConfigurationH

#include "GUIConfiguration.h"
#define WM_WINSCP_USER   (WM_USER + 0x2000)
// WM_USER_STOP = WM_WINSCP_USER + 2 (in forms/Synchronize.cpp)
// WM_INTERUPT_IDLE = WM_WINSCP_USER + 3 (in windows/ConsoleRunner.cpp)
// WM_COMPONENT_HIDE = WM_WINSCP_USER + 4 (forms/CustomScpExplorer.cpp)
// WM_TRAY_ICON = WM_WINSCP_USER + 5 (forms/CustomScpExplorer.cpp)
// WM_WINSCP_USER + 6 was WM_LOG_UPDATE
#define WM_MANAGES_CAPTION (WM_WINSCP_USER + 7)
#define WM_WANTS_MOUSEWHEEL (WM_WINSCP_USER + 8)
#define WM_CAN_DISPLAY_UPDATES (WM_WINSCP_USER + 9)
// CM_DPICHANGED + 10 (packages/my/PasTools.pas)
#define WM_WANTS_MOUSEWHEEL_INACTIVE (WM_WINSCP_USER + 11)
#define WM_WANTS_SCREEN_TIPS (WM_WINSCP_USER + 12)
// WM_USER_SHCHANGENOTIFY + 13 (packages/filemng/DriveView.pas)
// WM_PASTE_FILES + 14 (forms/CustomScpExplorer.cpp)

#if 0

#define C(Property) (Property != rhc.Property) ||
struct TSynchronizeChecklistConfiguration
{
  UnicodeString WindowParams;
  UnicodeString ListParams;
  bool operator !=(TSynchronizeChecklistConfiguration & rhc)
    { return C(WindowParams) C(ListParams) 0; };
};
typedef TSynchronizeChecklistConfiguration TFindFileConfiguration;

struct TConsoleWinConfiguration
{
  UnicodeString WindowSize;
  bool operator !=(TConsoleWinConfiguration & rhc)
    { return C(WindowSize) 0; };
};

enum TIncrementalSearch { isOff = -1, isNameStartOnly, isName, isAll };

struct TLoginDialogConfiguration : public TConsoleWinConfiguration
{
  TIncrementalSearch SiteSearch;
  bool operator !=(TLoginDialogConfiguration & rhc)
    { return (TConsoleWinConfiguration::operator !=(rhc)) || C(SiteSearch) 0; };
};

#undef C

class TCustomWinConfiguration : public TGUIConfiguration
{
static const int MaxHistoryCount = 50;
private:
  TInterface FInterface;
  TInterface FAppliedInterface;
  TStringList * FHistory;
  TStrings * FEmptyHistory;
  TSynchronizeChecklistConfiguration FSynchronizeChecklist;
  TFindFileConfiguration FFindFile;
  TConsoleWinConfiguration FConsoleWin;
  TLoginDialogConfiguration FLoginDialog;
  TInterface FDefaultInterface;
  bool FCanApplyInterfaceImmediately;
  bool FConfirmExitOnCompletion;
  bool FSynchronizeSummary;
  UnicodeString FSessionColors;
  UnicodeString FFontColors;
  bool FCopyShortCutHintShown;
  bool FHttpForWebDAV;
  TNotifyEvent FOnMasterPasswordRecrypt;
  UnicodeString FDefaultFixedWidthFontName;
  int FDefaultFixedWidthFontSize;

  void SetInterface(TInterface value);
  void SetHistory(const UnicodeString Index, TStrings * value);
  TStrings * GetHistory(const UnicodeString Index);
  void SetSynchronizeChecklist(TSynchronizeChecklistConfiguration value);
  void SetFindFile(TFindFileConfiguration value);
  void SetConsoleWin(TConsoleWinConfiguration value);
  void SetLoginDialog(TLoginDialogConfiguration value);
  void SetConfirmExitOnCompletion(bool value);
  void SetSynchronizeSummary(bool value);
  UnicodeString GetDefaultFixedWidthFontName();
  int GetDefaultFixedWidthFontSize();

protected:
  virtual void SaveData(THierarchicalStorage * Storage, bool All);
  virtual void LoadData(THierarchicalStorage * Storage);
  virtual void LoadAdmin(THierarchicalStorage * Storage);
  virtual void Saved();
  void ClearHistory();
  virtual void DefaultHistory();
  void RecryptPasswords(TStrings * RecryptPasswordErrors);
  virtual bool GetUseMasterPassword() = 0;
  UnicodeString FormatDefaultWindowParams(int Width, int Height);
  UnicodeString FormatDefaultWindowSize(int Width, int Height);

public:
  TCustomWinConfiguration();
  virtual ~TCustomWinConfiguration();
  virtual void Default();
  virtual void AskForMasterPasswordIfNotSet() = 0;
  void AskForMasterPasswordIfNotSetAndNeededToPersistSessionData(TSessionData * SessionData);
  static UnicodeString GetValidHistoryKey(UnicodeString Key);

  __property TInterface Interface = { read = FInterface, write = SetInterface };
  __property TInterface AppliedInterface = { read = FAppliedInterface, write = FAppliedInterface };
  __property bool CanApplyInterfaceImmediately = { read = FCanApplyInterfaceImmediately, write = FCanApplyInterfaceImmediately };
  __property TStrings * History[UnicodeString Name] = { read = GetHistory, write = SetHistory };
  __property TSynchronizeChecklistConfiguration SynchronizeChecklist = { read = FSynchronizeChecklist, write = SetSynchronizeChecklist };
  __property TFindFileConfiguration FindFile = { read = FFindFile, write = SetFindFile };
  __property TConsoleWinConfiguration ConsoleWin = { read = FConsoleWin, write = SetConsoleWin };
  __property TLoginDialogConfiguration LoginDialog = { read = FLoginDialog, write = SetLoginDialog };
  __property bool ConfirmExitOnCompletion  = { read=FConfirmExitOnCompletion, write=SetConfirmExitOnCompletion };
  __property bool SynchronizeSummary  = { read = FSynchronizeSummary, write = SetSynchronizeSummary };
  __property UnicodeString SessionColors  = { read=FSessionColors, write=FSessionColors };
  __property UnicodeString FontColors  = { read=FFontColors, write=FFontColors };
  __property bool CopyShortCutHintShown  = { read=FCopyShortCutHintShown, write=FCopyShortCutHintShown };
  __property bool UseMasterPassword = { read = GetUseMasterPassword };
  __property bool HttpForWebDAV = { read = FHttpForWebDAV, write = FHttpForWebDAV };
  __property TNotifyEvent OnMasterPasswordRecrypt = { read = FOnMasterPasswordRecrypt, write = FOnMasterPasswordRecrypt };
  __property UnicodeString DefaultFixedWidthFontName = { read = GetDefaultFixedWidthFontName };
  __property int DefaultFixedWidthFontSize = { read = GetDefaultFixedWidthFontSize };
};

extern TCustomWinConfiguration * CustomWinConfiguration;

#endif // #if 0

#endif
