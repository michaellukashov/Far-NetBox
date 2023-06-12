
#pragma once

#include <FileSystems.h>

using SSH_FX_TYPE = int32_t;
using SSH_FXP_TYPE = int32_t;
using SSH_FILEXFER_ATTR_TYPE = uint32_t;
using SSH_FILEXFER_TYPE = uint8_t;
using SSH_FXF_TYPE = uint32_t;
using ACE4_TYPE = uint32_t;

class TSFTPPacket;
struct TOverwriteFileParams;
struct TSFTPSupport;
class TSecureShell;
class TEncryption;

// moved to FileSystems.h
// enum TSFTPOverwriteMode { omOverwrite, omAppend, omResume };
// extern const int32_t SFTPMaxVersion;

NB_DEFINE_CLASS_ID(TSFTPFileSystem);
class NB_CORE_EXPORT TSFTPFileSystem final : public TCustomFileSystem
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
  static bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSFTPFileSystem); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPFileSystem) || TCustomFileSystem::is(Kind); }
public:
  TSFTPFileSystem() = delete;
  explicit TSFTPFileSystem(TTerminal * ATerminal) noexcept;
  virtual ~TSFTPFileSystem() noexcept;

  void Init(void * Data /*TSecureShell* */) override;
  void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) override;
  virtual void AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *AProperties,
    TChmodSessionAction &Action) override;
  virtual bool LoadFilesProperties(TStrings *AFileList) override;
  virtual UnicodeString CalculateFilesChecksumInitialize(const UnicodeString & Alg);
  virtual void CalculateFilesChecksum(
    const UnicodeString Alg, TStrings *AFileList, TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel) override;
  virtual void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    int32_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    int32_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, int32_t AParams,
    TFileOperationProgressType *OperationProgress, uint32_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  virtual void DirectorySunk(
    const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType * CopyParam) override;
  virtual void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, int32_t AAttrs,
    const TCopyParamType* CopyParam, int32_t AParams, TFileOperationProgressType* OperationProgress,
    uint32_t AFlags, TDownloadSessionAction& Action) override;
  virtual void RemoteCreateDirectory(const UnicodeString ADirName, bool Encrypt) override;
  virtual void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, int32_t Params, TRmSessionAction &Action) override;
  virtual void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, int32_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual UnicodeString GetHomeDirectory() override;
  virtual bool IsCapable(int32_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList *FileList) override;
  virtual void ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void ReadSymlink(TRemoteFile *ASymlinkFile,
    TRemoteFile *&AFile) override;
  virtual void RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual void RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual TStrings * GetFixedPaths() const override;
  virtual void SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo & GetSessionInfo() const override;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  virtual bool TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  virtual void ClearCaches() override;

  UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
