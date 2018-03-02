//------------------------------------------------------------------------------
// testnetbox_06.cpp
//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "testutils.h"
//#include <gmock/gmock.h>
#include <UnicodeString.hpp>

#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_CPP11_NO_SHUFFLE
#include <catch/catch.hpp>

// stubs
bool AppendExceptionStackTraceAndForget(TStrings *&MoreMessages)
{
  return false;
}

TCustomFarPlugin *FarPlugin = nullptr;

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

TEST_CASE_METHOD(base_fixture_t, "testIsA01", "netbox")
{
  TObject obj1;
  TPersistent per1;
  SECTION("isa")
  {
    CHECK(isa<TObject>(&obj1));
    CHECK(isa<TObject>(&per1));
    CHECK(isa<TPersistent>(&per1));
    CHECK(!isa<TPersistent>(&obj1));
  }
  SECTION("dyn_cast")
  {
    TObject *obj2 = dyn_cast<TObject>(&obj1);
    CHECK(obj2 != nullptr);
    TPersistent *per2 = dyn_cast<TPersistent>(&per1);
    CHECK(per2 != nullptr);
    TObject *obj3 = dyn_cast<TObject>(per2);
    CHECK(obj3 != nullptr);
    TPersistent *per3 = dyn_cast<TPersistent>(obj3);
    CHECK(per3 != nullptr);
    TPersistent *per4 = dyn_cast<TPersistent>(&obj1);
    CHECK(per4 == nullptr);
  }
}
