//------------------------------------------------------------------------------
// testnetbox_02.cpp
// Тесты для NetBox
// testnetbox_02 --run_test=testnetbox_02/test1 --log_level=all 2>&1 | tee res.txt
//------------------------------------------------------------------------------

#include "nbafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_02"
#define BOOST_TEST_MAIN
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
// #include <boost/type_traits/is_base_of.hpp>
#include <boost/signals/signal1.hpp>
#include <boost/bind.hpp>

#include "winstuff.h"
#include "puttyexp.h"
#include "FarUtil.h"

#include "testutils.h"
#include "TestTexts.h"
#include "Common.h"
#include "FileMasks.h"
#include "WinSCPPlugin.h"
#include "GUITools.h"
#include "GUIConfiguration.h"
#include "TextsCore.h"
#include "FileOperationProgress.h"
#include "HierarchicalStorage.h"
#include "CoreMain.h"

using namespace boost::unit_test;

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t : System::TObject
{
public:
    base_fixture_t() :
        System::TObject(),
        OnChangeNotifyEventTriggered(false),
        ClickEventHandlerTriggered(false),
        onStringListChangeTriggered(false)
    {
        // BOOST_TEST_MESSAGE("base_fixture_t ctor");
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
    void OnChangeNotifyEvent(System::TObject *Sender)
    {
        BOOST_TEST_MESSAGE("OnChangeNotifyEvent triggered");
        OnChangeNotifyEventTriggered = true;
    }
    void ClickEventHandler(System::TObject *Sender)
    {
        BOOST_TEST_MESSAGE("ClickEventHandler triggered");
        ClickEventHandlerTriggered = true;
    }
    void onStringListChange(System::TObject *Sender)
    {
        BOOST_TEST_MESSAGE("onStringListChange triggered");
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

TCustomFarPlugin *CreateFarPlugin(HINSTANCE HInst);

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(testnetbox_02)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
    if (1)
    {
        std::wstring Text = ::StringOfChar(' ', 4);
        BOOST_CHECK_EQUAL("    ", System::W2MB(Text.c_str()).c_str());
    }
    if (1)
    {
        std::wstring Message = L"long long long long long long long long long text";
        System::TStringList MessageLines;
        int MaxMessageWidth = 20;
        FarWrapText(Message, &MessageLines, MaxMessageWidth);
        BOOST_TEST_MESSAGE("MessageLines = " << System::W2MB(MessageLines.GetText().c_str()));
        BOOST_CHECK_EQUAL(3, MessageLines.GetCount());
        BOOST_CHECK_EQUAL("long long long long ", System::W2MB(MessageLines.GetString(0).c_str()).c_str());
        BOOST_CHECK_EQUAL("long long long long ", System::W2MB(MessageLines.GetString(1).c_str()).c_str());
        BOOST_CHECK_EQUAL("long text", System::W2MB(MessageLines.GetString(2).c_str()).c_str());
    }
}

class TClass1 : System::TObject
{
public:
    TClass1() :
        OnChangeNotifyEventTriggered(false)
    {
    }
    const System::notify_signal_type &GetOnChange() const { return FOnChange; }
    void SetOnChange(const TNotifyEvent &Event) { FOnChange.connect(Event); }
    virtual void Changed()
    {
        if (FOnChange.num_slots() > 0)
        {
            FOnChange(this);
            OnChangeNotifyEventTriggered = true;
        }
    }
    void Change(const std::wstring str)
    {
        Changed();
    }

    bool OnChangeNotifyEventTriggered;
private:
    System::notify_signal_type FOnChange;
};

class TClass2 // : public boost::signals::trackable
{
  // typedef void result_type;
  typedef boost::signal2<void, TClass2 *, int> click_signal_type;
  typedef click_signal_type::slot_type click_slot_type;
  // typedef click_signal_type::slot_function_type click_slot_type;

public:
    TClass2() :
        OnClickTriggered(false)
    {
    }
    
    const click_signal_type &GetOnClick() const { return m_OnClick; }
    boost::signals::connection SetOnClick(const click_slot_type &onClick)
    {
        return m_OnClick.connect(onClick);
        // DEBUG_PRINTF(L"m_OnClick.num_slots = %d", m_OnClick.num_slots());
    }
    void Click()
    {
        m_OnClick(this, 1);
        OnClickTriggered = true;
    }
    bool OnClickTriggered;
private:
    click_signal_type m_OnClick;
};

class TClass3
{
public:
    TClass3() :
        ClickEventHandlerTriggered(false)
    {
    }
    void ClickEventHandler(TClass2 *Sender, int Data)
    {
        BOOST_TEST_MESSAGE("TClass3: ClickEventHandler triggered");
        ClickEventHandlerTriggered = true;
    }
public:
    bool ClickEventHandlerTriggered;
};

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
    if (1)
    {
        TClass2 cl2;
        BOOST_CHECK_EQUAL(false, cl2.OnClickTriggered);
        cl2.Click();
        BOOST_CHECK_EQUAL(true, cl2.OnClickTriggered);
    }
    if (1)
    {
        TClass2 cl2;
        TClass3 cl3;
        cl2.SetOnClick(boost::bind(&TClass3::ClickEventHandler, &cl3, _1, _2));
        BOOST_CHECK(cl2.GetOnClick().num_slots() > 0);
        cl2.Click();
        BOOST_CHECK_EQUAL(true, cl2.OnClickTriggered);
        BOOST_CHECK_EQUAL(true, cl3.ClickEventHandlerTriggered);
    }
}

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
    if (1)
    {
        TClass1 cl1;
        BOOST_CHECK_EQUAL(false, cl1.OnChangeNotifyEventTriggered);
        cl1.SetOnChange(boost::bind(&base_fixture_t::OnChangeNotifyEvent, this, _1));
        cl1.Change(L"line 1");
        BOOST_CHECK_EQUAL(true, cl1.OnChangeNotifyEventTriggered);
    }
}

