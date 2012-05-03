//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include <Common.h>
#include <RemoteFiles.h>
#include <Terminal.h>
#include <Exceptions.h>
#include "GUIConfiguration.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "SynchronizeController.h"
//---------------------------------------------------------------------------
TSynchronizeController::TSynchronizeController(
  const synchronize_slot_type & AOnSynchronize, const synchronizeinvalid_slot_type & AOnSynchronizeInvalid,
  const synchronizetoomanydirectories_slot_type & AOnTooManyDirectories)
{
  FOnSynchronize.connect(AOnSynchronize);
  FOnSynchronizeInvalid.connect(AOnSynchronizeInvalid);
  FOnTooManyDirectories.connect(AOnTooManyDirectories);
  FSynchronizeMonitor = NULL;
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
                                       const synchronizeabort_slot_type & OnAbort, const synchronizethreads_slot_type & OnSynchronizeThreads,
                                       const synchronizelog_slot_type & OnSynchronizeLog)
{
  if (Start)
  {
    try
    {
      FSynchronizeLog.connect(OnSynchronizeLog);
      assert(!FSynchronizeLog.empty());

      FOptions = Options;
      if (FLAGSET(Params.Options, soSynchronize) &&
          (!FOnSynchronize.empty()))
      {
        FOnSynchronize(this, Params.LocalDirectory,
                       Params.RemoteDirectory, CopyParam,
                       Params, NULL, FOptions, true);
      }

      FCopyParam = CopyParam;
      FSynchronizeParams = Params;

      // assert(OnAbort);
      FSynchronizeAbort.connect(OnAbort);

      if (FLAGSET(FSynchronizeParams.Options, soRecurse))
      {
        SynchronizeLog(slScan,
                       FMTLOAD(SYNCHRONIZE_SCAN, FSynchronizeParams.LocalDirectory.c_str()));
      }
      int Directories = 0;
      Error(SNotImplemented, 256);
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
      Error(SNotImplemented, 257);
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
  TObject * /*Sender*/, const UnicodeString Directory, bool & SubdirsChanged)
{
  try
  {
    UnicodeString RemoteDirectory;
    UnicodeString RootLocalDirectory;
    RootLocalDirectory = IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory);
    RemoteDirectory = UnixIncludeTrailingBackslash(FSynchronizeParams.RemoteDirectory);

    UnicodeString LocalDirectory = IncludeTrailingBackslash(Directory);

    assert(LocalDirectory.SubString(0, RootLocalDirectory.Length()) ==
           RootLocalDirectory);
    RemoteDirectory = RemoteDirectory +
                      ToUnixPath(LocalDirectory.SubString(RootLocalDirectory.Length() + 1,
                                 LocalDirectory.Length() - RootLocalDirectory.Length()));

    SynchronizeLog(slChange, FMTLOAD(SYNCHRONIZE_CHANGE,
                                     ExcludeTrailingBackslash(LocalDirectory).c_str()));

    if (!FOnSynchronize.empty())
    {
      // this is completelly wrong as the options structure
      // can contain non-root specific options in future
      TSynchronizeOptions * Options =
        ((LocalDirectory == RootLocalDirectory) ? FOptions : NULL);
      TSynchronizeChecklist * Checklist = NULL;
      FOnSynchronize(this, LocalDirectory, RemoteDirectory, FCopyParam,
                     FSynchronizeParams, &Checklist, Options, false);
      if (Checklist != NULL)
      {
        {
          BOOST_SCOPE_EXIT ( (&Checklist) )
          {
            delete Checklist;
          } BOOST_SCOPE_EXIT_END
          if (FLAGSET(FSynchronizeParams.Options, soRecurse))
          {
            SubdirsChanged = false;
            assert(Checklist != NULL);
            for (size_t Index = 0; Index < Checklist->GetCount(); Index++)
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
    Error(SNotImplemented, 258);
  }
  assert(!FSynchronizeAbort.empty());
  FSynchronizeAbort(NULL, Close);
}
//---------------------------------------------------------------------------
void TSynchronizeController::LogOperation(TSynchronizeOperation Operation,
    const UnicodeString FileName)
{
  TSynchronizeLogEntry Entry;
  UnicodeString Message;
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
    const UnicodeString Message)
{
  if (!FSynchronizeLog.empty())
  {
    FSynchronizeLog(this, Entry, Message);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeFilter(TObject * /*Sender*/,
    const UnicodeString DirectoryName, bool & Add)
{
  if ((FOptions != NULL) && (FOptions->Filter != NULL))
  {
    if (IncludeTrailingBackslash(ExtractFilePath(DirectoryName)) ==
        IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory))
    {
      size_t FoundIndex;
      Add = FOptions->Filter->Find(ExtractFileName(DirectoryName, true), FoundIndex);
    }
  }
  TFileMasks::TParams MaskParams; // size does not matter for directories
  Add = Add && FCopyParam.AllowTransfer(DirectoryName, osLocal, true, MaskParams);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeInvalid(
  TObject * /*Sender*/, const UnicodeString Directory, const UnicodeString ErrorStr)
{
  if (!FOnSynchronizeInvalid.empty())
  {
    FOnSynchronizeInvalid(this, Directory, ErrorStr);
  }

  SynchronizeAbort(false);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeTooManyDirectories(
  TObject * /*Sender*/, int & MaxDirectories)
{
  if (!FOnTooManyDirectories.empty())
  {
    FOnTooManyDirectories(this, MaxDirectories);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeDirectoriesChange(
  TObject * /*Sender*/, int Directories)
{
  SynchronizeLog(slDirChange, FMTLOAD(SYNCHRONIZE_START, Directories));
}
