
#pragma once

#include <FileSystems.h>

typedef int32_t SSH_FX_TYPES;
typedef int32_t SSH_FXP_TYPES;
typedef uint32_t SSH_FILEXFER_ATTR_TYPES;
typedef uint8_t SSH_FILEXFER_TYPES;
typedef uint32_t SSH_FXF_TYPES;
typedef uint32_t ACE4_TYPES;

class TSFTPPacket;
struct TOverwriteFileParams;
struct TSFTPSupport;
class TSecureShell;
//---------------------------------------------------------------------------
__removed enum TSFTPOverwriteMode { omOverwrite, omAppend, omResume };
__removed extern const int SFTPMaxVersion;
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
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSFTPFileSystem); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPFileSystem) || TCustomFileSystem::is(Kind); }
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
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) const override;
  virtual void AnyCommand(UnicodeString Command,
    TCaptureOutputEvent OutputEvent) override;
  virtual void ChangeDirectory(UnicodeString Directory) override;
  virtual void CachedChangeDirectory(UnicodeString Directory) override;
  virtual void AnnounceFileListOperation() override;
  virtual void ChangeFileProperties(UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *AProperties,
    TChmodSessionAction &Action) override;
  virtual bool LoadFilesProperties(TStrings *AFileList) override;
  virtual void CalculateFilesChecksum(UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void CopyToLocal(const TStrings *AFilesToCopy,
    UnicodeString TargetDir, const TCopyParamType *CopyParam,
    intptr_t Params, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void __fastcall Source(
    TLocalFileHandle & Handle, const UnicodeString & TargetDir, UnicodeString & DestFileName,
    const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TUploadSessionAction & Action, bool & ChildError);
  virtual void __fastcall DirectorySunk(
    const UnicodeString & DestFullName, const TRemoteFile * File, const TCopyParamType * CopyParam);
  virtual void __fastcall Sink(
    const UnicodeString & FileName, const TRemoteFile * File,
    const UnicodeString & TargetDir, UnicodeString & DestFileName, int Attrs,
    const TCopyParamType * CopyParam, int Params, TFileOperationProgressType * OperationProgress,
    unsigned int Flags, TDownloadSessionAction & Action);
  virtual bool IsCapable(intptr_t Capability) const override;
  virtual void LookupUsersGroups() override;
  virtual void ReadCurrentDirectory() override;
  virtual void ReadDirectory(TRemoteFileList *FileList) override;
  virtual void ReadFile(UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void ReadSymlink(TRemoteFile *SymlinkFile,
    TRemoteFile *&AFile) override;
  virtual void RemoteRenameFile(UnicodeString AFileName,
    UnicodeString ANewName) override;
  virtual void RemoteCopyFile(UnicodeString AFileName,
    UnicodeString ANewName) override;
  virtual TStrings *GetFixedPaths() const override;
  virtual void SpaceAvailable(UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo &GetSessionInfo() const override;
  virtual void __fastcall RenameFile(const UnicodeString FileName, const TRemoteFile * File,
  virtual bool TemporaryTransferFile(UnicodeString AFileName) override;
  virtual void __fastcall CopyFile(const UnicodeString FileName, const TRemoteFile * File,
  virtual UnicodeString RemoteGetUserName() const override;
  virtual void GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void LockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UnlockFile(UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
  virtual void __fastcall ClearCaches();

protected:
  TSecureShell *FSecureShell;
  TFileSystemInfo FFileSystemInfo;
  bool FFileSystemInfoValid;
  intptr_t FVersion;
  UnicodeString FCurrentDirectory;
  UnicodeString FDirectoryToChangeTo;
  UnicodeString FHomeDirectory;
  AnsiString FEOL;
  TList *FPacketReservations;
  rde::vector<uintptr_t> FPacketNumbers;
  SSH_FXP_TYPES FPreviousLoggedPacket;
  int FNotLoggedPackets;
  int FBusy;
  void *FBusyToken;
  bool FAvoidBusy;
  TStrings *FExtensions;
  TSFTPSupport *FSupport;
  TAutoSwitch FUtfStrings;
  bool FUtfDisablingAnnounced;
  bool FSignedTS;
  TStrings *FFixedPaths;
  uint32_t FMaxPacketSize;
  bool FSupportsStatVfsV2;
  uintptr_t FCodePage;
  bool FSupportsHardlink;
  std::unique_ptr<TStringList> FChecksumAlgs;
  std::unique_ptr<TStringList> FChecksumSftpAlgs;

  void SendCustomReadFile(TSFTPPacket *Packet, TSFTPPacket *Response,
    uint32_t Flags);
  void CustomReadFile(UnicodeString AFileName,
    TRemoteFile *&AFile, SSH_FXP_TYPES Type, TRemoteFile *ALinkedByFile = nullptr,
    SSH_FX_TYPES AllowStatus = -1);
  virtual UnicodeString RemoteGetCurrentDirectory() const override;
  UnicodeString GetHomeDirectory();
  SSH_FX_TYPES GotStatusPacket(TSFTPPacket *Packet, SSH_FX_TYPES AllowStatus);
  bool RemoteFileExists(UnicodeString FullPath, TRemoteFile **AFile = nullptr);
  TRemoteFile *LoadFile(TSFTPPacket *Packet,
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
  SSH_FX_TYPES ReceivePacket(TSFTPPacket *Packet, SSH_FXP_TYPES ExpectedType = -1,
    SSH_FX_TYPES AllowStatus = -1, bool TryOnly = false);
  bool PeekPacket();
  void RemoveReservation(intptr_t Reservation);
  void SendPacket(const TSFTPPacket *Packet);
  SSH_FX_TYPES ReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *AResponse, SSH_FXP_TYPES ExpectedType = -1, SSH_FX_TYPES AllowStatus = -1, bool TryOnly = false);
  SSH_FX_TYPES SendPacketAndReceiveResponse(const TSFTPPacket *Packet,
    TSFTPPacket *Response, SSH_FXP_TYPES ExpectedType = -1, SSH_FX_TYPES AllowStatus = -1);
  void UnreserveResponse(TSFTPPacket *Response);
  void TryOpenDirectory(UnicodeString Directory);
  bool SupportsExtension(UnicodeString Extension) const;
  void ResetConnection();
  void DoCalculateFilesChecksum(
    UnicodeString Alg, UnicodeString SftpAlg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType *OperationProgress, bool FirstLevel);
  void RegisterChecksumAlg(UnicodeString Alg, UnicodeString SftpAlg);
  void DoDeleteFile(UnicodeString AFileName, SSH_FXP_TYPES Type);

    TFileOperationProgressType *OperationProgress, uintptr_t Flags);
  void SFTPSource(UnicodeString AFileName,
    const TRemoteFile *AFile,
    UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TOpenRemoteFileParams &OpenParams,
    TOverwriteFileParams &FileParams,
    TFileOperationProgressType *OperationProgress, uintptr_t Flags,
    TUploadSessionAction &Action, bool &ChildError);
  RawByteString SFTPOpenRemoteFile(UnicodeString AFileName,
    SSH_FXF_TYPES OpenType, int64_t Size = -1);
  intptr_t SFTPOpenRemote(void *AOpenParams, void *Param2);
  void SFTPCloseRemote(RawByteString Handle,
    UnicodeString AFileName, TFileOperationProgressType *OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket *Packet);
  void SFTPConfirmOverwrite(UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams,
    TOverwriteMode &OverwriteMode);
  bool SFTPConfirmResume(UnicodeString DestFileName, bool PartialBiggerThanSource,
    TFileOperationProgressType *OperationProgress);
  char *GetEOL() const;
  inline void BusyStart();
  inline void BusyEnd();
  uint32_t TransferBlockSize(uint32_t Overhead,
    TFileOperationProgressType *OperationProgress,
    uint32_t MinPacketSize = 0,
    uint32_t MaxPacketSize = 0) const;
  uint32_t UploadBlockSize(RawByteString Handle,
    TFileOperationProgressType *OperationProgress) const;
  uint32_t DownloadBlockSize(
    TFileOperationProgressType *OperationProgress) const;
  intptr_t PacketLength(uint8_t *LenBuf, SSH_FXP_TYPES ExpectedType) const;
  void Progress(TFileOperationProgressType *OperationProgress);

private:
  const TSessionData *GetSessionData() const;
};

