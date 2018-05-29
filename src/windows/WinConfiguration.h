
//---------------------------------------------------------------------------
#ifndef WinConfigurationH
#define WinConfigurationH
//---------------------------------------------------------------------------
#include "CustomWinConfiguration.h"
#if 0
#include "CustomDirView.hpp"
//---------------------------------------------------------------------------
enum TEditor { edInternal, edExternal, edOpen };
enum TGenerateUrlCodeTarget { guctUrl, guctScript, guctAssembly };
enum TScriptFormat { sfScriptFile, sfBatchFile, sfCommandLine };
enum TLocaleFlagOverride { lfoLanguageIfRecommended, lfoLanguage, lfoAlways, lfoNever };
//---------------------------------------------------------------------------
#define C(Property) (Property != rhc.Property) ||
struct TScpExplorerConfiguration
{
  UnicodeString WindowParams;
  UnicodeString DirViewParams;
  UnicodeString ToolbarsLayout;
  bool SessionsTabs;
  bool StatusBar;
  UnicodeString LastLocalTargetDirectory;
  int ViewStyle;
  bool ShowFullAddress;
  bool DriveView;
  int DriveViewWidth;
  int DriveViewWidthPixelsPerInch;
  bool operator!=(TScpExplorerConfiguration &rhc)
  {
    return C(WindowParams) C(DirViewParams) C(ToolbarsLayout)
      C(SessionsTabs) C(StatusBar)
      C(LastLocalTargetDirectory) C(ViewStyle) C(ShowFullAddress)
      C(DriveView) C(DriveViewWidth) C(DriveViewWidthPixelsPerInch) 0;
  };
};
//---------------------------------------------------------------------------
struct TScpCommanderPanelConfiguration
{
  UnicodeString DirViewParams;
  bool StatusBar;
  bool DriveView;
  int DriveViewHeight;
  int DriveViewHeightPixelsPerInch;
  int DriveViewWidth;
  int DriveViewWidthPixelsPerInch;
  bool operator!=(TScpCommanderPanelConfiguration &rhc)
  {
    return C(DirViewParams) C(StatusBar)
      C(DriveView) C(DriveViewHeight) C(DriveViewHeightPixelsPerInch)
      C(DriveViewWidth) C(DriveViewWidthPixelsPerInch) 0;
  };
};
//---------------------------------------------------------------------------
struct TScpCommanderConfiguration
{
  UnicodeString WindowParams;
  double LocalPanelWidth;
  UnicodeString ToolbarsLayout;
  bool SessionsTabs;
  bool StatusBar;
  TOperationSide CurrentPanel;
  TNortonLikeMode NortonLikeMode;
  bool PreserveLocalDirectory;
  TScpCommanderPanelConfiguration LocalPanel;
  TScpCommanderPanelConfiguration RemotePanel;
  bool CompareByTime;
  bool CompareBySize;
  bool SwappedPanels;
  bool TreeOnLeft;
  bool ExplorerKeyboardShortcuts;
  bool SystemContextMenu;
  bool operator!=(TScpCommanderConfiguration &rhc)
  {
    return C(WindowParams) C(LocalPanelWidth) C(ToolbarsLayout)
      C(SessionsTabs) C(StatusBar)
      C(LocalPanel) C(RemotePanel) C(CurrentPanel)
      C(NortonLikeMode) C(PreserveLocalDirectory)
      C(CompareBySize) C(CompareByTime) C(SwappedPanels)
      C(TreeOnLeft) C(ExplorerKeyboardShortcuts) C(SystemContextMenu) 0;
  };

  TCompareCriterias CompareCriterias()
  {
    TCompareCriterias Criterias;
    if (CompareByTime)
    {
      Criterias << ccTime;
    }
    if (CompareBySize)
    {
      Criterias << ccSize;
    }
    return Criterias;
  }
};
//---------------------------------------------------------------------------
struct TFontConfiguration
{
  UnicodeString FontName;
  int FontSize;
  int FontCharset;
  int FontStyle;

  TFontConfiguration()
  {
    FontSize = 0;
    FontCharset = DEFAULT_CHARSET;
    FontStyle = 0;
  }

