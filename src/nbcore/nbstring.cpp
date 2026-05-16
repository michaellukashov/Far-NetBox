#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////////////////
// CMBaseString

class CNilMStringData : public CMStringData
{
public:
  CNilMStringData();

public:
  wchar_t achNil[2]{};
};

CNilMStringData::CNilMStringData() : CMStringData()
{
  nRefs = 2; // Never gets freed
  nDataLength = 0;
  nAllocLength = 0;
  achNil[0] = 0;
  achNil[1] = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CMBaseString

NB_CORE_DLL(CMStringData *) nbstr_allocate(int nChars, int nCharSize)
{
  nChars++; // nil char
  const size_t nDataBytes = nCharSize * nChars;
  const size_t nTotalSize = nDataBytes + sizeof(CMStringData);

  CMStringData *pData = static_cast<CMStringData *>(nbcore_alloc(nTotalSize));
  if (pData == nullptr)
    return nullptr;

  pData->nRefs = 1;
  pData->nAllocLength = nChars - 1;
  pData->nDataLength = 0;
  return pData;
}

NB_CORE_DLL(void) nbstr_free(CMStringData *pData)
{
  nbcore_free(pData);
}

NB_CORE_DLL(CMStringData *) nbstr_realloc(CMStringData *pData, int nChars, int nCharSize)
{
  nChars++; // nil char
  const ULONG nDataBytes = nCharSize * nChars;
  const ULONG nTotalSize = nDataBytes + sizeof(CMStringData);

  CMStringData *pNewData = static_cast<CMStringData *>(nbcore_realloc(pData, nTotalSize));
  if (pNewData == nullptr)
    return nullptr;

  pNewData->nAllocLength = nChars - 1;
  return pNewData;
}

NB_CORE_DLL(void *) nbstr_memcpy(void *pDst, void const *pSrc, size_t nSize)
{
  memmove(pDst, pSrc, nSize);
  return pDst;
}

NB_CORE_DLL(CMStringData *) nbstr_getNil()
{
  // C++11 guarantees thread-safe initialization of function-local statics.
  static CNilMStringData * nil = new CNilMStringData();
  nil->AddRef();
  return nil;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CMStringData

// nbstr_lock / nbstr_unlock intentionally avoid Interlocked ops because a locked
// buffer is never shared across threads (IsShared() == false). The caller must
// ensure no concurrent AddRef/Release while the buffer is locked.
NB_CORE_DLL(void) nbstr_lock(CMStringData *pThis)
{
  pThis->nRefs--; // Locked buffers can't be shared, so no interlocked operation necessary
  if (pThis->nRefs == 0)
    pThis->nRefs = -1;
}

NB_CORE_DLL(void) nbstr_release(CMStringData *pThis)
{
  if (InterlockedDecrement(&pThis->nRefs) <= 0)
    nbstr_free(pThis);
}

NB_CORE_DLL(void) nbstr_unlock(CMStringData *pThis)
{
  if (pThis->IsLocked())
  {
    pThis->nRefs++; // Locked buffers can't be shared, so no interlocked operation necessary
    if (pThis->nRefs == 0)
      pThis->nRefs = 1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// don't remove it
// this code just instantiates templates for CMStringW[A/W]

#if !defined(__MINGW32__)
template CMStringW;
#endif // defined(__MINGW32__)
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW &str1, const CMStringW &str2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW &str1, const wchar_t *psz2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const wchar_t *psz1, const CMStringW &str2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW &str1, wchar_t ch2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW &str1, char ch2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(wchar_t ch1, const CMStringW &str2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(char ch1, const CMStringW &str2);

#if !defined(__MINGW32__)
template CMStringA;
#endif // defined(__MINGW32__)
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA &str1, const CMStringA &str2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA &str1, const char *psz2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const char *psz1, const CMStringA &str2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA &str1, wchar_t ch2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA &str1, char ch2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(wchar_t ch1, const CMStringA &str2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(char ch1, const CMStringA &str2);