BOOST_FIXTURE_TEST_CASE(test4, base_fixture_t)
{
    if (1)
    {
        System::TStringList strings;
        strings.SetOnChange(boost::bind(&base_fixture_t::onStringListChange, this, _1));
        strings.Add(L"line 1");
        // BOOST_CHECK_EQUAL(true, OnChangeNotifyEventTriggered);
        BOOST_CHECK_EQUAL(true, onStringListChangeTriggered);
    }
}

BOOST_FIXTURE_TEST_CASE(test5, base_fixture_t)
{
    if (1)
    {
        TFileOperationProgressType OperationProgress;
    }
}

BOOST_FIXTURE_TEST_CASE(test6, base_fixture_t)
{
    BOOST_CHECK_THROW(System::Error(SListIndexError, 0), ExtException);
}

BOOST_FIXTURE_TEST_CASE(test7, base_fixture_t)
{
    System::TStringList Lines;
    Lines.SetSorted(true);
    if (1)
    {
        Lines.SetDuplicates(System::dupAccept);
        Lines.Add(L"aaa");
        Lines.Add(L"aaa");
        Lines.Add(L"bbb");
        BOOST_CHECK(3 == Lines.GetCount());
        BOOST_CHECK(0 == Lines.IndexOf(L"aaa"));
        BOOST_CHECK(2 == Lines.IndexOf(L"bbb"));
    }
    Lines.Clear();
    if (1)
    {
        Lines.SetDuplicates(System::dupIgnore);
        Lines.Add(L"aaa");
        Lines.Add(L"aaa");
        Lines.Add(L"bbb");
        BOOST_CHECK(2 == Lines.GetCount());
        BOOST_CHECK(1 == Lines.IndexOf(L"bbb"));
    }
    Lines.Clear();
    if (1)
    {
        Lines.SetDuplicates(System::dupError);
        Lines.Add(L"aaa");
        Lines.Add(L"bbb");
        BOOST_CHECK_THROW(Lines.Add(L"aaa"), std::exception);
    }
}

