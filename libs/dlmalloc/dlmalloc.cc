/*
  This is a version (aka dlmalloc) of malloc/free/realloc written by
  Doug Lea and released to the public domain, as explained at
  http://creativecommons.org/publicdomain/zero/1.0/ Send questions,
  comments, complaints, performance data, etc to dl@cs.oswego.edu
*/

/*
  C++ version by Gregory Popovitch (greg7mdp@gmail.com) and released 
  to the public domain as well, as explained at
  http://creativecommons.org/publicdomain/zero/1.0/.

  based on Version 2.8.6 Wed Aug 29 06:57:58 2012  Doug Lea
           (ftp://gee.cs.oswego.edu/pub/misc/malloc.c)
*/

/*

* Quickstart

  This library is all in one file to simplify the most common usage:
  ftp it, compile it (-O3), and link it into another program. All of
  the compile-time options default to reasonable values for use on
  most platforms.  You might later want to step through various
  compile-time and dynamic tuning options.

  For convenience, an include file for code using this malloc is at:
     ftp://gee.cs.oswego.edu/pub/misc/malloc-2.8.6.h
  You don't really need this .h file unless you call functions not
  defined in your system include files.  The .h file contains only the
  excerpts from this file needed for using this malloc on ANSI C/C++
  systems, so long as you haven't changed compile-time options about
  naming and tuning parameters.  If you do, then you can create your
  own malloc.h that does include all settings by cutting at the point
  indicated below. Note that you may already by default be using a C
  library containing a malloc that is based on some version of this
  malloc (for example in linux). You might still want to use the one
  in this file to customize settings or to avoid overheads associated
  with library versions.

* Vital statistics:

  Supported pointer/size_t representation:       4 or 8 bytes
       size_t MUST be an unsigned type of the same width as
       pointers. (If you are using an ancient system that declares
       size_t as a signed type, or need it to be a different width
       than pointers, you can use a previous release of this malloc
       (e.g. 2.7.2) supporting these.)

  Alignment:                                     8 bytes (minimum)
       This suffices for nearly all current machines and C compilers.
       However, you can define MALLOC_ALIGNMENT to be wider than this
       if necessary (up to 128bytes), at the expense of using more space.

  Minimum overhead per allocated chunk:   4 or  8 bytes (if 4byte sizes)
                                          8 or 16 bytes (if 8byte sizes)
       Each malloced chunk has a hidden word of overhead holding size
       and status information, and additional cross-check word
       if FOOTERS is defined.

  Minimum allocated size: 4-byte ptrs:  16 bytes    (including overhead)
                          8-byte ptrs:  32 bytes    (including overhead)

       Even a request for zero bytes (i.e., malloc(0)) returns a
       pointer to something of the minimum allocatable size.
       The maximum overhead wastage (i.e., number of extra bytes
       allocated than were requested in malloc) is less than or equal
       to the minimum size, except for requests >= mmap_threshold that
       are serviced via mmap(), where the worst case wastage is about
       32 bytes plus the remainder from a system page (the minimal
       mmap unit); typically 4096 or 8192 bytes.

  Security: static-safe; optionally more or less
       The "security" of malloc refers to the ability of malicious
       code to accentuate the effects of errors (for example, freeing
       space that is not currently malloc'ed or overwriting past the
       ends of chunks) in code that calls malloc.  This malloc
       guarantees not to modify any memory locations below the base of
       heap, i.e., static variables, even in the presence of usage
       errors.  The routines additionally detect most improper frees
       and reallocs.  All this holds as long as the static bookkeeping
       for malloc itself is not corrupted by some other means.  This
       is only one aspect of security -- these checks do not, and
       cannot, detect all possible programming errors.

       If FOOTERS is defined nonzero, then each allocated chunk
       carries an additional check word to verify that it was malloced
       from its space.  These check words are the same within each
       execution of a program using malloc, but differ across
       executions, so externally crafted fake chunks cannot be
       freed. This improves security by rejecting frees/reallocs that
       could corrupt heap memory, in addition to the checks preventing
       writes to statics that are always on.  This may further improve
       security at the expense of time and space overhead.  (Note that
       FOOTERS may also be worth using with MSPACES.)

       By default detected errors cause the program to abort (calling
       "abort()"). You can override this to instead proceed past
       errors by defining PROCEED_ON_ERROR.  In this case, a bad free
       has no effect, and a malloc that encounters a bad address
       caused by user overwrites will ignore the bad address by
       dropping pointers and indices to all known memory. This may
       be appropriate for programs that should continue if at all
       possible in the face of programming errors, although they may
       run out of memory because dropped memory is never reclaimed.

       If you don't like either of these options, you can define
       CORRUPTION_ERROR_ACTION and USAGE_ERROR_ACTION to do anything
       else. And if if you are sure that your program using malloc has
       no errors or vulnerabilities, you can define INSECURE to 1,
       which might (or might not) provide a small performance improvement.

       It is also possible to limit the maximum total allocatable
       space, using malloc_set_footprint_limit. This is not
       designed as a security feature in itself (calls to set limits
       are not screened or privileged), but may be useful as one
       aspect of a secure implementation.

  Thread-safety: thread-safe unless USE_LOCKS defined to zero
       When USE_LOCKS is defined, each public call to malloc, free,
       etc is surrounded with a lock. By default, this uses a plain
       pthread mutex, win32 critical section, or a spin-lock if if
       available for the platform and not disabled by setting
       USE_SPIN_LOCKS=0.  However, if USE_RECURSIVE_LOCKS is defined,
       recursive versions are used instead (which are not required for
       base functionality but may be needed in layered extensions).
       Using a global lock is not especially fast, and can be a major
       bottleneck.  It is designed only to provide minimal protection
       in concurrent environments, and to provide a basis for
       extensions.  If you are using malloc in a concurrent program,
       consider instead using nedmalloc
       (http://www.nedprod.com/programs/portable/nedmalloc/) or
       ptmalloc (See http://www.malloc.de), which are derived from
       versions of this malloc.

  System requirements: Any combination of MORECORE and/or MMAP/MUNMAP
       This malloc can use unix sbrk or any emulation (invoked using
       the CALL_MORECORE macro) and/or mmap/munmap or any emulation
       (invoked using CALL_MMAP/CALL_MUNMAP) to get and release system
       memory.  On most unix systems, it tends to work best if both
       MORECORE and MMAP are enabled.  On Win32, it uses emulations
       based on VirtualAlloc. It also uses common C library functions
       like memset.

  Compliance: I believe it is compliant with the Single Unix Specification
       (See http://www.unix.org). Also SVID/XPG, ANSI C, and probably
       others as well.

* Overview of algorithms

  This is not the fastest, most space-conserving, most portable, or
  most tunable malloc ever written. However it is among the fastest
  while also being among the most space-conserving, portable and
  tunable.  Consistent balance across these factors results in a good
  general-purpose allocator for malloc-intensive programs.

  In most ways, this malloc is a best-fit allocator. Generally, it
  chooses the best-fitting existing chunk for a request, with ties
  broken in approximately least-recently-used order. (This strategy
  normally maintains low fragmentation.) However, for requests less
  than 256bytes, it deviates from best-fit when there is not an
  exactly fitting available chunk by preferring to use space adjacent
  to that used for the previous small request, as well as by breaking
  ties in approximately most-recently-used order. (These enhance
  locality of series of small allocations.)  And for very large requests
  (>= 256Kb by default), it relies on system memory mapping
  facilities, if supported.  (This helps avoid carrying around and
  possibly fragmenting memory used only for large chunks.)

  All operations (except malloc_stats and mallinfo) have execution
  times that are bounded by a constant factor of the number of bits in
  a size_t, not counting any clearing in calloc or copying in realloc,
  or actions surrounding MORECORE and MMAP that have times
  proportional to the number of non-contiguous regions returned by
  system allocation routines, which is often just 1. In real-time
  applications, you can optionally suppress segment traversals using
  NO_SEGMENT_TRAVERSAL, which assures bounded execution even when
  system allocators return non-contiguous spaces, at the typical
  expense of carrying around more memory and increased fragmentation.

  The implementation is not very modular and seriously overuses
  macros. Perhaps someday all C compilers will do as good a job
  inlining modular code as can now be done by brute-force expansion,
  but now, enough of them seem not to.

  Some compilers issue a lot of warnings about code that is
  dead/unreachable only on some platforms, and also about intentional
  uses of negation on unsigned types. All known cases of each can be
  ignored.

  For a longer but out of date high-level description, see
     http://gee.cs.oswego.edu/dl/html/malloc.html

* MSPACES
  If MSPACES is defined, then in addition to malloc, free, etc.,
  this file also defines mspace_malloc, mspace_free, etc. These
  are versions of malloc routines that take an "mspace" argument
  obtained using create_mspace, to control all internal bookkeeping.
  If ONLY_MSPACES is defined, only these versions are compiled.
  So if you would like to use this allocator for only some allocations,
  and your system malloc for others, you can compile with
  ONLY_MSPACES and then do something like...
    static mspace mymspace = create_mspace(0,0); // for example
    #define mymalloc(bytes)  mspace_malloc(mymspace, bytes)

  (Note: If you only need one instance of an mspace, you can instead
  use "USE_DL_PREFIX" to relabel the global malloc.)

  You can similarly create thread-local allocators by storing
  mspaces as thread-locals. For example:
    static __thread mspace tlms = 0;
    void*  tlmalloc(size_t bytes) {
      if (tlms == 0) tlms = create_mspace(0, 0);
      return mspace_malloc(tlms, bytes);
    }
    void  tlfree(void* mem) { mspace_free(tlms, mem); }

  Unless FOOTERS is defined, each mspace is completely independent.
  You cannot allocate from one and free to another (although
  conformance is only weakly checked, so usage errors are not always
  caught). If FOOTERS is defined, then each chunk carries around a tag
  indicating its originating mspace, and frees are directed to their
  originating spaces. Normally, this requires use of locks.

 -------------------------  Compile-time options ---------------------------

Be careful in setting #define values for numerical constants of type
size_t. On some systems, literal values are not automatically extended
to size_t precision unless they are explicitly casted. You can also
use the symbolic values MAX_SIZE_T, SIZE_T_ONE, etc below.

WIN32                    default: defined if _WIN32 defined
  Defining WIN32 sets up defaults for MS environment and compilers.
  Otherwise defaults are for unix. Beware that there seem to be some
  cases where this malloc might not be a pure drop-in replacement for
  Win32 malloc: Random-looking failures from Win32 GDI API's (eg;
  SetDIBits()) may be due to bugs in some video driver implementations
  when pixel buffers are malloc()ed, and the region spans more than
  one VirtualAlloc()ed region. Because dlmalloc uses a small (64Kb)
  default granularity, pixel buffers may straddle virtual allocation
  regions more often than when using the Microsoft allocator.  You can
  avoid this by using VirtualAlloc() and VirtualFree() for all pixel
  buffers rather than using malloc().  If this is not possible,
  recompile this malloc with a larger DEFAULT_GRANULARITY. Note:
  in cases where MSC and gcc (cygwin) are known to differ on WIN32,
  conditions use _MSC_VER to distinguish them.

DLMALLOC_EXPORT       default: extern
  Defines how public APIs are declared. If you want to export via a
  Windows DLL, you might define this as
    #define DLMALLOC_EXPORT extern  __declspec(dllexport)
  If you want a POSIX ELF shared object, you might use
    #define DLMALLOC_EXPORT extern __attribute__((visibility("default")))

MALLOC_ALIGNMENT         default: (size_t)(2 * sizeof(void *))
  Controls the minimum alignment for malloc'ed chunks.  It must be a
  power of two and at least 8, even on machines for which smaller
  alignments would suffice. It may be defined as larger than this
  though. Note however that code and data structures are optimized for
  the case of 8-byte alignment.

MSPACES                  default: 0 (false)
  If true, compile in support for independent allocation spaces.
  This is only supported if HAVE_MMAP is true.

ONLY_MSPACES             default: 0 (false)
  If true, only compile in mspace versions, not regular versions.

USE_LOCKS                default: 1 (true)
  Causes each call to each public routine to be surrounded with
  pthread or WIN32 mutex lock/unlock. (If set true, this can be
  overridden on a per-mspace basis for mspace versions.) If set to a
  non-zero value other than 1, locks are used, but their
  implementation is left out, so lock functions must be supplied manually,
  as described below.

USE_SPIN_LOCKS           default: 1 iff USE_LOCKS and spin locks available
  If true, uses custom spin locks for locking. This is currently
  supported only gcc >= 4.1, older gccs on x86 platforms, and recent
  MS compilers.  Otherwise, posix locks or win32 critical sections are
  used.

USE_RECURSIVE_LOCKS      default: not defined
  If defined nonzero, uses recursive (aka reentrant) locks, otherwise
  uses plain mutexes. This is not required for malloc proper, but may
  be needed for layered allocators such as nedmalloc.

LOCK_AT_FORK            default: not defined
  If defined nonzero, performs pthread_atfork upon initialization
  to initialize child lock while holding parent lock. The implementation
  assumes that pthread locks (not custom locks) are being used. In other
  cases, you may need to customize the implementation.

FOOTERS                  default: 0
  If true, provide extra checking and dispatching by placing
  information in the footers of allocated chunks. This adds
  space and time overhead.

INSECURE                 default: 0
  If true, omit checks for usage errors and heap space overwrites.

USE_DL_PREFIX            default: NOT defined
  Causes compiler to prefix all public routines with the string 'dl'.
  This can be useful when you only want to use this malloc in one part
  of a program, using your regular system malloc elsewhere.

MALLOC_INSPECT_ALL       default: NOT defined
  If defined, compiles malloc_inspect_all and mspace_inspect_all, that
  perform traversal of all heap space.  Unless access to these
  functions is otherwise restricted, you probably do not want to
  include them in secure implementations.

ABORT                    default: defined as abort()
  Defines how to abort on failed checks.  On most systems, a failed
  check cannot die with an "assert" or even print an informative
  message, because the underlying print routines in turn call malloc,
  which will fail again.  Generally, the best policy is to simply call
  abort(). It's not very useful to do more than this because many
  errors due to overwriting will show up as address faults (null, odd
  addresses etc) rather than malloc-triggered checks, so will also
  abort.  Also, most compilers know that abort() does not return, so
  can better optimize code conditionally calling it.

PROCEED_ON_ERROR           default: defined as 0 (false)
  Controls whether detected bad addresses cause them to bypassed
  rather than aborting. If set, detected bad arguments to free and
  realloc are ignored. And all bookkeeping information is zeroed out
  upon a detected overwrite of freed heap space, thus losing the
  ability to ever return it from malloc again, but enabling the
  application to proceed. If PROCEED_ON_ERROR is defined, the
  static variable malloc_corruption_error_count is compiled in
  and can be examined to see if errors have occurred. This option
  generates slower code than the default abort policy.

DEBUG                    default: NOT defined
  The DEBUG setting is mainly intended for people trying to modify
  this code or diagnose problems when porting to new platforms.
  However, it may also be able to better isolate user errors than just
  using runtime checks.  The assertions in the check routines spell
  out in more detail the assumptions and invariants underlying the
  algorithms.  The checking is fairly extensive, and will slow down
  execution noticeably. Calling malloc_stats or mallinfo with DEBUG
  set will attempt to check every non-mmapped allocated and free chunk
  in the course of computing the summaries.

ABORT_ON_ASSERT_FAILURE   default: defined as 1 (true)
  Debugging assertion failures can be nearly impossible if your
  version of the assert macro causes malloc to be called, which will
  lead to a cascade of further failures, blowing the runtime stack.
  ABORT_ON_ASSERT_FAILURE cause assertions failures to call abort(),
  which will usually make debugging easier.

MALLOC_FAILURE_ACTION     default: sets errno to ENOMEM, or no-op on win32
  The action to take before "return 0" when malloc fails to be able to
  return memory because there is none available.

HAVE_MORECORE             default: 1 (true) unless win32 or ONLY_MSPACES
  True if this system supports sbrk or an emulation of it.

MORECORE                  default: sbrk
  The name of the sbrk-style system routine to call to obtain more
  memory.  See below for guidance on writing custom MORECORE
  functions. The type of the argument to sbrk/MORECORE varies across
  systems.  It cannot be size_t, because it supports negative
  arguments, so it is normally the signed type of the same width as
  size_t (sometimes declared as "intptr_t").  It doesn't much matter
  though. Internally, we only call it with arguments less than half
  the max value of a size_t, which should work across all reasonable
  possibilities, although sometimes generating compiler warnings.

MORECORE_CONTIGUOUS       default: 1 (true) if HAVE_MORECORE
  If true, take advantage of fact that consecutive calls to MORECORE
  with positive arguments always return contiguous increasing
  addresses.  This is true of unix sbrk. It does not hurt too much to
  set it true anyway, since malloc copes with non-contiguities.
  Setting it false when definitely non-contiguous saves time
  and possibly wasted space it would take to discover this though.

MORECORE_CANNOT_TRIM      default: NOT defined
  True if MORECORE cannot release space back to the system when given
  negative arguments. This is generally necessary only if you are
  using a hand-crafted MORECORE function that cannot handle negative
  arguments.

NO_SEGMENT_TRAVERSAL       default: 0
  If non-zero, suppresses traversals of memory segments
  returned by either MORECORE or CALL_MMAP. This disables
  merging of segments that are contiguous, and selectively
  releasing them to the OS if unused, but bounds execution times.

HAVE_MMAP                 default: 1 (true)
  True if this system supports mmap or an emulation of it.  If so, and
  HAVE_MORECORE is not true, MMAP is used for all system
  allocation. If set and HAVE_MORECORE is true as well, MMAP is
  primarily used to directly allocate very large blocks. It is also
  used as a backup strategy in cases where MORECORE fails to provide
  space from system. Note: A single call to MUNMAP is assumed to be
  able to unmap memory that may have be allocated using multiple calls
  to MMAP, so long as they are adjacent.

HAVE_MREMAP               default: 1 on linux, else 0
  If true realloc() uses mremap() to re-allocate large blocks and
  extend or shrink allocation spaces.

MMAP_CLEARS               default: 1 except on WINCE.
  True if mmap clears memory so calloc doesn't need to. This is true
  for standard unix mmap using /dev/zero and on WIN32 except for WINCE.

USE_BUILTIN_FFS            default: 0 (i.e., not used)
  Causes malloc to use the builtin ffs() function to compute indices.
  Some compilers may recognize and intrinsify ffs to be faster than the
  supplied C version. Also, the case of x86 using gcc is special-cased
  to an asm instruction, so is already as fast as it can be, and so
  this setting has no effect. Similarly for Win32 under recent MS compilers.
  (On most x86s, the asm version is only slightly faster than the C version.)

malloc_getpagesize         default: derive from system includes, or 4096.
  The system page size. To the extent possible, this malloc manages
  memory from the system in page-size units.  This may be (and
  usually is) a function rather than a constant. This is ignored
  if WIN32, where page size is determined using getSystemInfo during
  initialization.

USE_DEV_RANDOM             default: 0 (i.e., not used)
  Causes malloc to use /dev/random to initialize secure magic seed for
  stamping footers. Otherwise, the current time is used.

NO_MALLINFO                default: 0
  If defined, don't compile "mallinfo". This can be a simple way
  of dealing with mismatches between system declarations and
  those in this file.

MALLINFO_FIELD_TYPE        default: size_t
  The type of the fields in the mallinfo struct. This was originally
  defined as "int" in SVID etc, but is more usefully defined as
  size_t. The value is used only if  HAVE_USR_INCLUDE_MALLOC_H is not set

NO_MALLOC_STATS            default: 0
  If defined, don't compile "malloc_stats". This avoids calls to
  fprintf and bringing in stdio dependencies you might not want.

REALLOC_ZERO_BYTES_FREES    default: not defined
  This should be set if a call to realloc with zero bytes should
  be the same as a call to free. Some people think it should. Otherwise,
  since this malloc returns a unique pointer for malloc(0), so does
  realloc(p, 0).

LACKS_UNISTD_H, LACKS_FCNTL_H, LACKS_SYS_PARAM_H, LACKS_SYS_MMAN_H
LACKS_STRINGS_H, LACKS_STRING_H, LACKS_SYS_TYPES_H,  LACKS_ERRNO_H
LACKS_STDLIB_H LACKS_SCHED_H LACKS_TIME_H  default: NOT defined unless on WIN32
  Define these if your system does not have these header files.
  You might need to manually insert some of the declarations they provide.

DEFAULT_GRANULARITY        default: page size if MORECORE_CONTIGUOUS,
                                system_info.dwAllocationGranularity in WIN32,
                                otherwise 64K.
      Also settable using mallopt(M_GRANULARITY, x)
  The unit for allocating and deallocating memory from the system.  On
  most systems with contiguous MORECORE, there is no reason to
  make this more than a page. However, systems with MMAP tend to
  either require or encourage larger granularities.  You can increase
  this value to prevent system allocation functions to be called so
  often, especially if they are slow.  The value must be at least one
  page and must be a power of two.  Setting to 0 causes initialization
  to either page size or win32 region size.  (Note: In previous
  versions of malloc, the equivalent of this option was called
  "TOP_PAD")

DEFAULT_TRIM_THRESHOLD    default: 2MB
      Also settable using mallopt(M_TRIM_THRESHOLD, x)
  The maximum amount of unused top-most memory to keep before
  releasing via malloc_trim in free().  Automatic trimming is mainly
  useful in long-lived programs using contiguous MORECORE.  Because
  trimming via sbrk can be slow on some systems, and can sometimes be
  wasteful (in cases where programs immediately afterward allocate
  more large chunks) the value should be high enough so that your
  overall system performance would improve by releasing this much
  memory.  As a rough guide, you might set to a value close to the
  average size of a process (program) running on your system.
  Releasing this much memory would allow such a process to run in
  memory.  Generally, it is worth tuning trim thresholds when a
  program undergoes phases where several large chunks are allocated
  and released in ways that can reuse each other's storage, perhaps
  mixed with phases where there are no such chunks at all. The trim
  value must be greater than page size to have any useful effect.  To
  disable trimming completely, you can set to MAX_SIZE_T. Note that the trick
  some people use of mallocing a huge space and then freeing it at
  program startup, in an attempt to reserve system memory, doesn't
  have the intended effect under automatic trimming, since that memory
  will immediately be returned to the system.

DEFAULT_MMAP_THRESHOLD       default: 256K
      Also settable using mallopt(M_MMAP_THRESHOLD, x)
  The request size threshold for using MMAP to directly service a
  request. Requests of at least this size that cannot be allocated
  using already-existing space will be serviced via mmap.  (If enough
  normal freed space already exists it is used instead.)  Using mmap
  segregates relatively large chunks of memory so that they can be
  individually obtained and released from the host system. A request
  serviced through mmap is never reused by any other request (at least
  not directly; the system may just so happen to remap successive
  requests to the same locations).  Segregating space in this way has
  the benefits that: Mmapped space can always be individually released
  back to the system, which helps keep the system level memory demands
  of a long-lived program low.  Also, mapped memory doesn't become
  `locked' between other chunks, as can happen with normally allocated
  chunks, which means that even trimming via malloc_trim would not
  release them.  However, it has the disadvantage that the space
  cannot be reclaimed, consolidated, and then used to service later
  requests, as happens with normal chunks.  The advantages of mmap
  nearly always outweigh disadvantages for "large" chunks, but the
  value of "large" may vary across systems.  The default is an
  empirically derived value that works well in most systems. You can
  disable mmap by setting to MAX_SIZE_T.

MAX_RELEASE_CHECK_RATE   default: 4095 unless not HAVE_MMAP
  The number of consolidated frees between checks to release
  unused segments when freeing. When using non-contiguous segments,
  especially with multiple mspaces, checking only for topmost space
  doesn't always suffice to trigger trimming. To compensate for this,
  free() will, with a period of MAX_RELEASE_CHECK_RATE (or the
  current number of segments, if greater) try to release unused
  segments to the OS when freeing chunks that result in
  consolidation. The best value for this parameter is a compromise
  between slowing down frees with relatively costly checks that
  rarely trigger versus holding on to unused memory. To effectively
  disable, set to MAX_SIZE_T. This may lead to a very slight speed
  improvement at the expense of carrying around more memory.
*/

/* Version identifier to allow people to support multiple versions */
#ifndef DLMALLOC_VERSION
#define DLMALLOC_VERSION 20806
#endif /* DLMALLOC_VERSION */

#ifndef DLMALLOC_EXPORT
#define DLMALLOC_EXPORT extern
#endif

#ifndef WIN32
#ifdef _WIN32
#define WIN32 1
#endif  /* _WIN32 */
#ifdef _WIN32_WCE
#define LACKS_FCNTL_H
#define WIN32 1
#endif /* _WIN32_WCE */
#endif  /* WIN32 */
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#define HAVE_MMAP 1
#define HAVE_MORECORE 0
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#define LACKS_STRING_H
#define LACKS_STRINGS_H
#define LACKS_SYS_TYPES_H
//#define LACKS_ERRNO_H
#define LACKS_SCHED_H
#ifndef MALLOC_FAILURE_ACTION
#define MALLOC_FAILURE_ACTION
#endif /* MALLOC_FAILURE_ACTION */
#ifndef MMAP_CLEARS
#ifdef _WIN32_WCE /* WINCE reportedly does not clear */
#define MMAP_CLEARS 0
#else
#define MMAP_CLEARS 1
#endif /* _WIN32_WCE */
#endif /*MMAP_CLEARS */
#endif  /* WIN32 */

#if defined(DARWIN) || defined(_DARWIN)
/* Mac OSX docs advise not to use sbrk; it seems better to use mmap */
#ifndef HAVE_MORECORE
#define HAVE_MORECORE 0
#define HAVE_MMAP 1
/* OSX allocators provide 16 byte alignment */
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT ((size_t)16U)
#endif
#endif  /* HAVE_MORECORE */
#endif  /* DARWIN */

#if defined __cplusplus && (__GNUC__ >= 3 || __GNUC_MINOR__ >= 8) && !defined(__CYGWIN__)
    #define DLTHROW	throw ()
#else
    #define DLTHROW
#endif

#ifndef LACKS_SYS_TYPES_H
    #include <sys/types.h>  /* For size_t */
#endif  /* LACKS_SYS_TYPES_H */

/* The maximum possible size_t value has all bits set */
#define MAX_SIZE_T           (~(size_t)0)

#ifndef USE_LOCKS
    #define USE_LOCKS  1
#endif 

#if USE_LOCKS /* Spin locks for gcc >= 4.1, older gcc on x86, MSC >= 1310 */
#if ((defined(__GNUC__) &&                                              \
      ((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)) ||      \
       defined(__i386__) || defined(__x86_64__))) ||                    \
     (defined(_MSC_VER) && _MSC_VER>=1310))
#ifndef USE_SPIN_LOCKS
#define USE_SPIN_LOCKS 1
#endif /* USE_SPIN_LOCKS */
#elif USE_SPIN_LOCKS
#error "USE_SPIN_LOCKS defined without implementation"
#endif /* ... locks available... */
#elif !defined(USE_SPIN_LOCKS)
#define USE_SPIN_LOCKS 0
#endif /* USE_LOCKS */

#ifndef ONLY_MSPACES
#define ONLY_MSPACES 0
#endif  /* ONLY_MSPACES */
#ifndef MSPACES
#if ONLY_MSPACES
#define MSPACES 1
#else   /* ONLY_MSPACES */
#define MSPACES 0
#endif  /* ONLY_MSPACES */
#endif  /* MSPACES */
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT ((size_t)(2 * sizeof(void *)))
#endif  /* MALLOC_ALIGNMENT */
#ifndef FOOTERS
#define FOOTERS 0
#endif  /* FOOTERS */
#ifndef ABORT
#define ABORT  abort()
#endif  /* ABORT */
#ifndef ABORT_ON_ASSERT_FAILURE
#define ABORT_ON_ASSERT_FAILURE 1
#endif  /* ABORT_ON_ASSERT_FAILURE */
#ifndef PROCEED_ON_ERROR
#define PROCEED_ON_ERROR 0
#endif  /* PROCEED_ON_ERROR */

