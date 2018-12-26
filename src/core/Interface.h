
#pragma once
//---------------------------------------------------------------------------
#include <Classes.hpp>
#if defined(FARPLUGIN)
#include "Configuration.h"
#include "SessionData.h"
#endif // FARPLUGIN
__removed #include <typeinfo>
#define HELP_NONE ""
#define SCRIPT_SWITCH "script"
#define COMMAND_SWITCH "Command"
#define SESSIONNAME_SWITCH "SessionName"
#define NEWPASSWORD_SWITCH "newpassword"
#define INI_NUL "nul"
#define PRESERVETIME_SWITCH "preservetime"
#define PRESERVETIMEDIRS_SWITCH_VALUE "all"
#define NOPRESERVETIME_SWITCH "nopreservetime"
#define PERMISSIONS_SWITCH "permissions"
#define NOPERMISSIONS_SWITCH "nopermissions"
#define SPEED_SWITCH "speed"
#define TRANSFER_SWITCH "transfer"
#define FILEMASK_SWITCH "filemask"
#define RESUMESUPPORT_SWITCH "resumesupport"
#define NEWERONLY_SWICH "neweronly"
#define NONEWERONLY_SWICH "noneweronly"
#define DELETE_SWITCH "delete"
#define REFRESH_SWITCH "refresh"
#define RAWTRANSFERSETTINGS_SWITCH L"rawtransfersettings"
extern const wchar_t *TransferModeNames[];
extern const int TransferModeNamesCount;
extern const wchar_t *ToggleNames[];
enum TToggle { ToggleOff, ToggleOn };
//---------------------------------------------------------------------------
#if defined(FARPLUGIN)
NB_CORE_EXPORT TConfiguration *CreateConfiguration();
class TOptions;
NB_CORE_EXPORT TOptions *GetGlobalOptions();
#endif // FARPLUGIN

NB_CORE_EXPORT void ShowExtendedException(Exception *E);
NB_CORE_EXPORT bool AppendExceptionStackTraceAndForget(TStrings *&MoreMessages);
__removed void IgnoreException(const std::type_info &ExceptionType);
__removed UnicodeString GetExceptionDebugInfo();

NB_CORE_EXPORT UnicodeString GetCompanyRegistryKey();
NB_CORE_EXPORT UnicodeString GetRegistryKey();
NB_CORE_EXPORT void *BusyStart();
NB_CORE_EXPORT void BusyEnd(void *Token);
NB_CORE_EXPORT static const uint32_t GUIUpdateInterval = 100;
NB_CORE_EXPORT void SetNoGUI();
NB_CORE_EXPORT bool ProcessGUI(bool Force = false);
NB_CORE_EXPORT UnicodeString GetAppNameString();
NB_CORE_EXPORT UnicodeString GetSshVersionString();
NB_CORE_EXPORT void CopyToClipboard(const UnicodeString Text);
NB_CORE_EXPORT HANDLE StartThread(void *SecurityAttributes, DWORD StackSize,
  /*TThreadFunc ThreadFunc,*/ void *Parameter, DWORD CreationFlags,
  TThreadID &ThreadId);
NB_CORE_EXPORT bool TextFromClipboard(UnicodeString &Text, bool Trim);

NB_CORE_EXPORT void WinInitialize();
NB_CORE_EXPORT void WinFinalize();

#if 0
typedef void (__closure *TButtonSubmitEvent)(TObject *Sender, uint32_t &Answer);
#endif // #if 0
typedef nb::FastDelegate2<void,
  TObject * /*Sender*/, uint32_t & /*Answer*/> TButtonSubmitEvent;

struct NB_CORE_EXPORT TQueryButtonAlias : public TObject
{
  TQueryButtonAlias();

  uintptr_t Button{0};
  UnicodeString Alias;
  TButtonSubmitEvent OnSubmit;
  int GroupWith{0};
  bool Default{false};
  TShiftStateFlag GrouppedShiftState{};
  bool ElevationRequired{false};
  bool MenuButton{false};
  UnicodeString ActionAlias;

