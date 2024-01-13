
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <nbutils.h>
#include <Exceptions.h>
#include <FileBuffer.h>
#include <Windows.hpp>
#include <Math.hpp>
#include "FileInfo.h"

// #pragma package(smart_init)

#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )
struct VS_VERSION_INFO_STRUCT32
{
  WORD wLength;
  WORD wValueLength;
  WORD wType;
  WCHAR szKey[1];
};

static uint32_t VERSION_GetFileVersionInfo_PE(const wchar_t * FileName, uint32_t DataSize, void * Data)
{
  uint32_t Len = 0;

  bool NeedFree = false;
  HMODULE Module = ::GetModuleHandle(FileName);
  if (Module == nullptr)
  {
    Module = ::LoadLibraryEx(FileName, nullptr, LOAD_LIBRARY_AS_DATAFILE);
    NeedFree = true;
  }
  if (Module == nullptr)
  {
    Len = 0;
  }
  else
  {
    try__finally
    {
      const HRSRC Rsrc = ::FindResource(Module, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
      if (Rsrc != nullptr)
      {
        Len = ::SizeofResource(Module, static_cast<HRSRC>(Rsrc));
        const HANDLE Mem = ::LoadResource(Module, static_cast<HRSRC>(Rsrc));
        if (Mem != nullptr)
        {
          try__finally
          {
            VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(LockResource(Mem));
            const VS_FIXEDFILEINFO * FixedInfo =
              reinterpret_cast<VS_FIXEDFILEINFO *>(DWORD_ALIGN(VersionInfo, VersionInfo->szKey + nb::StrLength(VersionInfo->szKey) + 1));

            if (FixedInfo->dwSignature != VS_FFI_SIGNATURE)
            {
              Len = 0;
            }
            else
            {
              if (Data != nullptr)
              {
                if (DataSize < Len)
                {
                  Len = DataSize;
                }
                if (Len > 0)
                {
                  memmove(Data, VersionInfo, Len);
                }
              }
            }
          }
          __finally
          {
            ::FreeResource(Mem);
          } end_try__finally
        }
      }
    }
    __finally
    {
      if (NeedFree)
      {
        ::FreeLibrary(Module);
      }
    } end_try__finally
  }

  return Len;
}

static uint32_t GetFileVersionInfoSizeFix(const wchar_t * FileName, DWORD * AHandle)
{
  uint32_t Len;
  if (IsWin7())
  {
    *AHandle = 0;
    Len = VERSION_GetFileVersionInfo_PE(FileName, 0, nullptr);

    if (Len != 0)
    {
      Len = (Len * 2) + 4;
    }
  }
  else
  {
    Len = ::GetFileVersionInfoSize(const_cast<wchar_t *>(FileName), AHandle);
  }

  return Len;
}

bool GetFileVersionInfoFix(const wchar_t * FileName, DWORD Handle,
  uint32_t DataSize, void * Data)
{
  bool Result;

  if (IsWin7())
  {
    const VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(Data);

    const uint32_t Len = VERSION_GetFileVersionInfo_PE(FileName, DataSize, Data);

    Result = (Len != 0);
    if (Result)
    {
      constexpr const char Signature[] = "FE2X";
      const uint32_t BufSize = nb::ToUInt32(VersionInfo->wLength + NBChTraitsCRT<char>::SafeStringLen(Signature));

      if (DataSize >= BufSize)
      {
        const uint32_t ConvBuf = DataSize - VersionInfo->wLength;
        memmove((static_cast<char *>(Data)) + VersionInfo->wLength, Signature, ConvBuf > 4 ? 4 : ConvBuf);
      }
    }
  }
  else
  {
    Result = ::GetFileVersionInfo(FileName, Handle, nb::ToDWord(DataSize), Data) != FALSE;
  }

  return Result;
}

// Return pointer to file version info block
void * CreateFileInfo(const UnicodeString & AFileName)
{
  DWORD Handle;
  void * Result = nullptr;
  // Get file version info block size
  const uint32_t Size = GetFileVersionInfoSizeFix(AFileName.c_str(), &Handle);
  // If size is valid
  if (Size > 0)
  {
    Result = nb::calloc<void *>(Size, 1);
    // Get file version info block
    if (!GetFileVersionInfoFix(AFileName.c_str(), Handle, Size, Result))
    {
      nb_free(Result);
      Result = nullptr;
    }
  }
  return Result;
}

// Free file version info block memory
void FreeFileInfo(const void * FileInfo)
{
  if (FileInfo)
    nb_free(FileInfo);
}

using TTranslations = TTranslation[65536];
using PTranslations = TTranslation *;

// Return pointer to fixed file version info
PVSFixedFileInfo GetFixedFileInfo(const void * FileInfo)
{
  UINT Len;
  PVSFixedFileInfo Result = nullptr;
  if (FileInfo && !::VerQueryValue(FileInfo, L"\\", reinterpret_cast<void **>(&Result), &Len))
  {
#ifdef NDEBUG
    throw Exception("Fixed file info not available");
#endif
  }
  return Result;
}

// Return number of available file version info translations
uint32_t GetTranslationCount(const void * FileInfo)
{
  PTranslations P{nullptr};
  UINT Len{0};
  if (FileInfo && !::VerQueryValue(FileInfo, L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&P), &Len))
  {
    throw Exception("File info translations not available");
  }
  return Len / 4;
}

