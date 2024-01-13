#pragma once

#include <nbtypes.h>

struct TTranslation {
  Word Language{0}, CharSet{0};
};

// Return pointer to file version info block
NB_CORE_EXPORT void * CreateFileInfo(const UnicodeString & AFileName);

// Free file version info block memory
NB_CORE_EXPORT void FreeFileInfo(const void * FileInfo);

// Return pointer to fixed file version info
NB_CORE_EXPORT PVSFixedFileInfo GetFixedFileInfo(const void * FileInfo);

// Return number of available file version info translations
NB_CORE_EXPORT uint32_t GetTranslationCount(const void * FileInfo);

// Return i-th translation in the file version info translation list
NB_CORE_EXPORT TTranslation GetTranslation(const void * FileInfo, uint32_t I);

// Return the name of the specified language
NB_CORE_EXPORT UnicodeString GetLanguage(Word Language);

// Return the value of the specified file version info string using the
// specified translation
NB_CORE_EXPORT UnicodeString GetFileInfoString(const void * FileInfo,
  TTranslation Translation, const UnicodeString & StringName, bool AllowEmpty = false);

NB_CORE_EXPORT int32_t CalculateCompoundVersion(int32_t MajorVer, int32_t MinorVer, int32_t Release = 0);
int32_t ZeroBuildNumber(int32_t CompoundVersion);

NB_CORE_EXPORT int32_t StrToCompoundVersion(const UnicodeString & AStr);

NB_CORE_EXPORT int32_t CompareVersion(const UnicodeString & V1, const UnicodeString & V2);

