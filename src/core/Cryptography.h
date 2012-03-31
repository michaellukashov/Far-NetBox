//---------------------------------------------------------------------------
#ifndef CryptographyH
#define CryptographyH
//---------------------------------------------------------------------------
void __fastcall CryptographyInitialize();
void __fastcall CryptographyFinalize();
void __fastcall ScramblePassword(std::wstring &Password);
bool __fastcall UnscramblePassword(std::wstring &Password);
void __fastcall AES256EncyptWithMAC(const std::wstring Input, const std::wstring Password,
                         std::wstring Salt, std::wstring &Output, std::wstring Mac);
void __fastcall AES256EncyptWithMAC(const std::wstring Input, const std::wstring Password,
                         std::wstring &Output);
bool __fastcall AES256DecryptWithMAC(const std::wstring Input, const std::wstring Password,
                          std::wstring Salt, std::wstring &Output, std::wstring Mac);
bool __fastcall AES256DecryptWithMAC(const std::wstring Input, const std::wstring Password,
                          std::wstring &Output);
void __fastcall AES256CreateVerifier(const std::wstring Input, std::wstring &Verifier);
bool __fastcall AES256Verify(const std::wstring Input, std::wstring Verifier);
int __fastcall IsValidPassword(const std::wstring Password);
size_t __fastcall PasswordMaxLength();
//---------------------------------------------------------------------------
#endif
