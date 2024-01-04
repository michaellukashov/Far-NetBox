#pragma once

#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <vcl.h>
#include <Sysutils.hpp>

#include "FarPlugin.h"
#include "Cryptography.h"
#include "WinSCPSecurity.h"

#define CATCH_CONFIG_MAIN
//#define CATCH_CONFIG_RUNNER
#define DO_NOT_USE_WMAIN
#define CATCH_CONFIG_CPP11_NO_SHUFFLE
#include <catch/catch.hpp>

#include <TransientFunction.h>

//------------------------------------------------------------------------------

#define TEST_CASE_TODO(exp) \
    std::cerr << "TODO: " << #exp << std::endl

#define REQUIRE_EQUAL(exp1, exp2) \
  REQUIRE(exp1 == exp2)
/*
std::ostringstream& operator<<(std::ostringstream& os, const AnsiString& value)
{
  os << std::string(value.c_str());
  return os;
}

std::ostringstream& operator<<(std::ostringstream& os, const UnicodeString& value)
{
  os << std::string(W2MB(value.c_str()).c_str());
  return os;
}
*/
std::ostream& operator<<(std::ostream& os, const UnicodeString& value)
{
  os << std::string(W2MB(value.c_str()).c_str());
  return os;
}

//------------------------------------------------------------------------------
class TStubFarPlugin : public TCustomFarPlugin
{
public:
    explicit TStubFarPlugin() :
      TCustomFarPlugin(OBJECT_CLASS_TCustomFarPlugin, 0) // GetModuleHandle(0))
    {
      INFO("TStubFarPlugin()");
//      CryptographyInitialize();
    }
    ~TStubFarPlugin()
    {
      INFO("~TStubFarPlugin()");
//      CryptographyFinalize();
    }
protected:
    virtual void GetPluginInfoEx(PLUGIN_FLAGS & Flags,
      TStrings * DiskMenuStrings, TStrings * PluginMenuStrings,
      TStrings * PluginConfigStrings, TStrings * CommandPrefixes) override
    {
        DEBUG_PRINTF(L"call");
    }
    virtual TCustomFarFileSystem * OpenPluginEx(OPENFROM OpenFrom, intptr_t Item) override
    {
        DEBUG_PRINTF(L"call");
        return NULL;
    }
    virtual bool ConfigureEx(const GUID * Guid) override
    {
        DEBUG_PRINTF(L"call");
        return false;
    }
    virtual int32_t ProcessEditorEventEx(const struct ProcessEditorEventInfo * Info) override
    {
        DEBUG_PRINTF(L"call");
        return -1;
    }
    virtual int32_t ProcessEditorInputEx(const INPUT_RECORD * Rec) override
    {
        DEBUG_PRINTF(L"call");
        return -1;
    }
};

//------------------------------------------------------------------------------

static TCustomFarPlugin * CreateStub()
{
  return new TStubFarPlugin();
}

//------------------------------------------------------------------------------

// mocks

class TTestGlobalFunctions : public TGlobals
{
public:
  virtual HINSTANCE GetInstanceHandle() const override;
  virtual UnicodeString GetMsg(int32_t Id) const override;
  virtual UnicodeString GetCurrentDirectory() const override;
  virtual UnicodeString GetStrVersionNumber() const override;
  virtual bool InputDialog(const UnicodeString & ACaption,
    const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword,
    TStrings * History, bool PathInput,
    TInputDialogInitializeEvent && OnInitialize, bool Echo) override;
  virtual uint32_t MoreMessageDialog(const UnicodeString & AMessage,
    TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
    const TMessageParams * Params) override;
};

HINSTANCE TTestGlobalFunctions::GetInstanceHandle() const
{
  HINSTANCE Result = nullptr;
  if (FarPlugin)
  {
    Result = FarPlugin->GetPluginHandle();
  }
  return Result;
}

UnicodeString TTestGlobalFunctions::GetMsg(int32_t Id) const
{
  const HINSTANCE Instance = GetInstanceHandle();
  UnicodeString Result = ::LoadStrFrom(Instance, Id);
  return Result;
}

UnicodeString TTestGlobalFunctions::GetCurrentDirectory() const
{
  UnicodeString Path(nb::NB_MAX_PATH, 0);
  int Length = ::GetCurrentDirectoryW((DWORD)Path.Length(), (wchar_t *)Path.c_str());
  UnicodeString Result = UnicodeString(Path.c_str(), Length);
  return Result;
}

UnicodeString TTestGlobalFunctions::GetStrVersionNumber() const
{
  return UnicodeString();
}

bool TTestGlobalFunctions::InputDialog(const UnicodeString & ACaption,
  const UnicodeString & APrompt, UnicodeString & Value, const UnicodeString & HelpKeyword,
  TStrings * History, bool PathInput,
  TInputDialogInitializeEvent && OnInitialize, bool Echo)
{
  return false;
}

uint32_t TTestGlobalFunctions::MoreMessageDialog(const UnicodeString & AMessage,
  TStrings * MoreMessages, TQueryType Type, uint32_t Answers,
  const TMessageParams * Params)
{
  return 0;
}

