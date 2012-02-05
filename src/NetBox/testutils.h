#pragma once
#include "stdafx.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "FarPlugin.h"
#include "Cryptography.h"
#include "WinSCPSecurity.h"

//------------------------------------------------------------------------------

#define TEST_CASE_TODO(exp) \
    std::cerr << "TODO: " << #exp << std::endl

//------------------------------------------------------------------------------
class TStubFarPlugin : public TCustomFarPlugin
{
public:
    explicit TStubFarPlugin() :
        TCustomFarPlugin(GetModuleHandle(0))
    {
        CryptographyInitialize();
    }
    ~TStubFarPlugin()
    {
        CryptographyFinalize();
    }
protected:
    virtual void GetPluginInfoEx(PLUGIN_FLAGS &Flags,
        nb::TStrings *DiskMenuStrings, nb::TStrings *PluginMenuStrings,
        nb::TStrings *PluginConfigStrings, nb::TStrings *CommandPrefixes)
    {
        DEBUG_PRINTF(L"call");
    }
    virtual TCustomFarFileSystem *OpenPluginEx(int OpenFrom, LONG_PTR Item)
    {
        DEBUG_PRINTF(L"call");
        return NULL;
    }
    virtual bool ConfigureEx(const struct ConfigureInfo *Info)
    {
        DEBUG_PRINTF(L"call");
        return false;
    }
    virtual int ProcessEditorEventEx(const struct ProcessEditorEventInfo *Info)
    {
        DEBUG_PRINTF(L"call");
        return -1;
    }
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec)
    {
        DEBUG_PRINTF(L"call");
        return -1;
    }
};

//------------------------------------------------------------------------------

static TCustomFarPlugin *CreateStub()
{
    return new TStubFarPlugin();
}

//------------------------------------------------------------------------------

