
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <RemoteFiles.h>
#include <Terminal.h>
#include <Queue.h>
#include <Exceptions.h>
#include "GUIConfiguration.h"
#include "CoreMain.h"
#include "TextsCore.h"
#include "SynchronizeController.h"

#if defined(__BORLANDC__)
#pragma package(smart_init)
#endif // defined(__BORLANDC__)


// ============================================================================
// TSyncPoller — FindFirstChangeNotification-based directory watcher
// ============================================================================

class TSyncPoller final : public TSimpleThread
{
  NB_DISABLE_COPY(TSyncPoller)
public:
  TSyncPoller() = delete;
  explicit TSyncPoller(const UnicodeString & Directory, bool Recursive,
    TSynchronizeThreadsEvent OnSynchronizeThreads,
    TThreadMethod OnChange, TSynchronizeInvalidEvent OnInvalid) noexcept;
  virtual ~TSyncPoller() noexcept override;

  void StartPoller();
  void StopPoller();

protected:
  virtual void Execute() override;

private:
  UnicodeString FDirectory;
  bool FRecursive{false};
  TSynchronizeThreadsEvent FOnSynchronizeThreads;
  TThreadMethod FOnChange;
  TSynchronizeInvalidEvent FOnInvalid;
  HANDLE FChangeHandle{INVALID_HANDLE_VALUE};
  HANDLE FStopEvent{INVALID_HANDLE_VALUE};
  bool FStarted{false};

  bool OpenNotification();
  void CloseNotification();
};

TSyncPoller::TSyncPoller(const UnicodeString & Directory, bool Recursive,
  TSynchronizeThreadsEvent OnSynchronizeThreads,
  TThreadMethod OnChange, TSynchronizeInvalidEvent OnInvalid) noexcept :
  TSimpleThread(OBJECT_CLASS_TSyncPoller),
  FDirectory(Directory),
  FRecursive(Recursive),
  FOnSynchronizeThreads(OnSynchronizeThreads),
  FOnChange(OnChange),
  FOnInvalid(OnInvalid)
{
}

TSyncPoller::~TSyncPoller() noexcept
{
  StopPoller();
}

void TSyncPoller::StartPoller()
{
  DebugAssert(!FStarted);
  FStopEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
  TSimpleThread::InitSimpleThread("NetBox Sync Poller");
  Start();
  FStarted = true;
}

void TSyncPoller::StopPoller()
{
  if (FStarted)
  {
    // Set stop event but do NOT WaitFor here.
    // WaitFor would deadlock if the thread is blocked in
    // TFarDialog::Synchronize() waiting for the main thread.
    // The thread will exit naturally when it sees the stop event.
    ::SetEvent(FStopEvent);
    FStarted = false;
  }
}

bool TSyncPoller::OpenNotification()
{
  FChangeHandle = ::FindFirstChangeNotification(
    FDirectory.c_str(),
    FRecursive,
    FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_DIR_NAME |
    FILE_NOTIFY_CHANGE_LAST_WRITE);
  return FChangeHandle != INVALID_HANDLE_VALUE;
}

void TSyncPoller::CloseNotification()
{
  if (FChangeHandle != INVALID_HANDLE_VALUE)
  {
    ::FindCloseChangeNotification(FChangeHandle);
    FChangeHandle = INVALID_HANDLE_VALUE;
  }
}

void TSyncPoller::Execute()
{
  if (!OpenNotification())
  {
    if (FOnInvalid)
      FOnInvalid(nullptr, FDirectory, L"Failed to open directory notification");
    return;
  }

  const DWORD PollInterval = nb::Min<DWORD>(
    static_cast<DWORD>(GetGUIConfiguration()->GetKeepUpToDateChangeDelay()), 2000u);
  HANDLE Handles[] = { FChangeHandle, FStopEvent };

  while (!IsFinished())
  {
    const DWORD WaitResult = ::WaitForMultipleObjects(
      _countof(Handles), Handles, FALSE, PollInterval);

    if (WaitResult == WAIT_OBJECT_0)  // Change detected
    {
      // Marshal to main thread for sync
      if (FOnSynchronizeThreads)
      {
        FOnSynchronizeThreads(this, FOnChange);
      }

      // Re-arm immediately (per design decision)
      if (!::FindNextChangeNotification(FChangeHandle))
      {
        // Handle invalidated (e.g., directory deleted) — reopen
        CloseNotification();
        if (!OpenNotification())
        {
          if (FOnInvalid)
            FOnInvalid(nullptr, FDirectory, L"Lost directory notification");
          break;
        }
      }
    }
    else if (WaitResult == WAIT_OBJECT_0 + 1)  // Stop event
    {
      break;
    }
    // WAIT_TIMEOUT — loop and re-wait
  }

  CloseNotification();
}

