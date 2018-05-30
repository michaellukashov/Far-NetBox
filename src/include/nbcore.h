#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
// command line support

NB_CORE_DLL(void)    CmdLine_Parse(LPTSTR ptszCmdLine);
NB_CORE_DLL(LPCTSTR) CmdLine_GetOption(LPCTSTR ptszParameter);

///////////////////////////////////////////////////////////////////////////////

typedef uint32_t MEVENT;

///////////////////////////////////////////////////////////////////////////////
// events, hooks & services

#define MAXMODULELABELLENGTH 64

typedef int (*NETBOXHOOK)(WPARAM, LPARAM);
typedef int (*NETBOXHOOKPARAM)(WPARAM, LPARAM, LPARAM);
typedef int (*NETBOXHOOKOBJ)(void *, WPARAM, LPARAM);
typedef int (*NETBOXHOOKOBJPARAM)(void *, WPARAM, LPARAM, LPARAM);

typedef intptr_t (*NETBOXSERVICE)(WPARAM, LPARAM);
typedef intptr_t (*NETBOXSERVICEPARAM)(WPARAM, LPARAM, LPARAM);
typedef intptr_t (*NETBOXSERVICEOBJ)(void *, WPARAM, LPARAM);
typedef intptr_t (*NETBOXSERVICEOBJPARAM)(void *, WPARAM, LPARAM, LPARAM);

#ifdef _WIN64
#define CALLSERVICE_NOTFOUND      ((intptr_t)0x8000000000000000)
#else
#define CALLSERVICE_NOTFOUND      ((int)0x80000000)
#endif

NB_CORE_DLL(HANDLE)  CreateHookableEvent(const char *name);
NB_CORE_DLL(int)     DestroyHookableEvent(HANDLE hEvent);
NB_CORE_DLL(int)     SetHookDefaultForHookableEvent(HANDLE hEvent, NETBOXHOOK pfnHook);
NB_CORE_DLL(int)     CallPluginEventHook(HINSTANCE hInst, HANDLE hEvent, WPARAM wParam = 0, LPARAM lParam = 0);
NB_CORE_DLL(int)     NotifyEventHooks(HANDLE hEvent, WPARAM wParam = 0, LPARAM lParam = 0);
NB_CORE_DLL(int)     NotifyFastHook(HANDLE hEvent, WPARAM wParam = 0, LPARAM lParam = 0);

NB_CORE_DLL(HANDLE)  HookEvent(const char *name, NETBOXHOOK hookProc);
NB_CORE_DLL(HANDLE)  HookEventParam(const char *name, NETBOXHOOKPARAM hookProc, LPARAM lParam = 0);
NB_CORE_DLL(HANDLE)  HookEventObj(const char *name, NETBOXHOOKOBJ hookProc, void *object);
NB_CORE_DLL(HANDLE)  HookEventObjParam(const char *name, NETBOXHOOKOBJPARAM hookProc, void *object, LPARAM lParam);
NB_CORE_DLL(HANDLE)  HookEventMessage(const char *name, HWND hwnd, UINT message);

// executes the event handler if event is missing
NB_CORE_DLL(HANDLE)  HookTemporaryEvent(const char *name, NETBOXHOOK hookProc);

NB_CORE_DLL(int)     UnhookEvent(HANDLE hHook);
NB_CORE_DLL(void)    KillObjectEventHooks(void *pObject);
NB_CORE_DLL(void)    KillModuleEventHooks(HINSTANCE pModule);

NB_CORE_DLL(HANDLE)  CreateServiceFunction(const char *name, NETBOXSERVICE serviceProc);
NB_CORE_DLL(HANDLE)  CreateServiceFunctionParam(const char *name, NETBOXSERVICEPARAM serviceProc, LPARAM lParam);
NB_CORE_DLL(HANDLE)  CreateServiceFunctionObj(const char *name, NETBOXSERVICEOBJ serviceProc, void *object);
NB_CORE_DLL(HANDLE)  CreateServiceFunctionObjParam(const char *name, NETBOXSERVICEOBJPARAM serviceProc, void *object, LPARAM lParam);
NB_CORE_DLL(HANDLE)  CreateProtoServiceFunction(const char *szModule, const char *szService, NETBOXSERVICE serviceProc);
NB_CORE_DLL(int)     DestroyServiceFunction(HANDLE hService);
NB_CORE_DLL(bool)    ServiceExists(const char *name);

