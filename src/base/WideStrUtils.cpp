
#include <vcl.h>

#include "WideStrUtils.hpp"

nb::TEncodeType DetectUTF8Encoding(RawByteString S)
{
  return nb::DetectUTF8Encoding(reinterpret_cast<const uint8_t *>(S.c_str()), S.Length());
}
