//---------------------------------------------------------------------------
#ifndef PuttyToolsH
#define PuttyToolsH
//---------------------------------------------------------------------------
enum TKeyType { ktUnopenable, ktUnknown, ktSSH1, ktSSH2, ktOpenSSH, ktSSHCom };
TKeyType KeyType(const UnicodeString & FileName);
UnicodeString KeyTypeName(TKeyType KeyType);
//---------------------------------------------------------------------------
__int64 ParseSize(const UnicodeString & SizeStr);
//---------------------------------------------------------------------------
bool HasGSSAPI();
//---------------------------------------------------------------------------
void AES256EncodeWithMAC(char * Data, size_t Len, const char * Password,
  size_t PasswordLen, const char * Salt);
//---------------------------------------------------------------------------
#endif
