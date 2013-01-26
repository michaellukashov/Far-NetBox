//---------------------------------------------------------------------------
#ifndef PuttyIntfH
#define PuttyIntfH
//---------------------------------------------------------------------------
void PuttyInitialize();
void PuttyFinalize();
//---------------------------------------------------------------------------
void DontSaveRandomSeed();
//---------------------------------------------------------------------------
#include "PuttyTools.h"
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

    extern CRITICAL_SECTION noise_section;
}
//---------------------------------------------------------------------------
#endif
