#pragma once

enum TKeyType
{
  ktUnopenable,
  ktUnknown,
  ktSSH1,
  ktSSH2,
  ktOpenSSHAuto,
  ktOpenSSHPem,
  ktOpenSSHNew,
  ktSSHCom,
  ktSSH1Public,
  ktSSH2PublicRFC4716,
  ktSSH2PublicOpenSSH,
};

TKeyType GetKeyType(const UnicodeString & AFileName);
UnicodeString GetKeyTypeName(TKeyType KeyType);

int64_t ParseSize(const UnicodeString & SizeStr);

bool HasGSSAPI(const UnicodeString & CustomPath);

void AES256EncodeWithMAC(char * Data, size_t Len, const char * Password,
  size_t PasswordLen, const char * Salt);

UnicodeString NormalizeFingerprint(const UnicodeString & Fingerprint);
UnicodeString GetKeyTypeFromFingerprint(const UnicodeString & Fingerprint);

