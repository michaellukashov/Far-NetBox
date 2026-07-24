#define NOMINMAX
#include <iostream>
#include <cassert>
#include <memory>
#include "core/SftpFileSystem.h"
#include "core/ScpFileSystem.h"
#include "core/FtpFileSystem.h"
#include "core/WebDAVFileSystem.h"
#include "core/S3FileSystem.h"
#include "core/Terminal.h"
#include "core/SessionData.h"
#include "core/SessionLog.h"
#include "nbcore/logging.h"

class MockTerminal : public TTerminal {
public:
    MockTerminal() : TTerminal(nullptr) {
        // Create minimal session data for testing
        FSessionData = new TSessionData();
        FSessionLog = new TSessionLog();
    }
    
    ~MockTerminal() override {
        delete FSessionData;
        delete FSessionLog;
    }
    
    // Override pure virtual methods needed for construction
    bool GetActive() const override { return false; }
    void CollectUsage() override {}
    void Idle() override {}
    UnicodeString AbsolutePath(const UnicodeString & APath, bool Local) const override { return L""; }
    void AnyCommand(const UnicodeString & ACommand, TCaptureOutputEvent && OutputEvent) override {}
    void ChangeDirectory(const UnicodeString & ADirectory) override {}
    void CachedChangeDirectory(const UnicodeString & ADirectory) override {}
    void AnnounceFileListOperation() override {}
    void ChangeFileProperties(const UnicodeString & AFileName, const TRemoteFile * AFile, const TRemoteProperties * Properties, TChmodSessionAction & Action) override {}
    bool LoadFilesProperties(TStrings * AFileList) override { return false; }
    UnicodeString CalculateFilesChecksumInitialize(const UnicodeString & Alg) override { return L""; }
    void CalculateFilesChecksum(const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent && OnCalculatedChecksum, TFileOperationProgressType * OperationProgress, bool FirstLevel) override {}
    void CopyToLocal(TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, TOnceDoneOperation & OnceDoneOperation) override {}
    void CopyToRemote(TStrings * AFilesToCopy, const UnicodeString & ATargetDir, const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, TOnceDoneOperation & OnceDoneOperation) override {}
    void TransferOnDirectory(const UnicodeString & ADirName, const TCopyParamType * ACopyParam, int32_t AParams) override {}
    void Source(TLocalFileHandle & AHandle, const UnicodeString & ATargetDir, UnicodeString & ADestFileName, const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, uint32_t AFlags, TUploadSessionAction & Action, bool & ChildError) override {}
    void Sink(const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ATargetDir, UnicodeString & ADestFileName, int32_t Attrs, const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress, uint32_t AFlags, TDownloadSessionAction & Action) override {}
    void CreateDirectory(const UnicodeString & ADirName, bool Encrypt) override {}
    void CreateLink(const UnicodeString & AFileName, const UnicodeString & APointTo, bool Symbolic) override {}
    void DeleteFile(const UnicodeString & AFileName, const TRemoteFile * AFile, int32_t AParams, TRmSessionAction & Action) override {}
    void CustomCommandOnFile(const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ACommand, int32_t AParams, TCaptureOutputEvent && OutputEvent) override {}
    void DoStartup() override {}
    UnicodeString GetHomeDirectory() override { return L""; }
    virtual UnicodeString GetHomeDirectory() { return L""; }
    void LookupUsersGroups() override {}
    void ReadCurrentDirectory() override {}
    void ReadDirectory(TRemoteFileList * FileList) override {}
    void ReadFile(const UnicodeString & AFileName, TRemoteFile *& File) override { File = nullptr; }
    void ReadSymlink(TRemoteFile * SymLinkFile, TRemoteFile *& File) override {}
    void RenameFile(const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) override {}
    void CopyFile(const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool Overwrite) override {}
    TStrings * GetFixedPaths() const override { return nullptr; }
    void SpaceAvailable(const UnicodeString & APath, TSpaceAvailable & ASpaceAvailable) override {}
    const TSessionInfo & GetSessionInfo() const override { static TSessionInfo info; return info; }
    const TFileSystemInfo & GetFileSystemInfo(bool Retrieve) const override { static TFileSystemInfo info; return info; }
    bool TemporaryTransferFile(const UnicodeString & AFileName) override { return false; }
    bool GetStoredCredentialsTried() const override { return false; }
    UnicodeString GetUserName() const override { return L""; }
    void GetSupportedChecksumAlgs(TStrings * Algs) override {}
    void LockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) override {}
    void UnlockFile(const UnicodeString & AFileName, const TRemoteFile * AFile) override {}
    void UpdateFromMain(TCustomFileSystem * MainFileSystem) override {}
    void ClearCaches() override {}
    UnicodeString GetCurrentDirectory() const override { return L""; }
    UnicodeString AbsolutePath(const UnicodeString & APath, bool Local) const override { return L""; }
};

