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

#include "testutils.h"
#include "TestTexts.h"
#include "Common.h"
#include "FileOperationProgress.h"
#include "HierarchicalStorage.h"

using namespace boost::unit_test;

// to compile with libputty
extern "C" void modalfatalbox(char * fmt, ...)
{
  va_list Param;
  va_start(Param, fmt);
  // SSHFatalError(fmt, Param);
  va_end(Param);
}

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
    base_fixture_t() :
        OnChangeNotifyEventTriggered(false),
        ClickEventHandlerTriggered(false),
        onStringListChangeTriggered(false)
    {
        // BOOST_TEST_MESSAGE("base_fixture_t ctor");
        FarPlugin = CreateStub();
    }

    virtual ~base_fixture_t()
    {
        delete FarPlugin;
        FarPlugin = NULL;
    }

public:
    void OnChangeNotifyEvent(TObject *Sender)
    {
        BOOST_TEST_MESSAGE("OnChangeNotifyEvent triggered");
        OnChangeNotifyEventTriggered = true;
    }
    void ClickEventHandler(TObject *Sender)
    {
        BOOST_TEST_MESSAGE("ClickEventHandler triggered");
        ClickEventHandlerTriggered = true;
    }
    void onStringListChange(TObject *Sender)
    {
        BOOST_TEST_MESSAGE("onStringListChange triggered");
        onStringListChangeTriggered = true;
    }
protected:
    bool OnChangeNotifyEventTriggered;
    bool ClickEventHandlerTriggered;
    bool onStringListChangeTriggered;
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

class TClass1 : TObject
{
public:
    TClass1() :
        OnChangeNotifyEventTriggered(false)
    {
    }
    const notify_signal_type &GetOnChange() const { return FOnChange; }
    void SetOnChange(const notify_slot_type &Event) { FOnChange.connect(Event); }
    virtual void Changed()
    {
        if (FOnChange.num_slots() > 0)
        {
            FOnChange(this);
            OnChangeNotifyEventTriggered = true;
        }
    }
    void Change(std::wstring str)
    {
        Changed();
    }

    bool OnChangeNotifyEventTriggered;
private:
    notify_signal_type FOnChange;
};

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
    boost::signals::connection SetOnClick(const click_slot_type &onClick)
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

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
    if (1)
    {
        TClass1 cl1;
        BOOST_CHECK_EQUAL(false, cl1.OnChangeNotifyEventTriggered);
        cl1.SetOnChange(boost::bind(&base_fixture_t::OnChangeNotifyEvent, this, _1));
        cl1.Change(L"line 1");
        BOOST_CHECK_EQUAL(true, cl1.OnChangeNotifyEventTriggered);
    }
}

BOOST_FIXTURE_TEST_CASE(test4, base_fixture_t)
{
    if (1)
    {
        TStringList strings;
        strings.SetOnChange(boost::bind(&base_fixture_t::onStringListChange, this, _1));
        strings.Add(L"line 1");
        // BOOST_CHECK_EQUAL(true, OnChangeNotifyEventTriggered);
        BOOST_CHECK_EQUAL(true, onStringListChangeTriggered);
    }
}

BOOST_FIXTURE_TEST_CASE(test5, base_fixture_t)
{
    if (1)
    {
        TFileOperationProgressType OperationProgress;
    }
}

BOOST_FIXTURE_TEST_CASE(test6, base_fixture_t)
{
    BOOST_CHECK_THROW(::Error(SListIndexError, 0), ExtException);
}

BOOST_FIXTURE_TEST_CASE(test7, base_fixture_t)
{
    TStringList Lines;
    Lines.SetSorted(true);
    if (1)
    {
        Lines.SetDuplicates(dupAccept);
        Lines.Add(L"aaa");
        Lines.Add(L"aaa");
        Lines.Add(L"bbb");
        BOOST_CHECK(3 == Lines.GetCount());
        BOOST_CHECK(0 == Lines.IndexOf(L"aaa"));
        BOOST_CHECK(2 == Lines.IndexOf(L"bbb"));
    }
    Lines.Clear();
    if (1)
    {
        Lines.SetDuplicates(dupIgnore);
        Lines.Add(L"aaa");
        Lines.Add(L"aaa");
        Lines.Add(L"bbb");
        BOOST_CHECK(2 == Lines.GetCount());
        BOOST_CHECK(1 == Lines.IndexOf(L"bbb"));
    }
    Lines.Clear();
    if (1)
    {
        Lines.SetDuplicates(dupError);
        Lines.Add(L"aaa");
        Lines.Add(L"bbb");
        BOOST_CHECK_THROW(Lines.Add(L"aaa"), std::exception);
    }
}

BOOST_FIXTURE_TEST_CASE(test8, base_fixture_t)
{
   std::wstring RootKey = L"Software\\Michael Lukashov\\TestNetBox";
   TRegistryStorage Storage(RootKey);
   Storage.SetAccessMode(smReadWrite);
   BOOST_CHECK(Storage.OpenRootKey(true));
   std::wstring SubKey = L"SubKey1";
   Storage.DeleteSubKey(SubKey);
   BOOST_CHECK(!Storage.KeyExists(SubKey));
   BOOST_CHECK(Storage.OpenSubKey(SubKey, true));
   Storage.SetAccessMode(smReadWrite);
   Storage.Writeint(L"IntVal", 1234);
   // BOOST_TEST_MESSAGE("Storage.GetFailed = " << Storage.GetFailed());
   Storage.CloseSubKey();
   BOOST_CHECK(Storage.KeyExists(SubKey));
   int res = Storage.Readint(L"IntVal", -1);
   BOOST_TEST_MESSAGE("res = " << res);
   BOOST_CHECK(1234 == res);
}

BOOST_AUTO_TEST_SUITE_END()
