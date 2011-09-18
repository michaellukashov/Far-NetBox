//------------------------------------------------------------------------------
// testnetbox_10.cpp
// Тесты для NetBox
// testnetbox_10 --run_test=testnetbox_10/test1 --log_level=all
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_10"
#define BOOST_TEST_MAIN
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
// #include <boost/type_traits/is_base_of.hpp>
#include <boost/signals/signal1.hpp>
#include <boost/bind.hpp>

#include "winstuff.h"
#include "puttyexp.h"
#include "FarUtil.h"

#include "delegate.h"
#include "TestTexts.h"
#include "Common.h"
#include "FileMasks.h"

using namespace boost::unit_test;

#define TEST_CASE_TODO(exp) \
    std::cerr << "TODO: " << #exp << std::endl

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
        // BOOST_TEST_MESSAGE("base_fixture_t ctor");
    }

    virtual ~base_fixture_t()
    {
    }

    bool scp_test(std::string host, int port, std::string user, std::string password);

public:
    void OnChangeNotifyEvent(TObject *Sender)
    {
        BOOST_TEST_MESSAGE("OnChangeNotifyEvent triggered");
        OnChangeNotifyEventTriggered = true;
    }
    void ClickEventHandler(TObject *Sender)
    {
        BOOST_TEST_MESSAGE("ClickEventHandler triggered");
        ClickEventHandlerTriggered = true;
    }
    void onStringListChange(TObject *Sender)
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

BOOST_AUTO_TEST_SUITE(testnetbox_10)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
    std::wstring path = L"C:\\test";
    AppendPathDelimiterW(path);
    BOOST_CHECK(path == L"C:\\test\\");
}

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
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

static const std::string filename = "output.txt";

class App
{
public:
    App()
    {
        std::remove(filename.c_str());
    }
    // Определяем делегат Callback,
    // который принимает 1 параметр и ничего не возвращает.
    typedef CDelegate1<void, std::string> Callback;

    // Это метод класса App.
    void OutputToConsole(std::string str)
    {
        std::cout << str << std::endl;
    }

    // А это статический метод класса App.
    static void OutputToFile(std::string str)
    {
        std::ofstream fout(filename, std::ios::out | std::ios::ate | std::ios::app);
        fout << str << std::endl;
        fout.close();
    }
};

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
    App app;
    // Создаём делегат.
    App::Callback callback = NULL;
    BOOST_REQUIRE(callback.IsNull());
    if (!callback.IsNull()) callback("1");

    // Добавляем ссылку на OutputToFile.
    // Вызываем её через делегата.
    callback += NewDelegate(App::OutputToFile);
    BOOST_REQUIRE(!callback.IsNull());
    if (!callback.IsNull()) callback("2");

    // Добавляем ссылку на OutputToConsole.
    // Вызывается вся цепочка:
    // сначала OutputToFile, потом OutputToConsole.
    callback += NewDelegate(&app, &App::OutputToConsole);
    BOOST_REQUIRE(!callback.IsNull());
    if (!callback.IsNull()) callback("3");
    std::ifstream is;
    is.open(filename.c_str(), std::ios::in);
    BOOST_CHECK(!is.fail());
}

BOOST_FIXTURE_TEST_CASE(test4, base_fixture_t)
{
    // Тесты на ::FmtLoadStr FMTLOAD ::Format ::LoadStr ::LoadStrPart ::CutToChar ::TrimLeft ::TrimRight
    {
        std::wstring str = ::FmtLoadStr(CONST_TEST_STRING, L"lalala", 42);
        // BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        // BOOST_TEST_MESSAGE("length = " << str.size());
        BOOST_CHECK(::W2MB(str.c_str()) == "test string: \"lalala\" 42");
    }
    {
        std::wstring str2 = FMTLOAD(CONST_TEST_STRING, L"lalala", 42);
        // BOOST_TEST_MESSAGE("str2 = " << ::W2MB(str2.c_str()));
        BOOST_CHECK(::W2MB(str2.c_str()) == "test string: \"lalala\" 42");
    }
    {
        std::wstring str2 = ::Format(L"test: %s %d", L"lalala", 42);
        BOOST_TEST_MESSAGE("str2 = " << ::W2MB(str2.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str2.c_str()), std::string("test: lalala 42"));
    }
    {
        std::wstring str3 = FORMAT(L"test: %s %d", L"lalala", 42);
        BOOST_TEST_MESSAGE("str3 = " << ::W2MB(str3.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str3.c_str()), std::string("test: lalala 42"));
    }
    {
        std::wstring str = ::TrimLeft(L"");
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string(""));
    }
    {
        std::wstring str = ::TrimLeft(L"1");
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("1"));
    }
    {
        std::wstring str = ::TrimLeft(L" 1");
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("1"));
    }
    {
        std::wstring str = ::TrimRight(L"");
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string(""));
    }
    {
        std::wstring str = ::TrimRight(L"1");
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("1"));
    }
    {
        std::wstring str = ::TrimRight(L"1 ");
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("1"));
    }
    {
        // std::wstring CutToChar(std::wstring &Str, char Ch, bool Trim)
        std::wstring Str1 = L" part 1 | part 2 ";
        std::wstring str1 = ::CutToChar(Str1, '|', false);
        BOOST_TEST_MESSAGE("str1 = \"" << ::W2MB(str1.c_str()) << "\"");
        // BOOST_TEST_MESSAGE("Str1 = \"" << ::W2MB(Str1.c_str()) << "\"");
        // DEBUG_PRINTF(L"NetBox: str1 = \"%s\"", str1.c_str());
        BOOST_CHECK_EQUAL(::W2MB(str1.c_str()), std::string(" part 1 "));

        std::wstring str2 = ::CutToChar(Str1, '|', true);
        BOOST_TEST_MESSAGE("str2 = \"" << ::W2MB(str2.c_str()) << "\"");
        BOOST_CHECK_EQUAL(::W2MB(str2.c_str()), std::string("part 2"));
    }
    {
        std::wstring str = ::LoadStr(CONST_TEST_STRING);
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("test string: \"%s\" %d"));
    }
    {
        std::wstring str = ::LoadStrPart(CONST_TEST_STRING2, 1);
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("test string part 1"));
    }
    {
        std::wstring str = ::LoadStrPart(CONST_TEST_STRING2, 2);
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("part 2"));
    }
}