NB_CORE_DLL(intptr_t) CallService(const char *name, WPARAM wParam = 0, LPARAM lParam = 0);
NB_CORE_DLL(intptr_t) CallServiceSync(const char *name, WPARAM wParam = 0, LPARAM lParam = 0);

NB_CORE_DLL(intptr_t) CallFunctionSync(intptr_t(__stdcall *func)(void *), void *arg);
NB_CORE_DLL(int)     CallFunctionAsync(void (__stdcall *func)(void *), void *arg);
NB_CORE_DLL(void)    KillModuleServices(HINSTANCE hInst);
NB_CORE_DLL(void)    KillObjectServices(void *pObject);

NB_CORE_DLL(int)      ProtoServiceExists(LPCSTR szModule, const char *szService);
NB_CORE_DLL(intptr_t)  CallProtoService(LPCSTR szModule, const char *szService, WPARAM wParam = 0, LPARAM lParam = 0);

///////////////////////////////////////////////////////////////////////////////
// exceptions

typedef uint32_t (__cdecl *pfnExceptionFilter)(uint32_t code, EXCEPTION_POINTERS *info);

NB_CORE_DLL(pfnExceptionFilter) GetExceptionFilter();
NB_CORE_DLL(pfnExceptionFilter) SetExceptionFilter(pfnExceptionFilter pMirandaExceptFilter);

///////////////////////////////////////////////////////////////////////////////
// http support

NB_CORE_DLL(char *) nbcore_urlEncode(const char *szUrl);

///////////////////////////////////////////////////////////////////////////////
// language packs support

NB_CORE_DLL(uint32_t) nbcore_hash(const void *key, uint32_t len);

#pragma optimize("gt", on)
__forceinline uint32_t nbcore_hashstr(const char *key)
{
  if (key == nullptr)
  {
    return 0;
  }
  else
  {
    const uint32_t len = static_cast<uint32_t>(strlen(key));
    return nbcore_hash(key, len);
  }
}

__forceinline uint32_t nbcore_hashstrW(const wchar_t *key)
{
  if (key == nullptr)
  {
    return 0;
  }
  else
  {
    uint32_t len = static_cast<uint32_t>(wcslen(static_cast<const wchar_t *>(key)));
    return nbcore_hash(key, len * sizeof(wchar_t));
  }
}
#pragma optimize("", on)

#define nbcore_hashstrT nbcore_hashstrW

///////////////////////////////////////////////////////////////////////////////
// lists

typedef int (*FSortFunc)(void *, void *); // sort function prototype

// Assumes first 32 bit value of the data is the numeric key
// and uses it to perform sort/search operations, this results
// in much better performance as no compare function calls needed
// Incredibly useful for Hash Tables
#define NumericKeySort ((FSortFunc)(void*) -1)
#define HandleKeySort  ((FSortFunc)(void*) -2)
#define PtrKeySort     ((FSortFunc)(void*) -3)

typedef struct
{
  void    **items;
  int     realCount;
  int     limit;
  int     increment;

  FSortFunc sortFunc;
}
SortedList;

NB_CORE_DLL(SortedList *) List_Create(int p_limit, int p_increment);
NB_CORE_DLL(void)        List_Destroy(SortedList *p_list);
NB_CORE_DLL(void *)       List_Find(SortedList *p_list, void *p_value);
NB_CORE_DLL(int)         List_GetIndex(SortedList *p_list, void *p_value, int *p_index);
NB_CORE_DLL(int)         List_IndexOf(SortedList *p_list, void *p_value);
NB_CORE_DLL(int)         List_Insert(SortedList *p_list, void *p_value, int p_index);
NB_CORE_DLL(int)         List_InsertPtr(SortedList *list, void *p);
NB_CORE_DLL(int)         List_Remove(SortedList *p_list, int index);
NB_CORE_DLL(int)         List_RemovePtr(SortedList *list, void *p);
NB_CORE_DLL(void)        List_Copy(SortedList *s, SortedList *d, size_t itemSize);
NB_CORE_DLL(void)        List_ObjCopy(SortedList *s, SortedList *d, size_t itemSize);

///////////////////////////////////////////////////////////////////////////////
// logging functions

NB_CORE_DLL(HANDLE) nbcore_createLog(const char *pszName, const wchar_t *ptszDescr, const wchar_t *ptszFile, unsigned options);
NB_CORE_DLL(void)   nbcore_closeLog(HANDLE hLogger);

NB_C_CORE_DLL(int)  nbcore_writeLogA(HANDLE hLogger, const char *format, ...);
NB_C_CORE_DLL(int)  nbcore_writeLogW(HANDLE hLogger, const wchar_t *format, ...);

