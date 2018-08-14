#pragma once

#include <nbcore.h>

#if defined(__cplusplus)

///////////////////////////////////////////////////////////////////////////////
// nb_ptr - automatic pointer for buffers, allocated using nbcore_alloc/nbcore_calloc

template<class T> class nb_ptr
{
protected:
  T *data;

public:
  __inline explicit nb_ptr() noexcept : data(nullptr) {}
  __inline explicit nb_ptr(T *_p) noexcept : data(_p) {}
  __inline ~nb_ptr() { nbcore_free(data); }
  __inline T *get() const { return data; }
  __inline T *operator = (T *_p) { if (data) nbcore_free(data); data = _p; return data; }
  __inline T *operator->() const { return data; }
  __inline operator T *() const { return data; }
  __inline operator intptr_t() const { return (intptr_t)data; }
  __inline T *detach() { T *res = data; data = nullptr; return res; }
};

typedef nb_ptr<char>  ptrA;
typedef nb_ptr<wchar_t> ptrW;

///////////////////////////////////////////////////////////////////////////////
// nb_cs - simple wrapper for the critical sections

class nb_cs
{
  CRITICAL_SECTION m_cs;

public:
  __inline nb_cs() noexcept { ::InitializeCriticalSection(&m_cs); }
  __inline ~nb_cs() noexcept { ::DeleteCriticalSection(&m_cs); }

  __inline operator CRITICAL_SECTION &() { return m_cs; }
};

///////////////////////////////////////////////////////////////////////////////
// nb_cslock - simple locker for the critical sections

class nb_cslock
{
  CRITICAL_SECTION &cs;
  __inline nb_cslock &operator = (const nb_cslock &) { return *this; }

public:
  __inline nb_cslock(CRITICAL_SECTION &_cs) : cs(_cs) { ::EnterCriticalSection(&cs); }
  __inline ~nb_cslock() { ::LeaveCriticalSection(&cs); }
};

//////////////////////////////////////////////////////////////////////////////
//pass_ptrA, pass_ptrW and pass_ptrT - automatic pointer for passwords

class pass_ptrA : public nb_ptr<char>
{
public:
  __inline explicit pass_ptrA() noexcept : nb_ptr() {}
  __inline explicit pass_ptrA(char *_p) noexcept : nb_ptr(_p) {}
  __inline ~pass_ptrA() { zero(); }
  __inline char *operator = (char *_p) { zero(); return nb_ptr::operator=(_p); }
  __inline void zero()
  {
    if (data) SecureZeroMemory(data, nbcore_strlen(data));
  }
};

class pass_ptrW : public nb_ptr<wchar_t>
{
public:
  __inline explicit pass_ptrW() noexcept : nb_ptr() {}
  __inline explicit pass_ptrW(wchar_t *_p) noexcept : nb_ptr(_p) {}
  __inline ~pass_ptrW() { zero(); }
  __inline wchar_t *operator = (wchar_t *_p) { zero(); return nb_ptr::operator=(_p); }
  __inline void zero()
  {
    if (data) SecureZeroMemory(data, nbcore_wstrlen(data)*sizeof(wchar_t));
  }
};

///////////////////////////////////////////////////////////////////////////////
// nb_cslockfull - controllable locker for the critical sections

class nb_cslockfull
{
  CRITICAL_SECTION &cs;
  bool bIsLocked{false};
  __inline nb_cslockfull &operator = (const nb_cslockfull &) = delete;
  __inline nb_cslockfull(const nb_cslockfull &) = delete;

public:
  __inline void lock() { bIsLocked = true; EnterCriticalSection(&cs); }
  __inline void unlock() { bIsLocked = false; LeaveCriticalSection(&cs); }

  __inline nb_cslockfull(CRITICAL_SECTION &_cs) : cs(_cs) { lock(); }
  __inline ~nb_cslockfull() { if (bIsLocked) unlock(); }
};

///////////////////////////////////////////////////////////////////////////////
// basic class for classes that should be cleared inside new()

class MZeroedObject
{
public:
  __inline void *operator new(size_t size)
  {
    return nbcore_calloc(size);
  }
  __inline void operator delete(void *p)
  {
    nbcore_free(p);
  }
};

///////////////////////////////////////////////////////////////////////////////
// general lists' templates

#define  NumericKeySortT -1
#define  HandleKeySortT  -2
#define  PtrKeySortT     -3