#ifndef INSECURE
#define INSECURE 0
#endif  /* INSECURE */
#ifndef MALLOC_INSPECT_ALL
#define MALLOC_INSPECT_ALL 0
#endif  /* MALLOC_INSPECT_ALL */
#ifndef HAVE_MMAP
#define HAVE_MMAP 1
#endif  /* HAVE_MMAP */
#ifndef MMAP_CLEARS
#define MMAP_CLEARS 1
#endif  /* MMAP_CLEARS */
#ifndef HAVE_MREMAP
#ifdef linux
#define HAVE_MREMAP 1
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE /* Turns on mremap() definition */
#endif
#else   /* linux */
#define HAVE_MREMAP 0
#endif  /* linux */
#endif  /* HAVE_MREMAP */
#ifndef MALLOC_FAILURE_ACTION
#define MALLOC_FAILURE_ACTION  errno = ENOMEM;
#endif  /* MALLOC_FAILURE_ACTION */
#ifndef HAVE_MORECORE
#if ONLY_MSPACES
#define HAVE_MORECORE 0
#else   /* ONLY_MSPACES */
#define HAVE_MORECORE 1
#endif  /* ONLY_MSPACES */
#endif  /* HAVE_MORECORE */
#if !HAVE_MORECORE
#define MORECORE_CONTIGUOUS 0
#else   /* !HAVE_MORECORE */
#define MORECORE_DEFAULT sbrk
#ifndef MORECORE_CONTIGUOUS
#define MORECORE_CONTIGUOUS 1
#endif  /* MORECORE_CONTIGUOUS */
#endif  /* HAVE_MORECORE */
#ifndef DEFAULT_GRANULARITY
#if (MORECORE_CONTIGUOUS || defined(WIN32))
#define DEFAULT_GRANULARITY (0)  /* 0 means to compute in init_mparams */
#else   /* MORECORE_CONTIGUOUS */
#define DEFAULT_GRANULARITY ((size_t)64U * (size_t)1024U)
#endif  /* MORECORE_CONTIGUOUS */
#endif  /* DEFAULT_GRANULARITY */
#ifndef DEFAULT_TRIM_THRESHOLD
#ifndef MORECORE_CANNOT_TRIM
#define DEFAULT_TRIM_THRESHOLD ((size_t)2U * (size_t)1024U * (size_t)1024U)
#else   /* MORECORE_CANNOT_TRIM */
#define DEFAULT_TRIM_THRESHOLD MAX_SIZE_T
#endif  /* MORECORE_CANNOT_TRIM */
#endif  /* DEFAULT_TRIM_THRESHOLD */
#ifndef DEFAULT_MMAP_THRESHOLD
#if HAVE_MMAP
#define DEFAULT_MMAP_THRESHOLD ((size_t)256U * (size_t)1024U)
#else   /* HAVE_MMAP */
#define DEFAULT_MMAP_THRESHOLD MAX_SIZE_T
#endif  /* HAVE_MMAP */
#endif  /* DEFAULT_MMAP_THRESHOLD */
#ifndef MAX_RELEASE_CHECK_RATE
#if HAVE_MMAP
#define MAX_RELEASE_CHECK_RATE 4095
#else
#define MAX_RELEASE_CHECK_RATE MAX_SIZE_T
#endif /* HAVE_MMAP */
#endif /* MAX_RELEASE_CHECK_RATE */
#ifndef USE_BUILTIN_FFS
#define USE_BUILTIN_FFS 0
#endif  /* USE_BUILTIN_FFS */
#ifndef USE_DEV_RANDOM
#define USE_DEV_RANDOM 0
#endif  /* USE_DEV_RANDOM */
#ifndef NO_MALLINFO
#define NO_MALLINFO 0
#endif  /* NO_MALLINFO */
#ifndef MALLINFO_FIELD_TYPE
#define MALLINFO_FIELD_TYPE size_t
#endif  /* MALLINFO_FIELD_TYPE */
#ifndef NO_MALLOC_STATS
#define NO_MALLOC_STATS 0
#endif  /* NO_MALLOC_STATS */
#ifndef NO_SEGMENT_TRAVERSAL
#define NO_SEGMENT_TRAVERSAL 0
#endif /* NO_SEGMENT_TRAVERSAL */

/*
  mallopt tuning options.  SVID/XPG defines four standard parameter
  numbers for mallopt, normally defined in malloc.h.  None of these
  are used in this malloc, so setting them has no effect. But this
  malloc does support the following options.
*/

#define M_TRIM_THRESHOLD     (-1)
#define M_GRANULARITY        (-2)
#define M_MMAP_THRESHOLD     (-3)

/* ------------------------ Mallinfo declarations ------------------------ */

#if !NO_MALLINFO
/*
  This version of malloc supports the standard SVID/XPG mallinfo
  routine that returns a struct containing usage properties and
  statistics. It should work on any system that has a
  /usr/include/malloc.h defining struct mallinfo.  The main
  declaration needed is the mallinfo struct that is returned (by-copy)
  by mallinfo().  The malloinfo struct contains a bunch of fields that
  are not even meaningful in this version of malloc.  These fields are
  are instead filled by mallinfo() with other numbers that might be of
  interest.

  HAVE_USR_INCLUDE_MALLOC_H should be set if you have a
  /usr/include/malloc.h file that includes a declaration of struct
  mallinfo.  If so, it is included; else a compliant version is
  declared below.  These must be precisely the same for mallinfo() to
  work.  The original SVID version of this struct, defined on most
  systems with mallinfo, declares all fields as ints. But some others
  define as unsigned long. If your system defines the fields using a
  type of different width than listed here, you MUST #include your
  system version and #define HAVE_USR_INCLUDE_MALLOC_H.
*/

/* #define HAVE_USR_INCLUDE_MALLOC_H */

#ifdef HAVE_USR_INCLUDE_MALLOC_H
#include "/usr/include/malloc.h"
#else /* HAVE_USR_INCLUDE_MALLOC_H */
#ifndef STRUCT_MALLINFO_DECLARED
/* HP-UX (and others?) redefines mallinfo unless _STRUCT_MALLINFO is defined */
#define _STRUCT_MALLINFO
#define STRUCT_MALLINFO_DECLARED 1
struct mallinfo {
    MALLINFO_FIELD_TYPE arena;    /* non-mmapped space allocated from system */
    MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
    MALLINFO_FIELD_TYPE smblks;   /* always 0 */
    MALLINFO_FIELD_TYPE hblks;    /* always 0 */
    MALLINFO_FIELD_TYPE hblkhd;   /* space in mmapped regions */
    MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
    MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
    MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
    MALLINFO_FIELD_TYPE fordblks; /* total free space */
    MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
};
#endif /* STRUCT_MALLINFO_DECLARED */
#endif /* HAVE_USR_INCLUDE_MALLOC_H */
#endif /* NO_MALLINFO */

/*
  Try to persuade compilers to inline. The most critical functions for
  inlining are defined as macros, so these aren't used for them.
*/

#ifndef FORCEINLINE
  #if defined(__GNUC__)
      #define FORCEINLINE __inline __attribute__ ((always_inline))
  #elif defined(_MSC_VER)
      #define FORCEINLINE __forceinline
  #else
      #define FORCEINLINE inline
  #endif
#endif

#ifndef NOINLINE
    #if defined(__GNUC__)
        #define NOINLINE __attribute__ ((noinline))
    #elif defined(_MSC_VER)
        #define NOINLINE __declspec(noinline)
    #else
        #define NOINLINE
    #endif
#endif

extern "C" {

#if !ONLY_MSPACES

/* ------------------- Declarations of public routines ------------------- */

#ifndef USE_DL_PREFIX
#define dlcalloc               calloc
#define dlfree                 free
#define dlmalloc               malloc
#define dlmemalign             memalign
#define dlposix_memalign       posix_memalign
#define dlrealloc              realloc
#define dlrealloc_in_place     realloc_in_place
#define dlvalloc               valloc
#define dlpvalloc              pvalloc
#define dlmallinfo             mallinfo
#define dlmallopt              mallopt
#define dlmalloc_trim          malloc_trim
#define dlmalloc_stats         malloc_stats
#define dlmalloc_usable_size   malloc_usable_size
#define dlmalloc_footprint     malloc_footprint
#define dlmalloc_max_footprint malloc_max_footprint
#define dlmalloc_footprint_limit malloc_footprint_limit
#define dlmalloc_set_footprint_limit malloc_set_footprint_limit
#define dlmalloc_inspect_all   malloc_inspect_all
#define dlindependent_calloc   independent_calloc
#define dlindependent_comalloc independent_comalloc
#define dlbulk_free            bulk_free
#endif /* USE_DL_PREFIX */

/*
  malloc(size_t n)
  Returns a pointer to a newly allocated chunk of at least n bytes, or
  null if no space is available, in which case errno is set to ENOMEM
  on ANSI C systems.

  If n is zero, malloc returns a minimum-sized chunk. (The minimum
  size is 16 bytes on most 32bit systems, and 32 bytes on 64bit
  systems.)  Note that size_t is an unsigned type, so calls with
  arguments that would be negative if signed are interpreted as
  requests for huge amounts of space, which will often fail. The
  maximum supported value of n differs across systems, but is in all
  cases less than the maximum representable value of a size_t.
*/
DLMALLOC_EXPORT void* dlmalloc(size_t) DLTHROW;

/*
  free(void* p)
  Releases the chunk of memory pointed to by p, that had been previously
  allocated using malloc or a related routine such as realloc.
  It has no effect if p is null. If p was not malloced or already
  freed, free(p) will by default cause the current program to abort.
*/
DLMALLOC_EXPORT void  dlfree(void*) DLTHROW;

/*
  calloc(size_t n_elements, size_t element_size);
  Returns a pointer to n_elements * element_size bytes, with all locations
  set to zero.
*/
DLMALLOC_EXPORT void* dlcalloc(size_t, size_t) DLTHROW;

/*
  realloc(void* p, size_t n)
  Returns a pointer to a chunk of size n that contains the same data
  as does chunk p up to the minimum of (n, p's size) bytes, or null
  if no space is available.

  The returned pointer may or may not be the same as p. The algorithm
  prefers extending p in most cases when possible, otherwise it
  employs the equivalent of a malloc-copy-free sequence.

  If p is null, realloc is equivalent to malloc.

  If space is not available, realloc returns null, errno is set (if on
  ANSI) and p is NOT freed.

  if n is for fewer bytes than already held by p, the newly unused
  space is lopped off and freed if possible.  realloc with a size
  argument of zero (re)allocates a minimum-sized chunk.

  The old unix realloc convention of allowing the last-free'd chunk
  to be used as an argument to realloc is not supported.
*/
DLMALLOC_EXPORT void* dlrealloc(void*, size_t) DLTHROW;

/*
  realloc_in_place(void* p, size_t n)
  Resizes the space allocated for p to size n, only if this can be
  done without moving p (i.e., only if there is adjacent space
  available if n is greater than p's current allocated size, or n is
  less than or equal to p's size). This may be used instead of plain
  realloc if an alternative allocation strategy is needed upon failure
  to expand space; for example, reallocation of a buffer that must be
  memory-aligned or cleared. You can use realloc_in_place to trigger
  these alternatives only when needed.

  Returns p if successful; otherwise null.
*/
DLMALLOC_EXPORT void* dlrealloc_in_place(void*, size_t);

/*
  memalign(size_t alignment, size_t n);
  Returns a pointer to a newly allocated chunk of n bytes, aligned
  in accord with the alignment argument.

  The alignment argument should be a power of two. If the argument is
  not a power of two, the nearest greater power is used.
  8-byte alignment is guaranteed by normal malloc calls, so don't
  bother calling memalign with an argument of 8 or less.

  Overreliance on memalign is a sure way to fragment space.
*/
DLMALLOC_EXPORT void* dlmemalign(size_t, size_t);

/*
  int posix_memalign(void** pp, size_t alignment, size_t n);
  Allocates a chunk of n bytes, aligned in accord with the alignment
  argument. Differs from memalign only in that it (1) assigns the
  allocated memory to *pp rather than returning it, (2) fails and
  returns EINVAL if the alignment is not a power of two (3) fails and
  returns ENOMEM if memory cannot be allocated.
*/
DLMALLOC_EXPORT int dlposix_memalign(void**, size_t, size_t) DLTHROW;

/*
  valloc(size_t n);
  Equivalent to memalign(pagesize, n), where pagesize is the page
  size of the system. If the pagesize is unknown, 4096 is used.
*/
DLMALLOC_EXPORT void* dlvalloc(size_t) DLTHROW;

/*
  mallopt(int parameter_number, int parameter_value)
  Sets tunable parameters The format is to provide a
  (parameter-number, parameter-value) pair.  mallopt then sets the
  corresponding parameter to the argument value if it can (i.e., so
  long as the value is meaningful), and returns 1 if successful else
  0.  To workaround the fact that mallopt is specified to use int,
  not size_t parameters, the value -1 is specially treated as the
  maximum unsigned size_t value.

  SVID/XPG/ANSI defines four standard param numbers for mallopt,
  normally defined in malloc.h.  None of these are use in this malloc,
  so setting them has no effect. But this malloc also supports other
  options in mallopt. See below for details.  Briefly, supported
  parameters are as follows (listed defaults are for "typical"
  configurations).

  Symbol            param #  default    allowed param values
  M_TRIM_THRESHOLD     -1   2*1024*1024   any   (-1 disables)
  M_GRANULARITY        -2     page size   any power of 2 >= page size
  M_MMAP_THRESHOLD     -3      256*1024   any   (or 0 if no MMAP support)
*/
DLMALLOC_EXPORT int dlmallopt(int, int);

/*
  malloc_footprint();
  Returns the number of bytes obtained from the system.  The total
  number of bytes allocated by malloc, realloc etc., is less than this
  value. Unlike mallinfo, this function returns only a precomputed
  result, so can be called frequently to monitor memory consumption.
  Even if locks are otherwise defined, this function does not use them,
  so results might not be up to date.
*/
DLMALLOC_EXPORT size_t dlmalloc_footprint(void);

/*
  malloc_max_footprint();
  Returns the maximum number of bytes obtained from the system. This
  value will be greater than current footprint if deallocated space
  has been reclaimed by the system. The peak number of bytes allocated
  by malloc, realloc etc., is less than this value. Unlike mallinfo,
  this function returns only a precomputed result, so can be called
  frequently to monitor memory consumption.  Even if locks are
  otherwise defined, this function does not use them, so results might
  not be up to date.
*/
DLMALLOC_EXPORT size_t dlmalloc_max_footprint(void);

/*
  malloc_footprint_limit();
  Returns the number of bytes that the heap is allowed to obtain from
  the system, returning the last value returned by
  malloc_set_footprint_limit, or the maximum size_t value if
  never set. The returned value reflects a permission. There is no
  guarantee that this number of bytes can actually be obtained from
  the system.
*/
DLMALLOC_EXPORT size_t dlmalloc_footprint_limit();

/*
  malloc_set_footprint_limit();
  Sets the maximum number of bytes to obtain from the system, causing
  failure returns from malloc and related functions upon attempts to
  exceed this value. The argument value may be subject to page
  rounding to an enforceable limit; this actual value is returned.
  Using an argument of the maximum possible size_t effectively
  disables checks. If the argument is less than or equal to the
  current malloc_footprint, then all future allocations that require
  additional system memory will fail. However, invocation cannot
  retroactively deallocate existing used memory.
*/
DLMALLOC_EXPORT size_t dlmalloc_set_footprint_limit(size_t bytes);

#if MALLOC_INSPECT_ALL
/*
  malloc_inspect_all(void(*handler)(void *start,
                                    void *end,
                                    size_t used_bytes,
                                    void* callback_arg),
                      void* arg);
  Traverses the heap and calls the given handler for each managed
  region, skipping all bytes that are (or may be) used for bookkeeping
  purposes.  Traversal does not include include chunks that have been
  directly memory mapped. Each reported region begins at the start
  address, and continues up to but not including the end address.  The
  first used_bytes of the region contain allocated data. If
  used_bytes is zero, the region is unallocated. The handler is
  invoked with the given callback argument. If locks are defined, they
  are held during the entire traversal. It is a bad idea to invoke
  other malloc functions from within the handler.

  For example, to count the number of in-use chunks with size greater
  than 1000, you could write:
  static int count = 0;
  void count_chunks(void* start, void* end, size_t used, void* arg) {
    if (used >= 1000) ++count;
  }
  then:
    malloc_inspect_all(count_chunks, NULL);

  malloc_inspect_all is compiled only if MALLOC_INSPECT_ALL is defined.
*/
DLMALLOC_EXPORT void dlmalloc_inspect_all(void(*handler)(void*, void *, size_t, void*),
                           void* arg);

#endif /* MALLOC_INSPECT_ALL */

#if !NO_MALLINFO
/*
  mallinfo()
  Returns (by copy) a struct containing various summary statistics:

  arena:     current total non-mmapped bytes allocated from system
  ordblks:   the number of free chunks
  smblks:    always zero.
  hblks:     current number of mmapped regions
  hblkhd:    total bytes held in mmapped regions
  usmblks:   the maximum total allocated space. This will be greater
                than current total if trimming has occurred.
  fsmblks:   always zero
  uordblks:  current total allocated space (normal or mmapped)
  fordblks:  total free space
  keepcost:  the maximum number of bytes that could ideally be released
               back to system via malloc_trim. ("ideally" means that
               it ignores page restrictions etc.)

  Because these fields are ints, but internal bookkeeping may
  be kept as longs, the reported values may wrap around zero and
  thus be inaccurate.
*/
DLMALLOC_EXPORT struct mallinfo dlmallinfo(void);
#endif /* NO_MALLINFO */

/*
  independent_calloc(size_t n_elements, size_t element_size, void* chunks[]);

  independent_calloc is similar to calloc, but instead of returning a
  single cleared space, it returns an array of pointers to n_elements
  independent elements that can hold contents of size elem_size, each
  of which starts out cleared, and can be independently freed,
  realloc'ed etc. The elements are guaranteed to be adjacently
  allocated (this is not guaranteed to occur with multiple callocs or
  mallocs), which may also improve cache locality in some
  applications.

  The "chunks" argument is optional (i.e., may be null, which is
  probably the most typical usage). If it is null, the returned array
  is itself dynamically allocated and should also be freed when it is
  no longer needed. Otherwise, the chunks array must be of at least
  n_elements in length. It is filled in with the pointers to the
  chunks.

  In either case, independent_calloc returns this pointer array, or
  null if the allocation failed.  If n_elements is zero and "chunks"
  is null, it returns a chunk representing an array with zero elements
  (which should be freed if not wanted).

  Each element must be freed when it is no longer needed. This can be
  done all at once using bulk_free.

  independent_calloc simplifies and speeds up implementations of many
  kinds of pools.  It may also be useful when constructing large data
  structures that initially have a fixed number of fixed-sized nodes,
  but the number is not known at compile time, and some of the nodes
  may later need to be freed. For example:

  struct Node { int item; struct Node* next; };

  struct Node* build_list() {
    struct Node** pool;
    int n = read_number_of_nodes_needed();
    if (n <= 0) return 0;
    pool = (struct Node**)(independent_calloc(n, sizeof(struct Node), 0);
    if (pool == 0) die();
    // organize into a linked list...
    struct Node* first = pool[0];
    for (i = 0; i < n-1; ++i)
      pool[i]->next = pool[i+1];
    free(pool);     // Can now free the array (or not, if it is needed later)
    return first;
  }
*/
DLMALLOC_EXPORT void** dlindependent_calloc(size_t, size_t, void**);

/*
  independent_comalloc(size_t n_elements, size_t sizes[], void* chunks[]);

  independent_comalloc allocates, all at once, a set of n_elements
  chunks with sizes indicated in the "sizes" array.    It returns
  an array of pointers to these elements, each of which can be
  independently freed, realloc'ed etc. The elements are guaranteed to
  be adjacently allocated (this is not guaranteed to occur with
  multiple callocs or mallocs), which may also improve cache locality
  in some applications.

  The "chunks" argument is optional (i.e., may be null). If it is null
  the returned array is itself dynamically allocated and should also
  be freed when it is no longer needed. Otherwise, the chunks array
  must be of at least n_elements in length. It is filled in with the
  pointers to the chunks.

  In either case, independent_comalloc returns this pointer array, or
  null if the allocation failed.  If n_elements is zero and chunks is
  null, it returns a chunk representing an array with zero elements
  (which should be freed if not wanted).

  Each element must be freed when it is no longer needed. This can be
  done all at once using bulk_free.

  independent_comallac differs from independent_calloc in that each
  element may have a different size, and also that it does not
  automatically clear elements.

  independent_comalloc can be used to speed up allocation in cases
  where several structs or objects must always be allocated at the
  same time.  For example:

  struct Head { ... }
  struct Foot { ... }

  void send_message(char* msg) {
    int msglen = strlen(msg);
    size_t sizes[3] = { sizeof(struct Head), msglen, sizeof(struct Foot) };
    void* chunks[3];
    if (independent_comalloc(3, sizes, chunks) == 0)
      die();
    struct Head* head = (struct Head*)(chunks[0]);
    char*        body = (char*)(chunks[1]);
    struct Foot* foot = (struct Foot*)(chunks[2]);
    // ...
  }

  In general though, independent_comalloc is worth using only for
  larger values of n_elements. For small values, you probably won't
  detect enough difference from series of malloc calls to bother.

  Overuse of independent_comalloc can increase overall memory usage,
  since it cannot reuse existing noncontiguous small chunks that
  might be available for some of the elements.
*/
DLMALLOC_EXPORT void** dlindependent_comalloc(size_t, size_t*, void**);

/*
  bulk_free(void* array[], size_t n_elements)
  Frees and clears (sets to null) each non-null pointer in the given
  array.  This is likely to be faster than freeing them one-by-one.
  If footers are used, pointers that have been allocated in different
  mspaces are not freed or cleared, and the count of all such pointers
  is returned.  For large arrays of pointers with poor locality, it
  may be worthwhile to sort this array before calling bulk_free.
*/
DLMALLOC_EXPORT size_t  dlbulk_free(void**, size_t n_elements);

/*
  pvalloc(size_t n);
  Equivalent to valloc(minimum-page-that-holds(n)), that is,
  round up n to nearest pagesize.
 */
DLMALLOC_EXPORT void*  dlpvalloc(size_t);

/*
  malloc_trim(size_t pad);

  If possible, gives memory back to the system (via negative arguments
  to sbrk) if there is unused memory at the `high' end of the malloc
  pool or in unused MMAP segments. You can call this after freeing
  large blocks of memory to potentially reduce the system-level memory
  requirements of a program. However, it cannot guarantee to reduce
  memory. Under some allocation patterns, some large free blocks of
  memory will be locked between two used chunks, so they cannot be
  given back to the system.

  The `pad' argument to malloc_trim represents the amount of free
  trailing space to leave untrimmed. If this argument is zero, only
  the minimum amount of memory to maintain internal data structures
  will be left. Non-zero arguments can be supplied to maintain enough
  trailing space to service future expected allocations without having
  to re-obtain memory from the system.

  Malloc_trim returns 1 if it actually released any memory, else 0.
*/
DLMALLOC_EXPORT int  dlmalloc_trim(size_t);

/*
  malloc_stats();
  Prints on stderr the amount of space obtained from the system (both
  via sbrk and mmap), the maximum amount (which may be more than
  current if malloc_trim and/or munmap got called), and the current
  number of bytes allocated via malloc (or realloc, etc) but not yet
  freed. Note that this is the number of bytes allocated, not the
  number requested. It will be larger than the number requested
  because of alignment and bookkeeping overhead. Because it includes
  alignment wastage as being in use, this figure may be greater than
  zero even when no user-level chunks are allocated.

  The reported current and maximum system memory can be inaccurate if
  a program makes other calls to system memory allocation functions
  (normally sbrk) outside of malloc.

  malloc_stats prints only the most commonly interesting statistics.
  More information can be obtained by calling mallinfo.
*/
DLMALLOC_EXPORT void  dlmalloc_stats(void);

/*
  malloc_usable_size(void* p);

  Returns the number of bytes you can actually use in
  an allocated chunk, which may be more than you requested (although
  often not) due to alignment and minimum size constraints.
  You can use this many bytes without worrying about
  overwriting other allocated objects. This is not a particularly great
  programming practice. malloc_usable_size can be more useful in
  debugging and assertions, for example:

  p = malloc(n);
  assert(malloc_usable_size(p) >= 256);
*/
size_t dlmalloc_usable_size(void*);

#endif /* ONLY_MSPACES */

#if MSPACES

/*
  mspace is an opaque type representing an independent
  region of space that supports mspace_malloc, etc.
*/
typedef void* mspace;

/*
  create_mspace creates and returns a new independent space with the
  given initial capacity, or, if 0, the default granularity size.  It
  returns null if there is no system memory available to create the
  space.  If argument locked is non-zero, the space uses a separate
  lock to control access. The capacity of the space will grow
  dynamically as needed to service mspace_malloc requests.  You can
  control the sizes of incremental increases of this space by
  compiling with a different DEFAULT_GRANULARITY or dynamically
  setting with mallopt(M_GRANULARITY, value).
*/
DLMALLOC_EXPORT mspace create_mspace(size_t capacity, int locked);

/*
  destroy_mspace destroys the given space, and attempts to return all
  of its memory back to the system, returning the total number of
  bytes freed. After destruction, the results of access to all memory
  used by the space become undefined.
*/
DLMALLOC_EXPORT size_t destroy_mspace(mspace msp);

/*
  create_mspace_with_base uses the memory supplied as the initial base
  of a new mspace. Part (less than 128*sizeof(size_t) bytes) of this
  space is used for bookkeeping, so the capacity must be at least this
  large. (Otherwise 0 is returned.) When this initial space is
  exhausted, additional memory will be obtained from the system.
  Destroying this space will deallocate all additionally allocated
  space (if possible) but not the initial base.
*/
DLMALLOC_EXPORT mspace create_mspace_with_base(void* base, size_t capacity, int locked);

/*
  mspace_track_large_chunks controls whether requests for large chunks
  are allocated in their own untracked mmapped regions, separate from
  others in this mspace. By default large chunks are not tracked,
  which reduces fragmentation. However, such chunks are not
  necessarily released to the system upon destroy_mspace.  Enabling
  tracking by setting to true may increase fragmentation, but avoids
  leakage when relying on destroy_mspace to release all memory
  allocated using this space.  The function returns the previous
  setting.
*/
DLMALLOC_EXPORT int mspace_track_large_chunks(mspace msp, int enable);


/*
  mspace_malloc behaves as malloc, but operates within
  the given space.
*/
DLMALLOC_EXPORT void* mspace_malloc(mspace msp, size_t bytes);

/*
  mspace_free behaves as free, but operates within
  the given space.

  If compiled with FOOTERS==1, mspace_free is not actually needed.
  free may be called instead of mspace_free because freed chunks from
  any space are handled by their originating spaces.
*/
DLMALLOC_EXPORT void mspace_free(mspace msp, void* mem);

/*
  mspace_realloc behaves as realloc, but operates within
  the given space.

  If compiled with FOOTERS==1, mspace_realloc is not actually
  needed.  realloc may be called instead of mspace_realloc because
  realloced chunks from any space are handled by their originating
  spaces.
*/
DLMALLOC_EXPORT void* mspace_realloc(mspace msp, void* mem, size_t newsize);

/*
  mspace_calloc behaves as calloc, but operates within
  the given space.
*/
DLMALLOC_EXPORT void* mspace_calloc(mspace msp, size_t n_elements, size_t elem_size);

/*
  mspace_memalign behaves as memalign, but operates within
  the given space.
*/
DLMALLOC_EXPORT void* mspace_memalign(mspace msp, size_t alignment, size_t bytes);

/*
  mspace_independent_calloc behaves as independent_calloc, but
  operates within the given space.
*/
DLMALLOC_EXPORT void** mspace_independent_calloc(mspace msp, size_t n_elements,
                                 size_t elem_size, void* chunks[]);

/*
  mspace_independent_comalloc behaves as independent_comalloc, but
  operates within the given space.
*/
DLMALLOC_EXPORT void** mspace_independent_comalloc(mspace msp, size_t n_elements,
                                   size_t sizes[], void* chunks[]);

/*
  mspace_footprint() returns the number of bytes obtained from the
  system for this space.
*/
DLMALLOC_EXPORT size_t mspace_footprint(mspace msp);

/*
  mspace_max_footprint() returns the peak number of bytes obtained from the
  system for this space.
*/
DLMALLOC_EXPORT size_t mspace_max_footprint(mspace msp);


#if !NO_MALLINFO
/*
  mspace_mallinfo behaves as mallinfo, but reports properties of
  the given space.
*/
DLMALLOC_EXPORT struct mallinfo mspace_mallinfo(mspace msp);
#endif /* NO_MALLINFO */

/*
  malloc_usable_size(void* p) behaves the same as malloc_usable_size;
*/
DLMALLOC_EXPORT size_t mspace_usable_size(const void* mem);

/*
  mspace_malloc_stats behaves as malloc_stats, but reports
  properties of the given space.
*/
DLMALLOC_EXPORT void mspace_malloc_stats(mspace msp);

/*
  mspace_trim behaves as malloc_trim, but
  operates within the given space.
*/
DLMALLOC_EXPORT int mspace_trim(mspace msp, size_t pad);

/*
  An alias for mallopt.
*/
DLMALLOC_EXPORT int mspace_mallopt(int, int);

#endif /* MSPACES */

}  /* end of extern "C" */

