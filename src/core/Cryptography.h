//---------------------------------------------------------------------------
#ifndef CryptographyH
#define CryptographyH
//---------------------------------------------------------------------------
void __fastcall CryptographyInitialize();
void __fastcall CryptographyFinalize();
void __fastcall ScramblePassword(UnicodeString &Password);
bool __fastcall UnscramblePassword(UnicodeString &Password);
void __fastcall AES256EncyptWithMAC(const UnicodeString Input, const UnicodeString Password,
                         UnicodeString Salt, UnicodeString &Output, UnicodeString Mac);
void __fastcall AES256EncyptWithMAC(const UnicodeString Input, const UnicodeString Password,
                         UnicodeString &Output);
bool __fastcall AES256DecryptWithMAC(const UnicodeString Input, const UnicodeString Password,
                          UnicodeString Salt, UnicodeString &Output, UnicodeString Mac);
bool __fastcall AES256DecryptWithMAC(const UnicodeString Input, const UnicodeString Password,
                          UnicodeString &Output);
void __fastcall AES256CreateVerifier(const UnicodeString Input, UnicodeString &Verifier);
bool __fastcall AES256Verify(const UnicodeString Input, UnicodeString Verifier);
int __fastcall IsValidPassword(const UnicodeString Password);
size_t __fastcall PasswordMaxLength();
//---------------------------------------------------------------------------
#endif