// Return i-th translation in the file version info translation list
TTranslation GetTranslation(const void * FileInfo, uint32_t I)
{
  PTranslations P = nullptr;
  UINT Len{0};

  if (FileInfo && !::VerQueryValue(FileInfo, L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&P), &Len))
  {
    throw Exception("File info translations not available");
  }
  if (I * sizeof(TTranslation) >= Len)
  {
    throw Exception("Specified translation not available");
  }
  return P[I];
}

// Return the name of the specified language
UnicodeString GetLanguage(Word Language)
{
  wchar_t P[256]{};

  const uint32_t Len = ::VerLanguageName(Language, P, _countof(P));
  if (Len > _countof(P))
  {
    throw Exception("Language not available");
  }
  return UnicodeString(P, nb::ToInt32(Len));
}

// Return the value of the specified file version info string using the
// specified translation
UnicodeString GetFileInfoString(const void * FileInfo,
  TTranslation Translation, const UnicodeString & StringName, bool AllowEmpty)
{
  UnicodeString Result;
  wchar_t * P{nullptr};
  UINT Len{0};

  const UnicodeString SubBlock =
    UnicodeString(L"\\StringFileInfo\\") + IntToHex(Translation.Language, 4) + IntToHex(Translation.CharSet, 4) + L"\\" + StringName;
  if (!VerQueryValue(FileInfo, SubBlock.c_str(), reinterpret_cast<void**>(&P), &Len))
  {
    if (!AllowEmpty)
    {
      throw Exception("Specified file info string not available");
    }
  }
  else
  {
    Result = UnicodeString(P, nb::ToInt32(Len));
    PackStr(Result);
  }
  return Result;
}

int32_t CalculateCompoundVersion(int32_t MajorVer, int32_t MinorVer, int32_t Release)
{
  const int32_t CompoundVer = 10000 * (Release + 100 * (MinorVer + 100 * MajorVer));
  return CompoundVer;
}

int32_t ZeroBuildNumber(int32_t CompoundVersion)
{
  return (CompoundVersion / 10000 * 10000);
}

int32_t StrToCompoundVersion(const UnicodeString & AStr)
{
  UnicodeString Str = AStr;
  const int32_t MajorVer = nb::Min(StrToIntPtr(CutToChar(Str, L'.', false)), nb::ToInt32(99));
  const int32_t MinorVer = nb::Min(StrToIntPtr(CutToChar(Str, L'.', false)), nb::ToInt32(99));
  const int32_t Release = Str.IsEmpty() ? 0 : nb::Min(StrToIntPtr(CutToChar(Str, L'.', false)), nb::ToInt32(99));
  return CalculateCompoundVersion(MajorVer, MinorVer, Release);
}

int32_t CompareVersion(const UnicodeString & V1, const UnicodeString & V2)
{
  int32_t Result = 0;
  UnicodeString _V1(V1), _V2(V2);
  while ((Result == 0) && (!_V1.IsEmpty() || !_V2.IsEmpty()))
  {
    const int32_t C1 = StrToIntDef(CutToChar(_V1, L'.', false), 0);
    const int32_t C2 = StrToIntDef(CutToChar(_V2, L'.', false), 0);
    // Result = CompareValue(C1, C2);
    if (C1 < C2)
    {
      Result = -1;
    }
    else if (C1 > C2)
    {
      Result = 1;
    }
    else
      Result = 0;
  }
  return Result;
}
