//------------------------------------------------------------------------------
// testnetbox_04.cpp
// Тесты для NetBox
// testnetbox_04 --run_test=testnetbox_04/test1 --log_level=all 2>&1 | tee res.txt
//------------------------------------------------------------------------------

#include "nbafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_04"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/scope_exit.hpp>

#include "TestTexts.h"
#include "Common.h"
#include "FarPlugin.h"
#include "testutils.h"
#include "FileBuffer.h"

#include "ne_session.h"
#include "ne_request.h"

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
    InitPlatformId();
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    // ne_sock_init();
  }

  virtual ~base_fixture_t()
  {
    WSACleanup();
  }
public:
protected:
  static int read_block(void * udata, const char * data, size_t len)
  {
    std::vector<unsigned char> *vec = static_cast<std::vector<unsigned char> *>(udata);
    for (unsigned int n = 0; n < len; ++n)
      vec->push_back((unsigned char)data[n]);
  }
};

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(testnetbox_04)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
  ne_session * sess = ne_session_create("http", "farmanager.com", 80);
  ne_request * req = ne_request_create(sess, "GET", "/svn/trunk/unicode_far/vbuild.m4");
  std::vector<unsigned char> vec;
  // ne_add_response_body_reader(req, ne_accept_2xx, read_block, &vec);
  int rv = ne_request_dispatch(req);
  BOOST_TEST_MESSAGE("rv = " << rv);
  if (rv)
  {
    BOOST_FAIL("Request failed: " << ne_get_error(sess));
  }
  BOOST_CHECK(rv == 0);
  const ne_status * statstruct = ne_get_status(req);
  BOOST_TEST_MESSAGE("statstruct->code = " << statstruct->code);
  BOOST_TEST_MESSAGE("statstruct->reason_phrase = " << statstruct->reason_phrase);
  if (vec.size() > 0)
  {
    BOOST_TEST_MESSAGE("response = " << (char *)&vec[0]);
  }
  ne_request_destroy(req);
  ne_session_destroy(sess);
}

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
  UnicodeString FileName("vbuild.m4");
  DeleteFile(FileName);
  // std::ofstream o(AnsiString(FileName).c_str(), std::ios::binary);
  // Neon::Request r("farmanager.com", "/svn/trunk/unicode_far/vbuild.m4");
  // o << r; 
  // o.close();
  BOOST_CHECK(::FileExists(FileName));
}

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
  std::string hostname = "farmanager.com";
  std::string scheme = "http";
  // Neon::HttpPort port = 80;
  // Neon::Session session(hostname, scheme, port);
}

BOOST_AUTO_TEST_SUITE_END()
