/*  Wide Finder 2 Benchmark Implementation (http://wikis.sun.com/display/WideFinder)
 *  Copyright (c) 2010, Dmitry Vyukov
 *  Distributed under the terms of the GNU General Public License as published by the Free Software Foundation,
 *  either version 3 of the License, or (at your option) any later version.
 *  See: http://www.gnu.org/licenses
 */

#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64
#include <process.h>
#include <intrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <assert.h>

#include <stdint.h>
#include <time.h>

typedef HANDLE pthread_t;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4505) // unreferenced local function has been removed
#endif // _MSC_VER

static int pthread_join(pthread_t th, void **p)
{
  (void)p;
  WaitForSingleObject(th, INFINITE);
  CloseHandle(th);
  return 0;
}

typedef CRITICAL_SECTION pthread_mutex_t;

static int pthread_mutex_init(pthread_mutex_t *mtx, void *attr)
{
  (void)attr;
  InitializeCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_destroy(pthread_mutex_t *mtx)
{
  DeleteCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_lock(pthread_mutex_t *mtx)
{
  EnterCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t *mtx)
{
  LeaveCriticalSection(mtx);
  return 0;
}

static bool pthread_mutex_tryenter(pthread_mutex_t *mtx)
{
  return TryEnterCriticalSection(mtx) != FALSE;
}

typedef struct pthread_cond_t_
{
  HANDLE event_;
} pthread_cond_t;

typedef struct pthread_condattr_t_
{
  int pshared;
} pthread_condattr_t;

inline int pthread_cond_init(pthread_cond_t *cv,
  const pthread_condattr_t *)
{
  cv->event_ = CreateEventW(NULL, // no security
      FALSE,      // auto reset
      FALSE,      // initial state
      NULL);      // unnamed
  return 0;
}

inline int pthread_cond_timedwait(pthread_cond_t *cv,
  pthread_mutex_t *external_mutex, DWORD timeout_millisecs)
{
  // Release the <external_mutex> here and wait for event
  LeaveCriticalSection(external_mutex);
  DWORD res = WaitForSingleObject(
    cv->event_,
    timeout_millisecs);

  // Reacquire the mutex before returning.
  EnterCriticalSection(external_mutex);
  return res;
}

inline int pthread_cond_signal(pthread_cond_t *cv)
{
  SetEvent(cv->event_);
  return 0;
}

inline int pthread_cond_destroy(pthread_cond_t *cv)
{
  CloseHandle(cv->event_);
  return 0;
}

#ifndef _WINSOCK2API_

// MSVC defines this in winsock2.h!?
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

#endif //ifndef _WINSOCK2API_

inline int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime );
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER
