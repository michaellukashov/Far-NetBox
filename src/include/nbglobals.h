#pragma once

#if defined(__cplusplus)
#include <cstdlib>
#include <cstdint>
#include <exception>
#include <gsl/gsl>
#endif

#ifdef USE_DLMALLOC

#include <dlmalloc/dlmalloc-2.8.6.h>

#if defined(__cplusplus)

#define nb_malloc(size) ::dlcalloc(1, size)
#define nb_calloc(count, size) ::dlcalloc(count, size)
#define nb_realloc(ptr, size) ::dlrealloc(ptr, size)

template<typename T>
void nb_free(const T * ptr) { ::dlfree(reinterpret_cast<void *>(const_cast<T *>(ptr))); }

#else //!defined(__cplusplus)

#define nb_malloc(size) dlcalloc(1, size)
#define nb_calloc(count, size) dlcalloc(count, size)
#define nb_realloc(ptr, size) dlrealloc(ptr, size)

#define nb_free(ptr) dlfree((void *)(ptr))

#endif //defined(__cplusplus)

#else //!USE_DLMALLOC

#if defined(__cplusplus)

#define nb_malloc(size) ::malloc(size)
#define nb_calloc(count, size) ::calloc(count, size)
#define nb_realloc(ptr, size) ::realloc(ptr, size)

template<typename T>
void nb_free(const T * ptr) { ::free(reinterpret_cast<void *>(const_cast<T *>(ptr))); }

#else //!defined(__cplusplus)

#define nb_malloc(size) malloc(size)
#define nb_calloc(count, size) calloc(count, size)
#define nb_realloc(ptr, size) realloc(ptr, size)

#define nb_free(ptr) free((void *)(ptr))

#endif //defined(__cplusplus)

#endif //!USE_DLMALLOC

#if defined(_MSC_VER)
#if (_MSC_VER < 1900)

#ifndef noexcept
#define noexcept throw()
#endif

#endif
#endif

#ifndef STRICT
#define STRICT 1
#endif

#if defined(__cplusplus)

namespace nb {

template<typename T>
inline T calloc(size_t count, size_t size) { return static_cast<T>(nb_calloc(count, size)); }

template<typename T>
inline T realloc(T ptr, size_t size) { return static_cast<T>(nb_realloc(ptr, size)); }

inline char * chcalloc(size_t size) { return calloc<char *>(1, size); }
inline wchar_t * wchcalloc(size_t size) { return calloc<wchar_t *>(1, size * sizeof(wchar_t)); }

inline void * operator_new(size_t size)
{
  void * p = nb::calloc<void *>(1, size);
#ifndef NDEBUG
  if (!p)
  {
    static std::bad_alloc badalloc;
    throw badalloc;
  }
#endif // ifndef NDEBUG
  return p;
}

template<typename T>
inline T * operator_new(size_t size) { return static_cast<T *>(operator_new(size)); }

inline void operator_delete(void * p)
{
  nb_free(p);
}

} // namespace nb

#endif //defined(__cplusplus)

#ifdef USE_DLMALLOC
/// custom memory allocation
#define DEF_CUSTOM_MEM_ALLOCATION_IMPL     \
  public:                                  \
  void * operator new(size_t sz)           \
  {                                        \
    return nb::operator_new(sz);           \
  }                                        \
  void operator delete(void * p)           \
  {                                        \
    nb::operator_delete(p);                \
  }                                        \
  void operator delete(void * p, size_t)   \
  {                                        \
    nb::operator_delete(p);                \
  }                                        \
  void * operator new[](size_t sz)         \
  {                                        \
    return nb::operator_new(sz);           \
  }                                        \
  void operator delete[](void * p)         \
  {                                        \
    nb::operator_delete(p);                \
  }                                        \
  void operator delete[](void * p, size_t) \
  {                                        \
    nb::operator_delete(p);                \
  }                                        \
  void * operator new(size_t, void * p)    \
  {                                        \
    return p;                              \
  }                                        \
  void operator delete(void *, void *)     \
  {                                        \
  }                                        \
  void * operator new[](size_t, void * p)  \
  {                                        \
    return p;                              \
  }                                        \
  void operator delete[](void *, void *)   \
  {                                        \
  }

#ifdef _DEBUG
#define CUSTOM_MEM_ALLOCATION_IMPL DEF_CUSTOM_MEM_ALLOCATION_IMPL \
  void * operator new(size_t sz, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    return nb::operator_new(sz); \
  } \
  void * operator new[](size_t sz, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    return nb::operator_new(sz); \
  } \
  void operator delete(void * p, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    nb::operator_delete(p); \
  } \
  void operator delete[](void * p, const char * /*lpszFileName*/, int /*nLine*/) \
  { \
    nb::operator_delete(p); \
  }
#else //!_DEBUG
#define CUSTOM_MEM_ALLOCATION_IMPL DEF_CUSTOM_MEM_ALLOCATION_IMPL
#endif //_DEBUG

#else //!USE_DLMALLOC
#define CUSTOM_MEM_ALLOCATION_IMPL
#endif //USE_DLMALLOC

#if defined(__cplusplus)

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter

#define NB_STATIC_ASSERT(Condition, Message) \
  static_assert(bool(Condition), Message)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif  //_WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif  //_WIN32_IE

// winnls.h
#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL 0x10000000
#endif

#ifndef HIDESBASE
#define HIDESBASE
#endif

