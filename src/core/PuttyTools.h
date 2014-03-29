//---------------------------------------------------------------------------
#ifndef PuttyToolsH
#define PuttyToolsH
//---------------------------------------------------------------------------
enum TKeyType
{
  ktUnopenable,
  ktUnknown,
  ktSSH1,
  ktSSH2,
  ktOpenSSH,
  ktSSHCom
};

TKeyType KeyType(const UnicodeString & AFileName);
UnicodeString KeyTypeName(TKeyType KeyType);
//---------------------------------------------------------------------------
int64_t ParseSize(const UnicodeString & SizeStr);
//---------------------------------------------------------------------------
bool HasGSSAPI(const UnicodeString & CustomPath);
//---------------------------------------------------------------------------
void AES256EncodeWithMAC(char * Data, size_t Len, const char * Password,
  size_t PasswordLen, const char * Salt);
//---------------------------------------------------------------------------
UnicodeString NormalizeFingerprint(const UnicodeString & Fingerprint);
UnicodeString KeyTypeFromFingerprint(const UnicodeString & Fingerprint);
//---------------------------------------------------------------------------
#endif
