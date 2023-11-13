
#pragma once

#include <Classes.hpp>

constexpr const int32_t PWALG_SIMPLE = 1;
constexpr const int32_t PWALG_SIMPLE_MAGIC = 0xA3;
constexpr const int32_t PWALG_SIMPLE_MAXLEN = 50;
constexpr const int32_t PWALG_SIMPLE_FLAG = 0xFF;

NB_CORE_EXPORT RawByteString EncryptPassword(const UnicodeString & Password, const UnicodeString & Key, Integer Algorithm = PWALG_SIMPLE);
NB_CORE_EXPORT UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & Key, Integer Algorithm = PWALG_SIMPLE);
NB_CORE_EXPORT RawByteString SetExternalEncryptedPassword(const RawByteString & Password);
NB_CORE_EXPORT bool GetExternalEncryptedPassword(const RawByteString & Encrypted, RawByteString & Password);

NB_CORE_EXPORT bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len, UnicodeString & Error);

