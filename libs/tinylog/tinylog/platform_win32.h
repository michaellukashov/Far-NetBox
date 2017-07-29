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

#include <winsock2.h>
#include <windows.h>
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

#define pthread_t           HANDLE
#define pthread_attr_t      DWORD

#if 0
typedef __int8              int8_t;
typedef __int16             int16_t;
typedef __int32             int32_t;
typedef __int64             int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;

typedef uint64_t            off64_t;

#define lseek64             _lseeki64

#define FMT64               "I64"
#endif // #if 0

static int pthread_create(pthread_t* th, pthread_attr_t* attr, void* (*func) (void*), void* ctx)
{
  th[0] = (pthread_t)_beginthreadex(0, 0, (unsigned(__stdcall *)(void*))func, ctx, 0, 0);
  if (attr && attr[0])
    SetThreadAffinityMask(th[0], attr[0]);
  return 0;
}

static int pthread_join(pthread_t th, void** p)
{
  (void)p;
  WaitForSingleObject(th, INFINITE);
  CloseHandle(th);
  return 0;
}

static void* atomic_exchange_acq_rel_ptr(void** p, void* xchg)
{
  return (void*)_InterlockedExchange64((int64_t*)p, (int64_t)xchg);
}

static int get_cpu_count()
{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}

static void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off)
{
  (void)prot;
  (void)flags;
  (void)fd;
  (void)off;
  assert(fd == -1);
  return VirtualAlloc(addr, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

static int munmap(void* addr, size_t len)
{
  (void)len;
  VirtualFree(addr, 0, MEM_RELEASE);
  return 0;
}

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 4
#define O_LARGEFILE 0
#define MAP_SHARED 1
#define MAP_ANON 0

static size_t pread64(int fd, void* buf, size_t nbytes, uint64_t offset)
{
  _lseeki64(fd, (int64_t)offset, SEEK_SET);
  return _read(fd, buf, (uint32_t)nbytes);
}

#define pthread_spinlock_t CRITICAL_SECTION
#define pthread_mutex_t CRITICAL_SECTION

static int pthread_spin_init(pthread_spinlock_t* mtx, int pshared)
{
  (void)pshared;
  InitializeCriticalSection(mtx);
  return 0;
}

static int pthread_spin_destroy(pthread_spinlock_t* mtx)
{
  DeleteCriticalSection(mtx);
  return 0;
}

static int pthread_spin_lock(pthread_spinlock_t* mtx)
{
  EnterCriticalSection(mtx);
  return 0;
}

static int pthread_spin_unlock(pthread_spinlock_t* mtx)
{
  LeaveCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_init(pthread_mutex_t* mtx, void* attr)
{
  (void)attr;
  InitializeCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_destroy(pthread_mutex_t* mtx)
{
  DeleteCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_lock(pthread_mutex_t* mtx)
{
  EnterCriticalSection(mtx);
  return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t* mtx)
{
  LeaveCriticalSection(mtx);
  return 0;
}

#define sparc_prefetch_read_many(p)

typedef HANDLE sem_t;

static int sem_init(sem_t* sem, int pshared, unsigned value)
{
  (void)pshared;
  *sem = CreateSemaphoreW(0, value, 1 << 30, 0);
  return 0;
}

static int sem_destroy(sem_t* sem)
{
  CloseHandle(*sem);
  return 0;
}

static int sem_wait(sem_t* sem)
{
  WaitForSingleObject(*sem, INFINITE);
  return 0;
}

static int sem_post(sem_t* sem)
{
  ReleaseSemaphore(*sem, 1, 0);
  return 0;
}
