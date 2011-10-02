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
// #include <boost/type_traits/is_base_of.hpp>
#include <boost/scope_exit.hpp>

#include "TestTexts.h"
#include "Common.h"
#include "FarPlugin.h"
#include "testutils.h"

using namespace boost::unit_test;

//------------------------------------------------------------------------------
// stub
TCustomFarPlugin *FarPlugin = NULL;
//------------------------------------------------------------------------------

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
    base_fixture_t()
    {
        // BOOST_TEST_MESSAGE("base_fixture_t ctor");
    }

    virtual ~base_fixture_t()
    {
    }
public:
protected:
};

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(testnetbox_01)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
    TList list;
    BOOST_CHECK_EQUAL(0, list.GetCount());
    TObject obj1;
    TObject obj2;
    if (1)
    {
        list.Add(&obj1);
        BOOST_CHECK_EQUAL(1, list.GetCount());
        list.Add(&obj2);
        BOOST_CHECK_EQUAL(2, list.GetCount());
        BOOST_CHECK_EQUAL(0, list.IndexOf(&obj1));
        BOOST_CHECK_EQUAL(1, list.IndexOf(&obj2));
    }
    list.Clear();
    if (1)
    {
        BOOST_CHECK_EQUAL(0, list.GetCount());
        list.Insert(0, &obj1);
        BOOST_CHECK_EQUAL(1, list.GetCount());
        list.Insert(0, &obj2);
        BOOST_CHECK_EQUAL(2, list.GetCount());
        BOOST_CHECK_EQUAL(1, list.IndexOf(&obj1));
        BOOST_CHECK_EQUAL(0, list.IndexOf(&obj2));
    }
    if (1)
    {
        list.Delete(1);
        BOOST_CHECK_EQUAL(1, list.GetCount());
        BOOST_CHECK_EQUAL(-1, list.IndexOf(&obj1));
        BOOST_CHECK_EQUAL(0, list.IndexOf(&obj2));
        BOOST_CHECK_EQUAL(1, list.Add(&obj1));
        list.Delete(0);
        BOOST_CHECK_EQUAL(0, list.IndexOf(&obj1));
        BOOST_CHECK_EQUAL(0, list.Remove(&obj1));
        BOOST_CHECK_EQUAL(-1, list.IndexOf(&obj1));
        BOOST_CHECK_EQUAL(0, list.GetCount());
    }
    if (1)
    {
        list.Add(&obj1);
        list.Add(&obj2);
        list.Extract(&obj1);
        BOOST_CHECK_EQUAL(1, list.GetCount());
        BOOST_CHECK_EQUAL(0, list.IndexOf(&obj2));
        list.Add(&obj1);
        BOOST_CHECK_EQUAL(2, list.GetCount());
        list.Move(0, 1);
        BOOST_CHECK_EQUAL(0, list.IndexOf(&obj1));
        BOOST_CHECK_EQUAL(1, list.IndexOf(&obj2));
    }
}

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
    std::wstring str;
    if (1)
    {
        TStringList strings;
        BOOST_CHECK_EQUAL(0, strings.GetCount());
        BOOST_CHECK_EQUAL(0, strings.Add(L"line 1"));
        BOOST_CHECK_EQUAL(1, strings.GetCount());
        str = strings.GetString(0);
        // DEBUG_PRINTF(L"str = %s", str.c_str());
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), "line 1");
        strings.PutString(0, L"line 0");
        BOOST_CHECK_EQUAL(1, strings.GetCount());
        str = strings.GetString(0);
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), "line 0");
        strings.PutString(0, L"line 00");
        BOOST_CHECK_EQUAL(1, strings.GetCount());
        BOOST_CHECK_EQUAL(::W2MB(strings.GetString(0).c_str()), "line 00");
        strings.Add(L"line 11");
        BOOST_CHECK_EQUAL(2, strings.GetCount());
        BOOST_CHECK_EQUAL(::W2MB(strings.GetString(1).c_str()), "line 11");
        strings.Delete(1);
        BOOST_CHECK_EQUAL(1, strings.GetCount());
    }
    TStringList strings;
    if (1)
    {
        BOOST_CHECK_EQUAL(0, strings.GetCount());
        strings.Add(L"line 1");
        str = strings.GetText();
        // DEBUG_PRINTF(L"str = %s", str.c_str());
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "line 1\n");
    }
    if (1)
    {
        strings.Add(L"line 2");
        BOOST_CHECK_EQUAL(2, strings.GetCount());
        str = strings.GetText();
        // DEBUG_PRINTF(L"str = %s", str.c_str());
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "line 1\nline 2\n");
        strings.Insert(0, L"line 0");
        BOOST_CHECK_EQUAL(3, strings.GetCount());
        str = strings.GetText();
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()).c_str(), "line 0\nline 1\nline 2\n");
        strings.PutObject(0, NULL);
        std::wstring str = strings.GetString(0);
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), "line 0");
    }
}

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
    std::wstring Text = L"text text text text text1\ntext text text text text2\n";
    TStringList Lines;
    Lines.SetText(Text);
    BOOST_CHECK_EQUAL(2, Lines.GetCount());
    BOOST_TEST_MESSAGE("Lines 0 = " << ::W2MB(Lines.GetString(0).c_str()));
    BOOST_TEST_MESSAGE("Lines 1 = " << ::W2MB(Lines.GetString(1).c_str()));
    BOOST_CHECK_EQUAL("text text text text text1", ::W2MB(Lines.GetString(0).c_str()).c_str());
    BOOST_CHECK_EQUAL("text text text text text2", ::W2MB(Lines.GetString(1).c_str()).c_str());
}

