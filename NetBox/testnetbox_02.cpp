//------------------------------------------------------------------------------
// testnetbox_02.cpp
// Тесты для NetBox
// testnetbox_02 --run_test=testnetbox_02/test1 --log_level=all
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_02"
#define BOOST_TEST_MAIN
// #define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/signals/signal1.hpp>
#include <boost/signals/signal2.hpp>
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

BOOST_AUTO_TEST_SUITE(testnetbox_02)

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

class TClass2 // : public boost::signals::trackable
{
  // typedef void result_type;
  typedef boost::signal2<void, TClass2 *, int> click_signal_type;
  typedef click_signal_type::slot_type click_slot_type;
  // typedef click_signal_type::slot_function_type click_slot_type;

public:
    TClass2() :
        OnClickTriggered(false)
    {
    }
    
    const click_signal_type &GetOnClick() const { return m_OnClick; }
    boost::signals::connection SetOnClick(const click_slot_type& onClick)
    {
        return m_OnClick.connect(onClick);
        // DEBUG_PRINTF(L"m_OnClick.num_slots = %d", m_OnClick.num_slots());
    }
    void Click()
    {
        m_OnClick(this, 1);
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

BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
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
        cl2.SetOnClick(boost::bind(&TClass3::ClickEventHandler, &cl3, _1, _2));
        BOOST_CHECK(cl2.GetOnClick().num_slots() > 0);
        cl2.Click();
        BOOST_CHECK_EQUAL(true, cl2.OnClickTriggered);
        BOOST_CHECK_EQUAL(true, cl3.ClickEventHandlerTriggered);
    }
}

BOOST_AUTO_TEST_SUITE_END()
