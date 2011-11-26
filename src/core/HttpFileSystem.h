#pragma once

#include <FileSystems.h>
#include "FtpFileSystem.h"
#include "EasyURL.h"
//---------------------------------------------------------------------------
class TCURLIntf;
class THTTPCommandSet;
// class TSecureShell;
struct TListDataEntry;
class TMessageQueue;
class TiXmlElement;
//---------------------------------------------------------------------------
class THTTPFileSystem : public TCustomFileSystem
{
friend class CEasyURL;
friend class TFileListHelper;
public:
  explicit THTTPFileSystem(TTerminal *ATerminal);
  virtual void Init(TSecureShell *SecureShell);
  virtual ~THTTPFileSystem();

  virtual void Open();
  virtual void Close();
  virtual bool GetActive();
  virtual void Idle();
  virtual std::wstring AbsolutePath(std::wstring Path, bool Local);
  virtual void AnyCommand(const std::wstring Command,
    const captureoutput_slot_type *OutputEvent);
  virtual void ChangeDirectory(const std::wstring Directory);
  virtual void CachedChangeDirectory(const std::wstring Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const std::wstring FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const std::wstring & Alg,
    TStrings * FileList, TStrings * Checksums,
    calculatedchecksum_slot_type *OnCalculatedChecksum);
  virtual void CopyToLocal(TStrings * FilesToCopy,
    const std::wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const std::wstring TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const std::wstring DirName);
  virtual void CreateLink(const std::wstring FileName, const std::wstring PointTo, bool Symbolic);
  virtual void DeleteFile(const std::wstring FileName,
    const TRemoteFile * File, int Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const std::wstring FileName,
    const TRemoteFile * File, std::wstring Command, int Params, const captureoutput_slot_type &OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(int Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const std::wstring FileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RenameFile(const std::wstring FileName,
    const std::wstring NewName);
  virtual void CopyFile(const std::wstring FileName,
    const std::wstring NewName);
  virtual std::wstring FileUrl(const std::wstring FileName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const std::wstring Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo();
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const std::wstring & FileName);
  virtual bool GetStoredCredentialsTried();
  virtual std::wstring GetUserName();

protected:
  enum TOverwriteMode { omOverwrite, omResume };
protected:
  bool PostMessage(unsigned int Type, WPARAM wParam, LPARAM lParam);
  bool ProcessMessage();
  void DiscardMessages();
  void WaitForMessages();
  unsigned int WaitForReply(bool Command, bool WantLastCode);
  unsigned int WaitForCommandReply(bool WantLastCode = true);
  void WaitForFatalNonCommandReply();
  void PoolForFatalNonCommandReply();
  void GotNonCommandReply(unsigned int Reply);
  void GotReply(unsigned int Reply, unsigned int Flags = 0,
    std::wstring Error = L"", unsigned int * Code = NULL,
    TStrings ** Response = NULL);
  void ResetReply();
  void HandleReplyStatus(std::wstring Response);
  void DoWaitForReply(unsigned int& ReplyToAwait, bool WantLastCode);
  bool KeepWaitingForReply(unsigned int& ReplyToAwait, bool WantLastCode);
  inline bool NoFinalLastCode();

  bool HandleStatus(const wchar_t * Status, int Type);
  bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
    bool HasTime1, bool HasTime2, void * UserData, int & RequestResult);
  // bool HandleAsynchRequestVerifyCertificate(
    // const TFtpsCertificateData & Data, int & RequestResult);
  bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    unsigned int Count);
  bool HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
    bool FileTransfer);
  bool HandleReply(int Command, unsigned int Reply);
  bool HandleCapabilities(bool Mfmt);
  bool CheckError(int ReturnCode, const wchar_t * Context);
  void EnsureLocation();
  std::wstring ActualCurrentDirectory();
  void Discard();
  void DoChangeDirectory(const std::wstring & Directory);

  void Sink(const std::wstring FileName,
    const TRemoteFile * File, const std::wstring TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TDownloadSessionAction & Action);
  void SinkRobust(const std::wstring FileName,
    const TRemoteFile * File, const std::wstring TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void SinkFile(std::wstring FileName, const TRemoteFile * File, void * Param);
  void SourceRobust(const std::wstring FileName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void Source(const std::wstring FileName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TUploadSessionAction & Action);
  void DirectorySource(const std::wstring DirectoryName,
    const std::wstring TargetDir, int Attrs, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags);
  bool ConfirmOverwrite(std::wstring & FileName,
    TOverwriteMode & OverwriteMode, TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams, int Params, bool AutoResume);
  void ReadDirectoryProgress(__int64 Bytes);
  void ResetFileTransfer();
  void DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void FileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void ResetCaches();
  void CaptureOutput(const std::wstring & Str);
  void DoReadDirectory(TRemoteFileList * FileList);
  void FileTransfer(const std::wstring & FileName, const std::wstring & LocalFile,
    const std::wstring & RemoteFile, const std::wstring & RemotePath, bool Get,
    __int64 Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);
  TDateTime ConvertLocalTimestamp(time_t Time);
  TDateTime ConvertRemoteTimestamp(time_t Time, bool HasTime);
  void SetLastCode(int Code);

  static bool Unquote(std::wstring & Str);
  static std::wstring ExtractStatusMessage(std::wstring Status);

protected:
  // __property TStrings * Output = { read = FOutput };
  TStrings *GetOutput() { return FOutput; };
  // __property int ReturnCode = { read = FReturnCode };
  int GetReturnCode() { return FReturnCode; }

  virtual std::wstring GetCurrentDirectory();

protected:
  const wchar_t *GetOption(int OptionID) const;
  int GetOptionVal(int OptionID) const;

private:
  enum TCommand
  {
    CMD_UNKNOWN,
    PASS,
    SYST,
    FEAT
  };

  // TSecureShell * FSecureShell;
  THTTPCommandSet * FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  std::wstring FCurrentDirectory;
  std::wstring FHomeDirectory;
  TRemoteFileList * FFileList;
  TRemoteFileList * FFileListCache;
  TStrings * FOutput;
  int FReturnCode;
  std::wstring FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  captureoutput_signal_type FOnCaptureOutput;
  TSessionInfo FSessionInfo;
  std::wstring FUserName;
  TDateTime FLastDataSent;
  TCURLIntf * FCURLIntf;
  bool FPasswordFailed;
  std::wstring FSystem;
  bool FActive;
  bool FWaitingForReply;
  enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort;
  bool FIgnoreFileList;
  bool FFileTransferCancelled;
  __int64 FFileTransferResumed;
  bool FFileTransferPreserveTime;
  unsigned long FFileTransferCPSLimit;
  bool FAwaitingProgress;
  unsigned int FReply;
  unsigned int FCommandReply;
  TCommand FLastCommand;
  bool FMultineResponse;
  int FLastCode;
  int FLastCodeClass;
  int FLastReadDirectoryProgress;
  TStrings * FLastResponse;
  TStrings * FLastError;
  TCriticalSection * FQueueCriticalSection;
  TCriticalSection * FTransferStatusCriticalSection;
  TMessageQueue *FQueue;
  HANDLE FQueueEvent;
  TAutoSwitch FListAll;
  bool FDoListAll;
  mutable std::wstring FOptionScratch;
  HANDLE FAbortEvent;
  int m_ProgressPercent; ///< Progress percent value
  THTTPFileSystem *Self;

  void ClearAliases();
  void ClearAlias(std::wstring Alias);
  void CustomReadFile(const std::wstring FileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static std::wstring DelimitStr(std::wstring Str);
  void DetectReturnVar();
  bool IsLastLine(std::wstring & Line);
  static bool IsTotalListingLine(const std::wstring Line);
  void ExecCommand(const std::wstring & Cmd, int Params,
    const std::wstring & CmdString);
  void ExecCommand(TFSCommand Cmd, int Params = -1, ...);
  void ReadCommandOutput(int Params, const std::wstring * Cmd = NULL);
  void SCPResponse(bool * GotLastLine = NULL);
  void SCPDirectorySource(const std::wstring DirectoryName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, int Level);
  void SCPError(const std::wstring Message, bool Fatal);
  void SCPSendError(const std::wstring Message, bool Fatal);
  void SCPSink(const std::wstring TargetDir,
    const std::wstring FileName, const std::wstring SourceDir,
    const TCopyParamType * CopyParam, bool & Success,
    TFileOperationProgressType * OperationProgress, int Params, int Level);
  void SCPSource(const std::wstring FileName,
    const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, int Level);
  void SendCommand(const std::wstring Cmd);
  void SkipFirstLine();
  void UnsetNationalVars();
  TRemoteFile * CreateRemoteFile(const std::wstring & ListingStr,
    TRemoteFile * LinkedByFile = NULL);
  void CaptureOutput(const std::wstring & AddedLine, bool StdError);
  void ChangeFileToken(const std::wstring & DelimitedName,
    const TRemoteToken & Token, TFSCommand Cmd, const std::wstring & RecursiveStr);

  static bool RemoveLastLine(std::wstring & Line,
    int & ReturnCode, std::wstring LastLine = L"");
private:
  enum
  {
    REPLY_CONNECT =      0x01,
    REPLY_2XX_CODE =     0x02,
    REPLY_ALLOW_CANCEL = 0x04
  };
    enum ItemType
    {
        ItemDirectory,
        ItemFile,
    };
private:
    //From IProtocol
    virtual bool Connect(HANDLE abortEvent, std::wstring &errorInfo);
    virtual bool CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, std::wstring &errorInfo);
    virtual bool MakeDirectory(const wchar_t *path, std::wstring &errorInfo);
    virtual bool GetList(const std::wstring &Directory, std::vector<TListDataEntry> &Entries);
    virtual bool GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo);
    virtual bool PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo);
    virtual bool Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType type, std::wstring &errorInfo);
    virtual bool Delete(const wchar_t *path, const ItemType type, std::wstring &errorInfo);
    virtual bool Aborted() const
    {
        return FCURLIntf->Aborted();
    }

