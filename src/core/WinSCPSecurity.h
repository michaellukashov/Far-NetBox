//---------------------------------------------------------------------------
#ifndef SecurityH
#define SecurityH
//---------------------------------------------------------------------------
#define PWALG_SIMPLE 1
#define PWALG_SIMPLE_MAGIC 0xA3
#define PWALG_SIMPLE_STRING ((std::string)"0123456789ABCDEF")
#define PWALG_SIMPLE_MAXLEN 50
#define PWALG_SIMPLE_FLAG 0xFF
int random(int range);
UnicodeString EncryptPassword(const UnicodeString Password, const UnicodeString Key, int Algorithm = PWALG_SIMPLE);
UnicodeString DecryptPassword(const UnicodeString Password, const UnicodeString Key, int Algorithm = PWALG_SIMPLE);
UnicodeString SetExternalEncryptedPassword(const UnicodeString Password);
bool GetExternalEncryptedPassword(const UnicodeString Encrypted, UnicodeString & Password);
//---------------------------------------------------------------------------
#endif
