#pragma once

#include <SessionData.h>

bool SaveDialog(const UnicodeString & ATitle, const UnicodeString & Filter,
  const UnicodeString & ADefaultExt, UnicodeString & AFileName);

bool AutodetectProxy(UnicodeString & AHostName, intptr_t & APortNumber);
bool IsWin64();
void CopyToClipboard(const UnicodeString & AText);
void CopyToClipboard(TStrings * Strings);

void VerifyAndConvertKey(UnicodeString & AFileName);
void VerifyKey(const UnicodeString & AFileName);
void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt);
void VerifyCertificate(const UnicodeString & AFileName);
