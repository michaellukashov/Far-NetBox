#pragma once

#include <nbcore.h>
#include <nbsystem_cpp.h>

#if defined(__cplusplus)

template <class T>
inline const T Min(const T a, const T b) { return a < b ? a : b; }

template <class T>
inline const T Max(const T a, const T b) { return a > b ? a : b; }

template <class T>
inline const T Round(const T a, const T b) { return a / b + (a % b * 2 > b ? 1 : 0); }

template <class T>
inline double ToDouble(const T a) { return static_cast<double>(a); }

template <class T>
inline Word ToWord(const T a) { return static_cast<Word>(a); }
template <class T>
inline DWORD ToDWord(const T a) { return static_cast<DWORD>(a); }

template <class T>
inline typename std::is_convertible<T, intptr_t>::value
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_integral<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_enum<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_pointer<T>::value, intptr_t>::type
ToIntPtr(T a) { return (intptr_t)(a); }

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::is_convertible<T, uintptr_t>::value
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_enum<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_pointer<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return (uintptr_t)(a); }

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<sizeof(T) >= sizeof(uintptr_t), uintptr_t>::value
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

inline uintptr_t
ToUIntPtr(int64_t a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::is_convertible<T, int>::value
ToInt(T a) { return static_cast<int>(a); }

template <class T>
inline int
ToInt(T a) { return static_cast<int>(a); }

template <class T>
inline typename std::is_convertible<T, uint32_t>::value
ToUInt32(T a) { return static_cast<uint32_t>(a); }

template <class T>
inline uint32_t
ToUInt32(T a) { return static_cast<uint32_t>(a); }

template <class T>
inline typename std::is_convertible<T, int64_t>::value
ToInt64(T a) { return static_cast<int64_t>(a); }

template <class T>
inline int64_t
ToInt64(T a) { return static_cast<int64_t>(a); }

template <class T>
inline typename std::is_convertible<T, void *>::value
ToPtr(T a) { return const_cast<void *>(a); }

template <class T>
inline void *ToPtr(T a) { return reinterpret_cast<void *>((intptr_t)(a)); }

// MakeOtherType<T>::Type gives an other type corresponding to integer type T.
template <typename T>
struct MakeOtherType { typedef T Type; };

#define NB_SPECIALIZE_MAKE_T(T, U) \
  template <> \
  struct MakeOtherType<T> { typedef U Type; }

NB_SPECIALIZE_MAKE_T(DWORD, size_t);
NB_SPECIALIZE_MAKE_T(int, size_t);
NB_SPECIALIZE_MAKE_T(int64_t, size_t);

// Casts integer to other type
template <typename Int>
inline typename MakeOtherType<Int>::Type ToSizeT(Int Value)
{
  return static_cast<typename MakeOtherType<Int>::Type>(Value);
}

template<typename T>
inline void ClearStruct(T &s) { ::ZeroMemory(&s, sizeof(s)); }

template<typename T>
inline void ClearStruct(T *s) { T dont_instantiate_this_template_with_pointers = s; }

template<typename T, size_t N>
inline void ClearArray(T (&a)[N]) { ::ZeroMemory(a, sizeof(a[0]) * N); }

#endif // #if defined(__cplusplus)