  // keep in sync with SameFont
  bool operator!=(const TFontConfiguration &rhc)
  {
    return !SameText(FontName, rhc.FontName) || C(FontSize)
      C(FontCharset) C(FontStyle) 0;
  };
};
//---------------------------------------------------------------------------
struct TEditorConfiguration
{
  TFontConfiguration Font;
  TColor FontColor;
  TColor BackgroundColor;
  bool WordWrap;
  UnicodeString FindText;
  UnicodeString ReplaceText;
  bool FindMatchCase;
  bool FindWholeWord;
  bool FindDown;
  unsigned int TabSize;
  unsigned int MaxEditors;
  unsigned int EarlyClose;
  bool SDIShellEditor;
  UnicodeString WindowParams;
  int Encoding;
  bool WarnOnEncodingFallback;
  bool WarnOrLargeFileSize;
  bool operator!=(TEditorConfiguration &rhc)
  {
    return C(Font) C(FontColor) C(BackgroundColor) C(WordWrap) C(FindText) C(ReplaceText)
      C(FindMatchCase) C(FindWholeWord) C(FindDown) C(TabSize)
      C(MaxEditors) C(EarlyClose) C(SDIShellEditor) C(WindowParams)
      C(Encoding) C(WarnOnEncodingFallback) C(WarnOrLargeFileSize) 0;
  };
};
//---------------------------------------------------------------------------
enum TQueueViewShow { qvShow, qvHideWhenEmpty, qvHide };
struct TQueueViewConfiguration
{
  int Height;
  int HeightPixelsPerInch;
  UnicodeString Layout;
  TQueueViewShow Show;
  TQueueViewShow LastHideShow;
  bool ToolBar;
  bool Label;
  bool operator!=(TQueueViewConfiguration &rhc)
  { return C(Height) C(HeightPixelsPerInch) C(Layout) C(Show) C(LastHideShow) C(ToolBar) C(Label) 0; };
};
//---------------------------------------------------------------------------
struct TUpdatesData
{
  int ForVersion;
  int Version;
  UnicodeString Message;
  bool Critical;
  UnicodeString Release;
  bool Disabled;
  UnicodeString Url;
  UnicodeString UrlButton;
  UnicodeString NewsUrl;
  TSize NewsSize;
  UnicodeString DownloadUrl;
  __int64 DownloadSize;
  UnicodeString DownloadSha256;
  UnicodeString AuthenticationError;
  bool OpenGettingStarted;
  UnicodeString DownloadingUrl;
  TSize TipsSize;
  UnicodeString TipsUrl;
  UnicodeString Tips;
  int TipsIntervalDays;
  int TipsIntervalRuns;
  bool operator!=(TUpdatesData &rhc)
  {
    return C(ForVersion) C(Version) C(Message) C(Critical) C(Release)
      C(Disabled) C(Url) C(UrlButton) C(NewsUrl) C(NewsSize)
      C(DownloadUrl) C(DownloadSize) C(DownloadSha256) C(AuthenticationError)
      C(OpenGettingStarted) C(DownloadingUrl)
      C(TipsSize) C(TipsUrl) C(Tips) C(TipsIntervalDays) C(TipsIntervalRuns) 0;
  };
  void Reset()
  {
    ForVersion = 0;
    Version = 0;
    Message = L"";
    Critical = false;
    Release = L"";
    Disabled = false;
    Url = L"";
    UrlButton = L"";
    NewsUrl = L"";
    NewsSize = TSize();
    DownloadUrl = L"";
    DownloadSize = 0;
    DownloadSha256 = L"";
    AuthenticationError = L"";
    OpenGettingStarted = false;
    DownloadingUrl = L"";
    TipsSize = TSize();
    TipsUrl = L"";
    Tips = L"";
    TipsIntervalDays = 7;
    TipsIntervalRuns = 5;
  }
};
//---------------------------------------------------------------------------
enum TConnectionType { ctDirect, ctAuto, ctProxy };
//---------------------------------------------------------------------------
struct TUpdatesConfiguration
{
  TDateTime Period;
  TDateTime LastCheck;
  TConnectionType ConnectionType;
  UnicodeString ProxyHost;
  int ProxyPort;
  TAutoSwitch BetaVersions;
  bool ShowOnStartup;
  UnicodeString AuthenticationEmail;
  bool HaveResults;
  bool ShownResults;
  UnicodeString DotNetVersion;
  UnicodeString ConsoleVersion;
  TUpdatesData Results;

  bool operator!=(TUpdatesConfiguration &rhc)
  {
    return C(Period) C(LastCheck) C(ConnectionType) C(ProxyHost) C(ProxyPort)
      C(BetaVersions) C(ShowOnStartup) C(AuthenticationEmail)
      C(HaveResults) C(ShownResults) C(DotNetVersion)
      C(ConsoleVersion) C(Results)  0;
  };

  bool HaveValidResultsForVersion(int CompoundVersion)
  {
    return
      HaveResults &&
      (double(Period) > 0) &&
      (Results.ForVersion == CompoundVersion);
  }
};
//---------------------------------------------------------------------------
struct TEditorData
{
  TEditorData();
  TEditorData(const TEditorData &Source);

  TFileMasks FileMask;
  TEditor Editor;
  UnicodeString ExternalEditor;
  bool ExternalEditorText;
  bool SDIExternalEditor;
  bool DetectMDIExternalEditor;

  bool operator==(const TEditorData &rhd) const;
  void DecideExternalEditorText();
  static bool DecideExternalEditorText(const UnicodeString ExternalEditor);
};
//---------------------------------------------------------------------------
#undef C
//---------------------------------------------------------------------------
class TEditorPreferences
{
public:
  TEditorPreferences();
  TEditorPreferences(const TEditorData &Data);
  bool Matches(const UnicodeString FileName, bool Local,
    const TFileMasks::TParams &Params) const;
  void Load(THierarchicalStorage *Storage, bool Legacy);
  void Save(THierarchicalStorage *Storage) const;
  void LegacyDefaults();
  UnicodeString ExtractExternalEditorName() const;

  static UnicodeString GetDefaultExternalEditor();

  bool operator==(const TEditorPreferences &rhp) const;

  __property const TEditorData *Data = { read = GetConstData };
  __property UnicodeString Name = { read = GetName };

  TEditorData *GetData();

private:
  TEditorData FData;
  mutable UnicodeString FName;

