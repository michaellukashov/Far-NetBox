
#include <vcl.h>
#pragma hdrstop

#include <limits>
#include <wincrypt.h>
#include <Common.h>
#include "WinSCPSecurity.h"

//#pragma package(smart_init)

constexpr const uint8_t PWALG_SIMPLE_INTERNAL = 0x00;
constexpr const uint8_t PWALG_SIMPLE_EXTERNAL = 0x01;
constexpr const uint8_t PWALG_SIMPLE_INTERNAL2 = 0x02;
constexpr const char * PWALG_SIMPLE_STRING = "0123456789ABCDEF";

RawByteString SimpleEncryptChar(uint8_t Ch)
{
  Ch = static_cast<uint8_t>((~Ch) ^ PWALG_SIMPLE_MAGIC);
  RawByteString Result("..");
  const RawByteString PwStr(PWALG_SIMPLE_STRING);
  Result[1] = PwStr[((Ch & 0xF0) >> 4) + 1];
  Result[2] = PwStr[((Ch & 0x0F) >> 0) + 1];
  return Result;
}

uint8_t SimpleDecryptNextChar(RawByteString & Str)
{
  if (Str.Length() >= 2)
  {
    const RawByteString PwStr(PWALG_SIMPLE_STRING);
    const uint8_t Result = static_cast<uint8_t>(
      ~((((PwStr.Pos(Str.c_str()[0]) - 1) << 4) +
         ((PwStr.Pos(Str.c_str()[1]) - 1) << 0)) ^ PWALG_SIMPLE_MAGIC));
    Str.Delete(1, 2);
    return Result;
  }
  return 0x00;
}

RawByteString EncryptPassword(const UnicodeString & UnicodePassword, const UnicodeString & UnicodeKey, Integer /* Algorithm */)
{
  UTF8String Password = UTF8String(UnicodePassword);
  const UTF8String Key = UTF8String(UnicodeKey);

  RawByteString Result;

  // if (!::RandSeed) { ::Randomize(); }
  Password = Key + Password;
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_FLAG)); // Flag
  const int32_t Len = Password.Length();
  if (Len > std::numeric_limits<uint8_t>::max())
  {
    Result += SimpleEncryptChar(PWALG_SIMPLE_INTERNAL2);
    Result += SimpleEncryptChar(static_cast<uint8_t>(Len >> 8));
    Result += SimpleEncryptChar(static_cast<uint8_t>(Len & 0xFF));
  }
  else
  {
    Result += SimpleEncryptChar(PWALG_SIMPLE_INTERNAL);
    Result += SimpleEncryptChar(static_cast<uint8_t>(Len));
  }
  const int32_t DataLen =
    (Result.Length() / 2) +
    1 + // Shift
    Password.Length();
  const int32_t Shift = (DataLen < PWALG_SIMPLE_MAXLEN) ? Random(PWALG_SIMPLE_MAXLEN - DataLen) : 0;
  Result += SimpleEncryptChar(static_cast<uint8_t>(Shift));
  for (int32_t Index = 0; Index < Shift; Index++)
    Result += SimpleEncryptChar(static_cast<uint8_t>(Random(256)));
  for (int32_t Index = 0; Index < Password.Length(); Index++)
    Result += SimpleEncryptChar(Password.c_str()[Index]);
  while (Result.Length() < PWALG_SIMPLE_MAXLEN * 2)
    Result += SimpleEncryptChar(static_cast<uint8_t>(Random(256)));
  return Result;
}

UnicodeString DecryptPassword(const RawByteString & APassword, const UnicodeString & UnicodeKey, Integer /*Algorithm*/)
{
  RawByteString Password = APassword;
  int32_t Length;
  const uint8_t Flag = SimpleDecryptNextChar(Password);
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    const uint8_t Version = SimpleDecryptNextChar(Password);
    if (Version == PWALG_SIMPLE_INTERNAL)
    {
      Length = SimpleDecryptNextChar(Password);
    }
    else if (Version == PWALG_SIMPLE_INTERNAL2)
    {
      Length = (nb::ToInt32(SimpleDecryptNextChar(Password)) << 8) + SimpleDecryptNextChar(Password);
    }
    else
    {
      Length = -1;
    }
  }
  else
  {
    Length = Flag;
  }

  UTF8String Result;
  if (Length >= 0)
  {
    Password.Delete(1, static_cast<int32_t>(SimpleDecryptNextChar(Password)) * 2);
    for (int32_t Index = 0; Index < Length; Index++)
    {
      Result += static_cast<char>(SimpleDecryptNextChar(Password));
    }
    if (Flag == PWALG_SIMPLE_FLAG)
    {
      const UTF8String Key = UTF8String(UnicodeKey);
      if (Result.SubString(1, Key.Length()) != Key)
      {
        Result = UTF8String();
      }
      else
      {
        Result.Delete(1, Key.Length());
      }
    }
  }
  return UnicodeString(Result);
}

