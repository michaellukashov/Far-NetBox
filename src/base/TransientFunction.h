// TransientFunction: A light-weight alternative to std::function [C++11]
// Pass any callback - including capturing lambdas - cheaply and quickly as a 
// function argument
// 
// Based on: 
// https://deplinenoise.wordpress.com/2014/02/23/using-c11-capturing-lambdas-w-vanilla-c-api-functions/
//
//  - No instantiation of called function at each call site
//  - Simple to use - use TransientFunction<> as the function argument
//  - Low cost: cheap setup, one indirect function call to invoke
//  - No risk of dynamic allocation (unlike std::function)
//
//  - Not C-friendly, unlike the original Reverse - but easy to adapt
//  - NOT PERSISTENT: Use for synchronous calls only
//  - No thought has been given to argument forwarding in this implementation 
//
// Compiles as C++11 using recent g++, clang, and msvc compilers
// Try it at https://godbolt.org
//
// Prior art:
// A proposal to the C++ ISO committee has been written by Vittorio Romeo,
// which lists a number of other instances of this same pattern:
// function_ref: a non-owning reference to a Callable
// https://vittorioromeo.info/Misc/p0792r1.html
// https://www.youtube.com/watch?v=6EQRoqELeWc
// https://vittorioromeo.info/index/blog/passing_functions_to_functions.html
//

#pragma once

// For std::decay
#include <type_traits>

template<typename>
struct TransientFunction; // intentionally not defined

template<typename R, typename ...Args>
struct TransientFunction<R(Args...)>
{
  using Dispatcher = R(*)(void*, Args...);
  
  Dispatcher m_Dispatcher; // A pointer to the static function that will call the 
                           // wrapped invokable object
  void* m_Target{nullptr}; // A pointer to the invokable object

  // Dispatch() is instantiated by the TransientFunction constructor,
  // which will store a pointer to the function in m_Dispatcher.
  template<typename S>
  static R Dispatch(void* target, Args... args)
  {
    return (*(S*)target)(args...);
  }
  
  template<typename T>
  TransientFunction(T&& target)
    : m_Dispatcher(&Dispatch<typename std::decay<T>::type>)
    , m_Target(&target)
  {
  }

  // Specialize for reference-to-function, to ensure that a valid pointer is 
  // stored.
  using TargetFunctionRef = R(Args...);
  TransientFunction(TargetFunctionRef target)
    : m_Dispatcher(Dispatch<TargetFunctionRef>)
  {
    static_assert(sizeof(void*) == sizeof target, 
    "It will not be possible to pass functions by reference on this platform. "
    "Please use explicit function pointers i.e. foo(target) -> foo(&target)");
    m_Target = (void*)target;
  }

  R operator()(Args... args) const
  {
    return m_Dispatcher(m_Target, args...);
  }
};


#if defined(TRANSIENTFUNCTION_DEMO)

// Example usage:
void vv(TransientFunction<void()>);
void vi(TransientFunction<void(int)>);
int iiii(TransientFunction<int(int, int, int)>);

static void g() {}

int demonstration()
{
  // Non-capturing lambda, void()
  vv([]{});

  // Capturing lambda, void()
  int x = 3;
  vv([&]{x++;}); // x == 4

  // Free function, void()
  // both pointer to function and reference to function are valid
  vv(&g);
  vv(g);

  // std::function, void(); commented out because it wrecks codegen
  //   std::function<void()> f = [&]{ x += 21; };
  //   vv(f);

  vi([&](int i){x += i;}); // x == 11

  // x == 11 + 3 + 4 * 5 == 34
  return iiii([&](int a, int b, int c) { return x + a + b * c; });
}

/*
By defining vv(), vi() and iiii() as follows, clang and gcc are able to inline
and simplify all code in demonstration(). Mark those functions static for even
less generated code.

demonstration():
        mov     eax, 34
        ret
*/

void vv(TransientFunction<void()> f)
{
  f();
}

void vi(TransientFunction<void(int)> f)
{
  f(7);
}

int iiii(TransientFunction<int(int, int, int)> f)
{
  return f(3, 4, 5);
}

#endif


#if 0
#include <stdint.h>
#include <stdlib.h>
class MemAllocStack;

// For a more "real-world" perspective, we can look at the examples from
// https://deplinenoise.wordpress.com/2014/02/23/using-c11-capturing-lambdas-w-vanilla-c-api-functions/


// QuickSort without TransientFunction:

typedef int64_t          (*QuickCompareFunc)(const void*, const void*);
typedef int64_t (*QuickCompareFuncReentrant)(const void*, const void*, void* arg);

void QuickSort (void* __restrict elems, int64_t elem_count, size_t elem_size, 
                QuickCompareFunc cf);
void QuickSort (void* __restrict elems, int64_t elem_count, size_t elem_size, 
                QuickCompareFuncReentrant cf, void* arg);

template <typename T, typename Func>
void QuickSort(T* __restrict elems, int64_t elem_count, Func f)
{
  auto compare_closure = [](const void* ll, const void* rr,
                            void* context) -> int64_t {
    const T* l = (const T*) ll;
    const T* r = (const T*) rr;
    return (*(Func*)context)(l, r);
  };
  QuickSort(elems, elem_count, sizeof elems[0], compare_closure, &f);
}

// QuickSort with TransientFunction

// Use TransientFunction in comparison function typedef
using QuickCompareTFunc = TransientFunction<int64_t (const void*, const void*)>;

// Only one external function required - no overload required for reentrant 
// comparison function: pass any closure
void QuickSortTF(void* __restrict elems, int64_t elem_count, size_t elem_size, 
                 QuickCompareTFunc cf);

template <typename T, typename Func>
void QuickSortTF(T* __restrict elems, int64_t elem_count, Func f)
{
  // capture function rather than pass as arg
  auto compare_closure = [&f](const void* ll, const void* rr) -> int64_t { 
    const T* l = (const T*) ll;
    const T* r = (const T*) rr;
    return f(l, r); // simpler dispatch
  };

  // Pass the closure directly
  QuickSortTF(elems, elem_count, sizeof elems[0], compare_closure); 
}


// DecompressFile
// Without TransientFunction:
typedef bool (ZReadFn)(void* target_buffer, size_t read_size, void* user_data);

bool DecompressFile(void* buffer, size_t compressed_size, size_t decompressed_size, 
                    MemAllocStack* scratch, void *user_data, ZReadFn* read_fn);

template <typename Fn>
bool DecompressFile(void* buffer, size_t cs, size_t ds, 
                    MemAllocStack* scratch, Fn fn)
{
 auto closure = [](void* target_buffer, size_t read_size, void* user_data) -> bool
 {
   return (*(Fn*)user_data)(target_buffer, read_size);
 };

 return DecompressFile(buffer, cs, ds, scratch, &fn, closure);
}


// Using TransientFunction:

using ZReafTFn = TransientFunction<bool (void* target_buffer, size_t read_size)>; 
bool DecompressFile(void* buffer, size_t compressed_size, size_t decompressed_size,
                    MemAllocStack* scratch, ZReadFn read_fn);

// That's it. No forwarding template required.

#endif



/*
Copyright (c) 2018 Jonathan Adamczewski
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
*/
