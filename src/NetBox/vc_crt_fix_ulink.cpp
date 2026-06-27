// validator: no-self-include
/*
vc_crt_fix_ulink.cpp

Workaround for Visual C++ CRT incompatibility with old Windows versions (ulink version)
*/
/*
Copyright © 2010 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#include <delayimp.h>

//----------------------------------------------------------------------------
#ifndef _WIN64
static BOOL WINAPI sim_GetModuleHandleExW(DWORD flg, LPCWSTR name, HMODULE* pm)
{
    // GET_MODULE_HANDLE_EX_FLAG_PIN not implemented (and unneeded)
    HMODULE   hm;
    wchar_t   buf[MAX_PATH];

    *pm = NULL; // prepare to any return's
    if(flg & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
      MEMORY_BASIC_INFORMATION  mbi;
      if(!VirtualQuery(name, &mbi, sizeof(mbi))) return FALSE;
      hm = (HMODULE)mbi.AllocationBase;
      if(flg & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT) goto done;
      if(!GetModuleFileNameW(hm, buf, ARRAYSIZE(buf))) return FALSE;
      name = buf;
    } else if(flg & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT) {
      hm = GetModuleHandleW(name);
      goto done;
    }
    hm = LoadLibraryW(name);
done:
    return (*pm = hm) != NULL;
}
#endif

//----------------------------------------------------------------------------
static BOOL WINAPI sim_InitializeCriticalSectionEx(LPCRITICAL_SECTION psec,
                                                   DWORD cnt, DWORD flg)
{
    InitializeCriticalSection(psec);
    psec->SpinCount = cnt | flg;
    return TRUE;
}

//----------------------------------------------------------------------------
static int WINAPI sim_LCMapStringEx(LPCWSTR, DWORD flg, LPCWSTR src, int scn,
                                    LPWSTR dst, int dcn,
                                    LPNLSVERSIONINFO, LPVOID, LPARAM)
{
    return LCMapStringW(LOCALE_USER_DEFAULT, flg, src, scn, dst, dcn);
}

//----------------------------------------------------------------------------
static int WINAPI sim_CompareStringEx(LPCWSTR, DWORD flg, LPCWCH s1, int c1,
                                      LPCWCH s2, int c2, LPNLSVERSIONINFO,
                                      LPVOID, LPARAM)
{
    return CompareStringW(LOCALE_USER_DEFAULT, flg, s1, c1, s2, c2);
}

//----------------------------------------------------------------------------
#ifndef _WIN64
static DWORD WINAPI sim_FlsAlloc(PFLS_CALLBACK_FUNCTION)
{
    return TlsAlloc();
}
#endif

//----------------------------------------------------------------------------
static void WINAPI sim_InitializeSRWLock(PSRWLOCK SRWLock)
{
    SRWLock->Ptr = nullptr;
}

//----------------------------------------------------------------------------
static void WINAPI sim_ReleaseSRWLock(PSRWLOCK SRWLock)
{
    InterlockedExchangePointer(&SRWLock->Ptr, nullptr);
}

//----------------------------------------------------------------------------
static BOOLEAN WINAPI sim_TryAcquireSRWLock(PSRWLOCK SRWLock)
{
    return InterlockedCompareExchangePointer(&SRWLock->Ptr, (PVOID)1, nullptr) == nullptr;
}

//----------------------------------------------------------------------------
static VOID WINAPI sim_AcquireSRWLock(PSRWLOCK SRWLock)
{
    unsigned sc = 0;
    while(!sim_TryAcquireSRWLock(SRWLock)) {
      // backoff: [0,10) yield, [10,20) Sleep(0), [20,inf) Sleep(1)
      if(sc < 10) YieldProcessor();
      else Sleep(sc < 20 ? 0 : 1);
      ++sc;
    }
}

//----------------------------------------------------------------------------
static VOID WINAPI sim_InitializeConditionVariable(PCONDITION_VARIABLE CondVar)
{
    CondVar->Ptr = nullptr;
}

//----------------------------------------------------------------------------
static BOOL WINAPI sim_SleepConditionVariableSRW(PCONDITION_VARIABLE, PSRWLOCK, DWORD, ULONG)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

//----------------------------------------------------------------------------
static BOOL WINAPI sim_SleepConditionVariableCS(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

//----------------------------------------------------------------------------
static void WINAPI sim_WakeConditionVariable(PCONDITION_VARIABLE)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

//----------------------------------------------------------------------------
static FARPROC WINAPI delayFailureHook(/*dliNotification*/unsigned dliNotify,
                                       PDelayLoadInfo pdli)
{
    if(   dliNotify == dliFailGetProc
       && pdli && pdli->cb == sizeof(*pdli)
       && pdli->dlp.fImportByName && pdli->dlp.szProcName)
    {
#ifndef _WIN64
      if(!lstrcmpA(pdli->dlp.szProcName, "GetModuleHandleExW"))
        return (FARPROC)sim_GetModuleHandleExW;
      if(!lstrcmpA(pdli->dlp.szProcName, "FlsAlloc"))
        return (FARPROC)sim_FlsAlloc;
      if(!lstrcmpA(pdli->dlp.szProcName, "FlsFree"))
        return (FARPROC)TlsFree;
      if(!lstrcmpA(pdli->dlp.szProcName, "FlsGetValue"))
        return (FARPROC)TlsGetValue;
      if(!lstrcmpA(pdli->dlp.szProcName, "FlsSetValue"))
        return (FARPROC)TlsSetValue;
#endif
      if(!lstrcmpA(pdli->dlp.szProcName, "InitializeCriticalSectionEx"))
        return (FARPROC)sim_InitializeCriticalSectionEx;
      if(!lstrcmpA(pdli->dlp.szProcName, "LCMapStringEx"))
        return (FARPROC)sim_LCMapStringEx;
      if(!lstrcmpA(pdli->dlp.szProcName, "CompareStringEx"))
        return (FARPROC)sim_CompareStringEx;
      if(!lstrcmpA(pdli->dlp.szProcName, "SleepConditionVariableSRW"))
        return (FARPROC)sim_SleepConditionVariableSRW;
      if(!lstrcmpA(pdli->dlp.szProcName, "WakeAllConditionVariable"))
        return (FARPROC)sim_WakeConditionVariable;
      if(!lstrcmpA(pdli->dlp.szProcName, "ReleaseSRWLockExclusive"))
        return (FARPROC)sim_ReleaseSRWLock;
      if(!lstrcmpA(pdli->dlp.szProcName, "AcquireSRWLockExclusive"))
        return (FARPROC)sim_AcquireSRWLock;
      if(!lstrcmpA(pdli->dlp.szProcName, "TryAcquireSRWLockExclusive"))
        return (FARPROC)sim_TryAcquireSRWLock;
      if(!lstrcmpA(pdli->dlp.szProcName, "InitializeSRWLock"))
        return (FARPROC)sim_InitializeSRWLock;
      // Shared SRW locks are simulated as exclusive (matches vc_crt_fix_impl.cpp);
      // on Vista+ the real kernel32 shared semantics are resolved via delayload.
      if(!lstrcmpA(pdli->dlp.szProcName, "AcquireSRWLockShared"))
        return (FARPROC)sim_AcquireSRWLock;
      if(!lstrcmpA(pdli->dlp.szProcName, "ReleaseSRWLockShared"))
        return (FARPROC)sim_ReleaseSRWLock;
      if(!lstrcmpA(pdli->dlp.szProcName, "TryAcquireSRWLockShared"))
        return (FARPROC)sim_TryAcquireSRWLock;
      if(!lstrcmpA(pdli->dlp.szProcName, "WakeConditionVariable"))
        return (FARPROC)sim_WakeConditionVariable;
      if(!lstrcmpA(pdli->dlp.szProcName, "SleepConditionVariableCS"))
        return (FARPROC)sim_SleepConditionVariableCS;
      if(!lstrcmpA(pdli->dlp.szProcName, "InitializeConditionVariable"))
        return (FARPROC)sim_InitializeConditionVariable;
    }
    return nullptr;
}

