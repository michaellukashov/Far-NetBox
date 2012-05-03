//---------------------------------------------------------------------------
#ifndef PuttyIntfH
#define PuttyIntfH

#include <Commdlg.h>

//---------------------------------------------------------------------------
void __fastcall PuttyInitialize();
void __fastcall PuttyFinalize();
//---------------------------------------------------------------------------
void __fastcall DontSaveRandomSeed();
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
