#pragma once

#include <rdestl/vector.h>
#include <Classes.hpp>

NB_CORE_EXPORT UnicodeString ReplaceStr(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat);
NB_CORE_EXPORT bool StartsStr(const UnicodeString & SubStr, const UnicodeString & Str);
NB_CORE_EXPORT bool EndsStr(const UnicodeString & SubStr, const UnicodeString & Str);
NB_CORE_EXPORT bool EndsText(const UnicodeString & SubStr, const UnicodeString & Str);
NB_CORE_EXPORT UnicodeString LeftStr(const UnicodeString & AStr, int32_t Len);

using TBytes = nb::vector_t<uint8_t>;

NB_CORE_EXPORT UnicodeString EncodeBase64(const char * AStr, int32_t Len);
NB_CORE_EXPORT TBytes DecodeBase64(const UnicodeString & AStr);

