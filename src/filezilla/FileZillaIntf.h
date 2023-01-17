
#pragma once

#include <rdestl/map.h>

#include <time.h>
#include <FileZillaOpt.h>
#include <FilezillaTools.h>

class CFileZillaApi;
class TFileZillaIntern;

struct TRemoteFileTime
{
CUSTOM_MEM_ALLOCATION_IMPL
  WORD Year;
  WORD Month;
  WORD Day;
  WORD Hour;
  WORD Minute;
  WORD Second;
  bool HasTime;
  bool HasYear;
  bool HasSeconds;
  bool HasDate;
  bool Utc;
};

struct TListDataEntry
{
CUSTOM_MEM_ALLOCATION_IMPL
  const wchar_t * Name;
  const wchar_t * Permissions;
  const wchar_t * HumanPerm;
  const wchar_t * OwnerGroup; // deprecated, to be replaced with Owner/Group
  const wchar_t * Owner;
  const wchar_t * Group;
  int64_t Size;
  bool Dir;
  bool Link;
  TRemoteFileTime Time;
  const wchar_t * LinkTarget;
};

struct TFtpsCertificateData
{
CUSTOM_MEM_ALLOCATION_IMPL
  struct TContact
  {
  CUSTOM_MEM_ALLOCATION_IMPL
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
  CUSTOM_MEM_ALLOCATION_IMPL
    int Year;
    int Month;
    int Day;
    int Hour;
    int Min;
    int Sec;
  };

  TValidityTime ValidFrom;
  TValidityTime ValidUntil;

  const wchar_t * SubjectAltName;

  const uint8_t * HashSha1;
  static const size_t HashSha1Len = 20;
  const uint8_t * HashSha256;
  static const size_t HashSha256Len = 32;

  const uint8_t * Certificate;
  size_t CertificateLen;

  int VerificationResult;
  int VerificationDepth;
};

struct TNeedPassRequestData
{
CUSTOM_MEM_ALLOCATION_IMPL
    wchar_t * Password;
};

class t_server;
class TFTPServerCapabilities;
typedef struct x509_st X509;
typedef struct evp_pkey_st EVP_PKEY;

class TFileZillaIntf : public CFileZillaTools
{
NB_DISABLE_COPY(TFileZillaIntf)
friend class TFileZillaIntern;

public:
  enum TLogLevel
  {
    LOG_STATUS = 0,
    LOG_ERROR = 1,
    LOG_COMMAND = 2,
    LOG_REPLY = 3,
    LOG_APIERROR = 5,
    LOG_WARNING = 6,
    LOG_PROGRESS = 7,
    LOG_INFO = 8,
    LOG_DEBUG = 9,
  };

  enum TMessageType
  {
    MSG_OTHER = 0,
    MSG_TRANSFERSTATUS = 1
  };

  enum
  {
    FILEEXISTS_OVERWRITE = 0,
    FILEEXISTS_RESUME = 1,
    FILEEXISTS_RENAME = 2,
    FILEEXISTS_SKIP = 3,
    FILEEXISTS_COMPLETE = 4,
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

  explicit TFileZillaIntf() noexcept;
  virtual ~TFileZillaIntf() noexcept;

  bool Init();
  void Destroying();

  bool SetCurrentPath(const wchar_t * APath);
  bool GetCurrentPath(wchar_t * APath, size_t MaxLen);

  bool UsingMlsd();
  bool UsingUtf8();
  std::string GetTlsVersionStr();
  std::string GetCipherName();

  bool Cancel();

  bool Connect(const wchar_t * Host, int Port, const wchar_t * User,
    const wchar_t * Pass, const wchar_t * Account,
    const wchar_t * Path, int ServerType, int Pasv, int TimeZoneOffset, int UTF8, int CodePage,
    int iForcePasvIp, int iUseMlsd,
    int iDupFF, int iUndupFF,
    X509 * Certificate, EVP_PKEY * PrivateKey);
  bool Close(bool AllowBusy);

  bool List(const wchar_t * APath);
  bool ListFile(const wchar_t * FileName, const wchar_t * APath);

  bool CustomCommand(const wchar_t * Command);

  bool MakeDir(const wchar_t* APath);
  bool Chmod(int Value, const wchar_t* FileName, const wchar_t* APath);
  bool Delete(const wchar_t* FileName, const wchar_t* APath, bool FileNameOnly);
  bool RemoveDir(const wchar_t* FileName, const wchar_t* APath);
  bool Rename(const wchar_t* OldName, const wchar_t* NewName,
    const wchar_t* APath, const wchar_t* ANewPath);

