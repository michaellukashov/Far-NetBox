//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "CoreMain.h"

#include "Common.h"
#include "Interface.h"
#include "Configuration.h"
#include "PuttyIntf.h"
#include "Cryptography.h"
#ifndef NO_FILEZILLA
#include "FileZillaIntf.h"
#endif
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TConfiguration * Configuration = NULL;
TStoredSessionList * StoredSessions = NULL;
//---------------------------------------------------------------------------
TQueryButtonAlias::TQueryButtonAlias() :
  Button(0)
{
  OnClick = NULL;
}
//---------------------------------------------------------------------------
TQueryParams::TQueryParams(unsigned int AParams, UnicodeString AHelpKeyword)
{
  Params = AParams;
  Aliases = NULL;
  AliasesCount = 0;
  Timer = 0;
  TimerEvent = NULL;
  TimerMessage = L"";
  TimerAnswers = 0;
  Timeout = 0;
  TimeoutAnswer = 0;
  NoBatchAnswers = 0;
  HelpKeyword = AHelpKeyword;
}
//---------------------------------------------------------------------------
bool IsAuthenticationPrompt(TPromptKind Kind)
{
  return
    (Kind == pkUserName) || (Kind == pkPassphrase) || (Kind == pkTIS) ||
    (Kind == pkCryptoCard) || (Kind == pkKeybInteractive) ||
    (Kind == pkPassword) || (Kind == pkNewPassword);
}
//---------------------------------------------------------------------------
void CoreInitialize()
{
  CALLSTACK;
//!CLEANBEGIN
#ifdef _DEBUG
  CallstackTls = TlsAlloc();
#endif
//!CLEANEND
  TRACE("CoreInitialize A");
  Randomize();
  CryptographyInitialize();

  // configuration needs to be created and loaded before putty is initialized,
  // so that random seed path is known
  Configuration = CreateConfiguration();
  TRACEFMT("CoreInitialize [%s]", Configuration->GetVersionStr().c_str());

  TRACE("CoreInitialize E");
  try
  {
    TRACE("CoreInitialize E1");
    Configuration->Load();
    TRACE("CoreInitialize F");
  }
  catch (Exception & E)
  {
    TRACE("CoreInitialize G");
    ShowExtendedException(&E);
  }

  TRACE("CoreInitialize B");
  PuttyInitialize();
  #ifndef NO_FILEZILLA
  TFileZillaIntf::Initialize();
  #endif

  TRACE("CoreInitialize H");
  StoredSessions = new TStoredSessionList();

  try
  {
    TRACE("CoreInitialize I");
    StoredSessions->Load();
  }
  catch (Exception & E)
  {
    TRACE("CoreInitialize J");
    ShowExtendedException(&E);
  }
  TRACE("CoreInitialize K");
}
//---------------------------------------------------------------------------
void CoreFinalize()
{
  try
  {
    // only modified, implicit
    Configuration->Save(false, false);
  }
  catch(Exception & E)
  {
    ShowExtendedException(&E);
  }

  #ifndef NO_FILEZILLA
  TFileZillaIntf::Finalize();
  #endif
  PuttyFinalize();

  delete StoredSessions;
  StoredSessions = NULL;
  delete Configuration;
  Configuration = NULL;

  CryptographyFinalize();
//!CLEANBEGIN
#ifdef _DEBUG
  TlsFree(CallstackTls);
#endif
//!CLEANEND
}
//---------------------------------------------------------------------------
void CoreSetResourceModule(void * ResourceHandle)
{
  #ifndef NO_FILEZILLA
  TFileZillaIntf::SetResourceModule(ResourceHandle);
  #else
  USEDPARAM(ResourceHandle);
  #endif
}
//---------------------------------------------------------------------------
void CoreMaintenanceTask()
{
  DontSaveRandomSeed();
}
//---------------------------------------------------------------------------
