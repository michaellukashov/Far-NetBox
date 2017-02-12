//------------------------------------------------------------------------------
// testnetbox_02.cpp
//------------------------------------------------------------------------------

#include <Classes.hpp>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "winstuff.h"
#include "puttyexp.h"
#include "FarUtil.h"

#include "TestTexts.h"
#include "Common.h"
#include "StrUtils.hpp"
#include "FileMasks.h"
#include "WinSCPPlugin.h"
#include "GUITools.h"
#include "GUIConfiguration.h"
#include "TextsCore.h"
#include "FileOperationProgress.h"
#include "HierarchicalStorage.h"
#include "CoreMain.h"

#include "testutils.h"

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t : TObject
{
public:
  base_fixture_t() :
    TObject(),
    OnChangeNotifyEventTriggered(false),
    ClickEventHandlerTriggered(false),
    onStringListChangeTriggered(false)
  {
    // INFO("base_fixture_t ctor");
    FarPlugin = CreateStub();
    // CoreInitialize();
  }

  virtual ~base_fixture_t()
  {
    delete FarPlugin;
    FarPlugin = NULL;
    // CoreFinalize();
  }

  bool scp_test(std::string host, int port, std::string user, std::string password);
public:
  void OnChangeNotifyEvent(TObject * Sender)
  {
    INFO("OnChangeNotifyEvent triggered");
    OnChangeNotifyEventTriggered = true;
  }
  void ClickEventHandler(TObject * Sender)
  {
    INFO("ClickEventHandler triggered");
    ClickEventHandlerTriggered = true;
  }
  void onStringListChange(TObject * Sender)
  {
    INFO("onStringListChange triggered");
    onStringListChangeTriggered = true;
  }
protected:
  bool OnChangeNotifyEventTriggered;
  bool ClickEventHandlerTriggered;
  bool onStringListChangeTriggered;
};

//------------------------------------------------------------------------------

bool base_fixture_t::scp_test(std::string host, int port, std::string user, std::string password)
{
  return false;
}

//------------------------------------------------------------------------------

TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst);

//------------------------------------------------------------------------------

TEST_CASE_METHOD(base_fixture_t, "test1", "netbox")
{
  if (1)
  {
    UnicodeString Text = ::StringOfChar(' ', 4);
    REQUIRE_EQUAL("    ", W2MB(Text.c_str()).c_str());
  }
  if (1)
  {
    UnicodeString Message = L"long long long long long long long long long text";
    TStringList MessageLines;
    int MaxMessageWidth = 20;
    FarWrapText(Message, &MessageLines, MaxMessageWidth);
    INFO("MessageLines = " << MessageLines.GetText().c_str());
    REQUIRE_EQUAL(4, MessageLines.GetCount());
    REQUIRE_EQUAL("long long long", W2MB(MessageLines.GetString(0).c_str()).c_str());
    REQUIRE_EQUAL("long long long", W2MB(MessageLines.GetString(1).c_str()).c_str());
    REQUIRE_EQUAL("long long long", W2MB(MessageLines.GetString(2).c_str()).c_str());
    REQUIRE_EQUAL("text", W2MB(MessageLines.GetString(3).c_str()).c_str());
  }
}

class TClass1 : TObject
{
public:
  TClass1() :
    OnChangeNotifyEventTriggered(false)
  {
  }
  TNotifyEvent & GetOnChange() { return FOnChange; }
  void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }
  virtual void Changed()
  {
    if (!FOnChange.empty())
    {
      FOnChange(this);
      OnChangeNotifyEventTriggered = true;
    }
  }
  void Change(const UnicodeString & Str)
  {
    Changed();
  }

  bool OnChangeNotifyEventTriggered;
private:
  TNotifyEvent FOnChange;
};

class TClass2;
typedef nb::FastDelegate2<void, TClass2 *, int> TClickEvent;

class TClass2
{

public:
  TClass2() :
    OnClickTriggered(false),
    m_OnClick(NULL)
  {
  }

  TClickEvent GetOnClick() const { return m_OnClick; }
  void SetOnClick(TClickEvent onClick)
  {
    m_OnClick = onClick;
    // DEBUG_PRINTF(L"m_OnClick.num_slots = %d", m_OnClick.num_slots());
  }
  void Click()
  {
    if (m_OnClick)
      m_OnClick(this, 1);
    OnClickTriggered = true;
  }
  bool OnClickTriggered;
private:
  TClickEvent m_OnClick;
};

