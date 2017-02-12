//------------------------------------------------------------------------------
// testnetbox_04.cpp
//------------------------------------------------------------------------------

#include <Classes.hpp>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <Common.h>
#include <FileBuffer.h>

#include "TestTexts.h"
#include "FarPlugin.h"
#include "testutils.h"

#include "neon/src/ne_session.h"
#include "neon/src/ne_request.h"

// #include "dl/include.hpp"

#include "calculator.hpp"
#include "DynamicQueue.hpp"

#include "testutils.h"

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
  base_fixture_t()
  {
    InitPlatformId();
  }
  virtual ~base_fixture_t()
  {
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

TEST_CASE_METHOD(base_fixture_t, "test1", "netbox")
{
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

  ne_session * sess = ne_session_create("http", "farmanager.com", 80);
  ne_request * req = ne_request_create(sess, "GET", "/svn/trunk/unicode_far/vbuild.m4");
  std::vector<unsigned char> vec;
  // ne_add_response_body_reader(req, ne_accept_2xx, read_block, &vec);
  int rv = ne_request_dispatch(req);
  INFO("rv = " << rv);
  if (rv)
  {
    FAIL("Request failed: " << ne_get_error(sess));
  }
  REQUIRE(rv == 0);
  const ne_status * statstruct = ne_get_status(req);
  INFO("statstruct->code = " << statstruct->code);
  INFO("statstruct->reason_phrase = " << statstruct->reason_phrase);
  if (vec.size() > 0)
  {
    INFO("response = " << (char *)&vec[0]);
  }
  ne_request_destroy(req);
  ne_session_destroy(sess);
  
  WSACleanup();
}

/*TEST_CASE_METHOD(base_fixture_t, "test2", "netbox")
{
  team::calculator calc("calculator_dll.dll");
  INFO("sum = " << calc.sum(10, 20));
  INFO("mul = " << calc.mul(10, 20));
  INFO("sqrt = " << calc.sqrt(25));
}*/

TEST_CASE_METHOD(base_fixture_t, "test3", "netbox")
{
  DynamicQueue<int> q;
  q.Reserve(10);
  q.Put(1);
  q.Put(2);
  q.Put(3);
  int val = q.Get();
  INFO("val = " << val);
  REQUIRE(val == 1);
  val = q.Get();
  INFO("val = " << val);
  REQUIRE(val == 2);
  val = q.Get();
  INFO("val = " << val);
  REQUIRE(val == 3);
}
