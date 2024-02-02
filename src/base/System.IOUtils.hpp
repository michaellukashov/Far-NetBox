#pragma once

#include <Global.h>
#include <Classes.hpp>

class TFile
{
public:
  static UnicodeString ReadAllText(const UnicodeString & FileName);
  static void WriteAllText(const UnicodeString & FileName, const UnicodeString & Text);
  static TFileStream * OpenRead(const UnicodeString & FileName);
  static TFileStream * OpenWrite(const UnicodeString & FileName);
  static TBytes ReadAllBytes(const UnicodeString & FileName);
};

