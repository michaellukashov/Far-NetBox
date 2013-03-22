//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "WinSCPSecurity.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define PWALG_SIMPLE_INTERNAL 0x00
#define PWALG_SIMPLE_EXTERNAL 0x01
//---------------------------------------------------------------------------
int random(int range)
{
  return static_cast<int>(static_cast<double>(rand()) / (static_cast<double>(RAND_MAX) / range));
}

//---------------------------------------------------------------------------
RawByteString SimpleEncryptChar(unsigned char Ch)
{
  Ch = (unsigned char)((~Ch) ^ PWALG_SIMPLE_MAGIC);
  return
    PWALG_SIMPLE_STRING.SubString(((Ch & 0xF0) >> 4) + 1, 1) +
    PWALG_SIMPLE_STRING.SubString(((Ch & 0x0F) >> 0) + 1, 1);
}
//---------------------------------------------------------------------------
unsigned char SimpleDecryptNextChar(RawByteString & Str)
{
  if (Str.Length() > 0)
  {
    unsigned char Result = (unsigned char)
      ~((((PWALG_SIMPLE_STRING.Pos(Str.c_str()[0])-1) << 4) +
         ((PWALG_SIMPLE_STRING.Pos(Str.c_str()[1])-1) << 0)) ^ PWALG_SIMPLE_MAGIC);
    Str.Delete(1, 2);
    return Result;
  }
  else { return 0x00; }
}
//---------------------------------------------------------------------------
RawByteString EncryptPassword(const UnicodeString & UnicodePassword, const UnicodeString & UnicodeKey, Integer /* Algorithm */)
{
  UTF8String Password = UnicodePassword;
  UTF8String Key = UnicodeKey;

  RawByteString Result("");
  intptr_t Shift, Index;

  if (!RandSeed) Randomize();
  Password = Key + Password;
  Shift = (Password.Length() < PWALG_SIMPLE_MAXLEN) ?
    static_cast<unsigned char>(random(PWALG_SIMPLE_MAXLEN - static_cast<int>(Password.Length()))) : 0;
  Result += SimpleEncryptChar(static_cast<unsigned char>(PWALG_SIMPLE_FLAG)); // Flag
  Result += SimpleEncryptChar(static_cast<unsigned char>(PWALG_SIMPLE_INTERNAL)); // Dummy
  Result += SimpleEncryptChar(static_cast<unsigned char>(Password.Length()));
  Result += SimpleEncryptChar(static_cast<unsigned char>(Shift));
  for (Index = 0; Index < Shift; ++Index)
    Result += SimpleEncryptChar(static_cast<unsigned char>(random(256)));
  for (Index = 0; Index < Password.Length(); ++Index)
    Result += SimpleEncryptChar(static_cast<unsigned char>(Password.c_str()[Index]));
  while (Result.Length() < PWALG_SIMPLE_MAXLEN * 2)
    Result += SimpleEncryptChar(static_cast<unsigned char>(random(256)));
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString DecryptPassword(const RawByteString & Password, const UnicodeString & UnicodeKey, Integer /* Algorithm */)
{
  UTF8String Key = UnicodeKey;
  UTF8String Result("");
  Integer Index;
  unsigned char Length, Flag;

  RawByteString Psw = Password;
  Flag = SimpleDecryptNextChar(Psw);
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    /* Dummy = */ SimpleDecryptNextChar(Psw);
    Length = SimpleDecryptNextChar(Psw);
  }
  else
    Length = Flag;
  Psw.Delete(1, (static_cast<Integer>(SimpleDecryptNextChar(Psw))*2));
  for (Index = 0; Index < Length; ++Index)
    Result += static_cast<char>(SimpleDecryptNextChar(Psw));
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    if (Result.SubString(1, Key.Length()) != Key)
      Result = "";
    else
      Result.Delete(1, Key.Length());
  }
  return UnicodeString(Result);
}
//---------------------------------------------------------------------------
RawByteString SetExternalEncryptedPassword(const RawByteString & Password)
{
  RawByteString Result;
  Result += SimpleEncryptChar(static_cast<unsigned char>(PWALG_SIMPLE_FLAG));
  Result += SimpleEncryptChar(static_cast<unsigned char>(PWALG_SIMPLE_EXTERNAL));
  Result += UTF8String(BytesToHex(reinterpret_cast<const unsigned char *>(Password.c_str()), Password.Length()));
  return Result;
}
//---------------------------------------------------------------------------
bool GetExternalEncryptedPassword(const RawByteString & Encrypted, RawByteString & Password)
{
  RawByteString Enc = Encrypted;
  bool Result =
    (SimpleDecryptNextChar(Enc) == PWALG_SIMPLE_FLAG) &&
    (SimpleDecryptNextChar(Enc) == PWALG_SIMPLE_EXTERNAL);
  if (Result)
  {
    Password = HexToBytes(UnicodeString(UTF8String(Enc.c_str(), Enc.Length())));
  }
  return Result;
}
//---------------------------------------------------------------------------

