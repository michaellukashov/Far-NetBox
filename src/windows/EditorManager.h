
#ifndef EditorManagerH
#define EditorManagerH

#include <vector>

class TManagedTerminal;
class TTerminalQueue;

struct TEditedFileData
{
  TEditedFileData();
  ~TEditedFileData();

  UnicodeString LocalRootDirectory;
  UnicodeString RemoteDirectory;
  bool ForceText;
  TManagedTerminal * Terminal;
  TSessionData * SessionData;
  TTerminalQueue * Queue;
  UnicodeString SessionName;
  UnicodeString OriginalFileName;
  UnicodeString Command;
  TDateTime SourceTimestamp;
};

typedef void (__closure * TEditedFileChangedEvent)
  (const UnicodeString & FileName, TEditedFileData * Data, HANDLE CompleteEvent, bool & Retry);
typedef void (__closure * TEditedFileReloadEvent)
  (const UnicodeString & FileName, TEditedFileData * Data);
typedef void (__closure * TEditedFileEarlyClosedEvent)
  (const TEditedFileData * Data, bool & KeepOpen);
typedef void (__closure * TEditedFileUploadComplete)
  (TObject * Token);

typedef void (__closure * TEditedFileProcessEvent)
  (const UnicodeString FileName, TEditedFileData * Data, TObject * Token, void * Arg);

class TEditorManager
{
public:
  TEditorManager();
  ~TEditorManager();

  bool Empty(bool IgnoreClosed);
  bool CanAddFile(const UnicodeString RemoteDirectory,
    const UnicodeString OriginalFileName, const UnicodeString SessionName,
    TObject *& Token, UnicodeString & ExistingLocalRootDirectory,
    UnicodeString & ExistingLocalDirectory);
  bool CloseInternalEditors(TNotifyEvent CloseCallback);
  bool CloseExternalFilesWithoutProcess();

  void AddFileInternal(const UnicodeString FileName,
    TEditedFileData * Data, TObject * Token);
  void AddFileExternal(const UnicodeString FileName,
    TEditedFileData * Data, HANDLE Process);

  void Check();

  void FileChanged(TObject * Token);
  void FileReload(TObject * Token);
  void FileClosed(TObject * Token, bool Forced);

  void ProcessFiles(TEditedFileProcessEvent Callback, void * Arg);
  TEditedFileData * FindByUploadCompleteEvent(HANDLE UploadCompleteEvent);

  __property TEditedFileChangedEvent OnFileChange = { read = FOnFileChange, write = FOnFileChange };
  __property TEditedFileReloadEvent OnFileReload = { read = FOnFileReload, write = FOnFileReload };
  __property TEditedFileEarlyClosedEvent OnFileEarlyClosed = { read = FOnFileEarlyClosed, write = FOnFileEarlyClosed };
  __property TEditedFileUploadComplete OnFileUploadComplete = { read = FOnFileUploadComplete, write = FOnFileUploadComplete };
  __property TCriticalSection * Section = { read = FSection };

private:
  struct TFileData
  {
    UnicodeString FileName;
    bool External;
    HANDLE Process;
    TObject * Token;
    TDateTime Timestamp;
    TEditedFileData * Data;
    bool Closed;
    HANDLE UploadCompleteEvent;
    TDateTime Opened;
    bool Reupload;
    bool Reloading;
    unsigned int Saves;
  };

  std::vector<TFileData> FFiles;
  std::vector<HANDLE> FProcesses;
  std::vector<HANDLE> FUploadCompleteEvents;
  TEditedFileChangedEvent FOnFileChange;
  TEditedFileReloadEvent FOnFileReload;
  TEditedFileEarlyClosedEvent FOnFileEarlyClosed;
  TEditedFileUploadComplete FOnFileUploadComplete;
  TCriticalSection * FSection;

  void AddFile(TFileData & FileData, TEditedFileData * Data);
  void UploadComplete(int Index);
  bool CloseFile(int Index, bool IgnoreErrors, bool Delete);
  void CloseProcess(int Index);
  bool EarlyClose(int Index);
  bool HasFileChanged(int Index, TDateTime & NewTimestamp);
  void CheckFileChange(int Index, bool Force);
  int FindFile(const TObject * Token);
  void ReleaseFile(int Index);
  TDateTime NormalizeTimestamp(const TDateTime & Timestamp);
  bool GetFileTimestamp(const UnicodeString & FileName, TDateTime & Timestamp);

  enum TWaitHandle { PROCESS, EVENT };
  int WaitFor(unsigned int Count, const HANDLE * Handles,
    TWaitHandle WaitFor);
};

#endif
