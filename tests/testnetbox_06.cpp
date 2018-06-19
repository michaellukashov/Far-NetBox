//------------------------------------------------------------------------------
// testnetbox_06.cpp
//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <functional>

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
//  ROProperty<int> Data{ [&]()->int { return GetData(); } };
  ROProperty2<int, TBase> Data{ this, [](TBase* self)->int { return self->GetData(); } };
//  ROProperty<int> Data2{ fastdelegate::FastDelegate0<int>(std::bind([this]()->int { return FData; }) ) };
  ROProperty2<int, TBase> Data2{ this, [](TBase* self)->int { return self->FData; } };
//  ROProperty2<int, TBase> Data2_1{ this, nb::bind(&TBase::GetData, this) };
  ROProperty2<bool, TBase> AutoSort{ this, [](TBase* self)->bool { return self->FAutoSort; } };
  ROProperty2<UnicodeString, TBase> Data3{ this, [](TBase* self)->UnicodeString { return self->FString; } };

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

class TBase2
{
public:
  RWProperty<int> Data{ nb::bind(&TBase2::GetData, this), nb::bind(&TBase2::SetData, this) };
  RWProperty<int> Data1{ [&]()->int { return GetData(); }, [&](int Value) { SetData(Value); } };
  RWProperty<int> Data2{ [&]()->int { return FData; }, [&](int Value) { FData = Value; } };
  RWProperty<bool> AutoSort{ [&]()->bool { return FAutoSort; }, [&](bool Value) { FAutoSort = Value; } };
  RWProperty<UnicodeString> Data3{ [&]()->UnicodeString { return FString; }, [&](const UnicodeString Value) { FString = Value; } };

  RWProperty<int> Data4{ nb::bind(&TBase2::GetData, this), [&](int Value) { FData = Value; } };
  RWProperty<int> Data4_1{ [&]()->int { return GetData(); }, [&](int Value) { FData = Value; } };
  RWProperty<int> Data5{ [&]()->int { return FData; }, nb::bind(&TBase2::SetData, this) };

  void Modify()
  {
    FData = 2;
    FString = "43";
    FAutoSort = false;
  }
protected:
  virtual int GetData() const { return FData; }
  virtual void SetData(int Value) { FData = Value; }
private:
  int FData = 1;
  UnicodeString FString = "42";
  bool FAutoSort = true;
};

//template<int s> struct Wow;
//Wow<sizeof(TBase2)> wow;

TEST_CASE_METHOD(base_fixture_t, "properties02", "netbox")
{
  SECTION("TBase2")
  {
    TBase2 obj1;
    CHECK(obj1.Data == 1);
    CHECK(obj1.Data1 == 1);
    obj1.Data = 2;
    CHECK(obj1.Data1 == 2);
    CHECK(obj1.Data == 2);
    CHECK(obj1.Data2 == 2);
    obj1.Data2 = 3;
    CHECK(obj1.Data2 == 3);
    CHECK(3 == obj1.Data2);
    CHECK(true == obj1.AutoSort);
    obj1.AutoSort = false;
    CHECK(false == obj1.AutoSort);
    CHECK("42" == obj1.Data3());
    obj1.Data3 = "43";
    CHECK("43" == obj1.Data3());
  }
}

TEST_CASE_METHOD(base_fixture_t, "properties03", "netbox")
{
  SECTION("TBase2::Data4")
  {
    TBase2 obj1;
    CHECK(1 == obj1.Data4);
    obj1.Data4 = 4;
    CHECK(4 == obj1.Data4);
    CHECK(4 == obj1.Data4_1);
    obj1.Data4_1 = 41;
    CHECK(41 == obj1.Data4_1);
    CHECK(41 == obj1.Data4);
  }
  SECTION("TBase2::Data5")
  {
    TBase2 obj2;
    CHECK(1 == obj2.Data5);
    obj2.Data5 = 5;
    CHECK(5 == obj2.Data5);
    CHECK(5 == obj2.Data4);
  }
}

namespace prop_01 {

template <typename T>
class ROProp
{
  using TProp = TransientFunction<T()>;
  TProp getter_;
public:
  ROProp(const TProp& Getter) : getter_(Getter) {}

  friend bool inline operator==(const ROProp &lhs, const T &rhs)
  {
    return lhs.getter_() == rhs;
  }
  friend bool inline operator==(const T &lhs, const ROProp &rhs)
  {
    return rhs.getter_() == lhs;
  }
};

class TBase1
{
public:
//  ROProp<int> Data1{ nb::bind(&TBase1::GetData1, this) };
  ROProp<int> Data1{ nb::bind(&TBase1::GetData1, this) };
  ROProp<int> Data2{ [&]() { return GetData1(); } };
private:
  int GetData1() { return 42; }
};

} // namespace prop_01

