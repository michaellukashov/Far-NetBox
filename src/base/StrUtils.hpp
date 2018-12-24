#pragma once

#include <Classes.hpp>

NB_CORE_EXPORT UnicodeString ReplaceStr(const UnicodeString Str, const UnicodeString What, const UnicodeString ByWhat);
NB_CORE_EXPORT bool StartsStr(const UnicodeString SubStr, const UnicodeString Str);
NB_CORE_EXPORT bool EndsStr(const UnicodeString SubStr, const UnicodeString Str);
NB_CORE_EXPORT bool EndsText(const UnicodeString SubStr, const UnicodeString Str);
