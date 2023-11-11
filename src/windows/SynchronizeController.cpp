
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

__removed #pragma package(smart_init)

TSynchronizeController::TSynchronizeController(
  TSynchronizeEvent AOnSynchronize, TSynchronizeInvalidEvent AOnSynchronizeInvalid,
  TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories) noexcept :
  FOnSynchronize(AOnSynchronize),
  FOptions(nullptr),
  FOnSynchronizeThreads(nullptr),
  FSynchronizeMonitor(nullptr),
  FSynchronizeAbort(nullptr),
  FOnSynchronizeInvalid(AOnSynchronizeInvalid),
  FOnTooManyDirectories(AOnTooManyDirectories),
  FSynchronizeLog(nullptr),
  FCopyParam(OBJECT_CLASS_TCopyParamType)
{
  FOnSynchronize = AOnSynchronize;
  FOnSynchronizeInvalid = AOnSynchronizeInvalid;
  FOnTooManyDirectories = AOnTooManyDirectories;
  FSynchronizeMonitor = nullptr;
  FSynchronizeAbort = nullptr;
  FSynchronizeLog = nullptr;
  FOptions = nullptr;
}

TSynchronizeController::~TSynchronizeController() noexcept
{
  DebugAssert(FSynchronizeMonitor == nullptr);
}

void TSynchronizeController::StartStop(TObject * /*Sender*/,
  bool Start, const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
  TSynchronizeOptions * Options,
  TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent /*OnSynchronizeThreads*/,
  TSynchronizeLogEvent OnSynchronizeLog)
{
  if (Start)
  {
    GetConfiguration()->Usage->Inc(L"KeepUpToDates");

    try
    {
      DebugAssert(OnSynchronizeLog != nullptr);
      FSynchronizeLog = OnSynchronizeLog;

      FOptions = Options;
      if (FLAGSET(Params.Options, soSynchronize) &&
          (FOnSynchronize != nullptr))
      {
        FOnSynchronize(this, Params.LocalDirectory,
          Params.RemoteDirectory, CopyParam,
          Params, nullptr, FOptions, /*Full=*/true);
      }

      FCopyParam = CopyParam;
      FCopyParam.IncludeFileMask.SetRoots(Params.LocalDirectory, Params.RemoteDirectory);
      FSynchronizeParams = Params;

      DebugAssert(OnAbort);
      FSynchronizeAbort = OnAbort;

      if (FLAGSET(FSynchronizeParams.Options, soRecurse))
      {
        SynchronizeLog(slScan,
          FMTLOAD(SYNCHRONIZE_SCAN, FSynchronizeParams.LocalDirectory));
      }
      ThrowNotImplemented(256);
      /*
      // FIXME
      FSynchronizeMonitor = new TDiscMonitor(dyn_cast<TComponent>(Sender));
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
      int Directories = FSynchronizeMonitor->Directories->Count;
      FSynchronizeMonitor->Open();
      SynchronizeLog(slStart, FMTLOAD(SYNCHRONIZE_START, Directories));
      */
    }
    catch (...)
    {
      // FIXME SAFE_DESTROY(FSynchronizeMonitor);
      ThrowNotImplemented(257);
      throw;
    }
  }
  else
  {
    FOptions = nullptr;
    // SAFE_DESTROY(FSynchronizeMonitor);
  }
}

void TSynchronizeController::SynchronizeChange(
  TObject * /*Sender*/, const UnicodeString & Directory, bool & SubdirsChanged)
{
  try
  {
    UnicodeString RootLocalDirectory = ::IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory);
    UnicodeString RemoteDirectory = base::UnixIncludeTrailingBackslash(FSynchronizeParams.RemoteDirectory);

    UnicodeString LocalDirectory = ::IncludeTrailingBackslash(Directory);

    DebugAssert(LocalDirectory.SubString(1, RootLocalDirectory.Length()) ==
      RootLocalDirectory);
    RemoteDirectory = RemoteDirectory +
      base::ToUnixPath(LocalDirectory.SubString(RootLocalDirectory.Length() + 1,
        LocalDirectory.Length() - RootLocalDirectory.Length()));

    SynchronizeLog(slChange, FMTLOAD(SYNCHRONIZE_CHANGE,
      ::ExcludeTrailingBackslash(LocalDirectory)));

    if (FOnSynchronize != nullptr)
    {
      TSynchronizeOptions DefaultOptions; // Just as a container for the Files field
      // this is completely wrong as the options structure
      // can contain non-root specific options in future
      TSynchronizeOptions * Options =
        ((LocalDirectory == RootLocalDirectory) ? FOptions : &DefaultOptions);
      TSynchronizeChecklist * Checklist = nullptr;
      FOnSynchronize(this, LocalDirectory, RemoteDirectory, FCopyParam,
        FSynchronizeParams, &Checklist, Options, false);
      if (Checklist != nullptr)
      {
        try__finally
        {
          if (FLAGSET(FSynchronizeParams.Options, soRecurse))
          {
            SubdirsChanged = false;
            DebugAssert(Checklist != nullptr);
            for (int32_t Index = 0; Index < Checklist->Count(); Index++)
            {
              const TChecklistItem * Item = Checklist->GetItem(Index);
              // note that there may be action saDeleteRemote even if nothing has changed
              // so this is sub-optimal
              if (Item->IsDirectory)
              {
                if ((Item->Action == saUploadNew) ||
                    (Item->Action == saDeleteRemote))
                {
                  SubdirsChanged = true;
                  break;
                }
                else
                {
                  DebugFail();
                }
              }
            }
          }
          else
          {
            SubdirsChanged = false;
          }
        },
        __finally__removed
        ({
          delete Checklist;
        }) end_try__finally
      }
    }
  }
  catch(Exception & E)
  {
    SynchronizeAbort(isa<EFatal>(&E));
  }
}

