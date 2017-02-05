#pragma once

#include <tchar.h>
#include <stdint.h>

enum TEncodeType
{ 
  etUSASCII,
  etUTF8,
  etANSI,
};

TEncodeType DetectUTF8Encoding(const uint8_t * str, intptr_t len);