  bool FileTransfer(
    const wchar_t * LocalFile, const wchar_t * RemoteFile,
    const wchar_t * RemotePath, bool Get, int64_t Size, int Type, void * UserData,
    TTransferOutEvent OnTransferOut, TTransferInEvent OnTransferIn);

  virtual const wchar_t * Option(int32_t OptionID) const = 0;
  virtual int32_t OptionVal(int32_t OptionID) const = 0;

  void SetDebugLevel(TLogLevel Level);
  bool HandleMessage(WPARAM wParam, LPARAM lParam);

protected:
  bool FZPostMessage(WPARAM wParam, LPARAM lParam);
  virtual bool DoPostMessage(TMessageType Type, WPARAM wParam, LPARAM lParam) = 0;

  virtual bool HandleStatus(const wchar_t * Status, int Type) = 0;
  virtual bool HandleAsynchRequestOverwrite(
    wchar_t * FileName1, size_t FileName1Len, const wchar_t * FileName2,
    const wchar_t * Path1, const wchar_t * Path2,
    int64_t Size1, int64_t Size2, time_t LocalTime,
    bool HasLocalTime1, const TRemoteFileTime & RemoteTime, void * UserData,
    HANDLE & LocalFileHandle,
    int & RequestResult) = 0;
  virtual bool HandleAsynchRequestVerifyCertificate(
    const TFtpsCertificateData & Data, int & RequestResult) = 0;
  virtual bool HandleAsynchRequestNeedPass(
    struct TNeedPassRequestData & Data, int & RequestResult) = 0;
  virtual bool HandleListData(const wchar_t * Path, const TListDataEntry * Entries,
    uint32_t Count) = 0;
  virtual bool HandleTransferStatus(bool Valid, int64_t TransferSize,
    int64_t Bytes, bool FileTransfer) = 0;
  virtual bool HandleReply(intptr_t Command, uintptr_t Reply) = 0;
  virtual bool HandleCapabilities(TFTPServerCapabilities * ServerCapabilities) = 0;
  virtual bool CheckError(intptr_t ReturnCode, const wchar_t * Context);

  inline bool Check(intptr_t ReturnCode, const wchar_t * Context, intptr_t Expected = -1);

private:
  CFileZillaApi * FFileZillaApi{nullptr};
  TFileZillaIntern * FIntern{nullptr};
  t_server * FServer{nullptr};
};

enum ftp_capabilities_t
{
  unknown,
  yes,
  no
};

enum ftp_capability_names_t
{
  syst_command = 1, // reply of SYST command as option
  feat_command,
  clnt_command, // set to 'yes' if CLNT should be sent
  utf8_command, // set to 'yes' if OPTS UTF8 ON should be sent
  mlsd_command,
  opts_mlst_command, // Arguments for OPTS MLST command
  mfmt_command,
  pret_command,
  mdtm_command,
  mode_z_support,
  tvfs_support, // Trivial virtual file store (RFC 3659)
  list_hidden_support, // LIST -a command
  rest_stream, // supports REST+STOR in addition to APPE
};

class TFTPServerCapabilities //: public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TFTPServerCapabilities)
public:
  TFTPServerCapabilities(){}
  ftp_capabilities_t GetCapability(ftp_capability_names_t Name) const;
  ftp_capabilities_t GetCapabilityString(ftp_capability_names_t Name, std::string * Option = nullptr) const;
  void SetCapability(ftp_capability_names_t Name, ftp_capabilities_t Cap);
  void SetCapability(ftp_capability_names_t Name, ftp_capabilities_t Cap, const std::string & Option);
  void Clear() { FCapabilityMap.clear(); }
  void Assign(TFTPServerCapabilities * Source)
  {
    FCapabilityMap.clear();
    if (Source != nullptr)
    {
      for (nb::map_t<ftp_capability_names_t, t_cap>::iterator it = Source->FCapabilityMap.begin();
        it != Source->FCapabilityMap.end(); ++it)
      {
        FCapabilityMap.insert(*it);
      }
    }
  }
protected:
  struct t_cap
  {
  CUSTOM_MEM_ALLOCATION_IMPL
    t_cap() :
      cap(unknown),
      option(),
      number(0)
    {
    }
    ftp_capabilities_t cap;
    std::string option;
    int number;
  };

  mutable nb::map_t<ftp_capability_names_t, t_cap> FCapabilityMap;
};


