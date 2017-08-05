
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <WinCrypt.h>

#include "WinSCPSecurity.h"

#define PWALG_SIMPLE_INTERNAL 0x00
#define PWALG_SIMPLE_EXTERNAL 0x01

RawByteString SimpleEncryptChar(uint8_t Ch)
{
  Ch = static_cast<uint8_t>((~Ch) ^ PWALG_SIMPLE_MAGIC);
  return
    PWALG_SIMPLE_STRING.SubString(((Ch & 0xF0) >> 4) + 1, 1) +
    PWALG_SIMPLE_STRING.SubString(((Ch & 0x0F) >> 0) + 1, 1);
}

uint8_t SimpleDecryptNextChar(RawByteString & Str)
{
  if (Str.Length() > 0)
  {
    uint8_t Result = static_cast<uint8_t>(
      ~((((PWALG_SIMPLE_STRING.Pos(Str.c_str()[0]) - 1) << 4) +
        ((PWALG_SIMPLE_STRING.Pos(Str.c_str()[1]) - 1) << 0)) ^ PWALG_SIMPLE_MAGIC));
    Str.Delete(1, 2);
    return Result;
  }
  return 0x00;
}

RawByteString EncryptPassword(UnicodeString UnicodePassword, UnicodeString UnicodeKey, Integer /* Algorithm */)
{
  UTF8String Password = UTF8String(UnicodePassword);
  UTF8String Key = UTF8String(UnicodeKey);

  RawByteString Result("");

  if (!::RandSeed)
  {
    ::Randomize();
    ::RandSeed = 1;
  }
  Password = Key + Password;
  intptr_t Shift = (Password.Length() < PWALG_SIMPLE_MAXLEN) ?
                     static_cast<uint8_t>(random(PWALG_SIMPLE_MAXLEN - static_cast<int>(Password.Length()))) : 0;
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_FLAG)); // Flag
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_INTERNAL)); // Dummy
  Result += SimpleEncryptChar(static_cast<uint8_t>(Password.Length()));
  Result += SimpleEncryptChar(static_cast<uint8_t>(Shift));
  for (intptr_t Index = 0; Index < Shift; ++Index)
    Result += SimpleEncryptChar(static_cast<uint8_t>(random(256)));
  for (intptr_t Index = 0; Index < Password.Length(); ++Index)
    Result += SimpleEncryptChar(static_cast<uint8_t>(Password.c_str()[Index]));
  while (Result.Length() < PWALG_SIMPLE_MAXLEN * 2)
    Result += SimpleEncryptChar(static_cast<uint8_t>(random(256)));
  return Result;
}

UnicodeString DecryptPassword(RawByteString Password, UnicodeString UnicodeKey, Integer /*Algorithm*/)
{
  UTF8String Key = UTF8String(UnicodeKey);
  UTF8String Result("");
  uint8_t Length;

  uint8_t Flag = SimpleDecryptNextChar(Password);
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    /* Dummy = */
    SimpleDecryptNextChar(Password);
    Length = SimpleDecryptNextChar(Password);
  }
  else
    Length = Flag;
  Password.Delete(1, (static_cast<Integer>(SimpleDecryptNextChar(Password)) * 2));
  for (uint8_t Index = 0; Index < Length; ++Index)
    Result += static_cast<char>(SimpleDecryptNextChar(Password));
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    if (Result.SubString(1, Key.Length()) != Key)
      Result = "";
    else
      Result.Delete(1, Key.Length());
  }
  return UnicodeString(Result);
}

RawByteString SetExternalEncryptedPassword(RawByteString Password)
{
  RawByteString Result;
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_FLAG));
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_EXTERNAL));
  Result += UTF8String(BytesToHex(reinterpret_cast<const uint8_t *>(Password.c_str()), Password.Length()));
  return Result;
}

