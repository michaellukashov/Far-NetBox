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

//typedef struct
//{
//  int waiters_count_;
//  // Number of waiting threads.

//  CRITICAL_SECTION waiters_count_lock_;
//  // Serialize access to <waiters_count_>.

//  HANDLE sema_;
//  // Semaphore used to queue up threads waiting for the condition to
//  // become signaled.

//  HANDLE waiters_done_;
//  // An auto-reset event used by the broadcast/signal thread to wait
//  // for all the waiting thread(s) to wake up and be released from the
//  // semaphore.

//  size_t was_broadcast_;
//  // Keeps track of whether we were broadcasting or signaling.  This
//  // allows us to optimize the code if we're just signaling.
//} pthread_cond_t;

typedef struct pthread_cond_t_
{
  u_int waiters_count_;
    // Count of the number of waiters.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.
  HANDLE sema_;
  // Semaphore used to queue up threads waiting for the condition to
  // become signaled.

  HANDLE waiters_done_;
  size_t was_broadcast_;

  enum {
    E_SIGNAL = 0,
    E_BROADCAST = 1,
    E_MAX_EVENTS = 2
  };

  HANDLE events_[E_MAX_EVENTS];
  // Signal and broadcast event HANDLEs.
} pthread_cond_t;

typedef struct pthread_condattr_t_
{
  int pshared;
} pthread_condattr_t;

//inline int pthread_cond_init (pthread_cond_t *cv,
//                   const pthread_condattr_t *)
//{
//  cv->waiters_count_ = 0;
//  cv->was_broadcast_ = 0;
//  cv->sema_ = CreateSemaphoreW (NULL,       // no security
//                                0,          // initially 0
//                                0x7fffffff, // max count
//                                NULL);      // unnamed
//  InitializeCriticalSection (&cv->waiters_count_lock_);
//  cv->waiters_done_ = CreateEvent (NULL,  // no security
//                                   FALSE, // auto-reset
//                                   FALSE, // non-signaled initially
//                                   NULL); // unnamed
//}

inline int pthread_cond_init(pthread_cond_t *cv,
                   const pthread_condattr_t *)
{
  // Create an auto-reset event.
  cv->events_[pthread_cond_t_::E_SIGNAL] = CreateEvent(NULL,  // no security
                                     FALSE, // auto-reset event
                                     FALSE, // non-signaled initially
                                     NULL); // unnamed

  // Create a manual-reset event.
  cv->events_[pthread_cond_t_::E_BROADCAST] = CreateEvent(NULL,  // no security
                                        TRUE,  // manual-reset
                                        FALSE, // non-signaled initially
                                        NULL); // unnamed
  return 0;
}

inline int pthread_cond_wait(pthread_cond_t *cv,
                   pthread_mutex_t *external_mutex)
{
  // Release the <external_mutex> here and wait for either event
  // to become signaled, due to <pthread_cond_signal> being
  // called or <pthread_cond_broadcast> being called.
  LeaveCriticalSection(external_mutex);
  WaitForMultipleObjects(2, // Wait on both <events_>
                          cv->events_,
                          FALSE, // Wait for either event to be signaled
                          INFINITE); // Wait "forever"

  // Reacquire the mutex before returning.
  EnterCriticalSection(external_mutex);
  return 0;
}

inline int pthread_cond_signal(pthread_cond_t *cv)
{
  EnterCriticalSection(&cv->waiters_count_lock_);
  int have_waiters = cv->waiters_count_ > 0;
  LeaveCriticalSection(&cv->waiters_count_lock_);

  // If there aren't any waiters, then this is a no-op.
  if (have_waiters)
    ReleaseSemaphore(cv->sema_, 1, 0);
  return 0;
}

inline int pthread_cond_broadcast(pthread_cond_t *cv)
{
  // This is needed to ensure that <waiters_count_> and <was_broadcast_> are
  // consistent relative to each other.
  EnterCriticalSection(&cv->waiters_count_lock_);
  int have_waiters = 0;

  if (cv->waiters_count_ > 0)
  {
    // We are broadcasting, even if there is just one waiter...
    // Record that we are broadcasting, which helps optimize
    // <pthread_cond_wait> for the non-broadcast case.
    cv->was_broadcast_ = 1;
    have_waiters = 1;
  }

  if (have_waiters)
  {
    // Wake up all the waiters atomically.
    ReleaseSemaphore(cv->sema_, cv->waiters_count_, 0);

    LeaveCriticalSection(&cv->waiters_count_lock_);

    // Wait for all the awakened threads to acquire the counting
    // semaphore.
    WaitForSingleObject(cv->waiters_done_, INFINITE);
    // This assignment is okay, even without the <waiters_count_lock_> held
    // because no other waiter threads can wake up to access it.
    cv->was_broadcast_ = 0;
  }
  else
    LeaveCriticalSection(&cv->waiters_count_lock_);
  return 0;
}
