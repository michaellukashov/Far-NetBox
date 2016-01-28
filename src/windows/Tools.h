#pragma once

#include <SessionData.h>

bool SaveDialog(const UnicodeString & Title, const UnicodeString & Filter,
  const UnicodeString & DefaultExt, UnicodeString & FileName);

bool AutodetectProxy(UnicodeString & HostName, int & PortNumber);
bool IsWin64();
void CopyToClipboard(const UnicodeString & Text);
void CopyToClipboard(TStrings * Strings);

void VerifyAndConvertKey(UnicodeString & FileName);
void VerifyKey(const UnicodeString & AFileName);
void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt);
void VerifyCertificate(const UnicodeString & AFileName);
