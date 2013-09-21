//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <RemoteFiles.h>
#include <Terminal.h>
#include <DiscMon.hpp>
#include <Exceptions.h>
#include "GUIConfiguration.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "SynchronizeController.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TSynchronizeController::TSynchronizeController(
  TSynchronizeEvent AOnSynchronize, TSynchronizeInvalidEvent AOnSynchronizeInvalid,
  TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories)
{
  FOnSynchronize = AOnSynchronize;
  FSynchronizeParams.Params = 0;
  FSynchronizeParams.Options = 0;
  FOnSynchronizeInvalid = AOnSynchronizeInvalid;
  FOnTooManyDirectories = AOnTooManyDirectories;
  FSynchronizeMonitor = nullptr;
  FSynchronizeAbort = nullptr;
  FSynchronizeLog = nullptr;
  FOptions = nullptr;
  FOnSynchronizeThreads = nullptr;
}
//---------------------------------------------------------------------------
TSynchronizeController::~TSynchronizeController()
{
  assert(FSynchronizeMonitor == nullptr);
}
//---------------------------------------------------------------------------
void TSynchronizeController::StartStop(TObject * /*Sender*/,
  bool Start, const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
  TSynchronizeOptions * Options,
  TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
  TSynchronizeLogEvent OnSynchronizeLog)
{
  if (Start)
  {
    // Configuration->GetUsage()->Inc(L"KeepUpToDates");

    try
    {
      assert(OnSynchronizeLog != nullptr);
      FSynchronizeLog = OnSynchronizeLog;

      FOptions = Options;
      if (FLAGSET(Params.Options, soSynchronize) &&
          (FOnSynchronize != nullptr))
      {
        FOnSynchronize(this, Params.LocalDirectory,
          Params.RemoteDirectory, CopyParam,
          Params, nullptr, FOptions, true);
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
      Classes::Error(SNotImplemented, 256);
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
      FSynchronizeMonitor->ChangeDelay = GetGUIConfiguration()->GetKeepUpToDateChangeDelay();
      FSynchronizeMonitor->OnTooManyDirectories = SynchronizeTooManyDirectories;
      FSynchronizeMonitor->OnDirectoriesChange = SynchronizeDirectoriesChange;
      FSynchronizeMonitor->OnFilter = SynchronizeFilter;
      FSynchronizeMonitor->AddDirectory(FSynchronizeParams.LocalDirectory,
        FLAGSET(FSynchronizeParams.Options, soRecurse));
      FSynchronizeMonitor->OnChange = SynchronizeChange;
      FSynchronizeMonitor->OnInvalid = SynchronizeInvalid;
      FSynchronizeMonitor->OnSynchronize = OnSynchronizeThreads;
      // get count before open to avoid thread issues
      int Directories = FSynchronizeMonitor->Directories->Count;
      FSynchronizeMonitor->Open();
      SynchronizeLog(slStart, FMTLOAD(SYNCHRONIZE_START, Directories));
      */
    }
    catch (...)
    {
      // FIXME SAFE_DESTROY(FSynchronizeMonitor);
      Classes::Error(SNotImplemented, 257);
      throw;
    }
  }
  else
  {
    FOptions = nullptr;
    // SAFE_DESTROY(FSynchronizeMonitor);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeChange(
  TObject * /*Sender*/, const UnicodeString & Directory, bool & SubdirsChanged)
{
  try
  {
    UnicodeString RemoteDirectory;
    UnicodeString RootLocalDirectory;
    RootLocalDirectory = IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory);
    RemoteDirectory = UnixIncludeTrailingBackslash(FSynchronizeParams.RemoteDirectory);

    UnicodeString LocalDirectory = IncludeTrailingBackslash(Directory);

    assert(LocalDirectory.SubString(1, RootLocalDirectory.Length()) ==
      RootLocalDirectory);
    RemoteDirectory = RemoteDirectory +
      ToUnixPath(LocalDirectory.SubString(RootLocalDirectory.Length() + 1,
        LocalDirectory.Length() - RootLocalDirectory.Length()));

    SynchronizeLog(slChange, FMTLOAD(SYNCHRONIZE_CHANGE,
      ExcludeTrailingBackslash(LocalDirectory).c_str()));

    if (FOnSynchronize != nullptr)
    {
      // this is completely wrong as the options structure
      // can contain non-root specific options in future
      TSynchronizeOptions * Options =
        ((LocalDirectory == RootLocalDirectory) ? FOptions : nullptr);
      TSynchronizeChecklist * Checklist = nullptr;
      FOnSynchronize(this, LocalDirectory, RemoteDirectory, FCopyParam,
        FSynchronizeParams, &Checklist, Options, false);
      if (Checklist != nullptr)
      {
        std::auto_ptr<TSynchronizeChecklist> ChecklistPtr(Checklist);
        (void)ChecklistPtr;
        if (FLAGSET(FSynchronizeParams.Options, soRecurse))
        {
          SubdirsChanged = false;
          assert(Checklist != nullptr);
          for (intptr_t Index = 0; Index < Checklist->GetCount(); ++Index)
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
  catch(Exception & E)
  {
    SynchronizeAbort(dynamic_cast<EFatal*>(&E) != nullptr);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeAbort(bool Close)
{
  if (FSynchronizeMonitor != nullptr)
  {
    // FIXME FSynchronizeMonitor->Close();
    Classes::Error(SNotImplemented, 258);
  }
  assert(FSynchronizeAbort);
  FSynchronizeAbort(nullptr, Close);
}
//---------------------------------------------------------------------------
void TSynchronizeController::LogOperation(TSynchronizeOperation Operation,
  const UnicodeString & FileName)
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
  const UnicodeString & Message)
{
  if (FSynchronizeLog != nullptr)
  {
    FSynchronizeLog(this, Entry, Message);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeFilter(TObject * /*Sender*/,
  const UnicodeString & DirectoryName, bool & Add)
{
  if ((FOptions != nullptr) && (FOptions->Filter != nullptr))
  {
    if (IncludeTrailingBackslash(ExtractFilePath(DirectoryName)) ==
          IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory))
    {
      intptr_t FoundIndex;
      Add = FOptions->Filter->Find(ExtractFileName(DirectoryName, true), FoundIndex);
    }
  }
  TFileMasks::TParams MaskParams; // size/time does not matter for directories
  Add = Add && FCopyParam.AllowTransfer(DirectoryName, osLocal, true, MaskParams);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeInvalid(
  TObject * /*Sender*/, const UnicodeString & Directory, const UnicodeString & ErrorStr)
{
  if (FOnSynchronizeInvalid != nullptr)
  {
    FOnSynchronizeInvalid(this, Directory, ErrorStr);
  }

  SynchronizeAbort(false);
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeTooManyDirectories(
  TObject * /*Sender*/, intptr_t & MaxDirectories)
{
  if (FOnTooManyDirectories != nullptr)
  {
    FOnTooManyDirectories(this, MaxDirectories);
  }
}
//---------------------------------------------------------------------------
void TSynchronizeController::SynchronizeDirectoriesChange(
  TObject * /*Sender*/, intptr_t Directories)
{
  SynchronizeLog(slDirChange, FMTLOAD(SYNCHRONIZE_START, Directories));
}
