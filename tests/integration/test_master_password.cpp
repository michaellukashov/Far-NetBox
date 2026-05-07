#define NOMINMAX
#include <SecureString.h>
#include <Cryptography.h>
#include <Sysutils.hpp>
#include <cassert>
#include <cstdio>

// Test: AES256CreateVerifier and AES256Verify round-trip
void test_verifier_roundtrip()
{
  UnicodeString password = L"TestPassword123!";
  RawByteString verifier;
  AES256CreateVerifier(password, verifier);
  assert(!verifier.IsEmpty());
  assert(AES256Verify(password, verifier));
  assert(!AES256Verify(L"WrongPassword", verifier));
  std::printf("  PASS: AES256 verifier round-trip\n");
}

// Test: IsValidPassword for various strengths
void test_password_strength()
{
  // Too short
  assert(IsValidPassword(L"ab") <= 0);
  // No mixed case
  assert(IsValidPassword(L"abcdefgh") <= 0);
  // Good password (mixed case + digits)
  assert(IsValidPassword(L"TestPass123") > 0);
  std::printf("  PASS: password strength validation\n");
}

// Test: TSecureString with AES256 verifier
void test_secure_string_with_verifier()
{
  UnicodeString password = L"SecureTest456!";
  TSecureString secure(password);
  assert(!secure.IsEmpty());
  assert(secure.GetValue() == password);

  RawByteString verifier;
  AES256CreateVerifier(password, verifier);
  assert(AES256Verify(secure.GetValue(), verifier));
  secure.Clear();
  assert(secure.IsEmpty());
  std::printf("  PASS: TSecureString with AES256 verifier\n");
}

// Test: TSecureString move semantics
void test_secure_string_move()
{
  TSecureString s1(L"moveable");
  TSecureString s2(std::move(s1));
  assert(s2.GetValue() == L"moveable");
  assert(s1.IsEmpty());

  TSecureString s3;
  s3 = std::move(s2);
  assert(s3.GetValue() == L"moveable");
  assert(s2.IsEmpty());
  std::printf("  PASS: TSecureString move semantics\n");
}

// Test: Multiple passwords with same verifier
void test_verifier_consistency()
{
  UnicodeString password = L"ConsistentPassword1!";
  RawByteString verifier;
  AES256CreateVerifier(password, verifier);

  // Verify same password multiple times
  for (int i = 0; i < 10; ++i)
  {
    assert(AES256Verify(password, verifier));
  }
  std::printf("  PASS: verifier consistency across multiple checks\n");
}

int main()
{
  std::printf("Master Password Integration Tests\n");
  std::printf("=================================\n");
  test_verifier_roundtrip();
  test_password_strength();
  test_secure_string_with_verifier();
  test_secure_string_move();
  test_verifier_consistency();
  std::printf("\nAll tests passed.\n");
  return 0;
}