// ============================================================================
// TSynchronizeController implementation
// ============================================================================

TSynchronizeController::TSynchronizeController(
  TSynchronizeEvent AOnSynchronize, TSynchronizeInvalidEvent AOnSynchronizeInvalid,
  TSynchronizeTooManyDirectoriesEvent AOnTooManyDirectories) noexcept :
  FOnSynchronize(AOnSynchronize),
  FOptions(nullptr),
  FOnSynchronizeThreads(nullptr),
  FSyncPoller(nullptr),
  FSynchronizeAbort(nullptr),
  FOnSynchronizeInvalid(AOnSynchronizeInvalid),
  FOnTooManyDirectories(AOnTooManyDirectories),
  FSynchronizeLog(nullptr),
  FCopyParam(OBJECT_CLASS_TCopyParamType)
{
  FOnSynchronize = AOnSynchronize;
  FOnSynchronizeInvalid = AOnSynchronizeInvalid;
  FOnTooManyDirectories = AOnTooManyDirectories;
  FSyncPoller = nullptr;
  FSynchronizeAbort = nullptr;
  FSynchronizeLog = nullptr;
  FOptions = nullptr;
}

TSynchronizeController::~TSynchronizeController() noexcept
{
  DebugAssert(FSyncPoller == nullptr);
}

void TSynchronizeController::StartStop(TObject * /*Sender*/,
  bool Start, const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
  TSynchronizeOptions * Options,
  TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
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
      FOnSynchronizeThreads = OnSynchronizeThreads;

      // Create callback for polling — marshals SynchronizeChange to main thread
      TThreadMethod ChangeCallback = nb::bind(
        &TSynchronizeController::DoSynchronizeChange, this);

      FSyncPoller = new TSyncPoller(
        FSynchronizeParams.LocalDirectory,
        FLAGSET(FSynchronizeParams.Options, soRecurse),
        OnSynchronizeThreads,
        ChangeCallback,
        nb::bind(&TSynchronizeController::DoSynchronizeInvalid, this));

      SynchronizeLog(slStart, FMTLOAD(SYNCHRONIZE_START, 1));

      try
      {
        FSyncPoller->StartPoller();
      }
      catch (...)
      {
        SAFE_DESTROY(FSyncPoller);
        throw;
      }
    }
    catch (...)
    {
      if (FSyncPoller != nullptr)
      {
        FSyncPoller->StopPoller();
        SAFE_DESTROY(FSyncPoller);
      }
      throw;
    }
  }
  else
  {
    FOptions = nullptr;
    if (FSyncPoller != nullptr)
    {
      FSyncPoller->StopPoller();
      SAFE_DESTROY(FSyncPoller);
    }
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
        }
        __finally__removed
        {
#if defined(__BORLANDC__)
          delete Checklist;
#endif // defined(__BORLANDC__)
        } end_try__finally
      }
    }
  }
  catch(Exception & E)
  {
    SynchronizeAbort(nb::isa<EFatal>(&E));
  }
}

void TSynchronizeController::SynchronizeAbort(bool Close)
{
  if (FSyncPoller != nullptr)
  {
    FSyncPoller->StopPoller();
    SAFE_DESTROY(FSyncPoller);
  }
  DebugAssert(FSynchronizeAbort);
  FSynchronizeAbort(nullptr, Close);
}


void TSynchronizeController::NotifySynchronizeChange(const UnicodeString & Directory)
{
  FSubdirsChanged = false;
  SynchronizeChange(this, Directory, FSubdirsChanged);
}

void TSynchronizeController::DoSynchronizeChange()
{
  NotifySynchronizeChange(FSynchronizeParams.LocalDirectory);
}

void TSynchronizeController::DoSynchronizeInvalid(TSynchronizeController * /*Sender*/,
  const UnicodeString & Directory, const UnicodeString & ErrorStr)
{
  SynchronizeInvalid(this, Directory, ErrorStr);
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
  TObject * /*Sender*/, int32_t & MaxDirectories)
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
