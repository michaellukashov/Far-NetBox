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

TEST_CASE_METHOD(base_fixture_t, "testUnicodeString02", "netbox")
{
  UnicodeString a("1");
  UnicodeString b(a);
  a.Unique();
  CHECK(a == "1");
  a = "2";
  b.Unique();
  CHECK(b == "1");
  b = "3";
  CHECK(a == "2");
  CHECK(b == "3");
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

TEST_CASE_METHOD(base_fixture_t, "tryfinally01", "netbox")
{
  SECTION("nothrows")
  {
    int a = 1;
    WARN("before try__finally");
    printf("a = %d\n", a);
    try__finally
    {
      WARN("in try__finally");
      a = 2;
      printf("a = %d\n", a);
      // throw std::runtime_error("error in try block");
    },
    __finally
    {
      WARN("in __finally");
      a = 3;
    } end_try__finally
    WARN("after try__finally");
    printf("a = %d\n", a);
    CHECK(a == 3);
  }
  SECTION("throws")
  {
    int a = 1;
    {
      auto throws = [&]()
        try__finally
        {
          WARN("in try__finally");
          a = 2;
          printf("a = %d\n", a);
          throw std::runtime_error("error in try block");
        },
        __finally
        {
          WARN("in __finally");
          a = 3;
        } end_try__finally;
      REQUIRE_THROWS_AS(throws(), std::runtime_error);
    }
    WARN("after try__finally");
    printf("a = %d\n", a);
    CHECK(a == 3);
  }
  SECTION("throws2")
  {
    int a = 1;
    try
    {
      try__finally
      {
        WARN("in try__finally");
        a = 2;
        printf("a = %d\n", a);
        throw std::runtime_error("error in try block");
      },
      __finally
      {
        WARN("in __finally");
        a = 3;
      } end_try__finally
    }
    catch(std::runtime_error &)
    {
      WARN("in catch");
    }
    printf("a = %d\n", a);
    CHECK(a == 3);
  }
}

class TBase
{
public:
  ROProperty<int> Data{nb::bind(&TBase::GetData, this)};
  ROProperty<int> Data2{[&]()->int { return FData; }};
  ROProperty<bool> AutoSort{[&]()->bool { return FAutoSort; }};
  ROProperty<UnicodeString> Data3{[&]()->UnicodeString { return FString; }};

  void Modify()
  {
    FData = 2;
    FString = "43";
    FAutoSort = false;
  }
protected:
  virtual int GetData() const { return FData; }
private:
  int FData = 1;
  UnicodeString FString = "42";
  bool FAutoSort = true;
};

//template<int s> struct Wow;
//Wow<sizeof(TBase)> wow;

class TDerived : public TBase
{
protected:
  virtual int GetData() const override { return FData; }
private:
  int FData = 3;
};

TEST_CASE_METHOD(base_fixture_t, "properties01", "netbox")
{
  SECTION("TBase")
  {
    TBase obj1;
    CHECK(obj1.Data == 1);
    CHECK(obj1.Data2 == 1);
    bool x = obj1.AutoSort;
    CHECK(x == true);
    CHECK(obj1.AutoSort == true);
    CHECK(true == obj1.AutoSort);
    CHECK("42" == obj1.Data3);
    CHECK(obj1.Data3() == "42");
    obj1.Modify();
    CHECK(obj1.AutoSort == false);
    CHECK(false == obj1.AutoSort);
    CHECK("43" == obj1.Data3);
    CHECK(obj1.Data3() == "43");
  }
  SECTION("TDerived")
  {
    TDerived d1;
    CHECK(d1.Data == 3);
    CHECK(3 == d1.Data);
    CHECK(1 == d1.Data2);
    d1.Modify();
    CHECK(3 == d1.Data);
    CHECK(2 == d1.Data2);
  }
}