/*
  ========================================================================
  To make a fully customizable malloc.h header file, cut everything
  above this line, put into file malloc.h, edit to suit, and #include it
  on the next line, as well as in programs that use this malloc.
  ========================================================================
*/

/* #include "malloc.h" */

/*------------------------------ internal #includes ---------------------- */

#ifdef _MSC_VER
#pragma warning( disable : 4146 ) /* no "unsigned" warnings */
#endif /* _MSC_VER */
#if !NO_MALLOC_STATS
#include <stdio.h>       /* for printing in malloc_stats */
#endif /* NO_MALLOC_STATS */
#ifndef LACKS_ERRNO_H
#include <errno.h>       /* for MALLOC_FAILURE_ACTION */
#endif /* LACKS_ERRNO_H */
#ifdef DEBUG
    #if ABORT_ON_ASSERT_FAILURE
        #undef assert
        #define assert(x) if(!(x)) ABORT
    #else /* ABORT_ON_ASSERT_FAILURE */
        #include <assert.h>
    #endif /* ABORT_ON_ASSERT_FAILURE */
#else  /* DEBUG */
    #ifndef assert
        #define assert(x)
    #endif
    #define DEBUG 0
#endif /* DEBUG */

#if !defined(WIN32) && !defined(LACKS_TIME_H)
#include <time.h>        /* for magic initialization */
#endif /* WIN32 */
#ifndef LACKS_STDLIB_H
#include <stdlib.h>      /* for abort() */
#endif /* LACKS_STDLIB_H */
#ifndef LACKS_STRING_H
#include <string.h>      /* for memset etc */
#endif  /* LACKS_STRING_H */
#if USE_BUILTIN_FFS
#ifndef LACKS_STRINGS_H
#include <strings.h>     /* for ffs */
#endif /* LACKS_STRINGS_H */
#endif /* USE_BUILTIN_FFS */
#if HAVE_MMAP
#ifndef LACKS_SYS_MMAN_H
/* On some versions of linux, mremap decl in mman.h needs __USE_GNU set */
#if (defined(linux) && !defined(__USE_GNU))
#define __USE_GNU 1
#include <sys/mman.h>    /* for mmap */
#undef __USE_GNU
#else
#include <sys/mman.h>    /* for mmap */
#endif /* linux */
#endif /* LACKS_SYS_MMAN_H */
#ifndef LACKS_FCNTL_H
#include <fcntl.h>
#endif /* LACKS_FCNTL_H */
#endif /* HAVE_MMAP */
#ifndef LACKS_UNISTD_H
#include <unistd.h>     /* for sbrk, sysconf */
#else /* LACKS_UNISTD_H */
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
extern void*     sbrk(ptrdiff_t);
#endif /* FreeBSD etc */
#endif /* LACKS_UNISTD_H */

/* Declarations for locking */
#if USE_LOCKS
#ifndef WIN32
#if defined (__SVR4) && defined (__sun)  /* solaris */
#include <thread.h>
#elif !defined(LACKS_SCHED_H)
#include <sched.h>
#endif /* solaris or LACKS_SCHED_H */
#if (defined(USE_RECURSIVE_LOCKS) && USE_RECURSIVE_LOCKS != 0) || !USE_SPIN_LOCKS
#include <pthread.h>
#endif /* USE_RECURSIVE_LOCKS ... */
#elif defined(_MSC_VER)
#ifndef _M_AMD64
/* These are already defined on AMD64 builds */
extern "C" {
    LONG __cdecl _InterlockedCompareExchange(LONG volatile *Dest, LONG Exchange, LONG Comp);
    LONG __cdecl _InterlockedExchange(LONG volatile *Target, LONG Value);
}
#endif /* _M_AMD64 */
#pragma intrinsic (_InterlockedCompareExchange)
#pragma intrinsic (_InterlockedExchange)
#define interlockedcompareexchange _InterlockedCompareExchange
#define interlockedexchange _InterlockedExchange
#elif defined(WIN32) && defined(__GNUC__)
#define interlockedcompareexchange(a, b, c) __sync_val_compare_and_swap(a, c, b)
#define interlockedexchange __sync_lock_test_and_set
#endif /* Win32 */
#else /* USE_LOCKS */
#endif /* USE_LOCKS */

#ifndef LOCK_AT_FORK
    #define LOCK_AT_FORK 0
#endif

/* Declarations for bit scanning on win32 */
#if defined(_MSC_VER) && _MSC_VER>=1300
#ifndef BitScanForward /* Try to avoid pulling in WinNT.h */
extern "C" {
    unsigned char _BitScanForward(unsigned long *index, unsigned long mask);
    unsigned char _BitScanReverse(unsigned long *index, unsigned long mask);
}

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif /* BitScanForward */
#endif /* defined(_MSC_VER) && _MSC_VER>=1300 */

#ifndef WIN32
#ifndef malloc_getpagesize
#  ifdef _SC_PAGESIZE         /* some SVR4 systems omit an underscore */
#    ifndef _SC_PAGE_SIZE
#      define _SC_PAGE_SIZE _SC_PAGESIZE
#    endif
#  endif
#  ifdef _SC_PAGE_SIZE
#    define malloc_getpagesize sysconf(_SC_PAGE_SIZE)
#  else
#    if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
       extern size_t getpagesize();
#      define malloc_getpagesize getpagesize()
#    else
#      ifdef WIN32 /* use supplied emulation of getpagesize */
#        define malloc_getpagesize getpagesize()
#      else
#        ifndef LACKS_SYS_PARAM_H
#          include <sys/param.h>
#        endif
#        ifdef EXEC_PAGESIZE
#          define malloc_getpagesize EXEC_PAGESIZE
#        else
#          ifdef NBPG
#            ifndef CLSIZE
#              define malloc_getpagesize NBPG
#            else
#              define malloc_getpagesize (NBPG * CLSIZE)
#            endif
#          else
#            ifdef NBPC
#              define malloc_getpagesize NBPC
#            else
#              ifdef PAGESIZE
#                define malloc_getpagesize PAGESIZE
#              else /* just guess */
#                define malloc_getpagesize ((size_t)4096U)
#              endif
#            endif
#          endif
#        endif
#      endif
#    endif
#  endif
#endif
#endif

/* ------------------- size_t and alignment properties -------------------- */

/* The byte and bit size of a size_t */
#define SIZE_T_BITSIZE      (sizeof(size_t) << 3)

/* Some constants coerced to size_t */
/* Annoying but necessary to avoid errors on some platforms */
#define HALF_MAX_SIZE_T     (MAX_SIZE_T / 2U)

// The bit mask value corresponding to MALLOC_ALIGNMENT
#define CHUNK_ALIGN_MASK    (MALLOC_ALIGNMENT - 1)

// True if address a has acceptable alignment
bool is_aligned(void *p) { return ((size_t)p & CHUNK_ALIGN_MASK) == 0; }

// the number of bytes to offset an address to align it
size_t align_offset(void *p)
{
    return (((size_t)p & CHUNK_ALIGN_MASK) == 0) ? 0 :
        ((MALLOC_ALIGNMENT - ((size_t)p & CHUNK_ALIGN_MASK)) & CHUNK_ALIGN_MASK);
}

/* -------------------------- MMAP preliminaries ------------------------- */

/*
   If HAVE_MORECORE or HAVE_MMAP are false, we just define calls and
   checks to fail so compiler optimizer can delete code rather than
   using so many "#if"s.
*/


/* MORECORE and MMAP must return MFAIL on failure */
#define MFAIL                ((void*)(MAX_SIZE_T))
#define CMFAIL               ((char*)(MFAIL)) /* defined for convenience */

#if HAVE_MMAP

#ifndef WIN32
#define MUNMAP_DEFAULT(a, s)  munmap((a), (s))
#define MMAP_PROT            (PROT_READ | PROT_WRITE)
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS        MAP_ANON
#endif /* MAP_ANON */
#ifdef MAP_ANONYMOUS
#define MMAP_FLAGS           (MAP_PRIVATE | MAP_ANONYMOUS)
#define MMAP_DEFAULT(s)       mmap(0, (s), MMAP_PROT, MMAP_FLAGS, -1, 0)
#else /* MAP_ANONYMOUS */
/*
   Nearly all versions of mmap support MAP_ANONYMOUS, so the following
   is unlikely to be needed, but is supplied just in case.
*/
#define MMAP_FLAGS           (MAP_PRIVATE)
static int dev_zero_fd = -1; /* Cached file descriptor for /dev/zero. */
void MMAP_DEFAULT(size_t s) {
    if (dev_zero_fd < 0)
        dev_zero_fd = open("/dev/zero", O_RDWR);
    mmap(0, s, MMAP_PROT, MMAP_FLAGS, dev_zero_fd, 0);
}

#endif /* MAP_ANONYMOUS */

#define DIRECT_MMAP_DEFAULT(s) MMAP_DEFAULT(s)

#else /* WIN32 */

/* Win32 MMAP via VirtualAlloc */
static FORCEINLINE void* win32mmap(size_t size) {
    void* ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return (ptr != 0)? ptr: MFAIL;
}

/* For direct MMAP, use MEM_TOP_DOWN to minimize interference */
static FORCEINLINE void* win32direct_mmap(size_t size) {
    void* ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
                             PAGE_READWRITE);
    return (ptr != 0)? ptr: MFAIL;
}

/* This function supports releasing coalesed segments */
static FORCEINLINE int win32munmap(void* ptr, size_t size) {
    MEMORY_BASIC_INFORMATION minfo;
    char* cptr = (char*)ptr;
    while (size) {
        if (VirtualQuery(cptr, &minfo, sizeof(minfo)) == 0)
            return -1;
        if (minfo.BaseAddress != cptr || minfo.AllocationBase != cptr ||
            minfo.State != MEM_COMMIT || minfo.RegionSize > size)
            return -1;
        if (VirtualFree(cptr, 0, MEM_RELEASE) == 0)
            return -1;
        cptr += minfo.RegionSize;
        size -= minfo.RegionSize;
    }
    return 0;
}

#define MMAP_DEFAULT(s)             win32mmap(s)
#define MUNMAP_DEFAULT(a, s)        win32munmap((a), (s))
#define DIRECT_MMAP_DEFAULT(s)      win32direct_mmap(s)
#endif /* WIN32 */
#endif /* HAVE_MMAP */

#if HAVE_MREMAP
#ifndef WIN32
#define MREMAP_DEFAULT(addr, osz, nsz, mv) mremap((addr), (osz), (nsz), (mv))
#endif /* WIN32 */
#endif /* HAVE_MREMAP */

/**
 * Define CALL_MORECORE
 */
#if HAVE_MORECORE
    #ifdef MORECORE
        #define CALL_MORECORE(S)    MORECORE(S)
    #else  /* MORECORE */
        #define CALL_MORECORE(S)    MORECORE_DEFAULT(S)
    #endif /* MORECORE */
#else  /* HAVE_MORECORE */
    #define CALL_MORECORE(S)        MFAIL
#endif /* HAVE_MORECORE */

/**
 * Define CALL_MMAP/CALL_MUNMAP/CALL_DIRECT_MMAP
 */
#if HAVE_MMAP
    #define USE_MMAP_BIT            1

    #ifdef MMAP
        #define CALL_MMAP(s)        MMAP(s)
    #else /* MMAP */
        #define CALL_MMAP(s)        MMAP_DEFAULT(s)
    #endif /* MMAP */
    #ifdef MUNMAP
        #define CALL_MUNMAP(a, s)   MUNMAP((a), (s))
    #else /* MUNMAP */
        #define CALL_MUNMAP(a, s)   MUNMAP_DEFAULT((a), (s))
    #endif /* MUNMAP */
    #ifdef DIRECT_MMAP
        #define CALL_DIRECT_MMAP(s) DIRECT_MMAP(s)
    #else /* DIRECT_MMAP */
        #define CALL_DIRECT_MMAP(s) DIRECT_MMAP_DEFAULT(s)
    #endif /* DIRECT_MMAP */
#else  /* HAVE_MMAP */
    #define USE_MMAP_BIT            0

    #define MMAP(s)                 MFAIL
    #define MUNMAP(a, s)            (-1)
    #define DIRECT_MMAP(s)          MFAIL
    #define CALL_DIRECT_MMAP(s)     DIRECT_MMAP(s)
    #define CALL_MMAP(s)            MMAP(s)
    #define CALL_MUNMAP(a, s)       MUNMAP((a), (s))
#endif /* HAVE_MMAP */

/**
 * Define CALL_MREMAP
 */
#if HAVE_MMAP && HAVE_MREMAP
    #ifdef MREMAP
        #define CALL_MREMAP(addr, osz, nsz, mv) MREMAP((addr), (osz), (nsz), (mv))
    #else /* MREMAP */
        #define CALL_MREMAP(addr, osz, nsz, mv) MREMAP_DEFAULT((addr), (osz), (nsz), (mv))
    #endif /* MREMAP */
#else  /* HAVE_MMAP && HAVE_MREMAP */
    #define CALL_MREMAP(addr, osz, nsz, mv)     MFAIL
#endif /* HAVE_MMAP && HAVE_MREMAP */

/* mstate bit set if continguous morecore disabled or failed */
#define USE_NONCONTIGUOUS_BIT (4U)

/* segment bit set in create_mspace_with_base */
#define EXTERN_BIT            (8U)


/* --------------------------- Lock preliminaries ------------------------ */

/*
  When locks are defined, there is one global lock, plus
  one per-mspace lock.

  The global lock_ensures that mparams.magic and other unique
  mparams values are initialized only once. It also protects
  sequences of calls to MORECORE.  In many cases sys_alloc requires
  two calls, that should not be interleaved with calls by other
  threads.  This does not protect against direct calls to MORECORE
  by other threads not using this lock, so there is still code to
  cope the best we can on interference.

  Per-mspace locks surround calls to malloc, free, etc.
  By default, locks are simple non-reentrant mutexes.

  Because lock-protected regions generally have bounded times, it is
  OK to use the supplied simple spinlocks. Spinlocks are likely to
  improve performance for lightly contended applications, but worsen
  performance under heavy contention.

  If USE_LOCKS is > 1, the definitions of lock routines here are
  bypassed, in which case you will need to define the type MLOCK_T,
  and at least INITIAL_LOCK, DESTROY_LOCK, ACQUIRE_LOCK, RELEASE_LOCK
  and TRY_LOCK.  You must also declare a
    static MLOCK_T malloc_global_mutex = { initialization values };.

*/

#if !USE_LOCKS
#define USE_LOCK_BIT               (0U)
#define INITIAL_LOCK(l)            (0)
#define DESTROY_LOCK(l)            (0)
#define ACQUIRE_MALLOC_GLOBAL_LOCK()
#define RELEASE_MALLOC_GLOBAL_LOCK()

#else
#if USE_LOCKS > 1
/* -----------------------  User-defined locks ------------------------ */
/* Define your own lock implementation here */
/* #define INITIAL_LOCK(lk)  ... */
/* #define DESTROY_LOCK(lk)  ... */
/* #define ACQUIRE_LOCK(lk)  ... */
/* #define RELEASE_LOCK(lk)  ... */
/* #define TRY_LOCK(lk) ... */
/* static MLOCK_T malloc_global_mutex = ... */

#elif USE_SPIN_LOCKS

/* First, define CAS_LOCK and CLEAR_LOCK on ints */
/* Note CAS_LOCK defined to return 0 on success */

#if defined(__GNUC__)&& (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1))
#define CAS_LOCK(sl)     __sync_lock_test_and_set(sl, 1)
#define CLEAR_LOCK(sl)   __sync_lock_release(sl)

#elif (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
/* Custom spin locks for older gcc on x86 */
static FORCEINLINE int x86_cas_lock(int *sl) {
  int ret;
  int val = 1;
  int cmp = 0;
  __asm__ __volatile__  ("lock; cmpxchgl %1, %2"
                         : "=a" (ret)
                         : "r" (val), "m" (*(sl)), "0"(cmp)
                         : "memory", "cc");
  return ret;
}

static FORCEINLINE void x86_clear_lock(int* sl) {
  assert(*sl != 0);
  int prev = 0;
  int ret;
  __asm__ __volatile__ ("lock; xchgl %0, %1"
                        : "=r" (ret)
                        : "m" (*(sl)), "0"(prev)
                        : "memory");
}

#define CAS_LOCK(sl)     x86_cas_lock(sl)
#define CLEAR_LOCK(sl)   x86_clear_lock(sl)

#else /* Win32 MSC */
#define CAS_LOCK(sl)     interlockedexchange(sl, 1)
#define CLEAR_LOCK(sl)   interlockedexchange (sl, 0)

#endif /* ... gcc spins locks ... */

/* How to yield for a spin lock */
#define SPINS_PER_YIELD       63
#if defined(_MSC_VER)
#define SLEEP_EX_DURATION     50 /* delay for yield/sleep */
#define SPIN_LOCK_YIELD  SleepEx(SLEEP_EX_DURATION, FALSE)
#elif defined (__SVR4) && defined (__sun) /* solaris */
#define SPIN_LOCK_YIELD   thr_yield();
#elif !defined(LACKS_SCHED_H)
#define SPIN_LOCK_YIELD   sched_yield();
#else
#define SPIN_LOCK_YIELD
#endif /* ... yield ... */

#if !defined(USE_RECURSIVE_LOCKS) || USE_RECURSIVE_LOCKS == 0
/* Plain spin locks use single word (embedded in malloc_states) */
static int spin_acquire_lock(long *sl) {
    int spins = 0;
    while (*(volatile long *)sl != 0 || CAS_LOCK(sl)) {
        if ((++spins & SPINS_PER_YIELD) == 0) {
            SPIN_LOCK_YIELD;
        }
    }
    return 0;
}

#define MLOCK_T               long
#define TRY_LOCK(sl)          !CAS_LOCK(sl)
#define RELEASE_LOCK(sl)      CLEAR_LOCK(sl)
#define ACQUIRE_LOCK(sl)      (CAS_LOCK(sl)? spin_acquire_lock(sl) : 0)
#define INITIAL_LOCK(sl)      (*sl = 0)
#define DESTROY_LOCK(sl)      (0)
static MLOCK_T malloc_global_mutex = 0;

#else /* USE_RECURSIVE_LOCKS */
/* types for lock owners */
#ifdef WIN32
#define THREAD_ID_T           DWORD
#define CURRENT_THREAD        GetCurrentThreadId()
#define EQ_OWNER(X,Y)         ((X) == (Y))
#else
/*
  Note: the following assume that pthread_t is a type that can be
  initialized to (casted) zero. If this is not the case, you will need to
  somehow redefine these or not use spin locks.
*/
#define THREAD_ID_T           pthread_t
#define CURRENT_THREAD        pthread_self()
#define EQ_OWNER(X,Y)         pthread_equal(X, Y)
#endif

struct malloc_recursive_lock {
    int sl;
    unsigned int c;
    THREAD_ID_T threadid;
};

#define MLOCK_T  struct malloc_recursive_lock
static MLOCK_T malloc_global_mutex = { 0, 0, (THREAD_ID_T)0};

static FORCEINLINE void recursive_release_lock(MLOCK_T *lk) {
    assert(lk->sl != 0);
    if (--lk->c == 0) {
        CLEAR_LOCK(&lk->sl);
    }
}

static FORCEINLINE int recursive_acquire_lock(MLOCK_T *lk) {
    THREAD_ID_T mythreadid = CURRENT_THREAD;
    int spins = 0;
    for (;;) {
        if (*((volatile int *)(&lk->sl)) == 0) {
            if (!CAS_LOCK(&lk->sl)) {
                lk->threadid = mythreadid;
                lk->c = 1;
                return 0;
            }
        }
        else if (EQ_OWNER(lk->threadid, mythreadid)) {
            ++lk->c;
            return 0;
        }
        if ((++spins & SPINS_PER_YIELD) == 0) {
            SPIN_LOCK_YIELD;
        }
    }
}

static FORCEINLINE int recursive_try_lock(MLOCK_T *lk) {
    THREAD_ID_T mythreadid = CURRENT_THREAD;
    if (*((volatile int *)(&lk->sl)) == 0) {
        if (!CAS_LOCK(&lk->sl)) {
            lk->threadid = mythreadid;
            lk->c = 1;
            return 1;
        }
    }
    else if (EQ_OWNER(lk->threadid, mythreadid)) {
        ++lk->c;
        return 1;
    }
    return 0;
}

#define RELEASE_LOCK(lk)      recursive_release_lock(lk)
#define TRY_LOCK(lk)          recursive_try_lock(lk)
#define ACQUIRE_LOCK(lk)      recursive_acquire_lock(lk)
#define INITIAL_LOCK(lk)      ((lk)->threadid = (THREAD_ID_T)0, (lk)->sl = 0, (lk)->c = 0)
#define DESTROY_LOCK(lk)      (0)
#endif /* USE_RECURSIVE_LOCKS */

#elif defined(WIN32) /* Win32 critical sections */
#define MLOCK_T               CRITICAL_SECTION
#define ACQUIRE_LOCK(lk)      (EnterCriticalSection(lk), 0)
#define RELEASE_LOCK(lk)      LeaveCriticalSection(lk)
#define TRY_LOCK(lk)          TryEnterCriticalSection(lk)
#define INITIAL_LOCK(lk)      (!InitializeCriticalSectionAndSpinCount((lk), 0x80000000 | 4000))
#define DESTROY_LOCK(lk)      (DeleteCriticalSection(lk), 0)
#define NEED_GLOBAL_LOCK_INIT

static MLOCK_T malloc_global_mutex;
static volatile LONG malloc_global_mutex_status;

/* Use spin loop to initialize global lock */
static void init_malloc_global_mutex() {
  for (;;) {
    long stat = malloc_global_mutex_status;
    if (stat > 0)
      return;
    /* transition to < 0 while initializing, then to > 0) */
    if (stat == 0 &&
        interlockedcompareexchange(&malloc_global_mutex_status, (LONG)-1, (LONG)0) == 0) {
      InitializeCriticalSection(&malloc_global_mutex);
      interlockedexchange(&malloc_global_mutex_status, (LONG)1);
      return;
    }
    SleepEx(0, FALSE);
  }
}

#else /* pthreads-based locks */
#define MLOCK_T               pthread_mutex_t
#define ACQUIRE_LOCK(lk)      pthread_mutex_lock(lk)
#define RELEASE_LOCK(lk)      pthread_mutex_unlock(lk)
#define TRY_LOCK(lk)          (!pthread_mutex_trylock(lk))
#define INITIAL_LOCK(lk)      pthread_init_lock(lk)
#define DESTROY_LOCK(lk)      pthread_mutex_destroy(lk)

#if defined(USE_RECURSIVE_LOCKS) && USE_RECURSIVE_LOCKS != 0 && defined(linux) && !defined(PTHREAD_MUTEX_RECURSIVE)
/* Cope with old-style linux recursive lock initialization by adding */
/* skipped internal declaration from pthread.h */
extern int pthread_mutexattr_setkind_np __P ((pthread_mutexattr_t *__attr,
                                              int __kind));
#define PTHREAD_MUTEX_RECURSIVE PTHREAD_MUTEX_RECURSIVE_NP
#define pthread_mutexattr_settype(x,y) pthread_mutexattr_setkind_np(x,y)
#endif /* USE_RECURSIVE_LOCKS ... */

static MLOCK_T malloc_global_mutex = PTHREAD_MUTEX_INITIALIZER;

static int pthread_init_lock (MLOCK_T *lk) {
  pthread_mutexattr_t attr;
  if (pthread_mutexattr_init(&attr)) return 1;
#if defined(USE_RECURSIVE_LOCKS) && USE_RECURSIVE_LOCKS != 0
  if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) return 1;
#endif
  if (pthread_mutex_init(lk, &attr)) return 1;
  if (pthread_mutexattr_destroy(&attr)) return 1;
  return 0;
}

#endif /* ... lock types ... */

/* Common code for all lock types */
#define USE_LOCK_BIT               (2U)

#ifndef ACQUIRE_MALLOC_GLOBAL_LOCK
#define ACQUIRE_MALLOC_GLOBAL_LOCK()  ACQUIRE_LOCK(&malloc_global_mutex);
#endif

#ifndef RELEASE_MALLOC_GLOBAL_LOCK
#define RELEASE_MALLOC_GLOBAL_LOCK()  RELEASE_LOCK(&malloc_global_mutex);
#endif

#endif /* USE_LOCKS */

/* -----------------------  Chunk representations ------------------------ */

/*
  (The following includes lightly edited explanations by Colin Plumb.)

  The malloc_chunk declaration below is misleading (but accurate and
  necessary).  It declares a "view" into memory allowing access to
  necessary fields at known offsets from a given base.

  Chunks of memory are maintained using a `boundary tag' method as
  originally described by Knuth.  (See the paper by Paul Wilson
  ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a survey of such
  techniques.)  Sizes of free chunks are stored both in the front of
  each chunk and at the end.  This makes consolidating fragmented
  chunks into bigger chunks fast.  The head fields also hold bits
  representing whether chunks are free or in use.

  Here are some pictures to make it clearer.  They are "exploded" to
  show that the state of a chunk can be thought of as extending from
  the high 31 bits of the head field of its header through the
  prev_foot and PINUSE_BIT bit of the following chunk header.

  A chunk that's in use looks like:

   chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           | Size of previous chunk (if P = 0)                             |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         1| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               |
         +-                                                             -+
         |                                                               |
         +-                                                             -+
         |                                                               :
         +-      size - sizeof(size_t) available payload bytes          -+
         :                                                               |
 chunk-> +-                                                             -+
         |                                                               |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |1|
       | Size of next chunk (may or may not be in use)               | +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    And if it's free, it looks like this:

   chunk-> +-                                                             -+
           | User payload (must be in use, or we would have merged!)       |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         0| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Next pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Prev pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               :
         +-      size - sizeof(struct chunk) unused bytes               -+
         :                                                               |
 chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Size of this chunk                                            |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |0|
       | Size of next chunk (must be in use, or we would have merged)| +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               :
       +- User payload                                                -+
       :                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                                                     |0|
                                                                     +-+
  Note that since we always merge adjacent free chunks, the chunks
  adjacent to a free chunk must be in use.

  Given a pointer to a chunk (which can be derived trivially from the
  payload pointer) we can, in O(1) time, find out whether the adjacent
  chunks are free, and if so, unlink them from the lists that they
  are on and merge them with the current chunk.

  Chunks always begin on even word boundaries, so the mem portion
  (which is returned to the user) is also on an even word boundary, and
  thus at least double-word aligned.

  The P (PINUSE_BIT) bit, stored in the unused low-order bit of the
  chunk size (which is always a multiple of two words), is an in-use
  bit for the *previous* chunk.  If that bit is *clear*, then the
  word before the current chunk size contains the previous chunk
  size, and can be used to find the front of the previous chunk.
  The very first chunk allocated always has this bit set, preventing
  access to non-existent (or non-owned) memory. If pinuse is set for
  any given chunk, then you CANNOT determine the size of the
  previous chunk, and might even get a memory addressing fault when
  trying to do so.

  The C (CINUSE_BIT) bit, stored in the unused second-lowest bit of
  the chunk size redundantly records whether the current chunk is
  inuse (unless the chunk is mmapped). This redundancy enables usage
  checks within free and realloc, and reduces indirection when freeing
  and consolidating chunks.

  Each freshly allocated chunk must have both cinuse and pinuse set.
  That is, each allocated chunk borders either a previously allocated
  and still in-use chunk, or the base of its memory arena. This is
  ensured by making all allocations from the `lowest' part of any
  found chunk.  Further, no free chunk physically borders another one,
  so each free chunk is known to be preceded and followed by either
  inuse chunks or the ends of memory.

  Note that the `foot' of the current chunk is actually represented
  as the prev_foot of the NEXT chunk. This makes it easier to
  deal with alignments etc but can be very confusing when trying
  to extend or adapt this code.

  The exceptions to all this are

     1. The special chunk `top' is the top-most available chunk (i.e.,
        the one bordering the end of available memory). It is treated
        specially.  Top is never included in any bin, is used only if
        no other chunk is available, and is released back to the
        system if it is very large (see M_TRIM_THRESHOLD).  In effect,
        the top chunk is treated as larger (and thus less well
        fitting) than any other available chunk.  The top chunk
        doesn't update its trailing size field since there is no next
        contiguous chunk that would have to index off it. However,
        space is still allocated for it (TOP_FOOT_SIZE) to enable
        separation or merging when space is extended.

     3. Chunks allocated via mmap, have both cinuse and pinuse bits
        cleared in their head fields.  Because they are allocated
        one-by-one, each must carry its own prev_foot field, which is
        also used to hold the offset this chunk has within its mmapped
        region, which is needed to preserve alignment. Each mmapped
        chunk is trailed by the first two fields of a fake next-chunk
        for sake of usage checks.

*/

