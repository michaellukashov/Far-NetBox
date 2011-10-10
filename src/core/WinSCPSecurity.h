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
std::wstring EncryptPassword(std::wstring Password, std::wstring Key, int Algorithm = PWALG_SIMPLE);
std::wstring DecryptPassword(std::wstring Password, std::wstring Key, int Algorithm = PWALG_SIMPLE);
std::wstring SetExternalEncryptedPassword(std::wstring Password);
bool GetExternalEncryptedPassword(std::wstring Encrypted, std::wstring & Password);
//---------------------------------------------------------------------------
#endif