bool GetExternalEncryptedPassword(RawByteString Encrypted, RawByteString & Password)
{
  bool Result =
    (SimpleDecryptNextChar(Encrypted) == PWALG_SIMPLE_FLAG) &&
    (SimpleDecryptNextChar(Encrypted) == PWALG_SIMPLE_EXTERNAL);
  if (Result)
  {
    Password = HexToBytes(UTF8ToString(Encrypted));
  }
  return Result;
}

bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len, UnicodeString & Error)
{
  bool Result = false;

  // Parse the certificate into a context.
  const CERT_CONTEXT * CertContext =
    CertCreateCertificateContext(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, Certificate, ToDWord(Len));

  if (CertContext != nullptr)
  {
    CERT_CHAIN_PARA ChainPara;
    // Retrieve the certificate chain of the certificate
    // (a certificate without a valid root does not have a chain).
    ClearStruct(ChainPara);
    ChainPara.cbSize = sizeof(ChainPara);

    CERT_CHAIN_ENGINE_CONFIG ChainConfig;

    ClearStruct(ChainConfig);
    /*const size_t ChainConfigSize =
      reinterpret_cast<const char *>(&ChainConfig.CycleDetectionModulus) + sizeof(ChainConfig.CycleDetectionModulus) -
      reinterpret_cast<const char *>(&ChainConfig);
    // The hExclusiveRoot and hExclusiveTrustedPeople were added in Windows 7.
    // The CertGetCertificateChain fails with E_INVALIDARG when we include them to ChainConfig.cbSize.
    // DebugAssert(ChainConfigSize == 40);
    // DebugAssert(ChainConfigSize == sizeof(CERT_CHAIN_ENGINE_CONFIG) - sizeof(ChainConfig.hExclusiveRoot) - sizeof(ChainConfig.hExclusiveTrustedPeople));
    ChainConfig.cbSize = ChainConfigSize;*/
    ChainConfig.cbSize = sizeof(CERT_CHAIN_ENGINE_CONFIG);
    ChainConfig.hRestrictedRoot = nullptr;
    ChainConfig.hRestrictedTrust = nullptr;
    ChainConfig.hRestrictedOther = nullptr;
    ChainConfig.cAdditionalStore = 0;
    ChainConfig.rghAdditionalStore = nullptr;
    ChainConfig.dwFlags = CERT_CHAIN_CACHE_END_CERT;
    ChainConfig.dwUrlRetrievalTimeout = 0;
    ChainConfig.MaximumCachedCertificates = 0;
    ChainConfig.CycleDetectionModulus = 0;

    HCERTCHAINENGINE ChainEngine;
    bool ChainEngineResult = CertCreateCertificateChainEngine(&ChainConfig, &ChainEngine) != FALSE;
    if (ChainEngineResult)
    {
      const CERT_CHAIN_CONTEXT * ChainContext = nullptr;
      if (CertGetCertificateChain(ChainEngine, CertContext, nullptr, nullptr, &ChainPara,
        CERT_CHAIN_CACHE_END_CERT |
        CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
        nullptr, &ChainContext))
      {
        CERT_CHAIN_POLICY_PARA PolicyPara;

        PolicyPara.cbSize = sizeof(PolicyPara);
        PolicyPara.dwFlags = 0;
        PolicyPara.pvExtraPolicyPara = nullptr;

        CERT_CHAIN_POLICY_STATUS PolicyStatus;
        PolicyStatus.cbSize = sizeof(PolicyStatus);

        if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
          ChainContext, &PolicyPara, &PolicyStatus))
        {
          // Windows thinks the certificate is valid.
          Result = (PolicyStatus.dwError == S_OK);
          if (!Result)
          {
            Error = FORMAT("Error: %x, Chain index: %d, Element index: %d", PolicyStatus.dwError, PolicyStatus.lChainIndex, PolicyStatus.lElementIndex);
          }
        }

        CertFreeCertificateChain(ChainContext);
      }
      CertFreeCertificateChainEngine(ChainEngine);
    }
    CertFreeCertificateContext(CertContext);
  }
  return Result;
}

