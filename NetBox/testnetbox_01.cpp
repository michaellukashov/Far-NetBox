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
    if (1)
    {
        TStringList strings;
        BOOST_CHECK_EQUAL(0, strings.GetCount());
        strings.Add(L"line 1");
        BOOST_CHECK_EQUAL(1, strings.GetCount());
        {
            std::wstring str = strings.GetString(0);
            BOOST_CHECK_EQUAL(::W2MB(str.c_str()), "line 1");
        }
        strings.PutString(0, L"line 0");
        {
            std::wstring str = strings.GetString(0);
            BOOST_CHECK_EQUAL(::W2MB(str.c_str()), "line 0");
        }
    }
    if (0)
    {
        TStringList strings;
        BOOST_CHECK_EQUAL(0, strings.GetCount());
        strings.Add(L"line 1");
        BOOST_CHECK_EQUAL(::W2MB(strings.GetText().c_str()).c_str(), "line 1");
        strings.Add(L"line 2");
        BOOST_CHECK_EQUAL(2, strings.GetCount());
        BOOST_CHECK_EQUAL(::W2MB(strings.GetText().c_str()).c_str(), "line 1\nline 2");
        strings.Insert(0, L"line 0");
        BOOST_CHECK_EQUAL(3, strings.GetCount());
        BOOST_CHECK_EQUAL(::W2MB(strings.GetText().c_str()).c_str(), "line 0\nline 1\nline 2");
        strings.PutObject(0, NULL);
        std::wstring str = strings.GetString(0);
        BOOST_CHECK_EQUAL(::W2MB(str.c_str()), "line 0");
    }
}

BOOST_AUTO_TEST_SUITE_END()
