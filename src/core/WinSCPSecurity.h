
#pragma once

#include <Classes.hpp>

#if defined(__BORLANDC__)
#define PWALG_SIMPLE 1
#define PWALG_SIMPLE_MAGIC 0xA3
#define PWALG_SIMPLE_MAXLEN 50
#define PWALG_SIMPLE_FLAG 0xFF
#endif // defined(__BORLANDC__)

constexpr const int32_t PWALG_SIMPLE = 1;
constexpr const int32_t PWALG_SIMPLE_MAGIC = 0xA3;
constexpr const int32_t PWALG_SIMPLE_MAXLEN = 50;
constexpr const int32_t PWALG_SIMPLE_FLAG = 0xFF;

NB_CORE_EXPORT RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key, Integer Algorithm = PWALG_SIMPLE);
NB_CORE_EXPORT UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key, Integer Algorithm = PWALG_SIMPLE);
NB_CORE_EXPORT RawByteString SetExternalEncryptedPassword(const RawByteString & Password);
NB_CORE_EXPORT bool GetExternalEncryptedPassword(const RawByteString & Encrypted, RawByteString & Password);

NB_CORE_EXPORT bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len, UnicodeString & Error);