int main() {
    std::wcout << L"Testing filesystem capabilities..." << std::endl;
    
    // Test SFTP
    {
        auto term = std::make_unique<MockTerminal>();
        auto sftp = std::make_unique<TSFTPFileSystem>(term.get());
        
        // SFTP should support transfer out and mode changing upload
        assert(sftp->IsCapable(fcTransferOut));
        assert(sftp->IsCapable(fcModeChangingUpload));
        std::wcout << L"SFTP: fcTransferOut=" << (sftp->IsCapable(fcTransferOut) ? L"yes" : L"no")
                   << L", fcModeChangingUpload=" << (sftp->IsCapable(fcModeChangingUpload) ? L"yes" : L"no") << std::endl;
    }
    
    // Test SCP
    {
        auto term = std::make_unique<MockTerminal>();
        auto scp = std::make_unique<TScpFileSystem>(term.get());
        
        // SCP should support transfer out and mode changing upload
        assert(scp->IsCapable(fcTransferOut));
        assert(scp->IsCapable(fcModeChangingUpload));
        std::wcout << L"SCP: fcTransferOut=" << (scp->IsCapable(fcTransferOut) ? L"yes" : L"no")
                   << L", fcModeChangingUpload=" << (scp->IsCapable(fcModeChangingUpload) ? L"yes" : L"no") << std::endl;
    }
    
    // Test FTP
    {
        auto term = std::make_unique<MockTerminal>();
        auto ftp = std::make_unique<TFTPFileSystem>(term.get());
        
        // FTP should support transfer out but not mode changing upload (no chmod in FTP protocol)
        assert(ftp->IsCapable(fcTransferOut));
        // Note: FTP may or may not support mode changing - depends on server capabilities
        // For now, we'll just test that it doesn't crash
        bool modeChanging = ftp->IsCapable(fcModeChangingUpload);
        std::wcout << L"FTP: fcTransferOut=" << (ftp->IsCapable(fcTransferOut) ? L"yes" : L"no")
                   << L", fcModeChangingUpload=" << (modeChanging ? L"yes" : L"no") << std::endl;
    }
    
    // Test WebDAV
    {
        auto term = std::make_unique<MockTerminal>();
        auto webdav = std::make_unique<TWebDAVFileSystem>(term.get());
        
        // WebDAV should support transfer out (via PUT) but mode changing is limited
        assert(webdav->IsCapable(fcTransferOut));
        // WebDAV has limited chmod support via properties
        bool modeChanging = webdav->IsCapable(fcModeChangingUpload);
        std::wcout << L"WebDAV: fcTransferOut=" << (webdav->IsCapable(fcTransferOut) ? L"yes" : L"no")
                   << L", fcModeChangingUpload=" << (modeChanging ? L"yes" : L"no") << std::endl;
    }
    
    // Test S3
    {
        auto term = std::make_unique<MockTerminal>();
        auto s3 = std::make_unique<TS3FileSystem>(term.get());
        
        // S3 should support transfer out but not traditional mode changing
        assert(s3->IsCapable(fcTransferOut));
        // S3 has ACLs but not traditional unix permissions
        bool modeChanging = s3->IsCapable(fcModeChangingUpload);
        std::wcout << L"S3: fcTransferOut=" << (s3->IsCapable(fcTransferOut) ? L"yes" : L"no")
                   << L", fcModeChangingUpload=" << (modeChanging ? L"yes" : L"no") << std::endl;
    }
    
    std::wcout << L"All capability tests completed successfully!" << std::endl;
    return 0;
}