private:
    /**
     * Format error description
     * \param errCode system error code
     * \param info additional info
     * \return error description
     */
    std::wstring FormatErrorDescription(const DWORD errCode, const wchar_t *info = NULL) const;

private:
    /**
     * Send PROPFIND request
     * \param dir directory to load
     * \param response response buffer
     * \param errInfo buffer to save error message
     * \return false if error
     */
    bool SendPropFindRequest(const wchar_t *dir, std::wstring &response, std::wstring &errInfo);

    /**
     * Check response for valid code
     * \param expect expected response code
     * \param errInfo buffer to save error message
     * \return false if error (response unexpected)
     */
    bool CheckResponseCode(const long expect, std::wstring &errInfo);

    /**
     * Check response for valid code
     * \param expect1 expected response code
     * \param expect2 expected response code
     * \param errInfo buffer to save error message
     * \return false if error (response unexpected)
     */
    bool CheckResponseCode(const long expect1, const long expect2, std::wstring &errInfo);

    /**
     * Get incorrect response information
     * \param code response code
     * \return response information
     */
    std::wstring GetBadResponseInfo(const int code) const;

    /**
     * Get xml namespace
     * \param element xml element
     * \param name namespace name (URI)
     * \param defaultVal default namespace id
     * \return namespace id
     */
    std::string GetNamespace(const TiXmlElement *element, const char *name, const char *defaultVal) const;

    /**
     * Parse internet datetime
     * \param dt internet datetime
     * \return corresponding FILETIME (filled zero if error)
     */
    FILETIME ParseDateTime(const char *dt) const;

    /**
     * Check for hexadecimal char (0123456789abcdefABCDEF)
     * \param ch checked char
     * \return true if cahr is a hexadecimal
     */
    inline bool IsHexadecimal(const char ch) const
    {
        return ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
    }

    /**
     * Decode content with safe symbols wrapper (%XX)
     * \param src source std::string
     * \return decoded content
     */
    std::string DecodeHex(const std::string &src) const;

    /**
     * Encode URL to UTF8 format with unsafe symbols wrapper (%XX)
     * \param src source std::string
     * \return encoded URL
     */
    std::string EscapeUTF8URL(const wchar_t *src) const;

protected:
    CURLcode CURLPrepare(const char *webDavPath, const bool handleTimeout = true);
private:
  THTTPFileSystem(const THTTPFileSystem &);
  void operator=(const THTTPFileSystem &);
};