#define PINUSE_BIT          1
#define CINUSE_BIT          2
#define FLAG4_BIT           4
#define INUSE_BITS          (PINUSE_BIT | CINUSE_BIT)
#define FLAG_BITS           (PINUSE_BIT | CINUSE_BIT | FLAG4_BIT)

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MCHUNK_SIZE         (sizeof(mchunk))

#if FOOTERS
    #define CHUNK_OVERHEAD      (2 * sizeof(size_t))
#else // FOOTERS
    #define CHUNK_OVERHEAD      (sizeof(size_t))
#endif // FOOTERS

/* MMapped chunks need a second word of overhead ... */
#define MMAP_CHUNK_OVERHEAD (2 * sizeof(size_t))
/* ... and additional padding for fake next-chunk at foot */
#define MMAP_FOOT_PAD       (4 * sizeof(size_t))

/* The smallest size we can malloc is an aligned minimal chunk */
#define MIN_CHUNK_SIZE  ((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

// ===============================================================================
struct malloc_chunk_header {
    void set_size_and_pinuse_of_free_chunk(size_t s) {
        _head = s | PINUSE_BIT;
        set_foot(s);
    }

    void set_foot(size_t s)  { 
        ((malloc_chunk_header *)((char*)this + s))->_prev_foot = s; 
    }

    // extraction of fields from head words
    bool cinuse() const        { return !!(_head & CINUSE_BIT); }
    bool pinuse() const        { return !!(_head & PINUSE_BIT); }
    bool flag4inuse() const    { return !!(_head & FLAG4_BIT); }
    bool is_inuse() const      { return (_head & INUSE_BITS) != PINUSE_BIT; }
    bool is_mmapped() const    { return (_head & INUSE_BITS) == 0; }

    size_t chunksize() const   { return _head & ~(FLAG_BITS); }

    void clear_pinuse()        { _head &= ~PINUSE_BIT; }
    void set_flag4()           { _head |= FLAG4_BIT; }
    void clear_flag4()         { _head &= ~FLAG4_BIT; }

    // Treat space at ptr +/- offset as a chunk
    malloc_chunk_header * chunk_plus_offset(size_t s)  {
        return (malloc_chunk_header *)((char*)this + s);
    }
    malloc_chunk_header * chunk_minus_offset(size_t s) { 
        return (malloc_chunk_header *)((char*)this - s); 
    }

    // Ptr to next or previous physical malloc_chunk.
    malloc_chunk_header * next_chunk() { 
        return (malloc_chunk_header *)((char*)this + (_head & ~FLAG_BITS)); 
    }
    malloc_chunk_header * prev_chunk() { 
        return (malloc_chunk_header *)((char*)this - (_prev_foot));
    }

    // extract next chunk's pinuse bit
    size_t next_pinuse()  { return next_chunk()->_head & PINUSE_BIT; }

    size_t   _prev_foot;  // Size of previous chunk (if free). 
    size_t   _head;       // Size and inuse bits.
};

// ===============================================================================
struct malloc_chunk : public malloc_chunk_header {
    // Set size, pinuse bit, foot, and clear next pinuse
    void set_free_with_pinuse(size_t s, malloc_chunk* n)
    {
        n->clear_pinuse();
        set_size_and_pinuse_of_free_chunk(s);
    }

    // Get the internal overhead associated with chunk p
    size_t overhead_for() { return is_mmapped() ? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD; }

    // Return true if malloced space is not necessarily cleared
    bool calloc_must_clear()
    {
#if MMAP_CLEARS
        return !is_mmapped();
#else 
        return true;
#endif 
    }
    
    struct malloc_chunk* _fd;         // double links -- used only if free.
    struct malloc_chunk* _bk;
};

typedef malloc_chunk  mchunk;
typedef malloc_chunk* mchunkptr;
typedef malloc_chunk_header *hchunkptr;
typedef malloc_chunk* sbinptr;         // The type of bins of chunks
typedef unsigned int bindex_t;         // Described below
typedef unsigned int binmap_t;         // Described below
typedef unsigned int flag_t;           // The type of various bit flag sets

// conversion from malloc headers to user pointers, and back 
static FORCEINLINE void *chunk2mem(const void *p)       { return (void *)((char *)p + 2 * sizeof(size_t)); }
static FORCEINLINE mchunkptr mem2chunk(const void *mem) { return (mchunkptr)((char *)mem - 2 * sizeof(size_t)); }

// chunk associated with aligned address A
static FORCEINLINE mchunkptr align_as_chunk(char *A)    { return (mchunkptr)(A + align_offset(chunk2mem(A))); }

// Bounds on request (not chunk) sizes.
#define MAX_REQUEST         ((-MIN_CHUNK_SIZE) << 2)
#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - 1)

// pad request bytes into a usable size
static FORCEINLINE size_t pad_request(size_t req) 
{
    return (req + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK;
}

// pad request, checking for minimum (but not maximum)
static FORCEINLINE size_t request2size(size_t req) 
{
    return req < MIN_REQUEST ? MIN_CHUNK_SIZE : pad_request(req);
}


/* ------------------ Operations on head and foot fields ----------------- */

/*
  The head field of a chunk is or'ed with PINUSE_BIT when previous
  adjacent chunk in use, and or'ed with CINUSE_BIT if this chunk is in
  use, unless mmapped, in which case both bits are cleared.

  FLAG4_BIT is not used by this malloc, but might be useful in extensions.
*/

// Head value for fenceposts
#define FENCEPOST_HEAD  (INUSE_BITS | sizeof(size_t))


/* ---------------------- Overlaid data structures ----------------------- */

/*
  When chunks are not in use, they are treated as nodes of either
  lists or trees.

  "Small"  chunks are stored in circular doubly-linked lists, and look
  like this:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk in list             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk in list            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space (may be 0 bytes long)                .
            .                                                               .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Larger chunks are kept in a form of bitwise digital trees (aka
  tries) keyed on chunksizes.  Because malloc_tree_chunks are only for
  free chunks greater than 256 bytes, their size doesn't impose any
  constraints on user chunk sizes.  Each node looks like:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk of same size        |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk of same size       |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to left child (child[0])                  |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to right child (child[1])                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to parent                                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             bin index of this chunk                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space                                      .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Each tree holding treenodes is a tree of unique chunk sizes.  Chunks
  of the same size are arranged in a circularly-linked list, with only
  the oldest chunk (the next to be used, in our FIFO ordering)
  actually in the tree.  (Tree members are distinguished by a non-null
  parent pointer.)  If a chunk with the same size an an existing node
  is inserted, it is linked off the existing node using pointers that
  work in the same way as fd/bk pointers of small chunks.

  Each tree contains a power of 2 sized range of chunk sizes (the
  smallest is 0x100 <= x < 0x180), which is is divided in half at each
  tree level, with the chunks in the smaller half of the range (0x100
  <= x < 0x140 for the top nose) in the left subtree and the larger
  half (0x140 <= x < 0x180) in the right subtree.  This is, of course,
  done by inspecting individual bits.

  Using these rules, each node's left subtree contains all smaller
  sizes than its right subtree.  However, the node at the root of each
  subtree has no particular ordering relationship to either.  (The
  dividing line between the subtree sizes is based on trie relation.)
  If we remove the last chunk of a given size from the interior of the
  tree, we need to replace it with a leaf node.  The tree ordering
  rules permit a node to be replaced by any leaf below it.

  The smallest chunk in a tree (a common operation in a best-fit
  allocator) can be found by walking a path to the leftmost leaf in
  the tree.  Unlike a usual binary tree, where we follow left child
  pointers until we reach a null, here we follow the right child
  pointer any time the left one is null, until we reach a leaf with
  both child pointers null. The smallest chunk in the tree will be
  somewhere along that path.

  The worst case number of steps to add, find, or remove a node is
  bounded by the number of bits differentiating chunks within
  bins. Under current bin calculations, this ranges from 6 up to 21
  (for 32 bit sizes) or up to 53 (for 64 bit sizes). The typical case
  is of course much better.
*/

// ===============================================================================
struct malloc_tree_chunk : public malloc_chunk_header {
    malloc_tree_chunk *leftmost_child() {
        return _child[0] ? _child[0] : _child[1]; 
    }


    malloc_tree_chunk* _fd;
    malloc_tree_chunk* _bk;

    malloc_tree_chunk* _child[2];
    malloc_tree_chunk* _parent;
    bindex_t           _index;
};

typedef malloc_tree_chunk  tchunk;
typedef malloc_tree_chunk* tchunkptr;
typedef malloc_tree_chunk* tbinptr; // The type of bins of trees

/* ----------------------------- Segments -------------------------------- */

/*
  Each malloc space may include non-contiguous segments, held in a
  list headed by an embedded malloc_segment record representing the
  top-most space. Segments also include flags holding properties of
  the space. Large chunks that are directly allocated by mmap are not
  included in this list. They are instead independently created and
  destroyed without otherwise keeping track of them.

  Segment management mainly comes into play for spaces allocated by
  MMAP.  Any call to MMAP might or might not return memory that is
  adjacent to an existing segment.  MORECORE normally contiguously
  extends the current space, so this space is almost always adjacent,
  which is simpler and faster to deal with. (This is why MORECORE is
  used preferentially to MMAP when both are available -- see
  sys_alloc.)  When allocating using MMAP, we don't use any of the
  hinting mechanisms (inconsistently) supported in various
  implementations of unix mmap, or distinguish reserving from
  committing memory. Instead, we just ask for space, and exploit
  contiguity when we get it.  It is probably possible to do
  better than this on some systems, but no general scheme seems
  to be significantly better.

  Management entails a simpler variant of the consolidation scheme
  used for chunks to reduce fragmentation -- new adjacent memory is
  normally prepended or appended to an existing segment. However,
  there are limitations compared to chunk consolidation that mostly
  reflect the fact that segment processing is relatively infrequent
  (occurring only when getting memory from system) and that we
  don't expect to have huge numbers of segments:

  * Segments are not indexed, so traversal requires linear scans.  (It
    would be possible to index these, but is not worth the extra
    overhead and complexity for most programs on most platforms.)
  * New segments are only appended to old ones when holding top-most
    memory; if they cannot be prepended to others, they are held in
    different segments.

  Except for the top-most segment of an mstate, each segment record
  is kept at the tail of its segment. Segments are added by pushing
  segment records onto the list headed by &mstate.seg for the
  containing mstate.

  Segment flags control allocation/merge/deallocation policies:
  * If EXTERN_BIT set, then we did not allocate this segment,
    and so should not try to deallocate or merge with others.
    (This currently holds only for the initial segment passed
    into create_mspace_with_base.)
  * If USE_MMAP_BIT set, the segment may be merged with
    other surrounding mmapped segments and trimmed/de-allocated
    using munmap.
  * If neither bit is set, then the segment was obtained using
    MORECORE so can be merged with surrounding MORECORE'd segments
    and deallocated/trimmed using MORECORE with negative arguments.
*/

// ===============================================================================
struct malloc_segment {
    bool is_mmapped_segment()  { return !!(_sflags & USE_MMAP_BIT); }
    bool is_extern_segment()   { return !!(_sflags & EXTERN_BIT); }

    char*           _base;          // base address
    size_t          _size;          // allocated size
    malloc_segment* _next;          // ptr to next segment
    flag_t          _sflags;        // mmap and extern flag
};

typedef malloc_segment  msegment;
typedef malloc_segment* msegmentptr;

/* ------------- Malloc_params ------------------- */

/*
  malloc_params holds global properties, including those that can be
  dynamically set using mallopt. There is a single instance, mparams,
  initialized in init_mparams. Note that the non-zeroness of "magic"
  also serves as an initialization flag.
*/

// ===============================================================================
struct malloc_params {
    malloc_params() : _magic(0) {}

    void ensure_initialization()
    {
        if (!_magic)
            _init();
    }
    int change(int param_number, int value);

    size_t page_align(size_t sz) {
        return (sz + (_page_size - 1)) & ~(_page_size - 1);
    }

    size_t granularity_align(size_t sz) {
        return (sz + (_granularity - 1)) & ~(_granularity - 1);
    }

    bool is_page_aligned(char *S) {
        return ((size_t)S & (_page_size - 1)) == 0;
    }

    int _init();

    size_t _magic;
    size_t _page_size;
    size_t _granularity;
    size_t _mmap_threshold;
    size_t _trim_threshold;
    flag_t _default_mflags;
};

static malloc_params mparams;

/* ---------------------------- malloc_state ----------------------------- */

/*
   A malloc_state holds all of the bookkeeping for a space.
   The main fields are:

  Top
    The topmost chunk of the currently active segment. Its size is
    cached in topsize.  The actual size of topmost space is
    topsize+TOP_FOOT_SIZE, which includes space reserved for adding
    fenceposts and segment records if necessary when getting more
    space from the system.  The size at which to autotrim top is
    cached from mparams in trim_check, except that it is disabled if
    an autotrim fails.

  Designated victim (dv)
    This is the preferred chunk for servicing small requests that
    don't have exact fits.  It is normally the chunk split off most
    recently to service another small request.  Its size is cached in
    dvsize. The link fields of this chunk are not maintained since it
    is not kept in a bin.

  SmallBins
    An array of bin headers for free chunks.  These bins hold chunks
    with sizes less than MIN_LARGE_SIZE bytes. Each bin contains
    chunks of all the same size, spaced 8 bytes apart.  To simplify
    use in double-linked lists, each bin header acts as a malloc_chunk
    pointing to the real first node, if it exists (else pointing to
    itself).  This avoids special-casing for headers.  But to avoid
    waste, we allocate only the fd/bk pointers of bins, and then use
    repositioning tricks to treat these as the fields of a chunk.

  TreeBins
    Treebins are pointers to the roots of trees holding a range of
    sizes. There are 2 equally spaced treebins for each power of two
    from TREE_SHIFT to TREE_SHIFT+16. The last bin holds anything
    larger.

  Bin maps
    There is one bit map for small bins ("smallmap") and one for
    treebins ("treemap).  Each bin sets its bit when non-empty, and
    clears the bit when empty.  Bit operations are then used to avoid
    bin-by-bin searching -- nearly all "search" is done without ever
    looking at bins that won't be selected.  The bit maps
    conservatively use 32 bits per map word, even if on 64bit system.
    For a good description of some of the bit-based techniques used
    here, see Henry S. Warren Jr's book "Hacker's Delight" (and
    supplement at http://hackersdelight.org/). Many of these are
    intended to reduce the branchiness of paths through malloc etc, as
    well as to reduce the number of memory locations read or written.

  Segments
    A list of segments headed by an embedded malloc_segment record
    representing the initial space.

  Address check support
    The least_addr field is the least address ever obtained from
    MORECORE or MMAP. Attempted frees and reallocs of any address less
    than this are trapped (unless INSECURE is defined).

  Magic tag
    A cross-check field that should always hold same value as mparams._magic.

  Max allowed footprint
    The maximum allowed bytes to allocate from system (zero means no limit)

  Flags
    Bits recording whether to use MMAP, locks, or contiguous MORECORE

  Statistics
    Each space keeps track of current and maximum system memory
    obtained via MORECORE or MMAP.

  Trim support
    Fields holding the amount of unused topmost memory that should trigger
    trimming, and a counter to force periodic scanning to release unused
    non-topmost segments.

  Locking
    If USE_LOCKS is defined, the "mutex" lock is acquired and released
    around every public call using this mspace.

  Extension support
    A void* pointer and a size_t field that can be used to help implement
    extensions to this malloc.
*/


// ================================================================================
class malloc_state 
{
public:
    /* ----------------------- _malloc, _free, etc... --- */
    FORCEINLINE void* _malloc(size_t bytes);
    FORCEINLINE void  _free(mchunkptr p);


    /* ------------------------ Relays to internal calls to malloc/free from realloc, memalign etc */
#if ONLY_MSPACES
    void *internal_malloc(size_t b) { return mspace_malloc(this, b); }
    void internal_free(void *mem)   { mspace_free(this,mem); }
#else 
    #if MSPACES
        FORCEINLINE void *internal_malloc(size_t b); 
        FORCEINLINE void internal_free(void *mem);
    #else
        void *internal_malloc(size_t b) { return dlmalloc(b); }
        void  internal_free(void *mem)  { dlfree(mem); }
    #endif
#endif

    /* ------------------------ ----------------------- */

    struct mallinfo internal_mallinfo();
    void      internal_malloc_stats();

    void      init_top(mchunkptr p, size_t psize);
    void      init_bins();
    void      init(char* tbase, size_t tsize);

    /* ------------------------ System alloc/dealloc -------------------------- */
    void*     sys_alloc(size_t nb);
    size_t    release_unused_segments();
    int       sys_trim(size_t pad);
    void      dispose_chunk(mchunkptr p, size_t psize);

    /* ----------------------- Internal support for realloc, memalign, etc --- */
    mchunkptr try_realloc_chunk(mchunkptr p, size_t nb, int can_move);
    void*     internal_memalign(size_t alignment, size_t bytes);
    void**    ialloc(size_t n_elements, size_t* sizes, int opts, void* chunks[]);
    size_t    internal_bulk_free(void* array[], size_t nelem);
    void      internal_inspect_all(void(*handler)(void *start, void *end,
                                                  size_t used_bytes, void* callback_arg),
                                   void* arg);

    /* -------------------------- system alloc setup (Operations on mflags) ----- */
    bool      use_lock() const { return !!(_mflags & USE_LOCK_BIT); }
    void      enable_lock()    { _mflags |=  USE_LOCK_BIT; }
    void      set_lock(int l)  {
        _mflags = l ? _mflags | USE_LOCK_BIT : _mflags & ~USE_LOCK_BIT;
    }

#if USE_LOCKS
    void      disable_lock()   { _mflags &= ~USE_LOCK_BIT; }
    MLOCK_T&  get_mutex()      { return _mutex; }
#else
    void      disable_lock()   {}
#endif

    bool      use_mmap() const { return !!(_mflags & USE_MMAP_BIT); }
    void      enable_mmap()    { _mflags |=  USE_MMAP_BIT; }

#if HAVE_MMAP
    void      disable_mmap()   { _mflags &= ~USE_MMAP_BIT; }
#else
    void      disable_mmap()   {}
#endif

    /* ----------------------- Runtime Check Support ------------------------- */

    /*
      For security, the main invariant is that malloc/free/etc never
      writes to a static address other than malloc_state, unless static
      malloc_state itself has been corrupted, which cannot occur via
      malloc (because of these checks). In essence this means that we
      believe all pointers, sizes, maps etc held in malloc_state, but
      check all of those linked or offsetted from other embedded data
      structures.  These checks are interspersed with main code in a way
      that tends to minimize their run-time cost.

      When FOOTERS is defined, in addition to range checking, we also
      verify footer fields of inuse chunks, which can be used guarantee
      that the mstate controlling malloc/free is intact.  This is a
      streamlined version of the approach described by William Robertson
      et al in "Run-time Detection of Heap-based Overflows" LISA'03
      http://www.usenix.org/events/lisa03/tech/robertson.html The footer
      of an inuse chunk holds the xor of its mstate and a random seed,
      that is checked upon calls to free() and realloc().  This is
      (probabalistically) unguessable from outside the program, but can be
      computed by any code successfully malloc'ing any chunk, so does not
      itself provide protection against code that has already broken
      security through some other means.  Unlike Robertson et al, we
      always dynamically check addresses of all offset chunks (previous,
      next, etc). This turns out to be cheaper than relying on hashes.
    */


#if !INSECURE
    // Check if address a is at least as high as any from MORECORE or MMAP
    bool        ok_address(void *a) const { return (char *)a >= _least_addr; }

    // Check if address of next chunk n is higher than base chunk p
    static bool ok_next(void *p, void *n) { return p < n; }

    // Check if p has inuse status
    static bool ok_inuse(mchunkptr p)     { return p->is_inuse(); }
 
    // Check if p has its pinuse bit on
    static bool ok_pinuse(mchunkptr p)    { return p->pinuse(); }

    // Check if (alleged) mstate m has expected magic field
    bool        ok_magic() const          { return _magic == mparams._magic; }
    
    // In gcc, use __builtin_expect to minimize impact of checks
    #if defined(__GNUC__) && __GNUC__ >= 3
        static bool rtcheck(bool e)       { return __builtin_expect(e, 1); }
    #else
        static bool rtcheck(bool e)       { return e; }
    #endif
#else
    static bool ok_address(void *a)       { return true; }
    static bool ok_next(void *p, void *n) { return true; }
    static bool ok_inuse(mchunkptr p)     { return true; }
    static bool ok_pinuse(mchunkptr p)    { return true; }
    static bool ok_magic()                { return true; }
    static bool rtcheck(bool e)           { return true; }
#endif

    bool is_initialized() const           { return _top != 0; }

    bool use_noncontiguous()  const       { return !!(_mflags & USE_NONCONTIGUOUS_BIT); }
    void disable_contiguous()             { _mflags |=  USE_NONCONTIGUOUS_BIT; }

    // Return segment holding given address
    msegmentptr segment_holding(char* addr) const {
        msegmentptr sp = (msegmentptr)&_seg;
        for (;;) {
            if (addr >= sp->_base && addr < sp->_base + sp->_size)
                return sp;
            if ((sp = sp->_next) == 0)
                return 0;
        }
    }

    // Return true if segment contains a segment link
    int has_segment_link(msegmentptr ss) const {
        msegmentptr sp = (msegmentptr)&_seg;
        for (;;) {
            if ((char*)sp >= ss->_base && (char*)sp < ss->_base + ss->_size)
                return 1;
            if ((sp = sp->_next) == 0)
                return 0;
        }
    }
    
#ifndef MORECORE_CANNOT_TRIM
    bool should_trim(size_t s) const { return s > _trim_check; }
#else 
    bool should_trim(size_t s) const { return false; }
#endif 


    /* -------------------------- Debugging setup ---------------------------- */

#if ! DEBUG
    void check_free_chunk(mchunkptr) {}
    void check_inuse_chunk(mchunkptr) {}
    void check_malloced_chunk(void* , size_t) {}
    void check_mmapped_chunk(mchunkptr) {}
    void check_malloc_state() {}
    void check_top_chunk(mchunkptr) {}
#else /* DEBUG */
    void check_free_chunk(mchunkptr p)       { do_check_free_chunk(p); }
    void check_inuse_chunk(mchunkptr p)      { do_check_inuse_chunk(p); }
    void check_malloced_chunk(void* p, size_t s) { do_check_malloced_chunk(p, s); }
    void check_mmapped_chunk(mchunkptr p)    { do_check_mmapped_chunk(p); }
    void check_malloc_state()                { do_check_malloc_state(); }
    void check_top_chunk(mchunkptr p)        { do_check_top_chunk(p); }

    void do_check_any_chunk(mchunkptr p) const;
    void do_check_top_chunk(mchunkptr p) const;
    void do_check_mmapped_chunk(mchunkptr p) const;
    void do_check_inuse_chunk(mchunkptr p) const;
    void do_check_free_chunk(mchunkptr p) const;
    void do_check_malloced_chunk(void* mem, size_t s) const;
    void do_check_tree(tchunkptr t);
    void do_check_treebin(bindex_t i);
    void do_check_smallbin(bindex_t i);
    void do_check_malloc_state();
    int  bin_find(mchunkptr x);
    size_t traverse_and_check();
#endif // DEBUG

private:

    /* ---------------------------- Indexing Bins ---------------------------- */

    static bool  is_small(size_t s)          { return (s >> SMALLBIN_SHIFT) < NSMALLBINS; }
    static bindex_t  small_index(size_t s)   { return (bindex_t)(s  >> SMALLBIN_SHIFT); }
    static size_t small_index2size(size_t i) { return i << SMALLBIN_SHIFT; }
    static bindex_t  MIN_SMALL_INDEX()       { return small_index(MIN_CHUNK_SIZE); }

    // assign tree index for size S to variable I. Use x86 asm if possible 
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    FORCEINLINE static bindex_t compute_tree_index(size_t S)
    {
        unsigned int X = S >> TREEBIN_SHIFT;
        if (X == 0)
            return 0;
        else if (X > 0xFFFF)
            return NTREEBINS - 1;

        unsigned int K = (unsigned) sizeof(X)*__CHAR_BIT__ - 1 - (unsigned) __builtin_clz(X); 
        return (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1)));
    }

#elif defined (__INTEL_COMPILER)
    FORCEINLINE static bindex_t compute_tree_index(size_t S)
    {
        size_t X = S >> TREEBIN_SHIFT;
        if (X == 0)
            return 0;
        else if (X > 0xFFFF)
            return NTREEBINS - 1;

        unsigned int K = _bit_scan_reverse (X); 
        return (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1)));
    }

#elif defined(_MSC_VER) && _MSC_VER>=1300
    FORCEINLINE static bindex_t compute_tree_index(size_t S)
    {
        size_t X = S >> TREEBIN_SHIFT;
        if (X == 0)
            return 0;
        else if (X > 0xFFFF)
            return NTREEBINS - 1;

        unsigned int K;
        _BitScanReverse((DWORD *) &K, (DWORD) X);
        return (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1)));
    }

#else // GNUC
    FORCEINLINE static bindex_t compute_tree_index(size_t S)
    {
        size_t X = S >> TREEBIN_SHIFT;
        if (X == 0)
            return 0;
        else if (X > 0xFFFF)
            return NTREEBINS - 1;

        unsigned int Y = (unsigned int)X;
        unsigned int N = ((Y - 0x100) >> 16) & 8;
        unsigned int K = (((Y <<= N) - 0x1000) >> 16) & 4;
        N += K;
        N += K = (((Y <<= K) - 0x4000) >> 16) & 2;
        K = 14 - N + ((Y <<= K) >> 15);
        return (K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1));
    }
#endif // GNUC

    // Shift placing maximum resolved bit in a treebin at i as sign bit
    static bindex_t leftshift_for_tree_index(bindex_t i) 
    {
        return (i == NTREEBINS - 1) ? 0 :
            ((SIZE_T_BITSIZE - 1) - ((i >> 1) + TREEBIN_SHIFT - 2));
    }

    // The size of the smallest chunk held in bin with index i
    static bindex_t minsize_for_tree_index(bindex_t i)
    {
        return ((size_t)1 << ((i >> 1) + TREEBIN_SHIFT)) | 
            (((size_t)(i & 1)) << ((i >> 1) + TREEBIN_SHIFT - 1));
    }


    // ----------- isolate the least set bit of a bitmap
    static binmap_t least_bit(binmap_t x) { return x & -x; }

    // ----------- mask with all bits to left of least bit of x on
    static binmap_t left_bits(binmap_t x) { return (x<<1) | -(x<<1); }

    // index corresponding to given bit. Use x86 asm if possible
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    static bindex_t compute_bit2idx(binmap_t X)
    {
        unsigned int J;
        J = __builtin_ctz(X); 
        return (bindex_t)J;
    }