BOOST_FIXTURE_TEST_CASE(test5, base_fixture_t)
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

BOOST_FIXTURE_TEST_CASE(test6, base_fixture_t)
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

BOOST_FIXTURE_TEST_CASE(test7, base_fixture_t)
{
    {
        std::wstring str = ::StringReplace(L"AA", L"A", L"B");
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "BB");
    }
    {
        std::wstring str = ::AnsiReplaceStr(L"AA", L"A", L"B");
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "BB");
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
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "aa");
    }
    {
        std::wstring str = ::UpperCase(L"aa");
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "AA");
    }
    {
        std::wstring str = ::Trim(L" aa ");
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "aa");
    }
}

BOOST_FIXTURE_TEST_CASE(test8, base_fixture_t)
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

        int Start, Length;
        BOOST_CHECK_EQUAL(true, m.GetIsValid(Start, Length));
        m.SetMask(L"*.exe");
        BOOST_CHECK_EQUAL(true, m.Matches(L"test.exe"));
        BOOST_CHECK_EQUAL(false, m.Matches(L"test.txt"));
        BOOST_CHECK_EQUAL(false, m.Matches(L"test.log"));
    }
}

class TClass1 : TObject
{
public:
    TClass1() :
        FOnChange(NULL),
        OnChangeNotifyEventTriggered(false)
    {
    }
    
    TNotifyEvent GetOnChange() { return FOnChange; }
    void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }
    virtual void Changed()
    {
        if (FOnChange)
        {
            ((*this).*FOnChange)(this);
            OnChangeNotifyEventTriggered = true;
        }
    }
    void Change(std::wstring str)
    {
        Changed();
    }

    bool OnChangeNotifyEventTriggered;
private:
    TNotifyEvent FOnChange;
};

class TClass2
{
  typedef boost::signal1<void, TClass2 *, int> click_signal_type;
  typedef click_signal_type::slot_type click_slot_type;

public:
    TClass2() :
        OnClickTriggered(false)
    {
    }
    
    const click_signal_type &GetOnClick() const { return m_OnClick; }
    void SetOnClick(const click_slot_type& onClick)
    {
        m_OnClick.connect(onClick);
        // DEBUG_PRINTF(L"m_OnClick.num_slots = %d", m_OnClick.num_slots());
    }
    void Click()
    {
        m_OnClick(this);
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

BOOST_FIXTURE_TEST_CASE(test9, base_fixture_t)
{
    if (1)
    {
        TClass1 cl1;
        BOOST_CHECK_EQUAL(false, cl1.OnChangeNotifyEventTriggered);
        cl1.SetOnChange((TNotifyEvent)&base_fixture_t::OnChangeNotifyEvent);
        cl1.Change(L"line 1");
        BOOST_CHECK_EQUAL(true, cl1.OnChangeNotifyEventTriggered);
    }
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
        cl2.SetOnClick(boost::bind(&TClass3::ClickEventHandler, &cl3, _1));
        BOOST_CHECK(cl2.GetOnClick().num_slots() > 0);
        cl2.Click();
        BOOST_CHECK_EQUAL(true, cl2.OnClickTriggered);
        BOOST_CHECK_EQUAL(true, cl3.ClickEventHandlerTriggered);
    }
}

BOOST_FIXTURE_TEST_CASE(test10, base_fixture_t)
{
    if (1)
    {
        TStringList strings;
        strings.SetOnChange(boost::bind(&base_fixture_t::onStringListChange, this, _1));
        strings.Add(L"line 1");
        // BOOST_CHECK_EQUAL(true, OnChangeNotifyEventTriggered);
        BOOST_CHECK_EQUAL(true, onStringListChangeTriggered);
    }
}

BOOST_AUTO_TEST_SUITE_END()