NB_CORE_DLL(int)    nbcore_writeLogVA(HANDLE hLogger, const char *format, va_list args);
NB_CORE_DLL(int)    nbcore_writeLogVW(HANDLE hLogger, const wchar_t *format, va_list args);

///////////////////////////////////////////////////////////////////////////////
// md5 functions

typedef struct nbcore_md5_state_s
{
  uint32_t count[2];  /* message length in bits, lsw first */
  uint32_t abcd[4];    /* digest buffer */
  uint8_t   buf[64];    /* accumulate block */
} nbcore_md5_state_t;

NB_CORE_DLL(void) nbcore_md5_init(nbcore_md5_state_t *pms);
NB_CORE_DLL(void) nbcore_md5_append(nbcore_md5_state_t *pms, const uint8_t *data, size_t nbytes);
NB_CORE_DLL(void) nbcore_md5_finish(nbcore_md5_state_t *pms, uint8_t digest[16]);
NB_CORE_DLL(void) nbcore_md5_hash(const uint8_t *data, size_t len, uint8_t digest[16]);

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

///////////////////////////////////////////////////////////////////////////////
// modules

NB_CORE_DLL(void) RegisterModule(HINSTANCE hInst);
NB_CORE_DLL(void) UnregisterModule(HINSTANCE hInst);

NB_CORE_DLL(HINSTANCE) GetInstByAddress(void *codePtr);

///////////////////////////////////////////////////////////////////////////////
// print functions

NB_CORE_DLL(int)    nbcore_snprintf(char *buffer, size_t count, const char *fmt, ...);
NB_CORE_DLL(int)    nbcore_snwprintf(wchar_t *buffer, size_t count, const wchar_t *fmt, ...);
NB_CORE_DLL(int)    nbcore_vsnprintf(char *buffer, size_t count, const char *fmt, va_list va);
NB_CORE_DLL(int)    nbcore_vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *fmt, va_list va);

///////////////////////////////////////////////////////////////////////////////
// protocol functions

struct PROTO_INTERFACE;

