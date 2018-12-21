//---------------------------------------------------------------------------
#pragma once

#include <Classes.hpp>
//---------------------------------------------------------------------------
void CryptographyInitialize();
void CryptographyFinalize();
RawByteString ScramblePassword(const UnicodeString Password);
bool UnscramblePassword(const RawByteString Scrambled, const UnicodeString Password);
void AES256EncyptWithMAC(const RawByteString Input, const UnicodeString Password,
  RawByteString &Output);
bool AES256DecryptWithMAC(const RawByteString Input, const UnicodeString Password,
  RawByteString &Output);
void AES256CreateVerifier(const UnicodeString Input, RawByteString &Verifier);
bool AES256Verify(const UnicodeString Input, const RawByteString Verifier);
int IsValidPassword(const UnicodeString Password);
int PasswordMaxLength();
RawByteString GenerateEncryptKey();
void ValidateEncryptKey(const RawByteString & Key);
//---------------------------------------------------------------------------
class TFileBuffer;
//---------------------------------------------------------------------------
class TEncryption
{
public:
  TEncryption(const RawByteString & Key);
  ~TEncryption();

  static bool IsEncryptedFileName(const UnicodeString & FileName);

  void Encrypt(TFileBuffer & Buffer, bool Last);
  void Decrypt(TFileBuffer & Buffer);
  bool DecryptEnd(TFileBuffer & Buffer);
  UnicodeString EncryptFileName(const UnicodeString & FileName);
  UnicodeString DecryptFileName(const UnicodeString & FileName);

  static int GetOverhead();
  static int RoundToBlock(int Size);
  static int RoundToBlockDown(int Size);

private:
  RawByteString FKey;
  RawByteString FSalt;
  RawByteString FInputHeader;
  RawByteString FOverflowBuffer;
  bool FOutputtedHeader;
  void * FContext;

  void Init(const RawByteString & Key, const RawByteString & Salt);
  void Aes(char * Buffer, int Size);
  void Aes(RawByteString & Buffer);
  void Aes(TFileBuffer & Buffer, bool Last);
  void NeedSalt();
  void SetSalt();
};
//---------------------------------------------------------------------------
