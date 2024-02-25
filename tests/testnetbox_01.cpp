﻿//------------------------------------------------------------------------------
// testnetbox_01.cpp
//------------------------------------------------------------------------------


#include <vcl.h>
#pragma hdrstop

// #include "leak_detector.h"
// extern int GC_find_leak;

#include <Classes.hpp>
#include <time.h>
#include <Winhttp.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <functional>

#include <FileBuffer.h>

#include <Common.h>
#include <FarPlugin.h>
#include <FastDelegate.h>
// #include <CppProperties.h>
#include <Property.hpp>

#include "testutils.h"
//#include "xproperty/xproperty.hpp"
#include <type_traits.h>

template <typename T>
class propertyBase
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  T dummy = T();
  T &obj  = dummy;

  explicit propertyBase(T &Value):
    obj(Value)
  {}
};

// Read/Write specialization
template <typename T, bool canWrite = true, bool isPod = true>
class Property : private propertyBase<T>
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  Property() = default;

  Property(const Property<T, true, true> &Value):
    propertyBase<T>(Value.obj)
  {}

  explicit Property(T &Value):
    propertyBase<T>(Value)
  {}

  T & operator =(const Property<T, true, true> & Value)
  {
    this->obj = Value.obj;
    return this->obj;
  }

  T & operator =(const T & Value)
  {
    this->obj = Value;
    return this->obj;
  }

  T & operator =(T & Value)
  {
    this->obj = Value;

    return this->obj;
  }

  operator T &() const
  {
    return this->obj;
  }

};

// Read only specialization
template <typename T>
class Property<T, false, true> : private propertyBase<T>
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  Property() = default;

  Property(const Property<T, false, true> &Value):
    propertyBase<T>(Value.obj)
  {}

  explicit Property(T &Value) :
    propertyBase<T>(Value)
  {}

  Property &operator=(const Property &Value)
  {
    this->obj = Value.obj;
    return *this;
  }

  operator T() const
  {
    return this->obj;
  }

};

// Read/Write non-pod specialization
template <typename T>
class Property<T, true, false> : public T
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  using T::T;

  explicit Property(const T &Value):
    T(Value)
  {}
};

// Read only non-pod specialization
template <typename T>
class Property<T, false, false> : public T
{
  const T *const _obj = this;
public:
  using T::T;

  explicit Property(const T &Value):
    T(Value)
  {}

  const T * const operator->() const
  {
    return this->_obj;
  }
};

template <typename T>
using roProperty = Property<T, false, std::is_pod<T>::value>;

template <typename T>
using rwProperty = Property<T, true, std::is_pod<T>::value>;

//------------------------------------------------------------------------------
// stub
// TCustomFarPlugin * FarPlugin = nullptr;
//------------------------------------------------------------------------------

void FreeIEConfig(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* ie_config)
{
  if (ie_config->lpszAutoConfigUrl)
    GlobalFree(ie_config->lpszAutoConfigUrl);
  if (ie_config->lpszProxy)
    GlobalFree(ie_config->lpszProxy);
  if (ie_config->lpszProxyBypass)
    GlobalFree(ie_config->lpszProxyBypass);
}

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
  base_fixture_t()
  {
    INFO("base_fixture_t ctor");
    ::SetGlobals(new TTestGlobalFunctions());
  }

  virtual ~base_fixture_t()
  {
    INFO("base_fixture_t dtor");
    TGlobalsIntf * Intf = GetGlobals();
    SAFE_DESTROY_EX(TGlobalsIntf, Intf);
    ::SetGlobals(nullptr);
  }
public:
  void AnonFunction() { printf("in base_fixture_t::AnonFunction\n"); }
  void AnonFunction2(void * Param)
  {
    // printf("in base_fixture_t::AnonFunction2: param: %d\n", *(int *)Param);
  }
protected:
private:
};

//------------------------------------------------------------------------------
// mocks
/*
bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages)
{
  return false;
}
*/
//------------------------------------------------------------------------------

//TEST_CASE("base tests", "netbox")
//SECTION("")

TEST_CASE_METHOD(base_fixture_t, "test1", "netbox")
{

  TList list;
  REQUIRE(0 == list.GetCount());
  TObject obj1;
  TObject obj2;
  if (1)
  {
    list.Add(&obj1);
    REQUIRE(1 == list.GetCount());
    list.Add(&obj2);
    REQUIRE(2 == list.GetCount());
    REQUIRE(0 == list.IndexOf(&obj1));
    REQUIRE(1 == list.IndexOf(&obj2));
  }
  list.Clear();
  if (1)
  {
    REQUIRE(0 == list.GetCount());
    list.Insert(0, &obj1);
    REQUIRE(1 == list.GetCount());
    list.Insert(0, &obj2);
    REQUIRE(2 == list.GetCount());
    REQUIRE(1 == list.IndexOf(&obj1));
    REQUIRE(0 == list.IndexOf(&obj2));
  }
  if (1)
  {
    list.Delete(1);
    REQUIRE(1 == list.GetCount());
    REQUIRE(-1 == list.IndexOf(&obj1));
    REQUIRE(0 == list.IndexOf(&obj2));
    REQUIRE(1 == list.Add(&obj1));
    list.Delete(0);
    REQUIRE(0 == list.IndexOf(&obj1));
    REQUIRE(0 == list.Remove(&obj1));
    REQUIRE(-1 == list.IndexOf(&obj1));
    REQUIRE(0 == list.GetCount());
  }
  if (1)
  {
    list.Add(&obj1);
    list.Add(&obj2);
    list.Extract(&obj1);
    REQUIRE(1 == list.GetCount());
    REQUIRE(0 == list.IndexOf(&obj2));
    list.Add(&obj1);
    REQUIRE(2 == list.GetCount());
    list.Move(0, 1);
    REQUIRE(0 == list.IndexOf(&obj1));
    REQUIRE(1 == list.IndexOf(&obj2));
  }
}