NB_CORE_DLL(intptr_t) ProtoBroadcastAck(LPCSTR szModule, int type, int result, HANDLE hProcess, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////
// sha1 functions

#define NB_SHA1_HASH_SIZE 20
#define NB_SHA_BLOCKSIZE 64

typedef struct nbcore_sha1_ctx_
{
  uint32_t H[5];
  uint32_t W[80];
  int lenW;
  uint32_t sizeHi, sizeLo;
} nbcore_sha1_ctx;

NB_CORE_DLL(void) nbcore_sha1_init(nbcore_sha1_ctx *ctx);
NB_CORE_DLL(void) nbcore_sha1_append(nbcore_sha1_ctx *ctx, const uint8_t *dataIn, size_t len);
NB_CORE_DLL(void) nbcore_sha1_finish(nbcore_sha1_ctx *ctx, uint8_t hashout[NB_SHA1_HASH_SIZE]);
NB_CORE_DLL(void) nbcore_sha1_hash(uint8_t *dataIn, size_t len, uint8_t hashout[NB_SHA1_HASH_SIZE]);

NB_CORE_DLL(void) nbcore_hmac_sha1(uint8_t hashout[NB_SHA1_HASH_SIZE], const uint8_t *key, size_t keylen, const uint8_t *text, size_t textlen);

///////////////////////////////////////////////////////////////////////////////
// sha256 functions

#define NB_SHA256_HASH_SIZE 32

typedef struct SHA256_CONTEXT_
{
  uint32_t  h0, h1, h2, h3, h4, h5, h6, h7;
  uint32_t  nblocks;
  uint8_t buf[NB_SHA_BLOCKSIZE];
  int  count;
} SHA256_CONTEXT;

NB_CORE_DLL(void) nbcore_sha256_init(SHA256_CONTEXT *ctx);
NB_CORE_DLL(void) nbcore_sha256_write(SHA256_CONTEXT *ctx, const void *dataIn, size_t len);
NB_CORE_DLL(void) nbcore_sha256_final(SHA256_CONTEXT *ctx, uint8_t hashout[NB_SHA256_HASH_SIZE]);
NB_CORE_DLL(void) nbcore_sha256_hash(const void *dataIn, size_t len, uint8_t hashout[NB_SHA256_HASH_SIZE]);

NB_CORE_DLL(void) nbcore_hmac_sha256(uint8_t hashout[NB_SHA256_HASH_SIZE], const uint8_t *key, size_t keylen, const uint8_t *text, size_t textlen);

///////////////////////////////////////////////////////////////////////////////
// strings

NB_CORE_DLL(void *) nbcore_base64_decode(const char *input, size_t *outputLen);
NB_CORE_DLL(char *) nbcore_base64_encode(const void *input, size_t inputLen);
NB_CORE_DLL(char *) nbcore_base64_encodebuf(const void *input, size_t inputLen, char *output, size_t outLen);

__forceinline size_t nbcore_base64_encode_bufsize(size_t inputLen)
{
  return 4 * ((inputLen + 2) / 3) + 1;
}

NB_CORE_DLL(char *)  rtrim(char *str);
NB_CORE_DLL(wchar_t *) rtrimw(wchar_t *str);

NB_CORE_DLL(char *)  ltrim(char *str);  // returns pointer to the beginning of string
NB_CORE_DLL(wchar_t *) ltrimw(wchar_t *str);

NB_CORE_DLL(char *)  ltrimp(char *str); // returns pointer to the trimmed portion of string
NB_CORE_DLL(wchar_t *) ltrimpw(wchar_t *str);

NB_CORE_DLL(char *) strdel(char *str, size_t len);
NB_CORE_DLL(wchar_t *) strdelw(wchar_t *str, size_t len);

NB_CORE_DLL(int) wildcmp(const char *name, const char *mask);
NB_CORE_DLL(int) wildcmpw(const wchar_t *name, const wchar_t *mask);

NB_CORE_DLL(int) wildcmpi(const char *name, const char *mask);
NB_CORE_DLL(int) wildcmpiw(const wchar_t *name, const wchar_t *mask);

NB_CORE_DLL(char *)  bin2hex(const void *pData, size_t len, char *dest);
NB_CORE_DLL(wchar_t *) bin2hexW(const void *pData, size_t len, wchar_t *dest);

NB_CORE_DLL(bool) hex2bin(const char *pSrc, void *pData, size_t len);
NB_CORE_DLL(bool) hex2binW(const wchar_t *pSrc, void *pData, size_t len);

__forceinline char *lrtrim(char *str) { return ltrim(rtrim(str)); }
__forceinline char *lrtrimp(char *str) { return ltrimp(rtrim(str)); }

#if defined( __cplusplus )
NB_CORE_DLL(char *) replaceStr(char *&dest, const char *src);
NB_CORE_DLL(wchar_t *) replaceStrW(wchar_t *&dest, const wchar_t *src);
#else
NB_CORE_DLL(char *) replaceStr(char **dest, const char *src);
NB_CORE_DLL(wchar_t *) replaceStrW(wchar_t **dest, const wchar_t *src);
#endif

///////////////////////////////////////////////////////////////////////////////
// text conversion functions

typedef union
{
  char *a; // utf8 or ansi strings
  wchar_t *w; // strings of WCHARs
} MAllStrings;

typedef union
{
  char **a; // array of utf8 or ansi strings
  wchar_t **w; // array of strings of WCHARs
} MAllStringArray;

NB_CORE_DLL(wchar_t *) nbcore_a2u_cp(const char *src, int codepage);
NB_CORE_DLL(wchar_t *) nbcore_a2u(const char *src);
NB_CORE_DLL(char *)  nbcore_u2a_cp(const wchar_t *src, int codepage);
NB_CORE_DLL(char *)  nbcore_u2a(const wchar_t *src);

///////////////////////////////////////////////////////////////////////////////
// threads

typedef void (__cdecl *pThreadFunc)(void *);
typedef unsigned (__stdcall *pThreadFuncEx)(void *);
typedef unsigned (__cdecl *pThreadFuncOwner)(void *owner, void *param);

#if defined( __cplusplus )
NB_CORE_DLL(intptr_t) Thread_Push(HINSTANCE hInst, void *pOwner = nullptr);
#else
NB_CORE_DLL(intptr_t) Thread_Push(HINSTANCE hInst, void *pOwner);
#endif
NB_CORE_DLL(intptr_t) Thread_Pop();
NB_CORE_DLL(void)    Thread_Wait();

#if defined( __cplusplus )
NB_CORE_DLL(HANDLE) nbcore_forkthread(pThreadFunc aFunc, void *arg = nullptr);
NB_CORE_DLL(HANDLE) nbcore_forkthreadex(pThreadFuncEx aFunc, void *arg = nullptr, unsigned *pThreadID = nullptr);
NB_CORE_DLL(HANDLE) nbcore_forkthreadowner(pThreadFuncOwner aFunc, void *owner, void *arg = nullptr, unsigned *pThreadID = nullptr);
#else
NB_CORE_DLL(HANDLE) nbcore_forkthread(pThreadFunc aFunc, void *arg);
NB_CORE_DLL(HANDLE) nbcore_forkthreadex(pThreadFuncEx aFunc, void *arg, unsigned *pThreadID);
NB_CORE_DLL(HANDLE) nbcore_forkthreadowner(pThreadFuncOwner aFunc, void *owner, void *arg, unsigned *pThreadID);
#endif

NB_CORE_DLL(void) Thread_SetName(const char *szThreadName);

NB_CORE_DLL(void) KillObjectThreads(void *pObject);

///////////////////////////////////////////////////////////////////////////////
// utf8 interface

NB_CORE_DLL(char *) Utf8Decode(char *str, wchar_t **ucs2);
NB_CORE_DLL(char *) Utf8DecodeCP(char *str, int codepage, wchar_t **ucs2);
NB_CORE_DLL(int)   Utf8toUcs2(const char *src, size_t srclen, wchar_t *dst, size_t dstlen); // returns 0 on error

NB_CORE_DLL(wchar_t *) Utf8DecodeW(const char *str);

NB_CORE_DLL(char *) Utf8Encode(const char *str);
NB_CORE_DLL(char *) Utf8EncodeCP(const char *src, int codepage);

NB_CORE_DLL(char *) Utf8EncodeW(const wchar_t *str);
NB_CORE_DLL(int)   Ucs2toUtf8Len(const wchar_t *src);

NB_CORE_DLL(BOOL)  Utf8CheckString(const char *str);

#define nbcore_utf8decode(A, B)      Utf8Decode(A, B)
#define nbcore_utf8decodecp(A, B, C) Utf8DecodeCP(A, B, C)
#define nbcore_utf8decodeW(A)         Utf8DecodeW(A)
#define nbcore_utf8encode(A)         Utf8Encode(A)
#define nbcore_utf8encodecp(A, B)    Utf8EncodeCP(A, B)
#define nbcore_utf8encodeW(A)        Utf8EncodeW(A)
#define nbcore_utf8lenW(A)           Ucs2toUtf8Len(A)

__forceinline char *nbcore_utf8decodeA(const char *src)
{
  char *tmp = nbcore_strdup(src);
  nbcore_utf8decode(tmp, nullptr);
  return tmp;
}

///////////////////////////////////////////////////////////////////////////////
// Windows utilities

NB_CORE_DLL(BOOL) IsWinVerVistaPlus();
NB_CORE_DLL(BOOL) IsWinVer7Plus();
NB_CORE_DLL(BOOL) IsWinVer8Plus();
NB_CORE_DLL(BOOL) IsWinVer81Plus();
NB_CORE_DLL(BOOL) IsWinVer10Plus();

NB_CORE_DLL(BOOL) IsFullScreen();
NB_CORE_DLL(BOOL) IsWorkstationLocked();
NB_CORE_DLL(BOOL) IsScreenSaverRunning();
NB_CORE_DLL(BOOL) IsTerminalDisconnected();

NB_CORE_DLL(BOOL) GetOSDisplayString(wchar_t *buf, size_t bufSize);

/////////////////////////////////////////////////////////////////////////////////////////
// retrieves the hLangpack of a plugin by its HINSTANCE
// returns hLangpack if found, or 0 if error occurred

NB_CORE_DLL(int) GetPluginLangByInstance(HINSTANCE);

///////////////////////////////////////////////////////////////////////////////

NB_CORE_DLL(void) UnloadCoreModule();

#if defined(__cplusplus)
} // extern "C"

template <size_t _Size>
inline int nbcore_snprintf(char(&buffer)[_Size], const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  const int ret = nbcore_vsnprintf(buffer, _Size, fmt, args);
  va_end(args);
  return ret;
}

template <size_t _Size>
inline int nbcore_snwprintf(wchar_t(&buffer)[_Size], const wchar_t *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  const int ret = nbcore_vsnwprintf(buffer, _Size, fmt, args);
  va_end(args);
  return ret;
}

template <size_t _Size>
inline int nbcore_vsnprintf(char(&buffer)[_Size], const char *fmt, va_list va)
{
  return nbcore_vsnprintf(buffer, _Size, fmt, va);
}

template <size_t _Size>
inline int nbcore_vsnwprintf(wchar_t(&buffer)[_Size], const wchar_t *fmt, va_list va)
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
