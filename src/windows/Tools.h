#pragma once

#include <SessionData.h>

bool SaveDialog(UnicodeString ATitle, UnicodeString Filter,
  UnicodeString ADefaultExt, UnicodeString & AFileName);

bool AutodetectProxy(UnicodeString & AHostName, intptr_t & APortNumber);
bool IsWin64();
void CopyToClipboard(UnicodeString AText);
void CopyToClipboard(TStrings * Strings);

void VerifyAndConvertKey(UnicodeString AFileName, TSshProt SshProt);
void VerifyKey(UnicodeString FileName, TSshProt SshProt);
void VerifyKeyIncludingVersion(UnicodeString AFileName, TSshProt SshProt);
void VerifyCertificate(UnicodeString AFileName);
void OpenFileInExplorer(UnicodeString Path);
void OpenFolderInExplorer(UnicodeString Path);