BOOST_FIXTURE_TEST_CASE(test4, base_fixture_t)
{
    std::wstring Text = L"text, text text, text text1\ntext text text, text text2\n";
    TStringList Lines;
    Lines.SetCommaText(Text);
    BOOST_CHECK_EQUAL(6, Lines.GetCount());
    BOOST_CHECK_EQUAL("text", ::W2MB(Lines.GetString(0).c_str()).c_str());
    BOOST_CHECK_EQUAL(" text text", ::W2MB(Lines.GetString(1).c_str()).c_str());
    BOOST_CHECK_EQUAL(" text text1", ::W2MB(Lines.GetString(2).c_str()).c_str());
    BOOST_CHECK_EQUAL("text text text", ::W2MB(Lines.GetString(3).c_str()).c_str());
    BOOST_CHECK_EQUAL(" text text2", ::W2MB(Lines.GetString(4).c_str()).c_str());
    std::wstring Text2 = Lines.GetCommaText();
    BOOST_TEST_MESSAGE("Text2 = " << ::W2MB(Text2.c_str()));
    BOOST_CHECK_EQUAL("\"text\",\" text text\",\" text text1\",\"text text text\",\" text text2\",\"\"", ::W2MB(Text2.c_str()).c_str());
}

BOOST_FIXTURE_TEST_CASE(test5, base_fixture_t)
{
    TStringList Lines;
    TObject obj1;
    Lines.InsertObject(0, L"line 1", &obj1);
    BOOST_CHECK(&obj1 == Lines.GetObject(0));
}

BOOST_FIXTURE_TEST_CASE(test6, base_fixture_t)
{
    TStringList Lines;
    Lines.Add(L"bbb");
    Lines.Add(L"aaa");
    // BOOST_TEST_MESSAGE("Lines = " << ::W2MB(Lines.GetText().c_str()).c_str());
    {
        Lines.SetSorted(true);
        // BOOST_TEST_MESSAGE("Lines = " << ::W2MB(Lines.GetText().c_str()).c_str());
        BOOST_CHECK_EQUAL("aaa", ::W2MB(Lines.GetString(0).c_str()).c_str());
        BOOST_CHECK_EQUAL(2, Lines.GetCount());
    }
    {
        Lines.SetSorted(false);
        Lines.Add(L"Aaa");
        Lines.SetCaseSensitive(true);
        Lines.SetSorted(true);
        BOOST_CHECK_EQUAL(3, Lines.GetCount());
        // BOOST_TEST_MESSAGE("Lines = " << ::W2MB(Lines.GetText().c_str()).c_str());
        BOOST_CHECK_EQUAL("aaa", ::W2MB(Lines.GetString(0).c_str()).c_str());
        BOOST_CHECK_EQUAL("Aaa", ::W2MB(Lines.GetString(1).c_str()).c_str());
        BOOST_CHECK_EQUAL("bbb", ::W2MB(Lines.GetString(2).c_str()).c_str());
    }
}

