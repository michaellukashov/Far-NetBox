//------------------------------------------------------------------------------
// testnetbox_01.cpp
// Тесты для NetBox
// testnetbox_01 --run_test=netbox/test1 --log_level=all 2>&1 | tee res.txt
//------------------------------------------------------------------------------

// #include "leak_detector.h"
// extern int GC_find_leak;

#include <Classes.hpp>
#include <time.h>
#include <Winhttp.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <FileBuffer.h>

#include "Common.h"
#include "FarPlugin.h"
#include "testutils.h"
#include "FastDelegate.h"
#include "CppProperties.h"

//------------------------------------------------------------------------------
// stub
TCustomFarPlugin * FarPlugin = NULL;
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
    // INFO("base_fixture_t ctor");
    InitPlatformId();
  }

  virtual ~base_fixture_t()
  {
  }
public:
protected:
};

// mocks

TGlobalFunctionsIntf * GetGlobalFunctions()
{
  static TGlobalFunctions * GlobalFunctions = nullptr;
  if (!GlobalFunctions)
  {
//    GlobalFunctions = new TGlobalFunctions();
  }
  return GlobalFunctions;
}

bool AppendExceptionStackTraceAndForget(TStrings *& MoreMessages)
{
  return false;
}

//------------------------------------------------------------------------------
//TEST_CASE_METHOD(base_fixture_t, "base tests", "netbox")

