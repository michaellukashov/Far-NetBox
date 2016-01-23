#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include <FileBuffer.h>
#include <Windows.hpp>

#include "FileInfo.h"

#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )
struct VS_VERSION_INFO_STRUCT32
{
  WORD  wLength;
  WORD  wValueLength;
  WORD  wType;
  WCHAR szKey[1];
};

static uintptr_t VERSION_GetFileVersionInfo_PE(const wchar_t * FileName, uintptr_t DataSize, void * Data)
{
  uintptr_t Len = 0;

  bool NeedFree = false;
  HMODULE Module = ::GetModuleHandle(FileName);
  if (Module == nullptr)
  {
    Module = ::LoadLibraryEx(FileName, 0, LOAD_LIBRARY_AS_DATAFILE);
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
      SCOPE_EXIT
      {
        if (NeedFree)
        {
          ::FreeLibrary(Module);
        }
      };
      HRSRC Rsrc = ::FindResource(Module, MAKEINTRESOURCE(VS_VERSION_INFO),
        MAKEINTRESOURCE(VS_FILE_INFO));
      if (Rsrc == nullptr)
      {
      }
      else
      {
        Len = ::SizeofResource(Module, static_cast<HRSRC>(Rsrc));
        HANDLE Mem = ::LoadResource(Module, static_cast<HRSRC>(Rsrc));
        if (Mem == nullptr)
        {
        }
        else
        {
          try__finally
          {
            SCOPE_EXIT
            {
              ::FreeResource(Mem);
            };
            VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(LockResource(Mem));
            const VS_FIXEDFILEINFO * FixedInfo =
              (VS_FIXEDFILEINFO *)DWORD_ALIGN(VersionInfo, VersionInfo->szKey + wcslen(VersionInfo->szKey) + 1);

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
            FreeResource(Mem);
          };
        }
      }
    }
    __finally
    {
      if (NeedFree)
      {
        FreeLibrary(Module);
      }
    };
  }

  return Len;
}

static uintptr_t GetFileVersionInfoSizeFix(const wchar_t * FileName, DWORD * AHandle)
{
  uintptr_t Len;
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

bool GetFileVersionInfoFix(const wchar_t * FileName, uint32_t Handle,
  uintptr_t DataSize, void * Data)
{
  bool Result;

  if (IsWin7())
  {
    VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(Data);

    uintptr_t Len = VERSION_GetFileVersionInfo_PE(FileName, DataSize, Data);

    Result = (Len != 0);
    if (Result)
    {
      static const char Signature[] = "FE2X";
      uintptr_t BufSize = static_cast<uintptr_t>(VersionInfo->wLength + strlen(Signature));

      if (DataSize >= BufSize)
      {
        uintptr_t ConvBuf = DataSize - VersionInfo->wLength;
        memmove((static_cast<char *>(Data)) + VersionInfo->wLength, Signature, ConvBuf > 4 ? 4 : ConvBuf );
      }
    }
  }
  else
  {
    Result = ::GetFileVersionInfo(FileName, Handle, static_cast<DWORD>(DataSize), Data) != 0;
  }

  return Result;
}

// Return pointer to file version info block
void * CreateFileInfo(const UnicodeString & AFileName)
{
  DWORD Handle;
  uintptr_t Size;
  void * Result = nullptr;

  // Get file version info block size
  Size = GetFileVersionInfoSizeFix(AFileName.c_str(), &Handle);
  // If size is valid
  if (Size > 0)
  {
    Result = nb_malloc(Size);
    // Get file version info block
    if (!GetFileVersionInfoFix(AFileName.c_str(), Handle, Size, Result))
    {
      nb_free(Result);
      Result = nullptr;
    }
  }
  else
  {
  }
  return Result;
}

// Free file version info block memory
void FreeFileInfo(void * FileInfo)
{
  if (FileInfo)
    nb_free(FileInfo);
}

typedef TTranslation TTranslations[65536];
typedef TTranslation *PTranslations;

// Return pointer to fixed file version info
PVSFixedFileInfo GetFixedFileInfo(void * FileInfo)
{
  UINT Len;
  PVSFixedFileInfo Result = nullptr;
  if (FileInfo && !::VerQueryValue(FileInfo, L"\\", reinterpret_cast<void **>(&Result), &Len))
  {
    throw Exception(L"Fixed file info not available");
  }
  return Result;
}

// Return number of available file version info translations
uint32_t GetTranslationCount(void * FileInfo)
{
  PTranslations P;
  UINT Len;
  if (!::VerQueryValue(FileInfo, L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&P), &Len))
    throw Exception(L"File info translations not available");
  return Len / 4;
}

// Return i-th translation in the file version info translation list
TTranslation GetTranslation(void * FileInfo, intptr_t I)
{
  PTranslations P = nullptr;
  UINT Len;

  if (!::VerQueryValue(FileInfo, L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&P), &Len))
    throw Exception(L"File info translations not available");
  if (I * sizeof(TTranslation) >= Len)
    throw Exception(L"Specified translation not available");
  return P[I];
}

// Return the name of the specified language
UnicodeString GetLanguage(Word Language)
{
  uintptr_t Len;
  wchar_t P[256];

  Len = ::VerLanguageName(Language, P, _countof(P));
  if (Len > _countof(P))
    throw Exception(L"Language not available");
  return UnicodeString(P, Len);
}

// Return the value of the specified file version info string using the
// specified translation
UnicodeString GetFileInfoString(void * FileInfo,
  TTranslation Translation, const UnicodeString & StringName, bool AllowEmpty)
{
  UnicodeString Result;
  wchar_t * P;
  UINT Len;

  if (!::VerQueryValue(FileInfo, (UnicodeString(L"\\StringFileInfo\\") +
    ::IntToHex(Translation.Language, 4) +
    ::IntToHex(Translation.CharSet, 4) +
    L"\\" + StringName).c_str(), reinterpret_cast<void **>(&P), &Len))
  {
    if (!AllowEmpty)
    {
      throw Exception(L"Specified file info string not available");
    }
  }
  else
  {
    Result = UnicodeString(P, Len);
    PackStr(Result);
  }
  return Result;
}

int CalculateCompoundVersion(int MajorVer,
  int MinorVer, int Release, int Build)
{
  int CompoundVer = Build + 10000 * (Release + 100 * (MinorVer +
    100 * MajorVer));
  return CompoundVer;
}