  UnicodeString GetName() const;
  const TEditorData *GetConstData() const { return &FData; };
};
//---------------------------------------------------------------------------
class TEditorList
{
public:
  TEditorList();
  virtual ~TEditorList();

  const TEditorPreferences *Find(const UnicodeString FileName,
    bool Local, const TFileMasks::TParams &Params) const;

  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage) const;

  TEditorList &operator=(const TEditorList &rhl);
  bool operator==(const TEditorList &rhl) const;

  void Clear();
  void Add(TEditorPreferences *Editor);
  void Insert(int Index, TEditorPreferences *Editor);
  void Change(int Index, TEditorPreferences *Editor);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);
  void Saved();

  bool IsDefaultList() const;

  __property int Count = { read = GetCount };
  __property const TEditorPreferences *Editors[int Index] = { read = GetEditor };
  __property bool Modified = { read = FModified };

private:
  TList *FEditors;
  bool FModified;

  int GetCount() const;

  void Init();
  void Modify();
  const TEditorPreferences *GetEditor(int Index) const;
};
//---------------------------------------------------------------------------
class TBookmarks;
class TBookmarkList;
class TCustomCommandList;
enum TPathInCaption { picShort, picFull, picNone };
// constants must be compatible with legacy CopyOnDoubleClick
enum TDoubleClickAction { dcaOpen = 0, dcaCopy = 1, dcaEdit = 2 };
//---------------------------------------------------------------------------
typedef void (__closure *TMasterPasswordPromptEvent)();
//---------------------------------------------------------------------------
class TWinConfiguration : public TCustomWinConfiguration
{
private:
  UnicodeString FAutoStartSession;
  TDoubleClickAction FDoubleClickAction;
  bool FCopyOnDoubleClickConfirmation;
  bool FDDAllowMove;
  bool FDDAllowMoveInit;
  TAutoSwitch FDDTransferConfirmation;
  bool FDeleteToRecycleBin;
  bool FDimmHiddenFiles;
  bool FRenameWholeName;
  TScpCommanderConfiguration FScpCommander;
  TScpExplorerConfiguration FScpExplorer;
  bool FSelectDirectories;
  UnicodeString FSelectMask;
  bool FShowHiddenFiles;
  TFormatBytesStyle FFormatSizeBytes;
  bool FShowInaccesibleDirectories;
  bool FConfirmTransferring;
  bool FConfirmDeleting;
  bool FConfirmRecycling;
  bool FUseLocationProfiles;
  bool FUseSharedBookmarks;
  UnicodeString FDDTemporaryDirectory;
  bool FDDWarnLackOfTempSpace;
  bool FDDExtEnabled;
  bool FDDFakeFile;
  int FDDExtInstalled;
  int FDDExtTimeout;
  bool FConfirmClosingSession;
  double FDDWarnLackOfTempSpaceRatio;
  UnicodeString FTemporarySessionFile;
  UnicodeString FTemporaryKeyFile;
  TBookmarks *FBookmarks;
  TCustomCommandList *FCustomCommandList;
  TCustomCommandList *FExtensionList;
  UnicodeString FExtensionsDeleted;
  UnicodeString FExtensionsOrder;
  bool FCustomCommandsDefaults;
  TEditorConfiguration FEditor;
  TQueueViewConfiguration FQueueView;
  bool FEnableQueueByDefault;
  bool FEmbeddedSessions;
  bool FExpertMode;
  bool FDisableOpenEdit;
  bool FDefaultDirIsHome;
  int FDDDeleteDelay;
  bool FTemporaryDirectoryAppendSession;
  bool FTemporaryDirectoryAppendPath;
  bool FTemporaryDirectoryDeterministic;
  bool FTemporaryDirectoryCleanup;
  bool FConfirmTemporaryDirectoryCleanup;
  UnicodeString FDefaultTranslationFile;
  UnicodeString FInvalidDefaultTranslationMessage;
  bool FPreservePanelState;
  UnicodeString FLastStoredSession;
  UnicodeString FLastWorkspace;
  bool FAutoSaveWorkspace;
  bool FAutoSaveWorkspacePasswords;
  UnicodeString FAutoWorkspace;
  TPathInCaption FPathInCaption;
  bool FMinimizeToTray;
  bool FMinimizeToTrayOnce;
  bool FBalloonNotifications;
  unsigned int FNotificationsTimeout;
  unsigned int FNotificationsStickTime;
  TUpdatesConfiguration FUpdates;
  UnicodeString FVersionHistory;
  bool FCopyParamAutoSelectNotice;
  bool FLockToolbars;
  bool FSelectiveToolbarText;
  TEditorList *FEditorList;
  TEditorPreferences *FLegacyEditor;
  UnicodeString FDefaultKeyFile;
  bool FAutoOpenInPutty;
  TDateTime FDefaultUpdatesPeriod;
  bool FRefreshRemotePanel;
  TDateTime FRefreshRemotePanelInterval;
  TFontConfiguration FPanelFont;
  bool FNaturalOrderNumericalSorting;
  bool FFullRowSelect;
  bool FOfferedEditorAutoConfig;
  bool FUseMasterPassword;
  UnicodeString FPlainMasterPasswordEncrypt;
  UnicodeString FPlainMasterPasswordDecrypt;
  UnicodeString FMasterPasswordVerifier;
  TMasterPasswordPromptEvent FOnMasterPasswordPrompt;
  UnicodeString FOpenedStoredSessionFolders;
  bool FAutoImportedFromPuttyOrFilezilla;
  int FGenerateUrlComponents;
  TGenerateUrlCodeTarget FGenerateUrlCodeTarget;
  TScriptFormat FGenerateUrlScriptFormat;
  TAssemblyLanguage FGenerateUrlAssemblyLanguage;
  bool FExternalSessionInExistingInstance;
  bool FKeepOpenWhenNoSession;
  bool FLocalIconsByExt;
  TLocaleFlagOverride FBidiModeOverride;
  TLocaleFlagOverride FFlipChildrenOverride;
  bool FShowTips;
  UnicodeString FTipsSeen;
  TDateTime FTipsShown;
  int FRunsSinceLastTip;
  bool FLockedInterface;
  int FDontDecryptPasswords;
  int FMasterPasswordSession;
  bool FMasterPasswordSessionAsked;
  std::unique_ptr<TStringList> FCustomCommandOptions;
  bool FCustomCommandOptionsModified;
  int FLastMachineInstallations;
  __property int LastMachineInstallations = { read = FLastMachineInstallations, write = FLastMachineInstallations };
  int FMachineInstallations;
  LCID FDefaultLocale;
  std::unique_ptr<TStrings> FExtensionTranslations;