  static TQueryButtonAlias CreateYesToAllGrouppedWithYes();
  static TQueryButtonAlias CreateNoToAllGrouppedWithNo();
  static TQueryButtonAlias CreateAllAsYesToNewerGrouppedWithYes();
  static TQueryButtonAlias CreateIgnoreAsRenameGrouppedWithNo();
};

#if 0
typedef void (__closure *TQueryParamsTimerEvent)(uint32_t &Result);
#endif // #if 0
typedef nb::FastDelegate1<void, uint32_t & /*Result*/> TQueryParamsTimerEvent;

struct NB_CORE_EXPORT TQueryParams : public TObject
{
  explicit TQueryParams(uintptr_t AParams = 0, const UnicodeString AHelpKeyword = HELP_NONE);
  explicit TQueryParams(const TQueryParams &Source);

  void Assign(const TQueryParams &Source);

  const TQueryButtonAlias *Aliases{nullptr};
  uintptr_t AliasesCount{0};
  uintptr_t Params{0};
  uintptr_t Timer{0};
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uint32_t TimerAnswers{0};
  TQueryType TimerQueryType;
  uintptr_t Timeout{0};
  uintptr_t TimeoutAnswer{0};
  uintptr_t NoBatchAnswers{0};
  UnicodeString HelpKeyword;

public:
  TQueryParams &operator=(const TQueryParams &other);
};

enum TPromptKind
{
  pkPrompt,
  pkFileName,
  pkUserName,
  pkPassphrase,
  pkTIS,
  pkCryptoCard,
  pkKeybInteractive,
  pkPassword,
  pkNewPassword,
};

enum TPromptUserParam
{
  pupEcho = 0x01,
  pupRemember = 0x02,
};

NB_CORE_EXPORT bool IsAuthenticationPrompt(TPromptKind Kind);
NB_CORE_EXPORT bool IsPasswordOrPassphrasePrompt(TPromptKind Kind, TStrings *Prompts);
NB_CORE_EXPORT bool IsPasswordPrompt(TPromptKind Kind, TStrings *Prompts);
class TTerminal;
class TRemoteFile;
//---------------------------------------------------------------------------
#if 0
typedef void (__closure *TFileFoundEvent)
(TTerminal *Terminal, const UnicodeString FileName, const TRemoteFile *File,
  bool &Cancel);
#endif // #if 0
typedef nb::FastDelegate4<void,
  TTerminal * /*Terminal*/, UnicodeString /*FileName*/,
  const TRemoteFile * /*File*/,
  bool & /*Cancel*/> TFileFoundEvent;
#if 0
typedef void (__closure *TFindingFileEvent)
(TTerminal *Terminal, const UnicodeString Directory, bool &Cancel);
#endif // #if 0
typedef nb::FastDelegate3<void,
  TTerminal * /*Terminal*/, UnicodeString /*ADirectory*/, bool & /*Cancel*/> TFindingFileEvent;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TOperationVisualizer
{
  NB_DISABLE_COPY(TOperationVisualizer)
public:
  explicit TOperationVisualizer(bool UseBusyCursor = true) noexcept;
  ~TOperationVisualizer() noexcept;

private:
  bool FUseBusyCursor{false};
  void *FToken{nullptr};
};
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TInstantOperationVisualizer : public TOperationVisualizer
{
public:
  TInstantOperationVisualizer() noexcept;
  ~TInstantOperationVisualizer() noexcept;

private:
  TDateTime FStart;
};
//---------------------------------------------------------------------------
struct TClipboardHandler
{
  NB_DISABLE_COPY(TClipboardHandler)
public:
  TClipboardHandler() noexcept = default;

  UnicodeString Text;

  void Copy(TObject * /*Sender*/, uint32_t & /*Answer*/)
  {
    TInstantOperationVisualizer Visualizer; nb::used(Visualizer);
    CopyToClipboard(Text);
  }
};
//---------------------------------------------------------------------------