BOOST_FIXTURE_TEST_CASE(test8, base_fixture_t)
{
   std::wstring RootKey = L"Software\\Michael Lukashov\\TestNetBox";
   TRegistryStorage Storage(RootKey);
   Storage.SetAccessMode(smReadWrite);
   BOOST_CHECK(Storage.OpenRootKey(true));
   std::wstring SubKey = L"SubKey1";
   Storage.DeleteSubKey(SubKey);
   BOOST_CHECK(!Storage.KeyExists(SubKey));
   BOOST_CHECK(Storage.OpenSubKey(SubKey, true));
   Storage.SetAccessMode(smReadWrite);
   Storage.Writeint(L"IntVal", 1234);
   // BOOST_TEST_MESSAGE("Storage.GetFailed = " << Storage.GetFailed());
   Storage.CloseSubKey();
   BOOST_CHECK(Storage.KeyExists(SubKey));
   BOOST_CHECK(Storage.OpenSubKey(SubKey, false));
   int res = Storage.Readint(L"IntVal", -1);
   BOOST_TEST_MESSAGE("res = " << res);
   BOOST_CHECK(1234 == res);
}

BOOST_FIXTURE_TEST_CASE(test9, base_fixture_t)
{
    std::wstring path = L"C:\\test";
    AppendPathDelimiterW(path);
    BOOST_CHECK(path == L"C:\\test\\");
}

BOOST_FIXTURE_TEST_CASE(test10, base_fixture_t)
{
    Config cfg; //  = new Config();
    memset(&cfg, 0, sizeof(cfg));
    cfg.logtype = LGTYP_ASCII;
    void *ctx = log_init(NULL, &cfg);
    // strcpy(&ctx->currlogfilename.path, "putty.log");
    logfopen(ctx);
    log_eventlog(ctx, "test2: start");

    char buf[256];
    struct tm tm = ltime();
    time_t t = time(0);

    char buf2[256];
    _snprintf(buf2, sizeof(buf2) - 1, "%04d.%02d.%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    strftime2(buf, sizeof(buf) - 1, "%Y.%m.%d %H:%M:%S", &tm);
    BOOST_TEST_MESSAGE("buf = " << buf); //  << ", sizeof(buf) = " << sizeof(buf));
    BOOST_TEST_MESSAGE("buf2 = " << buf2);
    BOOST_CHECK(0 == strcmp(buf, buf2));
    log_eventlog(ctx, "test2: end");
    logfclose(ctx);
    log_free(ctx);
}

BOOST_FIXTURE_TEST_CASE(test11, base_fixture_t)
{
    // Тесты на ::FmtLoadStr FMTLOAD ::Format ::LoadStr ::LoadStrPart ::CutToChar ::TrimLeft ::TrimRight
    {
        std::wstring str = ::FmtLoadStr(CONST_TEST_STRING, L"lalala", 42);
        // BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        // BOOST_TEST_MESSAGE("length = " << str.size());
        BOOST_CHECK(System::W2MB(str.c_str()) == "test string: \"lalala\" 42");
    }
    {
        std::wstring str2 = FMTLOAD(CONST_TEST_STRING, L"lalala", 42);
        // BOOST_TEST_MESSAGE("str2 = " << System::W2MB(str2.c_str()));
        BOOST_CHECK(System::W2MB(str2.c_str()) == "test string: \"lalala\" 42");
    }
    {
        std::wstring str2 = ::Format(L"test: %s %d", L"lalala", 42);
        BOOST_TEST_MESSAGE("str2 = " << System::W2MB(str2.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str2.c_str()), std::string("test: lalala 42"));
    }
    {
        std::wstring str3 = FORMAT(L"test: %s %d", L"lalala", 42);
        BOOST_TEST_MESSAGE("str3 = " << System::W2MB(str3.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str3.c_str()), std::string("test: lalala 42"));
    }
    {
        std::wstring str = ::TrimLeft(L"");
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string(""));
    }
    {
        std::wstring str = ::TrimLeft(L"1");
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("1"));
    }
    {
        std::wstring str = ::TrimLeft(L" 1");
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("1"));
    }
    {
        std::wstring str = ::TrimRight(L"");
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string(""));
    }
    {
        std::wstring str = ::TrimRight(L"1");
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("1"));
    }
    {
        std::wstring str = ::TrimRight(L"1 ");
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("1"));
    }
    {
        // std::wstring CutToChar(std::wstring &Str, char Ch, bool Trim)
        std::wstring Str1 = L" part 1 | part 2 ";
        std::wstring str1 = ::CutToChar(Str1, '|', false);
        BOOST_TEST_MESSAGE("str1 = \"" << System::W2MB(str1.c_str()) << "\"");
        BOOST_TEST_MESSAGE("Str1 = \"" << System::W2MB(Str1.c_str()) << "\"");
        // BOOST_TEST_MESSAGE("Str1 = \"" << System::W2MB(Str1.c_str()) << "\"");
        // DEBUG_PRINTF(L"str1 = \"%s\"", str1.c_str());
        BOOST_CHECK_EQUAL(System::W2MB(str1.c_str()), std::string(" part 1 "));

        std::wstring str2 = ::CutToChar(Str1, '|', true);
        BOOST_TEST_MESSAGE("str2 = \"" << System::W2MB(str2.c_str()) << "\"");
        BOOST_TEST_MESSAGE("Str1 = \"" << System::W2MB(Str1.c_str()) << "\"");
        BOOST_CHECK_EQUAL(System::W2MB(str2.c_str()), std::string("part 2"));
    }
    {
        std::wstring str = ::LoadStr(CONST_TEST_STRING);
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("test string: \"%s\" %d"));
    }
    {
        std::wstring str = ::LoadStrPart(CONST_TEST_STRING2, 1);
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("test string part 1"));
    }
    {
        std::wstring str = ::LoadStrPart(CONST_TEST_STRING2, 2);
        BOOST_TEST_MESSAGE("str = " << System::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()), std::string("part 2"));
    }
}