TEST_CASE_METHOD(base_fixture_t, "test2", "netbox")
{
  UnicodeString str;
  bool res;
  if (1)
  {
    TStringList strings;
    REQUIRE(0 == strings.GetCount());
    REQUIRE(0 == strings.Add(L"line 1"));
    REQUIRE(1 == strings.GetCount());
    str = strings.GetString(0);
    // DEBUG_PRINTF(L"str = %s", str.c_str());
    {
      res = W2MB(str.c_str()) == "line 1";
      REQUIRE(res);
    }
    strings.SetString(0, L"line 0");
    REQUIRE(1 == strings.GetCount());
    str = strings.GetString(0);
    REQUIRE(W2MB(str.c_str()) == "line 0");
    strings.SetString(0, L"line 00");
    REQUIRE(1 == strings.GetCount());
    REQUIRE(W2MB(strings.GetString(0).c_str()) == "line 00");
    strings.Add(L"line 11");
    REQUIRE(2 == strings.GetCount());
    REQUIRE(W2MB(strings.GetString(1).c_str()) == "line 11");
    strings.Delete(1);
    REQUIRE(1 == strings.GetCount());
  }
  TStringList strings;
  if (1)
  {
    REQUIRE(0 == strings.GetCount());
    strings.Add(L"line 1");
    str = strings.GetText();
    INFO("str = " << str);
    // DEBUG_PRINTF(L"str = %s", BytesToHex(RawByteString(str.c_str(),  str.Length()), true, L',').c_str());
    INFO("str = " << BytesToHex(RawByteString(str.c_str(),  str.Length()), true, L','));
    REQUIRE(_wcsicmp(str.c_str(), L"line 1\x0D\x0A") == 0);
  }
  if (1)
  {
    strings.Add(L"line 2");
    REQUIRE(2 == strings.GetCount());
    str = strings.GetText();
//    INFO(L"str = " << str);
    REQUIRE(W2MB(str.c_str()) == "line 1\r\nline 2\r\n");
    strings.Insert(0, L"line 0");
    REQUIRE(3 == strings.GetCount());
    str = strings.GetText();
//    INFO(L"str = " << str);
    CHECK(W2MB(str.c_str()) == "line 0\r\nline 1\r\nline 2\r\n");
    strings.SetObject(0, nullptr);
    UnicodeString str = strings.GetString(0);
    REQUIRE(W2MB(str.c_str()) == "line 0");
  }
  {
    strings.SetString(0, L"line 12");
    REQUIRE(W2MB(strings.GetString(0).c_str()) == "line 12");
  }
}

