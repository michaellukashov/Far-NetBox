//---------------------------------------------------------------------------
#ifndef FileZillaIntfH
#define FileZillaIntfH
//---------------------------------------------------------------------------
#include <time.h>
#include <FileZillaOpt.h>
//---------------------------------------------------------------------------
class CFileZillaApi;
class TFileZillaIntern;
//---------------------------------------------------------------------------
struct TListDataEntry
{
  const wchar_t * Name;
  const wchar_t * Permissions;
  const wchar_t * OwnerGroup;
  __int64 Size;
  bool Dir;
  bool Link;
  int Year;
  int Month;
  int Day;
  int Hour;
  int Minute;
  bool HasTime;
  bool HasDate;
  const wchar_t * LinkTarget;
};
//---------------------------------------------------------------------------
struct TFtpsCertificateData
{
  struct TContact
  {
    const wchar_t * Organization;
    const wchar_t * Unit;
    const wchar_t * CommonName;
    const wchar_t * Mail;
    const wchar_t * Country;
    const wchar_t * StateProvince;
    const wchar_t * Town;
    const wchar_t * Other;
  };

  TContact Subject;
  TContact Issuer;

  struct TValidityTime
  {
    int Year;
    int Month;
    int Day;
    int Hour;
    int Min;
    int Sec;
  };

  TValidityTime ValidFrom;
  TValidityTime ValidUntil;

  const unsigned wchar_t * Hash;
  static const size_t HashLen = 20;

  int VerificationResult;
  int VerificationDepth;
};
//---------------------------------------------------------------------------
class t_server;
//---------------------------------------------------------------------------
class TFileZillaIntf
{
friend class TFileZillaIntern;

public:
  enum TLogLevel
  {
    LOG_STATUS = 0,
    LOG_ERROR = 1,
    LOG_COMMAND = 2,
    LOG_REPLY = 3,
    LOG_LIST = 4,
    LOG_APIERROR = 5,
    LOG_WARNING = 6,
    LOG_INFO = 7,
    LOG_DEBUG = 8
  };

  enum TMessageType
  {
    MSG_OTHER = 0,
    MSG_TRANSFERSTATUS = 1
  };

  enum
  {
    FILEEXISTS_OVERWRITE = 0,
    FILEEXISTS_OVERWRITEIFNEWER = 1,
    FILEEXISTS_RESUME = 2,
    FILEEXISTS_RENAME = 3,
    FILEEXISTS_SKIP = 4,
  };

  enum
  {
    REPLY_OK = 0x0001,
    REPLY_WOULDBLOCK = 0x0002,
    REPLY_ERROR = 0x0004,
    REPLY_OWNERNOTSET = 0x0008,
    REPLY_INVALIDPARAM = 0x0010,
    REPLY_NOTCONNECTED = 0x0020,
    REPLY_ALREADYCONNECTED = 0x0040,
    REPLY_BUSY = 0x0080,
    REPLY_IDLE = 0x0100,
    REPLY_NOTINITIALIZED = 0x0200,
    REPLY_ALREADYINIZIALIZED = 0x0400,
    REPLY_CANCEL = 0x0800,
    REPLY_DISCONNECTED = 0x1000, // Always sent when disconnected from server
    REPLY_CRITICALERROR = 0x2000, // Used for FileTransfers only
    REPLY_ABORTED = 0x4000, // Used for FileTransfers only
    REPLY_NOTSUPPORTED = 0x8000 // Command is not supported for the current server
  };

  enum
  {
    SERVER_FTP =              0x1000,
    SERVER_FTP_SSL_IMPLICIT = 0x1100,
    SERVER_FTP_SSL_EXPLICIT = 0x1200,
    SERVER_FTP_TLS_EXPLICIT = 0x1400
  };

  static void Initialize();
  static void Finalize();
  static void SetResourceModule(void * ResourceHandle);

  explicit TFileZillaIntf();
  virtual ~TFileZillaIntf();

  bool Init();
  void Destroying();

  bool SetCurrentPath(const wchar_t * Path);
  bool GetCurrentPath(wchar_t * Path, size_t MaxLen);

  bool Cancel();

  bool Connect(const wchar_t * Host, int Port, const wchar_t * User,
    const wchar_t * Pass, const wchar_t * Account, bool FwByPass,
    const wchar_t * Path, int ServerType, int Pasv, int TimeZoneOffset, int UTF8,
    bool bForcePasvIp);
  bool Close();

  bool List();
  bool List(const wchar_t * Path);

  bool CustomCommand(const wchar_t * Command);

  bool MakeDir(const wchar_t* Path);
  bool Chmod(int Value, const wchar_t* FileName, const wchar_t* Path);
  bool Delete(const wchar_t* FileName, const wchar_t* Path);
  bool RemoveDir(const wchar_t* FileName, const wchar_t* Path);
  bool Rename(const wchar_t* OldName, const wchar_t* NewName,
    const wchar_t* Path, const wchar_t* NewPath);

  bool FileTransfer(const wchar_t * LocalFile, const wchar_t * RemoteFile,
    const wchar_t * RemotePath, bool Get, __int64 Size, int Type, void * UserData);

  virtual const wchar_t * Option(int OptionID) const = 0;
  virtual int OptionVal(int OptionID) const = 0;

  void SetDebugLevel(TLogLevel Level);
  bool HandleMessage(WPARAM wParam, LPARAM lParam);

protected:
  bool PostMessage(WPARAM wParam, LPARAM lParam);
  virtual bool DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam) = 0;

  virtual bool HandleStatus(const wchar_t * Status, int Type) = 0;
  virtual bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    __int64 Size1, __int64 Size2, time_t Time1, time_t Time2,
    bool HasTime1, bool HasTime2, void * UserData, int & RequestResult) = 0;
  virtual bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult) = 0;
  virtual bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    unsigned int Count) = 0;
  virtual bool HandleTransferStatus(bool Valid, __int64 TransferSize,
    __int64 Bytes, int Percent, int TimeElapsed, int TimeLeft, int TransferRate,
    bool FileTransfer) = 0;
  virtual bool HandleReply(int Command, unsigned int Reply) = 0;
  virtual bool HandleCapabilities(bool Mfmt) = 0;
  virtual bool CheckError(int ReturnCode, const wchar_t * Context);

  inline bool Check(int ReturnCode, const wchar_t * Context, int Expected = -1);

private:
  CFileZillaApi * FFileZillaApi;
  TFileZillaIntern * FIntern;
  t_server * FServer;
};
//---------------------------------------------------------------------------
#endif // FileZillaIntfH
