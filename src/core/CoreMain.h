//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <Common.h>
//---------------------------------------------------------------------------
class TConfiguration;
class TStoredSessionList;
NB_CORE_EXPORT extern TStoredSessionList *StoredSessions;
//---------------------------------------------------------------------------
NB_CORE_EXPORT void CoreInitialize();
NB_CORE_EXPORT void CoreFinalize();
NB_CORE_EXPORT void CoreSetResourceModule(void *ResourceHandle);
NB_CORE_EXPORT void CoreMaintenanceTask();
NB_CORE_EXPORT TConfiguration *GetConfiguration();
//---------------------------------------------------------------------------
NB_CORE_EXPORT UnicodeString NeonVersion();
NB_CORE_EXPORT UnicodeString ExpatVersion();
//---------------------------------------------------------------------------
