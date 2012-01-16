//---------------------------------------------------------------------------
#ifndef CryptographyH
#define CryptographyH
//---------------------------------------------------------------------------
void CryptographyInitialize();
void CryptographyFinalize();
void ScramblePassword(std::wstring & Password);
bool UnscramblePassword(std::wstring & Password);
void AES256EncyptWithMAC(const std::wstring &Input, const std::wstring &Password,
  std::wstring Salt, std::wstring & Output, std::wstring Mac);
void AES256EncyptWithMAC(const std::wstring &Input, const std::wstring &Password,
  std::wstring & Output);
bool AES256DecryptWithMAC(const std::wstring &Input, const std::wstring &Password,
  std::wstring Salt, std::wstring & Output, std::wstring Mac);
bool AES256DecryptWithMAC(const std::wstring &Input, const std::wstring &Password,
  std::wstring & Output);
void AES256CreateVerifier(const std::wstring &Input, std::wstring & Verifier);
bool AES256Verify(const std::wstring &Input, std::wstring Verifier);
int IsValidPassword(const std::wstring &Password);
size_t PasswordMaxLength();
//---------------------------------------------------------------------------
#endif