#elif defined (__INTEL_COMPILER)
    static bindex_t compute_bit2idx(binmap_t X)
    {
        unsigned int J;
        J = _bit_scan_forward (X); 
        return (bindex_t)J;
    }

#elif defined(_MSC_VER) && _MSC_VER>=1300
    static bindex_t compute_bit2idx(binmap_t X)
    {
        unsigned int J;
        _BitScanForward((DWORD *) &J, X);
        return (bindex_t)J;
    }

#elif USE_BUILTIN_FFS
    static bindex_t compute_bit2idx(binmap_t X) { return ffs(X) - 1; }

#else
    static bindex_t compute_bit2idx(binmap_t X)
    {
        unsigned int Y = X - 1;
        unsigned int K = Y >> (16-4) & 16;
        unsigned int N = K;        Y >>= K;
        N += K = Y >> (8-3) &  8;  Y >>= K;
        N += K = Y >> (4-2) &  4;  Y >>= K;
        N += K = Y >> (2-1) &  2;  Y >>= K;
        N += K = Y >> (1-0) &  1;  Y >>= K;
        return (bindex_t)(N + Y);
    }
#endif // GNUC

    /* ------------------------ Set up inuse chunks with or without footers ---*/
#if !FOOTERS
    void mark_inuse_foot(malloc_chunk_header *, size_t) {}
#else 
    //Set foot of inuse chunk to be xor of mstate and seed 
    void  mark_inuse_foot(malloc_chunk_header *p, size_t s) {
        (((mchunkptr)((char*)p + s))->prev_foot = (size_t)this ^ mparams._magic); }
#endif

    void set_inuse(malloc_chunk_header *p, size_t s) {
        p->_head = (p->_head & PINUSE_BIT) | s | CINUSE_BIT;
        ((mchunkptr)(((char*)p) + s))->_head |= PINUSE_BIT;
        mark_inuse_foot(p,s);
    }

    void set_inuse_and_pinuse(malloc_chunk_header *p, size_t s) {
        p->_head = s | PINUSE_BIT | CINUSE_BIT;
        ((mchunkptr)(((char*)p) + s))->_head |= PINUSE_BIT;
        mark_inuse_foot(p,s);
    }

    void set_size_and_pinuse_of_inuse_chunk(malloc_chunk_header *p, size_t s) {
        p->_head = s | PINUSE_BIT | CINUSE_BIT;
        mark_inuse_foot(p, s);
    }
    
    /* ------------------------ Addressing by index. See  about smallbin repositioning --- */
    sbinptr  smallbin_at(bindex_t i) const { return (sbinptr)((char*)&_smallbins[i << 1]); }
    tbinptr* treebin_at(bindex_t i)  { return &_treebins[i]; }

    /* ----------------------- bit corresponding to given index ---------*/
    static binmap_t idx2bit(bindex_t i) { return ((binmap_t)1 << i); }

    // --------------- Mark/Clear bits with given index
    void     mark_smallmap(bindex_t i)      { _smallmap |=  idx2bit(i); }
    void     clear_smallmap(bindex_t i)     { _smallmap &= ~idx2bit(i); }
    binmap_t smallmap_is_marked(bindex_t i) const { return _smallmap & idx2bit(i); }

    void     mark_treemap(bindex_t i)       { _treemap  |=  idx2bit(i); }
    void     clear_treemap(bindex_t i)      { _treemap  &= ~idx2bit(i); }
    binmap_t treemap_is_marked(bindex_t i)  const { return _treemap & idx2bit(i); }

    /* ------------------------ ----------------------- */
    FORCEINLINE void insert_small_chunk(mchunkptr P, size_t S);
    FORCEINLINE void unlink_small_chunk(mchunkptr P, size_t S);
    FORCEINLINE void unlink_first_small_chunk(mchunkptr B, mchunkptr P, bindex_t I);
    FORCEINLINE void replace_dv(mchunkptr P, size_t S);

    /* ------------------------- Operations on trees ------------------------- */
    FORCEINLINE void insert_large_chunk(tchunkptr X, size_t S);
    FORCEINLINE void unlink_large_chunk(tchunkptr X);

    /* ------------------------ Relays to large vs small bin operations */
    FORCEINLINE void insert_chunk(mchunkptr P, size_t S);
    FORCEINLINE void unlink_chunk(mchunkptr P, size_t S);

    /* -----------------------  Direct-mmapping chunks ----------------------- */
    void*     mmap_alloc(size_t nb);
    mchunkptr mmap_resize(mchunkptr oldp, size_t nb, int flags);

    void      reset_on_error();
    void*     prepend_alloc(char* newbase, char* oldbase, size_t nb);
    void      add_segment(char* tbase, size_t tsize, flag_t mmapped);

    /* ------------------------ malloc --------------------------- */
    void*     tmalloc_large(size_t nb);
    void*     tmalloc_small(size_t nb);

    /* ------------------------Bin types, widths and sizes -------- */
    static const size_t NSMALLBINS      = 32;
    static const size_t NTREEBINS       = 32;
    static const size_t SMALLBIN_SHIFT  = 3;
    static const size_t SMALLBIN_WIDTH  = 1 << SMALLBIN_SHIFT;
    static const size_t TREEBIN_SHIFT   = 8;
    static const size_t MIN_LARGE_SIZE  = 1 << TREEBIN_SHIFT;
    static const size_t MAX_SMALL_SIZE  =  (MIN_LARGE_SIZE - 1);
    static const size_t MAX_SMALL_REQUEST = (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD);

    /* ------------------------ data members --------------------------- */
    binmap_t   _smallmap;
    binmap_t   _treemap;
    size_t     _dvsize;
    size_t     _topsize;
    char*      _least_addr;
    mchunkptr  _dv;
    mchunkptr  _top;
    size_t     _trim_check;
    size_t     _release_checks;
    size_t     _magic;
    mchunkptr  _smallbins[(NSMALLBINS+1)*2];
    tbinptr    _treebins[NTREEBINS];
public:
    size_t     _footprint;
    size_t     _max_footprint;
    size_t     _footprint_limit; // zero means no limit
    flag_t     _mflags;

#if USE_LOCKS
    MLOCK_T    _mutex;     // locate lock among fields that rarely change
#endif // USE_LOCKS
    msegment   _seg;

private:
    void*      _extp;      // Unused but available for extensions
    size_t     _exts;
};

typedef malloc_state*    mstate;

/* ------------- end malloc_state ------------------- */

#if FOOTERS
    malloc_state* get_mstate_for(malloc_chunk_header *p) {
        return (malloc_state*)(((mchunkptr)((char*)(p) +
                                     (p->chunksize())))->prev_foot ^ mparams._magic);
    }
#endif

/* ------------- Global malloc_state ------------------- */

#if !ONLY_MSPACES

// The global malloc_state used for all non-"mspace" calls
static malloc_state _gm_;
#define gm                 (&_gm_)
#define is_global(M)       ((M) == &_gm_)

#endif // !ONLY_MSPACES

/* -------------------------- system alloc setup ------------------------- */



// For mmap, use granularity alignment on windows, else page-align
#ifdef WIN32
    #define mmap_align(S) mparams.granularity_align(S)
#else
    #define mmap_align(S) mparams.page_align(S)
#endif

// For sys_alloc, enough padding to ensure can malloc request on success
#define SYS_ALLOC_PADDING (TOP_FOOT_SIZE + MALLOC_ALIGNMENT)


//  True if segment S holds address A
bool segment_holds(msegmentptr S, mchunkptr A) {
    return (char*)A >= S->_base && (char*)A < S->_base + S->_size;
}

/*
  TOP_FOOT_SIZE is padding at the end of a segment, including space
  that may be needed to place segment records and fenceposts when new
  noncontiguous segments are added.
*/
#define TOP_FOOT_SIZE \
    (align_offset(chunk2mem((void *)0))+pad_request(sizeof(struct malloc_segment))+MIN_CHUNK_SIZE)


/* -------------------------------  Hooks -------------------------------- */

/*
  PREACTION should be defined to return 0 on success, and nonzero on
  failure. If you are not using locking, you can redefine these to do
  anything you like.
*/

#if USE_LOCKS
    #define PREACTION(M)  ((M->use_lock())? ACQUIRE_LOCK(&(M)->get_mutex()) : 0)
    #define POSTACTION(M) { if (M->use_lock()) RELEASE_LOCK(&(M)->get_mutex()); }
#else // USE_LOCKS
    #ifndef PREACTION
        #define PREACTION(M) (0)
    #endif 

    #ifndef POSTACTION
        #define POSTACTION(M)
    #endif 
#endif // USE_LOCKS

/*
  CORRUPTION_ERROR_ACTION is triggered upon detected bad addresses.
  USAGE_ERROR_ACTION is triggered on detected bad frees and
  reallocs. The argument p is an address that might have triggered the
  fault. It is ignored by the two predefined actions, but might be
  useful in custom actions that try to help diagnose errors.
*/

#if PROCEED_ON_ERROR
    
    // A count of the number of corruption errors causing resets
    int malloc_corruption_error_count;
    
    #define CORRUPTION_ERROR_ACTION(m)  m->reset_on_error()
    #define USAGE_ERROR_ACTION(m, p)

#else // PROCEED_ON_ERROR
    
    #ifndef CORRUPTION_ERROR_ACTION
        #define CORRUPTION_ERROR_ACTION(m) ABORT
    #endif // CORRUPTION_ERROR_ACTION
    
    #ifndef USAGE_ERROR_ACTION
        #define USAGE_ERROR_ACTION(m,p) ABORT
    #endif // USAGE_ERROR_ACTION

#endif // PROCEED_ON_ERROR

/* ---------------------------- setting mparams -------------------------- */

#if LOCK_AT_FORK
    static void pre_fork(void)         { ACQUIRE_LOCK(&(gm)->get_mutex()); }
    static void post_fork_parent(void) { RELEASE_LOCK(&(gm)->get_mutex()); }
    static void post_fork_child(void)  { INITIAL_LOCK(&(gm)->get_mutex()); }
#endif // LOCK_AT_FORK

// Initialize mparams
int malloc_params::_init() {
#ifdef NEED_GLOBAL_LOCK_INIT
    if (malloc_global_mutex_status <= 0)
        init_malloc_global_mutex();
#endif

    ACQUIRE_MALLOC_GLOBAL_LOCK();
    if (_magic == 0) {
        size_t magic;
        size_t psize;
        size_t gsize;

#ifndef WIN32
        psize = malloc_getpagesize;
        gsize = ((DEFAULT_GRANULARITY != 0)? DEFAULT_GRANULARITY : psize);
#else // WIN32
        {
            SYSTEM_INFO system_info;
            GetSystemInfo(&system_info);
            psize = system_info.dwPageSize;
            gsize = ((DEFAULT_GRANULARITY != 0)?
                     DEFAULT_GRANULARITY : system_info.dwAllocationGranularity);
        }
#endif // WIN32

        /* Sanity-check configuration:
           size_t must be unsigned and as wide as pointer type.
           ints must be at least 4 bytes.
           alignment must be at least 8.
           Alignment, min chunk size, and page size must all be powers of 2.
        */
        if ((sizeof(size_t) != sizeof(char*)) ||
            (MAX_SIZE_T < MIN_CHUNK_SIZE)  ||
            (sizeof(int) < 4)  ||
            (MALLOC_ALIGNMENT < (size_t)8U) ||
            ((MALLOC_ALIGNMENT & (MALLOC_ALIGNMENT-1)) != 0) ||
            ((MCHUNK_SIZE      & (MCHUNK_SIZE-1))      != 0) ||
            ((gsize            & (gsize-1))            != 0) ||
            ((psize            & (psize-1))            != 0))
            ABORT;
        _granularity = gsize;
        _page_size = psize;
        _mmap_threshold = DEFAULT_MMAP_THRESHOLD;
        _trim_threshold = DEFAULT_TRIM_THRESHOLD;
#if MORECORE_CONTIGUOUS
        _default_mflags = USE_LOCK_BIT | USE_MMAP_BIT;
#else  // MORECORE_CONTIGUOUS
        _default_mflags = USE_LOCK_BIT | USE_MMAP_BIT | USE_NONCONTIGUOUS_BIT;
#endif // MORECORE_CONTIGUOUS

#if !ONLY_MSPACES
        // Set up lock for main malloc area
        gm->_mflags = _default_mflags;
        (void)INITIAL_LOCK(&gm->get_mutex());
#endif
#if LOCK_AT_FORK
        pthread_atfork(&pre_fork, &post_fork_parent, &post_fork_child);
#endif

        {
#if USE_DEV_RANDOM
            int fd;
            unsigned char buf[sizeof(size_t)];
            // Try to use /dev/urandom, else fall back on using time
            if ((fd = open("/dev/urandom", O_RDONLY)) >= 0 &&
                read(fd, buf, sizeof(buf)) == sizeof(buf)) {
                magic = *((size_t *) buf);
                close(fd);
            }
            else
#endif // USE_DEV_RANDOM
            {
#ifdef WIN32
                magic = (size_t)(GetTickCount() ^ (size_t)0x55555555U);
#elif defined(LACKS_TIME_H)
                magic = (size_t)&magic ^ (size_t)0x55555555U;
#else
                magic = (size_t)(time(0) ^ (size_t)0x55555555U);
#endif
            }
            magic |= (size_t)8U;    // ensure nonzero
            magic &= ~(size_t)7U;   // improve chances of fault for bad values
            // Until memory modes commonly available, use volatile-write
            (*(volatile size_t *)(&(_magic))) = magic;
        }
    }

    RELEASE_MALLOC_GLOBAL_LOCK();
    return 1;
}

// support for mallopt
int malloc_params::change(int param_number, int value) {
    size_t val;
    ensure_initialization();
    val = (value == -1)? MAX_SIZE_T : (size_t)value;

    switch(param_number) {
    case M_TRIM_THRESHOLD:
        _trim_threshold = val;
        return 1;

    case M_GRANULARITY:
        if (val >= _page_size && ((val & (val - 1)) == 0)) {
            _granularity = val;
            return 1;
        }
        else
            return 0;

    case M_MMAP_THRESHOLD:
        _mmap_threshold = val;
        return 1;

    default:
        return 0;
    }
}

#if DEBUG
/* ------------------------- Debugging Support --------------------------- */

// Check properties of any chunk, whether free, inuse, mmapped etc 
void malloc_state::do_check_any_chunk(mchunkptr p)  const {
    assert((is_aligned(chunk2mem(p))) || (p->_head == FENCEPOST_HEAD));
    assert(ok_address(p));
}

// Check properties of top chunk
void malloc_state::do_check_top_chunk(mchunkptr p) const {
    msegmentptr sp = segment_holding((char*)p);
    size_t  sz = p->_head & ~INUSE_BITS; // third-lowest bit can be set!
    assert(sp != 0);
    assert((is_aligned(chunk2mem(p))) || (p->_head == FENCEPOST_HEAD));
    assert(ok_address(p));
    assert(sz == _topsize);
    assert(sz > 0);
    assert(sz == ((sp->_base + sp->_size) - (char*)p) - TOP_FOOT_SIZE);
    assert(p->pinuse());
    assert(!p->chunk_plus_offset(sz)->pinuse());
}

// Check properties of (inuse) mmapped chunks
void malloc_state::do_check_mmapped_chunk(mchunkptr p) const {
    size_t  sz = p->chunksize();
    size_t len = (sz + (p->_prev_foot) + MMAP_FOOT_PAD);
    assert(p->is_mmapped());
    assert(use_mmap());
    assert((is_aligned(chunk2mem(p))) || (p->_head == FENCEPOST_HEAD));
    assert(ok_address(p));
    assert(!is_small(sz));
    assert((len & (mparams._page_size - 1)) == 0);
    assert(p->chunk_plus_offset(sz)->_head == FENCEPOST_HEAD);
    assert(p->chunk_plus_offset(sz+sizeof(size_t))->_head == 0);
}

// Check properties of inuse chunks
void malloc_state::do_check_inuse_chunk(mchunkptr p) const {
    do_check_any_chunk(p);
    assert(p->is_inuse());
    assert(p->next_pinuse());
    // If not pinuse and not mmapped, previous chunk has OK offset
    assert(p->is_mmapped() || p->pinuse() || (mchunkptr)p->prev_chunk()->next_chunk() == p);
    if (p->is_mmapped())
        do_check_mmapped_chunk(p);
}

// Check properties of free chunks
void malloc_state::do_check_free_chunk(mchunkptr p) const {
    size_t sz = p->chunksize();
    mchunkptr next = (mchunkptr)p->chunk_plus_offset(sz);
    do_check_any_chunk(p);
    assert(!p->is_inuse());
    assert(!p->next_pinuse());
    assert (!p->is_mmapped());
    if (p != _dv && p != _top) {
        if (sz >= MIN_CHUNK_SIZE) {
            assert((sz & CHUNK_ALIGN_MASK) == 0);
            assert(is_aligned(chunk2mem(p)));
            assert(next->_prev_foot == sz);
            assert(p->pinuse());
            assert (next == _top || next->is_inuse());
            assert(p->_fd->_bk == p);
            assert(p->_bk->_fd == p);
        }
        else  // markers are always of size sizeof(size_t)
            assert(sz == sizeof(size_t));
    }
}

// Check properties of malloced chunks at the point they are malloced
void malloc_state::do_check_malloced_chunk(void* mem, size_t s) const {
    if (mem != 0) {
        mchunkptr p = mem2chunk(mem);
        size_t sz = p->_head & ~INUSE_BITS;
        do_check_inuse_chunk(p);
        assert((sz & CHUNK_ALIGN_MASK) == 0);
        assert(sz >= MIN_CHUNK_SIZE);
        assert(sz >= s);
        // unless mmapped, size is less than MIN_CHUNK_SIZE more than request
        assert(p->is_mmapped() || sz < (s + MIN_CHUNK_SIZE));
    }
}

// Check a tree and its subtrees. 
void malloc_state::do_check_tree(tchunkptr t) {
    tchunkptr head = 0;
    tchunkptr u = t;
    bindex_t tindex = t->_index;
    size_t tsize = t->chunksize();
    bindex_t idx = compute_tree_index(tsize);
    assert(tindex == idx);
    assert(tsize >= MIN_LARGE_SIZE);
    assert(tsize >= minsize_for_tree_index(idx));
    assert((idx == NTREEBINS - 1) || (tsize < minsize_for_tree_index((idx+1))));

    do {
        // traverse through chain of same-sized nodes
        do_check_any_chunk((mchunkptr)u);
        assert(u->_index == tindex);
        assert(u->chunksize() == tsize);
        assert(!u->is_inuse());
        assert(!u->next_pinuse());
        assert(u->_fd->_bk == u);
        assert(u->_bk->_fd == u);
        if (u->_parent == 0) {
            assert(u->_child[0] == 0);
            assert(u->_child[1] == 0);
        }
        else {
            assert(head == 0); // only one node on chain has parent
            head = u;
            assert(u->_parent != u);
            assert (u->_parent->_child[0] == u ||
                    u->_parent->_child[1] == u ||
                    *((tbinptr*)(u->_parent)) == u);
            if (u->_child[0] != 0) {
                assert(u->_child[0]->_parent == u);
                assert(u->_child[0] != u);
                do_check_tree(u->_child[0]);
            }
            if (u->_child[1] != 0) {
                assert(u->_child[1]->_parent == u);
                assert(u->_child[1] != u);
                do_check_tree(u->_child[1]);
            }
            if (u->_child[0] != 0 && u->_child[1] != 0) {
                assert(u->_child[0]->chunksize() < u->_child[1]->chunksize());
            }
        }
        u = u->_fd;
    } while (u != t);
    assert(head != 0);
}

//  Check all the chunks in a treebin. 
void malloc_state::do_check_treebin(bindex_t i) {
    tbinptr* tb = (tbinptr*)treebin_at(i);
    tchunkptr t = *tb;
    int empty = (_treemap & (1U << i)) == 0;
    if (t == 0)
        assert(empty);
    if (!empty)
        do_check_tree(t);
}

//  Check all the chunks in a smallbin. 
void malloc_state::do_check_smallbin(bindex_t i) {
    sbinptr b = smallbin_at(i);
    mchunkptr p = b->_bk;
    unsigned int empty = (_smallmap & (1U << i)) == 0;
    if (p == b)
        assert(empty);
    if (!empty) {
        for (; p != b; p = p->_bk) {
            size_t size = p->chunksize();
            mchunkptr q;
            // each chunk claims to be free
            do_check_free_chunk(p);
            // chunk belongs in bin
            assert(small_index(size) == i);
            assert(p->_bk == b || p->_bk->chunksize() == p->chunksize());
            // chunk is followed by an inuse chunk
            q = (mchunkptr)p->next_chunk();
            if (q->_head != FENCEPOST_HEAD)
                do_check_inuse_chunk(q);
        }
    }
}

// Find x in a bin. Used in other check functions.
int malloc_state::bin_find(mchunkptr x) {
    size_t size = x->chunksize();
    if (is_small(size)) {
        bindex_t sidx = small_index(size);
        sbinptr b = smallbin_at(sidx);
        if (smallmap_is_marked(sidx)) {
            mchunkptr p = b;
            do {
                if (p == x)
                    return 1;
            } while ((p = p->_fd) != b);
        }
    }
    else {
        bindex_t tidx = compute_tree_index(size);
        if (treemap_is_marked(tidx)) {
            tchunkptr t = *treebin_at(tidx);
            size_t sizebits = size << leftshift_for_tree_index(tidx);
            while (t != 0 && t->chunksize() != size) {
                t = t->_child[(sizebits >> (SIZE_T_BITSIZE - 1)) & 1];
                sizebits <<= 1;
            }
            if (t != 0) {
                tchunkptr u = t;
                do {
                    if (u == (tchunkptr)x)
                        return 1;
                } while ((u = u->_fd) != t);
            }
        }
    }
    return 0;
}

// Traverse each chunk and check it; return total
size_t malloc_state::traverse_and_check() {
    size_t sum = 0;
    if (is_initialized()) {
        msegmentptr s = (msegmentptr)&_seg;
        sum += _topsize + TOP_FOOT_SIZE;
        while (s != 0) {
            mchunkptr q = align_as_chunk(s->_base);
            mchunkptr lastq = 0;
            assert(q->pinuse());
            while (segment_holds(s, q) &&
                   q != _top && q->_head != FENCEPOST_HEAD) {
                sum += q->chunksize();
                if (q->is_inuse()) {
                    assert(!bin_find(q));
                    do_check_inuse_chunk(q);
                }
                else {
                    assert(q == _dv || bin_find(q));
                    assert(lastq == 0 || lastq->is_inuse()); // Not 2 consecutive free
                    do_check_free_chunk(q);
                }
                lastq = q;
                q = (mchunkptr)q->next_chunk();
            }
            s = s->_next;
        }
    }
    return sum;
}


// Check all properties of malloc_state.
void malloc_state::do_check_malloc_state() {
    bindex_t i;
    size_t total;
    // check bins
    for (i = 0; i < NSMALLBINS; ++i)
        do_check_smallbin(i);
    for (i = 0; i < NTREEBINS; ++i)
        do_check_treebin(i);

    if (_dvsize != 0) { 
        // check dv chunk
        do_check_any_chunk(_dv);
        assert(_dvsize == _dv->chunksize());
        assert(_dvsize >= MIN_CHUNK_SIZE);
        assert(bin_find(_dv) == 0);
    }

    if (_top != 0) {
        // check top chunk
        do_check_top_chunk(_top);
        //assert(topsize == top->chunksize()); redundant
        assert(_topsize > 0);
        assert(bin_find(_top) == 0);
    }

    total = traverse_and_check();
    assert(total <= _footprint);
    assert(_footprint <= _max_footprint);
}
#endif // DEBUG

/* ----------------------------- statistics ------------------------------ */

#if !NO_MALLINFO
// ===============================================================================
struct mallinfo malloc_state::internal_mallinfo() {
    struct mallinfo nm = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    mparams.ensure_initialization();
    if (!PREACTION(this)) {
        check_malloc_state();
        if (is_initialized()) {
            size_t nfree = 1; // top always free
            size_t mfree = _topsize + TOP_FOOT_SIZE;
            size_t sum = mfree;
            msegmentptr s = &_seg;
            while (s != 0) {
                mchunkptr q = align_as_chunk(s->_base);
                while (segment_holds(s, q) &&
                       q != _top && q->_head != FENCEPOST_HEAD) {
                    size_t sz = q->chunksize();
                    sum += sz;
                    if (!q->is_inuse()) {
                        mfree += sz;
                        ++nfree;
                    }
                    q = (mchunkptr)q->next_chunk();
                }
                s = s->_next;
            }

            nm.arena    = sum;
            nm.ordblks  = nfree;
            nm.hblkhd   = _footprint - sum;
            nm.usmblks  = _max_footprint;
            nm.uordblks = _footprint - mfree;
            nm.fordblks = mfree;
            nm.keepcost = _topsize;
        }

        POSTACTION(this);
    }
    return nm;
}
#endif // !NO_MALLINFO

#if !NO_MALLOC_STATS
void malloc_state::internal_malloc_stats() {
    mparams.ensure_initialization();
    if (!PREACTION(this)) {
        size_t maxfp = 0;
        size_t fp = 0;
        size_t used = 0;
        check_malloc_state();
        if (is_initialized()) {
            msegmentptr s = &_seg;
            maxfp = _max_footprint;
            fp = _footprint;
            used = fp - (_topsize + TOP_FOOT_SIZE);

            while (s != 0) {
                mchunkptr q = align_as_chunk(s->_base);
                while (segment_holds(s, q) &&
                       q != _top && q->_head != FENCEPOST_HEAD) {
                    if (!q->is_inuse())
                        used -= q->chunksize();
                    q = (mchunkptr)q->next_chunk();
                }
                s = s->_next;
            }
        }
        POSTACTION(this); // drop lock
        fprintf(stderr, "max system bytes = %10lu\n", (unsigned long)(maxfp));
        fprintf(stderr, "system bytes     = %10lu\n", (unsigned long)(fp));
        fprintf(stderr, "in use bytes     = %10lu\n", (unsigned long)(used));
    }
}
#endif // NO_MALLOC_STATS

/* ----------------------- Operations on smallbins ----------------------- */

/*
  Various forms of linking and unlinking are defined as macros.  Even
  the ones for trees, which are very long but have very short typical
  paths.  This is ugly but reduces reliance on inlining support of
  compilers.
*/

// Link a free chunk into a smallbin 
void malloc_state::insert_small_chunk(mchunkptr p, size_t s) {
    bindex_t I  = small_index(s);
    mchunkptr B = smallbin_at(I);
    mchunkptr F = B;
    assert(s >= MIN_CHUNK_SIZE);
    if (!smallmap_is_marked(I))
        mark_smallmap(I);
    else if (rtcheck(ok_address(B->_fd)))
        F = B->_fd;
    else {
        CORRUPTION_ERROR_ACTION(this);
    }
    B->_fd = p;
    F->_bk = p;
    p->_fd = F;
    p->_bk = B;
}

// Unlink a chunk from a smallbin 
void malloc_state::unlink_small_chunk(mchunkptr p, size_t s) {
    mchunkptr F = p->_fd;
    mchunkptr B = p->_bk;
    bindex_t I = small_index(s);
    assert(p != B);
    assert(p != F);
    assert(p->chunksize() == small_index2size(I));
    if (rtcheck(F == smallbin_at(I) || (ok_address(F) && F->_bk == p))) { 
        if (B == F) {
            clear_smallmap(I);
        }
        else if (rtcheck(B == smallbin_at(I) ||
                         (ok_address(B) && B->_fd == p))) {
            F->_bk = B;
            B->_fd = F;
        }
        else {
            CORRUPTION_ERROR_ACTION(this);
        }
    }
    else {
        CORRUPTION_ERROR_ACTION(this);
    }
}

