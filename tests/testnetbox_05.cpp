//------------------------------------------------------------------------------
// testnetbox_05.cpp
//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <Classes.hpp>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <Common.h>
#include <FileBuffer.h>
#include <Terminal.h>

#include "TestTexts.h"
#include "TextsCore.h"
#include "FarPlugin.h"
#include "WinSCPPlugin.h"
#include "GUITools.h"
#include "GUIConfiguration.h"
#include "CoreMain.h"

#include "testutils.h"
#include <gmock/gmock.h>

/*******************************************************************************
            test suite
*******************************************************************************/
//#if 0

class TMockTerminal : public TTerminal
{
public:
  TMockTerminal() : TTerminal()
  {
  }
  MOCK_CONST_METHOD0(GetSessionData, TSessionData*());
};

class TMockWinSCPPlugin : public TWinSCPPlugin
{
public:
  TMockWinSCPPlugin(HINSTANCE HInst) : TWinSCPPlugin(HInst)
  {
    ON_CALL(*this, GetModuleName())
      .WillByDefault(testing::Return("testnetbox_05"));
    ON_CALL(*this, GetMsg(testing::_))
      .WillByDefault(testing::Invoke(this, &TMockWinSCPPlugin::GetMsgFake));
//      .WillByDefault(testing::Return(""));
    ON_CALL(*this, Initialize())
      .WillByDefault(testing::Invoke(this, &TMockWinSCPPlugin::InitializeFake));
    ON_CALL(*this, Finalize())
      .WillByDefault(testing::Invoke(this, &TMockWinSCPPlugin::FinalizeFake));
  }
  MOCK_CONST_METHOD0(GetModuleName, UnicodeString());
  MOCK_CONST_METHOD1(GetMsg, UnicodeString(intptr_t));
  MOCK_METHOD0(Initialize, void(void));
  MOCK_METHOD0(Finalize, void(void));

  UnicodeString GetMsgFake(intptr_t MsgId) const
  {
    DEBUG_PRINTF(L"GetMsgFake 1.1: MsgId: %d", MsgId);
    return "";
  }
  void InitializeFake()
  {
    DEBUG_PRINTF(L"InitializeFake called");
  }
  void FinalizeFake()
  {
    DEBUG_PRINTF(L"FinalizeFake called");
  }
};

TEST_CASE("testRemoteFileSetListingStr", "netbox")
{
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0");
  TGlobalsIntfInitializer<TTestGlobalFunctions> GlobalsIntfInitializer;
  testing::NiceMock<TMockWinSCPPlugin> StrictMockWinSCPPlugin(nullptr);
  StrictMockWinSCPPlugin.Initialize();
  FarPlugin = &StrictMockWinSCPPlugin;
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0.1");
  testing::NiceMock<TMockTerminal> MockTerminal;
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0.2");

  DEBUG_PRINTF(L"testRemoteFileSetListingStr 1");
  TSessionData SessionData("Test");
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 2");
  DEBUG_PRINTF(L"SessionData: %p", (void*)&SessionData);
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 3");
  ON_CALL(MockTerminal, GetSessionData())
    .WillByDefault(testing::Return(&SessionData));
//  MockTerminal.Init(&SessionData, GetConfiguration());
  DEBUG_PRINTF(L"MockTerminal.SessionData: %p", (void*)MockTerminal.GetSessionData());
  {
    TRemoteFile RemoteFile1(nullptr);
    DEBUG_PRINTF(L"testRemoteFileSetListingStr 4");
    RemoteFile1.SetTerminal(&MockTerminal);
    DEBUG_PRINTF(L"testRemoteFileSetListingStr 5");
    UnicodeString Str1("lrwxrwxrwx    1 root     root             7 2017-08-03 06:05:01 +0300 TZ -> /tmp/TZ");
    RemoteFile1.SetListingStr(Str1);
    INFO("FileName1: " << RemoteFile1.GetFileName());
    CHECK(RemoteFile1.GetFileName() == "TZ");
  }

  {
    TRemoteFile RemoteFile2(nullptr);
    RemoteFile2.SetTerminal(&MockTerminal);
    UnicodeString Str2("lrwxrwxrwx    1 root     root             7 2017-07-27 10:44:52.405136754 +0300 TZ2 -> /tmp/TZ2");
    RemoteFile2.SetListingStr(Str2);
    INFO("FileName2: " << RemoteFile2.GetFileName());
    CHECK(RemoteFile2.GetFileName() == "TZ2");
  }
  StrictMockWinSCPPlugin.Finalize();
}

//#endif // if 0
