#pragma once

#include <SessionData.h>

void VerifyKey(const UnicodeString & AFileName);
void VerifyKeyIncludingVersion(const UnicodeString & AFileName, TSshProt SshProt);
void VerifyCertificate(const UnicodeString & AFileName);