class TClass3
{
public:
  TClass3() :
    ClickEventHandlerTriggered(false)
  {
  }
  void ClickEventHandler(TClass2 * Sender, int Data)
  {
    INFO("TClass3: ClickEventHandler triggered");
    ClickEventHandlerTriggered = true;
  }
public:
  bool ClickEventHandlerTriggered;
};

TEST_CASE_METHOD(base_fixture_t, "test2", "netbox")
{
  if (1)
  {
    TClass2 cl2;
    REQUIRE_EQUAL(false, cl2.OnClickTriggered);
    cl2.Click();
    REQUIRE_EQUAL(true, cl2.OnClickTriggered);
  }
  if (1)
  {
    TClass2 cl2;
    TClass3 cl3;
    cl2.SetOnClick(nb::bind(&TClass3::ClickEventHandler, &cl3));
    REQUIRE(!cl2.GetOnClick().empty());
    cl2.Click();
    REQUIRE_EQUAL(true, cl2.OnClickTriggered);
    REQUIRE_EQUAL(true, cl3.ClickEventHandlerTriggered);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test3", "netbox")
{
  if (1)
  {
    TClass1 cl1;
    REQUIRE_EQUAL(false, cl1.OnChangeNotifyEventTriggered);
    cl1.SetOnChange(nb::bind(&base_fixture_t::OnChangeNotifyEvent, this));
    cl1.Change(L"line 1");
    REQUIRE_EQUAL(true, cl1.OnChangeNotifyEventTriggered);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test4", "netbox")
{
  if (1)
  {
    TStringList strings;
    strings.SetOnChange(nb::bind(&base_fixture_t::onStringListChange, this));
    strings.Add(L"line 1");
    // REQUIRE_EQUAL(true, OnChangeNotifyEventTriggered);
    REQUIRE_EQUAL(true, onStringListChangeTriggered);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test5", "netbox")
{
  if (1)
  {
    TFileOperationProgressType OperationProgress;
  }
}

TEST_CASE_METHOD(base_fixture_t, "test6", "netbox")
{
  REQUIRE_THROWS_AS(Error(SListIndexError, 0), ExtException);
}

TEST_CASE_METHOD(base_fixture_t, "test7", "netbox")
{
  TStringList Lines;
  Lines.SetSorted(true);
  if (1)
  {
    Lines.SetDuplicates(dupAccept);
    Lines.Add(L"aaa");
    Lines.Add(L"aaa");
    Lines.Add(L"bbb");
    REQUIRE(3 == Lines.GetCount());
    REQUIRE(0 == Lines.IndexOf(L"aaa"));
    REQUIRE(2 == Lines.IndexOf(L"bbb"));
  }
  Lines.Clear();
  if (1)
  {
    Lines.SetDuplicates(dupIgnore);
    Lines.Add(L"aaa");
    Lines.Add(L"aaa");
    Lines.Add(L"bbb");
    REQUIRE(2 == Lines.GetCount());
    REQUIRE(1 == Lines.IndexOf(L"bbb"));
  }
  Lines.Clear();
  if (1)
  {
    Lines.SetDuplicates(dupError);
    Lines.Add(L"aaa");
    Lines.Add(L"bbb");
    REQUIRE_THROWS_AS(Lines.Add(L"aaa"), std::exception);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test8", "netbox")
{
  UnicodeString RootKey = L"Software\\Michael Lukashov\\TestNetBox";
  TRegistryStorage Storage(RootKey);
  Storage.SetAccessMode(smReadWrite);
  REQUIRE(Storage.OpenRootKey(true));
  UnicodeString SubKey = L"SubKey1";
  Storage.DeleteSubKey(SubKey);
  REQUIRE(!Storage.KeyExists(SubKey));
  REQUIRE(Storage.OpenSubKey(SubKey, true));
  Storage.SetAccessMode(smReadWrite);
  Storage.WriteInteger(L"IntVal", 1234);
  // INFO("Storage.GetFailed = " << Storage.GetFailed());
  Storage.CloseSubKey();
  REQUIRE(Storage.KeyExists(SubKey));
  REQUIRE(Storage.OpenSubKey(SubKey, false));
  int res = Storage.ReadInteger(L"IntVal", -1);
  INFO("res = " << res);
  REQUIRE(1234 == res);
}

TEST_CASE_METHOD(base_fixture_t, "test9", "netbox")
{
  UnicodeString path = L"C:\\test";
  AppendPathDelimiterW(path);
  REQUIRE(path == L"C:\\test\\");
}

TEST_CASE_METHOD(base_fixture_t, "test10", "netbox")
{
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

TEST_CASE_METHOD(base_fixture_t, "test11", "netbox")
{
  // Тесты на ::FmtLoadStr FMTLOAD ::Format ::LoadStr ::LoadStrPart ::CutToChar ::TrimLeft ::TrimRight
  {
    UnicodeString str = FMTLOAD(CONST_TEST_STRING, L"lalala", 42);
    // INFO("str = " << W2MB(str.c_str()));
    // INFO("length = " << str.size());
    REQUIRE(W2MB(str.c_str()) == "test string: \"lalala\" 42");
  }
  {
    UnicodeString str2 = FMTLOAD(CONST_TEST_STRING, L"lalala", 42);
    // INFO("str2 = " << W2MB(str2.c_str()));
    REQUIRE(W2MB(str2.c_str()) == "test string: \"lalala\" 42");
  }
  {
    UnicodeString str2 = FORMAT(L"test: %s %d", L"lalala", 42);
    INFO("str2 = " << str2.c_str());
    REQUIRE_EQUAL(0, wcscmp(str2.c_str(), L"test: lalala 42"));
  }
  {
    UnicodeString str3 = FORMAT(L"test: %s %d", L"lalala", 42);
    INFO("str3 = " << str3.c_str());
    REQUIRE_EQUAL(0, wcscmp(str3.c_str(), L"test: lalala 42"));
  }
  {
    UnicodeString str = ::TrimLeft(L"");
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L""));
  }
  {
    UnicodeString str = ::TrimLeft(L"1");
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    UnicodeString str = ::TrimLeft(L" 1");
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    UnicodeString str = ::TrimRight(L"");
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L""));
  }
  {
    UnicodeString str = ::TrimRight(L"1");
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    UnicodeString str = ::TrimRight(L"1 ");
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"1"));
  }
  {
    // UnicodeString CutToChar(UnicodeString &Str, char Ch, bool Trim)
    UnicodeString Str1 = L" part 1 | part 2 ";
    UnicodeString str1 = ::CutToChar(Str1, '|', false);
    INFO("str1 = '" << str1.c_str() << "'");
    INFO("Str1 = '" << Str1.c_str() << "'");
    // INFO("Str1 = '" << W2MB(Str1.c_str()) << "'");
    // DEBUG_PRINTF(L"str1 = \"%s\"", str1.c_str());
    REQUIRE_EQUAL(0, wcscmp(str1.c_str(), L" part 1 "));

    UnicodeString str2 = ::CutToChar(Str1, '|', true);
    INFO("str2 = '" << str2.c_str() << "'");
    INFO("Str1 = '" << Str1.c_str() << "'");
    REQUIRE_EQUAL(0, wcscmp(str2.c_str(), L" part 2"));
  }
  {
    UnicodeString str = ::LoadStr(CONST_TEST_STRING);
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"test string: \"%s\" %d"));
  }
  {
    UnicodeString str = ::LoadStrPart(CONST_TEST_STRING2, 1);
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"test string part 1"));
  }
  {
    UnicodeString str = ::LoadStrPart(CONST_TEST_STRING2, 2);
    INFO("str = " << str.c_str());
    REQUIRE_EQUAL(0, wcscmp(str.c_str(), L"part 2"));
  }
}