  void SetDoubleClickAction(TDoubleClickAction value);
  void SetCopyOnDoubleClickConfirmation(bool value);
  void SetDDAllowMove(bool value);
  void SetDDAllowMoveInit(bool value);
  void SetDDTransferConfirmation(TAutoSwitch value);
  void SetDeleteToRecycleBin(bool value);
  void SetDimmHiddenFiles(bool value);
  void SetRenameWholeName(bool value);
  void SetScpCommander(TScpCommanderConfiguration value);
  void SetScpExplorer(TScpExplorerConfiguration value);
  void SetSelectDirectories(bool value);
  void SetShowHiddenFiles(bool value);
  void SetFormatSizeBytes(TFormatBytesStyle value);
  void SetShowInaccesibleDirectories(bool value);
  void SetConfirmTransferring(bool value);
  void SetConfirmDeleting(bool value);
  void SetConfirmRecycling(bool value);
  void SetUseLocationProfiles(bool value);
  void SetUseSharedBookmarks(bool value);
  void SetDDTemporaryDirectory(const UnicodeString value);
  void SetDDWarnLackOfTempSpace(bool value);
  void SetDDExtEnabled(bool value);
  bool GetDDExtEnabled() const;
  void SetDDExtTimeout(int value);
  void SetConfirmClosingSession(bool value);
  void SetDDWarnLackOfTempSpaceRatio(double value);
  void SetBookmarks(const UnicodeString Key, TBookmarkList *value);
  TBookmarkList *GetBookmarks(const UnicodeString Key);
  void SetSharedBookmarks(TBookmarkList *value);
  TBookmarkList *GetSharedBookmarks();
  void SetAutoStartSession(const UnicodeString value);
  void SetExpertMode(bool value);
  void SetDefaultDirIsHome(bool value);
  void SetEditor(TEditorConfiguration value);
  void SetQueueView(TQueueViewConfiguration value);
  void SetEnableQueueByDefault(bool value);
  void SetCustomCommandList(TCustomCommandList *value);
  void SetExtensionList(TCustomCommandList *value);
  void SetTemporaryDirectoryAppendSession(bool value);
  void SetTemporaryDirectoryAppendPath(bool value);
  void SetTemporaryDirectoryDeterministic(bool value);
  void SetTemporaryDirectoryCleanup(bool value);
  void SetConfirmTemporaryDirectoryCleanup(bool value);
  void SetPreservePanelState(bool value);
  void SetLastStoredSession(const UnicodeString value);
  void SetAutoSaveWorkspace(bool value);
  void SetAutoSaveWorkspacePasswords(bool value);
  void SetAutoWorkspace(UnicodeString value);
  void SetPathInCaption(TPathInCaption value);
  void SetMinimizeToTray(bool value);
  bool GetMinimizeToTray();
  void SetBalloonNotifications(bool value);
  void SetNotificationsTimeout(unsigned int value);
  void SetNotificationsStickTime(unsigned int value);
  void SetCopyParamAutoSelectNotice(bool value);
  TUpdatesConfiguration GetUpdates();
  void SetUpdates(TUpdatesConfiguration value);
  void SetVersionHistory(UnicodeString value);
  void SetLockToolbars(bool value);
  void SetSelectiveToolbarText(bool value);
  const TEditorList *GetEditorList();
  void SetEditorList(const TEditorList *value);
  void SetAutoOpenInPutty(bool value);
  void SetRefreshRemotePanel(bool value);
  void SetRefreshRemotePanelInterval(TDateTime value);
  void SetPanelFont(const TFontConfiguration &value);
  void SetNaturalOrderNumericalSorting(bool value);
  void SetFullRowSelect(bool value);
  void SetOfferedEditorAutoConfig(bool value);
  void SetLastMonitor(int value);
  int GetLastMonitor();
  void SetOpenedStoredSessionFolders(UnicodeString value);
  void SetAutoImportedFromPuttyOrFilezilla(bool value);
  void SetGenerateUrlComponents(int value);
  void SetGenerateUrlCodeTarget(TGenerateUrlCodeTarget value);
  void SetGenerateUrlScriptFormat(TScriptFormat value);
  void SetGenerateUrlAssemblyLanguage(TAssemblyLanguage value);
  void SetExternalSessionInExistingInstance(bool value);
  void SetKeepOpenWhenNoSession(bool value);
  void SetLocalIconsByExt(bool value);
  void SetBidiModeOverride(TLocaleFlagOverride value);
  void SetFlipChildrenOverride(TLocaleFlagOverride value);
  void SetShowTips(bool value);
  void SetTipsSeen(UnicodeString value);
  void SetTipsShown(TDateTime value);
  void SetRunsSinceLastTip(int value);
  bool GetHonorDrivePolicy();
  void SetHonorDrivePolicy(bool value);
  bool GetIsBeta();
  TStrings *GetCustomCommandOptions();
  void SetCustomCommandOptions(TStrings *value);
  void SetLockedInterface(bool value);
  int GetLocaleCompletenessTreshold();

