//---------------------------------------------------------------------------
#ifndef SecurityH
#define SecurityH
//---------------------------------------------------------------------------
#define PWALG_SIMPLE 1
#define PWALG_SIMPLE_MAGIC 0xA3
#define PWALG_SIMPLE_STRING ((RawByteString)"0123456789ABCDEF")
#define PWALG_SIMPLE_MAXLEN 50
#define PWALG_SIMPLE_FLAG 0xFF
int random(int range);
RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key, Integer Algorithm = PWALG_SIMPLE);
UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key, Integer Algorithm = PWALG_SIMPLE);
RawByteString SetExternalEncryptedPassword(const RawByteString & Password);
bool GetExternalEncryptedPassword(const RawByteString & Encrypted, RawByteString & Password);
//---------------------------------------------------------------------------
#endif
