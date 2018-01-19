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
#define CATCH_CONFIG_CPP11_NO_SHUFFLE
#include <catch/catch.hpp>

//------------------------------------------------------------------------------

#define TEST_CASE_TODO(exp) \
    std::cerr << "TODO: " << #exp << std::endl

#define REQUIRE_EQUAL(exp1, exp2) \
  REQUIRE(exp1 == exp2)

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
    virtual void GetPluginInfoEx(DWORD &Flags,
        TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
        TStrings *PluginConfigStrings, TStrings *CommandPrefixes)
    {
        DEBUG_PRINTF(L"call");
    }
    virtual TCustomFarFileSystem * OpenPluginEx(intptr_t OpenFrom, intptr_t Item)
    {
        DEBUG_PRINTF(L"call");
        return NULL;
    }
    virtual bool ConfigureEx(intptr_t Item)
    {
        DEBUG_PRINTF(L"call");
        return false;
    }
    virtual intptr_t ProcessEditorEventEx(intptr_t Event, void *Param)
    {
        DEBUG_PRINTF(L"call");
        return -1;
    }
    virtual intptr_t ProcessEditorInputEx(const INPUT_RECORD *Rec)
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
  virtual UnicodeString GetMsg(intptr_t Id) const override;
  virtual UnicodeString GetCurrDirectory() const override;
  virtual UnicodeString GetStrVersionNumber() const override;
  virtual bool InputDialog(UnicodeString ACaption,
    UnicodeString APrompt, UnicodeString & Value, UnicodeString HelpKeyword,
    TStrings * History, bool PathInput,
    TInputDialogInitializeEvent OnInitialize, bool Echo) override;
  virtual uintptr_t MoreMessageDialog(UnicodeString Message,
    TStrings * MoreMessages, TQueryType Type, uintptr_t Answers,
      const TMessageParams * Params) override;
};

HINSTANCE TTestGlobalFunctions::GetInstanceHandle() const
{
  HINSTANCE Result = nullptr;
  if (FarPlugin)
  {
    Result = FarPlugin->GetHandle();
  }
  return Result;
}

UnicodeString TTestGlobalFunctions::GetMsg(intptr_t Id) const
{
  return UnicodeString();
}

UnicodeString TTestGlobalFunctions::GetCurrDirectory() const
{
  UnicodeString Path(NB_MAX_PATH, 0);
  int Length = ::GetCurrentDirectory((DWORD)Path.Length(), (wchar_t *)Path.c_str());
  UnicodeString Result = UnicodeString(Path.c_str(), Length);
  return Result;
}

UnicodeString TTestGlobalFunctions::GetStrVersionNumber() const
{
  return UnicodeString();
}

bool TTestGlobalFunctions::InputDialog(UnicodeString ACaption, UnicodeString APrompt,
  UnicodeString & Value, UnicodeString HelpKeyword, TStrings * History, bool PathInput,
  TInputDialogInitializeEvent OnInitialize, bool Echo)
{
  return false;
}

uintptr_t TTestGlobalFunctions::MoreMessageDialog(UnicodeString Message, TStrings * MoreMessages,
  TQueryType Type, uintptr_t Answers,
  const TMessageParams * Params)
{
  return 0;
}

