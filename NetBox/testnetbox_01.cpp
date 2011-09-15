//------------------------------------------------------------------------------
// testnetbox_01.cpp
// Тесты для NetBox
// testnetbox_01 --run_test=testnetbox_01/test1 --log_level=all
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_01"
#define BOOST_TEST_MAIN
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "winstuff.h"
#include "puttyexp.h"
#include "FarUtil.h"

#include "delegate.h"
#include "TestTexts.h"
#include "Common.h"

using namespace boost::unit_test;

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
    base_fixture_t()
    {
    }

    virtual ~base_fixture_t()
    {
    }

    bool scp_test(std::string host, int port, std::string user, std::string password);
};

//------------------------------------------------------------------------------

bool base_fixture_t::scp_test(std::string host, int port, std::string user, std::string password)
{
    return false;
}

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(testnetbox_01)

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
    typedef CDelegate1<void, string> Callback;

    // Это метод класса App.
    void OutputToConsole(string str)
    {
        cout << str << endl;
    }

    // А это статический метод класса App.
    static void OutputToFile(string str)
    {
        ofstream fout(filename, ios::out | ios::ate | ios::app);
        fout << str << endl;
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
    is.open(filename.c_str(), ios::in);
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
        std::wstring str = LoadStr(CONST_TEST_STRING);
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("test string: \"%s\" %d"));
    }
    {
        std::wstring str = LoadStrPart(CONST_TEST_STRING2, 1);
        BOOST_TEST_MESSAGE("str = " << ::W2MB(str.c_str()));
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), std::string("test string part 1"));
    }
    {
        std::wstring str = LoadStrPart(CONST_TEST_STRING2, 2);
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
    BOOST_CHECK(scp_test(host, port, user, password));
}

BOOST_AUTO_TEST_SUITE_END()
