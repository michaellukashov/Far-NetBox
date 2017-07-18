#pragma once

#include <SessionData.h>

NB_CORE_EXPORT bool SaveDialog(const UnicodeString & ATitle, const UnicodeString & Filter,
  const UnicodeString & ADefaultExt, UnicodeString & AFileName);

NB_CORE_EXPORT bool AutodetectProxy(UnicodeString & AHostName, intptr_t & APortNumber);
NB_CORE_EXPORT bool IsWin64();
NB_CORE_EXPORT void CopyToClipboard(const UnicodeString & AText);
NB_CORE_EXPORT void CopyToClipboard(TStrings * Strings);

NB_CORE_EXPORT bool VerifyAndConvertKey(const UnicodeString & AFileName, bool TypeOnly);
NB_CORE_EXPORT bool VerifyKey(const UnicodeString & AFileName, bool TypeOnly);
NB_CORE_EXPORT void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt);
NB_CORE_EXPORT void VerifyCertificate(const UnicodeString & AFileName);
