//---------------------------------------------------------------------------
#ifndef PuttyIntfH
#define PuttyIntfH

#include <Commdlg.h>

//---------------------------------------------------------------------------
void PuttyInitialize();
void PuttyFinalize();
//---------------------------------------------------------------------------
void DontSaveRandomSeed();
//---------------------------------------------------------------------------
#include "PuttyTools.h"
//---------------------------------------------------------------------------
// #define MPEXT
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
