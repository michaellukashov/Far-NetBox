#ifndef FileInfoH
#define FileInfoH

#include "stdafx.h"

struct TTranslation {
  unsigned int Language, CharSet;
};

// Return pointer to file version info block
void * CreateFileInfo(std::wstring FileName);

// Free file version info block memory
void FreeFileInfo(void * FileInfo);

// Return pointer to fixed file version info
// FIXME PVSFixedFileInfo GetFixedFileInfo(void * FileInfo);

// Return number of available file version info translations
unsigned GetTranslationCount(void * FileInfo);

// Return i-th translation in the file version info translation list
TTranslation GetTranslation(void * FileInfo, unsigned i);

// Return the name of the specified language
std::wstring GetLanguage(unsigned int Language);

// Return the value of the specified file version info string using the
// specified translation
std::wstring GetFileInfoString(void * FileInfo,
  TTranslation Translation, std::wstring StringName);

int CalculateCompoundVersion(int MajorVer,
  int MinorVer, int Release, int Build);

#endif // FileInfoH
