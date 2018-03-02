#pragma once
#include "PuttyTools.h"
//---------------------------------------------------------------------------
NB_CORE_EXPORT void PuttyInitialize();
NB_CORE_EXPORT void PuttyFinalize();
//---------------------------------------------------------------------------
NB_CORE_EXPORT void DontSaveRandomSeed();
//---------------------------------------------------------------------------
#ifndef MPEXT
#define MPEXT
#endif
extern "C"
{
#include <putty.h>
#include <puttyexp.h>
#include <ssh.h>
#include <proxy.h>
#include <storage.h>
// Defined in misc.h - Conflicts with std::min/max
#undef min
#undef max

  extern CRITICAL_SECTION noise_section;
}
//---------------------------------------------------------------------------

