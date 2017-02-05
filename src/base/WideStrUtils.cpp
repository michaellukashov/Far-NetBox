#include "WideStrUtils.hpp"

TEncodeType DetectUTF8Encoding(const RawByteString & S)
{
  return ::DetectUTF8Encoding(reinterpret_cast<const uint8_t *>(S.c_str()), S.Length());
}
