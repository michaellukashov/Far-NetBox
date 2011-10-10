//---------------------------------------------------------------------------
#include "stdafx.h"

#include "Common.h"
#include "Security.h"
//---------------------------------------------------------------------------
#define PWALG_SIMPLE_INTERNAL 0x00
#define PWALG_SIMPLE_EXTERNAL 0x01
//---------------------------------------------------------------------------
int random(int range)
{
    return (double)rand() / ((double)RAND_MAX / (double)range);
}

//---------------------------------------------------------------------------
std::string SimpleEncryptChar(unsigned char Ch)
{
  Ch = (unsigned char)((~Ch) ^ PWALG_SIMPLE_MAGIC);
  return
    PWALG_SIMPLE_STRING.substr(((Ch & 0xF0) >> 4), 1) +
    PWALG_SIMPLE_STRING.substr(((Ch & 0x0F) >> 0), 1);
}
//---------------------------------------------------------------------------
unsigned char SimpleDecryptNextChar(std::string &Str)
{
  if (Str.size() > 0)
  {
    unsigned char Result = (unsigned char)
      ~((((PWALG_SIMPLE_STRING.find_first_of(Str.c_str()[0])) << 4) +
         ((PWALG_SIMPLE_STRING.find_first_of(Str.c_str()[1])) << 0)) ^ PWALG_SIMPLE_MAGIC);
    Str.erase(0, 2);
    return Result;
  }
  else return 0x00;
}
//---------------------------------------------------------------------------
std::wstring EncryptPassword(std::wstring Password, std::wstring Key, int /* Algorithm */)
{
  std::string Result("");
  int Shift, Index;

  // if (!RandSeed) Randomize();
  std::string Password2 = ::W2MB((Key + Password).c_str());
  Shift = (Password2.size() < PWALG_SIMPLE_MAXLEN) ?
    (unsigned char)random(PWALG_SIMPLE_MAXLEN - Password2.size()) : 0;
  // DEBUG_PRINTF(L"Shift = %d", Shift);
  Result += SimpleEncryptChar((char)PWALG_SIMPLE_FLAG); // Flag
  Result += SimpleEncryptChar((char)PWALG_SIMPLE_INTERNAL); // Dummy
  Result += SimpleEncryptChar((char)Password2.size());
  Result += SimpleEncryptChar((char)Shift);
  for (Index = 0; Index < Shift; Index++)
    Result += SimpleEncryptChar((unsigned char)random(256));
  for (Index = 0; Index < Password2.size(); Index++)
    Result += SimpleEncryptChar(Password2.c_str()[Index]);
  while (Result.size() < PWALG_SIMPLE_MAXLEN * 2)
    Result += SimpleEncryptChar((unsigned char)random(256));
  return ::MB2W(Result.c_str());
}
//---------------------------------------------------------------------------
std::wstring DecryptPassword(std::wstring Password, std::wstring Key, int /* Algorithm */)
{
  std::string Result("");
  int Index;
  unsigned char Length, Flag;
  std::string Password2 = ::W2MB(Password.c_str());
  std::string Key2 = ::W2MB(Key.c_str());
  Flag = SimpleDecryptNextChar(Password2);
  // DEBUG_PRINTF(L"Flag = %x, PWALG_SIMPLE_FLAG = %x", Flag, PWALG_SIMPLE_FLAG);
  if (Flag == (unsigned char)PWALG_SIMPLE_FLAG)
  {
    /* Dummy = */ SimpleDecryptNextChar(Password2);
    Length = SimpleDecryptNextChar(Password2);
    // DEBUG_PRINTF(L"Length = %d", Length);
  }
  else Length = PWALG_SIMPLE_FLAG;
  // DEBUG_PRINTF(L"Length = %d", Length);
  Password2.erase(0, ((int)SimpleDecryptNextChar(Password2))*2);
  for (Index = 0; Index < Length; Index++)
    Result += (char)SimpleDecryptNextChar(Password2);
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    if (Result.substr(0, Key.size()) != Key2) Result = "";
      else Result.erase(0, Key2.size());
  }
  return ::MB2W(Result.c_str());
}
//---------------------------------------------------------------------------
std::wstring SetExternalEncryptedPassword(std::wstring Password)
{
  std::string Result;
  Result += SimpleEncryptChar((char)PWALG_SIMPLE_FLAG);
  Result += SimpleEncryptChar((char)PWALG_SIMPLE_EXTERNAL);
  Result += ::W2MB(StrToHex(Password).c_str());
  return ::MB2W(Result.c_str());
}
//---------------------------------------------------------------------------
bool GetExternalEncryptedPassword(std::wstring Encrypted, std::wstring & Password)
{
  std::string Encrypted2 = ::W2MB(Encrypted.c_str());
  bool Result =
    (SimpleDecryptNextChar(Encrypted2) == PWALG_SIMPLE_FLAG) &&
    (SimpleDecryptNextChar(Encrypted2) == PWALG_SIMPLE_EXTERNAL);
  if (Result)
  {
    Password = ::HexToStr(::MB2W(Encrypted2.c_str()));
  }
  return Result;
}
//---------------------------------------------------------------------------
