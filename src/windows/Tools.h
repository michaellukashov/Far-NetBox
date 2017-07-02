#pragma once

#include <SessionData.h>

bool SaveDialog(const UnicodeString & ATitle, const UnicodeString & Filter,
  const UnicodeString & ADefaultExt, UnicodeString & AFileName);

bool AutodetectProxy(UnicodeString & AHostName, intptr_t & APortNumber);
bool IsWin64();
void CopyToClipboard(const UnicodeString & AText);
void CopyToClipboard(TStrings * Strings);

void VerifyAndConvertKey(const UnicodeString & AFileName, TSshProt SshProt);
void VerifyKey(const UnicodeString & FileName, TSshProt SshProt);
void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt);
void VerifyCertificate(const UnicodeString & AFileName);
void OpenFileInExplorer(const UnicodeString & Path);
void OpenFolderInExplorer(const UnicodeString & Path);