RawByteString SetExternalEncryptedPassword(const RawByteString & Password)
{
  RawByteString Result;
  Result += SimpleEncryptChar(PWALG_SIMPLE_FLAG);
  Result += SimpleEncryptChar(PWALG_SIMPLE_EXTERNAL);
  Result += UTF8String(BytesToHex(nb::ToUInt8Ptr(Password.c_str()), Password.Length()));
  return Result;
}

bool GetExternalEncryptedPassword(const RawByteString & AEncrypted, RawByteString & Password)
{
  RawByteString Encrypted = AEncrypted;
  const bool Result =
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
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, Certificate, nb::ToDWord(Len));

  if (CertContext == nullptr)
  {
    Error = L"Cannot create certificate context";
  }
  else
  {
    CERT_CHAIN_PARA ChainPara{};
    // Retrieve the certificate chain of the certificate
    // (a certificate without a valid root does not have a chain).
    nb::ClearStruct(ChainPara);
    ChainPara.cbSize = sizeof(ChainPara);

    CERT_CHAIN_ENGINE_CONFIG ChainConfig{};

    nb::ClearStruct(ChainConfig);
    /*const size_t ChainConfigSize =
      reinterpret_cast<const char *>(&ChainConfig.CycleDetectionModulus) + sizeof(ChainConfig.CycleDetectionModulus) -
      reinterpret_cast<const char *>(&ChainConfig);
    // The hExclusiveRoot and hExclusiveTrustedPeople were added in Windows 7.
    // The CertGetCertificateChain fails with E_INVALIDARG when we include them to ChainConfig.cbSize.
    DebugAssert(ChainConfigSize == 40);
    DebugAssert(ChainConfigSize == sizeof(CERT_CHAIN_ENGINE_CONFIG) - sizeof(ChainConfig.hExclusiveRoot) - sizeof(ChainConfig.hExclusiveTrustedPeople));*/
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

    HCERTCHAINENGINE ChainEngine{INVALID_HANDLE_VALUE};
    const bool ChainEngineResult = CertCreateCertificateChainEngine(&ChainConfig, &ChainEngine) != FALSE;
    if (!ChainEngineResult)
    {
      Error = L"Cannot create certificate chain engine";
    }
    else
    {
      const CERT_CHAIN_CONTEXT * ChainContext = nullptr;
      if (!CertGetCertificateChain(ChainEngine, CertContext, nullptr, nullptr, &ChainPara,
            CERT_CHAIN_CACHE_END_CERT |
            CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
            nullptr, &ChainContext))
      {
        Error = L"Cannot get certificate chain";
      }
      else
      {
        CERT_CHAIN_POLICY_PARA PolicyPara;

        PolicyPara.cbSize = sizeof(PolicyPara);
        PolicyPara.dwFlags = 0;
        PolicyPara.pvExtraPolicyPara = nullptr;

        CERT_CHAIN_POLICY_STATUS PolicyStatus{};
        PolicyStatus.cbSize = sizeof(PolicyStatus);

        if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, ChainContext, &PolicyPara, &PolicyStatus))
        {
          Error = L"Cannot verify certificate chain policy";
        }
        else
        {
          const DWORD PolicyError = PolicyStatus.dwError;
          // Windows thinks the certificate is valid.
          Result = (PolicyError == S_OK);
          if (!Result)
          {
            const UnicodeString ErrorStr = SysErrorMessage(PolicyError);
            Error = FORMAT(L"Error: %x (%s), Chain index: %d, Element index: %d", PolicyError, ErrorStr, PolicyStatus.lChainIndex, PolicyStatus.lElementIndex);
          }
        }
        if (ChainContext)
          CertFreeCertificateChain(ChainContext);
      }
      if (ChainEngine != INVALID_HANDLE_VALUE)
        CertFreeCertificateChainEngine(ChainEngine);
    }
    if (CertContext)
      CertFreeCertificateContext(CertContext);
  }
  return Result;
}

