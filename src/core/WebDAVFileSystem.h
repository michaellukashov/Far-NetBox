#pragma once

#include <FileSystems.h>
#include "Terminal.h"
#include "EasyURL.h"
//---------------------------------------------------------------------------
class TCURLIntf;
struct TListDataEntry;
class TMessageQueue;
class TiXmlElement;

//---------------------------------------------------------------------------
class TWebDAVFileSystem : public TCustomFileSystem
{
  friend class CEasyURL;
  friend class TWebDAVFileListHelper;
public:
  explicit TWebDAVFileSystem(TTerminal * ATerminal);
  virtual ~TWebDAVFileSystem();
  virtual void __fastcall Init();

  virtual void __fastcall Open();
  virtual void __fastcall Close();
  virtual bool __fastcall GetActive();
  virtual void __fastcall Idle();
  virtual UnicodeString __fastcall AbsolutePath(UnicodeString Path, bool Local);
  virtual void __fastcall AnyCommand(const UnicodeString Command,
    TCaptureOutputEvent * OutputEvent);
  virtual void __fastcall ChangeDirectory(const UnicodeString Directory);
  virtual void __fastcall CachedChangeDirectory(const UnicodeString Directory);
  virtual void __fastcall AnnounceFileListOperation();
  virtual void __fastcall ChangeFileProperties(const UnicodeString FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool __fastcall LoadFilesProperties(TStrings * FileList);
  virtual void __fastcall CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void __fastcall CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void __fastcall CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void __fastcall CreateDirectory(const UnicodeString DirName);
  virtual void __fastcall CreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  virtual void __fastcall DeleteFile(const UnicodeString FileName,
    const TRemoteFile * File, int Params, TRmSessionAction & Action);
  virtual void /* __fastcall */ CustomCommandOnFile(const UnicodeString FileName,
    const TRemoteFile * File, UnicodeString Command, int Params, TCaptureOutputEvent * OutputEvent);
  virtual void __fastcall DoStartup();
  virtual void __fastcall HomeDirectory();
  virtual bool __fastcall IsCapable(int Capability) const;
  virtual void __fastcall LookupUsersGroups();
  virtual void __fastcall ReadCurrentDirectory();
  virtual void __fastcall ReadDirectory(TRemoteFileList * FileList);
  virtual void __fastcall ReadFile(const UnicodeString FileName,
    TRemoteFile *& File);
  virtual void __fastcall ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void __fastcall RenameFile(const UnicodeString FileName,
    const UnicodeString NewName);
  virtual void __fastcall CopyFile(const UnicodeString FileName,
    const UnicodeString NewName);
  virtual UnicodeString __fastcall FileUrl(const UnicodeString FileName);
  virtual TStrings * __fastcall GetFixedPaths();
  virtual void __fastcall SpaceAvailable(const UnicodeString Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & __fastcall GetSessionInfo();
  virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve);
  virtual bool __fastcall TemporaryTransferFile(const UnicodeString & FileName);
  virtual bool __fastcall GetStoredCredentialsTried();
  virtual UnicodeString __fastcall GetUserName();

public:
  virtual void __fastcall __fastcall FileTransferProgress(__int64 TransferSize, __int64 Bytes);

protected:
#ifndef _MSC_VER
  __property TStrings * Output = { read = FOutput };
  __property int ReturnCode = { read = FReturnCode };
#endif
  virtual UnicodeString __fastcall GetCurrentDirectory();

  bool __fastcall HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    size_t Count);
  bool __fastcall HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
    bool FileTransfer);
  bool __fastcall HandleCapabilities(bool Mfmt);
  bool __fastcall CheckError(int ReturnCode, const wchar_t * Context);
  void __fastcall EnsureLocation();
  UnicodeString __fastcall ActualCurrentDirectory();
  void __fastcall Discard();
  void __fastcall DoChangeDirectory(const UnicodeString Directory);

  void __fastcall Sink(const UnicodeString FileName,
    const TRemoteFile * File, const UnicodeString TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TDownloadSessionAction & Action);
  void __fastcall SinkRobust(const UnicodeString FileName,
    const TRemoteFile * File, const UnicodeString TargetDir,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void SinkFile(const UnicodeString FileName, const TRemoteFile * File, void * Param);
  void __fastcall WebDAVSourceRobust(const UnicodeString FileName,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void __fastcall WebDAVSource(const UnicodeString FileName,
    const UnicodeString TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TUploadSessionAction & Action);
  void __fastcall WebDAVDirectorySource(const UnicodeString DirectoryName,
    const UnicodeString TargetDir, int Attrs, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags);
  bool __fastcall ConfirmOverwrite(UnicodeString & FileName,
    TOverwriteMode & OverwriteMode, TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams, int Params, bool AutoResume);
  void __fastcall ReadDirectoryProgress(__int64 Bytes);
  void __fastcall ResetFileTransfer();
  void __fastcall DoFileTransferProgress(__int64 TransferSize, __int64 Bytes);
  void __fastcall ResetCaches();
  void __fastcall CaptureOutput(const UnicodeString Str);
  void __fastcall DoReadDirectory(TRemoteFileList * FileList);
  void __fastcall FileTransfer(const UnicodeString FileName, const UnicodeString LocalFile,
    const UnicodeString RemoteFile, const UnicodeString RemotePath, bool Get,
    __int64 Size, int Type, TFileTransferData & UserData,
    TFileOperationProgressType * OperationProgress);

protected:
  const wchar_t * __fastcall GetOption(int OptionID) const;
  int __fastcall GetOptionVal(int OptionID) const;

private:
  enum TCommand
  {
    CMD_UNKNOWN,
    PASS,
    SYST,
    FEAT
  };

  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  UnicodeString FHomeDirectory;
  TRemoteFileList * FFileList;
  UnicodeString FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  captureoutput_signal_type FOnCaptureOutput;
  TSessionInfo FSessionInfo;
  UnicodeString FUserName;
  TDateTime FLastDataSent;
  TCURLIntf * FCURLIntf;
  bool FPasswordFailed;
  UnicodeString FSystem;
  bool FActive;
  bool FWaitingForReply;
  enum { ftaNone, ftaSkip, ftaCancel } FFileTransferAbort;
  bool FIgnoreFileList;
  bool FFileTransferCancelled;
  __int64 FFileTransferResumed;
  bool FFileTransferPreserveTime;
  size_t FFileTransferCPSLimit;
  bool FAwaitingProgress;
  TCommand FLastCommand;
  size_t FLastReadDirectoryProgress;
  TStrings * FLastResponse;
  TStrings * FLastError;
  TCriticalSection * FTransferStatusCriticalSection;
  TAutoSwitch FListAll;
  bool FDoListAll;
  mutable UnicodeString FOptionScratch;
  size_t m_ProgressPercent; ///< Progress percent value
  TWebDAVFileSystem * Self;

private:
  void __fastcall CustomReadFile(const UnicodeString FileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static UnicodeString __fastcall DelimitStr(const UnicodeString Str);
  TRemoteFile * __fastcall CreateRemoteFile(const UnicodeString ListingStr,
      TRemoteFile * LinkedByFile = NULL);
  void __fastcall CaptureOutput(const UnicodeString AddedLine, bool StdError);

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
  bool WebDAVCheckExisting(const wchar_t * path, const ItemType type, bool & isExist, UnicodeString & errorInfo);
  bool WebDAVMakeDirectory(const wchar_t * path, UnicodeString & errorInfo);
  bool WebDAVGetList(const UnicodeString Directory, UnicodeString & errorInfo);
  bool WebDAVGetFile(const wchar_t * remotePath, const wchar_t * localPath, const unsigned __int64 fileSize, UnicodeString & errorInfo);
  bool WebDAVPutFile(const wchar_t * remotePath, const wchar_t * localPath, const unsigned __int64 fileSize, UnicodeString & errorInfo);
  bool WebDAVRename(const wchar_t * srcPath, const wchar_t * dstPath, const ItemType type, UnicodeString & errorInfo);
  bool WebDAVDelete(const wchar_t * path, const ItemType type, UnicodeString & errorInfo);
  bool WebDAVAborted() const
  {
    return FCURLIntf->Aborted();
  }

private:
  /** @brief Format error description
    * @param errCode system error code
    * @param info additional info
    * @return error description
    */
  UnicodeString FormatErrorDescription(const DWORD errCode, const wchar_t * info = NULL) const;

private:
  /** @brief Send PROPFIND request
    * @param dir directory to load
    * @param responseCode response code
    * @param response response buffer
    * @param errInfo buffer to save error message
    * @return false if error
    */
  bool SendPropFindRequest(const wchar_t * dir, long & responseCode, UnicodeString & response, UnicodeString & errInfo);

  /** @brief Check response for valid code
    * @param expect expected response code
    * @param responseCode buffer to save error code
    * @param errInfo buffer to save error message
    * @return false if error (response unexpected)
    */
  bool CheckResponseCode(const long expect, long & responseCode, UnicodeString & errInfo);

  /** @brief response for valid code
    * @param expect1 expected response code
    * @param expect2 expected response code
    * @param responseCode buffer to save error code
    * @param errInfo buffer to save error message
    * @return false if error (response unexpected)
    */
  bool CheckResponseCode(const long expect1, const long expect2, long & responseCode, UnicodeString & errInfo);

  /** @brief Get incorrect response information
    * @param code response code
    * @return response information
    */
  UnicodeString GetBadResponseInfo(const int code) const;

  /** @brief Get xml namespace
    * @param element xml element
    * @param name namespace name (URI)
    * @param defaultVal default namespace id
    * @return namespace id
    */
  std::string GetNamespace(const TiXmlElement * element, const char * name, const char * defaultVal) const;

  /** @brief Parse internet datetime
    * @param dt internet datetime
    * @return corresponding FILETIME (filled zero if error)
    */
  FILETIME ParseDateTime(const char * dt) const;

  /** @brief Check for hexadecimal char (0123456789abcdefABCDEF)
    * @param ch checked char
    * @return true if cahr is a hexadecimal
    */
  inline bool IsHexadecimal(const char ch) const
  {
    return ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
  }

  /** @brief Decode content with safe symbols wrapper (%XX)
    * @param src source std::string
    * @return decoded content
    */
  std::string DecodeHex(const std::string & src) const;

  /** @brief Encode URL to UTF8 format with unsafe symbols wrapper (%XX)
    * @param src source std::string
    * @return encoded URL
    */
  std::string EscapeUTF8URL(const wchar_t * src) const;

protected:
  CURLcode CURLPrepare(const char * webDavPath, const bool handleTimeout = true);
private:
  TWebDAVFileSystem(const TWebDAVFileSystem &);
  void __fastcall operator=(const TWebDAVFileSystem &);
};

