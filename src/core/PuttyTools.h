
#pragma once

enum TKeyType
{
  ktUnopenable, ktUnknown,
  ktSSH1, ktSSH2,
  ktOpenSSHAuto, ktOpenSSHPEM, ktOpenSSHNew, ktSSHCom,
  ktSSH1Public, ktSSH2PublicRFC4716, ktSSH2PublicOpenSSH,
};

TKeyType GetKeyType(UnicodeString AFileName);
UnicodeString GetKeyTypeName(TKeyType KeyType);
bool IsKeyEncrypted(TKeyType KeyType, UnicodeString FileName, UnicodeString & Comment);
struct TPrivateKey;
TPrivateKey * LoadKey(TKeyType KeyType, UnicodeString FileName, UnicodeString Passphrase);
void ChangeKeyComment(TPrivateKey * PrivateKey, UnicodeString Comment);
void SaveKey(TKeyType KeyType, UnicodeString FileName,
  UnicodeString Passphrase, TPrivateKey * PrivateKey);
void FreeKey(TPrivateKey * PrivateKey);

int64_t ParseSize(UnicodeString SizeStr);

bool HasGSSAPI(UnicodeString CustomPath);

void AES256EncodeWithMAC(char * Data, size_t Len, const char * Password,
  size_t PasswordLen, const char * Salt);

UnicodeString NormalizeFingerprint(UnicodeString AFingerprint);
UnicodeString GetKeyTypeFromFingerprint(UnicodeString AFingerprint);

UnicodeString GetPuTTYVersion();

UnicodeString Sha256(const char * Data, size_t Size);

void DllHijackingProtection();