  bool GetDDExtInstalled();
  void AddVersionToHistory();
  bool GetAnyBetaInVersionHistory();
  void PurgePassword(UnicodeString &Password);
  void UpdateEntryInJumpList(
    bool Session, const UnicodeString Name, bool Add);
  TStringList *LoadJumpList(THierarchicalStorage *Storage,
    UnicodeString Name);
  void SaveJumpList(THierarchicalStorage *Storage,
    UnicodeString Name, TStringList *List);
  void TrimJumpList(TStringList *List);
  void UpdateIconFont();

protected:
  virtual TStorage GetStorage();
  virtual void SaveData(THierarchicalStorage *Storage, bool All);
  virtual void LoadData(THierarchicalStorage *Storage);
  virtual void LoadFrom(THierarchicalStorage *Storage);
  virtual void LoadAdmin(THierarchicalStorage *Storage);
  virtual void CopyData(THierarchicalStorage *Source, THierarchicalStorage *Target);
  virtual UnicodeString GetDefaultKeyFile();
  virtual void Saved();
  void RecryptPasswords(TStrings *RecryptPasswordErrors);
  virtual bool GetUseMasterPassword();
  bool SameStringLists(TStrings *Strings1, TStrings *Strings2);
  virtual HINSTANCE LoadNewResourceModule(LCID Locale,
    UnicodeString &FileName);
  void CheckTranslationVersion(const UnicodeString FileName,
    bool InternalLocaleOnError);
  virtual void DefaultLocalized();
  bool DetectRegistryStorage(HKEY RootKey);
  bool CanWriteToStorage();
  bool DoIsBeta(const UnicodeString ReleaseType);
  void AskForMasterPassword();
  void DoLoadExtensionList(const UnicodeString Path, const UnicodeString PathId, TStringList *DeletedExtensions);
  TStrings *GetExtensionsPaths();
  virtual int GetResourceModuleCompleteness(HINSTANCE Module);
  virtual bool IsTranslationComplete(HINSTANCE Module);
  void LoadExtensionList();
  void ReleaseExtensionTranslations();
  void LoadExtensionTranslations();

  __property bool DDFakeFile = { read=FDDFakeFile, write=FDDFakeFile };

public:
  TWinConfiguration();
  virtual ~TWinConfiguration();
  virtual void Default();
  void ClearTemporaryLoginData();
  virtual THierarchicalStorage *CreateScpStorage(bool &SessionList);
  UnicodeString TemporaryDir(bool Mask = false);
  TStrings *FindTemporaryFolders();
  void CleanupTemporaryFolders(TStrings *Folders = nullptr);
  UnicodeString ExpandedTemporaryDirectory();
  void CheckDefaultTranslation();
  const TEditorPreferences *DefaultEditorForFile(
    const UnicodeString FileName, bool Local, const TFileMasks::TParams &MaskParams);
  virtual UnicodeString DecryptPassword(RawByteString Password, const UnicodeString Key);
  virtual RawByteString StronglyRecryptPassword(RawByteString Password, const UnicodeString Key);
  void SetMasterPassword(const UnicodeString value);
  void ChangeMasterPassword(const UnicodeString value, TStrings *RecryptPasswordErrors);
  bool ValidateMasterPassword(const UnicodeString value);
  void ClearMasterPassword(TStrings *RecryptPasswordErrors);
  void BeginMasterPasswordSession();
  void EndMasterPasswordSession();
  virtual void AskForMasterPasswordIfNotSet();
  void AddSessionToJumpList(const UnicodeString SessionName);
  void DeleteSessionFromJumpList(const UnicodeString SessionName);
  void AddWorkspaceToJumpList(const UnicodeString Workspace);
  void DeleteWorkspaceFromJumpList(const UnicodeString Workspace);
  void UpdateJumpList();
  virtual void UpdateStaticUsage();
  void MinimizeToTrayOnce();
  void CustomCommandShortCuts(TShortCuts &ShortCuts) const;
  UnicodeString GetUserExtensionsPath();
  UnicodeString GetExtensionId(const UnicodeString ExtensionPath);
  UnicodeString ExtensionStringTranslation(const UnicodeString ExtensionId, const UnicodeString S);
  UnicodeString UniqueExtensionName(const UnicodeString ExtensionName, int Counter);
  UnicodeString GetProvisionaryExtensionId(const UnicodeString FileName);
  bool __fastcall IsDDExtRunning();