TEST_CASE_METHOD(base_fixture_t, "test12", "netbox")
{
  std::string host = "localhost";
  int port = 2222;
  std::string user = "testuser";
  std::string password = "testpassword";
  TEST_CASE_TODO(scp_test(host, port, user, password));
}

class TBaseClass1
{
public:
  virtual ~TBaseClass1()
  {}
};

class TDerivedClass1 : public TBaseClass1
{
};

class TBaseClass2
{
public:
  virtual ~TBaseClass2()
  {}
};

TEST_CASE_METHOD(base_fixture_t, "test13", "netbox")
{
  TBaseClass1 E1;
  TDerivedClass1 E2;
  TBaseClass2 E3;
  // typedef boost::is_base_of<TBaseClass1, TDerivedClass1>::type t1;
//  REQUIRE((::InheritsFrom<TBaseClass1, TBaseClass1>(&E1)));
//  REQUIRE((::InheritsFrom<TBaseClass1, TDerivedClass1>(&E2)));
  // REQUIRE(!(::InheritsFrom<TBaseClass2, TDerivedClass1>(&E2)));
  // REQUIRE(!(::InheritsFrom<TDerivedClass1, TBaseClass1>(E1)));
  // REQUIRE(!(::InheritsFrom<TBaseClass1, TBaseClass2>(E3)));
}