// Unlink the first chunk from a smallbin
void malloc_state::unlink_first_small_chunk(mchunkptr B, mchunkptr p, bindex_t I) {
    mchunkptr F = p->_fd;
    assert(p != B);
    assert(p != F);
    assert(p->chunksize() == small_index2size(I));
    if (B == F) {
        clear_smallmap(I);
    }
    else if (rtcheck(ok_address(F) && F->_bk == p)) {
        F->_bk = B;
        B->_fd = F;
    }
    else {
        CORRUPTION_ERROR_ACTION(this);
    }
}

// Replace dv node, binning the old one
// Used only when dvsize known to be small
void malloc_state::replace_dv(mchunkptr p, size_t s) {
    size_t DVS = _dvsize;
    assert(is_small(DVS));
    if (DVS != 0) {
        mchunkptr DV = _dv;
        insert_small_chunk(DV, DVS);
    }
    _dvsize = s;
    _dv = p;
}

/* ------------------------- Operations on trees ------------------------- */

// Insert chunk into tree
void malloc_state::insert_large_chunk(tchunkptr X, size_t s) {
    tbinptr* H;
    bindex_t I = compute_tree_index(s);
    H = treebin_at(I);
    X->_index = I;
    X->_child[0] = X->_child[1] = 0;
    if (!treemap_is_marked(I)) {
        mark_treemap(I);
        *H = X;
        X->_parent = (tchunkptr)H;
        X->_fd = X->_bk = X;
    }
    else {
        tchunkptr T = *H;
        size_t K = s << leftshift_for_tree_index(I);
        for (;;) {
            if (T->chunksize() != s) {
                tchunkptr* C = &(T->_child[(K >> (SIZE_T_BITSIZE - 1)) & 1]);
                K <<= 1;
                if (*C != 0)
                    T = *C;
                else if (rtcheck(ok_address(C))) {
                    *C = X;
                    X->_parent = T;
                    X->_fd = X->_bk = X;
                    break;
                }
                else {
                    CORRUPTION_ERROR_ACTION(this);
                    break;
                }
            }
            else {
                tchunkptr F = T->_fd;
                if (rtcheck(ok_address(T) && ok_address(F))) {
                    T->_fd = F->_bk = X;
                    X->_fd = F;
                    X->_bk = T;
                    X->_parent = 0;
                    break;
                }
                else {
                    CORRUPTION_ERROR_ACTION(this);
                    break;
                }
            }
        }
    }
}

/*
  Unlink steps:

  1. If x is a chained node, unlink it from its same-sized fd/bk links
     and choose its bk node as its replacement.
  2. If x was the last node of its size, but not a leaf node, it must
     be replaced with a leaf node (not merely one with an open left or
     right), to make sure that lefts and rights of descendents
     correspond properly to bit masks.  We use the rightmost descendent
     of x.  We could use any other leaf, but this is easy to locate and
     tends to counteract removal of leftmosts elsewhere, and so keeps
     paths shorter than minimally guaranteed.  This doesn't loop much
     because on average a node in a tree is near the bottom.
  3. If x is the base of a chain (i.e., has parent links) relink
     x's parent and children to x's replacement (or null if none).
*/

void malloc_state::unlink_large_chunk(tchunkptr X) {
    tchunkptr XP = X->_parent;
    tchunkptr R;
    if (X->_bk != X) {
        tchunkptr F = X->_fd;
        R = X->_bk;
        if (rtcheck(ok_address(F) && F->_bk == X && R->_fd == X)) {
            F->_bk = R;
            R->_fd = F;
        }
        else {
            CORRUPTION_ERROR_ACTION(this);
        }
    }
    else {
        tchunkptr* RP;
        if (((R = *(RP = &(X->_child[1]))) != 0) ||
            ((R = *(RP = &(X->_child[0]))) != 0)) {
            tchunkptr* CP;
            while ((*(CP = &(R->_child[1])) != 0) ||
                   (*(CP = &(R->_child[0])) != 0)) {
                R = *(RP = CP);
            }
            if (rtcheck(ok_address(RP)))
                *RP = 0;
            else {
                CORRUPTION_ERROR_ACTION(this);
            }
        }
    }
    if (XP != 0) {
        tbinptr* H = treebin_at(X->_index);
        if (X == *H) {
            if ((*H = R) == 0) 
                clear_treemap(X->_index);
        }
        else if (rtcheck(ok_address(XP))) {
            if (XP->_child[0] == X) 
                XP->_child[0] = R;
            else 
                XP->_child[1] = R;
        }
        else
            CORRUPTION_ERROR_ACTION(this);
        if (R != 0) {
            if (rtcheck(ok_address(R))) {
                tchunkptr C0, C1;
                R->_parent = XP;
                if ((C0 = X->_child[0]) != 0) {
                    if (rtcheck(ok_address(C0))) {
                        R->_child[0] = C0;
                        C0->_parent = R;
                    }
                    else
                        CORRUPTION_ERROR_ACTION(this);
                }
                if ((C1 = X->_child[1]) != 0) {
                    if (rtcheck(ok_address(C1))) {
                        R->_child[1] = C1;
                        C1->_parent = R;
                    }
                    else
                        CORRUPTION_ERROR_ACTION(this);
                }
            }
            else
                CORRUPTION_ERROR_ACTION(this);
        }
    }
}

// Relays to large vs small bin operations

void malloc_state::insert_chunk(mchunkptr p, size_t s)
{
    if (is_small(s)) 
        insert_small_chunk(p, s);
    else 
    { 
        tchunkptr tp = (tchunkptr)(p); 
        insert_large_chunk(tp, s); 
    }
}

void malloc_state::unlink_chunk(mchunkptr p, size_t s)
{
    if (is_small(s)) 
        unlink_small_chunk(p, s);
    else 
    {
        tchunkptr tp = (tchunkptr)(p); 
        unlink_large_chunk(tp); 
    }
}


// Relays to internal calls to malloc/free from realloc, memalign etc

#if !ONLY_MSPACES && MSPACES
    void *malloc_state::internal_malloc(size_t b) {
        return ((this == gm)? dlmalloc(b) : mspace_malloc(this, b)); 
    }
    void malloc_state::internal_free(void *mem) {
        if (this == gm) dlfree(mem); else mspace_free(this,mem);
    }
#endif

/* -----------------------  Direct-mmapping chunks ----------------------- */

/*
  Directly mmapped chunks are set up with an offset to the start of
  the mmapped region stored in the prev_foot field of the chunk. This
  allows reconstruction of the required argument to MUNMAP when freed,
  and also allows adjustment of the returned chunk to meet alignment
  requirements (especially in memalign).
*/

// Malloc using mmap
void* malloc_state::mmap_alloc(size_t nb) {
    size_t mmsize = mmap_align(nb + 6 * sizeof(size_t) + CHUNK_ALIGN_MASK);
    if (_footprint_limit != 0) {
        size_t fp = _footprint + mmsize;
        if (fp <= _footprint || fp > _footprint_limit)
            return 0;
    }
    if (mmsize > nb) {
        // Check for wrap around 0
        char* mm = (char*)(CALL_DIRECT_MMAP(mmsize));
        if (mm != CMFAIL) {
            size_t offset = align_offset(chunk2mem(mm));
            size_t psize = mmsize - offset - MMAP_FOOT_PAD;
            mchunkptr p = (mchunkptr)(mm + offset);
            p->_prev_foot = offset;
            p->_head = psize;
            mark_inuse_foot(p, psize);
            p->chunk_plus_offset(psize)->_head = FENCEPOST_HEAD;
            p->chunk_plus_offset(psize+sizeof(size_t))->_head = 0;

            if (_least_addr == 0 || mm < _least_addr)
                _least_addr = mm;
            if ((_footprint += mmsize) > _max_footprint)
                _max_footprint = _footprint;
            assert(is_aligned(chunk2mem(p)));
            check_mmapped_chunk(p);
            return chunk2mem(p);
        }
    }
    return 0;
}

// Realloc using mmap
mchunkptr malloc_state::mmap_resize(mchunkptr oldp, size_t nb, int flags) {
    size_t oldsize = oldp->chunksize();
    (void)flags;      // placate people compiling -Wunused
    if (is_small(nb)) // Can't shrink mmap regions below small size
        return 0;

    // Keep old chunk if big enough but not too big
    if (oldsize >= nb + sizeof(size_t) &&
        (oldsize - nb) <= (mparams._granularity << 1))
        return oldp;
    else {
        size_t offset = oldp->_prev_foot;
        size_t oldmmsize = oldsize + offset + MMAP_FOOT_PAD;
        size_t newmmsize = mmap_align(nb + 6 * sizeof(size_t) + CHUNK_ALIGN_MASK);
        char* cp = (char*)CALL_MREMAP((char*)oldp - offset,
                                      oldmmsize, newmmsize, flags);
        if (cp != CMFAIL) {
            mchunkptr newp = (mchunkptr)(cp + offset);
            size_t psize = newmmsize - offset - MMAP_FOOT_PAD;
            newp->_head = psize;
            mark_inuse_foot(newp, psize);
            newp->chunk_plus_offset(psize)->_head = FENCEPOST_HEAD;
            newp->chunk_plus_offset(psize+sizeof(size_t))->_head = 0;

            if (cp < _least_addr)
                _least_addr = cp;
            if ((_footprint += newmmsize - oldmmsize) > _max_footprint)
                _max_footprint = _footprint;
            check_mmapped_chunk(newp);
            return newp;
        }
    }
    return 0;
}


/* -------------------------- mspace management -------------------------- */

// Initialize top chunk and its size
void malloc_state::init_top(mchunkptr p, size_t psize) {
    // Ensure alignment
    size_t offset = align_offset(chunk2mem(p));
    p = (mchunkptr)((char*)p + offset);
    psize -= offset;

    _top = p;
    _topsize = psize;
    p->_head = psize | PINUSE_BIT;
    // set size of fake trailing chunk holding overhead space only once
    p->chunk_plus_offset(psize)->_head = TOP_FOOT_SIZE;
    _trim_check = mparams._trim_threshold; // reset on each update
}

// Initialize bins for a new mstate that is otherwise zeroed out
void malloc_state::init_bins() {
    // Establish circular links for smallbins
    bindex_t i;
    for (i = 0; i < NSMALLBINS; ++i) {
        sbinptr bin = smallbin_at(i);
        bin->_fd = bin->_bk = bin;
    }
}

#if PROCEED_ON_ERROR

// default corruption action
void malloc_state::reset_on_error() {
    int i;
    ++malloc_corruption_error_count;
    // Reinitialize fields to forget about all memory
    _smallmap = _treemap = 0;
    _dvsize = _topsize = 0;
    _seg._base = 0;
    _seg._size = 0;
    _seg._next = 0;
    _top = _dv = 0;
    for (i = 0; i < NTREEBINS; ++i)
        *treebin_at(i) = 0;
    init_bins();
}
#endif // PROCEED_ON_ERROR

/* Allocate chunk and prepend remainder with chunk in successor base. */
void* malloc_state::prepend_alloc(char* newbase, char* oldbase, size_t nb) {
    mchunkptr p = align_as_chunk(newbase);
    mchunkptr oldfirst = align_as_chunk(oldbase);
    size_t psize = (char*)oldfirst - (char*)p;
    mchunkptr q = (mchunkptr)p->chunk_plus_offset(nb);
    size_t qsize = psize - nb;
    set_size_and_pinuse_of_inuse_chunk(p, nb);

    assert((char*)oldfirst > (char*)q);
    assert(oldfirst->pinuse());
    assert(qsize >= MIN_CHUNK_SIZE);

    // consolidate remainder with first chunk of old base
    if (oldfirst == _top) {
        size_t tsize = _topsize += qsize;
        _top = q;
        q->_head = tsize | PINUSE_BIT;
        check_top_chunk(q);
    }
    else if (oldfirst == _dv) {
        size_t dsize = _dvsize += qsize;
        _dv = q;
        q->set_size_and_pinuse_of_free_chunk(dsize);
    }
    else {
        if (!oldfirst->is_inuse()) {
            size_t nsize = oldfirst->chunksize();
            unlink_chunk(oldfirst, nsize);
            oldfirst = (mchunkptr)oldfirst->chunk_plus_offset(nsize);
            qsize += nsize;
        }
        q->set_free_with_pinuse(qsize, oldfirst);
        insert_chunk(q, qsize);
        check_free_chunk(q);
    }

    check_malloced_chunk(chunk2mem(p), nb);
    return chunk2mem(p);
}

// Add a segment to hold a new noncontiguous region
void malloc_state::add_segment(char* tbase, size_t tsize, flag_t mmapped) {
    // Determine locations and sizes of segment, fenceposts, old top
    char* old_top = (char*)_top;
    msegmentptr oldsp = segment_holding(old_top);
    char* old_end = oldsp->_base + oldsp->_size;
    size_t ssize = pad_request(sizeof(struct malloc_segment));
    char* rawsp = old_end - (ssize + 4 * sizeof(size_t) + CHUNK_ALIGN_MASK);
    size_t offset = align_offset(chunk2mem(rawsp));
    char* asp = rawsp + offset;
    char* csp = (asp < (old_top + MIN_CHUNK_SIZE))? old_top : asp;
    mchunkptr sp = (mchunkptr)csp;
    msegmentptr ss = (msegmentptr)(chunk2mem(sp));
    mchunkptr tnext = (mchunkptr)sp->chunk_plus_offset(ssize);
    mchunkptr p = tnext;
    int nfences = 0;

    // reset top to new space
    init_top((mchunkptr)tbase, tsize - TOP_FOOT_SIZE);

    // Set up segment record
    assert(is_aligned(ss));
    set_size_and_pinuse_of_inuse_chunk(sp, ssize);
    *ss = _seg; // Push current record
    _seg._base = tbase;
    _seg._size = tsize;
    _seg._sflags = mmapped;
    _seg._next = ss;

    // Insert trailing fenceposts
    for (;;) {
        mchunkptr nextp = (mchunkptr)p->chunk_plus_offset(sizeof(size_t));
        p->_head = FENCEPOST_HEAD;
        ++nfences;
        if ((char*)(&(nextp->_head)) < old_end)
            p = nextp;
        else
            break;
    }
    assert(nfences >= 2);

    // Insert the rest of old top into a bin as an ordinary free chunk
    if (csp != old_top) {
        mchunkptr q = (mchunkptr)old_top;
        size_t psize = csp - old_top;
        mchunkptr tn = (mchunkptr)q->chunk_plus_offset(psize);
        q->set_free_with_pinuse(psize, tn);
        insert_chunk(q, psize);
    }

    check_top_chunk(_top);
}

/* -------------------------- System allocation -------------------------- */

// Get memory from system using MORECORE or MMAP
void* malloc_state::sys_alloc(size_t nb) {
    char* tbase = CMFAIL;
    size_t tsize = 0;
    flag_t mmap_flag = 0;
    size_t asize; // allocation size

    mparams.ensure_initialization();

    // Directly map large chunks, but only if already initialized
    if (use_mmap() && nb >= mparams._mmap_threshold && _topsize != 0) {
        void* mem = mmap_alloc(nb);
        if (mem != 0)
            return mem;
    }

    asize = mparams.granularity_align(nb + SYS_ALLOC_PADDING);
    if (asize <= nb)
        return 0; // wraparound
    if (_footprint_limit != 0) {
        size_t fp = _footprint + asize;
        if (fp <= _footprint || fp > _footprint_limit)
            return 0;
    }

    /*
      Try getting memory in any of three ways (in most-preferred to
      least-preferred order):
      1. A call to MORECORE that can normally contiguously extend memory.
      (disabled if not MORECORE_CONTIGUOUS or not HAVE_MORECORE or
      or main space is mmapped or a previous contiguous call failed)
      2. A call to MMAP new space (disabled if not HAVE_MMAP).
      Note that under the default settings, if MORECORE is unable to
      fulfill a request, and HAVE_MMAP is true, then mmap is
      used as a noncontiguous system allocator. This is a useful backup
      strategy for systems with holes in address spaces -- in this case
      sbrk cannot contiguously expand the heap, but mmap may be able to
      find space.
      3. A call to MORECORE that cannot usually contiguously extend memory.
      (disabled if not HAVE_MORECORE)

      In all cases, we need to request enough bytes from system to ensure
      we can malloc nb bytes upon success, so pad with enough space for
      top_foot, plus alignment-pad to make sure we don't lose bytes if
      not on boundary, and round this up to a granularity unit.
    */

    if (MORECORE_CONTIGUOUS && !use_noncontiguous()) {
        char* br = CMFAIL;
        size_t ssize = asize; // sbrk call size
        msegmentptr ss = (_top == 0)? 0 : segment_holding((char*)_top);
        ACQUIRE_MALLOC_GLOBAL_LOCK();

        if (ss == 0) {
            // First time through or recovery
            char* base = (char*)CALL_MORECORE(0);
            if (base != CMFAIL) {
                size_t fp;
                // Adjust to end on a page boundary
                if (!mparams.is_page_aligned(base))
                    ssize += (mparams.page_align((size_t)base) - (size_t)base);
                fp = _footprint + ssize; // recheck limits
                if (ssize > nb && ssize < HALF_MAX_SIZE_T &&
                    (_footprint_limit == 0 ||
                     (fp > _footprint && fp <= _footprint_limit)) &&
                    (br = (char*)(CALL_MORECORE(ssize))) == base) {
                    tbase = base;
                    tsize = ssize;
                }
            }
        }
        else {
            // Subtract out existing available top space from MORECORE request.
            ssize = mparams.granularity_align(nb - _topsize + SYS_ALLOC_PADDING);
            // Use mem here only if it did continuously extend old space
            if (ssize < HALF_MAX_SIZE_T &&
                (br = (char*)(CALL_MORECORE(ssize))) == ss->_base+ss->_size) {
                tbase = br;
                tsize = ssize;
            }
        }

        if (tbase == CMFAIL) {
            // Cope with partial failure
            if (br != CMFAIL) {
                // Try to use/extend the space we did get
                if (ssize < HALF_MAX_SIZE_T &&
                    ssize < nb + SYS_ALLOC_PADDING) {
                    size_t esize = mparams.granularity_align(nb + SYS_ALLOC_PADDING - ssize);
                    if (esize < HALF_MAX_SIZE_T) {
                        char* end = (char*)CALL_MORECORE(esize);
                        if (end != CMFAIL)
                            ssize += esize;
                        else {
                            // Can't use; try to release
                            (void) CALL_MORECORE(-ssize);
                            br = CMFAIL;
                        }
                    }
                }
            }
            if (br != CMFAIL) {
                // Use the space we did get
                tbase = br;
                tsize = ssize;
            }
            else
                disable_contiguous(); // Don't try contiguous path in the future
        }

        RELEASE_MALLOC_GLOBAL_LOCK();
    }

    if (HAVE_MMAP && tbase == CMFAIL) {
        // Try MMAP
        char* mp = (char*)(CALL_MMAP(asize));
        if (mp != CMFAIL) {
            tbase = mp;
            tsize = asize;
            mmap_flag = USE_MMAP_BIT;
        }
    }

    if (HAVE_MORECORE && tbase == CMFAIL) {
        // Try noncontiguous MORECORE
        if (asize < HALF_MAX_SIZE_T) {
            char* br = CMFAIL;
            char* end = CMFAIL;
            ACQUIRE_MALLOC_GLOBAL_LOCK();
            br = (char*)(CALL_MORECORE(asize));
            end = (char*)(CALL_MORECORE(0));
            RELEASE_MALLOC_GLOBAL_LOCK();
            if (br != CMFAIL && end != CMFAIL && br < end) {
                size_t ssize = end - br;
                if (ssize > nb + TOP_FOOT_SIZE) {
                    tbase = br;
                    tsize = ssize;
                }
            }
        }
    }

    if (tbase != CMFAIL) {

        if ((_footprint += tsize) > _max_footprint)
            _max_footprint = _footprint;

        if (!is_initialized()) {
            // first-time initialization
            if (_least_addr == 0 || tbase < _least_addr)
                _least_addr = tbase;
            _seg._base = tbase;
            _seg._size = tsize;
            _seg._sflags = mmap_flag;
            _magic = mparams._magic;
            _release_checks = MAX_RELEASE_CHECK_RATE;
            init_bins();
#if !ONLY_MSPACES
            if (is_global(this))
                init_top((mchunkptr)tbase, tsize - TOP_FOOT_SIZE);
            else
#endif
            {
                // Offset top by embedded malloc_state
                mchunkptr mn = (mchunkptr)mem2chunk(this)->next_chunk();
                init_top(mn, (size_t)((tbase + tsize) - (char*)mn) -TOP_FOOT_SIZE);
            }
        }

        else {
            // Try to merge with an existing segment
            msegmentptr sp = &_seg;
            // Only consider most recent segment if traversal suppressed
            while (sp != 0 && tbase != sp->_base + sp->_size)
                sp = (NO_SEGMENT_TRAVERSAL) ? 0 : sp->_next;
            if (sp != 0 &&
                !sp->is_extern_segment() &&
                (sp->_sflags & USE_MMAP_BIT) == mmap_flag &&
                segment_holds(sp, _top)) {
                // append
                sp->_size += tsize;
                init_top(_top, _topsize + tsize);
            }
            else {
                if (tbase < _least_addr)
                    _least_addr = tbase;
                sp = &_seg;
                while (sp != 0 && sp->_base != tbase + tsize)
                    sp = (NO_SEGMENT_TRAVERSAL) ? 0 : sp->_next;
                if (sp != 0 &&
                    !sp->is_extern_segment() &&
                    (sp->_sflags & USE_MMAP_BIT) == mmap_flag) {
                    char* oldbase = sp->_base;
                    sp->_base = tbase;
                    sp->_size += tsize;
                    return prepend_alloc(tbase, oldbase, nb);
                }
                else
                    add_segment(tbase, tsize, mmap_flag);
            }
        }

        if (nb < _topsize) {
            // Allocate from new or extended top space
            size_t rsize = _topsize -= nb;
            mchunkptr p = _top;
            mchunkptr r = _top = (mchunkptr)p->chunk_plus_offset(nb);
            r->_head = rsize | PINUSE_BIT;
            set_size_and_pinuse_of_inuse_chunk(p, nb);
            check_top_chunk(_top);
            check_malloced_chunk(chunk2mem(p), nb);
            return chunk2mem(p);
        }
    }

    MALLOC_FAILURE_ACTION;
    return 0;
}

/* -----------------------  system deallocation -------------------------- */

// Unmap and unlink any mmapped segments that don't contain used chunks
size_t malloc_state::release_unused_segments() {
    size_t released = 0;
    int nsegs = 0;
    msegmentptr pred = &_seg;
    msegmentptr sp = pred->_next;
    while (sp != 0) {
        char* base = sp->_base;
        size_t size = sp->_size;
        msegmentptr next = sp->_next;
        ++nsegs;
        if (sp->is_mmapped_segment() && !sp->is_extern_segment()) {
            mchunkptr p = align_as_chunk(base);
            size_t psize = p->chunksize();
            // Can unmap if first chunk holds entire segment and not pinned
            if (!p->is_inuse() && (char*)p + psize >= base + size - TOP_FOOT_SIZE) {
                tchunkptr tp = (tchunkptr)p;
                assert(segment_holds(sp, p));
                if (p == _dv) {
                    _dv = 0;
                    _dvsize = 0;
                }
                else {
                    unlink_large_chunk(tp);
                }
                if (CALL_MUNMAP(base, size) == 0) {
                    released += size;
                    _footprint -= size;
                    // unlink obsoleted record
                    sp = pred;
                    sp->_next = next;
                }
                else {
                    // back out if cannot unmap
                    insert_large_chunk(tp, psize);
                }
            }
        }
        if (NO_SEGMENT_TRAVERSAL) // scan only first segment
            break;
        pred = sp;
        sp = next;
    }
    // Reset check counter
    _release_checks = (((size_t) nsegs > (size_t) MAX_RELEASE_CHECK_RATE)?
                         (size_t) nsegs : (size_t) MAX_RELEASE_CHECK_RATE);
    return released;
}

int malloc_state::sys_trim(size_t pad) {
    size_t released = 0;
    mparams.ensure_initialization();
    if (pad < MAX_REQUEST && is_initialized()) {
        pad += TOP_FOOT_SIZE; // ensure enough room for segment overhead

        if (_topsize > pad) {
            // Shrink top space in _granularity - size units, keeping at least one
            size_t unit = mparams._granularity;
            size_t extra = ((_topsize - pad + (unit - 1)) / unit -
                            1) * unit;
            msegmentptr sp = segment_holding((char*)_top);

            if (!sp->is_extern_segment()) {
                if (sp->is_mmapped_segment()) {
                    if (HAVE_MMAP &&
                        sp->_size >= extra &&
                        !has_segment_link(sp)) {
                        // can't shrink if pinned
                        size_t newsize = sp->_size - extra;
                        (void)newsize; // placate people compiling -Wunused-variable
                        // Prefer mremap, fall back to munmap
                        if ((CALL_MREMAP(sp->_base, sp->_size, newsize, 0) != MFAIL) ||
                            (CALL_MUNMAP(sp->_base + newsize, extra) == 0)) {
                            released = extra;
                        }
                    }
                }
                else if (HAVE_MORECORE) {
                    if (extra >= HALF_MAX_SIZE_T) // Avoid wrapping negative
                        extra = (HALF_MAX_SIZE_T) + 1 - unit;
                    ACQUIRE_MALLOC_GLOBAL_LOCK();
                    {
                        // Make sure end of memory is where we last set it.
                        char* old_br = (char*)(CALL_MORECORE(0));
                        if (old_br == sp->_base + sp->_size) {
                            char* rel_br = (char*)(CALL_MORECORE(-extra));
                            char* new_br = (char*)(CALL_MORECORE(0));
                            if (rel_br != CMFAIL && new_br < old_br)
                                released = old_br - new_br;
                        }
                    }
                    RELEASE_MALLOC_GLOBAL_LOCK();
                }
            }

            if (released != 0) {
                sp->_size -= released;
                _footprint -= released;
                init_top(_top, _topsize - released);
                check_top_chunk(_top);
            }
        }

        // Unmap any unused mmapped segments
        if (HAVE_MMAP)
            released += release_unused_segments();

        // On failure, disable autotrim to avoid repeated failed future calls
        if (released == 0 && _topsize > _trim_check)
            _trim_check = MAX_SIZE_T;
    }

    return (released != 0)? 1 : 0;
}

/* Consolidate and bin a chunk. Differs from exported versions
   of free mainly in that the chunk need not be marked as inuse.
*/
void malloc_state::dispose_chunk(mchunkptr p, size_t psize) {
    mchunkptr next = (mchunkptr)p->chunk_plus_offset(psize);
    if (!p->pinuse()) {
        mchunkptr prev;
        size_t prevsize = p->_prev_foot;
        if (p->is_mmapped()) {
            psize += prevsize + MMAP_FOOT_PAD;
            if (CALL_MUNMAP((char*)p - prevsize, psize) == 0)
                _footprint -= psize;
            return;
        }
        prev = (mchunkptr)p->chunk_minus_offset(prevsize);
        psize += prevsize;
        p = prev;
        if (rtcheck(ok_address(prev))) {
            // consolidate backward
            if (p != _dv) {
                unlink_chunk(p, prevsize);
            }
            else if ((next->_head & INUSE_BITS) == INUSE_BITS) {
                _dvsize = psize;
                if (p) p->set_free_with_pinuse(psize, next);
                return;
            }
        }
        else {
            CORRUPTION_ERROR_ACTION(this);
            return;
        }
    }
    if (rtcheck(ok_address(next))) {
        if (!next->cinuse()) {
            // consolidate forward
            if (next == _top) {
                size_t tsize = _topsize += psize;
                _top = p;
                p->_head = tsize | PINUSE_BIT;
                if (p == _dv) {
                    _dv = 0;
                    _dvsize = 0;
                }
                return;
            }
            else if (next == _dv) {
                size_t dsize = _dvsize += psize;
                _dv = p;
                p->set_size_and_pinuse_of_free_chunk(dsize);
                return;
            }
            else {
                size_t nsize = next->chunksize();
                psize += nsize;
                unlink_chunk(next, nsize);
                p->set_size_and_pinuse_of_free_chunk(psize);
                if (p == _dv) {
                    _dvsize = psize;
                    return;
                }
            }
        }
        else {
            p->set_free_with_pinuse(psize, next);
        }
        insert_chunk(p, psize);
    }
    else {
        CORRUPTION_ERROR_ACTION(this);
    }
}

