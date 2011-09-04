//------------------------------------------------------------------------------
// testnetbox_01.cpp
// Тесты для NetBox
// testnetbox_01 --run_test=testnetbox_01/test1 --log_level=all
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <time.h>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_01"
#define BOOST_TEST_MAIN
#ifdef _WIN32
// #define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>

#include "winstuff.h"
#include "puttyexp.h"
#include "FarUtil.h"

using namespace boost::unit_test;

/*******************************************************************************
            test suite
*******************************************************************************/

struct base_fixture_t
{
    base_fixture_t()
    {
    }

    virtual ~base_fixture_t()
    {
    }
};

BOOST_AUTO_TEST_SUITE(testnetbox_01)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
    std::wstring path = L"C:\\test";
    AppendPathDelimiterW(path);
    BOOST_CHECK(path == L"C:\\test\\");
}

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
    char buf[256];
    struct tm tm = ltime();
    strftime2(buf, sizeof(buf) - 1, "%Y.%m.%d %H:%M:%S", &tm);
    BOOST_TEST_MESSAGE("buf = %s" << buf);
    // BOOST_CHECK(buf == "2011.09.04 10:07:37");
}

BOOST_AUTO_TEST_SUITE_END()
