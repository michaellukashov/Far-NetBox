
#pragma once

#include <Classes.hpp>

#define PWALG_SIMPLE 1
#define PWALG_SIMPLE_MAGIC 0xA3
#define PWALG_SIMPLE_STRING ((RawByteString)"0123456789ABCDEF")
#define PWALG_SIMPLE_MAXLEN 50
#define PWALG_SIMPLE_FLAG 0xFF
RawByteString EncryptPassword(UnicodeString Password, UnicodeString Key, Integer Algorithm = PWALG_SIMPLE);
UnicodeString DecryptPassword(RawByteString Password, UnicodeString Key, Integer Algorithm = PWALG_SIMPLE);
RawByteString SetExternalEncryptedPassword(RawByteString Password);
bool GetExternalEncryptedPassword(RawByteString Encrypted, RawByteString & Password);

bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len, UnicodeString & Error);