/* ---------------------------- malloc --------------------------- */

// allocate a large request from the best fitting chunk in a treebin
void* malloc_state::tmalloc_large(size_t nb) {
    tchunkptr v = 0;
    size_t rsize = -nb; // Unsigned negation
    tchunkptr t;
    bindex_t idx = compute_tree_index(nb);
    if ((t = *treebin_at(idx)) != 0) {
        // Traverse tree for this bin looking for node with size == nb
        size_t sizebits = nb << leftshift_for_tree_index(idx);
        tchunkptr rst = 0;  // The deepest untaken right subtree
        for (;;) {
            tchunkptr rt;
            size_t trem = t->chunksize() - nb;
            if (trem < rsize) {
                v = t;
                if ((rsize = trem) == 0)
                    break;
            }
            rt = t->_child[1];
            t = t->_child[(sizebits >> (SIZE_T_BITSIZE - 1)) & 1];
            if (rt != 0 && rt != t)
                rst = rt;
            if (t == 0) {
                t = rst; // set t to least subtree holding sizes > nb
                break;
            }
            sizebits <<= 1;
        }
    }
    if (t == 0 && v == 0) {
        // set t to root of next non-empty treebin
        binmap_t leftbits = left_bits(idx2bit(idx)) & _treemap;
        if (leftbits != 0) {
            binmap_t leastbit = least_bit(leftbits);
            bindex_t i = compute_bit2idx(leastbit);
            t = *treebin_at(i);
        }
    }

    while (t != 0) {
        // find smallest of tree or subtree
        size_t trem = t->chunksize() - nb;
        if (trem < rsize) {
            rsize = trem;
            v = t;
        }
        t = t->leftmost_child();
    }

    //  If dv is a better fit, return 0 so malloc will use it
    if (v != 0 && rsize < (size_t)(_dvsize - nb)) {
        if (rtcheck(ok_address(v))) {
            // split
            mchunkptr r = (mchunkptr)v->chunk_plus_offset(nb);
            assert(v->chunksize() == rsize + nb);
            if (rtcheck(ok_next(v, r))) {
                unlink_large_chunk(v);
                if (rsize < MIN_CHUNK_SIZE)
                    set_inuse_and_pinuse(v, (rsize + nb));
                else {
                    set_size_and_pinuse_of_inuse_chunk(v, nb);
                    r->set_size_and_pinuse_of_free_chunk(rsize);
                    insert_chunk(r, rsize);
                }
                return chunk2mem(v);
            }
        }
        CORRUPTION_ERROR_ACTION(this);
    }
    return 0;
}

// allocate a small request from the best fitting chunk in a treebin
void* malloc_state::tmalloc_small(size_t nb) {
    tchunkptr t, v;
    size_t rsize;
    binmap_t leastbit = least_bit(_treemap);
    bindex_t i = compute_bit2idx(leastbit);
    v = t = *treebin_at(i);
    rsize = t->chunksize() - nb;

    while ((t = t->leftmost_child()) != 0) {
        size_t trem = t->chunksize() - nb;
        if (trem < rsize) {
            rsize = trem;
            v = t;
        }
    }

    if (rtcheck(ok_address(v))) {
        mchunkptr r = (mchunkptr)v->chunk_plus_offset(nb);
        assert(v->chunksize() == rsize + nb);
        if (rtcheck(ok_next(v, r))) {
            unlink_large_chunk(v);
            if (rsize < MIN_CHUNK_SIZE)
                set_inuse_and_pinuse(v, (rsize + nb));
            else {
                set_size_and_pinuse_of_inuse_chunk(v, nb);
                r->set_size_and_pinuse_of_free_chunk(rsize);
                replace_dv(r, rsize);
            }
            return chunk2mem(v);
        }
    }

    CORRUPTION_ERROR_ACTION(this);
    return 0;
}

#if !ONLY_MSPACES

void* dlmalloc(size_t bytes) DLTHROW {
    /*
      Basic algorithm:
      If a small request (< 256 bytes minus per-chunk overhead):
      1. If one exists, use a remainderless chunk in associated smallbin.
      (Remainderless means that there are too few excess bytes to
      represent as a chunk.)
      2. If it is big enough, use the dv chunk, which is normally the
      chunk adjacent to the one used for the most recent small request.
      3. If one exists, split the smallest available chunk in a bin,
      saving remainder in dv.
      4. If it is big enough, use the top chunk.
      5. If available, get memory from system and use it
      Otherwise, for a large request:
      1. Find the smallest available binned chunk that fits, and use it
      if it is better fitting than dv chunk, splitting if necessary.
      2. If better fitting than any binned chunk, use the dv chunk.
      3. If it is big enough, use the top chunk.
      4. If request size >= mmap threshold, try to directly mmap this chunk.
      5. If available, get memory from system and use it

      The ugly goto's here ensure that postaction occurs along all paths.
    */

#if USE_LOCKS
    mparams.ensure_initialization(); // initialize in sys_alloc if not using locks
#endif
    return gm->_malloc(bytes);
}

void dlfree(void* mem) DLTHROW {
    /*
      Consolidate freed chunks with preceeding or succeeding bordering
      free chunks, if they exist, and then place in a bin.  Intermixed
      with special cases for top, dv, mmapped chunks, and usage errors.
    */

    if (mem != 0) {
        mchunkptr p  = mem2chunk(mem);
#if FOOTERS
        mstate fm = get_mstate_for(p);
        if (!fm->ok_magic()) {
            USAGE_ERROR_ACTION(fm, p);
            return;
        }
        fm->_free(p);
#else // FOOTERS
        gm->_free(p);
#endif // FOOTERS
    }
}

void* dlcalloc(size_t n_elements, size_t elem_size) DLTHROW {
    void* mem;
    size_t req = 0;
    if (n_elements != 0) {
        req = n_elements * elem_size;
        if (((n_elements | elem_size) & ~(size_t)0xffff) &&
            (req / n_elements != elem_size))
            req = MAX_SIZE_T; // force downstream failure on overflow
    }
    mem = dlmalloc(req);
    if (mem != 0 && mem2chunk(mem)->calloc_must_clear())
        memset(mem, 0, req);
    return mem;
}

#endif // !ONLY_MSPACES

/* ---------------------------- malloc --------------------------- */

void* malloc_state::_malloc(size_t bytes) {
    if (!PREACTION(this)) {
        void* mem;
        size_t nb;
        if (bytes <= MAX_SMALL_REQUEST) {
            bindex_t idx;
            binmap_t smallbits;
            nb = (bytes < MIN_REQUEST)? MIN_CHUNK_SIZE : pad_request(bytes);
            idx = small_index(nb);
            smallbits = _smallmap >> idx;

            if ((smallbits & 0x3U) != 0) {
                // Remainderless fit to a smallbin.
                mchunkptr b, p;
                idx += ~smallbits & 1;       // Uses next bin if idx empty
                b = smallbin_at(idx);
                p = b->_fd;
                assert(p->chunksize() == small_index2size(idx));
                unlink_first_small_chunk(b, p, idx);
                set_inuse_and_pinuse(p, small_index2size(idx));
                mem = chunk2mem(p);
                check_malloced_chunk(mem, nb);
                goto postaction;
            }

            else if (nb > _dvsize) {
                if (smallbits != 0) {
                    // Use chunk in next nonempty smallbin
                    mchunkptr b, p, r;
                    size_t rsize;
                    binmap_t leftbits = (smallbits << idx) & left_bits(malloc_state::idx2bit(idx));
                    binmap_t leastbit = least_bit(leftbits);
                    bindex_t i = compute_bit2idx(leastbit);
                    b = smallbin_at(i);
                    p = b->_fd;
                    assert(p->chunksize() == small_index2size(i));
                    unlink_first_small_chunk(b, p, i);
                    rsize = small_index2size(i) - nb;
                    // Fit here cannot be remainderless if 4byte sizes
                    if (sizeof(size_t) != 4 && rsize < MIN_CHUNK_SIZE)
                        set_inuse_and_pinuse(p, small_index2size(i));
                    else {
                        set_size_and_pinuse_of_inuse_chunk(p, nb);
                        r = (mchunkptr)p->chunk_plus_offset(nb);
                        r->set_size_and_pinuse_of_free_chunk(rsize);
                        replace_dv(r, rsize);
                    }
                    mem = chunk2mem(p);
                    check_malloced_chunk(mem, nb);
                    goto postaction;
                }

                else if (_treemap != 0 && (mem = tmalloc_small(nb)) != 0) {
                    check_malloced_chunk(mem, nb);
                    goto postaction;
                }
            }
        }
        else if (bytes >= MAX_REQUEST)
            nb = MAX_SIZE_T; // Too big to allocate. Force failure (in sys alloc)
        else {
            nb = pad_request(bytes);
            if (_treemap != 0 && (mem = tmalloc_large(nb)) != 0) {
                check_malloced_chunk(mem, nb);
                goto postaction;
            }
        }

        if (nb <= _dvsize) {
            size_t rsize = _dvsize - nb;
            mchunkptr p = _dv;
            if (rsize >= MIN_CHUNK_SIZE) {
                // split dv
                mchunkptr r = _dv = (mchunkptr)p->chunk_plus_offset(nb);
                _dvsize = rsize;
                r->set_size_and_pinuse_of_free_chunk(rsize);
                set_size_and_pinuse_of_inuse_chunk(p, nb);
            }
            else { // exhaust dv
                size_t dvs = _dvsize;
                _dvsize = 0;
                _dv = 0;
                set_inuse_and_pinuse(p, dvs);
            }
            mem = chunk2mem(p);
            check_malloced_chunk(mem, nb);
            goto postaction;
        }

        else if (nb < _topsize) {
            // Split top
            size_t rsize = _topsize -= nb;
            mchunkptr p = _top;
            mchunkptr r = _top = (mchunkptr)p->chunk_plus_offset(nb);
            r->_head = rsize | PINUSE_BIT;
            set_size_and_pinuse_of_inuse_chunk(p, nb);
            mem = chunk2mem(p);
            check_top_chunk(_top);
            check_malloced_chunk(mem, nb);
            goto postaction;
        }

        mem = sys_alloc(nb);

    postaction:
        POSTACTION(this);
        return mem;
    }

    return 0;
}

/* ---------------------------- free --------------------------- */

void malloc_state::_free(mchunkptr p) {
    if (!PREACTION(this)) {
        check_inuse_chunk(p);
        if (rtcheck(ok_address(p) && ok_inuse(p))) {
            size_t psize = p->chunksize();
            mchunkptr next = (mchunkptr)p->chunk_plus_offset(psize);
            if (!p->pinuse()) {
                size_t prevsize = p->_prev_foot;
                if (p->is_mmapped()) {
                    psize += prevsize + MMAP_FOOT_PAD;
                    if (CALL_MUNMAP((char*)p - prevsize, psize) == 0)
                        _footprint -= psize;
                    goto postaction;
                }
                else {
                    mchunkptr prev = (mchunkptr)p->chunk_minus_offset(prevsize);
                    psize += prevsize;
                    p = prev;
                    if (rtcheck(ok_address(prev))) {
                        // consolidate backward
                        if (p != _dv) {
                            unlink_chunk(p, prevsize);
                        }
                        else if ((next->_head & INUSE_BITS) == INUSE_BITS) {
                            _dvsize = psize;
                            p->set_free_with_pinuse(psize, next);
                            goto postaction;
                        }
                    }
                    else
                        goto erroraction;
                }
            }

            if (rtcheck(ok_next(p, next) && ok_pinuse(next))) {
                if (!next->cinuse()) {
                    // consolidate forward
                    if (next == _top) {
                        size_t tsize = _topsize += psize;
                        _top = p;
                        p->_head = tsize | PINUSE_BIT;
                        if (p == _dv) {
                            _dv = 0;
                            _dvsize = 0;
                        }
                        if (should_trim(tsize))
                            sys_trim(0);
                        goto postaction;
                    }
                    else if (next == _dv) {
                        size_t dsize = _dvsize += psize;
                        _dv = p;
                        p->set_size_and_pinuse_of_free_chunk(dsize);
                        goto postaction;
                    }
                    else {
                        size_t nsize = next->chunksize();
                        psize += nsize;
                        unlink_chunk(next, nsize);
                        p->set_size_and_pinuse_of_free_chunk(psize);
                        if (p == _dv) {
                            _dvsize = psize;
                            goto postaction;
                        }
                    }
                }
                else
                    p->set_free_with_pinuse(psize, next);

                if (is_small(psize)) {
                    insert_small_chunk(p, psize);
                    check_free_chunk(p);
                }
                else {
                    tchunkptr tp = (tchunkptr)p;
                    insert_large_chunk(tp, psize);
                    check_free_chunk(p);
                    if (--_release_checks == 0)
                        release_unused_segments();
                }
                goto postaction;
            }
        }
    erroraction:
        USAGE_ERROR_ACTION(this, p);
    postaction:
        POSTACTION(this);
    }
}

/* ------------ Internal support for realloc, memalign, etc -------------- */

// Try to realloc; only in-place unless can_move true
mchunkptr malloc_state::try_realloc_chunk(mchunkptr p, size_t nb, int can_move) {
    mchunkptr newp = 0;
    size_t oldsize = p->chunksize();
    mchunkptr next = (mchunkptr)p->chunk_plus_offset(oldsize);
    if (rtcheck(ok_address(p) && ok_inuse(p) &&
                ok_next(p, next) && ok_pinuse(next))) {
        if (p->is_mmapped()) {
            newp = mmap_resize(p, nb, can_move);
        }
        else if (oldsize >= nb) {
            // already big enough
            size_t rsize = oldsize - nb;
            if (rsize >= MIN_CHUNK_SIZE) {
                // split off remainder
                mchunkptr r = (mchunkptr)p->chunk_plus_offset(nb);
                set_inuse(p, nb);
                set_inuse(r, rsize);
                dispose_chunk(r, rsize);
            }
            newp = p;
        }
        else if (next == _top) {
            // extend into top
            if (oldsize + _topsize > nb) {
                size_t newsize = oldsize + _topsize;
                size_t newtopsize = newsize - nb;
                mchunkptr newtop = (mchunkptr)p->chunk_plus_offset(nb);
                set_inuse(p, nb);
                newtop->_head = newtopsize | PINUSE_BIT;
                _top = newtop;
                _topsize = newtopsize;
                newp = p;
            }
        }
        else if (next == _dv) {
            // extend into dv
            size_t dvs = _dvsize;
            if (oldsize + dvs >= nb) {
                size_t dsize = oldsize + dvs - nb;
                if (dsize >= MIN_CHUNK_SIZE) {
                    mchunkptr r = (mchunkptr)p->chunk_plus_offset(nb);
                    mchunkptr n = (mchunkptr)r->chunk_plus_offset(dsize);
                    set_inuse(p, nb);
                    r->set_size_and_pinuse_of_free_chunk(dsize);
                    n->clear_pinuse();
                    _dvsize = dsize;
                    _dv = r;
                }
                else {
                    // exhaust dv
                    size_t newsize = oldsize + dvs;
                    set_inuse(p, newsize);
                    _dvsize = 0;
                    _dv = 0;
                }
                newp = p;
            }
        }
        else if (!next->cinuse()) {
            // extend into next free chunk
            size_t nextsize = next->chunksize();
            if (oldsize + nextsize >= nb) {
                size_t rsize = oldsize + nextsize - nb;
                unlink_chunk(next, nextsize);
                if (rsize < MIN_CHUNK_SIZE) {
                    size_t newsize = oldsize + nextsize;
                    set_inuse(p, newsize);
                }
                else {
                    mchunkptr r = (mchunkptr)p->chunk_plus_offset(nb);
                    set_inuse(p, nb);
                    set_inuse(r, rsize);
                    dispose_chunk(r, rsize);
                }
                newp = p;
            }
        }
    }
    else {
        USAGE_ERROR_ACTION(m, chunk2mem(p));
    }
    return newp;
}

void* malloc_state::internal_memalign(size_t alignment, size_t bytes) {
    void* mem = 0;
    if (alignment < MIN_CHUNK_SIZE) // must be at least a minimum chunk size
        alignment = MIN_CHUNK_SIZE;
    if ((alignment & (alignment - 1)) != 0) {
        // Ensure a power of 2
        size_t a = MALLOC_ALIGNMENT << 1;
        while (a < alignment) a <<= 1;
        alignment = a;
    }
    if (bytes >= MAX_REQUEST - alignment) {
        MALLOC_FAILURE_ACTION;
    }
    else {
        size_t nb = request2size(bytes);
        size_t req = nb + alignment + MIN_CHUNK_SIZE - CHUNK_OVERHEAD;
        mem = internal_malloc(req);
        if (mem != 0) {
            mchunkptr p = mem2chunk(mem);
            if (PREACTION(this))
                return 0;
            if ((((size_t)(mem)) & (alignment - 1)) != 0) {
                // misaligned
                /*
                  Find an aligned spot inside chunk.  Since we need to give
                  back leading space in a chunk of at least MIN_CHUNK_SIZE, if
                  the first calculation places us at a spot with less than
                  MIN_CHUNK_SIZE leader, we can move to the next aligned spot.
                  We've allocated enough total room so that this is always
                  possible.
                */
                char* br = (char*)mem2chunk((void *)(((size_t)((char*)mem + alignment - 1)) &
                                                     -alignment));
                char* pos = ((size_t)(br - (char*)(p)) >= MIN_CHUNK_SIZE)?
                    br : br+alignment;
                mchunkptr newp = (mchunkptr)pos;
                size_t leadsize = pos - (char*)(p);
                size_t newsize = p->chunksize() - leadsize;

                if (p->is_mmapped()) {
                    // For mmapped chunks, just adjust offset
                    newp->_prev_foot = p->_prev_foot + leadsize;
                    newp->_head = newsize;
                }
                else {
                    // Otherwise, give back leader, use the rest
                    set_inuse(newp, newsize);
                    set_inuse(p, leadsize);
                    dispose_chunk(p, leadsize);
                }
                p = newp;
            }

            // Give back spare room at the end
            if (!p->is_mmapped()) {
                size_t size = p->chunksize();
                if (size > nb + MIN_CHUNK_SIZE) {
                    size_t remainder_size = size - nb;
                    mchunkptr remainder = (mchunkptr)p->chunk_plus_offset(nb);
                    set_inuse(p, nb);
                    set_inuse(remainder, remainder_size);
                    dispose_chunk(remainder, remainder_size);
                }
            }

            mem = chunk2mem(p);
            assert (p->chunksize() >= nb);
            assert(((size_t)mem & (alignment - 1)) == 0);
            check_inuse_chunk(p);
            POSTACTION(this);
        }
    }
    return mem;
}

/*
  Common support for independent_X routines, handling
    all of the combinations that can result.
  The opts arg has:
    bit 0 set if all elements are same size (using sizes[0])
    bit 1 set if elements should be zeroed
*/
void** malloc_state::ialloc(size_t n_elements, size_t* sizes, int opts,
                            void* chunks[]) {

    size_t    element_size;   // chunksize of each element, if all same 
    size_t    contents_size;  // total size of elements 
    size_t    array_size;     // request size of pointer array 
    void*     mem;            // malloced aggregate space 
    mchunkptr p;              // corresponding chunk 
    size_t    remainder_size; // remaining bytes while splitting 
    void**    marray;         // either "chunks" or malloced ptr array 
    mchunkptr array_chunk;    // chunk for malloced ptr array 
    flag_t    was_enabled;    // to disable mmap 
    size_t    size;
    size_t    i;

    mparams.ensure_initialization();
    // compute array length, if needed 
    if (chunks != 0) {
        if (n_elements == 0)
            return chunks; // nothing to do 
        marray = chunks;
        array_size = 0;
    }
    else {
        // if empty req, must still return chunk representing empty array 
        if (n_elements == 0)
            return (void**)internal_malloc(0);
        marray = 0;
        array_size = request2size(n_elements * (sizeof(void*)));
    }

    // compute total element size 
    if (opts & 0x1) {
        // all-same-size 
        element_size = request2size(*sizes);
        contents_size = n_elements * element_size;
    }
    else {
        // add up all the sizes 
        element_size = 0;
        contents_size = 0;
        for (i = 0; i != n_elements; ++i)
            contents_size += request2size(sizes[i]);
    }

    size = contents_size + array_size;

    /*
      Allocate the aggregate chunk.  First disable direct-mmapping so
      malloc won't use it, since we would not be able to later
      free/realloc space internal to a segregated mmap region.
    */
    was_enabled = use_mmap();
    disable_mmap();
    mem = internal_malloc(size - CHUNK_OVERHEAD);
    if (was_enabled)
        enable_mmap();
    if (mem == 0)
        return 0;

    if (PREACTION(this)) return 0;
    p = mem2chunk(mem);
    remainder_size = p->chunksize();

    assert(!p->is_mmapped());

    if (opts & 0x2) {
        // optionally clear the elements 
        memset((size_t*)mem, 0, remainder_size - sizeof(size_t) - array_size);
    }

    // If not provided, allocate the pointer array as final part of chunk 
    if (marray == 0) {
        size_t  array_chunk_size;
        array_chunk = (mchunkptr)p->chunk_plus_offset(contents_size);
        array_chunk_size = remainder_size - contents_size;
        marray = (void**) (chunk2mem(array_chunk));
        set_size_and_pinuse_of_inuse_chunk(array_chunk, array_chunk_size);
        remainder_size = contents_size;
    }

    // split out elements 
    for (i = 0; ; ++i) {
        marray[i] = chunk2mem(p);
        if (i != n_elements - 1) {
            if (element_size != 0)
                size = element_size;
            else
                size = request2size(sizes[i]);
            remainder_size -= size;
            set_size_and_pinuse_of_inuse_chunk(p, size);
            p = (mchunkptr)p->chunk_plus_offset(size);
        }
        else { 
            // the final element absorbs any overallocation slop 
            set_size_and_pinuse_of_inuse_chunk(p, remainder_size);
            break;
        }
    }

#if DEBUG
    if (marray != chunks) {
        // final element must have exactly exhausted chunk 
        if (element_size != 0) {
            assert(remainder_size == element_size);
        }
        else {
            assert(remainder_size == request2size(sizes[i]));
        }
        check_inuse_chunk(mem2chunk(marray));
    }
    for (i = 0; i != n_elements; ++i)
        check_inuse_chunk(mem2chunk(marray[i]));

#endif

    POSTACTION(this);
    return marray;
}

/* Try to free all pointers in the given array.
   Note: this could be made faster, by delaying consolidation,
   at the price of disabling some user integrity checks, We
   still optimize some consolidations by combining adjacent
   chunks before freeing, which will occur often if allocated
   with ialloc or the array is sorted.
*/
size_t malloc_state::internal_bulk_free(void* array[], size_t nelem) {
    size_t unfreed = 0;
    if (!PREACTION(this)) {
        void** a;
        void** fence = &(array[nelem]);
        for (a = array; a != fence; ++a) {
            void* mem = *a;
            if (mem != 0) {
                mchunkptr p = mem2chunk(mem);
                size_t psize = p->chunksize();
#if FOOTERS
                if (get_mstate_for(p) != m) {
                    ++unfreed;
                    continue;
                }
#endif
                check_inuse_chunk(p);
                *a = 0;
                if (rtcheck(ok_address(p) && ok_inuse(p))) {
                    void ** b = a + 1; // try to merge with next chunk
                    mchunkptr next = (mchunkptr)p->next_chunk();
                    if (b != fence && *b == chunk2mem(next)) {
                        size_t newsize = next->chunksize() + psize;
                        set_inuse(p, newsize);
                        *b = chunk2mem(p);
                    }
                    else
                        dispose_chunk(p, psize);
                }
                else {
                    CORRUPTION_ERROR_ACTION(this);
                    break;
                }
            }
        }
        if (should_trim(_topsize))
            sys_trim(0);
        POSTACTION(this);
    }
    return unfreed;
}

void malloc_state::init(char* tbase, size_t tsize) {
    _seg._base = _least_addr = tbase;
    _seg._size = _footprint = _max_footprint = tsize;
    _magic    = mparams._magic;
    _release_checks = MAX_RELEASE_CHECK_RATE;
    _mflags   = mparams._default_mflags;
    _extp     = 0;
    _exts     = 0;
    disable_contiguous();
    init_bins();
    mchunkptr mn = (mchunkptr)mem2chunk(this)->next_chunk();
    init_top(mn, (size_t)((tbase + tsize) - (char*)mn) - TOP_FOOT_SIZE);
    check_top_chunk(_top);
}

/* Traversal */
#if MALLOC_INSPECT_ALL
void malloc_state::internal_inspect_all(void(*handler)(void *start, void *end,
                                                       size_t used_bytes,
                                                       void* callback_arg),
                                        void* arg) {
    if (is_initialized()) {
        mchunkptr top = top;
        msegmentptr s;
        for (s = &seg; s != 0; s = s->next) {
            mchunkptr q = align_as_chunk(s->base);
            while (segment_holds(s, q) && q->head != FENCEPOST_HEAD) {
                mchunkptr next = (mchunkptr)q->next_chunk();
                size_t sz = q->chunksize();
                size_t used;
                void* start;
                if (q->is_inuse()) {
                    used = sz - CHUNK_OVERHEAD; // must not be mmapped
                    start = chunk2mem(q);
                }
                else {
                    used = 0;
                    if (is_small(sz)) {
                        // offset by possible bookkeeping
                        start = (void*)((char*)q + sizeof(struct malloc_chunk));
                    }
                    else {
                        start = (void*)((char*)q + sizeof(struct malloc_tree_chunk));
                    }
                }
                if (start < (void*)next)  // skip if all space is bookkeeping
                    handler(start, next, used, arg);
                if (q == top)
                    break;
                q = next;
            }
        }
    }
}
#endif // MALLOC_INSPECT_ALL

/* ------------------ Exported realloc, memalign, etc -------------------- */

#if !ONLY_MSPACES

void* dlrealloc(void* oldmem, size_t bytes) DLTHROW {
    void* mem = 0;
    if (oldmem == 0) {
        mem = dlmalloc(bytes);
    }
    else if (bytes >= MAX_REQUEST) {
        MALLOC_FAILURE_ACTION;
    }
#ifdef REALLOC_ZERO_BYTES_FREES
    else if (bytes == 0) {
        dlfree(oldmem);
    }
#endif // REALLOC_ZERO_BYTES_FREES
    else {
        size_t nb = request2size(bytes);
        mchunkptr oldp = mem2chunk(oldmem);
#if ! FOOTERS
        mstate m = gm;
#else 
        mstate m = get_mstate_for(oldp);
        if (!m->ok_magic()) {
            USAGE_ERROR_ACTION(m, oldmem);
            return 0;
        }
#endif
        if (!PREACTION(m)) {
            mchunkptr newp = m->try_realloc_chunk(oldp, nb, 1);
            POSTACTION(m);
            if (newp != 0) {
                m->check_inuse_chunk(newp);
                mem = chunk2mem(newp);
            }
            else {
                mem = m->internal_malloc(bytes);
                if (mem != 0) {
                    size_t oc = oldp->chunksize() - oldp->overhead_for();
                    memcpy(mem, oldmem, (oc < bytes)? oc : bytes);
                    m->internal_free(oldmem);
                }
            }
        }
    }
    return mem;
}

void* dlrealloc_in_place(void* oldmem, size_t bytes) {
    void* mem = 0;
    if (oldmem != 0) {
        if (bytes >= MAX_REQUEST) {
            MALLOC_FAILURE_ACTION;
        }
        else {
            size_t nb = request2size(bytes);
            mchunkptr oldp = mem2chunk(oldmem);
#if ! FOOTERS
            mstate m = gm;
#else 
            mstate m = get_mstate_for(oldp);
            if (!m->ok_magic()) {
                USAGE_ERROR_ACTION(m, oldmem);
                return 0;
            }
#endif
            if (!PREACTION(m)) {
                mchunkptr newp = m->try_realloc_chunk(oldp, nb, 0);
                POSTACTION(m);
                if (newp == oldp) {
                    m->check_inuse_chunk(newp);
                    mem = oldmem;
                }
            }
        }
    }
    return mem;
}

void* dlmemalign(size_t alignment, size_t bytes) {
    if (alignment <= MALLOC_ALIGNMENT) {
        return dlmalloc(bytes);
    }
    return gm->internal_memalign(alignment, bytes);
}