BOOST_FIXTURE_TEST_CASE(test7, base_fixture_t)
{
    TStringList Lines;
    {
        Lines.Add(L"bbb");
        BOOST_TEST_MESSAGE("before try");
        try
        {
            BOOST_TEST_MESSAGE("before BOOST_SCOPE_EXIT");
            BOOST_SCOPE_EXIT( (&Lines) )
            {
                BOOST_TEST_MESSAGE("in BOOST_SCOPE_EXIT");
                BOOST_CHECK(1 == Lines.GetCount());
            } BOOST_SCOPE_EXIT_END
            // throw std::exception("");
            BOOST_TEST_MESSAGE("after BOOST_SCOPE_EXIT_END");
        }
        catch (...)
        {
            BOOST_TEST_MESSAGE("in catch(...) block");
        }
        BOOST_TEST_MESSAGE("after try");
        Lines.Add(L"aaa");
        BOOST_CHECK(2 == Lines.GetCount());
    }
    Lines.Clear();
    Lines.BeginUpdate();
    {
        Lines.Add(L"bbb");
        BOOST_TEST_MESSAGE("before block");
        {
            BOOST_TEST_MESSAGE("before BOOST_SCOPE_EXIT");
            BOOST_SCOPE_EXIT( (&Lines) )
            {
                BOOST_TEST_MESSAGE("in BOOST_SCOPE_EXIT");
                BOOST_CHECK(1 == Lines.GetCount());
                Lines.EndUpdate();
            } BOOST_SCOPE_EXIT_END
            // throw std::exception("");
            BOOST_TEST_MESSAGE("after BOOST_SCOPE_EXIT_END");
        }
        BOOST_TEST_MESSAGE("after block");
        Lines.Add(L"aaa");
        BOOST_CHECK(2 == Lines.GetCount());
    }
    Lines.Clear();
    int cnt = 0;
    TStringList *Lines1 = new TStringList();
    int cnt1 = 0;
    TStringList *Lines2 = new TStringList();
    {
        Lines.BeginUpdate();
        cnt++;
        Lines1->BeginUpdate();
        cnt1++;
        Lines2->Add(L"bbb");
        BOOST_TEST_MESSAGE("before block");
        try
        {
            BOOST_TEST_MESSAGE("before BOOST_SCOPE_EXIT");
            BOOST_SCOPE_EXIT( (&Lines) (&Lines1) (&Lines2) (&cnt) (&cnt1) )
            {
                BOOST_TEST_MESSAGE("in BOOST_SCOPE_EXIT");
                Lines.EndUpdate();
                cnt--;
                Lines1->EndUpdate();
                cnt1--;
                delete Lines1;
                Lines1 = NULL;
                delete Lines2;
                Lines2 = NULL;
            } BOOST_SCOPE_EXIT_END
            BOOST_CHECK(1 == cnt);
            BOOST_CHECK(1 == cnt1);
            BOOST_CHECK(1 == Lines2->GetCount());
            throw std::exception("");
            BOOST_TEST_MESSAGE("after BOOST_SCOPE_EXIT_END");
        }
        catch (const std::exception &ex)
        {
            BOOST_TEST_MESSAGE("in catch block");
            BOOST_CHECK(NULL == Lines1);
            BOOST_CHECK(NULL == Lines2);
        }
        BOOST_TEST_MESSAGE("after block");
        BOOST_CHECK(0 == cnt);
        BOOST_CHECK(0 == cnt1);
        BOOST_CHECK(NULL == Lines1);
        BOOST_CHECK(NULL == Lines2);
    }
}

BOOST_FIXTURE_TEST_CASE(test8, base_fixture_t)
{
    std::wstring ProgramsFolder;
    std::wstring DefaultPuttyPathOnly = ::IncludeTrailingBackslash(ProgramsFolder) + L"PuTTY\\putty.exe";
    BOOST_CHECK(DefaultPuttyPathOnly == L"\\PuTTY\\putty.exe");
    BOOST_CHECK(L"" == ::ExcludeTrailingBackslash(::IncludeTrailingBackslash(ProgramsFolder)));
}

BOOST_FIXTURE_TEST_CASE(test9, base_fixture_t)
{
    std::wstring Folder = L"C:\\Program Files\\Putty";
    BOOST_TEST_MESSAGE("ExtractFileDir = " << ::W2MB(::ExtractFileDir(Folder).c_str()).c_str());
    BOOST_CHECK(L"C:\\Program Files" == ::ExtractFileDir(Folder));
    BOOST_CHECK(L"C:\\Program Files" == ::ExtractFilePath(Folder));
    BOOST_TEST_MESSAGE("GetCurrentDir = " << ::W2MB(::GetCurrentDir().c_str()).c_str());
    BOOST_CHECK(::GetCurrentDir().size() > 0);
    BOOST_CHECK(::DirectoryExists(::GetCurrentDir()));
}

BOOST_FIXTURE_TEST_CASE(test10, base_fixture_t)
{
    TDateTime dt = EncodeDateVerbose(2009, 12, 29);
    BOOST_TEST_MESSAGE("dt = " << dt);
    bg::date::ymd_type ymd(2009, 12, 29);
    BOOST_TEST_MESSAGE("ymd.year = " << ymd.year);
    unsigned short Y;
    unsigned short M;
    unsigned short D;
    dt.DecodeDate(Y, M, D);
    BOOST_CHECK(Y == ymd.year && M == ymd.month && D == ymd.day);
}

BOOST_AUTO_TEST_SUITE_END()
