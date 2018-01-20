
#pragma once

#include <FileSystems.h>
//---------------------------------------------------------------------------
typedef int32_t SSH_FX_TYPES;
typedef int32_t SSH_FXP_TYPES;
typedef uint32_t SSH_FILEXFER_ATTR_TYPES;
typedef uint8_t SSH_FILEXFER_TYPES;
typedef uint32_t SSH_FXF_TYPES;
typedef uint32_t ACE4_TYPES;
//---------------------------------------------------------------------------
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
  explicit __fastcall TSFTPFileSystem(TTerminal *ATerminal);
  virtual __fastcall ~TSFTPFileSystem();

  virtual void Init(void *Data /*TSecureShell* */) override;
  virtual void FileTransferProgress(int64_t /*TransferSize*/, int64_t /*Bytes*/) override {}

  virtual void Open() override;
  virtual void Close() override;
  virtual bool GetActive() const override;
  virtual void CollectUsage() override;
  virtual void Idle() override;
  virtual UnicodeString GetAbsolutePath(UnicodeString APath, bool Local) override;
  virtual UnicodeString __fastcall GetAbsolutePath(UnicodeString APath, bool Local) const override;
  virtual void __fastcall AnyCommand(const UnicodeString ACommand,
    TCaptureOutputEvent OutputEvent) override;
  virtual void __fastcall ChangeDirectory(const UnicodeString ADirectory) override;
  virtual void __fastcall CachedChangeDirectory(const UnicodeString ADirectory) override;
  virtual void __fastcall AnnounceFileListOperation() override;
  virtual void __fastcall ChangeFileProperties(const UnicodeString AFileName,
    const TRemoteFile *AFile, const TRemoteProperties *AProperties,
    TChmodSessionAction &Action) override;
  virtual bool __fastcall LoadFilesProperties(TStrings *AFileList) override;
  virtual void __fastcall CalculateFilesChecksum(const UnicodeString Alg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum) override;
  virtual void __fastcall CopyToLocal(const TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
    intptr_t AParams, TFileOperationProgressType *OperationProgress,
    TOnceDoneOperation &OnceDoneOperation) override;
  virtual void __fastcall CopyToRemote(TStrings *AFilesToCopy,
    const UnicodeString ATargetDir, const TCopyParamType *ACopyParam,
    intptr_t AParams, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation &OnceDoneOperation);
  virtual void __fastcall Source(
    TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
    const TCopyParamType *CopyParam, intptr_t AParams,
    TFileOperationProgressType *OperationProgress, uintptr_t AFlags,
    TUploadSessionAction &Action, bool &ChildError);
  virtual void __fastcall DirectorySunk(
    const UnicodeString & DestFullName, const TRemoteFile * File, const TCopyParamType * CopyParam);
  virtual void __fastcall Sink(
    const UnicodeString & FileName, const TRemoteFile * File,
    const UnicodeString & TargetDir, UnicodeString & DestFileName, int Attrs,
    const TCopyParamType * CopyParam, int Params, TFileOperationProgressType * OperationProgress,
    unsigned int Flags, TDownloadSessionAction & Action);
  virtual void __fastcall CreateDirectory(const UnicodeString ADirName);
  virtual void __fastcall CreateLink(const UnicodeString AFileName, const UnicodeString APointTo, bool Symbolic);
  virtual void __fastcall DeleteFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, intptr_t Params, TRmSessionAction &Action);
  virtual void __fastcall CustomCommandOnFile(const UnicodeString AFileName,
    const TRemoteFile *AFile, const UnicodeString ACommand, intptr_t AParams, TCaptureOutputEvent OutputEvent);
  virtual void __fastcall DoStartup();
  virtual void __fastcall HomeDirectory();
  virtual bool __fastcall IsCapable(intptr_t Capability) const override;
  virtual void __fastcall LookupUsersGroups() override;
  virtual void __fastcall ReadCurrentDirectory() override;
  virtual void __fastcall ReadDirectory(TRemoteFileList *FileList) override;
  virtual void __fastcall ReadFile(const UnicodeString AFileName,
    TRemoteFile *&AFile) override;
  virtual void __fastcall ReadSymlink(TRemoteFile *ASymlinkFile,
    TRemoteFile *&AFile) override;
  virtual void __fastcall RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual void __fastcall RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile *AFile,
    const UnicodeString ANewName) override;
  virtual TStrings * __fastcall GetFixedPaths() const override;
  virtual void __fastcall SpaceAvailable(const UnicodeString APath,
    TSpaceAvailable &ASpaceAvailable) override;
  virtual const TSessionInfo & __fastcall GetSessionInfo() const override;
  virtual const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve);
  virtual bool __fastcall TemporaryTransferFile(const UnicodeString AFileName) override;
  virtual bool __fastcall GetStoredCredentialsTried() const override;
  virtual UnicodeString __fastcall RemoteGetUserName() const override;
  virtual void __fastcall GetSupportedChecksumAlgs(TStrings *Algs) override;
  virtual void __fastcall LockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void __fastcall UnlockFile(const UnicodeString AFileName, const TRemoteFile *AFile) override;
  virtual void __fastcall UpdateFromMain(TCustomFileSystem *MainFileSystem) override;
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
  UnicodeString __fastcall GetHomeDirectory();
  SSH_FX_TYPES GotStatusPacket(TSFTPPacket *Packet, SSH_FX_TYPES AllowStatus);
  bool __fastcall RemoteFileExists(const UnicodeString AFullPath, TRemoteFile **AFile = nullptr);
  TRemoteFile * __fastcall LoadFile(TSFTPPacket *Packet,
    TRemoteFile *ALinkedByFile, const UnicodeString AFileName,
    TRemoteFileList *TempFileList = nullptr, bool Complete = true);
  void __fastcall LoadFile(TRemoteFile *AFile, TSFTPPacket *Packet,
    bool Complete = true);
  UnicodeString LocalCanonify(const UnicodeString APath) const;
  UnicodeString Canonify(UnicodeString APath);
  UnicodeString GetRealPath(const UnicodeString APath);
  UnicodeString GetRealPath(const UnicodeString APath, const UnicodeString ABaseDir);
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
  void TryOpenDirectory(const UnicodeString ADirectory);
  bool SupportsExtension(const UnicodeString AExtension) const;
  void ResetConnection();
  void DoCalculateFilesChecksum(
    const UnicodeString Alg, const UnicodeString SftpAlg,
    TStrings *AFileList, TStrings *Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType *OperationProgress, bool FirstLevel);
  void __fastcall RegisterChecksumAlg(const UnicodeString Alg, const UnicodeString SftpAlg);
  void __fastcall DoDeleteFile(const UnicodeString AFileName, SSH_FXP_TYPES Type);

  void __fastcall SFTPSource(const UnicodeString AFileName,
    const TRemoteFile *AFile,
    const UnicodeString TargetDir, const TCopyParamType *CopyParam, intptr_t Params,
    TOpenRemoteFileParams &OpenParams,
    TOverwriteFileParams &FileParams,
    TFileOperationProgressType *OperationProgress, uintptr_t Flags,
    TUploadSessionAction &Action, bool &ChildError);
  RawByteString __fastcall SFTPOpenRemoteFile(const UnicodeString AFileName,
    SSH_FXF_TYPES OpenType, int64_t Size = -1);
  intptr_t __fastcall SFTPOpenRemote(void *AOpenParams, void *Param2);
  void __fastcall SFTPCloseRemote(const RawByteString Handle,
    const UnicodeString AFileName, TFileOperationProgressType *OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket *Packet);
  void __fastcall SFTPConfirmOverwrite(const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
    const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
    const TOverwriteFileParams *FileParams,
    TOverwriteMode &OverwriteMode);
  bool __fastcall SFTPConfirmResume(const UnicodeString DestFileName, bool PartialBiggerThanSource,
    TFileOperationProgressType *OperationProgress);
  char *GetEOL() const;
  inline void BusyStart();
  inline void BusyEnd();
  uint32_t TransferBlockSize(uint32_t Overhead,
    TFileOperationProgressType *OperationProgress,
    uint32_t MinPacketSize = 0,
    uint32_t MaxPacketSize = 0) const;
  uint32_t UploadBlockSize(const RawByteString Handle,
    TFileOperationProgressType *OperationProgress) const;
  uint32_t DownloadBlockSize(
    TFileOperationProgressType *OperationProgress) const;
  intptr_t PacketLength(uint8_t *LenBuf, SSH_FXP_TYPES ExpectedType) const;
  void Progress(TFileOperationProgressType *OperationProgress);

private:
  const TSessionData *GetSessionData() const;
};
//---------------------------------------------------------------------------
