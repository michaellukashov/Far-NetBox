// validator: no-self-include
/*
vc_crt_fix_impl.cpp

Workaround for Visual C++ CRT incompatibility with old Windows versions
*/
/*
Copyright © 2011 Far Group
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

#include <memory>
#include <utility>

#include <Windows.h>
#include <WinNls.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

// Compatibility: LOCALE_NAME_SYSTEM_DEFAULT is Vista+, define for XP
#ifndef LOCALE_NAME_SYSTEM_DEFAULT
#define LOCALE_NAME_SYSTEM_DEFAULT L"!x-sys-default-locale"
#endif

// Debug helper: uncomment to trace wrapper calls
//#define WRAPPER_DEBUG
#ifdef WRAPPER_DEBUG
#define WRAPPER_DEBUG_OUTPUT(name) ::OutputDebugStringW(L"Wrapper called: " L##name L"\n")
#else
#define WRAPPER_DEBUG_OUTPUT(name) ((void)0)
#endif


template<typename T>
static T GetFunctionPointer(const wchar_t* ModuleName, const char* FunctionName, T Replacement)
{
	const auto Module = GetModuleHandleW(ModuleName);
	const auto Address = Module? GetProcAddress(Module, FunctionName) : nullptr;
	return Address? reinterpret_cast<T>(reinterpret_cast<void*>(Address)) : Replacement;
}

#define WRAPPER(name) Wrapper_ ## name
#define CREATE_AND_RETURN(ModuleName, ...) \
	{ \
		const auto ModHandle = GetModuleHandleW(ModuleName); \
		const auto Address = ModHandle ? GetProcAddress(ModHandle, __func__ + sizeof("Wrapper_") - 1) : nullptr; \
		if (Address) \
		{ \
			return reinterpret_cast<decltype(implementation::impl)*>(Address)(__VA_ARGS__); \
		} \
		return implementation::impl(__VA_ARGS__); \
	}

namespace modules
{
	static const wchar_t kernel32[] = L"kernel32";
}

static void* XorPointer(void* Ptr)
{
	static const auto Cookie = []
	{
		static void* Ptr;
		auto Result = reinterpret_cast<uintptr_t>(&Ptr);

		FILETIME CreationTime, NotUsed;
		if (GetThreadTimes(GetCurrentThread(), &CreationTime, &NotUsed, &NotUsed, &NotUsed))
		{
			Result ^= CreationTime.dwLowDateTime;
			Result ^= CreationTime.dwHighDateTime;
		}
		return Result;
	}();
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Ptr) ^ Cookie);
}

// EncodePointer (VC2010)
extern "C" PVOID WINAPI WRAPPER(EncodePointer)(PVOID Ptr)
{
	struct implementation
	{
		static PVOID WINAPI impl(PVOID Ptr)
		{
			return XorPointer(Ptr);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Ptr);
}

// DecodePointer(VC2010)
extern "C" PVOID WINAPI WRAPPER(DecodePointer)(PVOID Ptr)
{
	struct implementation
	{
		static PVOID WINAPI impl(PVOID Ptr)
		{
			return XorPointer(Ptr);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Ptr);
}

// GetModuleHandleExW (VC2012)
extern "C" BOOL WINAPI WRAPPER(GetModuleHandleExW)(DWORD Flags, LPCWSTR ModuleName, HMODULE *Module)
{
	struct implementation
	{
		static BOOL WINAPI impl(DWORD Flags, LPCWSTR ModuleName, HMODULE *Module)
		{
			if (Flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
			{
				MEMORY_BASIC_INFORMATION mbi;
				if (!VirtualQuery(ModuleName, &mbi, sizeof(mbi)))
					return FALSE;

				const auto ModuleValue = static_cast<HMODULE>(mbi.AllocationBase);

				if (!(Flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
				{
					wchar_t Buffer[MAX_PATH];
					if (!GetModuleFileNameW(ModuleValue, Buffer, ARRAYSIZE(Buffer)))
						return FALSE;
					// It's the same so not really necessary, but analysers report handle leak otherwise.
					*Module = LoadLibraryW(Buffer);
				}
				else
				{
					*Module = ModuleValue;
				}
				return TRUE;
			}

			// GET_MODULE_HANDLE_EX_FLAG_PIN not implemented

			if (const auto ModuleValue = (Flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT? GetModuleHandleW : LoadLibraryW)(ModuleName))
			{
				*Module = ModuleValue;
				return TRUE;
			}
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Flags, ModuleName, Module);
}

// VC2015
extern "C" void WINAPI WRAPPER(InitializeSListHead)(PSLIST_HEADER ListHead)
{
	struct implementation
	{
		static void WINAPI impl(PSLIST_HEADER ListHead)
		{
			*ListHead = {};
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ListHead);
}

#ifndef _WIN64
static bool atomic_assign(PSLIST_HEADER To, SLIST_HEADER const& New, SLIST_HEADER const& Old)
{
	return InterlockedCompareExchange64(
		static_cast<LONG64*>(static_cast<void*>(&To->Alignment)),
		New.Alignment,
		Old.Alignment
	) == static_cast<LONG64>(Old.Alignment);
}
#endif

extern "C" PSLIST_ENTRY WINAPI WRAPPER(InterlockedFlushSList)(PSLIST_HEADER ListHead)
{
	struct implementation
	{
		static PSLIST_ENTRY WINAPI impl(PSLIST_HEADER ListHead)
		{
#ifdef _WIN64
			// The oldest x64 OS (XP) already has SList, so this shall never be called.
			DebugBreak();
			return {};
#else
			if (!ListHead->Next.Next)
				return {};

			SLIST_HEADER OldHeader, NewHeader{};

			do
			{
				OldHeader = *ListHead;
				NewHeader.CpuId = OldHeader.CpuId;
			}
			while (!atomic_assign(ListHead, NewHeader, OldHeader));

			return OldHeader.Next.Next;
#endif
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ListHead);
}

extern "C" PSLIST_ENTRY WINAPI WRAPPER(InterlockedPushEntrySList)(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
{
	struct implementation
	{
		static PSLIST_ENTRY WINAPI impl(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
		{
#ifdef _WIN64
			// The oldest x64 OS (XP) already has SList, so this shall never be called.
			DebugBreak();
			return {};
#else
			SLIST_HEADER OldHeader, NewHeader;
			NewHeader.Next.Next = ListEntry;

			do
			{
				OldHeader = *ListHead;
				ListEntry->Next = OldHeader.Next.Next;
				NewHeader.Depth = OldHeader.Depth + 1;
				NewHeader.CpuId = OldHeader.CpuId;
			}
			while (!atomic_assign(ListHead, NewHeader, OldHeader));

			return OldHeader.Next.Next;
#endif
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ListHead, ListEntry);
}

// VC2017
extern "C" BOOL WINAPI WRAPPER(GetNumaHighestNodeNumber)(PULONG HighestNodeNumber)
{
	struct implementation
	{
		static BOOL WINAPI impl(PULONG HighestNodeNumber)
		{
			*HighestNodeNumber = 0;
			return TRUE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, HighestNodeNumber);
}

// VC2017
extern "C" BOOL WINAPI WRAPPER(GetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnLength)
{
	struct implementation
	{
		static BOOL WINAPI impl(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Info, PDWORD Size)
		{
			if (*Size < sizeof(*Info))
			{
				*Size = sizeof(*Info);
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return FALSE;
			}

			*Info = {};
			Info->ProcessorMask = 1;
			Info->Relationship = RelationProcessorCore;
			return TRUE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Buffer, ReturnLength);
}

// VC2019
extern "C" BOOL WINAPI WRAPPER(SetThreadStackGuarantee)(PULONG StackSizeInBytes)
{
	struct implementation
	{
		static BOOL WINAPI impl(PULONG)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, StackSizeInBytes);
}

// VC2019
extern "C" BOOL WINAPI WRAPPER(InitializeCriticalSectionEx)(LPCRITICAL_SECTION CriticalSection, DWORD SpinCount, DWORD Flags)
{
	struct implementation
	{
		static BOOL WINAPI impl(LPCRITICAL_SECTION CriticalSection, DWORD SpinCount, DWORD Flags)
		{
			InitializeCriticalSection(CriticalSection);
			CriticalSection->SpinCount = Flags | SpinCount;
			return TRUE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, CriticalSection, SpinCount, Flags);
}

static LCID locale_name_to_lcid(const wchar_t* LocaleName)
{
	if (!LocaleName)
		return LOCALE_USER_DEFAULT;

	if (!*LocaleName)
		return LOCALE_INVARIANT;

	if (!lstrcmp(LocaleName, LOCALE_NAME_SYSTEM_DEFAULT))
		return LOCALE_SYSTEM_DEFAULT;

	return LOCALE_USER_DEFAULT;
}

// VC2019
extern "C" int WINAPI WRAPPER(CompareStringEx)(LPCWSTR LocaleName, DWORD CmpFlags, LPCWCH String1, int Count1, LPCWCH String2, int Count2, LPNLSVERSIONINFO VersionInformation, LPVOID Reserved, LPARAM Param)
{
	struct implementation
	{
		static int WINAPI impl(LPCWSTR LocaleName, DWORD CmpFlags, LPCWCH String1, int Count1, LPCWCH String2, int Count2, LPNLSVERSIONINFO, LPVOID, LPARAM)
		{
			return CompareStringW(locale_name_to_lcid(LocaleName), CmpFlags, String1, Count1, String2, Count2);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, LocaleName, CmpFlags, String1, Count1, String2, Count2, VersionInformation, Reserved, Param);
}

// VC2019
extern "C" int WINAPI WRAPPER(LCMapStringEx)(LPCWSTR LocaleName, DWORD MapFlags, LPCWSTR SrcStr, int SrcCount, LPWSTR DestStr, int DestCount, LPNLSVERSIONINFO VersionInformation, LPVOID Reserved, LPARAM SortHandle)
{
	struct implementation
	{
		static int WINAPI impl(LPCWSTR LocaleName, DWORD MapFlags, LPCWSTR SrcStr, int SrcCount, LPWSTR DestStr, int DestCount, LPNLSVERSIONINFO, LPVOID, LPARAM)
		{
			return LCMapStringW(locale_name_to_lcid(LocaleName), MapFlags, SrcStr, SrcCount, DestStr, DestCount);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, LocaleName, MapFlags, SrcStr, SrcCount, DestStr, DestCount, VersionInformation, Reserved, SortHandle);
}


// XP-compatible sync primitive emulation (Vista+ APIs used by MSVC STL)
// All SRW locks and InitOnce operations use global critical sections
// (coarse-grained but safe for startup; prevents crashes from no-op stubs)

static CRITICAL_SECTION g_XPCompatSRWCS;
static volatile LONG g_XPCompatSRWCSInitialized = 0;

static void XPCompat_EnsureSRWCS()
{
	if (InterlockedCompareExchange(&g_XPCompatSRWCSInitialized, 1, 0) == 0)
	{
		InitializeCriticalSection(&g_XPCompatSRWCS);
	}
}

static CRITICAL_SECTION g_XPCompatInitOnceCS;
static volatile LONG g_XPCompatInitOnceCSInitialized = 0;

static void XPCompat_EnsureInitOnceCS()
{
	if (InterlockedCompareExchange(&g_XPCompatInitOnceCSInitialized, 1, 0) == 0)
	{
		InitializeCriticalSection(&g_XPCompatInitOnceCS);
	}
}

enum XP_InitOnceState { XP_INIT_ONCE_NEW = 0, XP_INIT_ONCE_BUSY = 1, XP_INIT_ONCE_DONE = 2, XP_INIT_ONCE_FAILED = 3 };
namespace srw_lock_impl
{
	constexpr uintptr_t EXCLUSIVE_LOCKED = 0x1;

	static void spin_wait(unsigned int Iteration)
	{
		if (Iteration < 10)
			YieldProcessor();
		else if (Iteration < 20)
			Sleep(0);
		else
			Sleep(1);
	}

	void initialize(SRWLOCK* SRWLock)
	{
		*SRWLock = {};
	}

	bool try_acquire(SRWLOCK* SRWLock)
	{
		return InterlockedCompareExchangePointer(
			&SRWLock->Ptr,
			reinterpret_cast<PVOID>(EXCLUSIVE_LOCKED),
			nullptr) == nullptr;
	}

	void acquire(SRWLOCK* SRWLock)
	{
		for (unsigned int SpinCount = 0; !try_acquire(SRWLock); spin_wait(SpinCount++))
			;
	}

	void release(SRWLOCK* SRWLock)
	{
		InterlockedExchangePointer(&SRWLock->Ptr, {});
	}
}

// VC2022
extern "C" BOOL WINAPI WRAPPER(SleepConditionVariableSRW)(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK SRWLock, DWORD Milliseconds, ULONG Flags)
{
	WRAPPER_DEBUG_OUTPUT("SleepConditionVariableSRW");
	struct implementation
	{
		static BOOL WINAPI impl(PCONDITION_VARIABLE, PSRWLOCK, DWORD, ULONG)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ConditionVariable, SRWLock, Milliseconds, Flags);
}

// VC2022
extern "C" void WINAPI WRAPPER(WakeAllConditionVariable)(PCONDITION_VARIABLE ConditionVariable)
{
	WRAPPER_DEBUG_OUTPUT("WakeAllConditionVariable");
	struct implementation
	{
		static void WINAPI impl(PCONDITION_VARIABLE)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ConditionVariable);
}

// VC2022 — Condition variable (singular), needed by OpenSSL thread_win.c
extern "C" void WINAPI WRAPPER(WakeConditionVariable)(PCONDITION_VARIABLE ConditionVariable)
{
	WRAPPER_DEBUG_OUTPUT("WakeConditionVariable");
	struct implementation
	{
		static void WINAPI impl(PCONDITION_VARIABLE)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, ConditionVariable);
}

// VC2022 — SleepConditionVariableCS, needed by OpenSSL thread_win.c
extern "C" BOOL WINAPI WRAPPER(SleepConditionVariableCS)(PCONDITION_VARIABLE ConditionVariable,
	PCRITICAL_SECTION CriticalSection, DWORD dwMilliseconds)
{
	WRAPPER_DEBUG_OUTPUT("SleepConditionVariableCS");
	struct implementation
	{
		static BOOL WINAPI impl(PCONDITION_VARIABLE, PCRITICAL_SECTION, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, ConditionVariable, CriticalSection, dwMilliseconds);
}

// VC2022 — InitializeConditionVariable, needed by OpenSSL thread_win.c
extern "C" void WINAPI WRAPPER(InitializeConditionVariable)(PCONDITION_VARIABLE ConditionVariable)
{
	WRAPPER_DEBUG_OUTPUT("InitializeConditionVariable");
	struct implementation
	{
		static void WINAPI impl(PCONDITION_VARIABLE ConditionVariable)
		{
			*ConditionVariable = {};
		}
	};
	CREATE_AND_RETURN(modules::kernel32, ConditionVariable);
}

// VC2022
extern "C" void WINAPI WRAPPER(AcquireSRWLockExclusive)(PSRWLOCK SRWLock)
{
	// WRAPPER_DEBUG_OUTPUT("AcquireSRWLockExclusive");
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			srw_lock_impl::acquire(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" void WINAPI WRAPPER(ReleaseSRWLockExclusive)(PSRWLOCK SRWLock)
{
	// WRAPPER_DEBUG_OUTPUT("ReleaseSRWLockExclusive");
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			srw_lock_impl::release(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" BOOLEAN WINAPI WRAPPER(TryAcquireSRWLockExclusive)(PSRWLOCK SRWLock)
{
	WRAPPER_DEBUG_OUTPUT("TryAcquireSRWLockExclusive");
	struct implementation
	{
		static BOOLEAN WINAPI impl(PSRWLOCK SRWLock)
		{
			return srw_lock_impl::try_acquire(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2019
extern "C" void WINAPI WRAPPER(InitializeSRWLock)(PSRWLOCK SRWLock)
{
	WRAPPER_DEBUG_OUTPUT("InitializeSRWLock");
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			srw_lock_impl::initialize(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// Shared SRW Lock functions (Vista+, needed for CRT compatibility)
extern "C" void WINAPI WRAPPER(AcquireSRWLockShared)(PSRWLOCK SRWLock)
{
	// WRAPPER_DEBUG_OUTPUT("AcquireSRWLockShared");
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			srw_lock_impl::acquire(SRWLock);
		};
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

extern "C" void WINAPI WRAPPER(ReleaseSRWLockShared)(PSRWLOCK SRWLock)
{
	// WRAPPER_DEBUG_OUTPUT("ReleaseSRWLockShared");
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			srw_lock_impl::release(SRWLock);
		}
		};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

extern "C" BOOLEAN WINAPI WRAPPER(TryAcquireSRWLockShared)(PSRWLOCK SRWLock)
{
	WRAPPER_DEBUG_OUTPUT("TryAcquireSRWLockShared");
	struct implementation
	{
		static BOOLEAN WINAPI impl(PSRWLOCK SRWLock)
		{
			return srw_lock_impl::try_acquire(SRWLock) ? TRUE : FALSE;
		}
		};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

extern "C" DWORD WINAPI WRAPPER(FlsAlloc)(PFLS_CALLBACK_FUNCTION Callback)
{
	WRAPPER_DEBUG_OUTPUT("FlsAlloc");
	struct implementation
	{
		static DWORD WINAPI impl(PFLS_CALLBACK_FUNCTION)
		{
			return TlsAlloc();
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Callback);
}

extern "C" PVOID WINAPI WRAPPER(FlsGetValue)(DWORD FlsIndex)
{
	WRAPPER_DEBUG_OUTPUT("FlsGetValue");
	struct implementation
	{
		static PVOID WINAPI impl(DWORD FlsIndex)
		{
			return TlsGetValue(FlsIndex);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, FlsIndex);
}

extern "C" BOOL WINAPI WRAPPER(FlsSetValue)(DWORD FlsIndex, PVOID FlsData)
{
	WRAPPER_DEBUG_OUTPUT("FlsSetValue");
	struct implementation
	{
		static BOOL WINAPI impl(DWORD FlsIndex, PVOID FlsData)
		{
			return TlsSetValue(FlsIndex, FlsData);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, FlsIndex, FlsData);
}

extern "C" BOOL WINAPI WRAPPER(FlsFree)(DWORD FlsIndex)
{
	WRAPPER_DEBUG_OUTPUT("FlsFree");
	struct implementation
	{
		static BOOL WINAPI impl(DWORD FlsIndex)
		{
			return TlsFree(FlsIndex);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, FlsIndex);
}

// Thread pool functions (Vista+)
extern "C" PTP_TIMER WINAPI WRAPPER(CreateThreadpoolTimer)(PTP_TIMER_CALLBACK Callback, PVOID Context, PTP_CALLBACK_ENVIRON CallbackEnviron)
{
	WRAPPER_DEBUG_OUTPUT("CreateThreadpoolTimer");
	struct implementation
	{
		static PTP_TIMER WINAPI impl(PTP_TIMER_CALLBACK, PVOID, PTP_CALLBACK_ENVIRON)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return nullptr;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Callback, Context, CallbackEnviron);
}

extern "C" void WINAPI WRAPPER(SetThreadpoolTimer)(PTP_TIMER Timer, PFILETIME DueTime, DWORD Period, DWORD WindowLength)
{
	WRAPPER_DEBUG_OUTPUT("SetThreadpoolTimer");
	struct implementation
	{
		static void WINAPI impl(PTP_TIMER, PFILETIME, DWORD, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Timer, DueTime, Period, WindowLength);
}

extern "C" void WINAPI WRAPPER(WaitForThreadpoolTimerCallbacks)(PTP_TIMER Timer, BOOL CancelPendingCallbacks)
{
	WRAPPER_DEBUG_OUTPUT("WaitForThreadpoolTimerCallbacks");
	struct implementation
	{
		static void WINAPI impl(PTP_TIMER, BOOL)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Timer, CancelPendingCallbacks);
}

extern "C" void WINAPI WRAPPER(CloseThreadpoolTimer)(PTP_TIMER Timer)
{
	WRAPPER_DEBUG_OUTPUT("CloseThreadpoolTimer");
	struct implementation
	{
		static void WINAPI impl(PTP_TIMER)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Timer);
}

extern "C" PTP_WAIT WINAPI WRAPPER(CreateThreadpoolWait)(PTP_WAIT_CALLBACK Callback, PVOID Context, PTP_CALLBACK_ENVIRON CallbackEnviron)
{
	WRAPPER_DEBUG_OUTPUT("CreateThreadpoolWait");
	struct implementation
	{
		static PTP_WAIT WINAPI impl(PTP_WAIT_CALLBACK, PVOID, PTP_CALLBACK_ENVIRON)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return nullptr;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Callback, Context, CallbackEnviron);
}

extern "C" void WINAPI WRAPPER(SetThreadpoolWait)(PTP_WAIT Wait, HANDLE Handle, PFILETIME Timeout)
{
	WRAPPER_DEBUG_OUTPUT("SetThreadpoolWait");
	struct implementation
	{
		static void WINAPI impl(PTP_WAIT, HANDLE, PFILETIME)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Wait, Handle, Timeout);
}

extern "C" void WINAPI WRAPPER(CloseThreadpoolWait)(PTP_WAIT Wait)
{
	WRAPPER_DEBUG_OUTPUT("CloseThreadpoolWait");
	struct implementation
	{
		static void WINAPI impl(PTP_WAIT)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Wait);
}

extern "C" void WINAPI WRAPPER(FreeLibraryWhenCallbackReturns)(PTP_CALLBACK_INSTANCE Instance, HMODULE Module)
{
	WRAPPER_DEBUG_OUTPUT("FreeLibraryWhenCallbackReturns");
	struct implementation
	{
		static void WINAPI impl(PTP_CALLBACK_INSTANCE, HMODULE)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Instance, Module);
}

// Sync primitives (Vista+)
extern "C" HANDLE WINAPI WRAPPER(CreateEventExW)(LPSECURITY_ATTRIBUTES lpEventAttributes, LPCWSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess)
{
	WRAPPER_DEBUG_OUTPUT("CreateEventExW");
	struct implementation
	{
		static HANDLE WINAPI impl(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return nullptr;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, lpEventAttributes, lpName, dwFlags, dwDesiredAccess);
}

extern "C" HANDLE WINAPI WRAPPER(CreateSemaphoreExW)(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName, DWORD dwFlags, DWORD dwDesiredAccess)
{
	WRAPPER_DEBUG_OUTPUT("CreateSemaphoreExW");
	struct implementation
	{
		static HANDLE WINAPI impl(LPSECURITY_ATTRIBUTES, LONG, LONG, LPCWSTR, DWORD, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return nullptr;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName, dwFlags, dwDesiredAccess);
}

extern "C" BOOL WINAPI WRAPPER(InitOnceBeginInitialize)(PINIT_ONCE InitOnce, DWORD Flags, PBOOL Pending, PVOID* Context)
{
	WRAPPER_DEBUG_OUTPUT("InitOnceBeginInitialize");
	struct implementation
	{
		static BOOL WINAPI impl(PINIT_ONCE InitOnce, DWORD Flags, PBOOL Pending, PVOID* Context)
		{
			(void)Flags; (void)Context;
			XPCompat_EnsureInitOnceCS();
			EnterCriticalSection(&g_XPCompatInitOnceCS);
			if (InitOnce->Ptr == (PVOID)XP_INIT_ONCE_DONE || InitOnce->Ptr == (PVOID)XP_INIT_ONCE_FAILED)
			{
				LeaveCriticalSection(&g_XPCompatInitOnceCS);
				*Pending = FALSE;
				return TRUE;
			}
			if (InitOnce->Ptr == (PVOID)XP_INIT_ONCE_BUSY)
			{
				LeaveCriticalSection(&g_XPCompatInitOnceCS);
				// Block until initialization completes, then report not-pending
				while (InitOnce->Ptr == (PVOID)XP_INIT_ONCE_BUSY)
				{
					Sleep(0);
				}
				*Pending = FALSE;
				return TRUE;
			}
			InitOnce->Ptr = (PVOID)XP_INIT_ONCE_BUSY;
			LeaveCriticalSection(&g_XPCompatInitOnceCS);
			*Pending = TRUE;
			return TRUE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, InitOnce, Flags, Pending, Context);
}

extern "C" BOOL WINAPI WRAPPER(InitOnceComplete)(PINIT_ONCE InitOnce, DWORD Flags, PVOID Context)
{
	WRAPPER_DEBUG_OUTPUT("InitOnceComplete");
	struct implementation
	{
		static BOOL WINAPI impl(PINIT_ONCE InitOnce, DWORD Flags, PVOID)
		{
			XPCompat_EnsureInitOnceCS();
			EnterCriticalSection(&g_XPCompatInitOnceCS);
			if (Flags & INIT_ONCE_INIT_FAILED)
				InitOnce->Ptr = (PVOID)XP_INIT_ONCE_FAILED;
			else
				InitOnce->Ptr = (PVOID)XP_INIT_ONCE_DONE;
			LeaveCriticalSection(&g_XPCompatInitOnceCS);
			return TRUE;
		}
		};
	CREATE_AND_RETURN(modules::kernel32, InitOnce, Flags, Context);
}

extern "C" BOOL WINAPI WRAPPER(InitOnceExecuteOnce)(PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, PVOID Parameter, PVOID* Context)
{
	WRAPPER_DEBUG_OUTPUT("InitOnceExecuteOnce");
	struct implementation
	{
		static BOOL WINAPI impl(PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, PVOID Parameter, PVOID*)
		{
			BOOL Pending;
			if (!Wrapper_InitOnceBeginInitialize(InitOnce, 0, &Pending, nullptr))
			{
				// Initialization failure (should not happen in our fallback)
				return TRUE;
			}
			if (!Pending)
			{
				// Already initialized by another thread
				return TRUE;
			}
			// Execute the callback
			BOOL Result = InitFn(InitOnce, Parameter, nullptr);
			if (Result)
				Wrapper_InitOnceComplete(InitOnce, 0, nullptr);
			else
				Wrapper_InitOnceComplete(InitOnce, INIT_ONCE_INIT_FAILED, nullptr);
			return Result;
		}
		};
	CREATE_AND_RETURN(modules::kernel32, InitOnce, InitFn, Parameter, Context);
}

// File functions (Vista+)
extern "C" BOOLEAN WINAPI WRAPPER(CreateSymbolicLinkW)(LPCWSTR lpSymlinkFileName, LPCWSTR lpTargetFileName, DWORD dwFlags)
{
	WRAPPER_DEBUG_OUTPUT("CreateSymbolicLinkW");
	struct implementation
	{
		static BOOLEAN WINAPI impl(LPCWSTR, LPCWSTR, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, lpSymlinkFileName, lpTargetFileName, dwFlags);
}

extern "C" BOOL WINAPI WRAPPER(GetFileInformationByHandleEx)(HANDLE hFile, DWORD FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
	WRAPPER_DEBUG_OUTPUT("GetFileInformationByHandleEx");
	struct implementation
	{
		static BOOL WINAPI impl(HANDLE, DWORD, LPVOID, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, hFile, FileInformationClass, lpFileInformation, dwBufferSize);
}

extern "C" BOOL WINAPI WRAPPER(SetFileInformationByHandle)(HANDLE hFile, DWORD FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize)
{
	WRAPPER_DEBUG_OUTPUT("SetFileInformationByHandle");
	struct implementation
	{
		static BOOL WINAPI impl(HANDLE, DWORD, LPVOID, DWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};
	CREATE_AND_RETURN(modules::kernel32, hFile, FileInformationClass, lpFileInformation, dwBufferSize);
}

// System info (Vista+)
extern "C" ULONGLONG WINAPI WRAPPER(GetTickCount64)()
{
	WRAPPER_DEBUG_OUTPUT("GetTickCount64");
	struct implementation
	{
		static ULONGLONG WINAPI impl()
		{
			// Fallback: use GetTickCount (wraps every 49.7 days)
			return static_cast<ULONGLONG>(GetTickCount());
		}
	};
	CREATE_AND_RETURN(modules::kernel32);
}

extern "C" DWORD WINAPI WRAPPER(GetCurrentProcessorNumber)()
{
	WRAPPER_DEBUG_OUTPUT("GetCurrentProcessorNumber");
	struct implementation
	{
		static DWORD WINAPI impl()
		{
			// Fallback: return 0 (first processor)
			return 0;
		}
	};
	CREATE_AND_RETURN(modules::kernel32);
}

extern "C" void WINAPI WRAPPER(FlushProcessWriteBuffers)()
{
	WRAPPER_DEBUG_OUTPUT("FlushProcessWriteBuffers");
	struct implementation
	{
		static void WINAPI impl()
		{
			// No-op on XP; memory barriers handled differently
		}
	};
	CREATE_AND_RETURN(modules::kernel32);
}

// System time (Vista+)
extern "C" void WINAPI WRAPPER(GetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime)
{
	WRAPPER_DEBUG_OUTPUT("GetSystemTimePreciseAsFileTime");
	struct implementation
	{
		static void WINAPI impl(LPFILETIME lpSystemTimeAsFileTime)
		{
			// Fallback: use GetSystemTimeAsFileTime (lower precision but available on XP)
			GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, lpSystemTimeAsFileTime);
}

// Locale (Vista+)
extern "C" int WINAPI WRAPPER(GetLocaleInfoEx)(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	WRAPPER_DEBUG_OUTPUT("GetLocaleInfoEx");
	struct implementation
	{
		static int WINAPI impl(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
		{
			return GetLocaleInfoW(locale_name_to_lcid(lpLocaleName), LCType, lpLCData, cchData);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, lpLocaleName, LCType, lpLCData, cchData);
}

// Locale (legacy)
extern "C" int WINAPI WRAPPER(GetLocaleInfoA)(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
	WRAPPER_DEBUG_OUTPUT("GetLocaleInfoA");
	struct implementation
	{
		static int WINAPI impl(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
		{
			return GetLocaleInfoA(Locale, LCType, lpLCData, cchData);
		}
	};
	CREATE_AND_RETURN(modules::kernel32, Locale, LCType, lpLCData, cchData);
}

#undef CREATE_AND_RETURN
#undef WRAPPER

// disable VS2015 telemetry
extern "C"
{
	void __vcrt_initialize_telemetry_provider() {}
	void __telemetry_main_invoke_trigger() {}
	void __telemetry_main_return_trigger() {}
	void __vcrt_uninitialize_telemetry_provider() {}
}
