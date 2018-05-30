#include "stdafx.h"

#define BLOCK_ALLOCATED 0xABBABABA
#define BLOCK_FREED   0xDEADBEEF

static int CheckBlock(void *blk)
{
  int result = FALSE;
  char *p = static_cast<char *>(blk) - sizeof(uint32_t) * 2;

#if !defined(__MINGW32__)
  __try
#endif // defined(__MINGW32__)
  {
    uint32_t size = *reinterpret_cast<uint32_t *>(p);
    uint32_t *b = reinterpret_cast<uint32_t *>(&p[sizeof(uint32_t)]);
    uint32_t *e = reinterpret_cast<uint32_t *>(&p[sizeof(uint32_t) * 2 + size]);

    if (*b != BLOCK_ALLOCATED || *e != BLOCK_ALLOCATED)
    {
      if (*b == BLOCK_FREED && *e == BLOCK_FREED)
        OutputDebugStringA("memory block is already deleted\n");
      else
        OutputDebugStringA("memory block is corrupted\n");
#if defined(_DEBUG)
      DebugBreak();
#endif
    }
    else result = TRUE;
  }
#if !defined(__MINGW32__)
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    OutputDebugStringA("access violation during checking memory block\n");
#if defined(_DEBUG)
    DebugBreak();
#endif
  }
#endif // defined(__MINGW32__)

  return result;
}

/******************************************************************************/

NB_C_CORE_DLL(void *) nbcore_alloc(size_t size)
{
  if (size == 0)
    return nullptr;

  char *p = static_cast<char *>(nb_malloc(size + sizeof(uint32_t) * 3));
  if (p == nullptr)
  {
    OutputDebugStringA("memory overflow\n");
#if defined(_DEBUG)
    DebugBreak();
#endif
    return nullptr;
  }

  *reinterpret_cast<uint32_t *>(p) = ToUInt32(size);
  *reinterpret_cast<uint32_t *>(&p[sizeof(uint32_t)]) = BLOCK_ALLOCATED;
  *reinterpret_cast<uint32_t *>(&p[size + sizeof(uint32_t) * 2]) = BLOCK_ALLOCATED;
  return p + sizeof(uint32_t) * 2;
}

/******************************************************************************/

NB_C_CORE_DLL(void *) nbcore_calloc(size_t size)
{
  void *p = nbcore_alloc(size);
  if (p != nullptr)
    memset(p, 0, size);
  return p;
}

/******************************************************************************/

NB_C_CORE_DLL(void *) nbcore_realloc(void *ptr, size_t size)
{
  char *p;

  if (ptr != nullptr)
  {
    if (!CheckBlock(ptr))
      return nullptr;
    p = static_cast<char *>(ptr) - sizeof(uint32_t) * 2;
  }
  else p = nullptr;

  p = static_cast<char *>(nb_realloc(p, size + sizeof(uint32_t) * 3));
  if (p == nullptr)
  {
    OutputDebugStringA("memory overflow\n");
#if defined(_DEBUG)
    DebugBreak();
#endif
    return nullptr;
  }

  *reinterpret_cast<uint32_t *>(p) = ToUInt32(size);
  *reinterpret_cast<uint32_t *>(&p[sizeof(uint32_t)]) = BLOCK_ALLOCATED;
  *reinterpret_cast<uint32_t *>(&p[size + sizeof(uint32_t) * 2]) = BLOCK_ALLOCATED;
  return p + sizeof(uint32_t) * 2;
}

/******************************************************************************/

NB_C_CORE_DLL(void) nbcore_free(void *ptr)
{
  if (ptr == nullptr)
    return;
  if (!CheckBlock(ptr))
    return;

  char *p = static_cast<char *>(ptr) - sizeof(uint32_t) * 2;
  uint32_t size = *reinterpret_cast<uint32_t *>(p);

  *reinterpret_cast<uint32_t *>(&p[sizeof(uint32_t)]) = BLOCK_FREED;
  *reinterpret_cast<uint32_t *>(&p[size + sizeof(uint32_t) * 2]) = BLOCK_FREED;
  nb_free(p);
}