//----------------------------------------------------------------------------
#ifndef _WIN64
#pragma comment(linker, "/delayload:kernel32.GetModuleHandleExW")
#pragma comment(linker, "/delayload:kernel32.FlsAlloc")
#pragma comment(linker, "/delayload:kernel32.FlsFree")
#pragma comment(linker, "/delayload:kernel32.FlsGetValue")
#pragma comment(linker, "/delayload:kernel32.FlsSetValue")
#endif
#pragma comment(linker, "/delayload:kernel32.CompareStringEx")
#pragma comment(linker, "/delayload:kernel32.LCMapStringEx")
#pragma comment(linker, "/delayload:kernel32.InitializeCriticalSectionEx")
#pragma comment(linker, "/delayload:kernel32.SleepConditionVariableSRW")
#pragma comment(linker, "/delayload:kernel32.WakeAllConditionVariable")
#pragma comment(linker, "/delayload:kernel32.ReleaseSRWLockExclusive")
#pragma comment(linker, "/delayload:kernel32.AcquireSRWLockExclusive")
#pragma comment(linker, "/delayload:kernel32.TryAcquireSRWLockExclusive")
#pragma comment(linker, "/delayload:kernel32.InitializeSRWLock")
#pragma comment(linker, "/delayload:kernel32.AcquireSRWLockShared")
#pragma comment(linker, "/delayload:kernel32.ReleaseSRWLockShared")
#pragma comment(linker, "/delayload:kernel32.TryAcquireSRWLockShared")
#pragma comment(linker, "/delayload:kernel32.WakeConditionVariable")
#pragma comment(linker, "/delayload:kernel32.SleepConditionVariableCS")
#pragma comment(linker, "/delayload:kernel32.InitializeConditionVariable")

//----------------------------------------------------------------------------
// VS2015sp3
#if (_MSC_FULL_VER >= 190024215) && !defined(DELAYIMP_INSECURE_WRITABLE_HOOKS)
const
#endif
PfnDliHook __pfnDliFailureHook2 = (PfnDliHook)delayFailureHook;

//----------------------------------------------------------------------------