  static void RestoreFont(const TFontConfiguration &Configuration, TFont *Font);
  static void StoreFont(TFont *Font, TFontConfiguration &Configuration);

  __property TScpCommanderConfiguration ScpCommander = { read = FScpCommander, write = SetScpCommander };
  __property TScpExplorerConfiguration ScpExplorer = { read = FScpExplorer, write = SetScpExplorer };
  __property bool SelectDirectories = { read = FSelectDirectories, write = SetSelectDirectories };
  __property UnicodeString SelectMask = { read = FSelectMask, write = FSelectMask };
  __property bool ShowHiddenFiles = { read = FShowHiddenFiles, write = SetShowHiddenFiles };
  __property TFormatBytesStyle FormatSizeBytes = { read = FFormatSizeBytes, write = SetFormatSizeBytes };
  __property bool ShowInaccesibleDirectories = { read = FShowInaccesibleDirectories, write = SetShowInaccesibleDirectories };
  __property TEditorConfiguration Editor = { read = FEditor, write = SetEditor };
  __property TQueueViewConfiguration QueueView = { read = FQueueView, write = SetQueueView };
  __property bool EnableQueueByDefault = { read = FEnableQueueByDefault, write = SetEnableQueueByDefault };
  __property TUpdatesConfiguration Updates = { read = GetUpdates, write = SetUpdates };
  __property UnicodeString VersionHistory = { read = FVersionHistory, write = SetVersionHistory };
  __property bool AnyBetaInVersionHistory = { read = GetAnyBetaInVersionHistory };
  __property bool IsBeta = { read = GetIsBeta };
  __property UnicodeString AutoStartSession = { read = FAutoStartSession, write = SetAutoStartSession };
  __property TDoubleClickAction DoubleClickAction = { read = FDoubleClickAction, write = SetDoubleClickAction };
  __property bool CopyOnDoubleClickConfirmation = { read = FCopyOnDoubleClickConfirmation, write = SetCopyOnDoubleClickConfirmation };
  __property bool DDAllowMove = { read = FDDAllowMove, write = SetDDAllowMove };
  __property bool DDAllowMoveInit = { read = FDDAllowMoveInit, write = SetDDAllowMoveInit };
  __property TAutoSwitch DDTransferConfirmation = { read = FDDTransferConfirmation, write = SetDDTransferConfirmation };
  __property bool DeleteToRecycleBin = { read = FDeleteToRecycleBin, write = SetDeleteToRecycleBin };
  __property bool DimmHiddenFiles = { read = FDimmHiddenFiles, write = SetDimmHiddenFiles };
  __property bool RenameWholeName = { read = FRenameWholeName, write = SetRenameWholeName };
  __property bool ConfirmTransferring = { read = FConfirmTransferring, write = SetConfirmTransferring};
  __property bool ConfirmDeleting = { read = FConfirmDeleting, write = SetConfirmDeleting};
  __property bool ConfirmRecycling = { read = FConfirmRecycling, write = SetConfirmRecycling};
  __property bool UseLocationProfiles = { read = FUseLocationProfiles, write = SetUseLocationProfiles};
  __property bool UseSharedBookmarks = { read = FUseSharedBookmarks, write = SetUseSharedBookmarks};
  __property UnicodeString DDTemporaryDirectory  = { read = FDDTemporaryDirectory, write = SetDDTemporaryDirectory };
  __property bool DDWarnLackOfTempSpace  = { read = FDDWarnLackOfTempSpace, write = SetDDWarnLackOfTempSpace };
  __property bool DDExtEnabled = { read = FDDExtEnabled, write = SetDDExtEnabled };
  __property bool DDExtInstalled = { read = GetDDExtInstalled };
  __property int DDExtTimeout = { read = FDDExtTimeout, write = SetDDExtTimeout };
  __property bool ConfirmClosingSession  = { read = FConfirmClosingSession, write = SetConfirmClosingSession };
  __property double DDWarnLackOfTempSpaceRatio  = { read = FDDWarnLackOfTempSpaceRatio, write = SetDDWarnLackOfTempSpaceRatio };
  __property TBookmarkList *Bookmarks[UnicodeString Key] = { read = GetBookmarks, write = SetBookmarks };
  __property TBookmarkList *SharedBookmarks = { read = GetSharedBookmarks, write = SetSharedBookmarks };
  __property bool EmbeddedSessions = { read = FEmbeddedSessions };
  __property bool ExpertMode = { read = FExpertMode, write = SetExpertMode };
  __property bool DefaultDirIsHome = { read = FDefaultDirIsHome, write = SetDefaultDirIsHome };
  __property bool DisableOpenEdit = { read = FDisableOpenEdit };
  __property TCustomCommandList *CustomCommandList = { read = FCustomCommandList, write = SetCustomCommandList };
  __property TCustomCommandList *ExtensionList = { read = FExtensionList, write = SetExtensionList };
  __property int DDDeleteDelay = { read = FDDDeleteDelay };
  __property bool TemporaryDirectoryAppendSession = { read = FTemporaryDirectoryAppendSession, write = SetTemporaryDirectoryAppendSession };
  __property bool TemporaryDirectoryAppendPath = { read = FTemporaryDirectoryAppendPath, write = SetTemporaryDirectoryAppendPath };
  __property bool TemporaryDirectoryDeterministic = { read = FTemporaryDirectoryDeterministic, write = SetTemporaryDirectoryDeterministic };
  __property bool TemporaryDirectoryCleanup = { read = FTemporaryDirectoryCleanup, write = SetTemporaryDirectoryCleanup };
  __property bool ConfirmTemporaryDirectoryCleanup = { read = FConfirmTemporaryDirectoryCleanup, write = SetConfirmTemporaryDirectoryCleanup };
  __property bool PreservePanelState = { read = FPreservePanelState, write = SetPreservePanelState };
  __property UnicodeString LastStoredSession = { read = FLastStoredSession, write = SetLastStoredSession };
  __property UnicodeString LastWorkspace = { read = FLastWorkspace, write = FLastWorkspace };
  __property bool AutoSaveWorkspace = { read = FAutoSaveWorkspace, write = SetAutoSaveWorkspace };
  __property bool AutoSaveWorkspacePasswords = { read = FAutoSaveWorkspacePasswords, write = SetAutoSaveWorkspacePasswords };
  __property UnicodeString AutoWorkspace = { read = FAutoWorkspace, write = SetAutoWorkspace };
  __property TPathInCaption PathInCaption = { read = FPathInCaption, write = SetPathInCaption };
  __property bool MinimizeToTray = { read = GetMinimizeToTray, write = SetMinimizeToTray };
  __property bool BalloonNotifications = { read = FBalloonNotifications, write = SetBalloonNotifications };
  __property unsigned int NotificationsTimeout = { read = FNotificationsTimeout, write = SetNotificationsTimeout };
  __property unsigned int NotificationsStickTime = { read = FNotificationsStickTime, write = SetNotificationsStickTime };
  __property UnicodeString DefaultTranslationFile = { read = FDefaultTranslationFile };
  __property bool CopyParamAutoSelectNotice = { read = FCopyParamAutoSelectNotice, write = SetCopyParamAutoSelectNotice };
  __property bool LockToolbars = { read = FLockToolbars, write = SetLockToolbars };
  __property bool SelectiveToolbarText = { read = FSelectiveToolbarText, write = SetSelectiveToolbarText };
  __property bool AutoOpenInPutty = { read = FAutoOpenInPutty, write = SetAutoOpenInPutty };
  __property bool RefreshRemotePanel = { read = FRefreshRemotePanel, write = SetRefreshRemotePanel };
  __property TDateTime RefreshRemotePanelInterval = { read = FRefreshRemotePanelInterval, write = SetRefreshRemotePanelInterval };
  __property TFontConfiguration PanelFont = { read = FPanelFont, write = SetPanelFont };
  __property bool NaturalOrderNumericalSorting = { read = FNaturalOrderNumericalSorting, write = SetNaturalOrderNumericalSorting };
  __property bool FullRowSelect = { read = FFullRowSelect, write = SetFullRowSelect };
  __property bool OfferedEditorAutoConfig = { read = FOfferedEditorAutoConfig, write = SetOfferedEditorAutoConfig };
  __property int LastMonitor = { read = GetLastMonitor, write = SetLastMonitor };
  __property const TEditorList *EditorList = { read = GetEditorList, write = SetEditorList };
  __property UnicodeString DefaultKeyFile = { read = GetDefaultKeyFile, write = FDefaultKeyFile };
  __property UnicodeString OpenedStoredSessionFolders = { read = FOpenedStoredSessionFolders, write = SetOpenedStoredSessionFolders };
  __property bool AutoImportedFromPuttyOrFilezilla = { read = FAutoImportedFromPuttyOrFilezilla, write = SetAutoImportedFromPuttyOrFilezilla };
  __property int GenerateUrlComponents = { read = FGenerateUrlComponents, write = SetGenerateUrlComponents };
  __property TGenerateUrlCodeTarget GenerateUrlCodeTarget = { read = FGenerateUrlCodeTarget, write = SetGenerateUrlCodeTarget };
  __property TScriptFormat GenerateUrlScriptFormat = { read = FGenerateUrlScriptFormat, write = SetGenerateUrlScriptFormat };
  __property TAssemblyLanguage GenerateUrlAssemblyLanguage = { read = FGenerateUrlAssemblyLanguage, write = SetGenerateUrlAssemblyLanguage };
  __property bool ExternalSessionInExistingInstance = { read = FExternalSessionInExistingInstance, write = SetExternalSessionInExistingInstance };
  __property bool KeepOpenWhenNoSession = { read = FKeepOpenWhenNoSession, write = SetKeepOpenWhenNoSession };
  __property bool LocalIconsByExt = { read = FLocalIconsByExt, write = SetLocalIconsByExt };
  __property TLocaleFlagOverride BidiModeOverride = { read = FBidiModeOverride, write = SetBidiModeOverride };
  __property TLocaleFlagOverride FlipChildrenOverride = { read = FFlipChildrenOverride, write = SetFlipChildrenOverride };
  __property bool ShowTips = { read = FShowTips, write = SetShowTips };
  __property UnicodeString TipsSeen = { read = FTipsSeen, write = SetTipsSeen };
  __property TDateTime TipsShown = { read = FTipsShown, write = SetTipsShown };
  __property int RunsSinceLastTip = { read = FRunsSinceLastTip, write = SetRunsSinceLastTip };
  __property bool HonorDrivePolicy = { read = GetHonorDrivePolicy, write = SetHonorDrivePolicy };
  __property TMasterPasswordPromptEvent OnMasterPasswordPrompt = { read = FOnMasterPasswordPrompt, write = FOnMasterPasswordPrompt };
  __property TStrings *CustomCommandOptions = { read = GetCustomCommandOptions, write = SetCustomCommandOptions };
  __property bool LockedInterface = { read = FLockedInterface, write = SetLockedInterface };
  __property LCID DefaultLocale = { read = FDefaultLocale };
  __property int LocaleCompletenessTreshold = { read = GetLocaleCompletenessTreshold };
};
//---------------------------------------------------------------------------
class TCustomCommandType
{
public:
  TCustomCommandType();
  TCustomCommandType(const TCustomCommandType &Other);

