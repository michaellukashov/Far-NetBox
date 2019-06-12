#pragma once

#include <Classes.hpp>

struct TTranslation
{
  Word Language{0}, CharSet{0};
};

// Return pointer to file version info block
NB_CORE_EXPORT void *CreateFileInfo(UnicodeString AFileName);

// Free file version info block memory
NB_CORE_EXPORT void FreeFileInfo(void *FileInfo);

// Return pointer to fixed file version info
NB_CORE_EXPORT PVSFixedFileInfo GetFixedFileInfo(void *FileInfo);

// Return number of available file version info translations
NB_CORE_EXPORT uint32_t GetTranslationCount(void *FileInfo);

// Return i-th translation in the file version info translation list
NB_CORE_EXPORT TTranslation GetTranslation(void *FileInfo, intptr_t I);

// Return the name of the specified language
NB_CORE_EXPORT UnicodeString GetLanguage(Word Language);

// Return the value of the specified file version info string using the
// specified translation
NB_CORE_EXPORT UnicodeString GetFileInfoString(void *FileInfo,
  TTranslation Translation, UnicodeString StringName, bool AllowEmpty = false);

NB_CORE_EXPORT intptr_t CalculateCompoundVersion(intptr_t MajorVer,
  intptr_t MinorVer, intptr_t Release, intptr_t Build);

NB_CORE_EXPORT intptr_t StrToCompoundVersion(UnicodeString AStr);

NB_CORE_EXPORT intptr_t CompareVersion(UnicodeString V1, UnicodeString V2);