namespace prop_02 {

template <typename T>
class ROProp : TransientFunction<const T()> // 16 bytes
{
  typedef TransientFunction<const T()> base_t;
public:
  using base_t::base_t;

  friend bool inline operator==(const ROProp &lhs, const T& rhs)
  {
    return lhs() == rhs;
  }
  friend bool inline operator==(const T& lhs, const ROProp &rhs)
  {
    return rhs() == lhs;
  }
};

template <typename T>
class RWProp // : TransientFunction<const T()>, TransientFunction<void(const T&)> // 32 bytes
{
  typedef TransientFunction<const T()> ro_base_t;
  typedef TransientFunction<void(const T&)> wr_base_t;
  ro_base_t getter_;
  wr_base_t setter_;
public:
  RWProp() = delete;
  template<typename T1, typename T2>
  RWProp(T1& getter, T2& setter) :
    getter_(getter),
    setter_(setter)
  {
//    Expects(_getter.m_Target != nullptr);
//    Expects(_setter.m_Target != nullptr);
  }

  void operator=(const T& Value)
  {
    setter_.operator()(Value);
  }

  friend bool inline operator==(const RWProp &lhs, const T& rhs)
  {
    return lhs.getter_() == rhs;
  }
  friend bool inline operator==(const T& lhs, const RWProp &rhs)
  {
    return rhs.getter_() == lhs;
  }
};

class TBase1
{
public:
  ROProp<int> Data1{ [&]() { return GetData1(); } };
  ROProp<const UnicodeString> StrData1{ [&]() { return GetStrData1(); } };
  ROProp<UnicodeString> StrData2{ [&]() { return GetStrData2(); } };
  RWProp<int> RWData1{ [&]() { return GetRWData1(); }, [&](const int Value) { SetRWData1(Value); }};
  RWProp<UnicodeString> RWStrData1{ [&]() { return GetRWStrData1(); }, [&](const UnicodeString& Value) { SetRWStrData1(Value); }};
private:
  int GetData1() { return 42; }
  UnicodeString GetStrData1() const { return "42"; }
  UnicodeString GetStrData2() { return "42"; }
  int GetRWData1() { return FIntData1; }
  void SetRWData1(int Value) { FIntData1 = Value; }
  UnicodeString GetRWStrData1() { return FStrData1; }
  void SetRWStrData1(const UnicodeString Value) { FStrData1 = Value; }

  int FIntData1{1};
  UnicodeString FStrData1{"test"};
};

//template<int s> struct CheckSizeT;
//CheckSizeT<sizeof(ROProp<UnicodeString>)> checkSize;

} // namespace prop_02

TEST_CASE_METHOD(base_fixture_t, "properties04", "netbox")
{
  SECTION("TBase1::Data1")
  {
    prop_02::TBase1 obj;
    {
      bool res = (obj.Data1 == 42);
      CHECK(res);
    }
    {
      bool res = (42 == obj.Data1);
      CHECK(res);
    }
  }
  SECTION("TBase1::StrData1")
  {
    prop_02::TBase1 obj;
    {
      bool res = (obj.StrData1 == "42");
      CHECK(res);
    }
    {
      bool res = ("42" == obj.StrData1);
      CHECK(res);
    }
  }
  SECTION("TBase1::StrData2")
  {
    prop_02::TBase1 obj;
    {
      bool res = (obj.StrData2 == "42");
      CHECK(res);
    }
    {
      bool res = ("42" == obj.StrData2);
      CHECK(res);
    }
  }
  SECTION("TBase1::RWData1")
  {
    prop_02::TBase1 obj;
    {
      bool res = (obj.RWData1 == 1);
      CHECK(res);
    }
    {
      bool res = (1 == obj.RWData1);
      CHECK(res);
    }
    {
      obj.RWData1 = 2;
      bool res = (2 == obj.RWData1);
      CHECK(res);
    }
    {
      bool res = ("test" == obj.RWStrData1);
      CHECK(res);
      obj.RWStrData1 = "42";
      res = ("42" == obj.RWStrData1);
      CHECK(res);
      res = (obj.RWStrData1 == "42");
      CHECK(res);
    }
  }
  SECTION("TBase1::Data1")
  {
    prop_01::TBase1 obj;
    {
      bool res = (obj.Data1 == 42);
      CHECK(res);
    }
    {
      bool res = (42 == obj.Data1);
      CHECK(res);
    }
  }
}