BOOST_FIXTURE_TEST_CASE(test12, base_fixture_t)
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

BOOST_FIXTURE_TEST_CASE(test13, base_fixture_t)
{
    TBaseClass1 E1;
    TDerivedClass1 E2;
    TBaseClass2 E3;
    // typedef boost::is_base_of<TBaseClass1, TDerivedClass1>::type t1;
    BOOST_CHECK((::InheritsFrom<TBaseClass1, TBaseClass1>(&E1)));
    BOOST_CHECK((::InheritsFrom<TBaseClass1, TDerivedClass1>(&E2)));
    // BOOST_CHECK(!(::InheritsFrom<TBaseClass2, TDerivedClass1>(&E2)));
    // BOOST_CHECK(!(::InheritsFrom<TDerivedClass1, TBaseClass1>(E1)));
    // BOOST_CHECK(!(::InheritsFrom<TBaseClass1, TBaseClass2>(E3)));
}

BOOST_FIXTURE_TEST_CASE(test14, base_fixture_t)
{
    {
        std::wstring str = ::StringReplace(L"AA", L"A", L"B");
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()).c_str(), "BB");
    }
    {
        std::wstring str = ::AnsiReplaceStr(L"AA", L"A", L"B");
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()).c_str(), "BB");
    }
    {
        std::wstring str = L"ABC";
        BOOST_CHECK_EQUAL(::Pos(str, L"DEF"), -1);
        BOOST_CHECK_EQUAL(::Pos(str, L"AB"), 0);
        BOOST_CHECK_EQUAL(::Pos(str, L"BC"), 1);
        BOOST_CHECK_EQUAL(::AnsiPos(str, 'D'), -1);
        BOOST_CHECK_EQUAL(::AnsiPos(str, 'A'), 0);
        BOOST_CHECK_EQUAL(::AnsiPos(str, 'B'), 1);
    }
    {
        std::wstring str = ::LowerCase(L"AA");
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()).c_str(), "aa");
    }
    {
        std::wstring str = ::UpperCase(L"aa");
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()).c_str(), "AA");
    }
    {
        std::wstring str = ::Trim(L" aa ");
        BOOST_CHECK_EQUAL(System::W2MB(str.c_str()).c_str(), "aa");
    }
}

BOOST_FIXTURE_TEST_CASE(test15, base_fixture_t)
{
    if (1)
    {
        BOOST_CHECK_EQUAL(true, TFileMasks::IsMask(L"*.txt;*.log;*.exe,*.cmd|*.bat"));
        // BOOST_CHECK_EQUAL(true, TFileMasks::IsAnyMask(L"*.*"));
        TFileMasks m(L"*.txt;*.log");
        BOOST_CHECK_EQUAL(false, m.Matches(L"test.exe"));
    }
    {
        TFileMasks m(L"*.txt;*.log");
        BOOST_CHECK_EQUAL(true, m.Matches(L"test.txt"));
    }
    if (1)
    {
        TFileMasks m(L"*.txt;*.log");
        BOOST_CHECK_EQUAL(true, m.Matches(L"test.log"));

        size_t Start, Length;
        BOOST_CHECK_EQUAL(true, m.GetIsValid(Start, Length));
        m.SetMask(L"*.exe");
        BOOST_CHECK_EQUAL(true, m.Matches(L"test.exe"));
        BOOST_CHECK_EQUAL(false, m.Matches(L"test.txt"));
        BOOST_CHECK_EQUAL(false, m.Matches(L"test.log"));
    }
}

