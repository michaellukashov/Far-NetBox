//---------------------------------------------------------------------------
#ifndef SftpFileSystemH
#define SftpFileSystemH

#include <FileSystems.h>
//---------------------------------------------------------------------------
class TSFTPPacket;
struct TOverwriteFileParams;
struct TSFTPSupport;
class TSecureShell;
//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
enum TSFTPOverwriteMode { omOverwrite, omAppend, omResume };
#endif
//---------------------------------------------------------------------------
class TSFTPFileSystem : public TCustomFileSystem
{
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
  virtual void FileTransferProgress(__int64 TransferSize, __int64 Bytes) {}

  virtual void Open();
  virtual void Close();
  virtual bool GetActive();
  virtual void Idle();
  virtual UnicodeString AbsolutePath(const UnicodeString & Path, bool Local);
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * FileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * FileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(TStrings * FilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CreateDirectory(const UnicodeString & DirName);
  virtual void CreateLink(const UnicodeString & FileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void DeleteFile(const UnicodeString & FileName,
    const TRemoteFile * File, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & FileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RenameFile(const UnicodeString & FileName,
    const UnicodeString & NewName);
  virtual void CopyFile(const UnicodeString & FileName,
    const UnicodeString & NewName);
  virtual UnicodeString FileUrl(const UnicodeString & FileName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & Path,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo();
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & FileName);
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
  char FPreviousLoggedPacket;
  int FNotLoggedPackets;
  int FBusy;
  bool FAvoidBusy;
  TStrings * FExtensions;
  TSFTPSupport * FSupport;
  bool FUtfStrings;
  bool FUtfNever;
  bool FSignedTS;
  bool FOpenSSH;
  TStrings * FFixedPaths;
  uintptr_t FMaxPacketSize;

  void SendCustomReadFile(TSFTPPacket * Packet, TSFTPPacket * Response,
    uintptr_t Flags);
  void CustomReadFile(const UnicodeString & FileName,
    TRemoteFile *& File, unsigned char Type, TRemoteFile * ALinkedByFile = NULL,
    int AllowStatus = -1);
  virtual UnicodeString GetCurrentDirectory();
  UnicodeString GetHomeDirectory();
  uintptr_t GotStatusPacket(TSFTPPacket * Packet, int AllowStatus);
  bool IsAbsolutePath(const UnicodeString & Path);
  bool RemoteFileExists(const UnicodeString & FullPath, TRemoteFile ** File = NULL);
  TRemoteFile * LoadFile(TSFTPPacket * Packet,
    TRemoteFile * ALinkedByFile, const UnicodeString & FileName,
    TRemoteFileList * TempFileList = NULL, bool Complete = true);
  void LoadFile(TRemoteFile * File, TSFTPPacket * Packet,
    bool Complete = true);
  UnicodeString LocalCanonify(const UnicodeString & Path);
  UnicodeString Canonify(const UnicodeString & Path);
  UnicodeString RealPath(const UnicodeString & Path);
  UnicodeString RealPath(const UnicodeString & Path, const UnicodeString & BaseDir);
  void ReserveResponse(const TSFTPPacket * Packet,
    TSFTPPacket * Response);
  uintptr_t ReceivePacket(TSFTPPacket * Packet, int ExpectedType = -1,
    int AllowStatus = -1);
  bool PeekPacket();
  void RemoveReservation(intptr_t Reservation);
  void SendPacket(const TSFTPPacket * Packet);
  uintptr_t ReceiveResponse(const TSFTPPacket * Packet,
    TSFTPPacket * Response, int ExpectedType = -1, int AllowStatus = -1);
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
  void DoDeleteFile(const UnicodeString & FileName, unsigned char Type);

  void SFTPSourceRobust(const UnicodeString & FileName,
    const TRemoteFile * File,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SFTPSource(const UnicodeString & FileName,
    const TRemoteFile * File,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TOpenRemoteFileParams & OpenParams,
    TOverwriteFileParams & FileParams,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TUploadSessionAction & Action, bool & ChildError);
  RawByteString SFTPOpenRemoteFile(const UnicodeString & FileName,
    uintptr_t OpenType, __int64 Size = -1);
  int SFTPOpenRemote(void * AOpenParams, void * Param2);
  void SFTPCloseRemote(const RawByteString & Handle,
    const UnicodeString & FileName, TFileOperationProgressType * OperationProgress,
    bool TransferFinished, bool Request, TSFTPPacket * Packet);
  void SFTPDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, uintptr_t LocalFileAttrs, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SFTPConfirmOverwrite(UnicodeString & FileName,
    const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOverwriteMode & Mode, const TOverwriteFileParams * FileParams);
  bool SFTPConfirmResume(const UnicodeString & DestFileName, bool PartialBiggerThanSource,
    TFileOperationProgressType * OperationProgress);
  void SFTPSinkRobust(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags);
  void SFTPSink(const UnicodeString & FileName,
    const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, uintptr_t Flags,
    TDownloadSessionAction & Action, bool & ChildError);
  void SFTPSinkFile(const UnicodeString & FileName,
    const TRemoteFile * File, void * Param);
  char * GetEOL() const;
  inline void BusyStart();
  inline void BusyEnd();
  uintptr_t TransferBlockSize(uintptr_t Overhead,
    TFileOperationProgressType * OperationProgress,
    uintptr_t MinPacketSize = 0,
    uintptr_t MaxPacketSize = 0);
  uintptr_t UploadBlockSize(const RawByteString & Handle,
    TFileOperationProgressType * OperationProgress);
  uintptr_t DownloadBlockSize(
    TFileOperationProgressType * OperationProgress);
  intptr_t PacketLength(unsigned char * LenBuf, intptr_t ExpectedType);

private:
  const TSessionData * GetSessionData() const;
};
//---------------------------------------------------------------------------
#endif // SftpFileSystemH
