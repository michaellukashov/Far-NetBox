#define NOMINMAX
#include <SecureString.h>
#include <cassert>
#include <cstdio>

// Test: Construction from UnicodeString
void test_construction()
{
  TSecureString s(L"hello");
  assert(!s.IsEmpty());
  assert(s.GetValue() == L"hello");
  std::printf("  PASS: construction from UnicodeString\n");
}

// Test: Default construction
void test_default_construction()
{
  TSecureString s;
  assert(s.IsEmpty());
  assert(s.GetValue().IsEmpty());
  std::printf("  PASS: default construction\n");
}

// Test: SetValue
void test_set_value()
{
  TSecureString s;
  assert(s.IsEmpty());
  s.SetValue(L"world");
  assert(!s.IsEmpty());
  assert(s.GetValue() == L"world");
  std::printf("  PASS: SetValue\n");
}

// Test: Clear
void test_clear()
{
  TSecureString s(L"data");
  assert(!s.IsEmpty());
  s.Clear();
  assert(s.IsEmpty());
  assert(s.GetValue().IsEmpty());
  std::printf("  PASS: Clear\n");
}

// Test: Move constructor
void test_move_constructor()
{
  TSecureString s1(L"move_me");
  TSecureString s2(std::move(s1));
  assert(s2.GetValue() == L"move_me");
  assert(s1.IsEmpty());
  std::printf("  PASS: move constructor\n");
}

// Test: Move assignment
void test_move_assignment()
{
  TSecureString s1(L"first");
  TSecureString s2(L"second");
  s2 = std::move(s1);
  assert(s2.GetValue() == L"first");
  assert(s1.IsEmpty());
  std::printf("  PASS: move assignment\n");
}

// Test: Overwrite existing value
void test_overwrite()
{
  TSecureString s(L"old");
  s.SetValue(L"new");
  assert(s.GetValue() == L"new");
  std::printf("  PASS: overwrite existing value\n");
}

// Test: Empty string
void test_empty_string()
{
  TSecureString s(L"");
  assert(s.IsEmpty());
  std::printf("  PASS: empty string\n");
}

// Test: Large string
void test_large_string()
{
  UnicodeString large(10000, L'A');
  TSecureString s(large);
  assert(s.GetValue() == large);
  assert(!s.IsEmpty());
  std::printf("  PASS: large string (10000 chars)\n");
}

// Test: Multiple clears
void test_multiple_clears()
{
  TSecureString s(L"test");
  s.Clear();
  s.Clear();
  s.Clear();
  assert(s.IsEmpty());
  std::printf("  PASS: multiple clears\n");
}

int main()
{
  std::printf("TSecureString Unit Tests\n");
  std::printf("=======================\n");
  test_construction();
  test_default_construction();
  test_set_value();
  test_clear();
  test_move_constructor();
  test_move_assignment();
  test_overwrite();
  test_empty_string();
  test_large_string();
  test_multiple_clears();
  std::printf("\nAll tests passed.\n");
  return 0;
}
