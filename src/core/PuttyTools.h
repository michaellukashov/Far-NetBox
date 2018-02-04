
#pragma once

#include <Classes.hpp>

enum TKeyType
{
  ktUnopenable, ktUnknown,
  ktSSH1, ktSSH2,
  ktOpenSSHAuto, ktOpenSSHPEM, ktOpenSSHNew, ktSSHCom,
  ktSSH1Public, ktSSH2PublicRFC4716, ktSSH2PublicOpenSSH,
};

NB_CORE_EXPORT TKeyType GetKeyType(const UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString GetKeyTypeName(TKeyType KeyType);
NB_CORE_EXPORT bool IsKeyEncrypted(TKeyType KeyType, UnicodeString FileName, UnicodeString &Comment);
struct TPrivateKey;
NB_CORE_EXPORT TPrivateKey *LoadKey(TKeyType KeyType, const UnicodeString FileName, const UnicodeString Passphrase);
NB_CORE_EXPORT void ChangeKeyComment(TPrivateKey *PrivateKey, const UnicodeString Comment);
NB_CORE_EXPORT void SaveKey(TKeyType KeyType, const UnicodeString FileName,
  UnicodeString Passphrase, TPrivateKey *PrivateKey);
NB_CORE_EXPORT void FreeKey(TPrivateKey *PrivateKey);

NB_CORE_EXPORT int64_t ParseSize(const UnicodeString SizeStr);

NB_CORE_EXPORT bool HasGSSAPI(const UnicodeString CustomPath);

NB_CORE_EXPORT void AES256EncodeWithMAC(char *Data, size_t Len, const char *Password,
  size_t PasswordLen, const char *Salt);

NB_CORE_EXPORT UnicodeString NormalizeFingerprint(const UnicodeString AFingerprint);
NB_CORE_EXPORT UnicodeString GetKeyTypeFromFingerprint(const UnicodeString AFingerprint);

NB_CORE_EXPORT UnicodeString GetPuTTYVersion();

NB_CORE_EXPORT UnicodeString Sha256(const char *Data, size_t Size);

NB_CORE_EXPORT void DllHijackingProtection();
UnicodeString ParseOpenSshPubLine(const UnicodeString ALine, const struct ssh_signkey *& Algorithm);
//---------------------------------------------------------------------------
UnicodeString GetKeyTypeHuman(const UnicodeString AKeyType);
//---------------------------------------------------------------------------