protected:
  TSecureShell *FSecureShell{nullptr};
  TFileSystemInfo FFileSystemInfo{};
  bool FFileSystemInfoValid{false};
  int32_t FVersion{0};
  UnicodeString FCurrentDirectory;
  UnicodeString FDirectoryToChangeTo;
  UnicodeString FHomeDirectory;
  AnsiString FEOL;
  std::unique_ptr<TList> FPacketReservations;
  nb::vector_t<uintptr_t> FPacketNumbers;
  SSH_FXP_TYPE FPreviousLoggedPacket{0};
  int32_t FNotLoggedWritePackets{0}, FNotLoggedReadPackets{0}, FNotLoggedStatusPackets{0}, FNotLoggedDataPackets{0};
  std::set<uint32_t> FNotLoggedRequests;
  int32_t FBusy{0};
  void * FBusyToken{nullptr};
  bool FAvoidBusy{false};
  std::unique_ptr<TStrings> FExtensions;
  std::unique_ptr<TSFTPSupport> FSupport;
  TAutoSwitch FUtfStrings{asAuto};
  bool FUtfDisablingAnnounced{false};
  bool FSignedTS{false};
  std::unique_ptr<TStrings> FFixedPaths;
  uint32_t FMaxPacketSize{0};
  bool FSupportsStatVfsV2{false};
  uint32_t FCodePage{0};
  bool FSupportsHardlink{false};
  std::unique_ptr<TStringList> FChecksumAlgs;
  std::unique_ptr<TStringList> FChecksumSftpAlgs;

  void SendCustomReadFile(TSFTPPacket *Packet, TSFTPPacket *Response,
    uint32_t Flags);
  void CustomReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile, SSH_FXP_TYPE Type, TRemoteFile *ALinkedByFile = nullptr,
    SSH_FX_TYPE AllowStatus = -1);
  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  SSH_FX_TYPE GotStatusPacket(TSFTPPacket *Packet, SSH_FX_TYPE AllowStatus, bool DoNotForceLog);
  bool RemoteFileExists(const UnicodeString AFullPath, TRemoteFile **AFile = nullptr);
  TRemoteFile * LoadFile(TSFTPPacket *Packet,
    TRemoteFile *ALinkedByFile, const UnicodeString AFileName,
    TRemoteFileList *TempFileList = nullptr, bool Complete = true);
  void LoadFile(TRemoteFile *AFile, TSFTPPacket *Packet,
    bool Complete = true);
  UnicodeString LocalCanonify(const UnicodeString APath) const;
  UnicodeString Canonify(const UnicodeString APath);
  UnicodeString GetRealPath(const UnicodeString APath);
  UnicodeString GetRealPath(const UnicodeString APath, const UnicodeString ABaseDir);
  void ReserveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *Response);
  SSH_FX_TYPE ReceivePacket(TSFTPPacket *Packet, SSH_FXP_TYPE ExpectedType = -1,
    SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false);
  bool PeekPacket();
  void RemoveReservation(int32_t Reservation);
  void SendPacket(const TSFTPPacket *Packet);
  SSH_FX_TYPE ReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *AResponse, SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false);
  SSH_FX_TYPE SendPacketAndReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *Response, SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1);
  void UnreserveResponse(TSFTPPacket *Response);
  void LogPacket(const TSFTPPacket * Packet, TLogLineType Type);
  void TryOpenDirectory(const UnicodeString ADirectory);
  bool SupportsExtension(const UnicodeString AExtension) const;
  void ResetConnection();
  void RegisterChecksumAlg(const UnicodeString Alg, const UnicodeString SftpAlg);
  void DoDeleteFile(const UnicodeString AFileName, SSH_FXP_TYPE Type);

  void SFTPSource(const UnicodeString AFileName,
    const TRemoteFile *AFile,
    const UnicodeString TargetDir, const TCopyParamType *CopyParam, int32_t Params,
    TOpenRemoteFileParams &OpenParams,
    TOverwriteFileParams &FileParams,
    TFileOperationProgressType *OperationProgress, uint32_t Flags,
    TUploadSessionAction &Action, bool &ChildError);
  RawByteString SFTPOpenRemoteFile(const UnicodeString AFileName,
    SSH_FXF_TYPE OpenType, bool EncryptNewFiles = false, int64_t Size = -1);
  int32_t SFTPOpenRemote(void *AOpenParams, void *Param2);
  void SFTPCloseRemote(const RawByteString Handle,
    const UnicodeString AFileName, TFileOperationProgressType *OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket *Packet);
  void SFTPConfirmOverwrite(const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    const TCopyParamType *CopyParam, int32_t AParams, TFileOperationProgressType *OperationProgress,
    TOverwriteMode &OverwriteMode, const TOverwriteFileParams *FileParams);
  bool SFTPConfirmResume(const UnicodeString DestFileName, bool PartialBiggerThanSource,
    TFileOperationProgressType *OperationProgress);
  char *GetEOL() const;
  void BusyStart();
  void BusyEnd();
  uint32_t TransferBlockSize(uint32_t Overhead,
    TFileOperationProgressType *OperationProgress, uint32_t MinPacketSize = 0, uint32_t MaxPacketSize = 0) const;
  uint32_t UploadBlockSize(const RawByteString Handle,
    TFileOperationProgressType *OperationProgress) const;
  uint32_t DownloadBlockSize(
   TFileOperationProgressType *OperationProgress) const;
  int32_t PacketLength(uint8_t *LenBuf, SSH_FXP_TYPE ExpectedType) const;
  void Progress(TFileOperationProgressType * OperationProgress);
  void AddPathString(TSFTPPacket & Packet, const UnicodeString Value, bool EncryptNewFiles = false);
  void WriteLocalFile(
    const TCopyParamType * CopyParam, TStream * FileStream, TFileBuffer & BlockBuf, UnicodeString ALocalFileName,
    TFileOperationProgressType * OperationProgress);
  bool DoesFileLookLikeSymLink(TRemoteFile * File) const;
  void DoCloseRemoteIfOpened(const RawByteString & Handle);

private:
  const TSessionData *GetSessionData() const;
};

