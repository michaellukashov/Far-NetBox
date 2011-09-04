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
    Config cfg; //  = new Config();
    memset(&cfg, 0, sizeof(cfg));
    cfg.logtype = LGTYP_ASCII;
    void *ctx = log_init(NULL, &cfg);
    // strcpy(&ctx->currlogfilename.path, "putty.log");
    logfopen(ctx);
    log_eventlog(ctx, "test2: start");
    
    char buf[256];
    struct tm tm = ltime();
    BOOST_TEST_MESSAGE("buf = " << buf << ", sizeof(buf) = " << sizeof(buf));
    strftime2(buf, sizeof(buf) - 1, "%Y.%m.%d %H:%M:%S", &tm);
    // BOOST_CHECK(buf == "2011.09.04 10:07:37");
    log_eventlog(ctx, "test2: end");
    logfclose(ctx);
    log_free(ctx);
}

BOOST_AUTO_TEST_SUITE_END()