#ifdef __GNUC__
#ifndef nullptr
#define nullptr NULL
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER<1600
#define nullptr NULL
#endif

#ifndef NB_CONCATENATE
#define NB_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define NB_CONCATENATE(s1, s2) NB_CONCATENATE_IMPL(s1, s2)
#endif

#undef __property
#ifdef _MSC_VER
#define __property NB_CONCATENATE(/, /)
#else
#define __property //
#endif

namespace nb {

constexpr const int32_t NB_MAX_PATH = (32 * 1024);
constexpr const int32_t NPOS = static_cast<int32_t>(-1);

namespace nballoc {

inline void destruct(char *)
{
}

inline void destruct(wchar_t *)
{
}

template<typename T>
inline void destruct(T * t) { t->~T(); }

} // namespace nballoc

#pragma warning(pop)

template<typename T>
struct custom_nballocator_t;

template<>
struct custom_nballocator_t<void>
{
public:
  using pointer = void *;
  using const_pointer = const void *;
  // reference to void members are impossible.
  using value_type = void;

  template<class U>
  struct rebind
  {
    using other = custom_nballocator_t<U>;
  };
};

template<typename T>
struct custom_nballocator_t
{
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using value_type = T;

  template<class U>
  struct rebind
  {
    using other = custom_nballocator_t<U>;
  };

  custom_nballocator_t() = default;
  custom_nballocator_t(const custom_nballocator_t &) = default;
  custom_nballocator_t & operator =(const custom_nballocator_t &) = default;
  custom_nballocator_t(custom_nballocator_t &&) noexcept = default;
  custom_nballocator_t & operator =(custom_nballocator_t &&) noexcept = default;

  /// Copy constructor
  template<class U>
  custom_nballocator_t(const custom_nballocator_t<U> &) noexcept {}

  ~custom_nballocator_t() noexcept = default;

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }

  size_type max_size() const noexcept
  {
    // The following has been carefully written to be independent of
    // the definition of size_t and to avoid signed/unsigned warnings.
    return (static_cast<size_t>(0) - static_cast<size_t>(1)) / sizeof(value_type);
  }

  pointer allocate(size_type n, custom_nballocator_t<void>::const_pointer hint = nullptr)
  {
    return static_cast<pointer>(operator_new(n * sizeof(T)));
  }

  /*pointer allocate(size_type s, void const * = nullptr)
  {
    if (0 == s)
      return nullptr;
    const auto temp = nb::calloc<pointer>(s, sizeof(T));
#ifndef NDEBUG
    if (temp == nullptr)
      throw std::bad_alloc();
#endif // ifndef NDEBUG
    return temp;
  }*/

  static void deallocate(pointer p, size_type)
  {
    nb_free(p);
  }

  void construct(pointer p, const T & val)
  {
    new(static_cast<void *>(p)) T(val);
  }

  void construct(pointer p)
  {
    new(static_cast<void*>(p)) T();
  }

  void destroy(pointer p)
  {
    nballoc::destruct(p);
  }
};

template<typename T, typename U>
inline bool operator ==(const custom_nballocator_t<T> &, const custom_nballocator_t<U> &)
{
  return true;
}

template<typename T, typename U>
inline bool operator !=(const custom_nballocator_t<T> &, const custom_nballocator_t<U> &)
{
  return false;
}

template<typename T>
bool CheckNullOrStructSize(const T * s) { return !s || (s->StructSize >= sizeof(T)); }

template<typename T>
bool CheckStructSize(const T * s) { return s && (s->StructSize >= sizeof(T)); }

// from Global.h
#define NB__TEXT(quote) L##quote      // r_winnt
#define NB_TEXT(quote) NB__TEXT(quote)

#ifdef _DEBUG

#define SELF_TEST(code) \
struct SelfTest {       \
  SelfTest() {          \
  code;                 \
}                       \
} _SelfTest;
#else
#define SELF_TEST(code)
#endif

#define NB_DISABLE_COPY(Class) \
public: \
  Class(Class &&) noexcept = delete; \
  Class & operator =(Class &&) noexcept = delete; \
  Class(const Class &) = delete; \
  Class & operator =(const Class &) = delete;

template <typename T1>
constexpr void used(const T1 &) {}

template <typename T1>
constexpr void unused(const T1 &) {}

template <typename T1>
constexpr void ignore(const T1 &) {}

template <typename T>
constexpr void ignore_result(const T &) {}

template <class T>
struct add_const
{
  using type = T const;
};

template <class T> struct
add_const<T&>
{
  using type = const T &;
};

template <class T> using add_const_t = typename add_const<T>::type;

} // namespace nb

/*#ifdef _WIN32
#define STD_ALLOC_CDECL __cdecl
#else
#define STD_ALLOC_CDECL
#endif

namespace std
{
  template <class _Tp1, class _Tp2>
  inline nb::custom_nballocator_t<_Tp2>& STD_ALLOC_CDECL
  __stl_alloc_rebind(nb::custom_nballocator_t<_Tp1>& __a, const _Tp2 *)
  {
    return (nb::custom_nballocator_t<_Tp2>&)(__a);
  }

  template <class _Tp1, class _Tp2>
  inline nb::custom_nballocator_t<_Tp2> STD_ALLOC_CDECL
  __stl_alloc_create(const nb::custom_nballocator_t<_Tp1>&, const _Tp2 *)
  {
    return nb::custom_nballocator_t<_Tp2>();
  }
}*/

#endif //__cplusplus
