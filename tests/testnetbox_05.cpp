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
#include <type_traits>

#include <Common.h>
#include <FileBuffer.h>
#include <Terminal.h>

#include "TestTexts.h"
#include "TextsCore.h"
#include "FarPlugin.h"
#include "WinSCPPlugin.h"
#include "GUITools.h"
#include "GUIConfiguration.h"
#include "FarConfiguration.h"
#include "CoreMain.h"

#include "testutils.h"
#include <gmock/gmock.h>

/*******************************************************************************
            test suite
*******************************************************************************/

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
  TMockWinSCPPlugin() : TMockWinSCPPlugin(nullptr) {}
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

intptr_t WINAPI SettingsControl(
  HANDLE hHandle,
  enum FAR_SETTINGS_CONTROL_COMMANDS Command,
  intptr_t Param1,
  void* Param2)
{
  DEBUG_PRINTF(L"SettingsControl");
  return 0;
}

const wchar_t* WINAPI GetMsg(
  const UUID* PluginId,
  intptr_t MsgId)
{
  DEBUG_PRINTF(L"GetMsg");
  return nullptr;
}

intptr_t WINAPI Message(
  const UUID* PluginId,
  const UUID* Id,
  FARMESSAGEFLAGS Flags,
  const wchar_t *HelpTopic,
  const wchar_t * const *Items,
  size_t ItemsNumber,
  intptr_t ButtonsNumber)
{
  DEBUG_PRINTF(L"Message");
  return 0;
}

class base_fixture_t
{
public:
  base_fixture_t()
  {
    INFO("base_fixture_t ctor");
    MockWinSCPPlugin.Initialize();
    PluginStartupInfo Info{};
    Info.StructSize = sizeof(Info);
    Info.SettingsControl = SettingsControl;
    Info.GetMsg = GetMsg;
    Info.Message = Message;
    FarStandardFunctions FSF{};
    FSF.StructSize = sizeof(FSF);
    Info.FSF = &FSF;

    MockWinSCPPlugin.SetStartupInfo(&Info);
    FarPlugin = &MockWinSCPPlugin;
    // DEBUG_PRINTF(L"MockTerminal.SessionData: %p", (void*)MockTerminal.GetSessionData());
  }

  virtual ~base_fixture_t()
  {
    INFO("base_fixture_t dtor");
    MockWinSCPPlugin.Finalize();
  }
public:
protected:
private:
  testing::NiceMock<TMockWinSCPPlugin> MockWinSCPPlugin;
};

TEST_CASE_METHOD(base_fixture_t, "testRemoteFileSetListingStr", "netbox")
{
  DEBUG_PRINTF(L"testRemoteFileSetListingStr 0");
  testing::NiceMock<TMockTerminal> MockTerminal;
  TSessionData SessionData("Test");
  // MockTerminal.Init(&SessionData, GetConfiguration());
  DEBUG_PRINTF(L"SessionData: %p", (void*)&SessionData);
  ON_CALL(MockTerminal, GetSessionData())
    .WillByDefault(testing::Return(&SessionData));
  // DEBUG_PRINTF(L"MockTerminal.SessionData: %p", (void*)MockTerminal.GetSessionData());
  CHECK(&SessionData == MockTerminal.GetSessionData());
  SECTION("RemoteFile01")
  {
    TRemoteFile RemoteFile1(nullptr);
    RemoteFile1.SetTerminal(&MockTerminal);
    UnicodeString Str1("lrwxrwxrwx    1 root     root             7 2017-08-03 06:05:01 +0300 TZ -> /tmp/TZ");
    RemoteFile1.SetListingStr(Str1);
    INFO("FileName1: " << RemoteFile1.GetFileName());
    CHECK(RemoteFile1.GetFileName() == "TZ");
  }
  SECTION("RemoteFile02")
  {
    TRemoteFile RemoteFile2(nullptr);
    RemoteFile2.SetTerminal(&MockTerminal);
    UnicodeString Str2("lrwxrwxrwx    1 root     root             7 2017-07-27 10:44:52.405136754 +0300 TZ2 -> /tmp/TZ2");
    RemoteFile2.SetListingStr(Str2);
    INFO("FileName2: " << RemoteFile2.GetFileName());
    CHECK(RemoteFile2.GetFileName() == "TZ2");
  }
}

TEST_CASE_METHOD(base_fixture_t, "dyncast01", "netbox")
{
  // TGUICopyParamType CopyParam(GetGUIConfiguration()->GetDefaultCopyParam());
  TConfiguration * Conf = GetConfiguration();
  CHECK(Conf);
  // WARN((int)Conf->FKind);
  // WARN((int)OBJECT_CLASS_TFarConfiguration);
  // WARN((int)OBJECT_CLASS_TStoredSessionList);
  // NB_DEFINE_CLASS_ID2(TFarConfiguration2);
  // __pragma(message ("TFarConfiguration2: " GSL_STRINGIFY(OBJECT_CLASS_TFarConfiguration2)))
  bool is; // = Conf->FKind == OBJECT_CLASS_TFarConfiguration;
  {
    TGUIConfiguration * GUIConfiguration = cast_to<TGUIConfiguration>(Conf);
    CHECK(GUIConfiguration);
  }
  // TFarConfiguration : public TGUIConfiguration
  static_assert(std::is_base_of<TConfiguration, TGUIConfiguration>::value, "check");
  static_assert(std::is_base_of<TGUIConfiguration, TFarConfiguration>::value, "check");
  {
    is = rtti::isa<TConfiguration>(Conf);
    CHECK(is);
    // is = rtti::isa<TFarConfiguration, TGUIConfiguration, TConfiguration>(Configuration);
    is = rtti::isa<TGUIConfiguration>(Conf);
    CHECK(is);
    is = rtti::isa<TFarConfiguration>(Conf);
    CHECK(is);
    // is = Conf->FKind == OBJECT_CLASS_TFarConfiguration;
    // CHECK(is);
    TGUIConfiguration * GUIConfiguration = rtti::dyn_cast_or_null<TGUIConfiguration>(Conf);
    CHECK(GUIConfiguration);
  }
  TGUICopyParamType GUICopyParamType;
  TGUICopyParamType & DefaultCopyParam = GUICopyParamType; // GUIConfiguration1->GetDefaultCopyParam();
  TGUICopyParamType CopyParam(DefaultCopyParam);
  CHECK(CopyParam.GetQueue() == false);
}