TEST_CASE_METHOD(base_fixture_t, "test14", "netbox")
{
  {
    UnicodeString str = ::ReplaceStr(L"AA", L"A", L"B");
    REQUIRE_EQUAL(W2MB(str.c_str()).c_str(), "BB");
  }
  {
    UnicodeString str = ::AnsiReplaceStr(L"AA", L"A", L"B");
    REQUIRE_EQUAL(W2MB(str.c_str()).c_str(), "BB");
  }
  {
    UnicodeString str = L"ABC";
    REQUIRE_EQUAL(::Pos(str, L"DEF"), 0);
    REQUIRE_EQUAL(::Pos(str, L"AB"), 1);
    REQUIRE_EQUAL(::Pos(str, L"BC"), 2);
    REQUIRE_EQUAL(::AnsiPos(str, 'D'), 0);
    REQUIRE_EQUAL(::AnsiPos(str, 'A'), 1);
    REQUIRE_EQUAL(::AnsiPos(str, 'B'), 2);
  }
  {
    UnicodeString str = ::LowerCase(L"AA");
    REQUIRE_EQUAL(W2MB(str.c_str()).c_str(), "aa");
  }
  {
    UnicodeString str = ::UpperCase(L"aa");
    REQUIRE_EQUAL(W2MB(str.c_str()).c_str(), "AA");
  }
  {
    UnicodeString str = ::Trim(L" aa ");
    REQUIRE_EQUAL(W2MB(str.c_str()).c_str(), "aa");
  }
}

TEST_CASE_METHOD(base_fixture_t, "test15", "netbox")
{
  if (1)
  {
    REQUIRE_EQUAL(true, TFileMasks::IsMask(L"*.txt;*.log;*.exe,*.cmd|*.bat"));
    // REQUIRE_EQUAL(true, TFileMasks::IsAnyMask(L"*.*"));
    TFileMasks m(L"*.txt;*.log");
    REQUIRE_EQUAL(false, m.Matches(L"test.exe"));
  }
  {
    TFileMasks m(L"*.txt;*.log");
    REQUIRE_EQUAL(true, m.Matches(L"test.txt"));
  }
  if (1)
  {
    TFileMasks m(L"*.txt;*.log");
    REQUIRE_EQUAL(true, m.Matches(L"test.log"));

    intptr_t Start, Length;
//    REQUIRE_EQUAL(true, m.GetIsValid(Start, Length));
    m.SetMask(L"*.exe");
    REQUIRE_EQUAL(true, m.Matches(L"test.exe"));
    REQUIRE_EQUAL(false, m.Matches(L"test.txt"));
    REQUIRE_EQUAL(false, m.Matches(L"test.log"));
  }
}

TEST_CASE_METHOD(base_fixture_t, "test16", "netbox")
{
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

TEST_CASE_METHOD(base_fixture_t, "test17", "netbox")
{
  if (1)
  {
    HINSTANCE HInst = GetModuleHandle(0);
    TCustomFarPlugin * FarPlugin = CreateFarPlugin(HInst);
    //DEBUG_PRINTF(L"FarPlugin = %x", FarPlugin);
    REQUIRE(FarPlugin != NULL);
    // SAFE_DESTROY(FarPlugin);
    delete FarPlugin;
    // REQUIRE(FarPlugin == NULL);
  }
}

TEST_CASE_METHOD(base_fixture_t, "test18", "netbox")
{
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

TEST_CASE_METHOD(base_fixture_t, "test19", "netbox")
{
  UnicodeString ProgramsFolder;
  ::SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
  INFO("ProgramsFolder = " << ProgramsFolder.c_str());
  REQUIRE(ProgramsFolder.Length() > 0);
}

// TEST_CASE_METHOD(base_fixture_t, "test20", "netbox")
// {
// random_ref();
// random_unref();
// }

TEST_CASE_METHOD(base_fixture_t, "test21", "netbox")
{
  INFO("RAND_MAX = " << RAND_MAX);
  for (int i = 0; i < 10; i++)
  {
    INFO("rand() = " << rand());
//    INFO("random(256) = " << random(256));
  }
  UnicodeString enc = ::EncryptPassword(L"1234ABC", L"234556");
  INFO("enc = " << enc.c_str());
  UnicodeString dec = ::DecryptPassword(enc, L"234556");
  INFO("dec = " << dec.c_str());
  REQUIRE(dec == L"1234ABC");
}

TEST_CASE_METHOD(base_fixture_t, "test22", "netbox")
{
  // FarPlugin->RunTests();
}
