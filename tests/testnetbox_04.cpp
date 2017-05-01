//------------------------------------------------------------------------------
// testnetbox_04.cpp
//------------------------------------------------------------------------------

#include <Classes.hpp>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <Common.h>
#include <FileBuffer.h>

#include "TestTexts.h"
#include "TextsCore.h"
#include "FarPlugin.h"
#include "WinSCPPlugin.h"
#include "GUITools.h"
#include "GUIConfiguration.h"

#include "testutils.h"

#include "neon/src/ne_session.h"
#include "neon/src/ne_request.h"

// #include "dl/include.hpp"

#include "calculator.hpp"
#include "DynamicQueue.hpp"

#include "testutils.h"

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
  base_fixture_t()
  {
    InitPlatformId();
  }
  virtual ~base_fixture_t()
  {
  }

public:
protected:
  static int read_block(void * udata, const char * data, size_t len)
  {
    typedef std::vector<unsigned char> uvector;
    uvector *vec = static_cast<uvector *>(udata);
    for (unsigned int n = 0; n < len; ++n)
      vec->push_back((unsigned char)data[n]);
  }
};

//------------------------------------------------------------------------------
#if 0
TEST_CASE_METHOD(base_fixture_t, "test1", "netbox")
{
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

  ne_session * sess = ne_session_create("http", "farmanager.com", 80);
  ne_request * req = ne_request_create(sess, "GET", "/svn/trunk/unicode_far/vbuild.m4");
  std::vector<unsigned char> vec;
  // ne_add_response_body_reader(req, ne_accept_2xx, read_block, &vec);
  int rv = ne_request_dispatch(req);
  INFO("rv = " << rv);
  if (rv)
  {
    FAIL("Request failed: " << ne_get_error(sess));
  }
  REQUIRE(rv == 0);
  const ne_status * statstruct = ne_get_status(req);
  INFO("statstruct->code = " << statstruct->code);
  INFO("statstruct->reason_phrase = " << statstruct->reason_phrase);
  if (vec.size() > 0)
  {
    INFO("response = " << (char *)&vec[0]);
  }
  ne_request_destroy(req);
  ne_session_destroy(sess);
  
  WSACleanup();
}
#endif
/*TEST_CASE_METHOD(base_fixture_t, "test2", "netbox")
{
  team::calculator calc("calculator_dll.dll");
  INFO("sum = " << calc.sum(10, 20));
  INFO("mul = " << calc.mul(10, 20));
  INFO("sqrt = " << calc.sqrt(25));
}*/

TEST_CASE_METHOD(base_fixture_t, "test3", "netbox")
{
  DynamicQueue<int> q;
  q.Reserve(10);
  q.Put(1);
  q.Put(2);
  q.Put(3);
  int val = q.Get();
  INFO("val = " << val);
  REQUIRE(val == 1);
  val = q.Get();
  INFO("val = " << val);
  REQUIRE(val == 2);
  val = q.Get();
  INFO("val = " << val);
  REQUIRE(val == 3);
}

