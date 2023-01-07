//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <Classes.hpp>
//---------------------------------------------------------------------------
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
intptr_t IsValidPassword(UnicodeString Password);
intptr_t PasswordMaxLength();
RawByteString GenerateEncryptKey();
void ValidateEncryptKey(const RawByteString AKey);
//---------------------------------------------------------------------------
class TFileBuffer;
typedef void AESContext;
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

  void Init(const RawByteString AKey, const RawByteString ASalt);
  void Aes(char * Buffer, intptr_t Size);
  void Aes(RawByteString & Buffer);
  void Aes(TFileBuffer & Buffer, bool Last);
  void NeedSalt();
  void SetSalt();
};
//---------------------------------------------------------------------------