void TSynchronizeController::SynchronizeAbort(bool Close)
{
  if (FSynchronizeMonitor != nullptr)
  {
    // FIXME FSynchronizeMonitor->Close();
    ThrowNotImplemented(258);
  }
  DebugAssert(FSynchronizeAbort);
  FSynchronizeAbort(nullptr, Close);
}

void TSynchronizeController::LogOperation(TSynchronizeOperation Operation,
  const UnicodeString & AFileName)
{
  TSynchronizeLogEntry Entry;
  UnicodeString Message;
  switch (Operation)
  {
    case soDelete:
      Entry = slDelete;
      Message = FMTLOAD(SYNCHRONIZE_DELETED, AFileName);
      break;

    default:
      DebugFail();
      // fallthru

    case soUpload:
      Entry = slUpload;
      Message = FMTLOAD(SYNCHRONIZE_UPLOADED, AFileName);
      break;
  }
  SynchronizeLog(Entry, Message);
}

void TSynchronizeController::SynchronizeLog(TSynchronizeLogEntry Entry,
  const UnicodeString & Message)
{
  if (FSynchronizeLog != nullptr)
  {
    FSynchronizeLog(this, Entry, Message);
  }
}

void TSynchronizeController::SynchronizeFilter(TObject * /*Sender*/,
  const UnicodeString & DirectoryName, bool & Add)
{
  if ((FOptions != nullptr) && (FOptions->Filter != nullptr))
  {
    if (::IncludeTrailingBackslash(::ExtractFilePath(DirectoryName)) ==
          ::IncludeTrailingBackslash(FSynchronizeParams.LocalDirectory))
    {
      int32_t FoundIndex;
      Add = FOptions->Filter->Find(base::ExtractFileName(DirectoryName, /*Unix=*/true), FoundIndex);
    }
  }

  if (Add && !FCopyParam.AllowAnyTransfer()) // optimization
  {
    TFileMasks::TParams MaskParams; // size/time does not matter for directories
    bool Hidden = FLAGSET(FileGetAttrFix(DirectoryName), faHidden);
    // Missing call to GetBaseFileName
    Add = FCopyParam.AllowTransfer(DirectoryName, osLocal, true, MaskParams, Hidden);
  }
}

void TSynchronizeController::SynchronizeInvalid(
  TObject * /*Sender*/, const UnicodeString & Directory, const UnicodeString & ErrorStr)
{
  if (FOnSynchronizeInvalid != nullptr)
  {
    FOnSynchronizeInvalid(this, Directory, ErrorStr);
  }

  SynchronizeAbort(false);
}

void TSynchronizeController::SynchronizeTooManyDirectories(
  TObject * /*Sender*/, int32_t &MaxDirectories)
{
  if (FOnTooManyDirectories != nullptr)
  {
    FOnTooManyDirectories(this, MaxDirectories);
  }
}

void TSynchronizeController::SynchronizeDirectoriesChange(
  TObject * /*Sender*/, int32_t Directories)
{
  SynchronizeLog(slDirChange, FMTLOAD(SYNCHRONIZE_START, Directories));
}

void LogSynchronizeEvent(TTerminal * ATerminal, const UnicodeString & Message)
{
  if (ATerminal != nullptr)
  {
    ATerminal->LogEvent(FORMAT("Keep up to date: %s", Message));
  }
}
