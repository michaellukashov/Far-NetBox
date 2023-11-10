
#pragma once

#include <Classes.hpp>
#include <SessionData.h>

enum TKeyType
{
  ktUnopenable, ktUnknown,
  ktSSH1, ktSSH2,
  ktOpenSSHAuto, ktOpenSSHPEM, ktOpenSSHNew, ktSSHCom,
  ktSSH1Public, ktSSH2PublicRFC4716, ktSSH2PublicOpenSSH,
};
NB_CORE_EXPORT TKeyType GetKeyType(const UnicodeString & AFileName);
NB_CORE_EXPORT bool IsKeyEncrypted(TKeyType KeyType, const UnicodeString & FileName, UnicodeString & Comment);
struct TPrivateKey;
NB_CORE_EXPORT TPrivateKey * LoadKey(TKeyType KeyType, const UnicodeString & FileName, const UnicodeString & Passphrase);
UnicodeString TestKey(TKeyType KeyType, const UnicodeString & FileName);
NB_CORE_EXPORT void ChangeKeyComment(TPrivateKey * PrivateKey, const UnicodeString & Comment);
void AddCertificateToKey(TPrivateKey * PrivateKey, const UnicodeString & CertificateFileName);
NB_CORE_EXPORT void SaveKey(TKeyType KeyType, const UnicodeString & FileName,
  const UnicodeString & Passphrase, TPrivateKey * PrivateKey);
NB_CORE_EXPORT void FreeKey(TPrivateKey * PrivateKey);
RawByteString LoadPublicKey(
  const UnicodeString & FileName, UnicodeString & Algorithm, UnicodeString & Comment, bool & HasCertificate);
UnicodeString GetPublicKeyLine(const UnicodeString & FileName, UnicodeString & Comment, bool & HasCertificate);
extern const UnicodeString PuttyKeyExt;

NB_CORE_EXPORT bool HasGSSAPI(const UnicodeString & CustomPath);

NB_CORE_EXPORT void AES256EncodeWithMAC(char * Data, size_t Len, const char * Password,
  size_t PasswordLen, const char * Salt);

NB_CORE_EXPORT void NormalizeFingerprint(UnicodeString & AFingerprint, UnicodeString & KeyName);
NB_CORE_EXPORT UnicodeString KeyTypeFromFingerprint(const UnicodeString & AFingerprint);

NB_CORE_EXPORT UnicodeString GetPuTTYVersion();

NB_CORE_EXPORT UnicodeString Sha256(const char * Data, size_t Size);
UnicodeString CalculateFileChecksum(TStream * Stream, const UnicodeString & Alg);

UnicodeString ParseOpenSshPubLine(const UnicodeString & ALine, const struct ssh_keyalg *& Algorithm);
void ParseCertificatePublicKey(const UnicodeString & Str, RawByteString & PublicKey, UnicodeString & Fingerprint);
bool IsCertificateValidityExpressionValid(
  const UnicodeString & Str, UnicodeString & Error, int & ErrorStart, int & ErrorLen);

UnicodeString GetKeyTypeHuman(const UnicodeString & AKeyType);

bool IsOpenSSH(const UnicodeString & SshImplementation);

NB_CORE_EXPORT UnicodeString GetKeyTypeName(TKeyType KeyType);

bool IsOpenSSH(const UnicodeString & SshImplementation);

TStrings * SshCipherList();
TStrings * SshKexList();
int HostKeyToPutty(THostKey HostKey);
TStrings * SshHostKeyList();
TStrings * SshMacList();

class TSessionData;
void SaveAsPutty(const UnicodeString & Name, TSessionData * Data);
class THierarchicalStorage;
void WritePuttySettings(THierarchicalStorage * Storage, const UnicodeString & Settings);
void SavePuttyDefaults(const UnicodeString & Name);

bool RandomSeedExists();

