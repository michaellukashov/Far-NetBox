#pragma once

#include <SessionData.h>

NB_CORE_EXPORT bool SaveDialog(UnicodeString ATitle, UnicodeString Filter,
  UnicodeString ADefaultExt, UnicodeString & AFileName);

NB_CORE_EXPORT bool AutodetectProxy(UnicodeString & AHostName, intptr_t & APortNumber);
NB_CORE_EXPORT bool IsWin64();
NB_CORE_EXPORT void CopyToClipboard(UnicodeString AText);
NB_CORE_EXPORT void CopyToClipboard(TStrings * Strings);

NB_CORE_EXPORT void VerifyAndConvertKey(UnicodeString AFileName, TSshProt SshProt);
NB_CORE_EXPORT void VerifyKey(UnicodeString FileName, TSshProt SshProt);
NB_CORE_EXPORT void VerifyKeyIncludingVersion(UnicodeString AFileName, TSshProt SshProt);
NB_CORE_EXPORT void VerifyCertificate(UnicodeString AFileName);
NB_CORE_EXPORT void OpenFileInExplorer(UnicodeString Path);
NB_CORE_EXPORT void OpenFolderInExplorer(UnicodeString Path);
