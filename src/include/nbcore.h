#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <nbtypes.h>

#ifdef NB_CORE_EXPORTS
#define NB_CORE_EXPORT // __declspec(dllexport)
#else
#define NB_CORE_EXPORT // __declspec(dllimport)
#endif

#define NB_CORE_DLL(T) NB_CORE_EXPORT T __stdcall
#define NB_C_CORE_DLL(T) NB_CORE_EXPORT T __cdecl

#pragma warning(disable: 4201 4127 4312 4706)

#if defined(__cplusplus)
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// memory functions

NB_C_CORE_DLL(void *) nbcore_alloc(size_t);
NB_C_CORE_DLL(void *) nbcore_calloc(size_t);
NB_C_CORE_DLL(void *) nbcore_realloc(void *ptr, size_t);
NB_C_CORE_DLL(void)   nbcore_free(void *ptr);

NB_CORE_DLL(size_t)   nbcore_strlen(const char *p);
NB_CORE_DLL(size_t)   nbcore_wstrlen(const wchar_t *p);

NB_CORE_DLL(char *)    nbcore_strcpy(char *dest, const char *src);
NB_CORE_DLL(wchar_t *) nbcore_wstrcpy(wchar_t *dest, const wchar_t *src);

NB_CORE_DLL(char *)    nbcore_strncpy(char *dest, const char *src, size_t len);
NB_CORE_DLL(wchar_t *) nbcore_wstrncpy(wchar_t *dest, const wchar_t *src, size_t len);

NB_CORE_DLL(char *)    nbcore_strcat(char *dest, const char *src);
NB_CORE_DLL(wchar_t *) nbcore_wstrcat(wchar_t *dest, const wchar_t *src);

NB_CORE_DLL(char *)    nbcore_strncat(char *dest, const char *src, size_t len);
NB_CORE_DLL(wchar_t *) nbcore_wstrncat(wchar_t *dest, const wchar_t *src, size_t len);

NB_CORE_DLL(int)      nbcore_strcmp(const char *p1, const char *p2);
NB_CORE_DLL(int)      nbcore_strncmp(const char *p1, const char *p2, size_t n);
NB_CORE_DLL(int)      nbcore_wstrcmp(const wchar_t *p1, const wchar_t *p2);
NB_CORE_DLL(int)      nbcore_wstrncmp(const wchar_t *p1, const wchar_t *p2, size_t n);

NB_CORE_DLL(int)      nbcore_strcmpi(const char *p1, const char *p2);
NB_CORE_DLL(int)      nbcore_strncmpi(const char *p1, const char *p2, size_t n);
NB_CORE_DLL(int)      nbcore_wstrcmpi(const wchar_t *p1, const wchar_t *p2);
NB_CORE_DLL(int)      nbcore_wstrncmpi(const wchar_t *p1, const wchar_t *p2, size_t n);

NB_CORE_DLL(char *)    nbcore_strdup(const char *str);
NB_CORE_DLL(wchar_t *) nbcore_wstrdup(const wchar_t *str);

NB_CORE_DLL(char *)    nbcore_strndup(const char *str, size_t len);
NB_CORE_DLL(wchar_t *) nbcore_wstrndup(const wchar_t *str, size_t len);

#if defined(__cplusplus)
} // extern "C"

template <size_t _Size>
inline int nbcore_snprintf(char(&buffer)[_Size], const char * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  const int ret = nbcore_vsnprintf(buffer, _Size, fmt, args);
  va_end(args);
  return ret;
}

template <size_t _Size>
inline int nbcore_snwprintf(wchar_t(&buffer)[_Size], const wchar_t * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  const int ret = nbcore_vsnwprintf(buffer, _Size, fmt, args);
  va_end(args);
  return ret;
}

template <size_t _Size>
inline int nbcore_vsnprintf(char(&buffer)[_Size], const char * fmt, va_list va)
{
  return nbcore_vsnprintf(buffer, _Size, fmt, va);
}

template <size_t _Size>
inline int nbcore_vsnwprintf(wchar_t(&buffer)[_Size], const wchar_t * fmt, va_list va)
{
  return nbcore_vsnwprintf(buffer, _Size, fmt, va);
}

#endif // if defined(__cplusplus)

#if 0
#ifndef NB_CORE_EXPORTS
#if !defined(_WIN64)
#pragma comment(lib, "nbcore.lib")
#else
#pragma comment(lib, "nbcore64.lib")
#endif
#endif
#endif
