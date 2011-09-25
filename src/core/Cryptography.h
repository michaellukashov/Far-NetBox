//---------------------------------------------------------------------------
#ifndef CryptographyH
#define CryptographyH
//---------------------------------------------------------------------------
void CryptographyInitialize();
void CryptographyFinalize();
void ScramblePassword(std::wstring & Password);
bool UnscramblePassword(std::wstring & Password);
void AES256EncyptWithMAC(std::wstring Input, std::wstring Password,
  std::wstring & Salt, std::wstring & Output, std::wstring & Mac);
void AES256EncyptWithMAC(std::wstring Input, std::wstring Password,
  std::wstring & Output);
bool AES256DecryptWithMAC(std::wstring Input, std::wstring Password,
  std::wstring Salt, std::wstring & Output, std::wstring Mac);
bool AES256DecryptWithMAC(std::wstring Input, std::wstring Password,
  std::wstring & Output);
void AES256CreateVerifier(std::wstring Input, std::wstring & Verifier);
bool AES256Verify(std::wstring Input, std::wstring Verifier);
int IsValidPassword(std::wstring Password);
int PasswordMaxLength();
//---------------------------------------------------------------------------
#endif