  enum TOptionKind { okUnknown, okLabel, okLink, okSeparator, okGroup, okTextBox, okFile, okDropDownList, okComboBox, okCheckBox };
  enum TOptionFlag { ofRun = 0x01, ofConfig = 0x02, ofSite = 0x04 };

  class TOption
  {
  public:
    TOption() {}

    UnicodeString Id;
    uintptr_t AFlags;
    TOptionKind Kind;
    UnicodeString Caption;
    UnicodeString Default;
    typedef std::vector<UnicodeString> TParams;
    TParams Params;
    UnicodeString FileCaption;
    UnicodeString FileFilter;
    UnicodeString FileInitial;
    UnicodeString FileExt;

    bool operator==(const TOption &Other) const;
    __property bool IsControl = { read = GetIsControl };
    bool HasPatterns(TCustomCommand *CustomCommandForOptions) const;

  private:
    bool GetIsControl() const;
  };

  TCustomCommandType &operator=(const TCustomCommandType &Other);
  bool Equals(const TCustomCommandType *Other) const;

  void LoadExtension(const UnicodeString Path);
  void LoadExtension(TStrings *Lines, const UnicodeString PathForBaseName);
  static UnicodeString GetExtensionId(const UnicodeString Name);

  __property UnicodeString Name = { read = FName, write = FName };
  __property UnicodeString Command = { read = FCommand, write = FCommand };
  __property int Params = { read = FParams, write = FParams };
  __property TShortCut ShortCut = { read = FShortCut, write = FShortCut };
  __property UnicodeString Id = { read = FId, write = FId };
  __property UnicodeString FileName = { read = FFileName, write = FFileName };
  __property UnicodeString Description = { read = FDescription, write = FDescription };
  __property UnicodeString HomePage = { read = FHomePage, write = FHomePage };
  __property UnicodeString OptionsPage = { read = FOptionsPage, write = FOptionsPage };

