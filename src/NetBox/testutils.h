#pragma once

#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <Classes.hpp>
#include <Sysutils.hpp>

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
    virtual void __fastcall GetPluginInfoEx(long unsigned &Flags,
        TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
        TStrings *PluginConfigStrings, TStrings *CommandPrefixes)
    {
        DEBUG_PRINTF(L"call");
    }
    virtual TCustomFarFileSystem * __fastcall OpenPluginEx(int OpenFrom, intptr_t Item)
    {
        DEBUG_PRINTF(L"call");
        return NULL;
    }
    virtual bool __fastcall ConfigureEx(int Item)
    {
        DEBUG_PRINTF(L"call");
        return false;
    }
    virtual int __fastcall ProcessEditorEventEx(int Event, void *Param)
    {
        DEBUG_PRINTF(L"call");
        return -1;
    }
    virtual int __fastcall ProcessEditorInputEx(const INPUT_RECORD *Rec)
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

