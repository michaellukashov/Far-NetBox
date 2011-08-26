//---------------------------------------------------------------------------
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
TConfiguration * Configuration = NULL;
TStoredSessionList * StoredSessions = NULL;
//---------------------------------------------------------------------------
TQueryButtonAlias::TQueryButtonAlias()
{
  OnClick = NULL;
}
//---------------------------------------------------------------------------
TQueryParams::TQueryParams(unsigned int AParams, std::wstring AHelpKeyword)
{
  Params = AParams;
  Aliases = NULL;
  AliasesCount = 0;
  Timer = 0;
  TimerEvent = NULL;
  TimerMessage = "";
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
  Randomize();
  CryptographyInitialize();

  // configuration needs to be created and loaded before putty is initialized,
  // so that random seed path is known
  Configuration = CreateConfiguration();

  try
  {
    Configuration->Load();
  }
  catch (std::exception & E)
  {
    ShowExtendedException(&E);
  }

  PuttyInitialize();
  #ifndef NO_FILEZILLA
  TFileZillaIntf::Initialize();
  #endif

  StoredSessions = new TStoredSessionList();

  try
  {
    StoredSessions->Load();
  }
  catch (std::exception & E)
  {
    ShowExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
void CoreFinalize()
{
  try
  {
    // only modified, implicit
    Configuration->Save(false, false);
  }
  catch(std::exception & E)
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
