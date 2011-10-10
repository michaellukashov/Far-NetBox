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

#include "testutils.h"
#include "delegate.h"
#include "TestTexts.h"
#include "Common.h"
#include "FileMasks.h"
#include "WinSCPPlugin.h"
#include "GUITools.h"
#include "GUIConfiguration.h"
#include "TextsCore.h"

using namespace boost::unit_test;

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t : TObject
{
public:
    base_fixture_t() :
        TObject()
    {
        // BOOST_TEST_MESSAGE("base_fixture_t ctor");
        FarPlugin = CreateStub();
    }

    virtual ~base_fixture_t()
    {
        delete FarPlugin;
        FarPlugin = NULL;
    }

    bool scp_test(std::string host, int port, std::string user, std::string password);
};

//------------------------------------------------------------------------------

bool base_fixture_t::scp_test(std::string host, int port, std::string user, std::string password)
{
    return false;
}

//------------------------------------------------------------------------------

TCustomFarPlugin *CreateFarPlugin(HINSTANCE HInst);

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
    BOOST_CHECK(::FileExists(::MB2W(filename.c_str())));
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
        // DEBUG_PRINTF(L"str1 = \"%s\"", str1.c_str());
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

BOOST_FIXTURE_TEST_CASE(test9, base_fixture_t)
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

BOOST_FIXTURE_TEST_CASE(test10, base_fixture_t)
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

BOOST_FIXTURE_TEST_CASE(test11, base_fixture_t)
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

BOOST_FIXTURE_TEST_CASE(test12, base_fixture_t)
{
    std::wstring ProgramsFolder;
    ::SpecialFolderLocation(CSIDL_PROGRAM_FILES, ProgramsFolder);
    BOOST_TEST_MESSAGE("ProgramsFolder = " << ::W2MB(ProgramsFolder.c_str()).c_str());
    BOOST_CHECK(ProgramsFolder.size() > 0);
}

BOOST_FIXTURE_TEST_CASE(test13, base_fixture_t)
{
    random_ref();
    random_unref();
}

BOOST_FIXTURE_TEST_CASE(test14, base_fixture_t)
{
    BOOST_TEST_MESSAGE("RAND_MAX = " << RAND_MAX);
    for (int i = 0; i < 10; i++)
    {
        BOOST_TEST_MESSAGE("rand() = " << rand());
        BOOST_TEST_MESSAGE("random(256) = " << random(256));
    }
    std::wstring enc = ::EncryptPassword(L"1234ABC", L"234556");
    BOOST_TEST_MESSAGE("enc = " << ::W2MB(enc.c_str()).c_str());
    std::wstring dec = ::DecryptPassword(enc, L"234556");
    BOOST_TEST_MESSAGE("dec = " << ::W2MB(dec.c_str()).c_str());
    BOOST_CHECK(dec == L"1234ABC");
}

BOOST_AUTO_TEST_SUITE_END()
