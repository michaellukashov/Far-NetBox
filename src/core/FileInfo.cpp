//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

//!CLEANBEGIN
#undef TRACE_FILE_APPL_INFO
#define TRACE_FILE_APPL_INFO
//!CLEANEND

#include <Common.h>
#include <Exceptions.h>
#include <Windows.hpp>
#include "FileInfo.h"
#include "FileBuffer.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define DWORD_ALIGN( base, ptr ) \
    ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )
struct VS_VERSION_INFO_STRUCT32
{
  WORD  wLength;
  WORD  wValueLength;
  WORD  wType;
  WCHAR szKey[1];
};
//---------------------------------------------------------------------------
unsigned int VERSION_GetFileVersionInfo_PE(const wchar_t * FileName, unsigned int DataSize, void * Data)
{
  CALLSTACK;
  unsigned int Len = 0;

  TRACEFMT("[%s]", FileName);
  bool NeedFree = false;
  HMODULE Module = GetModuleHandle(FileName);
  TRACE("0");
  if (Module == NULL)
  {
    TRACE("1");
    Module = LoadLibraryEx(FileName, 0, LOAD_LIBRARY_AS_DATAFILE);
    NeedFree = true;
  }
  if (Module == NULL)
  {
    TRACEFMT("Could not load %s", FileName);
  }
  else
  {
    TRY_FINALLY (
    {
      TRACE("2");
      HRSRC Rsrc = FindResource(Module, MAKEINTRESOURCE(VS_VERSION_INFO),
        MAKEINTRESOURCE(VS_FILE_INFO));
      if (Rsrc == NULL)
      {
        TRACEFMT("Could not find VS_VERSION_INFO in %s", FileName);
      }
      else
      {
        TRACE("3");
        Len = SizeofResource(Module, static_cast<HRSRC>(Rsrc));
        HANDLE Mem = LoadResource(Module, static_cast<HRSRC>(Rsrc));
        if (Mem == NULL)
        {
          TRACEFMT("Could not load VS_VERSION_INFO from %s", FileName);
        }
        else
        {
          TRACE("4");
          TRY_FINALLY (
          {
            VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(LockResource(Mem));
            const VS_FIXEDFILEINFO * FixedInfo =
              (VS_FIXEDFILEINFO *)DWORD_ALIGN(VersionInfo, VersionInfo->szKey + wcslen(VersionInfo->szKey) + 1);

            if (FixedInfo->dwSignature != VS_FFI_SIGNATURE)
            {
              TRACEFMT("vffi->dwSignature is %x, but not %x!\n",  int(FixedInfo->dwSignature), int(VS_FFI_SIGNATURE));
              Len = 0;
            }
            else
            {
              TRACE("5");
              if (Data != NULL)
              {
                TRACE("6");
                if (DataSize < Len)
                {
                  TRACE("7");
                  Len = DataSize;
                }
                if (Len > 0)
                {
                  TRACE("8");
                  memmove(Data, VersionInfo, Len);
                }
              }
            }
          }
          ,
          {
            TRACE("9");
            FreeResource(Mem);
          }
          );
        }
      }
    }
    ,
    {
      TRACE("10");
      if (NeedFree)
      {
        FreeLibrary(Module);
      }
    }
    );
  }

  TRACE("/");
  return Len;
}
//---------------------------------------------------------------------------
unsigned int GetFileVersionInfoSizeFix(const wchar_t * FileName, unsigned long * Handle)
{
  unsigned int Len;
  if (IsWin7())
  {
    *Handle = 0;
    Len = VERSION_GetFileVersionInfo_PE(FileName, 0, NULL);

    if (Len != 0)
    {
      Len = (Len * 2) + 4;
    }
  }
  else
  {
    Len = GetFileVersionInfoSize(const_cast<wchar_t *>(FileName), Handle);
  }

  return Len;
}
//---------------------------------------------------------------------------
bool GetFileVersionInfoFix(const wchar_t * FileName, unsigned long Handle,
  unsigned int DataSize, void * Data)
{
  CALLSTACK;
  bool Result;

  if (IsWin7())
  {
    VS_VERSION_INFO_STRUCT32 * VersionInfo = static_cast<VS_VERSION_INFO_STRUCT32 *>(Data);

    unsigned int Len = VERSION_GetFileVersionInfo_PE(FileName, DataSize, Data);

    Result = (Len != 0);
    if (Result)
    {
      static const char Signature[] = "FE2X";
      unsigned int BufSize = (unsigned int)(VersionInfo->wLength + strlen(Signature));

      if (DataSize >= BufSize)
      {
        unsigned int ConvBuf = DataSize - VersionInfo->wLength;
        memmove((static_cast<char*>(Data)) + VersionInfo->wLength, Signature, ConvBuf > 4 ? 4 : ConvBuf );
      }
    }
  }
  else
  {
    Result = GetFileVersionInfo(const_cast<wchar_t *>(FileName), Handle, DataSize, Data) != 0;
  }

  return Result;
}
//---------------------------------------------------------------------------
// Return pointer to file version info block
void * CreateFileInfo(UnicodeString FileName)
{
  CALLSTACK;
  unsigned long Handle;
  unsigned int Size;
  void * Result = NULL;

  TRACEFMT("CreateFileInfo 1 [%s]", FileName.c_str());

  // Get file version info block size
  Size = GetFileVersionInfoSizeFix(FileName.c_str(), &Handle);
  // If size is valid
  if (Size > 0)
  {
    TRACE("CreateFileInfo 2");
    Result = new char[Size];
    // Get file version info block
    TRACE("CreateFileInfo 3");
    if (!GetFileVersionInfoFix(FileName.c_str(), Handle, Size, Result))
    {
      TRACE("CreateFileInfo 4");
      delete[] Result;
      Result = NULL;
    }
    TRACE("CreateFileInfo 5");
  }
  else
  {
    TRACEFMT("CreateFileInfo E [%x]", static_cast<int>(GetLastError()));
  }
  return Result;
}
//---------------------------------------------------------------------------
// Free file version info block memory
void FreeFileInfo(void * FileInfo)
{
  if (FileInfo)
    delete[] FileInfo;
}
//---------------------------------------------------------------------------
typedef TTranslation TTranslations[65536];
typedef TTranslation *PTranslations;
//---------------------------------------------------------------------------
// Return pointer to fixed file version info
PVSFixedFileInfo GetFixedFileInfo(void * FileInfo)
{
#ifdef TRACE_FILE_APPL_INFO
  CALLSTACK;
#endif
  UINT Len;
  PVSFixedFileInfo Result = NULL;
#ifdef TRACE_FILE_APPL_INFO
  TRACE("GetFixedFileInfo 1");
#endif
  if (FileInfo && !VerQueryValue(FileInfo, L"\\", reinterpret_cast<void **>(&Result), &Len))
  {
    throw Exception(L"Fixed file info not available");
  }
#ifdef TRACE_FILE_APPL_INFO
  TRACE("GetFixedFileInfo 2");
#endif
  return Result;
}
//---------------------------------------------------------------------------
// Return number of available file version info translations
unsigned GetTranslationCount(void * FileInfo)
{
  CALLSTACK;
  PTranslations P;
  UINT Len;
  TRACE("GetTranslationCount 1");
  if (!VerQueryValue(FileInfo, L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&P), &Len))
    throw Exception(L"File info translations not available");
  TRACE("GetTranslationCount 2");
  return Len / 4;
}
//---------------------------------------------------------------------------
// Return i-th translation in the file version info translation list
TTranslation GetTranslation(void * FileInfo, intptr_t I)
{
  CALLSTACK;
  PTranslations P = NULL;
  UINT Len;

  TRACE("GetTranslation 1");
  if (!VerQueryValue(FileInfo, L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&P), &Len))
    throw Exception(L"File info translations not available");
  TRACE("GetTranslation 2");
  if (I * sizeof(TTranslation) >= Len)
    throw Exception(L"Specified translation not available");
  TRACE("GetTranslation 3");
  return P[I];
}
//---------------------------------------------------------------------------
// Return the name of the specified language
UnicodeString GetLanguage(Word Language)
{
  UINT Len;
  wchar_t P[256];

  Len = VerLanguageName(Language, P, LENOF(P));
  if (Len > LENOF(P))
    throw Exception(L"Language not available");
  return UnicodeString(P, Len);
}
//---------------------------------------------------------------------------
// Return the value of the specified file version info string using the
// specified translation
UnicodeString GetFileInfoString(void * FileInfo,
  TTranslation Translation, UnicodeString StringName)
{
  wchar_t * P;
  UINT Len;

  if (!VerQueryValue(FileInfo, (UnicodeString(L"\\StringFileInfo\\") +
    IntToHex(Translation.Language, 4) +
    IntToHex(Translation.CharSet, 4) +
    L"\\" + StringName).c_str(), reinterpret_cast<void**>(&P), &Len))
  {
    throw Exception("Specified file info string not available");
  }
  UnicodeString Result(P, Len);
  PackStr(Result);
  TRACEFMT("1 [%s] [%s]", StringName.c_str(), Result.c_str());
  return Result;
}
//---------------------------------------------------------------------------
int CalculateCompoundVersion(int MajorVer,
  int MinorVer, int Release, int Build)
{
  int CompoundVer = Build + 10000 * (Release + 100 * (MinorVer +
    100 * MajorVer));
  return CompoundVer;
}

