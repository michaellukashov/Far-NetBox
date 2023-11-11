
#pragma once

#include <Classes.hpp>

void CryptographyInitialize();
void CryptographyFinalize();
RawByteString ScramblePassword(const UnicodeString & Password);
bool UnscramblePassword(const RawByteString & Scrambled, const UnicodeString & Password);
void AES256EncryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Output);
bool AES256DecryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Output);
void AES256CreateVerifier(const UnicodeString & Input, RawByteString & Verifier);
bool AES256Verify(const UnicodeString & Input, const RawByteString & Verifier);
int32_t IsValidPassword(const UnicodeString & Password);
int32_t PasswordMaxLength();
RawByteString GenerateEncryptKey();
void ValidateEncryptKey(const RawByteString & AKey);

class TFileBuffer;
typedef void AESContext;

class TEncryption : public TObject
{
public:
  explicit TEncryption(const RawByteString & AKey) noexcept;
  virtual ~TEncryption(); //noexcept(false); //throw(std::runtime_error);

  static bool IsEncryptedFileName(const UnicodeString & AFileName);

  void Encrypt(TFileBuffer & Buffer, bool Last);
  void Decrypt(TFileBuffer & Buffer);
  bool DecryptEnd(TFileBuffer & Buffer);
  UnicodeString EncryptFileName(const UnicodeString & AFileName);
  UnicodeString DecryptFileName(const UnicodeString & AFileName);

  static int32_t GetOverhead();
  static int32_t RoundToBlock(int32_t Size);
  static int32_t RoundToBlockDown(int32_t Size);

private:
  RawByteString FKey;
  RawByteString FSalt;
  RawByteString FInputHeader;
  RawByteString FOverflowBuffer;
  bool FOutputtedHeader{false};
  AESContext * FContext{nullptr};

  void Init(const RawByteString & AKey, const RawByteString & ASalt);
  void Aes(char * Buffer, int32_t Size);
  void Aes(RawByteString & Buffer);
  void Aes(TFileBuffer & Buffer, bool Last);
  void NeedSalt();
  void SetSalt();
};

