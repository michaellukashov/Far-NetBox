
#pragma once

#include <FileSystems.h>
//---------------------------------------------------------------------------
using SSH_FX_TYPE = int32_t;
using SSH_FXP_TYPE = int32_t;
using SSH_FILEXFER_ATTR_TYPE = uint32_t;
using SSH_FILEXFER_TYPE = uint8_t;
using SSH_FXF_TYPE = uint32_t;
using ACE4_TYPE = uint32_t;
//---------------------------------------------------------------------------
class TSFTPPacket;
struct TOverwriteFileParams;
struct TSFTPSupport;
class TSecureShell;
class TEncryption;
//---------------------------------------------------------------------------
__removed enum TSFTPOverwriteMode { omOverwrite, omAppend, omResume };
__removed extern const int SFTPMaxVersion;
//---------------------------------------------------------------------------
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
  explicit TSFTPFileSystem(TTerminal *ATerminal) noexcept;
  virtual ~TSFTPFileSystem() noexcept;

  void Init(void *Data /*TSecureShell* */) override;
  void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  void Open() override;
  void Close() override;
  bool GetActive() const override;
  void CollectUsage() override;
  void Idle() override;
  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  void AnyCommand(UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  void ChangeDirectory(UnicodeString ADirectory) override;
  void CachedChangeDirectory(UnicodeString ADirectory) override;
  void AnnounceFileListOperation() override;
  void ChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *AProperties,
    TChmodSessionAction &Action) override;
  bool LoadFilesProperties(TStrings *AFileList) override;
  void CalculateFilesChecksum(UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  void CopyToLocal(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void CopyToRemote(TStrings *AFilesToCopy,
    UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  void Source(
    TLocalFileHandle &AHandle, UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  void DirectorySunk(
    UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType * CopyParam) override;
  void Sink(
    UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t AAttrs,
    const TCopyParamType* CopyParam, intptr_t AParams, TFileOperationProgressType* OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction& Action) override;
  void RemoteCreateDirectory(UnicodeString ADirName, bool Encrypt) override;
  void RemoteCreateLink(UnicodeString AFileName, UnicodeString APointTo, bool Symbolic) override;
  void RemoteDeleteFile(UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action) override;
  void CustomCommandOnFile(UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  void DoStartup() override;
  void HomeDirectory() override;
  UnicodeString GetHomeDirectory() override;
  bool IsCapable(intptr_t Capability) const override;
  void LookupUsersGroups() override;
  void ReadCurrentDirectory() override;
  void ReadDirectory(TRemoteFileList *FileList) override;
  void ReadFile(UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  void ReadSymlink(TRemoteFile *ASymlinkFile,
    TRemoteFile *&AFile) override;
  void RemoteRenameFile(UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ANewName) override;
  void RemoteCopyFile(UnicodeString AFileName, const TRemoteFile *AFile,
    UnicodeString ANewName) override;
  TStrings * GetFixedPaths() const override;
  void SpaceAvailable(UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  const TSessionInfo & GetSessionInfo() const override;
  const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) override;
  bool TemporaryTransferFile(UnicodeString AFileName) override;
  bool GetStoredCredentialsTried() const override;
  UnicodeString RemoteGetUserName() const override;
  void GetSupportedChecksumAlgs(TStrings *Algs) override;
  void LockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  void ClearCaches() override;

  UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
protected:
  TSecureShell *FSecureShell{nullptr};
  TFileSystemInfo FFileSystemInfo{};
  bool FFileSystemInfoValid{false};
  intptr_t FVersion{0};
  UnicodeString FCurrentDirectory;
  UnicodeString FDirectoryToChangeTo;
  UnicodeString FHomeDirectory;
  AnsiString FEOL;
  std::unique_ptr<TList> FPacketReservations;
  rde::vector<uintptr_t> FPacketNumbers;
  SSH_FXP_TYPE FPreviousLoggedPacket{0};
  intptr_t FNotLoggedPackets{0};
  intptr_t FBusy{0};
  void *FBusyToken{nullptr};
  bool FAvoidBusy{false};
  std::unique_ptr<TStrings> FExtensions;
  std::unique_ptr<TSFTPSupport> FSupport;
  TAutoSwitch FUtfStrings{asAuto};
  bool FUtfDisablingAnnounced{false};
  bool FSignedTS{false};
  TStrings *FFixedPaths{nullptr};
  uint32_t FMaxPacketSize{0};
  bool FSupportsStatVfsV2{false};
  uintptr_t FCodePage{0};
  bool FSupportsHardlink{false};
  std::unique_ptr<TStringList> FChecksumAlgs;
  std::unique_ptr<TStringList> FChecksumSftpAlgs;

  void SendCustomReadFile(TSFTPPacket *Packet, TSFTPPacket *Response,
    uint32_t Flags);
  void CustomReadFile(UnicodeString AFileName,
    TRemoteFile *&AFile, SSH_FXP_TYPE Type, TRemoteFile *ALinkedByFile = nullptr,
    SSH_FX_TYPE AllowStatus = -1);
  UnicodeString RemoteGetCurrentDirectory() const override;

  SSH_FX_TYPE GotStatusPacket(TSFTPPacket *Packet, SSH_FX_TYPE AllowStatus);
  bool RemoteFileExists(UnicodeString AFullPath, TRemoteFile **AFile = nullptr);
  TRemoteFile * LoadFile(TSFTPPacket *Packet,
    TRemoteFile *ALinkedByFile, UnicodeString AFileName,
    TRemoteFileList *TempFileList = nullptr, bool Complete = true);
  void LoadFile(TRemoteFile *AFile, TSFTPPacket *Packet,
    bool Complete = true);
  UnicodeString LocalCanonify(UnicodeString APath) const;
  UnicodeString Canonify(UnicodeString APath);
  UnicodeString GetRealPath(UnicodeString APath);
  UnicodeString GetRealPath(UnicodeString APath, UnicodeString ABaseDir);
  void ReserveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *Response);
  SSH_FX_TYPE ReceivePacket(TSFTPPacket *Packet, SSH_FXP_TYPE ExpectedType = -1,
    SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false);
  bool PeekPacket();
  void RemoveReservation(intptr_t Reservation);
  void SendPacket(const TSFTPPacket *Packet);
  SSH_FX_TYPE ReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *AResponse, SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false);
  SSH_FX_TYPE SendPacketAndReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *Response, SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1);
  void UnreserveResponse(TSFTPPacket *Response);
  void TryOpenDirectory(UnicodeString ADirectory);
  bool SupportsExtension(UnicodeString AExtension) const;
  void ResetConnection();
  void DoCalculateFilesChecksum(
    UnicodeString Alg, UnicodeString SftpAlg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType *OperationProgress, bool FirstLevel);
  void RegisterChecksumAlg(UnicodeString Alg, UnicodeString SftpAlg);
  void DoDeleteFile(UnicodeString AFileName, SSH_FXP_TYPE Type);

  void SFTPSource(UnicodeString AFileName,
    const TRemoteFile *AFile,
    UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TOpenRemoteFileParams &OpenParams,
    TOverwriteFileParams &FileParams,
    TFileOperationProgressType *OperationProgress, uintptr_t Flags,
    TUploadSessionAction &Action, bool &ChildError);
  RawByteString SFTPOpenRemoteFile(UnicodeString AFileName,
    SSH_FXF_TYPE OpenType, bool EncryptNewFiles = false, int64_t Size = -1);
  intptr_t SFTPOpenRemote(void *AOpenParams, void *Param2);
  void SFTPCloseRemote(const RawByteString Handle,
    UnicodeString AFileName, TFileOperationProgressType *OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket *Packet);
  void SFTPConfirmOverwrite(UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams,
    TOverwriteMode &OverwriteMode);
  bool SFTPConfirmResume(UnicodeString DestFileName, bool PartialBiggerThanSource,
    TFileOperationProgressType *OperationProgress);
  char *GetEOL() const;
  void BusyStart();
  void BusyEnd();
  uint32_t TransferBlockSize(uint32_t Overhead,
    TFileOperationProgressType *OperationProgress,
    uint32_t MinPacketSize = 0,
    uint32_t MaxPacketSize = 0) const;
  uint32_t UploadBlockSize(const RawByteString Handle,
    TFileOperationProgressType *OperationProgress) const;
  uint32_t DownloadBlockSize(
    TFileOperationProgressType *OperationProgress) const;
  void AddPathString(TSFTPPacket & Packet, UnicodeString Value, bool EncryptNewFiles = false);
  void WriteLocalFile(
    TStream * FileStream, TFileBuffer & BlockBuf, UnicodeString ALocalFileName,
    TFileOperationProgressType * OperationProgress);
  intptr_t PacketLength(uint8_t *LenBuf, SSH_FXP_TYPE ExpectedType) const;
  void Progress(TFileOperationProgressType *OperationProgress);

private:
  const TSessionData *GetSessionData() const;
};
//---------------------------------------------------------------------------
