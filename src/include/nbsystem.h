#pragma once

#include <vector>
#include <map>
#include <list>
#include <set>

#include <nbcore.h>

#if defined(__cplusplus)

namespace nb {

template<typename T> using vector_t = std::vector<T>;
template<typename T> using set_t = std::set<T>;
template<typename T> using list_t = std::list<T>;
template<typename Tk, typename Tv> using map_t = std::map<Tk, Tv>;

} // namespace nb

namespace nb {

template <class T>
inline constexpr T Min(const T a, const T b) { return a < b ? a : b; }

template <class T>
inline constexpr T Max(const T a, const T b) { return a > b ? a : b; }

template <class T>
inline constexpr T Round(const T a, const T b) { return a / b + (a % b * 2 > b ? 1 : 0); }

template <class T>
inline constexpr double ToDouble(const T a) { return static_cast<double>(a); }

template <class T>
inline constexpr Word ToWord(const T a) { return static_cast<Word>(a); }
template <class T>
inline constexpr DWORD ToDWord(const T a) { return static_cast<DWORD>(a); }

template <class T>
inline constexpr std::enable_if_t<std::is_integral_v<T>, intptr_t>
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline constexpr std::enable_if_t<std::is_enum_v<T>, int32_t>
ToIntPtr(T a) { return static_cast<int32_t>(a); }

template <class T>
inline constexpr std::enable_if_t<std::is_pointer_v<T>, intptr_t>
ToIntPtr(T a) { return reinterpret_cast<intptr_t>(a); }

template <class T>
inline constexpr typename std::enable_if<std::is_floating_point<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<int32_t>(a); }

template <class T>
inline constexpr typename std::is_convertible<T, uintptr_t>::value
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline constexpr typename std::enable_if<std::is_enum<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline constexpr typename std::enable_if<std::is_pointer<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return reinterpret_cast<uintptr_t>(a); }

template <class T>
inline constexpr typename std::enable_if<std::is_floating_point<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline constexpr typename std::enable_if<sizeof(T) >= sizeof(uintptr_t), uintptr_t>::value
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

inline constexpr uintptr_t
ToUIntPtr(int64_t a) { return static_cast<uintptr_t>(a); }

template <class T>
inline constexpr typename std::enable_if<std::is_floating_point<T>::value, int32_t>::type
ToInt32(T a) { return static_cast<int32_t>(a); }

inline constexpr int32_t
ToInt32(int64_t a) { return static_cast<int32_t>(a); }

template <class T>
inline constexpr int32_t
ToInt(T a) { return static_cast<int32_t>(a); }

template <class T>
inline constexpr typename std::is_convertible<T, uint32_t>::value
ToUInt32(T a) { return static_cast<uint32_t>(a); }

template <class T>
inline constexpr uint32_t
ToUInt32(T a) { return static_cast<uint32_t>(a); }

template <class T>
inline constexpr typename std::is_convertible<T, int64_t>::value
ToInt64(T a) { return static_cast<int64_t>(a); }

template <class T>
inline constexpr int64_t
ToInt64(T a) { return static_cast<int64_t>(a); }

template <class T>
inline constexpr uint64_t
ToUInt64(T a) { return static_cast<uint64_t>(a); }

template <class T>
inline constexpr uint16_t
ToUInt16(T a) { return static_cast<uint16_t>(a); }

template <class T>
inline constexpr typename std::is_convertible<T, void *>::value
ToPtr(T a) { return const_cast<void *>(a); }

template <class T>
inline constexpr void * ToPtr(T a) { return reinterpret_cast<void *>((intptr_t)(a)); }

inline constexpr int8_t *
ToInt8Ptr(void * a) { return static_cast<int8_t *>(a); }

inline constexpr const int8_t *
ToInt8Ptr(const void * a) { return static_cast<const int8_t *>(a); }

inline constexpr uint8_t *
ToUInt8Ptr(void * a) { return static_cast<uint8_t *>(a); }

inline constexpr const uint8_t *
ToUInt8Ptr(const void * a) { return static_cast<const uint8_t *>(a); }

// MakeOtherType<T>::Type gives an other type corresponding to integer type T.
template <typename T>
struct MakeOtherType { using Type = T; };

#define NB_SPECIALIZE_MAKE_T(T, U) \
  template <> \
  struct MakeOtherType<T> { using Type = U; }

NB_SPECIALIZE_MAKE_T(DWORD, size_t);
NB_SPECIALIZE_MAKE_T(int, size_t);
NB_SPECIALIZE_MAKE_T(int64_t, size_t);
NB_SPECIALIZE_MAKE_T(uint64_t, size_t);

// Casts integer to other type
template <typename Int>
inline typename MakeOtherType<Int>::Type ToSizeT(Int Value)
{
  return static_cast<typename MakeOtherType<Int>::Type>(Value);
}

template<typename T>
inline void ClearStruct(T & s) { ::ZeroMemory(&s, sizeof(s)); }

template<typename T>
inline void ClearStruct(T * s) { T dont_instantiate_this_template_with_pointers = s; }

template<typename T, size_t N>
inline void ClearArray(T (&a)[N]) { ::ZeroMemory(a, sizeof(a[0]) * N); }

} // namespace nb

#endif // #if defined(__cplusplus)

