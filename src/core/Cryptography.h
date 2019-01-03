//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <Classes.hpp>
//---------------------------------------------------------------------------
void CryptographyInitialize();
void CryptographyFinalize();
RawByteString ScramblePassword(const UnicodeString Password);
bool UnscramblePassword(const RawByteString Scrambled, const UnicodeString Password);
void AES256EncryptWithMAC(const RawByteString Input, const UnicodeString Password,
  RawByteString &Output);
bool AES256DecryptWithMAC(const RawByteString Input, const UnicodeString Password,
  RawByteString &Output);
void AES256CreateVerifier(const UnicodeString Input, RawByteString &Verifier);
bool AES256Verify(const UnicodeString Input, const RawByteString Verifier);
intptr_t IsValidPassword(const UnicodeString Password);
intptr_t PasswordMaxLength();
RawByteString GenerateEncryptKey();
void ValidateEncryptKey(const RawByteString & Key);
//---------------------------------------------------------------------------
class TFileBuffer;
//---------------------------------------------------------------------------
class TEncryption : public TObject
{
public:
  explicit TEncryption(const RawByteString AKey) noexcept;
  virtual ~TEncryption() noexcept;

  static bool IsEncryptedFileName(const UnicodeString AFileName);

  void Encrypt(TFileBuffer & Buffer, bool Last);
  void Decrypt(TFileBuffer & Buffer);
  bool DecryptEnd(TFileBuffer & Buffer);
  UnicodeString EncryptFileName(const UnicodeString AFileName);
  UnicodeString DecryptFileName(const UnicodeString AFileName);

  static intptr_t GetOverhead();
  static intptr_t RoundToBlock(intptr_t Size);
  static intptr_t RoundToBlockDown(intptr_t Size);

  void FreeContext();
private:
  RawByteString FKey;
  RawByteString FSalt;
  RawByteString FInputHeader;
  RawByteString FOverflowBuffer;
  bool FOutputtedHeader{false};
  void * FContext{nullptr};

  void Init(const RawByteString & Key, const RawByteString & Salt);
  void Aes(char * Buffer, intptr_t Size);
  void Aes(RawByteString & Buffer);
  void Aes(TFileBuffer & Buffer, bool Last);
  void NeedSalt();
  void SetSalt();
};
//---------------------------------------------------------------------------
