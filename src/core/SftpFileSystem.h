
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
class NB_CORE_EXPORT TSFTPFileSystem : public TCustomFileSystem
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
  explicit TSFTPFileSystem(TTerminal *ATerminal);
  virtual ~TSFTPFileSystem();

  virtual void Init(void *Data /*TSecureShell* */) override;
  virtual void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(const UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *AProperties,
    TChmodSessionAction &Action) override;
  virtual bool LoadFilesProperties(TStrings *AFileList) override;
  virtual void CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError) override;
  virtual void DirectorySunk(
    const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType * CopyParam) override;
  virtual void Sink(
    const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ATargetDir, UnicodeString &ADestFileName, intptr_t AAttrs,
    const TCopyParamType* CopyParam, intptr_t AParams, TFileOperationProgressType* OperationProgress,
    uintptr_t AFlags, TDownloadSessionAction& Action) override;
  virtual void RemoteCreateDirectory(const UnicodeString ADirName, bool Encrypt) override;
  virtual void RemoteCreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic) override;
  virtual void RemoteDeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action) override;
  virtual UnicodeString GetHomeDirectory();
  virtual void CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent) override;
  virtual void DoStartup() override;
  virtual void HomeDirectory() override;
  virtual bool IsCapable(intptr_t Capability) const override;
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
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool GetStoredCredentialsTried() const override;
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  virtual void ClearCaches() override;

protected:
  TSecureShell *FSecureShell{nullptr};
  TFileSystemInfo FFileSystemInfo{};
  bool FFileSystemInfoValid{false};
  intptr_t FVersion{0};
  UnicodeString FCurrentDirectory;
  UnicodeString FDirectoryToChangeTo;
  UnicodeString FHomeDirectory;
  AnsiString FEOL;
  TList *FPacketReservations{nullptr};
  rde::vector<uintptr_t> FPacketNumbers;
  SSH_FXP_TYPE FPreviousLoggedPacket{0};
  int FNotLoggedPackets{0};
  int FBusy{0};
  void *FBusyToken{nullptr};
  bool FAvoidBusy{false};
  TStrings *FExtensions{nullptr};
  TSFTPSupport *FSupport{nullptr};
  TAutoSwitch FUtfStrings{};
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
  void CustomReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile, SSH_FXP_TYPE Type, TRemoteFile *ALinkedByFile = nullptr,
    SSH_FX_TYPE AllowStatus = -1);
  virtual UnicodeString RemoteGetCurrentDirectory() const override;

  SSH_FX_TYPE GotStatusPacket(TSFTPPacket *Packet, SSH_FX_TYPE AllowStatus);
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
  void RemoveReservation(intptr_t Reservation);
  void SendPacket(const TSFTPPacket *Packet);
  SSH_FX_TYPE ReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *AResponse, SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false);
  SSH_FX_TYPE SendPacketAndReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *Response, SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1);
  void UnreserveResponse(TSFTPPacket *Response);
  void TryOpenDirectory(const UnicodeString ADirectory);
  bool SupportsExtension(const UnicodeString AExtension) const;
  void ResetConnection();
  void DoCalculateFilesChecksum(
    const UnicodeString Alg, const UnicodeString SftpAlg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType *OperationProgress, bool FirstLevel);
  void RegisterChecksumAlg(const UnicodeString Alg, const UnicodeString SftpAlg);
  void DoDeleteFile(const UnicodeString AFileName, SSH_FXP_TYPE Type);

  void SFTPSource(const UnicodeString AFileName,
    const TRemoteFile *AFile,
    const UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TOpenRemoteFileParams &OpenParams,
    TOverwriteFileParams &FileParams,
    TFileOperationProgressType *OperationProgress, uintptr_t Flags,
    TUploadSessionAction &Action, bool &ChildError);
  RawByteString SFTPOpenRemoteFile(const UnicodeString AFileName,
    SSH_FXF_TYPE OpenType, bool EncryptNewFiles = false, int64_t Size = -1);
  intptr_t SFTPOpenRemote(void *AOpenParams, void *Param2);
  void SFTPCloseRemote(const RawByteString Handle,
    const UnicodeString AFileName, TFileOperationProgressType *OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket *Packet);
  void SFTPConfirmOverwrite(const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams,
    TOverwriteMode &OverwriteMode);
  bool SFTPConfirmResume(const UnicodeString DestFileName, bool PartialBiggerThanSource,
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
  void AddPathString(TSFTPPacket & Packet, const UnicodeString & Value, bool EncryptNewFiles = false);
  void WriteLocalFile(
    TStream * FileStream, TFileBuffer & BlockBuf, const UnicodeString ALocalFileName,
    TFileOperationProgressType * OperationProgress);
  intptr_t PacketLength(uint8_t *LenBuf, SSH_FXP_TYPE ExpectedType) const;
  void Progress(TFileOperationProgressType *OperationProgress);

private:
  const TSessionData *GetSessionData() const;
};
//---------------------------------------------------------------------------
