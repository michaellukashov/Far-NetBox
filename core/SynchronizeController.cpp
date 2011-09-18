//---------------------------------------------------------------------------
#include "stdafx.h"
#include <Common.h>
#include <RemoteFiles.h>
#include <Terminal.h>
// #include <DiscMon.hpp>
#include <Exceptions.h>
#include "GUIConfiguration.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "SynchronizeController.h"
//---------------------------------------------------------------------------
TSynchronizeController::TSynchronizeController(
  TSynchronizeEvent AOnSynchronize, TSynchronizeInvalidEvent AOnSynchronizeInvalid,
  TSynchronizeTooManyDirectories AOnTooManyDirectories)
{
  FOnSynchronize = AOnSynchronize;
  FOnSynchronizeInvalid = AOnSynchronizeInvalid;
  FOnTooManyDirectories = AOnTooManyDirectories;
  FSynchronizeMonitor = NULL;
  FSynchronizeAbort = NULL;
  FSynchronizeLog = NULL;
  FOptions = NULL;
}
//---------------------------------------------------------------------------
TSynchronizeController::~TSynchronizeController()
{
  assert(FSynchronizeMonitor == NULL);
}
//---------------------------------------------------------------------------
void TSynchronizeController::StartStop(TObject * Sender,
  bool Start, const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
  TSynchronizeOptions * Options,
  TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
  TSynchronizeLog OnSynchronizeLog)
{
  if (Start)
  {
    try
    {
      assert(OnSynchronizeLog != NULL);
      FSynchronizeLog = OnSynchronizeLog;

      FOptions = Options;
      if (FLAGSET(Params.Options, soSynchronize) &&
          (FOnSynchronize != NULL))
      {
        // FIXME FOnSynchronize(this, Params.LocalDirectory,
          // Params.RemoteDirectory, CopyParam,
          // Params, NULL, FOptions, true);
      }

      FCopyParam = CopyParam;
      FSynchronizeParams = Params;

      assert(OnAbort);
      FSynchronizeAbort = OnAbort;

      if (FLAGSET(FSynchronizeParams.Options, soRecurse))
      {
        SynchronizeLog(slScan,
          FMTLOAD(SYNCHRONIZE_SCAN, FSynchronizeParams.LocalDirectory.c_str()));
      }
      int Directories = 0;
/*
      // FIXME 
      FSynchronizeMonitor = new TDiscMonitor(dynamic_cast<TComponent*>(Sender));
      FSynchronizeMonitor->SubTree = false;
      TMonitorFilters Filters;
      Filters << moFilename << moLastWrite;
      if (FLAGSET(FSynchronizeParams.Options, soRecurse))
      {
        Filters << moDirName;
      }
      FSynchronizeMonitor->Filters = Filters;
      FSynchronizeMonitor->MaxDirectories = 0;
      FSynchronizeMonitor->ChangeDelay = GUIConfiguration->KeepUpToDateChangeDelay;
      FSynchronizeMonitor->OnTooManyDirectories = SynchronizeTooManyDirectories;
      FSynchronizeMonitor->OnDirectoriesChange = SynchronizeDirectoriesChange;
      FSynchronizeMonitor->OnFilter = SynchronizeFilter;
      FSynchronizeMonitor->AddDirectory(FSynchronizeParams.LocalDirectory,
        FLAGSET(FSynchronizeParams.Options, soRecurse));
      FSynchronizeMonitor->OnChange = SynchronizeChange;
      FSynchronizeMonitor->OnInvalid = SynchronizeInvalid;
      FSynchronizeMonitor->OnSynchronize = OnSynchronizeThreads;
      // get count before open to avoid thread issues
      Directories = FSynchronizeMonitor->Directories->GetCount();
      FSynchronizeMonitor->Open();
*/
      SynchronizeLog(slStart, FMTLOAD(SYNCHRONIZE_START, Directories));
    }
    catch(...)
    {
      // FIXME SAFE_DESTROY((TObject *)FSynchronizeMonitor);
      throw;
    }
  }
  else
  {
    FOptions = NULL;
    // SAFE_DESTROY((TObject *)FSynchronizeMonitor);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeChange(
  TObject * /*Sender*/, const std::wstring Directory, bool & SubdirsChanged)
{
  try
  {
    std::wstring RemoteDirectory;
    std::wstring RootLocalDirectory;
    RootLocalDirectory = IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory);
    RemoteDirectory = UnixIncludeTrailingBackslash(FSynchronizeParams.RemoteDirectory);

    std::wstring LocalDirectory = IncludeTrailingBackslash(Directory);

    assert(LocalDirectory.substr(1, RootLocalDirectory.size()) ==
      RootLocalDirectory);
    RemoteDirectory = RemoteDirectory +
      ToUnixPath(LocalDirectory.substr(RootLocalDirectory.size() + 1,
        LocalDirectory.size() - RootLocalDirectory.size()));

    SynchronizeLog(slChange, FMTLOAD(SYNCHRONIZE_CHANGE,
      ExcludeTrailingBackslash(LocalDirectory).c_str()));

    if (FOnSynchronize != NULL)
    {
      // this is completelly wrong as the options structure
      // can contain non-root specific options in future
      TSynchronizeOptions * Options =
        ((LocalDirectory == RootLocalDirectory) ? FOptions : NULL);
      TSynchronizeChecklist * Checklist = NULL;
      // FIXME FOnSynchronize(this, LocalDirectory, RemoteDirectory, FCopyParam,
        // FSynchronizeParams, &Checklist, Options, false);
      if (Checklist != NULL)
      {
        try
        {
          if (FLAGSET(FSynchronizeParams.Options, soRecurse))
          {
            SubdirsChanged = false;
            assert(Checklist != NULL);
            for (int Index = 0; Index < Checklist->GetCount(); Index++)
            {
              const TSynchronizeChecklist::TItem * Item = Checklist->GetItem(Index);
              // note that there may be action saDeleteRemote even if nothing has changed
              // so this is sub-optimal
              if (Item->IsDirectory)
              {
                if ((Item->Action == TSynchronizeChecklist::saUploadNew) ||
                    (Item->Action == TSynchronizeChecklist::saDeleteRemote))
                {
                  SubdirsChanged = true;
                  break;
                }
                else
                {
                  assert(false);
                }
              }
            }
          }
          else
          {
            SubdirsChanged = false;
          }
        }
        catch(...)
        {
          delete Checklist;
        }
      }
    }
  }
  catch (const std::exception & E)
  {
    SynchronizeAbort(dynamic_cast<const EFatal *>(&E) != NULL);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeAbort(bool Close)
{
  if (FSynchronizeMonitor != NULL)
  {
    // FIXME FSynchronizeMonitor->Close();
  }
  assert(FSynchronizeAbort);
  // FIXME FSynchronizeAbort(NULL, Close);
}
//---------------------------------------------------------------------------
void TSynchronizeController::LogOperation(TSynchronizeOperation Operation,
  const std::wstring FileName)
{
  TSynchronizeLogEntry Entry;
  std::wstring Message;
  switch (Operation)
  {
    case soDelete:
      Entry = slDelete;
      Message = FMTLOAD(SYNCHRONIZE_DELETED, FileName.c_str());
      break;

    default:
      assert(false);
      // fallthru

    case soUpload:
      Entry = slUpload;
      Message = FMTLOAD(SYNCHRONIZE_UPLOADED, FileName.c_str());
      break;
  }
  SynchronizeLog(Entry, Message);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeLog(TSynchronizeLogEntry Entry,
  const std::wstring Message)
{
  if (FSynchronizeLog != NULL)
  {
    // FIXME FSynchronizeLog(this, Entry, Message);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeFilter(TObject * /*Sender*/,
  const std::wstring DirectoryName, bool & Add)
{
  if ((FOptions != NULL) && (FOptions->Filter != NULL))
  {
    if (IncludeTrailingBackslash(ExtractFilePath(DirectoryName)) ==
          IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory))
    {
      int FoundIndex;
      Add = FOptions->Filter->Find(ExtractFileName(DirectoryName, true), FoundIndex);
    }
  }
  TFileMasks::TParams MaskParams; // size does not matter for directories
  Add = Add && FCopyParam.AllowTransfer(DirectoryName, osLocal, true, MaskParams);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeInvalid(
  TObject * /*Sender*/, const std::wstring Directory, const std::wstring ErrorStr)
{
  if (FOnSynchronizeInvalid != NULL)
  {
    // FIXME FOnSynchronizeInvalid(this, Directory, ErrorStr);
  }

  SynchronizeAbort(false);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeTooManyDirectories(
  TObject * /*Sender*/, int & MaxDirectories)
{
  if (FOnTooManyDirectories != NULL)
  {
    // FIXME FOnTooManyDirectories(this, MaxDirectories);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeDirectoriesChange(
  TObject * /*Sender*/, int Directories)
{
  SynchronizeLog(slDirChange, FMTLOAD(SYNCHRONIZE_START, Directories));
}
