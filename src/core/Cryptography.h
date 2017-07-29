
#pragma once

#include <Classes.hpp>

void CryptographyInitialize();
void CryptographyFinalize();
RawByteString ScramblePassword(UnicodeString Password);
bool UnscramblePassword(RawByteString Scrambled, UnicodeString & Password);
void AES256EncyptWithMAC(RawByteString Input, UnicodeString Password,
  RawByteString & Output);
bool AES256DecryptWithMAC(RawByteString Input, UnicodeString Password,
  RawByteString & Output);
void AES256CreateVerifier(UnicodeString Input, RawByteString & Verifier);
bool AES256Verify(UnicodeString Input, RawByteString Verifier);
int IsValidPassword(UnicodeString Password);
int PasswordMaxLength();

