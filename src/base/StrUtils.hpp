#pragma once

#include <Classes.hpp>

NB_CORE_EXPORT UnicodeString ReplaceStr(const UnicodeString Str, const UnicodeString What, const UnicodeString ByWhat);
NB_CORE_EXPORT bool StartsStr(const UnicodeString SubStr, const UnicodeString Str);
NB_CORE_EXPORT bool EndsStr(const UnicodeString SubStr, const UnicodeString Str);
NB_CORE_EXPORT bool EndsText(const UnicodeString SubStr, const UnicodeString Str);
NB_CORE_EXPORT UnicodeString LeftStr(const UnicodeString AStr, intptr_t Len);

NB_CORE_EXPORT UnicodeString EncodeBase64(const char* AStr, intptr_t Len);
NB_CORE_EXPORT rde::vector<uint8_t> DecodeBase64(UnicodeString AStr);
