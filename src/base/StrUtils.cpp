
#include <vcl.h>

#include <Classes.hpp>
#include <StrUtils.hpp>
#include <Sysutils.hpp>

#include <openssl/evp.h>

UnicodeString ReplaceStr(const UnicodeString & Str, const UnicodeString & What, const UnicodeString & ByWhat)
{
  return ::StringReplaceAll(Str, What, ByWhat);
}

bool StartsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  return Str.Pos(SubStr) == 1;
}

bool EndsStr(const UnicodeString & SubStr, const UnicodeString & Str)
{
  if (SubStr.Length() > Str.Length())
    return false;
  return Str.SubStr(Str.Length() - SubStr.Length() + 1, SubStr.Length()) == SubStr;
}

NB_CORE_EXPORT bool EndsText(const UnicodeString & SubStr, const UnicodeString & Str)
{
  return EndsStr(SubStr, Str);
}

NB_CORE_EXPORT UnicodeString LeftStr(const UnicodeString & AStr, int32_t Len)
{
  return AStr.SubString(Len);
}


NB_CORE_EXPORT UnicodeString EncodeBase64(const char * AStr, int32_t Len)
{
  if (Len <= 0)
  {
    return UnicodeString();
  }

  // EVP_EncodeBlock output is 4/3 of input, rounded up to multiple of 3
  const int MaxOut = ((Len + 2) / 3) * 4 + 1;
  std::vector<unsigned char> Buffer(MaxOut);
  const int EncodedLen = EVP_EncodeBlock(Buffer.data(),
    reinterpret_cast<const unsigned char *>(AStr), Len);
  if (EncodedLen <= 0)
  {
    return UnicodeString();
  }
  return UnicodeString(reinterpret_cast<const char *>(Buffer.data()), EncodedLen);
}

NB_CORE_EXPORT TBytes DecodeBase64(const UnicodeString & AStr)
{
  ThrowNotImplemented(3135);
  // TBytes Result;
  // return Result;
  return TBytes();
}

bool CharIsInvalidPathCharacter(char C)
{
  switch (C)
  {
  case '<':
  case '>':
  case '?':
  case '/':
  case ',':
  case '*':
  case '+':
  case '=':
  case '[':
  case ']':
  case '|':
  case ':':
  case ';':
  case '"':
  case '\'':
  return true;
  default:
  return false;
  }
  return false;
}