TEST_CASE_METHOD(base_fixture_t, "test3", "netbox")
{
  UnicodeString Text = L"text text text text text1\ntext text text text text2\n";
  TStringList Lines;
  Lines.SetText(Text);
  REQUIRE(2 == Lines.GetCount());
  INFO("Lines 0 = " << Lines.GetString(0));
  INFO("Lines 1 = " << Lines.GetString(1));
  REQUIRE("text text text text text1" == W2MB(Lines.GetString(0).c_str()));
  REQUIRE("text text text text text2" == W2MB(Lines.GetString(1).c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "test4", "netbox")
{
  UnicodeString Text = L"text, text text, text text1\ntext text text, text text2\n";
  INFO("Text = '" << Text << "'");
  TStringList Lines;
  Lines.SetCommaText(Text);
  INFO("Lines = '" << Lines.GetText() << "'");
  CHECK(0 == wcscmp(L"text", Lines.GetString(0).c_str()));
  CHECK(0 == wcscmp(L" text text", Lines.GetString(1).c_str()));
  INFO("Lines[2] = '" << Lines.GetString(2) << "'");
  CHECK(0 == wcscmp(L" text text1", Lines.GetString(2).c_str()));
  CHECK(0 == wcscmp(L"text text text", Lines.GetString(3).c_str()));
  REQUIRE_EQUAL(5, Lines.GetCount());
  CHECK(0 == wcscmp(L" text text2", Lines.GetString(4).c_str()));
  UnicodeString Text2 = Lines.GetCommaText();
  INFO("Text2 = '" << Text2 << "'");
  CHECK(0 == wcscmp(L"\"text\",\" text text\",\" text text1\",\"text text text\",\" text text2\"", Text2.c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "test5", "netbox")
{
  TStringList Lines;
  TObject obj1;
  Lines.InsertObject(0, L"line 1", &obj1);
  REQUIRE(&obj1 == Lines.Objects[0]);
}

TEST_CASE_METHOD(base_fixture_t, "test6", "netbox")
{
  TStringList Lines;
  Lines.Add(L"bbb");
  Lines.Add(L"aaa");
  // INFO("Lines = " << W2MB(Lines.Text.c_str()).c_str());
  {
    Lines.SetSorted(true);
    // INFO("Lines = " << W2MB(Lines.Text.c_str()).c_str());
    REQUIRE("aaa" == W2MB(Lines.GetString(0).c_str()));
    REQUIRE(2 == Lines.GetCount());
  }
  {
    Lines.SetSorted(false);
    Lines.Add(L"Aaa");
    Lines.SetCaseSensitive(true);
    Lines.SetSorted(true);
    REQUIRE(3 == Lines.GetCount());
    // INFO("Lines = " << W2MB(Lines.Text.c_str()).c_str());
    REQUIRE("aaa" == W2MB(Lines.GetString(0).c_str()));
    REQUIRE("Aaa" == W2MB(Lines.GetString(1).c_str()));
    REQUIRE("bbb" == W2MB(Lines.GetString(2).c_str()));
  }
}

TEST_CASE_METHOD(base_fixture_t, "test7", "netbox")
{
  TStringList Lines;
  {
    Lines.Add(L"bbb");
    INFO("before try");
    try
    {
      INFO("before SCOPE_EXIT");
      SCOPE_EXIT
      {
        INFO("in SCOPE_EXIT");
        REQUIRE(1 == Lines.GetCount());
      };
      // throw std::exception("");
      INFO("after SCOPE_EXIT");
    }
    catch (...)
    {
      INFO("in catch(...) block");
    }
    INFO("after try");
    Lines.Add(L"aaa");
    REQUIRE(2 == Lines.GetCount());
  }
  Lines.Clear();
  Lines.BeginUpdate();
  {
    Lines.Add(L"bbb");
    INFO("before block");
    {
      INFO("before SCOPE_EXIT");
      SCOPE_EXIT
      {
        INFO("in SCOPE_EXIT");
        REQUIRE(1 == Lines.GetCount());
        Lines.EndUpdate();
      };
      // throw std::exception("");
      INFO("after SCOPE_EXIT");
    }
    INFO("after block");
    Lines.Add(L"aaa");
    REQUIRE(2 == Lines.GetCount());
  }
  Lines.Clear();
  int cnt = 0;
  TStringList * Lines1 = new TStringList();
  int cnt1 = 0;
  TStringList * Lines2 = new TStringList();
  {
    Lines.BeginUpdate();
    cnt++;
    Lines1->BeginUpdate();
    cnt1++;
    Lines2->Add(L"bbb");
    INFO("before block");
    try
    {
      INFO("before BOOST_SCOPE_EXIT");
      SCOPE_EXIT
      {
        INFO("in BOOST_SCOPE_EXIT");
        Lines.EndUpdate();
        cnt--;
        Lines1->EndUpdate();
        cnt1--;
        delete Lines1;
        Lines1 = nullptr;
        delete Lines2;
        Lines2 = nullptr;
      };
      REQUIRE(1 == cnt);
      REQUIRE(1 == cnt1);
      REQUIRE(1 == Lines2->GetCount());
      throw std::exception("");
      INFO("after BOOST_SCOPE_EXIT_END");
    }
    catch (const std::exception & ex)
    {
      INFO("in catch block: " << ex.what());
      REQUIRE(nullptr == Lines1);
      REQUIRE(nullptr == Lines2);
    }
    INFO("after block");
    REQUIRE(0 == cnt);
    REQUIRE(0 == cnt1);
    REQUIRE(nullptr == Lines1);
    REQUIRE(nullptr == Lines2);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test8", "netbox")
{
  UnicodeString ProgramsFolder;
  UnicodeString DefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
  REQUIRE(DefaultPuttyPathOnly == L"\\PuTTY\\putty.exe");
  REQUIRE(L"" == ::ExcludeTrailingBackslash(::IncludeTrailingBackslash(ProgramsFolder)));
}

TEST_CASE_METHOD(base_fixture_t, "test9", "netbox")
{
  UnicodeString Folder = L"C:\\Program Files\\Putty";
  INFO("ExtractFileDir = " << ::ExtractFileDir(Folder));
  CHECK(L"C:\\Program Files\\" == ::ExtractFileDir(Folder));
  INFO("1");
  CHECK(L"C:\\Program Files\\" == ::ExtractFilePath(Folder));
  INFO("2");
  INFO("GetCurrentDir = " << ::GetCurrentDir());
  INFO("3");
  CHECK(::GetCurrentDir().Length() > 0);
  INFO("4");
  CHECK(DirectoryExists(::GetCurrentDir()));
  INFO("5");
}

TEST_CASE_METHOD(base_fixture_t, "test10", "netbox")
{
  TDateTime dt1(23, 58, 59, 102);
  INFO("dt1 = " << dt1);
  REQUIRE(dt1 > 0.0);
  unsigned short H, M, S, MS;
  dt1.DecodeTime(H, M, S, MS);
  REQUIRE(H == 23);
  REQUIRE(M == 58);
  REQUIRE(S == 59);
  REQUIRE(MS == 102);
}

TEST_CASE_METHOD(base_fixture_t, "test11", "netbox")
{
  TDateTime dt1 = EncodeDateVerbose(2009, 12, 29);
  INFO("dt1 = " << dt1);
  // bg::date::ymd_type ymd(2009, 12, 29);
  // INFO("ymd.year = " << ymd.year << ", ymd.month = " << ymd.month << ", ymd.day = " << ymd.day);
  unsigned short Y, M, D;
  dt1.DecodeDate(Y, M, D);
  INFO("Y = " << Y << ", M = " << M << ", D = " << D);
  REQUIRE(Y == 2009);
  REQUIRE(M == 12);
  REQUIRE(D == 29);
  int DOW = ::DayOfWeek(dt1);
  REQUIRE(3 == DOW);
}

TEST_CASE_METHOD(base_fixture_t, "test12", "netbox")
{
//  INFO("Is2000 = " << Is2000());
  INFO("IsWin7 = " << IsWin7());
//  INFO("IsExactly2008R2 = " << IsExactly2008R2());
  TDateTime dt = ::EncodeDateVerbose(2009, 12, 29);
  FILETIME ft = ::DateTimeToFileTime(dt, dstmWin);
  INFO("ft.dwLowDateTime = " << ft.dwLowDateTime);
  INFO("ft.dwHighDateTime = " << ft.dwHighDateTime);
}

TEST_CASE_METHOD(base_fixture_t, "test13", "netbox")
{
  UnicodeString str_value = ::IntToStr(1234);
  INFO("str_value = " << str_value);
  REQUIRE(W2MB(str_value.c_str()) == "1234");
  intptr_t int_value = ::StrToIntPtr(L"1234");
  INFO("int_value = " << int_value);
  REQUIRE(int_value == 1234);
}

TEST_CASE_METHOD(base_fixture_t, "test14", "netbox")
{
  TStringList Strings1;
  TStringList Strings2;
  Strings1.AddStrings(&Strings2);
  REQUIRE(0 == Strings1.GetCount());
  Strings2.Add(L"lalalla");
  Strings1.AddStrings(&Strings2);
  REQUIRE(1 == Strings1.GetCount());
  REQUIRE(L"lalalla" == Strings1.GetString(0));
}

TEST_CASE_METHOD(base_fixture_t, "test15", "netbox")
{
  UnicodeString res = ::IntToHex(10, 2);
  INFO("res = " << res);
  REQUIRE(res == L"0A");
}

TEST_CASE_METHOD(base_fixture_t, "test16", "netbox")
{
  {
    UnicodeString Name1 = L"1";
    UnicodeString Name2 = L"2";
    intptr_t res = ::AnsiCompareIC(Name1, Name2);
    INFO("res = " << res);
    REQUIRE(res != 0);
    res = ::AnsiCompare(Name1, Name2);
    INFO("res = " << res);
    REQUIRE(res != 0);
  }
  {
    UnicodeString Name1 = L"abc";
    UnicodeString Name2 = L"ABC";
    intptr_t res = ::AnsiCompareIC(Name1, Name2);
    INFO("res = " << res);
    REQUIRE(res == 0);
    res = ::AnsiCompare(Name1, Name2);
    INFO("res = " << res);
    REQUIRE(res != 0);
  }
  {
    UnicodeString Name1 = L"Unlimited";
    UnicodeString Name2 = L"Unlimited";
    REQUIRE(::AnsiSameText(Name1, Name2));
  }
}

TEST_CASE_METHOD(base_fixture_t, "test17", "netbox")
{
  TStringList List1;
  List1.SetText(L"123\n456");
  REQUIRE(2 == List1.GetCount());
  INFO("List1.GetString(0] = " << List1.GetString(0));
  REQUIRE("123" == W2MB(List1.GetString(0).c_str()));
  INFO("List1.GetString(1] = " << List1.GetString(1));
  REQUIRE("456" == W2MB(List1.GetString(1).c_str()));
  List1.Move(0, 1);
  INFO("List1.GetString(0] = " << List1.GetString(0));
  REQUIRE("456" == W2MB(List1.GetString(0).c_str()));
  INFO("List1.GetString(1] = " << List1.GetString(1));
  REQUIRE("123" == W2MB(List1.GetString(1).c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "test18", "netbox")
{
  {
    UnicodeString Key = L"Interface";
    UnicodeString Res = ::CutToChar(Key, L'\\', false);
    REQUIRE(Key.IsEmpty());
    REQUIRE("Interface" == W2MB(Res.c_str()));
  }
  {
    UnicodeString Key = L"Interface\\SubKey";
    UnicodeString Res = ::CutToChar(Key, L'\\', false);
    REQUIRE("SubKey" == W2MB(Key.c_str()));
    REQUIRE("Interface" == W2MB(Res.c_str()));
  }
}

TEST_CASE_METHOD(base_fixture_t, "test19", "netbox")
{
  TStringList Strings1;
  Strings1.Add(L"Name1=Value1");
  REQUIRE(0 == Strings1.IndexOfName(L"Name1"));
}

TEST_CASE_METHOD(base_fixture_t, "test20", "netbox")
{
  TDateTime DateTime = Now();
  unsigned short H, M, S, MS;
  DateTime.DecodeTime(H, M, S, MS);
  // UnicodeString str = ::FormatDateTime(L"HH:MM:SS", DateTime);
  UnicodeString str = FORMAT(L"%02d:%02d:%02d", H, M, S);
  INFO("str = " << str.c_str());
  // REQUIRE(str == L"20:20:20");
}

TEST_CASE_METHOD(base_fixture_t, "test21", "netbox")
{
  UnicodeString str = ::FormatFloat(L"#,##0", 23.456);
  INFO("str = " << str);
  // REQUIRE(str.c_str() == L"23.46");
  REQUIRE("23.46" == W2MB(str.c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "test22", "netbox")
{
  UnicodeString FileName = L"testfile";
  ::DeleteFileW(FileName.c_str());
  std::string str = "test string";
  {
    unsigned int CreateAttrs = FILE_ATTRIBUTE_NORMAL;
    HANDLE FileHandle = ::CreateFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               nullptr, CREATE_ALWAYS, CreateAttrs, 0);
    REQUIRE(FileHandle != INVALID_HANDLE_VALUE);
    TStream * FileStream = new TSafeHandleStream(FileHandle);
    TFileBuffer * BlockBuf = new TFileBuffer();
    // BlockBuf->SetSize(1024);
    // BlockBuf->SetPosition(0);
    BlockBuf->Insert(0, str.c_str(), str.size());
    INFO("BlockBuf->GetSize = " << BlockBuf->GetSize());
    REQUIRE(BlockBuf->GetSize() == str.size());
    BlockBuf->WriteToStream(FileStream, BlockBuf->GetSize());
    delete FileStream; FileStream = nullptr;
    delete BlockBuf; BlockBuf = nullptr;
    ::CloseHandle(FileHandle);
    INFO("FileName1 = " << FileName);
    REQUIRE(FileExists(FileName));
  }
  {
    INFO("FileName2 = " << FileName);
    TSearchRec Rec;
    REQUIRE(FileSearchRec(FileName, Rec));
  }
  {
    HANDLE File = ::CreateFile(
                    FileName.c_str(),
                    GENERIC_READ,
                    0, // FILE_SHARE_READ,
                    nullptr,
                    OPEN_ALWAYS, // OPEN_EXISTING,
                    0, // FILE_ATTRIBUTE_NORMAL, // 0,
                    nullptr);
    DEBUG_PRINTF(L"File = %p", File);
    TStream * FileStream = new TSafeHandleStream(File);
    TFileBuffer * BlockBuf = new TFileBuffer();
    BlockBuf->ReadStream(FileStream, str.size(), true);
    INFO("BlockBuf->GetSize = " << BlockBuf->GetSize());
    REQUIRE(BlockBuf->GetSize() == str.size());
    delete FileStream; FileStream = nullptr;
    delete BlockBuf; BlockBuf = nullptr;
    ::CloseHandle(File);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test23", "netbox")
{
  UnicodeString Dir1 = L"subdir1";
  UnicodeString Dir2 = L"subdir1/subdir2";
  RemoveDir(Dir2);
  RemoveDir(Dir1);
  INFO("DirectoryExists(Dir2) = " << DirectoryExists(Dir2));
  REQUIRE(!DirectoryExists(Dir2));
  ForceDirectories(Dir2);
  REQUIRE(DirectoryExists(Dir2));
  RemoveDir(Dir2);
  ForceDirectories(Dir2);
  REQUIRE(DirectoryExists(Dir2));
  REQUIRE(::RecursiveDeleteFile(Dir1, false));
  REQUIRE(!DirectoryExists(Dir1));
}

TEST_CASE_METHOD(base_fixture_t, "test24", "netbox")
{
  TDateTime now = Now();
  INFO("now = " << (double)now);
  REQUIRE(now > 0.0);
}

TEST_CASE_METHOD(base_fixture_t, "test25", "netbox")
{
#if 0
  GC_find_leak = 1;
  int * i = (int *)malloc(sizeof(int));
  CHECK_LEAKS();
#endif
}

//------------------------------------------------------------------------------

class CBaseClass
{
protected:
  char * m_name;
public:
  CBaseClass(char * name) : m_name(name) {}
  void SimpleMemberFunction(int num, char * str)
  {
    // printf("In SimpleMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  int SimpleMemberFunctionReturnsInt(int num, char * str)
  {
    // printf("In SimpleMemberFunctionReturnsInt in %s. Num=%d, str = %s\n", m_name, num, str);
    return -1;
  }
  void ConstMemberFunction(int num, char * str) const
  {
    // printf("In ConstMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  virtual void SimpleVirtualFunction(int num, char * str)
  {
    // printf("In SimpleVirtualFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  static void StaticMemberFunction(int num, char * str)
  {
    // printf("In StaticMemberFunction. Num=%d, str =%s\n", num, str);
  }
};

TEST_CASE_METHOD(base_fixture_t, "test26", "netbox")
{
  typedef nb::FastDelegate2<void, int, char *> TEvent1;
  typedef nb::FastDelegate2<int, int, char *> TEvent2;

  SECTION("SimpleMemberFunction")
  {
    CBaseClass a("Base A");
    TEvent1 sig = nb::bind(&CBaseClass::SimpleMemberFunction, &a);
    sig(11, "abcd");
  }
  SECTION("SimpleMemberFunctionReturnsInt")
  {
    CBaseClass a("Base A");
    TEvent2 sig = nb::bind(&CBaseClass::SimpleMemberFunctionReturnsInt, &a);
    int Result = sig(10, "abc");
    INFO("Result = " << Result);
    REQUIRE(Result == -1);
  }
}
//------------------------------------------------------------------------------
TEST_CASE_METHOD(base_fixture_t, "test27", "netbox")
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ProxyConfig = {0};
    if (!WinHttpGetIEProxyConfigForCurrentUser(&ProxyConfig))
    {
        DWORD Err = GetLastError();
        switch (Err)
        {
        case ERROR_FILE_NOT_FOUND:
            DEBUG_PRINTF("The error is ERROR_FILE_NOT_FOUND");
            break;
        case ERROR_WINHTTP_INTERNAL_ERROR:
            DEBUG_PRINTF("ERROR_WINHTTP_INTERNAL_ERROR");
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            DEBUG_PRINTF("ERROR_NOT_ENOUGH_MEMORY");
            break;
        default:
            DEBUG_PRINTF("Look up error in header file.");
        }
    }
    else
    {
        //no error so check the proxy settings and free any strings
        INFO("AutoConfigDetect is: " << ProxyConfig.fAutoDetect);
        if (nullptr != ProxyConfig.lpszAutoConfigUrl)
        {
            INFO("AutoConfigURL is: " << ProxyConfig.lpszAutoConfigUrl);
        }
        if (nullptr != ProxyConfig.lpszProxy)
        {
            INFO("AutoConfigProxy is: " << ProxyConfig.lpszProxy);
        }
        if (nullptr != ProxyConfig.lpszProxyBypass)
        {
            INFO("AutoConfigBypass is: " << ProxyConfig.lpszProxyBypass);
        }
        FreeIEConfig(&ProxyConfig);
    }
}
//------------------------------------------------------------------------------
TEST_CASE_METHOD(base_fixture_t, "test28", "netbox")
{
  DEBUG_PRINTF(L"1");
  if (1)
  {
    UnicodeString Buf;
    UnicodeString DestFileName(L"FileName");
    TFileBuffer AsciiBuf;
    AsciiBuf.SetSize(0x1000);
    DEBUG_PRINTF(L"2");
    Buf.Clear();
    Buf.SetLength(MAX_PATH * 2);
    DEBUG_PRINTF(L"AsciiBuf.GetSize = %lld", AsciiBuf.GetSize());
    swprintf_s(const_cast<wchar_t *>(Buf.c_str()), Buf.Length(), L"C%s %lld %s",
      L"",
      AsciiBuf.GetSize(),
      DestFileName.c_str());
    DEBUG_PRINTF(L"3");
    INFO("Buf1 = " << Buf);
    DEBUG_PRINTF(L"Buf = %s", Buf.c_str());
    REQUIRE(Buf.GetLength() > 0);
  }
  if (1)
  {
    UnicodeString Buf;
    Buf.SetLength(20);
    UnicodeString DestFileName(L"FileName");
    TFileBuffer AsciiBuf;
    // swprintf_s causes error
    swprintf(const_cast<wchar_t *>(Buf.c_str()), L"C%s %lld %s",
      L"",
      AsciiBuf.GetSize(),
      DestFileName.c_str());
    INFO("Buf2 = " << Buf);
    REQUIRE(AnsiString(Buf) == "C 0 FileName");
    REQUIRE("C 0 FileName" == AnsiString(Buf));
    REQUIRE(AnsiString(Buf) == AnsiString("C 0 FileName"));
    REQUIRE(Buf == L"C 0 FileName");
    REQUIRE(L"C 0 FileName" == Buf);
  }
  DEBUG_PRINTF(L"4");
  if (1)
  {
    // Ctrl = T, Line = 1338899268 0 1338899268 0
    UnicodeString Line = L"1338899268 0 1338899268 0";
    unsigned long MTime, ATime;
    if (swscanf(Line.c_str(), L"%ld %*d %ld %*d",  &MTime, &ATime) != 2)
    {
      FAIL("swscanf");
    }
    REQUIRE(MTime == 1338899268LU);
    REQUIRE(ATime == 1338899268LU);
  }
  DEBUG_PRINTF(L"5");
  if (1)
  {
    intptr_t errCode = 0xFF;
    wchar_t codeNum[16];
    DEBUG_PRINTF(L"6");
    // swprintf_s(codeNum, sizeof(codeNum), L"[0x%08X]", errCode);  // Causes AV x64
    swprintf(codeNum, L"[0x%08X]", (unsigned int)errCode);
    DEBUG_PRINTF(L"7");
    INFO("codeNum = " << W2MB(codeNum));
    DEBUG_PRINTF(L"8");
    REQUIRE(AnsiString(codeNum) == AnsiString("[0x000000FF]")); // Causes AV x64
    // REQUIRE(AnsiString(codeNum) == AnsiString(""));
    REQUIRE(wcscmp(codeNum, L"[0x000000FF]") == 0);
  }
  DEBUG_PRINTF(L"9");
}
//------------------------------------------------------------------------------
#if 0
class Foo
{
public:
    Foo(int j) { i=new int[j]; }
    ~Foo() { delete i; }
private:
    int* i;
};

class Bar: public Foo
{
public:
    Bar(int j): Foo(j) { i=new char[j]; }
    ~Bar() { delete i; }
private:
    char* i;
};


TEST_CASE_METHOD(base_fixture_t, "test29", "netbox")
{
  Foo* f=new Foo(100);
  Foo* b=new Bar(200);
  *f=*b;
  delete f;
  delete b;
}
#endif
//------------------------------------------------------------------------------
TEST_CASE_METHOD(base_fixture_t, "test30", "netbox")
{
  // printf("0\n");
  INFO("Instructions1");
  UnicodeString Instructions = L"Using keyboard authentication.\x0A\x0A\x0APlease enter your password.";
  // printf("1\n");
  INFO("Instructions = " << Instructions);
  UnicodeString Instructions2 = ReplaceStrAll(Instructions, L"\x0D\x0A", L"\x01");
  // printf("2\n");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0A\x0D", L"\x01");
  // printf("3\n");
  Instructions2 = ReplaceStrAll(Instructions2, L"\x0A", L"\x01");
  // printf("4\n");
  Instructions2 = ReplaceStrAll(Instructions2, L"\x0D", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions2, L"\x01", L"\x0D\x0A");
  INFO("Instructions2 = " << Instructions2);
  CHECK(wcscmp(Instructions2.c_str(), UnicodeString(L"Using keyboard authentication.\x0D\x0A\x0D\x0A\x0D\x0APlease enter your password.").c_str()) == 0);
}
//------------------------------------------------------------------------------

namespace test {

template<typename F>
class scope_guard0
{
public:
  explicit scope_guard0(F&& f) : m_f(std::move(f)) {}
  ~scope_guard0() noexcept(false) { m_f(); }

private:
  const F m_f;
  NB_DISABLE_COPY(scope_guard0)
};

template<typename F, typename P>
class scope_guard1
{
  NB_DISABLE_COPY(scope_guard1)
public:
  explicit scope_guard1(F&& f, P p) noexcept : m_f(std::move(f)), m_p(p) {}
  ~scope_guard1() noexcept(false) { m_f(m_p); }

private:
  const F m_f;
  P m_p;
};

class make_scope_guard
{
public:
  template<typename F>
  scope_guard0<F> operator<<(F&& f) { return scope_guard0<F>(std::forward<F>(f)); }
};


} // namespace test
typedef nb::FastDelegate0<void> TAnonFunction;

class TTestAnonFunc
{
public:
  void AnonFunction() { printf("in AnonFunction\n"); }
};

TEST_CASE_METHOD(base_fixture_t, "test_scope_exit2", "netbox")
{
  // TTestAnonFunc TestAnonFunc;
  // TAnonFunction func = nb::bind(&TTestAnonFunc::AnonFunction, &TestAnonFunc);
  auto test_lambda1 = [&]()
  {
     // printf("in TEST_CASE_METHOD test_lambda1\n");
  };
  auto test_lambda2 = [&]()
  {
     // printf("in TEST_CASE_METHOD test_lambda2\n");
  };
  int Param = 42;
  // SCOPE_EXIT2(base_fixture_t::AnonFunction2, (void*)&Param);
  // detail::kscope_guard2<nb::FastDelegate1<void, int>, int> guard(nb::bind(&base_fixture_t::AnonFunction2, this), Param);
  // printf("in TEST_CASE_METHOD test_scope_exit2\n");
  const auto ANONYMOUS_VARIABLE(scope_exit_guard) = test::make_scope_guard() << \
    test_lambda2;
  test_lambda1();
//  TAnonFunction func(test_lambda2, this);
}

#if 0
namespace {

class TBase
{
public:
  MAKE_OBSERVED()
  XPROPERTY(int, TBase, Data, 42);
public:
  virtual int GetData() const = 0;
};

class TDerived : public TBase
{
public:
  virtual int GetData() const
  {
    return 42;
  }
};

} // namespace

TEST_CASE_METHOD(base_fixture_t, "testProperty01", "netbox")
{
  TDerived d;
  int data = d.Data;
  CHECK(data == 42);
  d.Data = 43;
  CHECK(d.Data == 43);
}

#endif // if 0

namespace {

class TBase
{
public:
  ROProperty<int> Data{nb::bind(&TBase::GetData, this)};
  ROProperty<int> Data2{nb::bind(&TBase::GetData2, this)};
  ROProperty<int> Data3{nb::bind(&TBase::GetData3, this)};

  RWProperty<int> RWData1{nb::bind(&TBase::GetRWData1, this), nb::bind(&TBase::SetRWData1, this)};
  RWProperty<UnicodeString> RWData2{nb::bind(&TBase::GetRWData2, this), nb::bind(&TBase::SetRWData2, this)};
public:
  virtual int GetData() const = 0;
  virtual int GetData2() const { return 41; }
  virtual int GetData3() const { return 41; }

  virtual int GetRWData1() const { return FRWData1; }
  virtual void SetRWData1(int Value) { FRWData1 = Value; }
  virtual UnicodeString GetRWData2() const { return FRWData2; }
  virtual void SetRWData2(const UnicodeString & Value) { FRWData2 = Value; }
private:
  int GetDataInternal() const { return GetData(); }

  int FRWData1 = 41;
  UnicodeString FRWData2 = "41";
};

class TDerived : public TBase
{
public:
  virtual int GetData() const override { return 42; }
  virtual int GetData2() const override { return 42; }
  virtual int GetData3() const override { return 42; }

  virtual int GetRWData1() const override { return FRWData1; }
  virtual void SetRWData1(int Value) override { FRWData1 = Value; }

  virtual UnicodeString GetRWData2() const override { return FRWData2; }
  virtual void SetRWData2(const UnicodeString & Value) override { FRWData2 = Value; }
private:
  int FRWData1 = 42;
  UnicodeString FRWData2 = "42";
};

class TBase2
{
public:
  // ROProperty<int> Data{nb::bind(&TBase::GetData, this)};
  ROProperty<int> Data2{ nb::bind(&TBase2::GetData2, this) };
  ROProperty<int> Data3{ nb::bind(&TBase2::GetData3, this) };

  RWProperty<int> RWData1{ nb::bind(&TBase2::GetRWData1, this), nb::bind(&TBase2::SetRWData1, this) };
  RWProperty<UnicodeString> RWData2{ nb::bind(&TBase2::GetRWData2, this), nb::bind(&TBase2::SetRWData2, this) };
  RWProperty<UnicodeString> RWData3{ nb::bind(&TBase2::GetRWData3, this), nb::bind(&TBase2::SetRWData3, this) };
  RWProperty<TDateTime> Modification{ nb::bind(&TBase2::GetModification, this), nb::bind(&TBase2::SetModification, this) };
private:
  int GetData2() const { return 41; }
  int GetData3() const { return 41; }

  int GetRWData1() const { return FRWData1; }
  void SetRWData1(int Value) { FRWData1 = Value; }
  UnicodeString GetRWData2() const { return FRWData2; }
  void SetRWData2(const UnicodeString & Value) { FRWData2 = Value; }
  UnicodeString GetRWData3() const { return FRWData3; }
  void SetRWData3(const UnicodeString & Value) { FRWData3 = Value; }
  TDateTime GetModification() const { return FModification; }
  void SetModification(const TDateTime & Value) { FModification = Value; }
private:
  int FRWData1 = 41;
  UnicodeString FRWData2 = "41";
  UnicodeString FRWData3 = "FRWData3";
  TDateTime FModification = TDateTime(10, 10, 10, 10);
};

class TDerived2 : public TBase2
{
public:
  ROProperty<UnicodeString> ROProp1{nb::bind(&TDerived2::GetROProp1, this)};
  RWProperty<UnicodeString> RWProp1{nb::bind(&TDerived2::GetRWProp1, this), nb::bind(&TDerived2::SetRWProp1, this)};
  RWProperty<UnicodeString> RWProp2{nb::bind(&TDerived2::GetRWProp2Const, this), nb::bind(&TDerived2::SetRWProp1, this)};
private:
  UnicodeString GetROProp1() { return FROProp1; }
  UnicodeString GetROProp1Const() const { return FROProp1; }

  UnicodeString GetRWProp1() const { return FRWProp1; }
  UnicodeString GetRWProp2Const() const { return FRWProp1; }
  void SetRWProp1(const UnicodeString & Value) { FRWProp1 = Value; }

  UnicodeString FROProp1 = "42";
  UnicodeString FRWProp1 = "RW";
};

} // namespace

TEST_CASE_METHOD(base_fixture_t, "testProperty02", "netbox")
{
  // printf("1\n");
  // printf("2\n");
  {
    TDerived d;
    int data = d.Data;
    // printf("3\n");
    CHECK(data == 42);
  }
  // printf("4\n");
  if (1)
  {
    TDerived d;
    int data2 = d.Data2;
    // printf("5\n");
    CHECK(data2 == 42);
  }
  // printf("6\n");
  {
    TDerived d;
    int data3 = d.Data3;
    // printf("7\n");
    CHECK(data3 == 42);
  }
  // printf("8\n");
//  d.Data = 43;
//  CHECK(d.Data == 43);
  {
    TDerived d;
    CHECK(d.RWData1 == 42);
    d.RWData1 = 43;
    CHECK(d.RWData1 == 43);
  }
  // printf("9\n");
  {
    TDerived d;
    CHECK(d.RWData2() == "42");
    d.RWData2 = "43";
    CHECK(d.RWData2() == "43");
  }
  // printf("10\n");
  SECTION("RWData2")
  {
    TBase2 b2;
    // printf("11\n");
    CHECK(b2.Data2 == 41);
    // printf("12\n");
    CHECK(b2.RWData2() == "41");
    // printf("13\n");
    b2.RWData2 = "42";
    // printf("13.1\n");
    CHECK(b2.RWData2() == "42");
    // printf("13.2\n");
//    CHECK(b2.RWData3() == "FRWData3");
    // printf("14\n");
    CHECK(b2.Modification() == TDateTime(10, 10, 10, 10));
    TBase2 b3 = b2;
    CHECK(b3.RWData2() == "42");
    CHECK(b3.RWData3() == "FRWData3");
    // printf("15\n");
    b3.RWData3 = "FRWData3-mod";
    CHECK(b3.RWData3() == "FRWData3-mod");
    CHECK(b3.Modification() == TDateTime(10, 10, 10, 10));
    TDateTime dt(10, 20, 59, 10);
    // printf("16\n");
    b3.Modification = dt;
    CHECK(b3.Modification() == TDateTime(10, 20, 59, 10));
  }
}

TEST_CASE_METHOD(base_fixture_t, "testProperty03", "netbox")
{
  SECTION("ROProp1")
  {
    TDerived2 d2;
    // printf("1\n");
    CHECK(d2.ROProp1() == "42");
    // printf("2\n");
  }
  SECTION("RWProp1")
  {
    TDerived2 d2;
    // printf("3\n");
    CHECK(d2.RWProp1() == "RW");
    // printf("4\n");
    d2.RWProp1 = "RW2";
    CHECK(d2.RWProp1() == "RW2");
    CHECK(d2.RWProp2() == "RW2");
    CHECK(L"RW2" == d2.RWProp2);
  }
}

namespace experimental {

template <typename Owner, typename T>
class ROProperty
{
CUSTOM_MEM_ALLOCATION_IMPL
using DataType = typename std::conditional<std::is_trivially_copyable_v<T>, T, const T&>::type;
using TGetter = T (Owner::*)() const;
protected:
  Owner * _owner{nullptr};
  TGetter _getter{nullptr};
public:
  ROProperty() = delete;
  explicit ROProperty(Owner * owner, TGetter && Getter) noexcept :
    _owner(owner),
    _getter(std::move(Getter))
  {
    Expects(_owner && _getter);
  }
  ROProperty(const ROProperty &) = default;
  ROProperty(ROProperty &&) noexcept = default;
  ROProperty & operator =(const ROProperty &) = default;
  ROProperty & operator =(ROProperty &&) noexcept = default;
  /*ROProperty(DataType in) : data(in) {}
  ROProperty(T&& in) : data(std::forward<T>(in)) {}
  T get() const {
     return _getter2();
  }*/

  /*T&& unwrap() &&
  {
    return std::move(_getter());
  }*/

  constexpr T operator()() const
  {
    Expects(_owner && _getter);
    // return std::invoke(_owner, _getter);
    return (_owner->*_getter)();
  }

  constexpr operator T() const
  {
    Expects(_owner && _getter);
    return (_owner->*_getter)();
  }

  /*operator T&() const
  {
    Expects(_getter);
    return _getter();
  }*/

  constexpr T operator->() const
  {
    Expects(_owner && _getter);
    return (_owner->*_getter)();
  }
  // constexpr decltype(auto) operator *() const { return _getter(); }
  constexpr T operator *() const { return (_owner->*_getter)(); }
  constexpr bool operator ==(DataType Value) const
  {
    Expects(_owner && _getter);
    return (_owner->*_getter)() == Value;
  }
};

template <typename Owner, typename T>
class RWProperty : public ROProperty<Owner, T>
{
CUSTOM_MEM_ALLOCATION_IMPL
using DataType = ROProperty<Owner, T>::DataType;
using TGetter = ROProperty<Owner, T>::TGetter;
using TSetter = void (Owner::*)(DataType);
private:
  TSetter _setter;
public:
  RWProperty() = delete;
  explicit RWProperty(Owner * owner, TGetter && Getter, TSetter && Setter) noexcept :
    ROProperty<Owner, T>(owner, std::move(Getter)),
    _setter(std::move(Setter))
  {
    Expects(_owner && _setter);
  }
  RWProperty(const RWProperty &) = default;
  RWProperty(RWProperty &&) noexcept = default;
  RWProperty & operator =(const RWProperty &) = default;
  RWProperty & operator =(RWProperty &&) noexcept = default;

  using ROProperty<Owner, T>::operator();
  void operator()(DataType Value)
  {
    Expects(_owner && _setter);
    (_owner->*_setter)(Value);
  }
  void operator =(DataType Value)
  {
    Expects(_owner && _setter);
    (_owner->*_setter)(Value);
  }
};

// template<int s> struct GetSizeT;
// GetSizeT<sizeof(ROProperty<UnicodeString>)> sz; //16
// template<int s> struct GetSizeT;
// GetSizeT<sizeof(RWProperty<UnicodeString>)> sz; //24

class TBase1
{
using Self = TBase1;
public:
  // ROProperty<int32_t> ROIntData1{&FData2};
  ROProperty<Self, int32_t> ROIntData1{this, &Self::GetROIntData1};
  ROProperty<Self, int32_t> ROIntData2{this, &Self::GetROIntData2};

  RWProperty<Self, UnicodeString> RWData1{this, &Self::GetData1, &Self::SetData1};
  // Property<UnicodeString> ROData1{nb::bind(&TBase1::GetData, this)};
  RWProperty<Self, int32_t> RWData2{this, &Self::GetROIntData1, &Self::SetData2};
private:
  UnicodeString GetData1() const { return FData1; }
  void SetData1(const UnicodeString & Value) { FData1 = Value; }

  int32_t GetROIntData2() const { return FData2; }
  int32_t GetROIntData1() const { return FData2; }
  void SetData2(int32_t Value) { FData2 = Value; }

  UnicodeString FData1;
  int32_t FData2{42};
};

// template<int s> struct GetSizeT;
// GetSizeT<sizeof(ROProperty<TBase1, UnicodeString>)> sz; // 16
// template<int s> struct GetSizeT;
// GetSizeT<sizeof(RWProperty<TBase1, UnicodeString>)> sz; // 24

} // namespace experimental

TEST_CASE_METHOD(base_fixture_t, "testProperty04", "netbox")
{
  experimental::TBase1 Base;
  SECTION("ROProperty01")
  {
    // Base.ROData2 = 234;
    int32_t ROIntData1 = Base.ROIntData1;
    CHECK(Base.ROIntData1 == 42);
    CHECK(ROIntData1 == 42);
  }
  SECTION("RWProperty01")
  {
    Base.RWData1 = "123";
    CHECK(Base.RWData1 == "123");
    CHECK(Base.RWData1 == "123");
    CHECK(Base.RWData1() == "123");
    CHECK(*Base.RWData1 == "123");
    CHECK(*Base.RWData1 == UnicodeString("123"));
  }
  SECTION("RWProperty02")
  {
    Base.RWData2 = 123;
    CHECK(123 == Base.RWData2);
    CHECK(Base.RWData2 == 123);
    CHECK(*Base.RWData2 == 123);
  }
}
