//------------------------------------------------------------------------------
// testnetbox_06.cpp
//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

//#include "testutils.h"
//#include <gmock/gmock.h>
#include <UnicodeString.hpp>

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CPP11_NO_SHUFFLE
#include <catch/catch.hpp>

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
};

TEST_CASE_METHOD(base_fixture_t, "testUnicodeString01", "netbox")
{
  SECTION("UnicodeString01")
  {
    UnicodeString a("1");
    CHECK(a == "1");
    CHECK(a != "2");
    CHECK("1" == a);
    CHECK("2" != a);
  }
  SECTION("UTF8String01")
  {
    UTF8String a("ab");
    CHECK(a == "ab");
    CHECK(a != "abc");
    CHECK("ab" == a);
    CHECK("abc" != a);
  }
  SECTION("AnsiString01")
  {
    AnsiString a("ab");
    CHECK(a == "ab");
    CHECK(a != "abc");
    CHECK("ab" == a);
    CHECK("abc" != a);
  }
  SECTION("RawByteString01")
  {
    RawByteString a("ab");
    CHECK(a == "ab");
    CHECK(a != "abc");
    CHECK("ab" == a);
    CHECK("abc" != a);
  }
}

