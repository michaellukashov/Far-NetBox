#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FileSystems.h"
#include "RemoteFiles.h"

TCustomFileSystem::TCustomFileSystem(TTerminal * ATerminal) :
  FTerminal(ATerminal)
{
  assert(FTerminal);
}

TCustomFileSystem::~TCustomFileSystem()
{
#ifdef USE_DLMALLOC
  dlmalloc_trim(0); // 64 * 1024);
#endif
}

NB_IMPLEMENT_CLASS(TCustomFileSystem, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TSinkFileParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TFileTransferData, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TOpenRemoteFileParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TOverwriteFileParams, NB_GET_CLASS_INFO(TObject), nullptr)
NB_IMPLEMENT_CLASS(TClipboardHandler, NB_GET_CLASS_INFO(TObject), nullptr)