  __property int OptionsCount = { read = GetOptionsCount };
  const TOption &GetOption(int Index) const;
  bool AnyOptionWithFlag(unsigned int Flag) const;
  UnicodeString GetOptionKey(const TOption &Option, const UnicodeString Site) const;
  UnicodeString GetCommandWithExpandedOptions(
    TStrings *CustomCommandOptions, const UnicodeString Site) const;

protected:
  bool ParseOption(const UnicodeString Value, TOption &Option, const UnicodeString ExtensionBaseName);
  int GetOptionsCount() const;
  UnicodeString GetOptionCommand(const TOption &Option, const UnicodeString Value) const;

private:
  UnicodeString FName;
  UnicodeString FCommand;
  int FParams;
  TShortCut FShortCut;
  UnicodeString FId;
  UnicodeString FFileName;
  UnicodeString FDescription;
  UnicodeString FHomePage;
  UnicodeString FOptionsPage;
  std::vector<TOption> FOptions;
};
//---------------------------------------------------------------------------
class TCustomCommandList
{
public:
  TCustomCommandList();
  ~TCustomCommandList();

  void Load(THierarchicalStorage *Storage);
  void Save(THierarchicalStorage *Storage);
  void Reset();
  void Modify();

  void Clear();
  void Add(const UnicodeString Name, const UnicodeString Command, int Params);
  void Add(TCustomCommandType *Command);
  void Insert(int Index, TCustomCommandType *Command);
  void Change(int Index, TCustomCommandType *Command);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);
  void SortBy(TStrings *Ids);

  const TCustomCommandType *Find(const UnicodeString Name) const;
  const TCustomCommandType *Find(TShortCut ShortCut) const;
  int FindIndexByFileName(const UnicodeString FileName) const;

  bool Equals(const TCustomCommandList *Other) const;
  void Assign(const TCustomCommandList *Other);

  void ShortCuts(TShortCuts &ShortCuts) const;

  __property bool Modified = { read = FModified };
  __property int Count = { read = GetCount };
  __property const TCustomCommandType *Commands[int Index] = { read = GetConstCommand };

private:
  bool FModified;
  TList *FCommands;
  int GetCount() const;
  const TCustomCommandType *GetConstCommand(int Index) const;
  TCustomCommandType *GetCommand(int Index);
};
//---------------------------------------------------------------------------
extern TWinConfiguration *WinConfiguration;
extern const UnicodeString WinSCPExtensionExt;
//---------------------------------------------------------------------------
#endif // #if 0
#endif