TEST_CASE_METHOD(base_fixture_t, "testConfig", "netbox")
{
  TEST_CASE_TODO("implement testConfig");
#if 0
  Config cfg; //  = new Config();
  memset(&cfg, 0, sizeof(cfg));
  cfg.logtype = LGTYP_ASCII;
  void * ctx = log_init(NULL, &cfg);
  // strcpy(&ctx->currlogfilename.path, "putty.log");
  logfopen(ctx);
  log_eventlog(ctx, "test2: start");

  char buf[256];
  struct tm tm = ltime();
  time_t t = time(0);
#endif

#if 0
  char buf2[256];
  _snprintf(buf2, sizeof(buf2) - 1, "%04d.%02d.%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  strftime2(buf, sizeof(buf) - 1, "%Y.%m.%d %H:%M:%S", &tm);
  INFO("buf = " << buf); //  << ", sizeof(buf) = " << sizeof(buf));
  INFO("buf2 = " << buf2);
  REQUIRE(0 == strcmp(buf, buf2));
  log_eventlog(ctx, "test2: end");
  logfclose(ctx);
  log_free(ctx);
#endif
}

TEST_CASE_METHOD(base_fixture_t, "testFmtLoadStr", "netbox")
{
  // Тесты на ::FmtLoadStr FMTLOAD ::Format ::LoadStr ::LoadStrPart ::CutToChar ::TrimLeft ::TrimRight
  TEST_CASE_TODO("fix FmtLoadStr for tests");
  {
    INFO("before FMTLOAD");
    UnicodeString str = FMTLOAD(CONST_TEST_STRING, L"lalala", 42);
    INFO("str = " << str);
    // INFO("length = " << str.size());
    REQUIRE(W2MB(str.c_str()) == "test string: \"lalala\" 42");
  }
  {
    UnicodeString str2 = FMTLOAD(CONST_TEST_STRING, L"lalala", 42);
    INFO("str2 = " << str2);
    REQUIRE(W2MB(str2.c_str()) == "test string: \"lalala\" 42");
  }
  {
    UnicodeString str2 = FORMAT(L"test: %s %d", L"lalala", 42);
    INFO("str2 = " << str2);
    REQUIRE_EQUAL(0, wcscmp(str2.c_str(), L"test: lalala 42"));
  }
  {
    UnicodeString str3 = FORMAT(L"test: %s %d", L"lalala", 42);
    INFO("str3 = " << str3);
    REQUIRE_EQUAL(0, wcscmp(str3.c_str(), L"test: lalala 42"));
  }
  {
    UnicodeString str = ::TrimLeft(L"");
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L""));
  }
  {
    UnicodeString str = ::TrimLeft(L"1");
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    UnicodeString str = ::TrimLeft(L" 1");
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    UnicodeString str = ::TrimRight(L"");
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L""));
  }
  {
    UnicodeString str = ::TrimRight(L"1");
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    UnicodeString str = ::TrimRight(L"1 ");
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    // UnicodeString CutToChar(UnicodeString &Str, char Ch, bool Trim)
    UnicodeString Str1 = L" part 1 | part 2 ";
    UnicodeString str1 = ::CutToChar(Str1, '|', false);
    INFO("str1 = '" << str1 << "'");
    INFO("Str1 = '" << Str1 << "'");
    // INFO("Str1 = '" << W2MB(Str1.c_str()) << "'");
    // DEBUG_PRINTF(L"str1 = \"%s\"", str1.c_str());
    REQUIRE_EQUAL(0, wcscmp(str1.c_str(), L" part 1 "));

    UnicodeString str2 = ::CutToChar(Str1, '|', true);
    INFO("str2 = '" << str2 << "'");
    INFO("Str1 = '" << Str1 << "'");
    REQUIRE_EQUAL(0, wcscmp(str2.c_str(), L" part 2"));
  }
  {
    UnicodeString str = ::LoadStr(CONST_TEST_STRING);
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"test string: \"%s\" %d"));
  }
  {
    UnicodeString str = ::LoadStrPart(CONST_TEST_STRING2, 1);
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"test string part 1"));
  }
  {
    UnicodeString str = ::LoadStrPart(CONST_TEST_STRING2, 2);
    INFO("str = " << str);
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"part 2"));
  }
}

TEST_CASE_METHOD(base_fixture_t, "testCopyParam", "netbox")
{
  TEST_CASE_TODO("implement testCopyParam");
  return;
  TGUICopyParamType DefaultCopyParam;
  TCopyParamType * CopyParam = new TCopyParamType(DefaultCopyParam);
  CopyParam->SetTransferMode(tmAscii);
  TCopyParamList CopyParamList;
  // INFO("CopyParamList.GetCount() = " << CopyParamList.GetCount());
  CopyParamList.Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, NULL);
  // INFO("CopyParamList.GetCount() = " << CopyParamList.GetCount());
  CopyParam = new TCopyParamType(DefaultCopyParam);
  CopyParam->SetTransferMode(tmAscii);
  CopyParamList.Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, NULL);
  // INFO("CopyParamList.GetCount() = " << CopyParamList.GetCount());
}

TEST_CASE_METHOD(base_fixture_t, "testWinSCPPlugin", "netbox")
{
  TEST_CASE_TODO("implement testWinSCPPlugin");
  if (1)
  {
    HINSTANCE HInst = ::GetModuleHandle(0);
    TWinSCPPlugin * FarPlugin = new TWinSCPPlugin(HInst);
    //DEBUG_PRINTF(L"FarPlugin = %x", FarPlugin);
    REQUIRE(FarPlugin != NULL);
    // SAFE_DESTROY(FarPlugin);
    delete FarPlugin;
    // REQUIRE(FarPlugin == NULL);
  }
}

TEST_CASE_METHOD(base_fixture_t, "testError", "netbox")
{
  TEST_CASE_TODO("implement Error");
  return;
  if (1)
  {
    REQUIRE_THROWS_AS(Error(SListIndexError, 0), ExtException);
  }
}

TEST_CASE_METHOD(base_fixture_t, "testStringList", "netbox")
{
  TEST_CASE_TODO("implement LoadFmt");
  return;
  TStringList Lines;
  Lines.SetSorted(true);
  Lines.Clear();
  if (1)
  {
    Lines.SetDuplicates(dupError);
    Lines.Add(L"aaa");
    Lines.Add(L"bbb");
    INFO("before REQUIRE_THROWS_AS");
    REQUIRE_THROWS_AS(Lines.Add(L"aaa"), std::exception);
  }
}