TEST_CASE("base tests", "netbox")
{

base_fixture_t fixture;

SECTION("test1")
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

SECTION("test2")
{
  UnicodeString str;
  if (1)
  {
    TStringList strings;
    REQUIRE(0 == strings.GetCount());
    REQUIRE(0 == strings.Add(L"line 1"));
    REQUIRE(1 == strings.GetCount());
    str = strings.GetString(0);
    // DEBUG_PRINTF(L"str = %s", str.c_str());
    REQUIRE(W2MB(str.c_str()) == "line 1");
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
    INFO("str = " << str.c_str());
    // DEBUG_PRINTF(L"str = %s", BytesToHex(RawByteString(str.c_str(),  str.Length()), true, L',').c_str());
    INFO("str = " << BytesToHex(RawByteString(str.c_str(),  str.Length()), true, L',').c_str());
    REQUIRE(_wcsicmp(str.c_str(), L"line 1\x0D\x0A") == 0);
  }
  if (1)
  {
    strings.Add(L"line 2");
    REQUIRE(2 == strings.GetCount());
    str = strings.GetText();
    INFO(L"str = " << str.c_str());
    REQUIRE(W2MB(str.c_str()).c_str() == "line 1\r\nline 2\r\n");
    strings.Insert(0, L"line 0");
    REQUIRE(3 == strings.GetCount());
    str = strings.GetText();
    INFO(L"str = " << str.c_str());
    REQUIRE(W2MB(str.c_str()).c_str() == "line 0\r\nline 1\r\nline 2\r\n");
    strings.SetObj(0, NULL);
    UnicodeString str = strings.GetString(0);
    REQUIRE(W2MB(str.c_str()) == "line 0");
  }
  {
    strings.SetString(0, L"line 12");
    REQUIRE(W2MB(strings.GetString(0).c_str()) == "line 12");
  }
}

SECTION("test3")
{
  UnicodeString Text = L"text text text text text1\ntext text text text text2\n";
  TStringList Lines;
  Lines.SetText(Text);
  REQUIRE(2 == Lines.GetCount());
  INFO("Lines 0 = " << Lines.GetString(0).c_str());
  INFO("Lines 1 = " << Lines.GetString(1).c_str());
  REQUIRE("text text text text text1" == W2MB(Lines.GetString(0).c_str()).c_str());
  REQUIRE("text text text text text2" == W2MB(Lines.GetString(1).c_str()).c_str());
}

SECTION("test4")
{
  UnicodeString Text = L"text, text text, text text1\ntext text text, text text2\n";
  TStringList Lines;
  Lines.SetCommaText(Text);
  REQUIRE(5 == Lines.GetCount());
  REQUIRE(0 == wcscmp(L"text", Lines.GetString(0).c_str()));
  REQUIRE(0 == wcscmp(L" text text", Lines.GetString(1).c_str()));
  REQUIRE(0 == wcscmp(L" text text1", Lines.GetString(2).c_str()));
  REQUIRE(0 == wcscmp(L"text text text", Lines.GetString(3).c_str()));
  REQUIRE(0 == wcscmp(L" text text2", Lines.GetString(4).c_str()));
  UnicodeString Text2 = Lines.GetCommaText();
  INFO("Text2 = '" << Text2.c_str() << "'");
  REQUIRE(0 == wcscmp(L"\"text\",\" text text\",\" text text1\",\"text text text\",\" text text2\"", Text2.c_str()));
}

SECTION("test5")
{
  TStringList Lines;
  TObject obj1;
  Lines.InsertObject(0, L"line 1", &obj1);
  REQUIRE(&obj1 == Lines.GetObj(0));
}

SECTION("test6")
{
  TStringList Lines;
  Lines.Add(L"bbb");
  Lines.Add(L"aaa");
  // INFO("Lines = " << W2MB(Lines.Text.c_str()).c_str());
  {
    Lines.SetSorted(true);
    // INFO("Lines = " << W2MB(Lines.Text.c_str()).c_str());
    REQUIRE("aaa" == W2MB(Lines.GetString(0).c_str()).c_str());
    REQUIRE(2 == Lines.GetCount());
  }
  {
    Lines.SetSorted(false);
    Lines.Add(L"Aaa");
    Lines.SetCaseSensitive(true);
    Lines.SetSorted(true);
    REQUIRE(3 == Lines.GetCount());
    // INFO("Lines = " << W2MB(Lines.Text.c_str()).c_str());
    REQUIRE("aaa" == W2MB(Lines.GetString(0).c_str()).c_str());
    REQUIRE("Aaa" == W2MB(Lines.GetString(1).c_str()).c_str());
    REQUIRE("bbb" == W2MB(Lines.GetString(2).c_str()).c_str());
  }
}

SECTION("test7")
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
        Lines1 = NULL;
        delete Lines2;
        Lines2 = NULL;
      };
      REQUIRE(1 == cnt);
      REQUIRE(1 == cnt1);
      REQUIRE(1 == Lines2->GetCount());
      throw std::exception("");
      INFO("after BOOST_SCOPE_EXIT_END");
    }
    catch (const std::exception & ex)
    {
      INFO("in catch block");
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

SECTION("test8")
{
  UnicodeString ProgramsFolder;
  UnicodeString DefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
  REQUIRE(DefaultPuttyPathOnly == L"\\PuTTY\\putty.exe");
  REQUIRE(L"" == ::ExcludeTrailingBackslash(::IncludeTrailingBackslash(ProgramsFolder)));
}

SECTION("test9")
{
  UnicodeString Folder = L"C:\\Program Files\\Putty";
  INFO("ExtractFileDir = " << ::ExtractFileDir(Folder).c_str());
  REQUIRE(L"C:\\Program Files\\" == ::ExtractFileDir(Folder));
  REQUIRE(L"C:\\Program Files\\" == ::ExtractFilePath(Folder));
  INFO("GetCurrentDir = " << ::GetCurrentDir().c_str());
  REQUIRE(::GetCurrentDir().Length() > 0);
  REQUIRE(::DirectoryExists(::GetCurrentDir()));
}

SECTION("test10")
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

SECTION("test11")
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

SECTION("test12")
{
//  INFO("Is2000 = " << Is2000());
  INFO("IsWin7 = " << IsWin7());
//  INFO("IsExactly2008R2 = " << IsExactly2008R2());
  TDateTime dt = ::EncodeDateVerbose(2009, 12, 29);
  FILETIME ft = ::DateTimeToFileTime(dt, dstmWin);
  INFO("ft.dwLowDateTime = " << ft.dwLowDateTime);
  INFO("ft.dwHighDateTime = " << ft.dwHighDateTime);
}

SECTION("test13")
{
  UnicodeString str_value = ::IntToStr(1234);
  INFO("str_value = " << str_value.c_str());
  REQUIRE(W2MB(str_value.c_str()) == "1234");
  int int_value = ::StrToInt(L"1234");
  INFO("int_value = " << int_value);
  REQUIRE(int_value == 1234);
}

SECTION("test14")
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

SECTION("test15")
{
  UnicodeString res = ::IntToHex(10, 2);
  INFO("res = " << res.c_str());
  REQUIRE(res == L"0A");
}

SECTION("test16")
{
  {
    UnicodeString Name1 = L"1";
    UnicodeString Name2 = L"2";
    int res = ::AnsiCompareIC(Name1, Name2);
    INFO("res = " << res);
    REQUIRE(res != 0);
    res = ::AnsiCompare(Name1, Name2);
    INFO("res = " << res);
    REQUIRE(res != 0);
  }
  {
    UnicodeString Name1 = L"abc";
    UnicodeString Name2 = L"ABC";
    int res = ::AnsiCompareIC(Name1, Name2);
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

SECTION("test17")
{
  TStringList List1;
  List1.SetText(L"123\n456");
  REQUIRE(2 == List1.GetCount());
  INFO("List1.GetString(0] = " << List1.GetString(0).c_str());
  REQUIRE("123" == W2MB(List1.GetString(0).c_str()));
  INFO("List1.GetString(1] = " << List1.GetString(1).c_str());
  REQUIRE("456" == W2MB(List1.GetString(1).c_str()));
  List1.Move(0, 1);
  INFO("List1.GetString(0] = " << List1.GetString(0).c_str());
  REQUIRE("456" == W2MB(List1.GetString(0).c_str()));
  INFO("List1.GetString(1] = " << List1.GetString(1).c_str());
  REQUIRE("123" == W2MB(List1.GetString(1).c_str()));
}

SECTION("test18")
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

SECTION("test19")
{
  TStringList Strings1;
  Strings1.Add(L"Name1=Value1");
  REQUIRE(0 == Strings1.IndexOfName(L"Name1"));
}

SECTION("test20")
{
  TDateTime DateTime = Now();
  unsigned short H, M, S, MS;
  DateTime.DecodeTime(H, M, S, MS);
  // UnicodeString str = ::FormatDateTime(L"HH:MM:SS", DateTime);
  UnicodeString str = FORMAT(L"%02d:%02d:%02d", H, M, S);
  INFO("str = " << str.c_str());
  // REQUIRE(str == L"20:20:20");
}

SECTION("test21")
{
  UnicodeString str = ::FormatFloat(L"#,##0", 23.456);
  INFO("str = " << str.c_str());
  // REQUIRE(str.c_str() == L"23.46");
  REQUIRE("23.46" == W2MB(str.c_str()));
}

SECTION("test22")
{
  UnicodeString FileName = L"testfile";
  ::DeleteFile(FileName.c_str());
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
    INFO("FileName1 = " << FileName.c_str());
    REQUIRE(::FileExists(FileName));
  }
  {
    INFO("FileName2 = " << FileName.c_str());
    TSearchRec Rec;
    REQUIRE(FileSearchRec(FileName, Rec));
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

SECTION("test23")
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

SECTION("test24")
{
  TDateTime now = Now();
  INFO("now = " << (double)now);
  REQUIRE(now > 0.0);
}

SECTION("test25")
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
  CBaseClass(char * name) : m_name(name) {};
  void SimpleMemberFunction(int num, char * str)
  {
    printf("In SimpleMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  int SimpleMemberFunctionReturnsInt(int num, char * str)
  {
    printf("In SimpleMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str); return -1;
  }
  void ConstMemberFunction(int num, char * str) const
  {
    printf("In ConstMemberFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  virtual void SimpleVirtualFunction(int num, char * str)
  {
    printf("In SimpleVirtualFunction in %s. Num=%d, str = %s\n", m_name, num, str);
  }
  static void StaticMemberFunction(int num, char * str)
  {
    printf("In StaticMemberFunction. Num=%d, str =%s\n", num, str);
  }
};

SECTION("test26")
{
  DEFINE_CALLBACK_TYPE2(TEvent, int, int, char *);
  TEvent sig;

  CBaseClass a("Base A");
  sig = fastdelegate::MakeDelegate(&a, &CBaseClass::SimpleMemberFunctionReturnsInt);
  int Result = sig(10, "abc");
  INFO("Result = " << Result);
  REQUIRE(Result == -1);
}
//------------------------------------------------------------------------------
SECTION("test27")
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
        if (NULL != ProxyConfig.lpszAutoConfigUrl)
        {
            INFO("AutoConfigURL is: " << ProxyConfig.lpszAutoConfigUrl);
        }
        if (NULL != ProxyConfig.lpszProxy)
        {
            INFO("AutoConfigProxy is: " << ProxyConfig.lpszProxy);
        }
        if (NULL != ProxyConfig.lpszProxyBypass)
        {
            INFO("AutoConfigBypass is: " << ProxyConfig.lpszProxyBypass);
        }
        FreeIEConfig(&ProxyConfig);
    }
}
//------------------------------------------------------------------------------
SECTION("test28")
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
    INFO("Buf1 = " << Buf.c_str());
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
    INFO("Buf2 = " << Buf.c_str());
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
    swprintf(codeNum, L"[0x%08X]", errCode);
    DEBUG_PRINTF(L"7");
    INFO("codeNum = " << codeNum);
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


SECTION("test29")
{
  Foo* f=new Foo(100);
  Foo* b=new Bar(200);
  *f=*b;
  delete f;
  delete b;
}
#endif
//------------------------------------------------------------------------------
SECTION("test30")
{
  UnicodeString Instructions = L"Using keyboard authentication.\x0A\x0A\x0APlease enter your password.";
  INFO("Instructions = " << Instructions.c_str());
  UnicodeString Instructions2 = ReplaceStrAll(Instructions, L"\x0D\x0A", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0A\x0D", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0A", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions, L"\x0D", L"\x01");
  Instructions2 = ReplaceStrAll(Instructions2, L"\x01", L"\x0D\x0A");
  INFO("Instructions2 = " << Instructions2.c_str());
  REQUIRE(wcscmp(Instructions2.c_str(), UnicodeString(L"Using keyboard authentication.\x0D\x0A\x0D\x0A\x0D\x0APlease enter your password.").c_str()) == 0);
}
//------------------------------------------------------------------------------

}
