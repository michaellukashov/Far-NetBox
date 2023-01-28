
#pragma once

#include <Classes.hpp>

void CryptographyInitialize();
void CryptographyFinalize();
RawByteString ScramblePassword(UnicodeString Password);
bool UnscramblePassword(RawByteString Scrambled, UnicodeString Password);
void AES256EncryptWithMAC(RawByteString Input, UnicodeString Password,
  RawByteString &Output);
bool AES256DecryptWithMAC(RawByteString Input, UnicodeString Password,
  RawByteString &Output);
void AES256CreateVerifier(UnicodeString Input, RawByteString &Verifier);
bool AES256Verify(UnicodeString Input, RawByteString Verifier);
int32_t IsValidPassword(UnicodeString Password);
int32_t PasswordMaxLength();
RawByteString GenerateEncryptKey();
void ValidateEncryptKey(const RawByteString AKey);

class TFileBuffer;
typedef void AESContext;

class TEncryption : public TObject
{
public:
  explicit TEncryption(const RawByteString AKey) noexcept;
  virtual ~TEncryption();

  static bool IsEncryptedFileName(const UnicodeString AFileName);

  void Encrypt(TFileBuffer & Buffer, bool Last);
  void Decrypt(TFileBuffer & Buffer);
  bool DecryptEnd(TFileBuffer & Buffer);
  UnicodeString EncryptFileName(const UnicodeString AFileName);
  UnicodeString DecryptFileName(const UnicodeString AFileName);

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

  void Init(const RawByteString AKey, const RawByteString ASalt);
  void Aes(char * Buffer, int32_t Size);
  void Aes(RawByteString & Buffer);
  void Aes(TFileBuffer & Buffer, bool Last);
  void NeedSalt();
  void SetSalt();
};

