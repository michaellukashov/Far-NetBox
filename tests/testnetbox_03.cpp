//------------------------------------------------------------------------------
// testnetbox_03.cpp
//------------------------------------------------------------------------------

#include <Classes.hpp>
#include <CppProperties.h>
#include <map>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <FileBuffer.h>

#include "TestTexts.h"
#include "Common.h"
#include "FarPlugin.h"
#include "testutils.h"
#include "Bookmarks.h"

#include "testutils.h"

//------------------------------------------------------------------------------
// stub
// TCustomFarPlugin *FarPlugin = NULL;
//------------------------------------------------------------------------------

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
  base_fixture_t()
  {
    // INFO("base_fixture_t ctor");
    InitPlatformId();
  }

  virtual ~base_fixture_t()
  {
  }
public:
protected:
};

//------------------------------------------------------------------------------

TEST_CASE_METHOD(base_fixture_t, "testTList", "netbox")
{
  TList list;
  REQUIRE_EQUAL(0, list.GetCount());
  TObject obj1;
  TObject obj2;
  if (1)
  {
    list.Add(&obj1);
    REQUIRE_EQUAL(1, list.GetCount());
    list.Add(&obj2);
    REQUIRE_EQUAL(2, list.GetCount());
    REQUIRE_EQUAL(0, list.IndexOf(&obj1));
    REQUIRE_EQUAL(1, list.IndexOf(&obj2));
  }
  list.Clear();
  if (1)
  {
    REQUIRE_EQUAL(0, list.GetCount());
    list.Insert(0, &obj1);
    REQUIRE_EQUAL(1, list.GetCount());
    list.Insert(0, &obj2);
    REQUIRE_EQUAL(2, list.GetCount());
    REQUIRE_EQUAL(1, list.IndexOf(&obj1));
    REQUIRE_EQUAL(0, list.IndexOf(&obj2));
  }
  if (1)
  {
    list.Delete(1);
    REQUIRE_EQUAL(1, list.GetCount());
    REQUIRE_EQUAL(-1, list.IndexOf(&obj1));
    REQUIRE_EQUAL(0, list.IndexOf(&obj2));
    REQUIRE_EQUAL(1, list.Add(&obj1));
    list.Delete(0);
    REQUIRE_EQUAL(0, list.IndexOf(&obj1));
    REQUIRE_EQUAL(0, list.Remove(&obj1));
    REQUIRE_EQUAL(-1, list.IndexOf(&obj1));
    REQUIRE_EQUAL(0, list.GetCount());
  }
  if (1)
  {
    list.Add(&obj1);
    list.Add(&obj2);
    list.Extract(&obj1);
    REQUIRE_EQUAL(1, list.GetCount());
    REQUIRE_EQUAL(0, list.IndexOf(&obj2));
    list.Add(&obj1);
    REQUIRE_EQUAL(2, list.GetCount());
    list.Move(0, 1);
    REQUIRE_EQUAL(0, list.IndexOf(&obj1));
    REQUIRE_EQUAL(1, list.IndexOf(&obj2));
  }
}

