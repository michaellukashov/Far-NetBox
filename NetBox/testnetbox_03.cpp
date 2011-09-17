//------------------------------------------------------------------------------
// testnetbox_03.cpp
// Тесты для NetBox
// testnetbox_03 --run_test=testnetbox_03/test1 --log_level=all
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_03"
#define BOOST_TEST_MAIN
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/signals/signal1.hpp>
#include <boost/bind.hpp>

#include "FarUtil.h"

#include "TestTexts.h"
#include "Common.h"

using namespace boost::unit_test;

#define TEST_CASE_TODO(exp) \
    std::cerr << "TODO: " << #exp << std::endl

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

BOOST_AUTO_TEST_SUITE(testnetbox_03)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
    if (1)
    {
        std::wstring Text = ::StringOfChar(' ', 4);
        BOOST_CHECK_EQUAL("    ", ::W2MB(Text.c_str()).c_str());
    }
    if (1)
    {
        std::wstring Message = L"long long long long long long long long long text";
        TStringList MessageLines;
        int MaxMessageWidth = 20;
        FarWrapText(Message, &MessageLines, MaxMessageWidth);
        BOOST_TEST_MESSAGE("MessageLines = " << ::W2MB(MessageLines.GetText().c_str()));
        BOOST_CHECK_EQUAL(3, MessageLines.GetCount());
        BOOST_CHECK_EQUAL("long long long long ", ::W2MB(MessageLines.GetString(0).c_str()).c_str());
        BOOST_CHECK_EQUAL("long long long long ", ::W2MB(MessageLines.GetString(1).c_str()).c_str());
        BOOST_CHECK_EQUAL("long text", ::W2MB(MessageLines.GetString(2).c_str()).c_str());
    }
}

BOOST_AUTO_TEST_SUITE_END()