int dlposix_memalign(void** pp, size_t alignment, size_t bytes) DLTHROW {
    void* mem = 0;
    if (alignment == MALLOC_ALIGNMENT)
        mem = dlmalloc(bytes);
    else {
        size_t d = alignment / sizeof(void*);
        size_t r = alignment % sizeof(void*);
        if (r != 0 || d == 0 || (d & (d - 1)) != 0)
            return EINVAL;
        else if (bytes <= MAX_REQUEST - alignment) {
            if (alignment <  MIN_CHUNK_SIZE)
                alignment = MIN_CHUNK_SIZE;
            mem = gm->internal_memalign(alignment, bytes);
        }
    }
    if (mem == 0)
        return ENOMEM;
    else {
        *pp = mem;
        return 0;
    }
}

void* dlvalloc(size_t bytes) DLTHROW {
    size_t pagesz;
    mparams.ensure_initialization();
    pagesz = mparams._page_size;
    return dlmemalign(pagesz, bytes);
}

void* dlpvalloc(size_t bytes) {
    size_t pagesz;
    mparams.ensure_initialization();
    pagesz = mparams._page_size;
    return dlmemalign(pagesz, (bytes + pagesz - 1) & ~(pagesz - 1));
}

void** dlindependent_calloc(size_t n_elements, size_t elem_size,
                            void* chunks[]) {
    size_t sz = elem_size; // serves as 1-element array
    return gm->ialloc(n_elements, &sz, 3, chunks);
}

void** dlindependent_comalloc(size_t n_elements, size_t sizes[],
                              void* chunks[]) {
    return gm->ialloc(n_elements, sizes, 0, chunks);
}

size_t dlbulk_free(void* array[], size_t nelem) {
    return gm->internal_bulk_free(array, nelem);
}

#if MALLOC_INSPECT_ALL
void dlmalloc_inspect_all(void(*handler)(void *start,
                                         void *end,
                                         size_t used_bytes,
                                         void* callback_arg),
                          void* arg) {
    mparams.ensure_initialization();
    if (!PREACTION(gm)) {
        internal_inspect_all(gm, handler, arg);
        POSTACTION(gm);
    }
}
#endif 

int dlmalloc_trim(size_t pad) {
    int result = 0;
    mparams.ensure_initialization();
    if (!PREACTION(gm)) {
        result = gm->sys_trim(pad);
        POSTACTION(gm);
    }
    return result;
}

size_t dlmalloc_footprint(void) {
    return gm->_footprint;
}

size_t dlmalloc_max_footprint(void) {
    return gm->_max_footprint;
}

size_t dlmalloc_footprint_limit(void) {
    size_t maf = gm->_footprint_limit;
    return maf == 0 ? MAX_SIZE_T : maf;
}

size_t dlmalloc_set_footprint_limit(size_t bytes) {
    size_t result;  // invert sense of 0
    if (bytes == 0)
        result = mparams.granularity_align(1); // Use minimal size
    else if (bytes == MAX_SIZE_T)
        result = 0;                    // disable
    else
        result = mparams.granularity_align(bytes);
    return gm->_footprint_limit = result;
}

#if !NO_MALLINFO
struct mallinfo dlmallinfo(void) {
    return gm->internal_mallinfo();
}
#endif

#if !NO_MALLOC_STATS
void dlmalloc_stats() {
    gm->internal_malloc_stats();
}
#endif 

int dlmallopt(int param_number, int value) {
    return mparams.change(param_number, value);
}

size_t dlmalloc_usable_size(void* mem) {
    if (mem != 0) {
        mchunkptr p = mem2chunk(mem);
        if (p->is_inuse())
            return p->chunksize() - p->overhead_for();
    }
    return 0;
}

#endif /* !ONLY_MSPACES */


/* ----------------------------- user mspaces ---------------------------- */

#if MSPACES

static mstate init_user_mstate(char* tbase, size_t tsize) {
    size_t msize = pad_request(sizeof(malloc_state));
    mchunkptr msp = align_as_chunk(tbase);
    mstate m = (mstate)(chunk2mem(msp));
    memset(m, 0, msize);
    (void)INITIAL_LOCK(&m->get_mutex());
    msp->_head = (msize | INUSE_BITS);
    m->init(tbase, tsize);
    return m;
}

mspace create_mspace(size_t capacity, int locked) {
    mstate m = 0;
    size_t msize;
    mparams.ensure_initialization();
    msize = pad_request(sizeof(malloc_state));
    if (capacity < (size_t) -(msize + TOP_FOOT_SIZE + mparams._page_size)) {
        size_t rs = ((capacity == 0)? mparams._granularity :
                     (capacity + TOP_FOOT_SIZE + msize));
        size_t tsize = mparams.granularity_align(rs);
        char* tbase = (char*)(CALL_MMAP(tsize));
        if (tbase != CMFAIL) {
            m = init_user_mstate(tbase, tsize);
            m->_seg._sflags = USE_MMAP_BIT;
            m->set_lock(locked);
        }
    }
    return (mspace)m;
}

mspace create_mspace_with_base(void* base, size_t capacity, int locked) {
    mstate m = 0;
    size_t msize;
    mparams.ensure_initialization();
    msize = pad_request(sizeof(malloc_state));
    if (capacity > msize + TOP_FOOT_SIZE &&
        capacity < (size_t) -(msize + TOP_FOOT_SIZE + mparams._page_size)) {
        m = init_user_mstate((char*)base, capacity);
        m->_seg._sflags = EXTERN_BIT;
        m->set_lock(locked);
    }
    return (mspace)m;
}

int mspace_track_large_chunks(mspace msp, int enable) {
    int ret = 0;
    mstate ms = (mstate)msp;
    if (!PREACTION(ms)) {
        if (!ms->use_mmap()) {
            ret = 1;
        }
        if (!enable) {
            ms->enable_mmap();
        } else {
            ms->disable_mmap();
        }
        POSTACTION(ms);
    }
    return ret;
}

size_t destroy_mspace(mspace msp) {
    size_t freed = 0;
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        msegmentptr sp = &ms->_seg;
        (void)DESTROY_LOCK(&ms->get_mutex()); // destroy before unmapped
        while (sp != 0) {
            char* base = sp->_base;
            size_t size = sp->_size;
            flag_t flag = sp->_sflags;
            (void)base; // placate people compiling -Wunused-variable
            sp = sp->_next;
            if ((flag & USE_MMAP_BIT) && !(flag & EXTERN_BIT) &&
                CALL_MUNMAP(base, size) == 0)
                freed += size;
        }
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return freed;
}

/* ----------------------------  mspace versions of malloc/calloc/free routines -------------------- */
void* mspace_malloc(mspace msp, size_t bytes) {
    mstate ms = (mstate)msp;
    if (!ms->ok_magic()) {
        USAGE_ERROR_ACTION(ms,ms);
        return 0;
    }
    return ms->_malloc(bytes);
}

void mspace_free(mspace msp, void* mem) {
    if (mem != 0) {
        mchunkptr p  = mem2chunk(mem);
#if FOOTERS
        mstate fm = get_mstate_for(p);
        (void)msp; // placate people compiling -Wunused
#else 
        mstate fm = (mstate)msp;
#endif
        if (!fm->ok_magic()) {
            USAGE_ERROR_ACTION(fm, p);
            return;
        }
        fm->_free(p);
    }
}

void* mspace_calloc(mspace msp, size_t n_elements, size_t elem_size) {
    void* mem;
    size_t req = 0;
    mstate ms = (mstate)msp;
    if (!ms->ok_magic()) {
        USAGE_ERROR_ACTION(ms,ms);
        return 0;
    }
    if (n_elements != 0) {
        req = n_elements * elem_size;
        if (((n_elements | elem_size) & ~(size_t)0xffff) &&
            (req / n_elements != elem_size))
            req = MAX_SIZE_T; // force downstream failure on overflow
    }
    mem = ms->internal_malloc(req);
    if (mem != 0 && mem2chunk(mem)->calloc_must_clear())
        memset(mem, 0, req);
    return mem;
}

void* mspace_realloc(mspace msp, void* oldmem, size_t bytes) {
    void* mem = 0;
    if (oldmem == 0) {
        mem = mspace_malloc(msp, bytes);
    }
    else if (bytes >= MAX_REQUEST) {
        MALLOC_FAILURE_ACTION;
    }
#ifdef REALLOC_ZERO_BYTES_FREES
    else if (bytes == 0) {
        mspace_free(msp, oldmem);
    }
#endif
    else {
        size_t nb = request2size(bytes);
        mchunkptr oldp = mem2chunk(oldmem);
#if ! FOOTERS
        mstate m = (mstate)msp;
#else 
        mstate m = get_mstate_for(oldp);
        if (!m->ok_magic()) {
            USAGE_ERROR_ACTION(m, oldmem);
            return 0;
        }
#endif
        if (!PREACTION(m)) {
            mchunkptr newp = m->try_realloc_chunk(oldp, nb, 1);
            POSTACTION(m);
            if (newp != 0) {
                m->check_inuse_chunk(newp);
                mem = chunk2mem(newp);
            }
            else {
                mem = mspace_malloc(m, bytes);
                if (mem != 0) {
                    size_t oc = oldp->chunksize() - oldp->overhead_for();
                    memcpy(mem, oldmem, (oc < bytes)? oc : bytes);
                    mspace_free(m, oldmem);
                }
            }
        }
    }
    return mem;
}

void* mspace_realloc_in_place(mspace msp, void* oldmem, size_t bytes) {
    void* mem = 0;
    if (oldmem != 0) {
        if (bytes >= MAX_REQUEST) {
            MALLOC_FAILURE_ACTION;
        }
        else {
            size_t nb = request2size(bytes);
            mchunkptr oldp = mem2chunk(oldmem);
#if ! FOOTERS
            mstate m = (mstate)msp;
#else
            mstate m = get_mstate_for(oldp);
            (void)msp; // placate people compiling -Wunused
            if (!m->ok_magic()) {
                USAGE_ERROR_ACTION(m, oldmem);
                return 0;
            }
#endif
            if (!PREACTION(m)) {
                mchunkptr newp = m->try_realloc_chunk(oldp, nb, 0);
                POSTACTION(m);
                if (newp == oldp) {
                    m->check_inuse_chunk(newp);
                    mem = oldmem;
                }
            }
        }
    }
    return mem;
}

void* mspace_memalign(mspace msp, size_t alignment, size_t bytes) {
    mstate ms = (mstate)msp;
    if (!ms->ok_magic()) {
        USAGE_ERROR_ACTION(ms,ms);
        return 0;
    }
    if (alignment <= MALLOC_ALIGNMENT)
        return mspace_malloc(msp, bytes);
    return ms->internal_memalign(alignment, bytes);
}

void** mspace_independent_calloc(mspace msp, size_t n_elements,
                                 size_t elem_size, void* chunks[]) {
    size_t sz = elem_size; // serves as 1-element array
    mstate ms = (mstate)msp;
    if (!ms->ok_magic()) {
        USAGE_ERROR_ACTION(ms,ms);
        return 0;
    }
    return ms->ialloc(n_elements, &sz, 3, chunks);
}

void** mspace_independent_comalloc(mspace msp, size_t n_elements,
                                   size_t sizes[], void* chunks[]) {
    mstate ms = (mstate)msp;
    if (!ms->ok_magic()) {
        USAGE_ERROR_ACTION(ms,ms);
        return 0;
    }
    return ms->ialloc(n_elements, sizes, 0, chunks);
}

size_t mspace_bulk_free(mspace msp, void* array[], size_t nelem) {
    return ((mstate)msp)->internal_bulk_free(array, nelem);
}

#if MALLOC_INSPECT_ALL
void mspace_inspect_all(mspace msp,
                        void(*handler)(void *start,
                                       void *end,
                                       size_t used_bytes,
                                       void* callback_arg),
                        void* arg) {
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        if (!PREACTION(ms)) {
            internal_inspect_all(ms, handler, arg);
            POSTACTION(ms);
        }
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
}
#endif // MALLOC_INSPECT_ALL

int mspace_trim(mspace msp, size_t pad) {
    int result = 0;
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        if (!PREACTION(ms)) {
            result = ms->sys_trim(pad);
            POSTACTION(ms);
        }
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return result;
}

#if !NO_MALLOC_STATS
void mspace_malloc_stats(mspace msp) {
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        ms->internal_malloc_stats();
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
}
#endif // NO_MALLOC_STATS

size_t mspace_footprint(mspace msp) {
    size_t result = 0;
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        result = ms->_footprint;
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return result;
}

size_t mspace_max_footprint(mspace msp) {
    size_t result = 0;
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        result = ms->_max_footprint;
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return result;
}

size_t mspace_footprint_limit(mspace msp) {
    size_t result = 0;
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        size_t maf = ms->_footprint_limit;
        result = (maf == 0) ? MAX_SIZE_T : maf;
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return result;
}

size_t mspace_set_footprint_limit(mspace msp, size_t bytes) {
    size_t result = 0;
    mstate ms = (mstate)msp;
    if (ms->ok_magic()) {
        if (bytes == 0)
            result = mparams.granularity_align(1); // Use minimal size
        if (bytes == MAX_SIZE_T)
            result = 0;                    // disable
        else
            result = mparams.granularity_align(bytes);
        ms->_footprint_limit = result;
    }
    else {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return result;
}

#if !NO_MALLINFO
struct mallinfo mspace_mallinfo(mspace msp) {
    mstate ms = (mstate)msp;
    if (!ms->ok_magic()) {
        USAGE_ERROR_ACTION(ms,ms);
    }
    return ms->internal_mallinfo();
}
#endif // NO_MALLINFO

size_t mspace_usable_size(const void* mem) {
    if (mem != 0) {
        mchunkptr p = mem2chunk(mem);
        if (p->is_inuse())
            return p->chunksize() - p->overhead_for();
    }
    return 0;
}

int mspace_mallopt(int param_number, int value) {
    return mparams.change(param_number, value);
}

#endif // MSPACES


/* -------------------- Alternative MORECORE functions ------------------- */

/*
  Guidelines for creating a custom version of MORECORE:

  * For best performance, MORECORE should allocate in multiples of pagesize.
  * MORECORE may allocate more memory than requested. (Or even less,
      but this will usually result in a malloc failure.)
  * MORECORE must not allocate memory when given argument zero, but
      instead return one past the end address of memory from previous
      nonzero call.
  * For best performance, consecutive calls to MORECORE with positive
      arguments should return increasing addresses, indicating that
      space has been contiguously extended.
  * Even though consecutive calls to MORECORE need not return contiguous
      addresses, it must be OK for malloc'ed chunks to span multiple
      regions in those cases where they do happen to be contiguous.
  * MORECORE need not handle negative arguments -- it may instead
      just return MFAIL when given negative arguments.
      Negative arguments are always multiples of pagesize. MORECORE
      must not misinterpret negative args as large positive unsigned
      args. You can suppress all such calls from even occurring by defining
      MORECORE_CANNOT_TRIM,

  As an example alternative MORECORE, here is a custom allocator
  kindly contributed for pre-OSX macOS.  It uses virtually but not
  necessarily physically contiguous non-paged memory (locked in,
  present and won't get swapped out).  You can use it by uncommenting
  this section, adding some #includes, and setting up the appropriate
  defines above:

      #define MORECORE osMoreCore

  There is also a shutdown routine that should somehow be called for
  cleanup upon program exit.

  #define MAX_POOL_ENTRIES 100
  #define MINIMUM_MORECORE_SIZE  (64 * 1024U)
  static int next_os_pool;
  void *our_os_pools[MAX_POOL_ENTRIES];

  void *osMoreCore(int size)
  {
    void *ptr = 0;
    static void *sbrk_top = 0;

    if (size > 0)
    {
      if (size < MINIMUM_MORECORE_SIZE)
         size = MINIMUM_MORECORE_SIZE;
      if (CurrentExecutionLevel() == kTaskLevel)
         ptr = PoolAllocateResident(size + RM_PAGE_SIZE, 0);
      if (ptr == 0)
      {
        return (void *) MFAIL;
      }
      // save ptrs so they can be freed during cleanup
      our_os_pools[next_os_pool] = ptr;
      next_os_pool++;
      ptr = (void *) ((((size_t) ptr) + RM_PAGE_MASK) & ~RM_PAGE_MASK);
      sbrk_top = (char *) ptr + size;
      return ptr;
    }
    else if (size < 0)
    {
      // we don't currently support shrink behavior
      return (void *) MFAIL;
    }
    else
    {
      return sbrk_top;
    }
  }

  // cleanup any allocated memory pools
  // called as last thing before shutting down driver

  void osCleanupMem(void)
  {
    void **ptr;

    for (ptr = our_os_pools; ptr < &our_os_pools[MAX_POOL_ENTRIES]; ptr++)
      if (*ptr)
      {
         PoolDeallocate(*ptr);
         *ptr = 0;
      }
  }

*/


/* -----------------------------------------------------------------------
History:
    v2.8.6 Wed Aug 29 06:57:58 2012  Doug Lea
      * fix bad comparison in dlposix_memalign
      * don't reuse adjusted asize in sys_alloc
      * add LOCK_AT_FORK -- thanks to Kirill Artamonov for the suggestion
      * reduce compiler warnings -- thanks to all who reported/suggested these

    v2.8.5 Sun May 22 10:26:02 2011  Doug Lea  (dl at gee)
      * Always perform unlink checks unless INSECURE
      * Add posix_memalign.
      * Improve realloc to expand in more cases; expose realloc_in_place.
        Thanks to Peter Buhr for the suggestion.
      * Add footprint_limit, inspect_all, bulk_free. Thanks
        to Barry Hayes and others for the suggestions.
      * Internal refactorings to avoid calls while holding locks
      * Use non-reentrant locks by default. Thanks to Roland McGrath
        for the suggestion.
      * Small fixes to mspace_destroy, reset_on_error.
      * Various configuration extensions/changes. Thanks
         to all who contributed these.

    V2.8.4a Thu Apr 28 14:39:43 2011 (dl at gee.cs.oswego.edu)
      * Update Creative Commons URL

    V2.8.4 Wed May 27 09:56:23 2009  Doug Lea  (dl at gee)
      * Use zeros instead of prev foot for is_mmapped
      * Add mspace_track_large_chunks; thanks to Jean Brouwers
      * Fix set_inuse in internal_realloc; thanks to Jean Brouwers
      * Fix insufficient sys_alloc padding when using 16byte alignment
      * Fix bad error check in mspace_footprint
      * Adaptations for ptmalloc; thanks to Wolfram Gloger.
      * Reentrant spin locks; thanks to Earl Chew and others
      * Win32 improvements; thanks to Niall Douglas and Earl Chew
      * Add NO_SEGMENT_TRAVERSAL and MAX_RELEASE_CHECK_RATE options
      * Extension hook in malloc_state
      * Various small adjustments to reduce warnings on some compilers
      * Various configuration extensions/changes for more platforms. Thanks
         to all who contributed these.

    V2.8.3 Thu Sep 22 11:16:32 2005  Doug Lea  (dl at gee)
      * Add max_footprint functions
      * Ensure all appropriate literals are size_t
      * Fix conditional compilation problem for some #define settings
      * Avoid concatenating segments with the one provided
        in create_mspace_with_base
      * Rename some variables to avoid compiler shadowing warnings
      * Use explicit lock initialization.
      * Better handling of sbrk interference.
      * Simplify and fix segment insertion, trimming and mspace_destroy
      * Reinstate REALLOC_ZERO_BYTES_FREES option from 2.7.x
      * Thanks especially to Dennis Flanagan for help on these.

    V2.8.2 Sun Jun 12 16:01:10 2005  Doug Lea  (dl at gee)
      * Fix memalign brace error.

    V2.8.1 Wed Jun  8 16:11:46 2005  Doug Lea  (dl at gee)
      * Fix improper #endif nesting in C++
      * Add explicit casts needed for C++

    V2.8.0 Mon May 30 14:09:02 2005  Doug Lea  (dl at gee)
      * Use trees for large bins
      * Support mspaces
      * Use segments to unify sbrk-based and mmap-based system allocation,
        removing need for emulation on most platforms without sbrk.
      * Default safety checks
      * Optional footer checks. Thanks to William Robertson for the idea.
      * Internal code refactoring
      * Incorporate suggestions and platform-specific changes.
        Thanks to Dennis Flanagan, Colin Plumb, Niall Douglas,
        Aaron Bachmann,  Emery Berger, and others.
      * Speed up non-fastbin processing enough to remove fastbins.
      * Remove useless cfree() to avoid conflicts with other apps.
      * Remove internal memcpy, memset. Compilers handle builtins better.
      * Remove some options that no one ever used and rename others.

    V2.7.2 Sat Aug 17 09:07:30 2002  Doug Lea  (dl at gee)
      * Fix malloc_state bitmap array misdeclaration

    V2.7.1 Thu Jul 25 10:58:03 2002  Doug Lea  (dl at gee)
      * Allow tuning of FIRST_SORTED_BIN_SIZE
      * Use PTR_UINT as type for all ptr->int casts. Thanks to John Belmonte.
      * Better detection and support for non-contiguousness of MORECORE.
        Thanks to Andreas Mueller, Conal Walsh, and Wolfram Gloger
      * Bypass most of malloc if no frees. Thanks To Emery Berger.
      * Fix freeing of old top non-contiguous chunk im sysmalloc.
      * Raised default trim and map thresholds to 256K.
      * Fix mmap-related #defines. Thanks to Lubos Lunak.
      * Fix copy macros; added LACKS_FCNTL_H. Thanks to Neal Walfield.
      * Branch-free bin calculation
      * Default trim and mmap thresholds now 256K.

    V2.7.0 Sun Mar 11 14:14:06 2001  Doug Lea  (dl at gee)
      * Introduce independent_comalloc and independent_calloc.
        Thanks to Michael Pachos for motivation and help.
      * Make optional .h file available
      * Allow > 2GB requests on 32bit systems.
      * new WIN32 sbrk, mmap, munmap, lock code from <Walter@GeNeSys-e.de>.
        Thanks also to Andreas Mueller <a.mueller at paradatec.de>,
        and Anonymous.
      * Allow override of MALLOC_ALIGNMENT (Thanks to Ruud Waij for
        helping test this.)
      * memalign: check alignment arg
      * realloc: don't try to shift chunks backwards, since this
        leads to  more fragmentation in some programs and doesn't
        seem to help in any others.
      * Collect all cases in malloc requiring system memory into sysmalloc
      * Use mmap as backup to sbrk
      * Place all internal state in malloc_state
      * Introduce fastbins (although similar to 2.5.1)
      * Many minor tunings and cosmetic improvements
      * Introduce USE_PUBLIC_MALLOC_WRAPPERS, USE_MALLOC_LOCK
      * Introduce MALLOC_FAILURE_ACTION, MORECORE_CONTIGUOUS
        Thanks to Tony E. Bennett <tbennett@nvidia.com> and others.
      * Include errno.h to support default failure action.

    V2.6.6 Sun Dec  5 07:42:19 1999  Doug Lea  (dl at gee)
      * return null for negative arguments
      * Added Several WIN32 cleanups from Martin C. Fong <mcfong at yahoo.com>
         * Add 'LACKS_SYS_PARAM_H' for those systems without 'sys/param.h'
          (e.g. WIN32 platforms)
         * Cleanup header file inclusion for WIN32 platforms
         * Cleanup code to avoid Microsoft Visual C++ compiler complaints
         * Add 'USE_DL_PREFIX' to quickly allow co-existence with existing
           memory allocation routines
         * Set 'malloc_getpagesize' for WIN32 platforms (needs more work)
         * Use 'assert' rather than 'ASSERT' in WIN32 code to conform to
           usage of 'assert' in non-WIN32 code
         * Improve WIN32 'sbrk()' emulation's 'findRegion()' routine to
           avoid infinite loop
      * Always call 'fREe()' rather than 'free()'

    V2.6.5 Wed Jun 17 15:57:31 1998  Doug Lea  (dl at gee)
      * Fixed ordering problem with boundary-stamping

    V2.6.3 Sun May 19 08:17:58 1996  Doug Lea  (dl at gee)
      * Added pvalloc, as recommended by H.J. Liu
      * Added 64bit pointer support mainly from Wolfram Gloger
      * Added anonymously donated WIN32 sbrk emulation
      * Malloc, calloc, getpagesize: add optimizations from Raymond Nijssen
      * malloc_extend_top: fix mask error that caused wastage after
        foreign sbrks
      * Add linux mremap support code from HJ Liu

    V2.6.2 Tue Dec  5 06:52:55 1995  Doug Lea  (dl at gee)
      * Integrated most documentation with the code.
      * Add support for mmap, with help from
        Wolfram Gloger (Gloger@lrz.uni-muenchen.de).
      * Use last_remainder in more cases.
      * Pack bins using idea from  colin@nyx10.cs.du.edu
      * Use ordered bins instead of best-fit threshhold
      * Eliminate block-local decls to simplify tracing and debugging.
      * Support another case of realloc via move into top
      * Fix error occuring when initial sbrk_base not word-aligned.
      * Rely on page size for units instead of SBRK_UNIT to
        avoid surprises about sbrk alignment conventions.
      * Add mallinfo, mallopt. Thanks to Raymond Nijssen
        (raymond@es.ele.tue.nl) for the suggestion.
      * Add `pad' argument to malloc_trim and top_pad mallopt parameter.
      * More precautions for cases where other routines call sbrk,
        courtesy of Wolfram Gloger (Gloger@lrz.uni-muenchen.de).
      * Added macros etc., allowing use in linux libc from
        H.J. Lu (hjl@gnu.ai.mit.edu)
      * Inverted this history list

    V2.6.1 Sat Dec  2 14:10:57 1995  Doug Lea  (dl at gee)
      * Re-tuned and fixed to behave more nicely with V2.6.0 changes.
      * Removed all preallocation code since under current scheme
        the work required to undo bad preallocations exceeds
        the work saved in good cases for most test programs.
      * No longer use return list or unconsolidated bins since
        no scheme using them consistently outperforms those that don't
        given above changes.
      * Use best fit for very large chunks to prevent some worst-cases.
      * Added some support for debugging

    V2.6.0 Sat Nov  4 07:05:23 1995  Doug Lea  (dl at gee)
      * Removed footers when chunks are in use. Thanks to
        Paul Wilson (wilson@cs.texas.edu) for the suggestion.

    V2.5.4 Wed Nov  1 07:54:51 1995  Doug Lea  (dl at gee)
      * Added malloc_trim, with help from Wolfram Gloger
        (wmglo@Dent.MED.Uni-Muenchen.DE).

    V2.5.3 Tue Apr 26 10:16:01 1994  Doug Lea  (dl at g)

    V2.5.2 Tue Apr  5 16:20:40 1994  Doug Lea  (dl at g)
      * realloc: try to expand in both directions
      * malloc: swap order of clean-bin strategy;
      * realloc: only conditionally expand backwards
      * Try not to scavenge used bins
      * Use bin counts as a guide to preallocation
      * Occasionally bin return list chunks in first scan
      * Add a few optimizations from colin@nyx10.cs.du.edu

    V2.5.1 Sat Aug 14 15:40:43 1993  Doug Lea  (dl at g)
      * faster bin computation & slightly different binning
      * merged all consolidations to one part of malloc proper
         (eliminating old malloc_find_space & malloc_clean_bin)
      * Scan 2 returns chunks (not just 1)
      * Propagate failure in realloc if malloc returns 0
      * Add stuff to allow compilation on non-ANSI compilers
          from kpv@research.att.com

    V2.5 Sat Aug  7 07:41:59 1993  Doug Lea  (dl at g.oswego.edu)
      * removed potential for odd address access in prev_chunk
      * removed dependency on getpagesize.h
      * misc cosmetics and a bit more internal documentation
      * anticosmetics: mangled names in macros to evade debugger strangeness
      * tested on sparc, hp-700, dec-mips, rs6000
          with gcc & native cc (hp, dec only) allowing
          Detlefs & Zorn comparison study (in SIGPLAN Notices.)

    Trial version Fri Aug 28 13:14:29 1992  Doug Lea  (dl at g.oswego.edu)
      * Based loosely on libg++-1.2X malloc. (It retains some of the overall
         structure of old version,  but most details differ.)

*/
