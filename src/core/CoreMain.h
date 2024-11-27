
#pragma once

#include <Common.h>
#include "SessionInfo.h"

#if defined(__BORLANDC__)
extern TConfiguration * Configuration;
extern TStoredSessionList * StoredSessions;
#endif // defined(__BORLANDC__)
extern bool AnySession;
extern TApplicationLog * ApplicationLog;
#define APPLOG_INTERNAL(S) if (ApplicationLog && ApplicationLog->Logging) ApplicationLog->Log(S)
#define AppLog(S) APPLOG_INTERNAL(S)
#define AppLogFmt(S, ...) AppLog(FORMAT(S, __VA_ARGS__))

NB_CORE_EXPORT void CoreInitialize();
NB_CORE_EXPORT void CoreFinalize();
NB_CORE_EXPORT void CoreSetResourceModule(void * ResourceHandle);
NB_CORE_EXPORT void CoreMaintenanceTask();
void CoreUpdateFinalStaticUsage();
TConfiguration * GetConfiguration();
TStoredSessionList * GetStoredSessions(bool * JustLoaded = nullptr);
void DeleteStoredSessions();

NB_CORE_EXPORT UnicodeString NeonVersion();
NB_CORE_EXPORT UnicodeString ExpatVersion();

