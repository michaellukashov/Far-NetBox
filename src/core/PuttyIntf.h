
#ifndef PuttyIntfH
#define PuttyIntfH

#include "PuttyTools.h"

void PuttyInitialize();
void PuttyFinalize();

void DontSaveRandomSeed();

#ifndef MPEXT
#define MPEXT
#endif
extern "C"
{
#include <putty/putty.h>
#include <putty/puttyexp.h>
#include <ssh.h>
#include <proxy.h>
#include <storage.h>
// Defined in misc.h - Conflicts with std::min/max
#undef min
#undef max

extern CRITICAL_SECTION noise_section;
}

#endif
