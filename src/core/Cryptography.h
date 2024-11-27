
#pragma once

#include <Classes.hpp>

void CryptographyInitialize();
void CryptographyFinalize();
void RequireTls();
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
using AESContext = void;

class TEncryption final : public TObject
{
  TEncryption() = delete;
public:
  explicit TEncryption(const RawByteString & AKey) noexcept;
  virtual ~TEncryption() = default;
  void Finalize();

  static bool IsEncryptedFileName(const UnicodeString & AFileName);

  void Encrypt(TFileBuffer & Buffer, bool Last);
  void Decrypt(TFileBuffer & Buffer);
  bool DecryptEnd(TFileBuffer & Buffer);
  UnicodeString EncryptFileName(const UnicodeString & AFileName);
  UnicodeString DecryptFileName(const UnicodeString & AFileName);

  static int32_t GetOverhead();
  static int64_t RoundToBlock(int64_t Size);
  static int64_t RoundToBlockDown(int64_t Size);

private:
  RawByteString FKey;
  RawByteString FSalt;
  RawByteString FInputHeader;
  RawByteString FOverflowBuffer;
  bool FOutputtedHeader{false};
  AESContext * FContext{nullptr};

  void Init(const RawByteString & AKey, const RawByteString & ASalt);
  void Aes(char * Buffer, int64_t Size);
  void Aes(RawByteString & Buffer);
  void Aes(TFileBuffer & Buffer, bool Last);
  void NeedSalt();
  void SetSalt();
};

