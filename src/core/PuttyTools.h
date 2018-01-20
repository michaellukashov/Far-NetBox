
#pragma once

#include <Classes.hpp>

enum TKeyType
{
  ktUnopenable, ktUnknown,
  ktSSH1, ktSSH2,
  ktOpenSSHAuto, ktOpenSSHPEM, ktOpenSSHNew, ktSSHCom,
  ktSSH1Public, ktSSH2PublicRFC4716, ktSSH2PublicOpenSSH,
};

NB_CORE_EXPORT TKeyType GetKeyType(UnicodeString AFileName);
NB_CORE_EXPORT UnicodeString GetKeyTypeName(TKeyType KeyType);
NB_CORE_EXPORT bool IsKeyEncrypted(TKeyType KeyType, UnicodeString FileName, UnicodeString &Comment);
struct TPrivateKey;
NB_CORE_EXPORT TPrivateKey *LoadKey(TKeyType KeyType, UnicodeString FileName, UnicodeString Passphrase);
NB_CORE_EXPORT void ChangeKeyComment(TPrivateKey *PrivateKey, UnicodeString Comment);
NB_CORE_EXPORT void SaveKey(TKeyType KeyType, UnicodeString FileName,
  UnicodeString Passphrase, TPrivateKey *PrivateKey);
NB_CORE_EXPORT void FreeKey(TPrivateKey *PrivateKey);

NB_CORE_EXPORT int64_t ParseSize(UnicodeString SizeStr);

NB_CORE_EXPORT bool HasGSSAPI(UnicodeString CustomPath);

NB_CORE_EXPORT void AES256EncodeWithMAC(char *Data, size_t Len, const char *Password,
  size_t PasswordLen, const char *Salt);

NB_CORE_EXPORT UnicodeString NormalizeFingerprint(UnicodeString AFingerprint);
NB_CORE_EXPORT UnicodeString GetKeyTypeFromFingerprint(UnicodeString AFingerprint);

NB_CORE_EXPORT UnicodeString GetPuTTYVersion();

NB_CORE_EXPORT UnicodeString Sha256(const char *Data, size_t Size);

NB_CORE_EXPORT void DllHijackingProtection();
UnicodeString __fastcall ParseOpenSshPubLine(const UnicodeString ALine, const struct ssh_signkey *& Algorithm);
//---------------------------------------------------------------------------
UnicodeString __fastcall GetKeyTypeHuman(const UnicodeString AKeyType);
//---------------------------------------------------------------------------