/******************************************************************************/

NB_CORE_DLL(char *) nbcore_strdup(const char *str)
{
  if (str == nullptr)
    return nullptr;

  char *p = static_cast<char *>(nbcore_alloc(strlen(str) + 1));
  if (p)
    strcpy(p, str);
  return p;
}

NB_CORE_DLL(wchar_t *) nbcore_wstrdup(const wchar_t *str)
{
  if (str == nullptr)
    return nullptr;

  wchar_t *p = static_cast<wchar_t *>(nbcore_alloc(sizeof(wchar_t) * (wcslen(str) + 1)));
  if (p)
    wcscpy(p, str);
  return p;
}

/******************************************************************************/

NB_CORE_DLL(char *) nbcore_strndup(const char *str, size_t len)
{
  if (str == nullptr || len == 0)
    return nullptr;

  char *p = static_cast<char *>(nbcore_alloc(len + 1));
  if (p)
  {
    memcpy(p, str, len);
    p[len] = 0;
  }
  return p;
}

NB_CORE_DLL(wchar_t *) nbcore_wstrndup(const wchar_t *str, size_t len)
{
  if (str == nullptr || len == 0)
    return nullptr;

  wchar_t *p = static_cast<wchar_t *>(nbcore_alloc(sizeof(wchar_t) * (len + 1)));
  if (p)
  {
    memcpy(p, str, sizeof(wchar_t) * len);
    p[len] = 0;
  }
  return p;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_snprintf(char *buffer, size_t count, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int len = _vsnprintf(buffer, count - 1, fmt, va);
  va_end(va);
  buffer[count - 1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_snwprintf(wchar_t *buffer, size_t count, const wchar_t *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int len = _vsnwprintf(buffer, count - 1, fmt, va);
  va_end(va);
  buffer[count - 1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_vsnprintf(char *buffer, size_t count, const char *fmt, va_list va)
{
  int len = _vsnprintf(buffer, count - 1, fmt, va);
  buffer[count - 1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *fmt, va_list va)
{
  int len = _vsnwprintf(buffer, count - 1, fmt, va);
  buffer[count - 1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(wchar_t *) nbcore_a2u_cp(const char *src, int codepage)
{
  if (src == nullptr)
    return nullptr;

  int cbLen = ::MultiByteToWideChar(codepage, 0, src, -1, nullptr, 0);
  wchar_t *result = static_cast<wchar_t *>(nbcore_alloc(sizeof(wchar_t) * (cbLen + 1)));
  if (result == nullptr)
    return nullptr;

  ::MultiByteToWideChar(codepage, 0, src, -1, result, cbLen);
  result[cbLen] = 0;
  return result;
}

/******************************************************************************/

NB_CORE_DLL(wchar_t *) nbcore_a2u(const char *src)
{
  return nbcore_a2u_cp(src, Langpack_GetDefaultCodePage());
}

/******************************************************************************/

NB_CORE_DLL(char *) nbcore_u2a_cp(const wchar_t *src, int codepage)
{
  if (src == nullptr)
    return nullptr;

  int cbLen = WideCharToMultiByte(codepage, 0, src, -1, nullptr, 0, nullptr, nullptr);
  char *result = static_cast<char *>(nbcore_alloc(cbLen + 1));
  if (result == nullptr)
    return nullptr;

  WideCharToMultiByte(codepage, 0, src, -1, result, cbLen, nullptr, nullptr);
  result[cbLen] = 0;
  return result;
}

/******************************************************************************/

NB_CORE_DLL(char *) nbcore_u2a(const wchar_t *src)
{
  return nbcore_u2a_cp(src, Langpack_GetDefaultCodePage());
}
