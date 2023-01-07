
#ifndef SetupH
#define SetupH
#if 0

#include <Interface.h>
#include <WinConfiguration.h>
#include <WinInterface.h>

void SetupInitialize();
void AddSearchPath(const UnicodeString Path);
void RemoveSearchPath(const UnicodeString Path);
class THttp;
THttp * CreateHttp();
void GetUpdatesMessage(UnicodeString & Message, bool & New, TQueryType & Type, bool Force);
bool CheckForUpdates(bool CachedResults);
bool QueryUpdates(TUpdatesConfiguration & Updates);
void FormatUpdatesMessage(UnicodeString & UpdatesMessage, const UnicodeString & AMessage, const TUpdatesConfiguration & Updates);
void EnableAutomaticUpdates();
void RegisterForDefaultProtocols();
void UnregisterForProtocols();
void LaunchAdvancedAssociationUI();
void TemporaryDirectoryCleanup();
void StartUpdateThread(TThreadMethod OnUpdatesChecked);
void StopUpdateThread();
UnicodeString CampaignUrl(UnicodeString URL);
void UpdateJumpList(TStrings * SessionNames, TStrings * WorkspaceNames);
bool AnyOtherInstanceOfSelf();
bool IsInstalled();
UnicodeString ProgramUrl(UnicodeString URL);
void AutoShowNewTip();
bool AnyTips();
void ShowTips();
UnicodeString FirstUnshownTip();
void TipsUpdateStaticUsage();
int GetNetVersion();
UnicodeString GetNetVersionStr();
UnicodeString GetNetCoreVersionStr();
UnicodeString GetPowerShellVersionStr();
UnicodeString GetPowerShellCoreVersionStr();
int ComRegistration(TConsole * Console);

#endif // #if 0
#endif
