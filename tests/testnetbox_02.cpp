//------------------------------------------------------------------------------
// testnetbox_02.cpp
//------------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <Classes.hpp>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <shlobj.h>

// #include "winstuff.h"
// #include "puttyexp.h"
#include "FarUtils.h"

#include "TestTexts.h"
#include "Common.h"
#include "StrUtils.hpp"
#include "FileMasks.h"
#include "TextsCore.h"
#include "FileOperationProgress.h"
#include "HierarchicalStorage.h"
#include "GUITools.h"
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
    INFO("base_fixture_t ctor1");
    // CoreInitialize();
    ::SetGlobals(new TTestGlobalFunctions());
    INFO("base_fixture_t ctor2");
  }

  virtual ~base_fixture_t()
  {
    INFO("base_fixture_t dtor1");
//    delete FarPlugin;
    // CoreFinalize();
    TGlobalsIntf * Intf = GetGlobals();
    SAFE_DESTROY_EX(TGlobalsIntf, Intf);
    ::SetGlobals(nullptr);
    INFO("base_fixture_t dtor2");
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

//TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst)
//{
//  return CreateStub();
//}

//------------------------------------------------------------------------------

TEST_CASE_METHOD(base_fixture_t, "test1", "netbox")
{
  if (1)
  {
    UnicodeString Text = ::StringOfChar(' ', 4);
    REQUIRE_EQUAL(L"    ", Text);
  }
  if (1)
  {
    UnicodeString Message = L"long long long long long long long long long text";
    TStringList MessageLines;
    int MaxMessageWidth = 20;
    FarWrapText(Message, &MessageLines, MaxMessageWidth);
    INFO("MessageLines = " << MessageLines.GetText());
    REQUIRE_EQUAL(3, MessageLines.GetCount());
    REQUIRE_EQUAL(UnicodeString(L"long long long long"), MessageLines.GetString(0));
    REQUIRE_EQUAL(UnicodeString(L"long long long long"), MessageLines.GetString(1));
    REQUIRE_EQUAL(UnicodeString(L"long text"), MessageLines.GetString(2));
    // REQUIRE_EQUAL(UnicodeString("text"), W2MB(MessageLines.GetString(3).c_str()));
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

#if 0
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
//    INFO("TClass3: ClickEventHandler triggered");
    ClickEventHandlerTriggered = true;
  }
public:
  bool ClickEventHandlerTriggered;
};

TEST_CASE_METHOD(base_fixture_t, "test2", "netbox")
{
  INFO("1");
  if (1)
  {
    TClass2 cl2;
    REQUIRE_EQUAL(false, cl2.OnClickTriggered);
    cl2.Click();
    REQUIRE_EQUAL(true, cl2.OnClickTriggered);
  }
  INFO("2");
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
#endif

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

TEST_CASE_METHOD(base_fixture_t, "testStringList", "netbox")
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
}

TEST_CASE_METHOD(base_fixture_t, "testStorage", "netbox")
{
  UnicodeString RootKey = L"Software\\Michael Lukashov\\TestNetBox";
  TRegistryStorage Storage(RootKey);
  // printf("1\n");
  Storage.SetAccessMode(smReadWrite);
  // printf("1.1\n");
  REQUIRE(Storage.OpenRootKey(true));
  // printf("1.2\n");
  UnicodeString SubKey = L"SubKey1";
  // printf("1.3\n");
  Storage.RecursiveDeleteSubKey(SubKey);
  // printf("2\n");
  REQUIRE(!Storage.KeyExists(SubKey));
  REQUIRE(Storage.OpenSubKey(SubKey, true));
  Storage.SetAccessMode(smReadWrite);
  Storage.WriteInteger(L"IntVal", 1234);
  // printf("3\n");
  // INFO("Storage.GetFailed = " << Storage.GetFailed());
  Storage.CloseSubKey();
  REQUIRE(Storage.KeyExists(SubKey));
  REQUIRE(Storage.OpenSubKey(SubKey, false));
  // printf("4\n");
  intptr_t res = Storage.ReadInteger(L"IntVal", -1);
  INFO("res = " << res);
  REQUIRE(1234 == res);
}

TEST_CASE_METHOD(base_fixture_t, "testAppendPathDelimiterW", "netbox")
{
  UnicodeString path = L"C:\\test";
  AppendPathDelimiter(path);
  REQUIRE(path == L"C:\\test\\");
}

TEST_CASE_METHOD(base_fixture_t, "testSCP", "netbox")
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

TEST_CASE_METHOD(base_fixture_t, "testDerivedClass", "netbox")
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

TEST_CASE_METHOD(base_fixture_t, "testReplaceStr", "netbox")
{
  {
    INFO("1");
    UnicodeString str = ::ReplaceStr(L"AA", L"A", L"B");
    INFO("str: " << str);
    REQUIRE(W2MB(str.c_str()) == "BB");
  }
  {
    INFO("2");
    UnicodeString str = ::AnsiReplaceStr(L"AA", L"A", L"B");
    REQUIRE(W2MB(str.c_str()) == "BB");
  }
  {
    INFO("3");
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
    REQUIRE(W2MB(str.c_str()) == "aa");
  }
  {
    UnicodeString str = ::UpperCase(L"aa");
    REQUIRE(W2MB(str.c_str()) == "AA");
  }
  {
    UnicodeString str = ::Trim(L" aa ");
    REQUIRE(W2MB(str.c_str()) == "aa");
  }
}

TEST_CASE_METHOD(base_fixture_t, "testFileMasks", "netbox")
{
  if (1)
  {
    TFileMasks m(L"*.txt;*.log");
    bool res = m.Matches(L"test.exe", true, false);
    REQUIRE(!res);
  }
  if (1)
  {
    REQUIRE_EQUAL(true, TFileMasks::IsMask(L"*.txt;*.log;*.exe,*.cmd|*.bat"));
    // REQUIRE_EQUAL(true, TFileMasks::IsAnyMask(L"*.*"));
    TFileMasks m(L"*.txt;*.log");
    REQUIRE_EQUAL(false, m.Matches(L"test.exe", true, false));
  }
  {
    TFileMasks m(L"*.txt;*.log");
    REQUIRE_EQUAL(true, m.Matches(L"test.txt", true, false));
  }
  if (1)
  {
    TFileMasks m(L"*.txt;*.log");
    REQUIRE_EQUAL(true, m.Matches(L"test.log", true, false));

//    intptr_t Start, Length;
//    REQUIRE_EQUAL(true, m.GetIsValid(Start, Length));
    m.SetMask(L"*.exe");
    REQUIRE_EQUAL(true, m.Matches(L"test.exe", true, false));
    REQUIRE_EQUAL(false, m.Matches(L"test.txt", true, false));
    REQUIRE_EQUAL(false, m.Matches(L"test.log", true, false));
  }
}

TEST_CASE_METHOD(base_fixture_t, "testProgramsFolder", "netbox")
{
  UnicodeString ProgramsFolder;
  ::SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
  INFO("ProgramsFolder = " << ProgramsFolder.c_str());
  REQUIRE(ProgramsFolder.Length() > 0);
}

// TEST_CASE_METHOD(base_fixture_t, "test_random", "netbox")
// {
// random_ref();
// random_unref();
// }

TEST_CASE_METHOD(base_fixture_t, "testEncrypt", "netbox")
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

TEST_CASE_METHOD(base_fixture_t, "testFarPlugin1", "netbox")
{
  if (1)
  {
    HINSTANCE HInst = GetModuleHandle(0);
  }
}

TEST_CASE_METHOD(base_fixture_t, "testFarPlugin2", "netbox")
{
  // FarPlugin->RunTests();
}
