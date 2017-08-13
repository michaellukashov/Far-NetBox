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

#include "testutils.h"
#include <gmock/gmock.h>

/*******************************************************************************
            test suite
*******************************************************************************/

class TMockTerminal : public TTerminal
{
public:
  MOCK_CONST_METHOD0(GetSessionData, TSessionData *());
  MOCK_METHOD0(GetSessionData, TSessionData *());
};

class TMockWinSCPPlugin : public TWinSCPPlugin
{
public:
  TMockWinSCPPlugin(HINSTANCE HInst) : TWinSCPPlugin(HInst)
  {
  }
  MOCK_CONST_METHOD0(GetModuleName, UnicodeString());
};

TEST_CASE("testRemoteFileSetListingStr", "netbox")
{
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0");
  testing::StrictMock<TMockTerminal> StrictMockTerminal;
  testing::StrictMock<TMockWinSCPPlugin> StrictMockWinSCPPlugin(nullptr);
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0.1");
  FarPlugin = &StrictMockWinSCPPlugin;
  ON_CALL(StrictMockWinSCPPlugin, GetModuleName())
    .WillByDefault(testing::Return("testnetbox_05"));
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0.2");

  DEBUG_PRINTF(L"testRemoteFileSetListingStr 1");
  TSessionData SessionData("Test");
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 2");
//  TConfiguration Configuration;
  ON_CALL(StrictMockTerminal, GetSessionData())
    .WillByDefault(testing::Return(&SessionData));
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 3");
  //TMockTerminal MockTerminal;
  //StrictMockTerminal.Init(&SessionData, &Configuration);
  {
    TRemoteFile RemoteFile1(nullptr);
    DEBUG_PRINTF(L"testRemoteFileSetListingStr 4");
    RemoteFile1.SetTerminal(&StrictMockTerminal);
    DEBUG_PRINTF(L"testRemoteFileSetListingStr 5");
    UnicodeString Str1("lrwxrwxrwx    1 root     root             7 2017-08-03 06:05:01 +0300 TZ -> /tmp/TZ");
    RemoteFile1.SetListingStr(Str1);
    INFO("FileName1: " << RemoteFile1.GetFileName());
    CHECK(RemoteFile1.GetFileName() == "TZ");
  }

  {
    TRemoteFile RemoteFile2(nullptr);
    RemoteFile2.SetTerminal(&StrictMockTerminal);
    UnicodeString Str2("lrwxrwxrwx    1 root     root             7 2017-07-27 10:44:52.405136754 +0300 TZ2 -> /tmp/TZ2");
    RemoteFile2.SetListingStr(Str2);
    INFO("FileName2: " << RemoteFile2.GetFileName());
    CHECK(RemoteFile2.GetFileName() == "TZ2");
  }
}

