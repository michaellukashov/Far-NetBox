#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////////////////
// CMBaseString

class CNilMStringData : public CMStringData
{
public:
  CNilMStringData();

public:
  wchar_t achNil[2];
};

CNilMStringData::CNilMStringData()
{
  nRefs = 2; // Never gets freed
  nDataLength = 0;
  nAllocLength = 0;
  achNil[0] = 0;
  achNil[1] = 0;
}

static CNilMStringData* m_nil = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////
// CMBaseString

NB_CORE_DLL(CMStringData*) nbstr_allocate(int nChars, int nCharSize)
{
  nChars++; // nil char
  size_t nDataBytes = nCharSize * nChars;
  size_t nTotalSize = nDataBytes + sizeof(CMStringData);

  CMStringData* pData = static_cast<CMStringData*>(nbcore_alloc(nTotalSize));
  if (pData == nullptr)
    return nullptr;

  pData->nRefs = 1;
  pData->nAllocLength = nChars - 1;
  pData->nDataLength = 0;
  return pData;
}

NB_CORE_DLL(void) nbstr_free(CMStringData* pData)
{
  nbcore_free(pData);
}

NB_CORE_DLL(CMStringData*) nbstr_realloc(CMStringData* pData, int nChars, int nCharSize)
{
  nChars++; // nil char
  ULONG nDataBytes = nCharSize * nChars;
  ULONG nTotalSize = nDataBytes + sizeof(CMStringData);

  CMStringData* pNewData = static_cast<CMStringData*>(nbcore_realloc(pData, nTotalSize));
  if (pNewData == nullptr)
    return nullptr;

  pNewData->nAllocLength = nChars - 1;
  return pNewData;
}

NB_CORE_DLL(CMStringData*) nbstr_getNil()
{
  if (m_nil == nullptr)
    m_nil = new CNilMStringData();
  m_nil->AddRef();
  return m_nil;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CMStringData

NB_CORE_DLL(void) nbstr_lock(CMStringData* pThis)
{
  pThis->nRefs--; // Locked buffers can't be shared, so no interlocked operation necessary
  if (pThis->nRefs == 0)
    pThis->nRefs = -1;
}

NB_CORE_DLL(void) nbstr_release(CMStringData* pThis)
{
  if (InterlockedDecrement(&pThis->nRefs) <= 0)
    nbstr_free(pThis);
}

NB_CORE_DLL(void) nbstr_unlock(CMStringData* pThis)
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

template CMStringW;
template NB_CORE_EXPORT CMStringW CALLBACK operator +(const CMStringW& str1, const CMStringW& str2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW& str1, const wchar_t* psz2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const wchar_t* psz1, const CMStringW& str2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW& str1, wchar_t ch2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW& str1, char ch2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(wchar_t ch1, const CMStringW& str2);
template NB_CORE_EXPORT CMStringW CALLBACK operator+(char ch1, const CMStringW& str2);

template CMStringA;
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, const CMStringA& str2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, const char* psz2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const char* psz1, const CMStringA& str2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, wchar_t ch2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, char ch2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(wchar_t ch1, const CMStringA& str2);
template NB_CORE_EXPORT CMStringA CALLBACK operator+(char ch1, const CMStringA& str2);