TEST_CASE_METHOD(base_fixture_t, "testTStringList", "netbox")
{
  UnicodeString str;
  if (1)
  {
    TStringList strings;
    REQUIRE_EQUAL(0, strings.GetCount());
    REQUIRE_EQUAL(0, strings.Add(L"line 1"));
    REQUIRE_EQUAL(1, strings.GetCount());
    str = strings.GetString(0);
    // DEBUG_PRINTF(L"str = %s", str.c_str());
    REQUIRE_EQUAL(W2MB(str.c_str()), "line 1");
    strings.SetString(0, L"line 0");
    REQUIRE_EQUAL(1, strings.GetCount());
    str = strings.GetString(0);
    REQUIRE_EQUAL(W2MB(str.c_str()), "line 0");
    strings.SetString(0, L"line 00");
    REQUIRE_EQUAL(1, strings.GetCount());
    REQUIRE_EQUAL(W2MB(strings.GetString(0).c_str()), "line 00");
    strings.Add(L"line 11");
    REQUIRE_EQUAL(2, strings.GetCount());
    REQUIRE_EQUAL(W2MB(strings.GetString(1).c_str()), "line 11");
    strings.Delete(1);
    REQUIRE_EQUAL(1, strings.GetCount());
  }
  TStringList strings;
  if (1)
  {
    REQUIRE_EQUAL(0, strings.GetCount());
    strings.Add(L"line 1");
    str = strings.GetText();
    DEBUG_PRINTF(L"str = '%s'", str.c_str());
    REQUIRE_EQUAL(W2MB(str.c_str()), "line 1\r\n");
  }
  if (1)
  {
    strings.Add(L"line 2");
    REQUIRE_EQUAL(2, strings.GetCount());
    str = strings.GetText();
    // DEBUG_PRINTF(L"str = %s", str.c_str());
    REQUIRE_EQUAL(W2MB(str.c_str()), "line 1\r\nline 2\r\n");
    strings.Insert(0, L"line 0");
    REQUIRE_EQUAL(3, strings.GetCount());
    str = strings.GetText();
    REQUIRE_EQUAL(W2MB(str.c_str()), "line 0\r\nline 1\r\nline 2\r\n");
    strings.SetObj(0, NULL);
    UnicodeString str = strings.GetString(0);
    REQUIRE_EQUAL(W2MB(str.c_str()), "line 0");
  }
}

