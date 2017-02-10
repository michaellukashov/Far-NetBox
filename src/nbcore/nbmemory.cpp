#include "stdafx.h"

#define BLOCK_ALLOCED 0xABBABABA
#define BLOCK_FREED   0xDEADBEEF

static int CheckBlock(void* blk)
{
  int result = FALSE;
  char* p = (char*)blk - sizeof(uint32_t)*2;
  uint32_t size, *b, *e;

  __try
  {
    size = *(uint32_t*)p;
    b = (uint32_t*)&p[ sizeof(uint32_t) ];
    e = (uint32_t*)&p[ sizeof(uint32_t)*2 + size ];

    if (*b != BLOCK_ALLOCED || *e != BLOCK_ALLOCED)
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
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    OutputDebugStringA("access violation during checking memory block\n");
    #if defined(_DEBUG)
      DebugBreak();
    #endif
  }

  return result;
}

/******************************************************************************/

NB_C_CORE_DLL(void*) nbcore_alloc(size_t size)
{
  if (size == 0)
    return NULL;

  char *p = (char*)nb_malloc(size + sizeof(uint32_t)* 3);
  if (p == NULL) {
    OutputDebugStringA("memory overflow\n");
    #if defined(_DEBUG)
      DebugBreak();
    #endif
    return NULL;
  }

  *(uint32_t*)p = (uint32_t)size;
  *(uint32_t*)&p[sizeof(uint32_t)] = BLOCK_ALLOCED;
  *(uint32_t*)&p[size + sizeof(uint32_t)*2] = BLOCK_ALLOCED;
  return p + sizeof(uint32_t)* 2;
}

/******************************************************************************/

NB_C_CORE_DLL(void*) nbcore_calloc(size_t size)
{
  void* p = nbcore_alloc(size);
  if (p != NULL)
    memset(p, 0, size);
  return p;
}

/******************************************************************************/

NB_C_CORE_DLL(void*) nbcore_realloc(void* ptr, size_t size)
{
  char *p;

  if (ptr != NULL) {
    if (!CheckBlock(ptr))
      return NULL;
    p = (char*)ptr - sizeof(uint32_t)*2;
  }
  else p = NULL;

  p = (char*)nb_realloc(p, size + sizeof(uint32_t)*3);
  if (p == NULL) {
    OutputDebugStringA("memory overflow\n");
    #if defined(_DEBUG)
      DebugBreak();
    #endif
    return NULL;
  }

  *(uint32_t*)p = (uint32_t)size;
  *(uint32_t*)&p[sizeof(uint32_t)] = BLOCK_ALLOCED;
  *(uint32_t*)&p[size + sizeof(uint32_t)*2] = BLOCK_ALLOCED;
  return p + sizeof(uint32_t)*2;
}

/******************************************************************************/

NB_C_CORE_DLL(void) nbcore_free(void* ptr)
{
  char* p;
  uint32_t size;

  if (ptr == NULL)
    return;
  if (!CheckBlock(ptr))
    return;

  p = (char*)ptr - sizeof(uint32_t)*2;
  size = *(uint32_t*)p;

  *(uint32_t*)&p[sizeof(uint32_t)] = BLOCK_FREED;
  *(uint32_t*)&p[size + sizeof(uint32_t)*2] = BLOCK_FREED;
  nb_free(p);
}

/******************************************************************************/

NB_CORE_DLL(char*) nbcore_strdup(const char *str)
{
  if (str == NULL)
    return NULL;

  char *p = (char*)nbcore_alloc(strlen(str)+1);
  if (p)
    strcpy(p, str);
  return p;
}

NB_CORE_DLL(wchar_t*) nbcore_wstrdup(const wchar_t *str)
{
  if (str == NULL)
    return NULL;

  wchar_t *p = (wchar_t*)nbcore_alloc(sizeof(wchar_t)*(wcslen(str)+1));
  if (p)
    wcscpy(p, str);
  return p;
}

/******************************************************************************/

NB_CORE_DLL(char*) nbcore_strndup(const char *str, size_t len)
{
  if (str == NULL || len == 0)
    return NULL;

  char *p = (char*)nbcore_alloc(len+1);
  if (p) {
    memcpy(p, str, len);
    p[len] = 0;
  }
  return p;
}

NB_CORE_DLL(wchar_t*) nbcore_wstrndup(const wchar_t *str, size_t len)
{
  if (str == NULL || len == 0)
    return NULL;

  wchar_t *p = (wchar_t*)nbcore_alloc(sizeof(wchar_t)*(len+1));
  if (p) {
    memcpy(p, str, sizeof(wchar_t)*len);
    p[len] = 0;
  }
  return p;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_snprintf(char *buffer, size_t count, const char* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int len = _vsnprintf(buffer, count-1, fmt, va);
  va_end(va);
  buffer[count-1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_snwprintf(wchar_t *buffer, size_t count, const wchar_t* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int len = _vsntprintf(buffer, count-1, fmt, va);
  va_end(va);
  buffer[count-1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_vsnprintf(char *buffer, size_t count, const char* fmt, va_list va)
{
  int len = _vsnprintf(buffer, count-1, fmt, va);
  buffer[count-1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(int) nbcore_vsnwprintf(wchar_t *buffer, size_t count, const wchar_t* fmt, va_list va)
{
  int len = _vsntprintf(buffer, count-1, fmt, va);
  buffer[count-1] = 0;
  return len;
}

/******************************************************************************/

NB_CORE_DLL(wchar_t*) nbcore_a2u_cp(const char* src, int codepage)
{
  if (src == NULL)
    return NULL;

  int cbLen = ::MultiByteToWideChar(codepage, 0, src, -1, NULL, 0);
  wchar_t* result = (wchar_t*)nbcore_alloc(sizeof(wchar_t)*(cbLen+1));
  if (result == NULL)
    return NULL;

  ::MultiByteToWideChar(codepage, 0, src, -1, result, cbLen);
  result[cbLen] = 0;
  return result;
}

/******************************************************************************/

NB_CORE_DLL(wchar_t*) nbcore_a2u(const char* src)
{
  return nbcore_a2u_cp(src, Langpack_GetDefaultCodePage());
}

/******************************************************************************/

NB_CORE_DLL(char*) nbcore_u2a_cp(const wchar_t* src, int codepage)
{
  if (src == NULL)
    return NULL;

  int cbLen = WideCharToMultiByte(codepage, 0, src, -1, NULL, 0, NULL, NULL);
  char* result = (char*)nbcore_alloc(cbLen+1);
  if (result == NULL)
    return NULL;

  WideCharToMultiByte(codepage, 0, src, -1, result, cbLen, NULL, NULL);
  result[cbLen] = 0;
  return result;
}

/******************************************************************************/

NB_CORE_DLL(char*) nbcore_u2a(const wchar_t* src)
{
  return nbcore_u2a_cp(src, Langpack_GetDefaultCodePage());
}
