//---------------------------------------------------------------------------
#ifndef SftpFileSystemH
#define SftpFileSystemH

#include <stdint.h>
#include <FileSystems.h>
//---------------------------------------------------------------------------
class TSFTPPacket;
struct TOverwriteFileParams;
struct TSFTPSupport;
class TSecureShell;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSFTPFileSystem : public TCustomFileSystem
{
NB_DISABLE_COPY(TSFTPFileSystem)
friend class TSFTPPacket;
friend class TSFTPQueue;
friend class TSFTPAsynchronousQueue;
friend class TSFTPUploadQueue;
friend class TSFTPDownloadQueue;
friend class TSFTPLoadFilesPropertiesQueue;
friend class TSFTPCalculateFilesChecksumQueue;
friend class TSFTPBusy;
public:
  explicit TSFTPFileSystem(TTerminal * ATermina);
  virtual ~TSFTPFileSystem();

  virtual void Init(void * Data); // TSecureShell *
  virtual void FileTransferProgress(int64_t TransferSize, int64_t Bytes) {}

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const { return FSecureShell->GetActive(); }
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString AbsolutePath(const UnicodeString & APath, bool Local);
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const UnicodeString & DirName);
  virtual void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void DeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& AFile);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& AFile);
  virtual void RemoteRenameFile(const UnicodeString & AFileName,
    const UnicodeString & NewName);
  virtual void CopyFile(const UnicodeString & AFileName,
    const UnicodeString & ANewName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo() const;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName);
  virtual bool GetStoredCredentialsTried();
  virtual UnicodeString GetUserName();

protected:
  TSecureShell * FSecureShell;
  TFileSystemInfo FFileSystemInfo;
  bool FFileSystemInfoValid;
  intptr_t FVersion;
  UnicodeString FCurrentDirectory;
  UnicodeString FDirectoryToChangeTo;
  UnicodeString FHomeDirectory;
  AnsiString FEOL;
  TList * FPacketReservations;
  rde::vector<uintptr_t> FPacketNumbers;
  uint8_t FPreviousLoggedPacket;
  int FNotLoggedPackets;
  int FBusy;
  void * FBusyToken;
  bool FAvoidBusy;
  TStrings * FExtensions;
  TSFTPSupport * FSupport;
  bool FUtfStrings;
  bool FSignedTS;
  TStrings * FFixedPaths;
  uint32_t FMaxPacketSize;
  bool FSupportsStatVfsV2;
  uintptr_t FCodePage;

  void SendCustomReadFile(TSFTPPacket * Packet, TSFTPPacket * Response,
    uint32_t Flags);
  void CustomReadFile(const UnicodeString & AFileName,
    TRemoteFile *& AFile, uint8_t Type, TRemoteFile * ALinkedByFile = nullptr,
    int AllowStatus = -1);
  virtual UnicodeString GetCurrentDirectory();
  UnicodeString GetHomeDirectory();
  uintptr_t GotStatusPacket(TSFTPPacket * Packet, int AllowStatus);
  bool RemoteFileExists(const UnicodeString & FullPath, TRemoteFile ** AFile = nullptr);
  TRemoteFile * LoadFile(TSFTPPacket * Packet,
    TRemoteFile * ALinkedByFile, const UnicodeString & AFileName,
    TRemoteFileList * TempFileList = nullptr, bool Complete = true);
  void LoadFile(TRemoteFile * AFile, TSFTPPacket * Packet,
    bool Complete = true);
  UnicodeString LocalCanonify(const UnicodeString & APath);
  UnicodeString Canonify(const UnicodeString & APath);
  UnicodeString RealPath(const UnicodeString & APath);
  UnicodeString RealPath(const UnicodeString & APath, const UnicodeString & ABaseDir);
  void ReserveResponse(const TSFTPPacket * Packet,
    TSFTPPacket * Response);
  uintptr_t ReceivePacket(TSFTPPacket * Packet, int ExpectedType = -1,
    int AllowStatus = -1);
  bool PeekPacket();
  void RemoveReservation(intptr_t Reservation);
  void SendPacket(const TSFTPPacket * Packet);
  uintptr_t ReceiveResponse(const TSFTPPacket * Packet,
    TSFTPPacket * AResponse, int ExpectedType = -1, int AllowStatus = -1);
  uintptr_t SendPacketAndReceiveResponse(const TSFTPPacket * Packet,
    TSFTPPacket * Response, int ExpectedType = -1, int AllowStatus = -1);
  void UnreserveResponse(TSFTPPacket * Response);
  void TryOpenDirectory(const UnicodeString & Directory);
  bool SupportsExtension(const UnicodeString & Extension) const;
  void ResetConnection();
  void DoCalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel);
  void DoDeleteFile(const UnicodeString & AFileName, uint8_t Type);

  void SFTPSourceRobust(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SFTPSource(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TOpenRemoteFileParams & OpenParams,
    TOverwriteFileParams & FileParams,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action, bool & ChildError);
  RawByteString SFTPOpenRemoteFile(const UnicodeString & AFileName,
    uint32_t OpenType, int64_t Size = -1);
  intptr_t SFTPOpenRemote(void * AOpenParams, void * Param2);
  void SFTPCloseRemote(const RawByteString & Handle,
    const UnicodeString & AFileName, TFileOperationProgressType * OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket * Packet);
  void SFTPDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, uintptr_t LocalFileAttrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SFTPConfirmOverwrite(const UnicodeString & AFullFileName, UnicodeString & AFileName,
    const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
    const TOverwriteFileParams * FileParams,
    OUT TOverwriteMode & Mode);
  bool SFTPConfirmResume(const UnicodeString & DestFileName, bool PartialBiggerThanSource,
    TFileOperationProgressType * OperationProgress);
  void SFTPSinkRobust(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SFTPSink(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action, bool & ChildError);
  void SFTPSinkFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, void * Param);
  char * GetEOL() const;
  inline void BusyStart();
  inline void BusyEnd();
  uint32_t TransferBlockSize(uint32_t Overhead,
    TFileOperationProgressType * OperationProgress,
    uint32_t MinPacketSize = 0,
    uint32_t MaxPacketSize = 0);
  uint32_t UploadBlockSize(const RawByteString & Handle,
    TFileOperationProgressType * OperationProgress);
  uint32_t DownloadBlockSize(
    TFileOperationProgressType * OperationProgress);
  intptr_t PacketLength(uint8_t * LenBuf, intptr_t ExpectedType);

private:
  inline const TSessionData * GetSessionData() const { return FTerminal->GetSessionData(); }
};
//---------------------------------------------------------------------------
#endif // SftpFileSystemH
