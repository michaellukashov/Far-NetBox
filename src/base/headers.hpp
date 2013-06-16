#pragma once

/*
headers.hpp

Стандартные заголовки
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
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

#include <nbglobals.h>
#include "stdafx.h"

#include <new>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cwchar>

#undef _W32API_OLD

#ifdef _MSC_VER
# include <sdkddkver.h>
# if _WIN32_WINNT < 0x0501
#  error Windows SDK v7.0 (or higher) required
# endif
#endif //_MSC_VER

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define SECURITY_WIN32
#include <windows.h>
#include <winsock2.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif  //_WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif  //_WIN32_IE

#include <FastDelegate.h>
#include <FastDelegateBind.h>
//---------------------------------------------------------------------------

#if defined(__cplusplus)

inline void * operator_new(size_t size)
{
  void * p = nb_malloc(size);
  /*if (!p)
  {
    static std::bad_alloc badalloc;
    throw badalloc;
  }*/
  return p;
}

inline void operator_delete(void * p)
{
  nb_free(p);
}

#endif // if defined(__cplusplus)

#ifdef USE_DLMALLOC
/// custom memory allocation
#define DEF_CUSTOM_MEM_ALLOCATION_IMPL            \
  public:                                         \
   void * operator new(size_t size)                \
  {                                               \
    return operator_new(size);                    \
  }                                               \
  void operator delete(void * p, size_t)          \
  {                                               \
    operator_delete(p);                           \
  }                                               \
   void * operator new[](size_t size)             \
  {                                               \
    return operator_new(size);                    \
  }                                               \
  void operator delete[](void * p, size_t)        \
  {                                               \
    operator_delete(p);                           \
  }                                               \
   void * operator new(size_t, void * p)          \
  {                                               \
    return p;                                     \
  }                                               \
  void operator delete(void *, void *)            \
  {                                               \
  }                                               \
   void * operator new[](size_t, void * p)        \
  {                                               \
    return p;                                     \
  }                                               \
  void operator delete[](void *, void *)          \
  {                                               \
  }

#ifdef _DEBUG
#define CUSTOM_MEM_ALLOCATION_IMPL DEF_CUSTOM_MEM_ALLOCATION_IMPL \
   void * operator new(size_t size, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    return operator_new(size); \
  } \
   void * operator new[](size_t size, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    return operator_new(size); \
  } \
  void operator delete(void* p, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    operator_delete(p); \
  } \
  void operator delete[](void* p, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    operator_delete(p); \
  }
#else
#define CUSTOM_MEM_ALLOCATION_IMPL DEF_CUSTOM_MEM_ALLOCATION_IMPL
#endif // ifdef _DEBUG

#else
#define CUSTOM_MEM_ALLOCATION_IMPL 
#endif // ifdef USE_DLMALLOC

//---------------------------------------------------------------------------

namespace nballoc {
  inline void destruct(char *) {}
  inline void destruct(wchar_t*) {}
  template <typename T> 
  inline void destruct(T * t){ t->~T(); }
} // namespace nballoc

template <typename T> struct custom_nballocator_t;

template <> struct custom_nballocator_t<void>
{
public:
    typedef void* pointer;
    typedef const void* const_pointer;
    // reference to void members are impossible.
    typedef void value_type;
    template <class U> 
        struct rebind { typedef custom_nballocator_t<U> other; };
};

template <typename T> 
struct custom_nballocator_t
{
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T value_type;

  template <class U> struct rebind { typedef custom_nballocator_t<U> other; };
  inline custom_nballocator_t() throw() {}
  inline custom_nballocator_t(const custom_nballocator_t&) throw() {}

  template <class U> custom_nballocator_t(const custom_nballocator_t<U>&) throw(){}

  ~custom_nballocator_t() throw() {}

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }

  pointer allocate(size_type s, void const * = 0)
  {
    if (0 == s)
      return nullptr;
    pointer temp = (pointer)nb_malloc(s * sizeof(T)); 
    if (temp == nullptr)
      throw std::bad_alloc();
    return temp;
  }

  void deallocate(pointer p, size_type)
  {
    nb_free(p);
  }

  size_type max_size() const throw()
  {
    // return std::numeric_limits<size_t>::max() / sizeof(T); 
    return size_t(-1) / sizeof(T); 
  }

  void construct(pointer p, const T & val)
  {
    new((void *)p) T(val);
  }

  void destroy(pointer p)
  {
    nballoc::destruct(p);
  }
};

template <typename T, typename U>
inline bool operator==(const custom_nballocator_t<T> &, const custom_nballocator_t<U> &)
{
  return false;
}

template <typename T, typename U>
inline bool operator!=(const custom_nballocator_t<T> &, const custom_nballocator_t<U> &)
{
  return true;
}

//---------------------------------------------------------------------------

// winnls.h
#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

#ifndef True
#define True true
#endif
#ifndef False
#define False false
#endif
#ifndef Integer
typedef intptr_t Integer;
#endif
#ifndef Int64
typedef __int64 Int64;
#endif
#ifndef Boolean
typedef bool Boolean;
#endif
#ifndef Word
typedef WORD Word;
#endif

#ifndef HIDESBASE
#define HIDESBASE 
#endif

#define NullToEmpty(s) (s?s:L"")

template <class T>
inline const T&Min(const T &a, const T &b) { return a<b?a:b; }

template <class T>
inline const T&Max(const T &a, const T &b) { return a>b?a:b; }

template <class T>
inline const T Round(const T &a, const T &b) { return a/b+(a%b*2>b?1:0); }

inline void* ToPtr(intptr_t T){ return reinterpret_cast<void*>(T); }

template<typename T>
inline void ClearStruct(T& s) { memset(&s, 0, sizeof(s)); }

template<typename T>
inline void ClearStruct(T* s) { T dont_instantiate_this_template_with_pointers = s; }

template<typename T, size_t N>
inline void ClearArray(T (&a)[N]) { memset(a, 0, sizeof(a[0])*N); }

#ifdef __GNUC__
#ifndef nullptr
#define nullptr NULL
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER<1600
#define nullptr NULL
#endif

template <typename T>
bool CheckNullOrStructSize(const T* s) {return !s || (s->StructSize >= sizeof(T));}
template <typename T>
bool CheckStructSize(const T* s) {return s && (s->StructSize >= sizeof(T));}

#ifdef _DEBUG
#define SELF_TEST(code) \
  namespace { \
    struct SelfTest { \
      SelfTest() { \
        code; \
      } \
    } _SelfTest; \
  }
#else
#define SELF_TEST(code)
#endif

//---------------------------------------------------------------------------

#define NB_DISABLE_COPY(Class) \
    Class(const Class &); \
    Class &operator=(const Class &);

#define NB_STATIC_ASSERT(Condition, Message) \
    static_assert(bool(Condition), Message)

//---------------------------------------------------------------------------

#include "UnicodeString.hpp"
#include "local.hpp"
