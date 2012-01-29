//---------------------------------------------------------------------------
#ifndef PuttyToolsH
#define PuttyToolsH
//---------------------------------------------------------------------------
enum TKeyType { ktUnopenable, ktUnknown, ktSSH1, ktSSH2, ktOpenSSH, ktSSHCom };
TKeyType KeyType(const std::wstring FileName);
std::wstring KeyTypeName(TKeyType KeyType);
//---------------------------------------------------------------------------
std::string DecodeUTF(const std::string &UTF);
std::string EncodeUTF(const std::wstring Source);
//---------------------------------------------------------------------------
__int64 ParseSize(const std::wstring SizeStr);
//---------------------------------------------------------------------------
bool HasGSSAPI();
//---------------------------------------------------------------------------
void AES256EncodeWithMAC(char *Data, size_t Len, const char *Password,
                         size_t PasswordLen, const char *Salt);
//---------------------------------------------------------------------------
#endif