BOOST_FIXTURE_TEST_CASE(test16, base_fixture_t)
{
    if (1)
    {
        HINSTANCE HInst = GetModuleHandle(0);
        TWinSCPPlugin *FarPlugin = new TWinSCPPlugin(HInst);
        //DEBUG_PRINTF(L"FarPlugin = %x", FarPlugin);
        BOOST_CHECK(FarPlugin != NULL);
        // SAFE_DESTROY(FarPlugin);
        delete FarPlugin;
        // BOOST_CHECK(FarPlugin == NULL);
    }
}

BOOST_FIXTURE_TEST_CASE(test17, base_fixture_t)
{
    if (1)
    {
        HINSTANCE HInst = GetModuleHandle(0);
        TCustomFarPlugin *FarPlugin = CreateFarPlugin(HInst);
        //DEBUG_PRINTF(L"FarPlugin = %x", FarPlugin);
        BOOST_CHECK(FarPlugin != NULL);
        // SAFE_DESTROY(FarPlugin);
        delete FarPlugin;
        // BOOST_CHECK(FarPlugin == NULL);
    }
}

BOOST_FIXTURE_TEST_CASE(test18, base_fixture_t)
{
    TGUICopyParamType FDefaultCopyParam;
    TCopyParamType *CopyParam = new TCopyParamType(FDefaultCopyParam);
    CopyParam->SetTransferMode(tmAscii);
    TCopyParamList FCopyParamList;
    // BOOST_TEST_MESSAGE("FCopyParamList.Count = " << FCopyParamList.GetCount());
    FCopyParamList.Add(LoadStr(COPY_PARAM_PRESET_ASCII), CopyParam, NULL);
    // BOOST_TEST_MESSAGE("FCopyParamList.Count = " << FCopyParamList.GetCount());
    CopyParam = new TCopyParamType(FDefaultCopyParam);
    CopyParam->SetTransferMode(tmAscii);
    FCopyParamList.Add(LoadStr(COPY_PARAM_PRESET_BINARY), CopyParam, NULL);
    // BOOST_TEST_MESSAGE("FCopyParamList.Count = " << FCopyParamList.GetCount());
}

BOOST_FIXTURE_TEST_CASE(test19, base_fixture_t)
{
    std::wstring ProgramsFolder;
    ::SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
    BOOST_TEST_MESSAGE("ProgramsFolder = " << System::W2MB(ProgramsFolder.c_str()).c_str());
    BOOST_CHECK(ProgramsFolder.size() > 0);
}

// BOOST_FIXTURE_TEST_CASE(test20, base_fixture_t)
// {
    // random_ref();
    // random_unref();
// }

BOOST_FIXTURE_TEST_CASE(test21, base_fixture_t)
{
    BOOST_TEST_MESSAGE("RAND_MAX = " << RAND_MAX);
    for (int i = 0; i < 10; i++)
    {
        BOOST_TEST_MESSAGE("rand() = " << rand());
        BOOST_TEST_MESSAGE("random(256) = " << random(256));
    }
    std::wstring enc = ::EncryptPassword(L"1234ABC", L"234556");
    BOOST_TEST_MESSAGE("enc = " << System::W2MB(enc.c_str()).c_str());
    std::wstring dec = ::DecryptPassword(enc, L"234556");
    BOOST_TEST_MESSAGE("dec = " << System::W2MB(dec.c_str()).c_str());
    BOOST_CHECK(dec == L"1234ABC");
}

BOOST_FIXTURE_TEST_CASE(test22, base_fixture_t)
{
    // FarPlugin->RunTests();
}

BOOST_AUTO_TEST_SUITE_END()