template<class T> struct LIST
{
  typedef int (*FTSortFunc)(const T *p1, const T *p2);

  __inline LIST(int aincr, FTSortFunc afunc = nullptr)
  {
    memset(this, 0, sizeof(*this));
    increment = aincr;
    sortFunc = afunc;
  }

  __inline LIST(int aincr, intptr_t id)
  {
    memset(this, 0, sizeof(*this));
    increment = aincr;
    sortFunc = FTSortFunc(id);
  }

  __inline LIST(const LIST &x)
  {
    items = nullptr;
    List_Copy((SortedList *)&x, (SortedList *)this, sizeof(T));
  }

  __inline LIST &operator = (const LIST &x)
  {
    destroy();
    List_Copy((SortedList *)&x, (SortedList *)this, sizeof(T));
    return *this;
  }

  __inline ~LIST()
  {
    destroy();
  }

  __inline T  *operator[](int idx) const { return (idx >= 0 && idx < count) ? items[idx] : nullptr; }
  __inline int getCount(void)     const { return count; }
  __inline T **getArray(void)     const { return items; }

  __inline int getIndex(T *p) const
  {
    int idx;
    return (!List_GetIndex((SortedList *)this, p, &idx)) ? -1 : idx;
  }

  __inline void destroy(void)         { List_Destroy((SortedList *)this); }
  __inline T   *find(T *p) const      { return (T *)List_Find((SortedList *)this, p); }
  __inline int  indexOf(T *p) const   { return List_IndexOf((SortedList *)this, p); }
  __inline int  insert(T *p, int idx) { return List_Insert((SortedList *)this, p, idx); }
  __inline int  remove(int idx)       { return List_Remove((SortedList *)this, idx); }

  __inline int  insert(T *p)          { return List_InsertPtr((SortedList *)this, p); }
  __inline int  remove(T *p)          { return List_RemovePtr((SortedList *)this, p); }

  __inline void put(int idx, T *p)   { items[idx] = p; }

protected:
  T        **items;
  int        count, limit, increment;
  FTSortFunc sortFunc;
};

template<class T> struct OBJLIST : public LIST<T>
{
  typedef int (*FTSortFunc)(const T *p1, const T *p2);

  __inline OBJLIST(int aincr, FTSortFunc afunc = nullptr) :
    LIST<T>(aincr, afunc)
  {}

  __inline OBJLIST(int aincr, intptr_t id) :
    LIST<T>(aincr, (FTSortFunc) id)
  {}

  __inline OBJLIST(const OBJLIST &x) :
    LIST<T>(x.increment, x.sortFunc)
  {
    this->items = nullptr;
    List_ObjCopy((SortedList *)&x, (SortedList *)this, sizeof(T));
  }

  __inline OBJLIST &operator = (const OBJLIST &x)
  {
    destroy();
    List_ObjCopy((SortedList *)&x, (SortedList *)this, sizeof(T));
    return *this;
  }

  ~OBJLIST()
  {
    destroy();
  }

  __inline void destroy(void)
  {
    for (int i = 0; i < this->count; i++)
      delete this->items[i];

    List_Destroy((SortedList *)this);
  }

  __inline int remove(int idx)
  {
    delete this->items[idx];
    return List_Remove((SortedList *)this, idx);
  }

  __inline int remove(T *p)
  {
    int i = getIndex(p);
    if (i != -1)
    {
      remove(i);
      return 1;
    }
    return 0;
  }

  __inline T &operator[](int idx) const { return *this->items[idx]; }
};

#define __A2W(s) L ## s
#define _A2W(s) __A2W(s)

class _A2T : public ptrW
{
public:
  __inline _A2T(const char *s) : ptrW(nbcore_a2u(s)) {}
  __inline _A2T(const char *s, int cp) : ptrW(nbcore_a2u_cp(s, cp)) {}
};

class _T2A : public ptrA
{
public:
  __forceinline _T2A(const wchar_t *s) : ptrA(nbcore_u2a(s)) {}
  __forceinline _T2A(const wchar_t *s, int cp) : ptrA(nbcore_u2a_cp(s, cp)) {}
};

class T2Utf : public ptrA
{
public:
  __forceinline T2Utf(const wchar_t *str) : ptrA(nbcore_utf8encodeW(str)) {}
  __forceinline operator BYTE *() const { return (BYTE *)data; }
#ifdef _XSTRING_
  std::string str() const { return std::string(data); }
#endif
};

///////////////////////////////////////////////////////////////////////////////
// basic class for classes that should be cleared inside new()

class NB_CORE_EXPORT MBinBuffer
{
  char *m_buf;
  size_t m_len;

public:
  MBinBuffer();
  ~MBinBuffer();

  __forceinline char  *data() const { return m_buf; }
  __forceinline bool   isEmpty() const { return m_len == 0; }
  __forceinline size_t length() const { return m_len; }

  // adds a buffer to the end
  void append(void *pBuf, size_t bufLen);

  // adds a buffer to the beginning
  void appendBefore(void *pBuf, size_t bufLen);

  // drops a part of buffer
  void remove(size_t sz);
};

///////////////////////////////////////////////////////////////////////////////
// parameter classes for XML, JSON & HTTP requests

struct PARAM
{
  const char *szName;
  __forceinline PARAM(const char *_name) : szName(_name)
  {}
};

struct BOOL_PARAM : public PARAM
{
  bool bValue;
  __forceinline BOOL_PARAM(const char *_name, bool _value) :
    PARAM(_name), bValue(_value)
  {}
};

struct INT_PARAM : public PARAM
{
  int32_t iValue;
  __forceinline INT_PARAM(const char *_name, int32_t _value) :
    PARAM(_name), iValue(_value)
  {}
};

struct INT64_PARAM : public PARAM
{
  int64_t iValue;
  __forceinline INT64_PARAM(const char *_name, int64_t _value) :
    PARAM(_name), iValue(_value)
  {}
};

struct CHAR_PARAM : public PARAM
{
  const char *szValue;
  __forceinline CHAR_PARAM(const char *_name, const char *_value) :
    PARAM(_name), szValue(_value)
  {}
};

struct WCHAR_PARAM : public PARAM
{
  const wchar_t *wszValue;
  __forceinline WCHAR_PARAM(const char *_name, const wchar_t *_value) :
    PARAM(_name), wszValue(_value)
  {}
};

#endif // #if defined(__cplusplus)

