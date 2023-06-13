
#pragma once

#include <Common.h>
#include "SessionInfo.h"

//extern TConfiguration *Configuration;
NB_CORE_EXPORT extern TStoredSessionList *StoredSessions;
extern bool AnySession;
extern TApplicationLog * ApplicationLog;
#define AppLog(S) if (ApplicationLog->Logging) ApplicationLog->Log(S)
#define AppLogFmt(S, ...) AppLog(FORMAT(S, __VA_ARGS__))

NB_CORE_EXPORT void CoreInitialize();
NB_CORE_EXPORT void CoreFinalize();
NB_CORE_EXPORT void CoreSetResourceModule(void *ResourceHandle);
NB_CORE_EXPORT void CoreMaintenanceTask();
void CoreUpdateFinalStaticUsage();
NB_CORE_EXPORT TConfiguration *GetConfiguration();

NB_CORE_EXPORT UnicodeString NeonVersion();
NB_CORE_EXPORT UnicodeString ExpatVersion();

