
#include <vcl.h>

#include "WideStrUtils.hpp"

nb::TEncodeType DetectUTF8Encoding(const RawByteString & S)
{
  return nb::DetectUTF8Encoding(nb::ToUInt8Ptr(S.c_str()), S.Length());
}
