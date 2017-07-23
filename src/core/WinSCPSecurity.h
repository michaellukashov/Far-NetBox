#pragma once

#include <Classes.hpp>

#define PWALG_SIMPLE 1
#define PWALG_SIMPLE_MAGIC 0xA3

NB_CORE_EXPORT RawByteString EncryptPassword(const UnicodeString & APassword, const UnicodeString & AKey, Integer AAlgorithm = PWALG_SIMPLE);
NB_CORE_EXPORT UnicodeString DecryptPassword(const RawByteString & APassword, const UnicodeString & AKey, Integer AAlgorithm = PWALG_SIMPLE);
NB_CORE_EXPORT RawByteString SetExternalEncryptedPassword(const RawByteString & APassword);
NB_CORE_EXPORT bool GetExternalEncryptedPassword(const RawByteString & AEncrypted, RawByteString & APassword);

NB_CORE_EXPORT bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len, UnicodeString & Error);