TEST_CASE_METHOD(base_fixture_t, "testTStringList2", "netbox")
{
  UnicodeString Text = L"text text text text text1\ntext text text text text2\n";
  TStringList Lines;
  Lines.SetText(Text);
  REQUIRE_EQUAL(2, Lines.GetCount());
  INFO("Lines 0 = " << W2MB(Lines.GetString(0).c_str()));
  INFO("Lines 1 = " << W2MB(Lines.GetString(1).c_str()));
  REQUIRE_EQUAL("text text text text text1", W2MB(Lines.GetString(0).c_str()));
  REQUIRE_EQUAL("text text text text text2", W2MB(Lines.GetString(1).c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "test4", "netbox")
{
  UnicodeString Text = L"text, text text, text text1\ntext text text, text text2\n";
  TStringList Lines;
  Lines.SetCommaText(Text);
  REQUIRE_EQUAL(5, Lines.GetCount());
  REQUIRE_EQUAL(L"text", Lines.GetString(0));
  REQUIRE_EQUAL(" text text", W2MB(Lines.GetString(1).c_str()));
  REQUIRE_EQUAL(" text text1", W2MB(Lines.GetString(2).c_str()));
  REQUIRE_EQUAL("text text text", W2MB(Lines.GetString(3).c_str()));
  REQUIRE_EQUAL(" text text2", W2MB(Lines.GetString(4).c_str()));
  UnicodeString Text2 = Lines.GetCommaText();
  INFO("Text2 = " << W2MB(Text2.c_str()));
  REQUIRE_EQUAL("\"text\",\" text text\",\" text text1\",\"text text text\",\" text text2\"", W2MB(Text2.c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "testTStringList3", "netbox")
{
  TStringList Lines;
  TObject obj1;
  Lines.InsertObject(0, L"line 1", &obj1);
  REQUIRE(&obj1 == Lines.GetObj(0));
}

TEST_CASE_METHOD(base_fixture_t, "testTStringList4", "netbox")
{
  TStringList Lines;
  Lines.Add(L"bbb");
  Lines.Add(L"aaa");
  // INFO("Lines = " << W2MB(Lines.GetText().c_str()).c_str());
  {
    Lines.SetSorted(true);
    // INFO("Lines = " << W2MB(Lines.GetText().c_str()).c_str());
    REQUIRE_EQUAL("aaa", W2MB(Lines.GetString(0).c_str()));
    REQUIRE_EQUAL(2, Lines.GetCount());
  }
  {
    Lines.SetSorted(false);
    Lines.Add(L"Aaa");
    Lines.SetCaseSensitive(true);
    Lines.SetSorted(true);
    REQUIRE_EQUAL(3, Lines.GetCount());
    INFO("Lines = " << W2MB(Lines.GetText().c_str()));
    REQUIRE_EQUAL("aaa", W2MB(Lines.GetString(0).c_str()));
    REQUIRE_EQUAL("Aaa", W2MB(Lines.GetString(1).c_str()));
    REQUIRE_EQUAL("bbb", W2MB(Lines.GetString(2).c_str()));
  }
}

TEST_CASE_METHOD(base_fixture_t, "testTStringList5", "netbox")
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
      INFO("after SCOPE_EXIT_END");
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
      INFO("before SCOPE_EXIT");
      SCOPE_EXIT
      {
        INFO("in SCOPE_EXIT");
        Lines.EndUpdate();
        cnt--;
        Lines1->EndUpdate();
        cnt1--;
        delete Lines1;
        Lines1 = NULL;
        delete Lines2;
        Lines2 = NULL;
      };
      REQUIRE(1 == cnt);
      REQUIRE(1 == cnt1);
      REQUIRE(1 == Lines2->GetCount());
      throw std::exception("");
      INFO("after SCOPE_EXIT_END");
    }
    catch (const std::exception & ex)
    {
      INFO("in catch block: " << ex.what());
      REQUIRE(NULL == Lines1);
      REQUIRE(NULL == Lines2);
    }
    INFO("after block");
    REQUIRE(0 == cnt);
    REQUIRE(0 == cnt1);
    REQUIRE(NULL == Lines1);
    REQUIRE(NULL == Lines2);
  }
}

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs1", "netbox")
{
  UnicodeString ProgramsFolder;
  UnicodeString DefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
  REQUIRE(DefaultPuttyPathOnly == L"\\PuTTY\\putty.exe");
  REQUIRE(L"" == ::ExcludeTrailingBackslash(::IncludeTrailingBackslash(ProgramsFolder)));
}

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs2", "netbox")
{
  UnicodeString Folder = L"C:\\Program Files\\Putty";
  INFO("ExtractFileDir = " << W2MB(::ExtractFileDir(Folder).c_str()).c_str());
  REQUIRE(L"C:\\Program Files\\" == ::ExtractFileDir(Folder));
  REQUIRE(L"C:\\Program Files\\" == ::ExtractFilePath(Folder));
  INFO("GetCurrentDir = " << W2MB(::GetCurrentDir().c_str()).c_str());
  REQUIRE(::GetCurrentDir().Length() > 0);
  REQUIRE(::DirectoryExists(::GetCurrentDir()));
}

TEST_CASE_METHOD(base_fixture_t, "testTDateTime1", "netbox")
{
  TDateTime dt1(23, 58, 59, 102);
  INFO("dt1 = " << dt1);
  REQUIRE(dt1 > 0.0);
  unsigned short H, M, S, MS;
  dt1.DecodeTime(H, M, S, MS);
  REQUIRE_EQUAL(H, 23);
  REQUIRE_EQUAL(M, 58);
  REQUIRE_EQUAL(S, 59);
  REQUIRE_EQUAL(MS, 102);
}

TEST_CASE_METHOD(base_fixture_t, "testTDateTime2", "netbox")
{
  TDateTime dt1 = EncodeDateVerbose(2009, 12, 29);
  INFO("dt1 = " << dt1);
#if 0
  bg::date::ymd_type ymd(2009, 12, 29);
  INFO("ymd.year = " << ymd.year << ", ymd.month = " << ymd.month << ", ymd.day = " << ymd.day);
  unsigned int Y, M, D;
  dt1.DecodeDate(Y, M, D);
  INFO("Y = " << Y << ", M = " << M << ", D = " << D);
  REQUIRE(Y == ymd.year);
  REQUIRE(M == ymd.month);
  REQUIRE(D == ymd.day);
#endif
  int DOW = ::DayOfWeek(dt1);
  REQUIRE_EQUAL(3, DOW);
}

TEST_CASE_METHOD(base_fixture_t, "testTDateTime3", "netbox")
{
//  INFO("Is2000 = " << Is2000());
  INFO("IsWin7 = " << IsWin7());
//  INFO("IsExactly2008R2 = " << IsExactly2008R2());
  TDateTime dt = ::EncodeDateVerbose(2009, 12, 29);
  FILETIME ft = ::DateTimeToFileTime(dt, dstmWin);
  INFO("ft.dwLowDateTime = " << ft.dwLowDateTime);
  INFO("ft.dwHighDateTime = " << ft.dwHighDateTime);
}

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs3", "netbox")
{
  UnicodeString str_value = ::IntToStr(1234);
  INFO("str_value = " << W2MB(str_value.c_str()));
  REQUIRE(W2MB(str_value.c_str()) == "1234");
  intptr_t int_value = ::StrToInt(L"1234");
  INFO("int_value = " << int_value);
  REQUIRE(int_value == 1234);
}

TEST_CASE_METHOD(base_fixture_t, "testTStringList6", "netbox")
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

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs4", "netbox")
{
  UnicodeString res = ::IntToHex(10, 2);
  INFO("res = " << W2MB(res.c_str()));
  REQUIRE(res == L"0A");
}

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs5", "netbox")
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

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs6", "netbox")
{
  TStringList List1;
  List1.SetText(L"123\n456");
  REQUIRE(2 == List1.GetCount());
  INFO("List1.GetString(0) = " << W2MB(List1.GetString(0).c_str()));
  REQUIRE("123" == W2MB(List1.GetString(0).c_str()));
  INFO("List1.GetString(1) = " << W2MB(List1.GetString(1).c_str()));
  REQUIRE("456" == W2MB(List1.GetString(1).c_str()));
  List1.Move(0, 1);
  INFO("List1.GetString(0) = " << W2MB(List1.GetString(0).c_str()));
  REQUIRE("456" == W2MB(List1.GetString(0).c_str()));
  INFO("List1.GetString(1) = " << W2MB(List1.GetString(1).c_str()));
  REQUIRE("123" == W2MB(List1.GetString(1).c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs7", "netbox")
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

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs8", "netbox")
{
  TStringList Strings1;
  Strings1.Add(L"Name1=Value1");
  REQUIRE(0 == Strings1.IndexOfName(L"Name1"));
}

TEST_CASE_METHOD(base_fixture_t, "testStringFuncs9", "netbox")
{
  TDateTime DateTime = Now();
  unsigned short H, M, S, MS;
  DateTime.DecodeTime(H, M, S, MS);
  // UnicodeString str = ::FormatDateTime(L"HH:MM:SS", DateTime);
  UnicodeString str = FORMAT(L"%02d:%02d:%02d", H, M, S);
  INFO("str = " << W2MB(str.c_str()));
  // REQUIRE(str == L"20:20:20");
}

TEST_CASE_METHOD(base_fixture_t, "test21", "netbox")
{
  UnicodeString str = ::FormatFloat(L"#,##0", 23.456);
  INFO("str = " << W2MB(str.c_str()));
  // REQUIRE(str.c_str() == L"23.46");
  REQUIRE("23.46" == W2MB(str.c_str()));
}

TEST_CASE_METHOD(base_fixture_t, "testFileFuncs1", "netbox")
{
  UnicodeString FileName = L"testfile";
  ::RemoveFile(FileName);
  std::string str = "test string";
  {
    unsigned int CreateAttrs = FILE_ATTRIBUTE_NORMAL;
    HANDLE FileHandle = ::CreateFile(FileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               NULL, CREATE_ALWAYS, CreateAttrs, 0);
    REQUIRE(FileHandle != 0);
    TStream * FileStream = new TSafeHandleStream(FileHandle);
    TFileBuffer * BlockBuf = new TFileBuffer();
    // BlockBuf->SetSize(1024);
    BlockBuf->SetPosition(0);
    BlockBuf->Insert(0, str.c_str(), str.size());
    INFO("BlockBuf->GetSize = " << BlockBuf->GetSize());
    REQUIRE(BlockBuf->GetSize() == str.size());
    BlockBuf->WriteToStream(FileStream, BlockBuf->GetSize());
    delete FileStream; FileStream = NULL;
    delete BlockBuf; BlockBuf = NULL;
    ::CloseHandle(FileHandle);
    INFO("FileName1 = " << W2MB(FileName.c_str()));
    REQUIRE(::FileExists(FileName));
  }
  {
    INFO("FileName2 = " << W2MB(FileName.c_str()));
    // WIN32_FIND_DATA Rec;
    // REQUIRE(FileSearchRec(FileName, Rec));
  }
  {
    HANDLE File = ::CreateFile(
                    FileName.c_str(),
                    GENERIC_READ,
                    0, // FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS, // OPEN_EXISTING,
                    0, // FILE_ATTRIBUTE_NORMAL, // 0,
                    NULL);
    DEBUG_PRINTF(L"File = %d", File);
    TStream * FileStream = new TSafeHandleStream(File);
    TFileBuffer * BlockBuf = new TFileBuffer();
    BlockBuf->ReadStream(FileStream, str.size(), true);
    INFO("BlockBuf->GetSize = " << BlockBuf->GetSize());
    REQUIRE(BlockBuf->GetSize() == str.size());
    delete FileStream; FileStream = NULL;
    delete BlockBuf; BlockBuf = NULL;
    ::CloseHandle(File);
  }
}

TEST_CASE_METHOD(base_fixture_t, "testFileFuncs2", "netbox")
{
  UnicodeString Dir1 = L"subdir1";
  UnicodeString Dir2 = L"subdir1/subdir2";
  ::RemoveDir(Dir2);
  ::RemoveDir(Dir1);
  INFO("DirectoryExists(Dir2) = " << DirectoryExists(Dir2));
  REQUIRE(!::DirectoryExists(Dir2));
  ::ForceDirectories(Dir2);
  REQUIRE(::DirectoryExists(Dir2));
  ::RemoveDir(Dir2);
  ::ForceDirectories(Dir2);
  REQUIRE(::DirectoryExists(Dir2));
  REQUIRE(::RecursiveDeleteFile(Dir1, false));
  REQUIRE(!::DirectoryExists(Dir1));
}

TEST_CASE_METHOD(base_fixture_t, "testDateTimeFuncs1", "netbox")
{
  TDateTime now = Now();
  INFO("now = " << (double)now);
  REQUIRE(now > 0.0);
}

TEST_CASE_METHOD(base_fixture_t, "testMallocFuncs1", "netbox")
{
#if 0
  GC_find_leak = 1;
  int * i = (int *)malloc(sizeof(int));
  CHECK_LEAKS();
#endif
}

TEST_CASE_METHOD(base_fixture_t, "testBookmarks1", "netbox")
{
  TBookmarks Bookmarks;
}

//------------------------------------------------------------------------------
class TestPropsClass
{
private:
  std::map<std::string, std::string> FAssignments;
  Property<std::string> FKey;
  int GetNumber() { return 42; }
  void AddWeight(float value) { }
  std::string GetKey()
  {
    // extra processing steps here
    return FKey();
  }
  void SetKey(std::string AKey)
  {
    // extra processing steps here
    FKey = AKey;
  }
  std::string GetAssignment(std::string AKey)
  {
    // extra processing steps here
    return FAssignments[Key];
  }
  void SetAssignment(std::string Key, std::string Value)
  {
    // extra processing steps here
    FAssignments[Key] = Value;
  }
public:
  TestPropsClass()
  {
    Number(this);
    WeightedValue(this);
    Key(this);
    Assignments(this);
  }
  Property<std::string> Name;
  Property<int> ID;
  ROProperty<int, TestPropsClass, &TestPropsClass::GetNumber> Number;
  WOProperty<float, TestPropsClass, &TestPropsClass::AddWeight> WeightedValue;
  RWProperty<std::string, TestPropsClass, &TestPropsClass::GetKey, &TestPropsClass::SetKey> Key;
  IndexedProperty<std::string, std::string, TestPropsClass, &TestPropsClass::GetAssignment, &TestPropsClass::SetAssignment > Assignments;
};

TEST_CASE_METHOD(base_fixture_t, "testTestPropsClass1", "netbox")
{
  TestPropsClass obj;
  obj.Name = "Name";
  obj.WeightedValue = 1234;
  obj.Key = "Key";
  obj.Assignments["Hours"] = "23";
  obj.Assignments["Minutes"] = "59";
  INFO("Name = " << obj.Name.get());
  INFO("Number = " << obj.Number.get());
  INFO("Key = " << obj.Key.get());
  // INFO("Assignments = " << obj.Assignments);
  INFO("Hours = " << obj.Assignments["Hours"]);
  INFO("Minutes = " << obj.Assignments["Minutes"]);
}

//------------------------------------------------------------------------------
