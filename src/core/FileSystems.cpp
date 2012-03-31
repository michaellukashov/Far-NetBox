//---------------------------------------------------------------------------
#include "stdafx.h"

#include "FileSystems.h"
#include "RemoteFiles.h"
#include "Common.h"
//---------------------------------------------------------------------------
TCustomFileSystem::TCustomFileSystem(TTerminal *ATerminal):
    FTerminal(ATerminal)
{
    assert(FTerminal);
}
//---------------------------------------------------------------------------
TCustomFileSystem::~TCustomFileSystem()
{
}
