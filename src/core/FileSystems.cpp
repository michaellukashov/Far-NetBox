//---------------------------------------------------------------------------
#include "stdafx.h"

#include "FileSystems.h"
#include "RemoteFiles.h"
#include "Common.h"
//---------------------------------------------------------------------------
TCustomFileTCustomFileSystem(TTerminal *ATerminal):
    FTerminal(ATerminal)
{
    assert(FTerminal);
}
//---------------------------------------------------------------------------
TCustomFile~TCustomFileSystem()
{
}
