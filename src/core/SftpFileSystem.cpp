
#include <vcl.h>
#pragma hdrstop

#include "SftpFileSystem.h"

#include "PuttyTools.h"
#include <Common.h>
#include <Exceptions.h>

#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "SecureShell.h"
#include "Cryptography.h"
#include <WideStrUtils.hpp>
#include <limits>

#include <memory>

// #pragma package(smart_init)

#undef FILE_OPERATION_LOOP_TERMINAL
#define FILE_OPERATION_LOOP_TERMINAL FTerminal

constexpr SSH_FX_TYPE
  SSH_FX_OK = 0,
  SSH_FX_EOF = 1,
  SSH_FX_NO_SUCH_FILE = 2,
  SSH_FX_PERMISSION_DENIED = 3,
  SSH_FX_FAILURE = 4,
  SSH_FX_OP_UNSUPPORTED = 8;

constexpr SSH_FXP_TYPE
  SSH_FXP_NONE = 0,
  SSH_FXP_INIT = 1,
  SSH_FXP_VERSION = 2,
  SSH_FXP_OPEN = 3,
  SSH_FXP_CLOSE = 4,
  SSH_FXP_READ = 5,
  SSH_FXP_WRITE = 6,
  SSH_FXP_LSTAT = 7,
  SSH_FXP_FSTAT = 8,
  SSH_FXP_SETSTAT = 9,
  SSH_FXP_FSETSTAT = 10,
  SSH_FXP_OPENDIR = 11,
  SSH_FXP_READDIR = 12,
  SSH_FXP_REMOVE = 13,
  SSH_FXP_MKDIR = 14,
  SSH_FXP_RMDIR = 15,
  SSH_FXP_REALPATH = 16,
  SSH_FXP_STAT = 17,
  SSH_FXP_RENAME = 18,
  SSH_FXP_READLINK = 19,
  SSH_FXP_SYMLINK = 20,
  SSH_FXP_LINK = 21,
  SSH_FXP_STATUS = 101,
  SSH_FXP_HANDLE = 102,
  SSH_FXP_DATA = 103,
  SSH_FXP_NAME = 104,
  SSH_FXP_ATTRS = 105,
  SSH_FXP_EXTENDED = 200,
  SSH_FXP_EXTENDED_REPLY = 201;
  // SSH_FXP_ATTRS = 105;

constexpr SSH_FILEXFER_ATTR_TYPE
  SSH_FILEXFER_ATTR_SIZE = 0x00000001,
  SSH_FILEXFER_ATTR_UIDGID = 0x00000002,
  SSH_FILEXFER_ATTR_PERMISSIONS = 0x00000004,
  SSH_FILEXFER_ATTR_ACMODTIME = 0x00000008,
  SSH_FILEXFER_ATTR_EXTENDED = 0x80000000,
  SSH_FILEXFER_ATTR_ACCESSTIME = 0x00000008,
  SSH_FILEXFER_ATTR_CREATETIME = 0x00000010,
  SSH_FILEXFER_ATTR_MODIFYTIME = 0x00000020,
  SSH_FILEXFER_ATTR_ACL = 0x00000040,
  SSH_FILEXFER_ATTR_OWNERGROUP = 0x00000080,
  SSH_FILEXFER_ATTR_SUBSECOND_TIMES = 0x00000100,
  SSH_FILEXFER_ATTR_BITS = 0x00000200,
  SSH_FILEXFER_ATTR_ALLOCATION_SIZE = 0x00000400,
  SSH_FILEXFER_ATTR_TEXT_HINT = 0x00000800,
  SSH_FILEXFER_ATTR_MIME_TYPE = 0x00001000,
  SSH_FILEXFER_ATTR_LINK_COUNT = 0x00002000,
  SSH_FILEXFER_ATTR_UNTRANSLATED_NAME = 0x00004000,
  SSH_FILEXFER_ATTR_CTIME = 0x00008000;
  // SSH_FILEXFER_ATTR_EXTENDED = 0x80000000;

constexpr SSH_FILEXFER_ATTR_TYPE
SSH_FILEXFER_ATTR_COMMON =
  (SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_OWNERGROUP |
   SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACCESSTIME |
   SSH_FILEXFER_ATTR_MODIFYTIME);

constexpr SSH_FILEXFER_TYPE
  SSH_FILEXFER_TYPE_REGULAR = 1,
  SSH_FILEXFER_TYPE_DIRECTORY = 2,
  SSH_FILEXFER_TYPE_SYMLINK = 3,
  SSH_FILEXFER_TYPE_SPECIAL = 4,
  SSH_FILEXFER_TYPE_UNKNOWN = 5;

constexpr SSH_FXF_TYPE
  SSH_FXF_READ = 0x00000001,
  SSH_FXF_WRITE = 0x00000002,
  SSH_FXF_APPEND = 0x00000004,
  SSH_FXF_CREAT = 0x00000008,
  SSH_FXF_TRUNC = 0x00000010,
  SSH_FXF_EXCL = 0x00000020,
  SSH_FXF_TEXT = 0x00000040,

  SSH_FXF_ACCESS_DISPOSITION = 0x00000007,
  SSH_FXF_CREATE_NEW = 0x00000000,
  SSH_FXF_CREATE_TRUNCATE = 0x00000001,
  SSH_FXF_OPEN_EXISTING = 0x00000002,
  SSH_FXF_OPEN_OR_CREATE = 0x00000003,
  SSH_FXF_TRUNCATE_EXISTING = 0x00000004,
  SSH_FXF_ACCESS_APPEND_DATA = 0x00000008,
  SSH_FXF_ACCESS_APPEND_DATA_ATOMIC = 0x00000010,
  SSH_FXF_ACCESS_TEXT_MODE = 0x00000020;

constexpr ACE4_TYPE
  ACE4_READ_DATA = 0x00000001,
  ACE4_LIST_DIRECTORY = 0x00000001,
  ACE4_WRITE_DATA = 0x00000002,
  ACE4_ADD_FILE = 0x00000002,
  ACE4_APPEND_DATA = 0x00000004,
  ACE4_ADD_SUBDIRECTORY = 0x00000004,
  ACE4_READ_NAMED_ATTRS = 0x00000008,
  ACE4_WRITE_NAMED_ATTRS = 0x00000010,
  ACE4_EXECUTE = 0x00000020,
  ACE4_DELETE_CHILD = 0x00000040,
  ACE4_READ_ATTRIBUTES = 0x00000080,
  ACE4_WRITE_ATTRIBUTES = 0x00000100,
  ACE4_DELETE = 0x00010000,
  ACE4_READ_ACL = 0x00020000,
  ACE4_WRITE_ACL = 0x00040000,
  ACE4_WRITE_OWNER = 0x00080000,
  ACE4_SYNCHRONIZE = 0x00100000;

constexpr uint32_t SSH_FILEXFER_ATTR_FLAGS_HIDDEN = 0x00000004;

using SSH_FXP_REALPATH_TYPE = uint8_t;
constexpr SSH_FXP_REALPATH_TYPE
  SSH_FXP_REALPATH_NO_CHECK = 0x00000001,
  SSH_FXP_REALPATH_STAT_IF = 0x00000002,
  SSH_FXP_REALPATH_STAT_ALWAYS = 0x00000003;

constexpr int32_t SFTP_MAX_PACKET_LEN = 1000 * 1024;

constexpr const char * SFTP_EXT_OWNER_GROUP = "owner-group-query@generic-extensions";
constexpr const char * SFTP_EXT_OWNER_GROUP_REPLY = "owner-group-query-reply@generic-extensions";
constexpr const char * SFTP_EXT_NEWLINE = "newline";
constexpr const char * SFTP_EXT_SUPPORTED = "supported";
constexpr const char * SFTP_EXT_SUPPORTED2 = "supported2";
constexpr const char * SFTP_EXT_FSROOTS = "fs-roots@vandyke.com";
constexpr const char * SFTP_EXT_VENDOR_ID = "vendor-id";
constexpr const char * SFTP_EXT_VERSIONS = "versions";
constexpr const char * SFTP_EXT_SPACE_AVAILABLE = "space-available";
constexpr const char * SFTP_EXT_CHECK_FILE = "check-file";
constexpr const char * SFTP_EXT_CHECK_FILE_NAME = "check-file-name";
constexpr const char * SFTP_EXT_STATVFS = "statvfs@openssh.com";
constexpr const char * SFTP_EXT_STATVFS_VALUE_V2 = "2";
constexpr const int64_t SFTP_EXT_STATVFS_ST_RDONLY = 0x1;
constexpr const int64_t SFTP_EXT_STATVFS_ST_NOSUID = 0x2;
constexpr const char * SFTP_EXT_HARDLINK = "hardlink@openssh.com";
constexpr const char * SFTP_EXT_HARDLINK_VALUE_V1 = "1";
constexpr const char * SFTP_EXT_COPY_FILE = "copy-file";
constexpr const char * SFTP_EXT_COPY_DATA = "copy-data";
constexpr const char * SFTP_EXT_LIMITS = "limits@openssh.com";
constexpr const char * SFTP_EXT_LIMITS_VALUE_V1 = "1";
constexpr const char * SFTP_EXT_POSIX_RENAME = "posix-rename@openssh.com";

constexpr wchar_t OGQ_LIST_OWNERS = 0x01;
constexpr wchar_t OGQ_LIST_GROUPS = 0x02;

// const int32_t SFTPMinVersion = 0;
// const int32_t SFTPMaxVersion = 6;
constexpr uint32_t SFTPNoMessageNumber = nb::ToUInt32(-1);

constexpr SSH_FX_TYPE asNo =            0;
constexpr SSH_FX_TYPE asOK =            1 << SSH_FX_OK;
constexpr SSH_FX_TYPE asEOF =           1 << SSH_FX_EOF;
constexpr SSH_FX_TYPE asPermDenied =    1 << SSH_FX_PERMISSION_DENIED;
constexpr SSH_FX_TYPE asOpUnsupported = 1 << SSH_FX_OP_UNSUPPORTED;
constexpr SSH_FX_TYPE asNoSuchFile =    1 << SSH_FX_NO_SUCH_FILE;
constexpr SSH_FX_TYPE asAll = static_cast<SSH_FX_TYPE>(0xFFFF);

#define GET_32BIT(cp) \
    ((static_cast<uint32_t>(static_cast<const uint8_t>((cp)[0])) << 24) | \
    (static_cast<uint32_t>(static_cast<const uint8_t>((cp)[1])) << 16) | \
    (static_cast<uint32_t>(static_cast<const uint8_t>((cp)[2])) << 8) | \
    (static_cast<uint32_t>(static_cast<const uint8_t>((cp)[3]))))

#define PUT_32BIT(cp, value) do { \
    (cp)[0] = static_cast<uint8_t>((value) >> 24); \
    (cp)[1] = static_cast<uint8_t>((value) >> 16); \
    (cp)[2] = static_cast<uint8_t>((value) >> 8); \
    (cp)[3] = static_cast<uint8_t>(value); } while(0)

constexpr uint32_t SFTP_PACKET_ALLOC_DELTA = 256;

// #pragma warn -inl

struct TSFTPSupport final : public TObject
{
  NB_DISABLE_COPY(TSFTPSupport)
public:
  TSFTPSupport() noexcept :
    AttribExtensions(std::make_unique<TStringList>())
  {
    Reset();
  }

  virtual ~TSFTPSupport() noexcept override = default;
  /*{
    SAFE_DESTROY(AttribExtensions);
  }*/

  void Reset()
  {
    AttributeMask = 0;
    AttributeBits = 0;
    OpenFlags = 0;
    AccessMask = 0;
    MaxReadSize = 0;
    OpenBlockVector = 0;
    BlockVector = 0;
    AttribExtensions->Clear();
    Loaded = false;
  }

  SSH_FILEXFER_ATTR_TYPE AttributeMask{0};
  uint32_t AttributeBits{0};
  uint32_t OpenFlags{0};
  uint32_t AccessMask{0};
  uint32_t MaxReadSize{0};
  uint32_t OpenBlockVector{0};
  uint32_t BlockVector{0};
  std::unique_ptr<TStrings> AttribExtensions;
  bool Loaded{false};
};

class TSFTPPacket : public TObject
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSFTPPacket); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPPacket) || TObject::is(Kind); }
public:
  explicit TSFTPPacket(uint32_t CodePage) noexcept = delete;
  TSFTPPacket() = delete;
  explicit TSFTPPacket(TObjectClassId Kind, uint32_t CodePage) noexcept :
    TObject(Kind)
  {
    Init(CodePage);
  }

  TSFTPPacket(const TSFTPPacket & Source) noexcept : TSFTPPacket(OBJECT_CLASS_TSFTPPacket, Source.FCodePage)
  {
    this->operator=(Source);
  }

  explicit TSFTPPacket(const TSFTPPacket & Source, uint32_t CodePage) noexcept : TSFTPPacket(OBJECT_CLASS_TSFTPPacket, CodePage)
  {
    *this = Source;
  }

  explicit TSFTPPacket(SSH_FXP_TYPE AType, uint32_t CodePage) noexcept : TSFTPPacket(OBJECT_CLASS_TSFTPPacket, CodePage)
  {
    ChangeType(AType);
  }

  explicit TSFTPPacket(const uint8_t * Source, uint32_t Len, uint32_t CodePage) noexcept : TSFTPPacket(OBJECT_CLASS_TSFTPPacket, CodePage)
  {
    FLength = Len;
    SetCapacity(FLength);
    memmove(GetData(), Source, Len);
  }

  explicit TSFTPPacket(const RawByteString & Source, uint32_t CodePage) noexcept : TSFTPPacket(OBJECT_CLASS_TSFTPPacket, CodePage)
  {
    FLength = nb::ToUInt32(Source.Length());
    SetCapacity(FLength);
    memmove(GetData(), Source.c_str(), Source.Length());
  }

  virtual ~TSFTPPacket() noexcept override
  {
    if (FData != nullptr)
    {
      nb_free(FData - FSendPrefixLen);
    }
    if (FReservedBy)
    {
      FReservedBy->UnreserveResponse(this);
    }
  }

  void ChangeType(SSH_FXP_TYPE AType)
  {
    FPosition = 0;
    FLength = 0;
    SetCapacity(0);
    FType = AType;
    AddByte(static_cast<uint8_t>(FType));
    if (FType != SSH_FXP_INIT) // && (FType != 1)
    {
      AssignNumber();
      AddCardinal(FMessageNumber);
    }
  }

  void Reuse()
  {
    AssignNumber();

    DebugAssert(GetLength() >= 5);

    // duplicated in AddCardinal()
    uint8_t Buf[4]{};
    PUT_32BIT(Buf, FMessageNumber);

    memmove(FData + 1, Buf, sizeof(Buf));
  }

  void AddByte(uint8_t Value)
  {
    Add(&Value, sizeof(Value));
  }

  void AddBool(bool Value)
  {
    AddByte(Value ? 1 : 0);
  }

  void AddCardinal(uint32_t Value)
  {
    // duplicated in Reuse()
    uint8_t Buf[4]{};
    PUT_32BIT(Buf, Value);
    Add(&Buf, sizeof(Buf));
  }

  void AddInt64(int64_t Value)
  {
    AddCardinal(nb::ToUInt32(Value >> 32));
    AddCardinal(nb::ToUInt32(Value & 0xFFFFFFFF));
  }

  void AddData(const void * Data, int32_t ALength)
  {
    AddCardinal(ALength);
    Add(Data, ALength);
  }

  void AddStringW(const UnicodeString & ValueW)
  {
    AddString(::W2MB(ValueW.c_str(), static_cast<UINT>(FCodePage)).c_str());
  }

  void AddString(const RawByteString & Value)
  {
    AddCardinal(nb::ToUInt32(Value.Length()));
    Add(Value.c_str(), Value.Length());
  }

  void AddUtfString(const UTF8String & Value)
  {
    AddString(Value);
  }

  void AddUtfString(const UnicodeString & Value)
  {
    AddUtfString(UTF8String(Value));
  }

  void AddString(const UnicodeString & Value, TAutoSwitch /*Utf*/)
  {
    AddStringW(Value);
#if 0
    // asAuto: Using UTF until we receive non-UTF string from the server
    if ((Utf == asOn) || (Utf == asAuto))
    {
      AddUtfString(Value);
    }
    else
    {
      AddString(RawByteString(AnsiString(Value)));
    }
#endif
  }

  // now purposeless alias to AddString
  inline void AddPathString(const UnicodeString & Value, TAutoSwitch Utf)
  {
    AddString(Value, Utf);
  }

  static SSH_FILEXFER_ATTR_TYPE AllocationSizeAttribute(int32_t Version)
  {
    return (Version >= 6) ? SSH_FILEXFER_ATTR_ALLOCATION_SIZE : SSH_FILEXFER_ATTR_SIZE;
  }

  void AddProperties(const uint16_t * Rights, TRemoteToken * Owner,
    TRemoteToken * Group, const int64_t * MTime, const int64_t * ATime,
    const int64_t * Size, bool IsDirectory, int32_t Version, TAutoSwitch Utf)
  {
    SSH_FILEXFER_ATTR_TYPE Flags = 0;
    if (Size != nullptr)
    {
      Flags |= AllocationSizeAttribute(Version);
    }
    // both or neither
    DebugAssert((Owner != nullptr) == (Group != nullptr));
    if ((Owner != nullptr) && (Group != nullptr))
    {
      if (Version < 4)
      {
        DebugAssert(Owner->GetIDValid() && Group->GetIDValid());
        Flags |= SSH_FILEXFER_ATTR_UIDGID;
      }
      else
      {
        DebugAssert(Owner->GetNameValid() && Group->GetNameValid());
        Flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
      }
    }
    if (Rights != nullptr)
    {
      Flags |= SSH_FILEXFER_ATTR_PERMISSIONS;
    }
    if ((Version < 4) && ((MTime != nullptr) || (ATime != nullptr)))
    {
      Flags |= SSH_FILEXFER_ATTR_ACMODTIME;
    }
    if ((Version >= 4) && (ATime != nullptr))
    {
      Flags |= SSH_FILEXFER_ATTR_ACCESSTIME;
    }
    if ((Version >= 4) && (MTime != nullptr))
    {
      Flags |= SSH_FILEXFER_ATTR_MODIFYTIME;
    }
    AddCardinal(Flags);

    if (Version >= 4)
    {
      AddByte(static_cast<uint8_t>(IsDirectory ?
        SSH_FILEXFER_TYPE_DIRECTORY : SSH_FILEXFER_TYPE_REGULAR));
    }

    if (Size != nullptr)
    {
      // this is SSH_FILEXFER_ATTR_SIZE for version <= 5, but
      // SSH_FILEXFER_ATTR_ALLOCATION_SIZE for version >= 6
      AddInt64(*Size);
    }

    if ((Owner != nullptr) && (Group != nullptr))
    {
      if (Version < 4)
      {
        DebugAssert(Owner->GetIDValid() && Group->GetIDValid());
        AddCardinal(nb::ToUInt32(Owner->GetID()));
        AddCardinal(nb::ToUInt32(Group->GetID()));
      }
      else
      {
        DebugAssert(Owner->GetNameValid() && Group->GetNameValid());
        AddString(Owner->GetName(), Utf);
        AddString(Group->GetName(), Utf);
      }
    }

    if (Rights != nullptr)
    {
      AddCardinal(*Rights);
    }

    if ((Version < 4) && ((MTime != nullptr) || (ATime != nullptr)))
    {
      // any way to reflect sbSignedTS here?
      // (note that casting int64_t > 2^31 < 2^32 to uint32_t is wrapped,
      // thus we never can set time after 2038, even if the server supports it)
      AddCardinal(nb::ToUInt32(ATime != nullptr ? *ATime : MTime != nullptr ? *MTime : 0));
      AddCardinal(nb::ToUInt32(MTime != nullptr ? *MTime : ATime != nullptr ? *ATime : 0));
    }
    if ((Version >= 4) && (ATime != nullptr))
    {
      AddInt64(*ATime);
    }
    if ((Version >= 4) && (MTime != nullptr))
    {
      AddInt64(*MTime);
    }
  }

  void AddProperties(const TRemoteProperties * Properties,
    uint16_t BaseRights, bool IsDirectory, int32_t Version, TAutoSwitch Utf,
    TChmodSessionAction * Action)
  {
    enum TValid
    {
      valNone = 0,
      valRights = 0x01,
      valOwner = 0x02,
      valGroup = 0x04,
      valMTime = 0x08,
      valATime = 0x10,
    } Valid = valNone;
    uint16_t RightsNum = 0;
    TRemoteToken Owner;
    TRemoteToken Group;
    int64_t MTime;
    int64_t ATime;

    if (Properties != nullptr)
    {
      if (Properties->Valid.Contains(vpGroup))
      {
        Valid = static_cast<TValid>(Valid | valGroup);
        Group = Properties->Group;
      }

      if (Properties->Valid.Contains(vpOwner))
      {
        Valid = static_cast<TValid>(Valid | valOwner);
        Owner = Properties->Owner;
      }

      if (Properties->Valid.Contains(vpRights))
      {
        Valid = static_cast<TValid>(Valid | valRights);
        TRights Rights = TRights(BaseRights).Combine(Properties->Rights);
        if (IsDirectory && Properties->AddXToDirectories)
        {
          Rights.AddExecute();
        }
        RightsNum = Rights;

        if (Action != nullptr)
        {
          Action->Rights(Rights);
        }
      }

      if (Properties->Valid.Contains(vpLastAccess) && Properties->LastAccess)
      {
        Valid = static_cast<TValid>(Valid | valATime);
        ATime = Properties->LastAccess;
      }

      if (Properties->Valid.Contains(vpModification) && Properties->Modification)
      {
        Valid = static_cast<TValid>(Valid | valMTime);
        MTime = Properties->Modification;
      }
    }

    AddProperties(
      (Valid & valRights) ? &RightsNum : nullptr,
      (Valid & valOwner) ? &Owner : nullptr,
      (Valid & valGroup) ? &Group : nullptr,
      (Valid & valMTime) ? &MTime : nullptr,
      (Valid & valATime) ? &ATime : nullptr,
      nullptr, IsDirectory, Version, Utf);
  }

  uint8_t GetByte() const
  {
    Need(sizeof(uint8_t));
    const uint8_t Result = FData[FPosition];
    DataConsumed(sizeof(uint8_t));
    return Result;
  }

  bool GetBool() const
  {
    return (GetByte() != 0);
  }

  bool CanGetBool() const
  {
    return (GetRemainingLength() >= sizeof(uint8_t));
  }

  uint32_t GetCardinal() const
  {
    const uint32_t Result = PeekCardinal();
    DataConsumed(sizeof(Result));
    return Result;
  }

  bool CanGetCardinal() const
  {
    return (GetRemainingLength() >= sizeof(uint32_t));
  }

  uint32_t GetSmallCardinal() const
  {
    Need(2);
    const uint32_t Result = (FData[FPosition] << 8) + FData[FPosition + 1];
    DataConsumed(2);
    return Result;
  }

  bool CanGetSmallCardinal() const
  {
    return (GetRemainingLength() >= 2);
  }

  int64_t GetInt64() const
  {
    const int64_t Hi = GetCardinal();
    const int64_t Lo = GetCardinal();
    return (Hi << 32) + Lo;
  }

  RawByteString GetRawByteString() const
  {
    RawByteString Result;
    uint32_t Len = GetCardinal();
    Need(Len);
    // cannot happen anyway as Need() would raise exception
    DebugAssert(Len < SFTP_MAX_PACKET_LEN);
    Result.SetLength(nb::ToInt32(Len));
    memmove(nb::ToPtr(Result.c_str()), FData + FPosition, Len);
    DataConsumed(Len);
    return Result;
  }

  bool CanGetString(uint32_t &Size) const
  {
    bool Result = CanGetCardinal();
    if (Result)
    {
      const uint32_t Len = PeekCardinal();
      Size = (sizeof(Len) + Len);
      Result = (Size <= GetRemainingLength());
    }
    return Result;
  }

  // For reading strings that are character strings (not byte strings
  // as file handles), and SFTP spec does not say explicitly that they
  // are in UTF. For most of them it actually does not matter as
  // the content should be pure ASCII (e.g. extension names, etc.)
  UnicodeString GetAnsiString() const
  {
    return UnicodeString(AnsiToString(GetRawByteString()));
  }

  RawByteString GetFileHandle() const
  {
    return GetRawByteString();
  }

  UnicodeString GetStringW() const
  {
    return ::MB2W(GetRawByteString().c_str(), static_cast<UINT>(FCodePage));
  }

  UnicodeString GetString(TAutoSwitch /*Utf*/) const
  {
    return GetStringW();
#if 0
    if (Utf != asOff)
    {
      return GetUtfString(Utf);
    }
    else
    {
      return GetAnsiString();
    }
#endif
  }

  // now purposeless alias to GetString(bool)
  UnicodeString GetPathString(TAutoSwitch Utf) const
  {
    return GetString(Utf);
  }

  void GetFile(TRemoteFile * AFile, int32_t Version, TDSTMode DSTMode, TAutoSwitch & Utf, bool SignedTS, bool Complete)
  {
    DebugAssert(AFile);
    UnicodeString ListingStr;
    uint32_t Permissions = 0;
    bool ParsingFailed = false;
    if (GetType() != SSH_FXP_ATTRS)
    {
      AFile->SetFileName(GetPathString(Utf));
      if (Version < 4)
      {
        ListingStr = GetString(Utf);
      }
    }
    const SSH_FILEXFER_ATTR_TYPE Flags = GetCardinal();
    if (Version >= 4)
    {
      const uint8_t FXType = GetByte();
      // -:regular, D:directory, L:symlink, S:special, U:unknown
      // O:socket, C:char device, B:block device, F:fifo

      // SSH-2.0-cryptlib returns file type 0 in response to SSH_FXP_LSTAT,
      // handle this undefined value as "unknown"
      constexpr const wchar_t * Types = L"U-DLSUOCBF";
      if (FXType > static_cast<uint8_t>(nb::StrLength(Types)))
      {
        throw Exception(FMTLOAD(SFTP_UNKNOWN_FILE_TYPE, nb::ToInt32(FXType)));
      }
      AFile->SetType(Types[FXType]);
    }
    if (Flags & SSH_FILEXFER_ATTR_SIZE)
    {
      AFile->SetSize(GetInt64());
    }
    // SFTP-6 only
    if (Flags & SSH_FILEXFER_ATTR_ALLOCATION_SIZE)
    {
      nb::used(GetInt64()); // skip
    }
    // SSH-2.0-3.2.0 F-SECURE SSH - Process Software MultiNet
    // sets SSH_FILEXFER_ATTR_UIDGID for v4, but does not include the UID/GUID
    if ((Flags & SSH_FILEXFER_ATTR_UIDGID) && (Version < 4))
    {
      AFile->GetFileOwner().SetID(nb::ToInt32(GetCardinal()));
      AFile->GetFileGroup().SetID(nb::ToInt32(GetCardinal()));
    }
    if (Flags & SSH_FILEXFER_ATTR_OWNERGROUP)
    {
      DebugAssert(Version >= 4);
      AFile->GetFileOwner().SetName(GetString(Utf));
      AFile->GetFileGroup().SetName(GetString(Utf));
    }
    if (Flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      Permissions = GetCardinal();
    }
    if (Version < 4)
    {
      if (Flags & SSH_FILEXFER_ATTR_ACMODTIME)
      {
        AFile->SetLastAccess(::UnixToDateTime(
          SignedTS ?
            nb::ToInt64(static_cast<int32_t>(GetCardinal())) :
            nb::ToInt64(GetCardinal()),
          DSTMode));
        AFile->SetModification(::UnixToDateTime(
          SignedTS ?
            nb::ToInt64(static_cast<int32_t>(GetCardinal())) :
            nb::ToInt64(GetCardinal()),
          DSTMode));
      }
    }
    else
    {
      if (Flags & SSH_FILEXFER_ATTR_ACCESSTIME)
      {
        AFile->SetLastAccess(::UnixToDateTime(GetInt64(), DSTMode));
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          nb::used(GetCardinal()); // skip access time subseconds
        }
      }
      if (Flags & SSH_FILEXFER_ATTR_CREATETIME)
      {
        nb::used(GetInt64()); // skip create time
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          nb::used(GetCardinal()); // skip create time subseconds
        }
      }
      if (Flags & SSH_FILEXFER_ATTR_MODIFYTIME)
      {
        AFile->SetModification(::UnixToDateTime(GetInt64(), DSTMode));
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          nb::used(GetCardinal()); // skip modification time subseconds
        }
      }
      // SFTP-6
      if (Flags & SSH_FILEXFER_ATTR_CTIME)
      {
        nb::used(GetInt64()); // skip attribute modification time
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          nb::used(GetCardinal()); // skip attribute modification time subseconds
        }
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_ACL)
    {
      nb::used(GetRawByteString());
    }

    if (Flags & SSH_FILEXFER_ATTR_BITS)
    {
      // while SSH_FILEXFER_ATTR_BITS is defined for SFTP5 only, vandyke 2.3.3 sets it
      // for SFTP4 as well
      SSH_FILEXFER_ATTR_TYPE Bits = GetCardinal();
      if (Version >= 6)
      {
        const uint32_t BitsValid = GetCardinal();
        Bits = Bits & BitsValid;
      }
      if (FLAGSET(Bits, SSH_FILEXFER_ATTR_FLAGS_HIDDEN))
      {
        AFile->SetIsHidden(true);
      }
    }

    // skip some SFTP-6 only fields
    if (Flags & SSH_FILEXFER_ATTR_TEXT_HINT)
    {
      nb::used(GetByte());
    }
    if (Flags & SSH_FILEXFER_ATTR_MIME_TYPE)
    {
      nb::used(GetAnsiString());
    }
    if (Flags & SSH_FILEXFER_ATTR_LINK_COUNT)
    {
      nb::used(GetCardinal());
    }
    if (Flags & SSH_FILEXFER_ATTR_UNTRANSLATED_NAME)
    {
      nb::used(GetPathString(Utf));
    }

    if ((Version < 4) && (GetType() != SSH_FXP_ATTRS))
    {
      try
      {
        // update permissions and user/group name
        // modification time and filename is ignored
        AFile->SetListingStr(ListingStr);
      }
      catch(...)
      {
        // ignore any error while parsing listing line,
        // SFTP specification do not recommend to parse it
        ParsingFailed = true;
      }
    }

    if (GetType() == SSH_FXP_ATTRS || Version >= 4 || ParsingFailed)
    {
      wchar_t AType = FILETYPE_DEFAULT;
      if (FLAGSET(Flags, SSH_FILEXFER_ATTR_PERMISSIONS))
      {
        AFile->GetRightsNotConst()->SetNumber(static_cast<uint16_t>(Permissions & TRights::rfAllSpecials));
        if (FLAGSET(Permissions, TRights::rfDirectory))
        {
          AType = FILETYPE_DIRECTORY;
        }
      }

      if (Version < 4)
      {
        AFile->SetType(AType);
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_EXTENDED)
    {
      const uint32_t ExtendedCount = GetCardinal();
      for (uint32_t Index = 0; Index < ExtendedCount; ++Index)
      {
        nb::used(GetRawByteString()); // skip extended_type
        nb::used(GetRawByteString()); // skip extended_data
      }
    }

    if (Complete)
    {
      AFile->Complete();
    }
  }

  uint8_t * GetNextData(uint32_t Size = 0)
  {
    if (Size > 0)
    {
      Need(Size);
    }
    return FPosition < FLength ? FData + FPosition : nullptr;
  }

  void DataConsumed(uint32_t Size) const
  {
    FPosition += Size;
  }

  void DataUpdated(uint32_t ALength)
  {
    FPosition = 0;
    FLength = ALength;
    FType = GetByte();
    if (FType != SSH_FXP_VERSION)
    {
      FMessageNumber = GetCardinal();
    }
    else
    {
      FMessageNumber = SFTPNoMessageNumber;
    }
  }

  void LoadFromFile(const UnicodeString & AFileName)
  {
    std::unique_ptr<TStringList> DumpLines(std::make_unique<TStringList>());
    RawByteString Dump;
    try__finally
    {
      DumpLines->LoadFromFile(AFileName);
      Dump = RawByteString(AnsiString(DumpLines->GetText()));
    }
    __finally__removed
    {
      // delete DumpLines;
    } end_try__finally

    SetCapacity(20 * 1024);
    uint8_t Byte[3];
    nb::ClearArray(Byte);
    int32_t Index = 1;
    uint32_t Length = 0;
    while (Index < Dump.Length())
    {
      const char C = Dump[Index];
      if (IsHex(C))
      {
        if (Byte[0] == '\0')
        {
          Byte[0] = C;
        }
        else
        {
          Byte[1] = C;
          DebugAssert(Length < GetCapacity());
          GetData()[Length] = HexToByte(UnicodeString(reinterpret_cast<char *>(Byte)));
          Length++;
          nb::ClearArray(Byte);
        }
      }
      ++Index;
    }
    DataUpdated(Length);
  }

  UnicodeString Dump() const
  {
    UnicodeString Result;
    for (uint32_t Index = 0; Index < GetLength(); ++Index)
    {
      Result += ByteToHex(GetData()[Index]) + L",";
      if (((Index + 1) % 25) == 0)
      {
        Result += L"\n";
      }
    }
    return Result;
  }

  TSFTPPacket & operator =(const TSFTPPacket & Source)
  {
    if (this == &Source)
      return *this;
    SetCapacity(0);
    Add(Source.GetData(), Source.GetLength());
    DataUpdated(Source.GetLength());
    FPosition = Source.FPosition;
    FReservedBy = Source.FReservedBy;
    FCodePage = Source.FCodePage;
    return *this;
  }

  __property uint32_t Length = { read = FLength };
  __property uint32_t RemainingLength = { read = GetRemainingLength };
  __property uint8_t * Data = { read = FData };
  __property uint8_t * SendData = { read = GetSendData };
  __property uint32_t SendLength = { read = GetSendLength };
  __property uint32_t Capacity = { read = FCapacity, write = SetCapacity };
  __property uint8_t Type = { read = FType };
  ROProperty2<SSH_FXP_TYPE> Type{&FType};
  __property uint8_t RequestType = { read = GetRequestType };
  __property uint32_t MessageNumber = { read = FMessageNumber, write = FMessageNumber };
  RWProperty2<uint32_t> MessageNumber{&FMessageNumber};
  __property TSFTPFileSystem * ReservedBy = { read = FReservedBy, write = FReservedBy };
  __property UnicodeString TypeName = { read = GetTypeName };
  ROProperty<UnicodeString> TypeName{nb::bind(&TSFTPPacket::GetTypeName, this)};

  uint32_t GetLength() const { return FLength; }
  uint8_t * GetData() const { return FData; }
  uint32_t GetCapacity() const { return FCapacity; }
  SSH_FXP_TYPE GetType() const { return FType; }
  uint32_t GetMessageNumber() const { return FMessageNumber; }
  void SetMessageNumber(uint32_t Value) { FMessageNumber = Value; }
  TSFTPFileSystem * GetReservedBy() const { return FReservedBy; }
  void SetReservedBy(TSFTPFileSystem * Value) { FReservedBy = Value; }

private:
  uint8_t * FData{nullptr};
  uint32_t FLength{0};
  uint32_t FCapacity{0};
  mutable uint32_t FPosition{0};
  SSH_FXP_TYPE FType{SSH_FXP_NONE};
  uint32_t FMessageNumber{SFTPNoMessageNumber};
  TSFTPFileSystem * FReservedBy{nullptr};

  static uint32_t FMessageCounter;
  static constexpr int32_t FSendPrefixLen = 4;
  uint32_t FCodePage{0};

  void Init(uint32_t CodePage)
  {
    FData = nullptr;
    FCapacity = 0;
    FLength = 0;
    FPosition = 0;
    FMessageNumber = SFTPNoMessageNumber;
    FType = static_cast<SSH_FXP_TYPE>(-1);
    FReservedBy = nullptr;
    FCodePage = CodePage;
  }

  void AssignNumber()
  {
    // this is not strictly thread-safe, but as it is accessed from multiple
    // threads only for multiple connection, it is not problem if two threads get
    // the same number
    FMessageNumber = (FMessageCounter << 8) + FType;
    FMessageCounter++;
  }

public:
  uint8_t GetRequestType() const
  {
    if (FMessageNumber != SFTPNoMessageNumber)
    {
      return static_cast<uint8_t>(FMessageNumber & 0xFF);
    }
    DebugAssert(GetType() == SSH_FXP_VERSION);
    return SSH_FXP_INIT;
  }

  void Add(const void * AData, uint32_t ALength)
  {
    if (GetLength() + ALength > GetCapacity())
    {
      SetCapacity(GetLength() + ALength + SFTP_PACKET_ALLOC_DELTA);
    }
    memmove(FData + GetLength(), AData, ALength);
    FLength += ALength;
  }

  void SetCapacity(uint32_t ACapacity)
  {
    if (ACapacity != GetCapacity())
    {
      FCapacity = ACapacity;
      if (FCapacity > 0)
      {
        uint8_t * NData = nb::calloc<uint8_t *>(1, FCapacity + FSendPrefixLen);
        NData += FSendPrefixLen;
        if (FData)
        {
          memmove(NData - FSendPrefixLen, FData - FSendPrefixLen,
            (FLength < FCapacity ? FLength : FCapacity) + FSendPrefixLen);
          nb_free(FData - FSendPrefixLen);
        }
        FData = NData;
      }
      else
      {
        if (FData)
        {
          nb_free(FData - FSendPrefixLen);
        }
        FData = nullptr;
      }
      if (FLength > FCapacity)
      {
        FLength = FCapacity;
      }
    }
  }

  UnicodeString GetTypeName() const
  {
    #define TYPE_CASE(TYPE) case (TYPE): return MB_TEXT(#TYPE)
    switch (GetType()) {
      TYPE_CASE(SSH_FXP_INIT);
      TYPE_CASE(SSH_FXP_VERSION);
      TYPE_CASE(SSH_FXP_OPEN);
      TYPE_CASE(SSH_FXP_CLOSE);
      TYPE_CASE(SSH_FXP_READ);
      TYPE_CASE(SSH_FXP_WRITE);
      TYPE_CASE(SSH_FXP_LSTAT);
      TYPE_CASE(SSH_FXP_FSTAT);
      TYPE_CASE(SSH_FXP_SETSTAT);
      TYPE_CASE(SSH_FXP_FSETSTAT);
      TYPE_CASE(SSH_FXP_OPENDIR);
      TYPE_CASE(SSH_FXP_READDIR);
      TYPE_CASE(SSH_FXP_REMOVE);
      TYPE_CASE(SSH_FXP_MKDIR);
      TYPE_CASE(SSH_FXP_RMDIR);
      TYPE_CASE(SSH_FXP_REALPATH);
      TYPE_CASE(SSH_FXP_STAT);
      TYPE_CASE(SSH_FXP_RENAME);
      TYPE_CASE(SSH_FXP_READLINK);
      TYPE_CASE(SSH_FXP_SYMLINK);
      TYPE_CASE(SSH_FXP_LINK);
      TYPE_CASE(SSH_FXP_STATUS);
      TYPE_CASE(SSH_FXP_HANDLE);
      TYPE_CASE(SSH_FXP_DATA);
      TYPE_CASE(SSH_FXP_NAME);
      TYPE_CASE(SSH_FXP_ATTRS);
      TYPE_CASE(SSH_FXP_EXTENDED);
      TYPE_CASE(SSH_FXP_EXTENDED_REPLY);
      default:
        return FORMAT("Unknown message (%d)", nb::ToInt32(GetType()));
    }
  }

  uint8_t * GetSendData() const
  {
    uint8_t * Result = FData - FSendPrefixLen;
    // this is not strictly const-object operation
    PUT_32BIT(Result, GetLength());
    return Result;
  }

  uint32_t GetSendLength() const
  {
    return FSendPrefixLen + GetLength();
  }

  uint32_t GetRemainingLength() const
  {
    return GetLength() - FPosition;
  }

private:
  void Need(uint32_t Size) const
  {
    if (Size > GetRemainingLength())
    {
      throw Exception(FMTLOAD(SFTP_PACKET_ERROR, nb::ToInt32(FPosition), nb::ToInt32(Size), nb::ToInt32(FLength)));
    }
  }

  uint32_t PeekCardinal() const
  {
    Need(sizeof(uint32_t));
    const uint8_t * cp = FData + FPosition;
    const uint32_t Result = GET_32BIT(cp);
    return Result;
  }

  UnicodeString GetUtfString(TAutoSwitch & Utf) const
  {
    DebugAssert(Utf != asOff);
    UnicodeString Result;
    const RawByteString S = GetRawByteString();

    if (Utf == asAuto)
    {
      const nb::TEncodeType EncodeType = DetectUTF8Encoding(S);
      if (EncodeType == nb::etANSI)
      {
        Utf = asOff;
        Result = AnsiToString(S);
      }
    }

    if (Utf != asOff)
    {
      Result = UTF8ToString(S);
    }

    return Result;
  }
};

class TSFTPQueuePacket final : public TSFTPPacket
{
  NB_DISABLE_COPY(TSFTPQueuePacket)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSFTPQueuePacket); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPQueuePacket) || TSFTPPacket::is(Kind); }
public:
  TSFTPQueuePacket() = delete;
  explicit TSFTPQueuePacket(SSH_FXP_TYPE AType, uint32_t CodePage) noexcept :
     TSFTPPacket(OBJECT_CLASS_TSFTPQueuePacket, CodePage)
  {
    ChangeType(AType);
  }

  void * Token{nullptr};
};

uint32_t TSFTPPacket::FMessageCounter = 0;

class TSFTPQueue : public TObject
{
  NB_DISABLE_COPY(TSFTPQueue)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSFTPQueue); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPQueue) || TObject::is(Kind); }
public:
  TSFTPQueue() = delete;
  explicit TSFTPQueue(TSFTPFileSystem * AFileSystem, uint32_t CodePage) noexcept :
    TObject(OBJECT_CLASS_TSFTPQueue),
    FRequests(std::make_unique<TList>()),
    FResponses(std::make_unique<TList>()),
    FFileSystem(AFileSystem),
    FCodePage(CodePage)
  {
    DebugAssert(FFileSystem);
  }

  virtual ~TSFTPQueue() noexcept override
  {
    DebugAssert(FResponses->GetCount() == FRequests->GetCount());
    for (int32_t Index = 0; Index < FRequests->GetCount(); ++Index)
    {
      TSFTPQueuePacket * Request = FRequests->GetAs<TSFTPQueuePacket>(Index);
      DebugAssert(Request);
      SAFE_DESTROY(Request);

      TSFTPPacket * Response = FResponses->GetAs<TSFTPPacket>(Index);
      DebugAssert(Response);
      SAFE_DESTROY(Response);
    }
//    SAFE_DESTROY(FRequests);
//    SAFE_DESTROY(FResponses);
  }

  bool Init()
  {
    return SendRequests();
  }

  virtual void Dispose(SSH_FXP_TYPE ExpectedType, SSH_FX_TYPE AllowStatus)
  {
    DebugAssert(FFileSystem->FTerminal->GetActive());

    while (FRequests->GetCount())
    {
      DebugAssert(FResponses->GetCount());

      std::unique_ptr<TSFTPQueuePacket> Request(DebugNotNull(FRequests->GetAs<TSFTPQueuePacket>(0)));
      std::unique_ptr<TSFTPPacket> Response(DebugNotNull(FResponses->GetAs<TSFTPPacket>(0)));

      // Particularly when ExpectedType >= 0, the ReceiveResponse may throw, and we have to remove the packets from queue
      FRequests->Delete(0);
      FResponses->Delete(0);

      try
      {
        ReceiveResponse(Request.get(), Response.get(), ExpectedType, AllowStatus);
      }
      catch(Exception & E)
      {
        if (ExpectedType < 0)
        {
          if (FFileSystem->FTerminal->GetActive())
          {
            FFileSystem->FTerminal->LogEvent("Error while disposing the SFTP queue.");
            FFileSystem->FTerminal->GetLog()->AddException(&E);
          }
          else
          {
            FFileSystem->FTerminal->LogEvent("Fatal error while disposing the SFTP queue.");
            throw;
          }
        }
        else
        {
          throw;
        }
      }
    }
  }

  void DisposeSafe(SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1)
  {
    if (FFileSystem->FTerminal->GetActive())
    {
      Dispose(ExpectedType, AllowStatus);
    }
  }

  bool ReceivePacket(TSFTPPacket * Packet,
    SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1, void ** Token = nullptr, bool TryOnly = false)
  {
    DebugAssert(FRequests->GetCount());
    bool Result;
    std::unique_ptr<TSFTPQueuePacket> Request(FRequests->GetAs<TSFTPQueuePacket>(0));
    try__finally
    {
      FRequests->Delete(0);
      DebugAssert(Request.get());
      if (Token != nullptr)
      {
        *Token = Request->Token;
      }

      std::unique_ptr<TSFTPPacket> Response(FResponses->GetAs<TSFTPPacket>(0));
      FResponses->Delete(0);
      DebugAssert(Response.get());

      ReceiveResponse(Request.get(), Response.get(), ExpectedType, AllowStatus, TryOnly);

      if ((Response->GetCapacity() == 0) && DebugAlwaysTrue(TryOnly))
      {
        FRequests->Insert(0, Request.release());
        FResponses->Insert(0, Response.release());
        Result = true;
      }
      else
      {
        if (Packet)
        {
          *Packet = *Response.get();
        }

        Result = !End(Response.get());
        if (Result)
        {
          SendRequests();
        }
      }
    }
    __finally__removed
    {
      // delete Request;
      // delete Response;
    } end_try__finally

    return Result;
  }

  bool Next(SSH_FXP_TYPE ExpectedType = -1, SSH_FX_TYPE AllowStatus = -1)
  {
    return ReceivePacket(nullptr, ExpectedType, AllowStatus);
  }

protected:
  std::unique_ptr<TList> FRequests{nullptr};
  std::unique_ptr<TList> FResponses{nullptr};
  TSFTPFileSystem * FFileSystem{nullptr};
  uint32_t FCodePage{0};

#if 0
  class TSFTPQueuePacket : public TSFTPPacket
  {
  public:
    TSFTPQueuePacket() :
      TSFTPPacket()
    {
      Token = nullptr;
    }

    void * Token;
  };
#endif // #if 0

  virtual bool InitRequest(TSFTPQueuePacket * Request) = 0;

  virtual bool End(TSFTPPacket * Response) = 0;

  virtual void SendPacket(TSFTPQueuePacket * Packet)
  {
    FFileSystem->SendPacket(Packet);
  }

  virtual void ReceiveResponse(
    const TSFTPPacket * Packet, TSFTPPacket * Response, SSH_FXP_TYPE ExpectedType = -1,
    SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false)
  {
    FFileSystem->ReceiveResponse(Packet, Response, ExpectedType, AllowStatus, TryOnly);
  }

  // sends as many requests as allowed by implementation
  virtual bool SendRequests() = 0;

  virtual bool SendRequest()
  {
    std::unique_ptr<TSFTPQueuePacket> Request(std::make_unique<TSFTPQueuePacket>(SSH_FXP_NONE, FCodePage));
    try__catch
    {
      if (!InitRequest(Request.get()))
      {
        Request.reset();
      }
    }
    __catch__removed
    {
      // delete Request;
      // throw;
    } end_try__catch

    if (Request != nullptr)
    {
      TSFTPPacket * Response = new TSFTPPacket(SSH_FXP_NONE, FCodePage);
      FRequests->Add(Request.get());
      FResponses->Add(Response);

      // make sure the response is reserved before actually ending the message
      // as we may receive response asynchronously before SendPacket finishes
      FFileSystem->ReserveResponse(Request.get(), Response);
      SendPacket(Request.release());
      return true;
    }

    return false;
  }
};

class TSFTPFixedLenQueue : public TSFTPQueue
{
public:
  TSFTPFixedLenQueue() = delete;
  explicit TSFTPFixedLenQueue(TSFTPFileSystem * AFileSystem, uint32_t CodePage) noexcept :
    TSFTPQueue(AFileSystem, CodePage)
  {
    FMissedRequests = 0;
  }
  virtual ~TSFTPFixedLenQueue() override = default;

  bool Init(int32_t QueueLen)
  {
    FMissedRequests = QueueLen - 1;
    return TSFTPQueue::Init();
  }

protected:
  int32_t FMissedRequests{0};

  // sends as many requests as allowed by implementation
  virtual bool SendRequests() override
  {
    bool Result = false;
    FMissedRequests++;
    while ((FMissedRequests > 0) && SendRequest())
    {
      Result = true;
      FMissedRequests--;
    }
    return Result;
  }
};

class TSFTPAsynchronousQueue : public TSFTPQueue
{
public:
  TSFTPAsynchronousQueue() = delete;
  // #pragma option push -vi- // WORKAROUND for internal compiler errors
  explicit TSFTPAsynchronousQueue(TSFTPFileSystem * AFileSystem, uint32_t CodePage) noexcept : TSFTPQueue(AFileSystem, CodePage)
  {
    FFileSystem->FSecureShell->RegisterReceiveHandler(nb::bind(&TSFTPAsynchronousQueue::ReceiveHandler, this));
    FReceiveHandlerRegistered = true;
  }
  // #pragma option pop

  virtual ~TSFTPAsynchronousQueue() noexcept override
  {
    UnregisterReceiveHandler();
  }

  virtual void Dispose(SSH_FXP_TYPE ExpectedType, SSH_FX_TYPE AllowStatus) override
  {
    // we do not want to receive asynchronous notifications anymore,
    // while waiting synchronously for pending responses
    UnregisterReceiveHandler();
    TSFTPQueue::Dispose(ExpectedType, AllowStatus);
  }

  bool Continue()
  {
    return SendRequest();
  }

protected:

  // event handler for incoming data
  void ReceiveHandler(TObject * /*Sender*/)
  {
    try
    {
      while (// optimization only as we call ReceivePacket with TryOnly anyway
        FFileSystem->PeekPacket() &&
        ReceivePacketAsynchronously())
      {
        // loop
      }
    }
    catch(Exception & E)  // prevent crash when server unexpectedly closes connection
    {
      DebugUsedParam(E);
      DEBUG_PRINTF("ReceiveHandler: %s\n", E.Message);
    }
  }

  virtual bool ReceivePacketAsynchronously() = 0;

  // sends as many requests as allowed by implementation
  virtual bool SendRequests() override
  {
    // noop
    return true;
  }

  // #pragma option push -vi- // See pragma at constructor
  void UnregisterReceiveHandler()
  {
    if (FReceiveHandlerRegistered)
    {
      FReceiveHandlerRegistered = false;
      FFileSystem->FSecureShell->UnregisterReceiveHandler(nb::bind(&TSFTPAsynchronousQueue::ReceiveHandler, this));
    }
  }
  // #pragma option pop

private:
  bool FReceiveHandlerRegistered{false};
};

class TSFTPDownloadQueue final : public TSFTPFixedLenQueue
{
  NB_DISABLE_COPY(TSFTPDownloadQueue)
public:
  TSFTPDownloadQueue() = delete;
  explicit TSFTPDownloadQueue(TSFTPFileSystem * AFileSystem, uint32_t CodePage) noexcept :
    TSFTPFixedLenQueue(AFileSystem, CodePage)
  {
  }
  virtual ~TSFTPDownloadQueue() override = default;

  bool Init(
    int32_t QueueLen, const RawByteString & AHandle, int64_t Offset, int64_t PartSize, TFileOperationProgressType * AOperationProgress)
  {
    FHandle = AHandle;
    FOffset = Offset;
    FTransferred = Offset;
    FPartSize = PartSize;
    OperationProgress = AOperationProgress;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  void InitFillGapRequest(int64_t Offset, uint32_t MissingLen,
    TSFTPPacket * Packet)
  {
    InitRequest(Packet, Offset, MissingLen);
  }

  bool ReceivePacket(TSFTPPacket * Packet, uint32_t & BlockSize)
  {
    void * Token{nullptr};
    const bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_DATA, asEOF, &Token);
    BlockSize = nb::ToUInt32(nb::ToIntPtr(Token));
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request) override
  {
    uint32_t BlockSize = FFileSystem->DownloadBlockSize(OperationProgress);
    if (FPartSize >= 0)
    {
      const int64_t Remaining = (FOffset + FPartSize) - FTransferred;
      if (Remaining < BlockSize)
      {
        // It's lower, so the cast is safe
        BlockSize = nb::ToUInt32(Remaining);
      }
    }
    const bool Result = (BlockSize > 0);
    if (Result)
    {
      InitRequest(Request, FTransferred, BlockSize);
      Request->Token = nb::ToPtr(BlockSize);
      FTransferred += BlockSize;
    }
    return Result;
  }

  void InitRequest(TSFTPPacket * Request, int64_t Offset,
    uint32_t Size) const
  {
    Request->ChangeType(SSH_FXP_READ);
    Request->AddString(FHandle);
    Request->AddInt64(Offset);
    Request->AddCardinal(Size);
  }

  virtual bool End(TSFTPPacket * Response) override
  {
    return (Response->GetType() != SSH_FXP_DATA);
  }

private:
  TFileOperationProgressType * OperationProgress{nullptr};
  int64_t FOffset{0};
  int64_t FTransferred{0};
  int64_t FPartSize{0};
  RawByteString FHandle;
};

class TSFTPUploadQueue final : public TSFTPAsynchronousQueue
{
  NB_DISABLE_COPY(TSFTPUploadQueue)
public:
  TSFTPUploadQueue() = delete;
  explicit TSFTPUploadQueue(TSFTPFileSystem * AFileSystem, uint32_t ACodePage, TEncryption * Encryption) noexcept :
    TSFTPAsynchronousQueue(AFileSystem, ACodePage),
    FEncryption(Encryption)
  {
    FStream = nullptr;
    FOnTransferIn = nullptr;
    OperationProgress = nullptr;
    FLastBlockSize = 0;
    FEnd = false;
    FConvertToken = false;
  }

  virtual ~TSFTPUploadQueue() noexcept override = default;
  /*{
//    SAFE_DESTROY(FStream);
  }*/

  bool Init(const UnicodeString & AFileName,
    HANDLE AFile, TTransferInEvent && OnTransferIn, TFileOperationProgressType * AOperationProgress,
    const RawByteString & AHandle, int64_t ATransferred,
    int32_t ConvertParams)
  {
    FFileName = AFileName;
    if (OnTransferIn == nullptr)
    {
      FStream = std::make_unique<TSafeHandleStream>(AFile);
    }
    OperationProgress = AOperationProgress;
    FHandle = AHandle;
    FOnTransferIn = OnTransferIn;
    FTransferred = ATransferred;
    FConvertParams = ConvertParams;

    return TSFTPAsynchronousQueue::Init();
  }

  void DisposeSafeWithErrorHandling()
  {
    DisposeSafe(SSH_FXP_STATUS);
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * ARequest) override
  {
    FTerminal = FFileSystem->FTerminal;
    // Buffer for one block of data
    TFileBuffer BlockBuf;

    const uint32_t BlockSize = GetBlockSize();
    bool Result = (BlockSize > 0);

    if (Result)
    {
      bool Last;
      if (FOnTransferIn)
      {
        const DWORD Read = BlockBuf.LoadFromIn(std::forward<TTransferInEvent>(FOnTransferIn), FTerminal, BlockSize);
        Last = (Read < nb::ToDWord(BlockSize));
      }
      else
      {
        FILE_OPERATION_LOOP_BEGIN
        {
          BlockBuf.LoadStream(FStream.get(), BlockSize, false);
        }
        FILE_OPERATION_LOOP_END(FMTLOAD(READ_ERROR, FFileName));
        Last = (FStream->Position >= FStream->Size);
      }

      FEnd = (BlockBuf.GetSize() == 0);
      Result = !FEnd;
      if (Result)
      {
        OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

        // We do ASCII transfer: convert EOL of current block
        if (OperationProgress->GetAsciiTransfer())
        {
          const int64_t PrevBufSize = BlockBuf.GetSize();
          BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
            FFileSystem->GetEOL(), FConvertParams, FConvertToken);
          // update transfer size with difference raised from EOL conversion
          OperationProgress->ChangeTransferSize(OperationProgress->GetTransferSize() -
            PrevBufSize + BlockBuf.GetSize());
        }

        if (FFileSystem->FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
        {
          FFileSystem->FTerminal->LogEvent(FORMAT("Write request offset: %d, len: %d",
            nb::ToInt32(FTransferred), nb::ToInt32(BlockBuf.GetSize())));
        }

        if (FEncryption != nullptr)
        {
          FEncryption->Encrypt(BlockBuf, Last);
        }

        ARequest->ChangeType(SSH_FXP_WRITE);
        ARequest->AddString(FHandle);
        ARequest->AddInt64(FTransferred);
        ARequest->AddData(BlockBuf.GetData(), nb::ToInt32(BlockBuf.GetSize()));
        FLastBlockSize = nb::ToUInt32(BlockBuf.GetSize());

        FTransferred += BlockBuf.GetSize();
      }
    }

    FTerminal = nullptr;
    return Result;
  }

  virtual void SendPacket(TSFTPQueuePacket * APacket) override
  {
    TSFTPAsynchronousQueue::SendPacket(APacket);
    OperationProgress->AddTransferred(FLastBlockSize);
  }

  virtual void ReceiveResponse(
    const TSFTPPacket * APacket, TSFTPPacket * AResponse, SSH_FXP_TYPE ExpectedType = -1,
    SSH_FX_TYPE AllowStatus = -1, bool TryOnly = false) override
  {
    TSFTPAsynchronousQueue::ReceiveResponse(APacket, AResponse, ExpectedType, AllowStatus, TryOnly);
    if (AResponse->GetCapacity() > 0)
    {
      // particularly when uploading a file that completely fits into send buffer
      // over slow line, we may end up seemingly completing the transfer immediately
      // but hanging the application for a long time waiting for responses
      // (common is that the progress window would not even manage to draw itself,
      // showing that upload finished, before the application "hangs")
      FFileSystem->Progress(OperationProgress);
    }
    else
    {
      DebugAssert(TryOnly);
    }
  }

  virtual bool ReceivePacketAsynchronously() override
  {
    // do not read response to close request
    const bool Result = (FRequests->GetCount() > 0);
    if (Result)
    {
      // Try only: We cannot read from the socket here as we are already called
      // from TSecureShell::HandleNetworkEvents as it would cause a recursion
      // that would potentially make PuTTY code process the SSH packets in wrong order.
      ReceivePacket(nullptr, SSH_FXP_STATUS, -1, nullptr, true);
    }
    return Result;
  }

  uint32_t GetBlockSize() const
  {
    return FFileSystem->UploadBlockSize(FHandle, OperationProgress);
  }

  virtual bool End(TSFTPPacket * /*Response*/) override
  {
    return FEnd;
  }

private:
  std::unique_ptr<TStream> FStream;
  TTransferInEvent FOnTransferIn;
  TTerminal * FTerminal{nullptr};
  TFileOperationProgressType * OperationProgress{nullptr};
  UnicodeString FFileName;
  uint32_t FLastBlockSize{0};
  bool FEnd{false};
  int64_t FTransferred{0};
  RawByteString FHandle;
  bool FConvertToken{false};
  int32_t FConvertParams{0};
  TEncryption * FEncryption{nullptr};
};

class TSFTPLoadFilesPropertiesQueue final : public TSFTPFixedLenQueue
{
  NB_DISABLE_COPY(TSFTPLoadFilesPropertiesQueue)
public:
  TSFTPLoadFilesPropertiesQueue() = delete;
  explicit TSFTPLoadFilesPropertiesQueue(TSFTPFileSystem * AFileSystem, uint32_t CodePage) noexcept :
    TSFTPFixedLenQueue(AFileSystem, CodePage)
  {
    FIndex = 0;
  }
  virtual ~TSFTPLoadFilesPropertiesQueue() override = default;

  bool Init(int32_t QueueLen, TStrings * AFileList)
  {
    FFileList = AFileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool ReceivePacket(TSFTPPacket * Packet, TRemoteFile *& File)
  {
    void * Token{nullptr};
    const bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_ATTRS, asAll, &Token);
    File = static_cast<TRemoteFile *>(Token);
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * ARequest) override
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile * File = FFileList->GetAs<TRemoteFile>(FIndex);
      ++FIndex;

      const bool MissingRights =
        (FFileSystem->FSupport->Loaded &&
         FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS) &&
         File->GetRights()->GetUnknown());
      const bool MissingOwnerGroup =
        (FFileSystem->FSecureShell->GetSshImplementation() == sshiBitvise) ||
        ((FFileSystem->FSupport->Loaded &&
         FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP) &&
         !File->GetFileOwner().GetIsSet()) || !File->GetFileGroup().GetIsSet());

      Result = (MissingRights || MissingOwnerGroup);
      if (Result)
      {
        ARequest->ChangeType(SSH_FXP_LSTAT);
        FFileSystem->AddPathString(*ARequest, FFileSystem->LocalCanonify(File->FileName));
        if (FFileSystem->FVersion >= 4)
        {
          ARequest->AddCardinal(
            FLAGMASK(MissingRights, SSH_FILEXFER_ATTR_PERMISSIONS) |
            FLAGMASK(MissingOwnerGroup, SSH_FILEXFER_ATTR_OWNERGROUP));
        }
        ARequest->Token = File;
      }
    }

    return Result;
  }

  virtual bool SendRequest() override
  {
    const bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool End(TSFTPPacket * /*Response*/) override
  {
    return (FRequests->GetCount() == 0);
  }

private:
  TStrings * FFileList{nullptr};
  int32_t FIndex{0};
};

class TSFTPCalculateFilesChecksumQueue final : public TSFTPFixedLenQueue
{
  NB_DISABLE_COPY(TSFTPCalculateFilesChecksumQueue)
public:
  TSFTPCalculateFilesChecksumQueue() = delete;
  explicit TSFTPCalculateFilesChecksumQueue(TSFTPFileSystem * AFileSystem, uint32_t CodePage) noexcept :
    TSFTPFixedLenQueue(AFileSystem, CodePage)
  {
    FIndex = 0;
  }
  virtual ~TSFTPCalculateFilesChecksumQueue() override = default;

  bool Init(int32_t QueueLen, const UnicodeString & Alg, TStrings * AFileList)
  {
    FAlg = Alg;
    FFileList = AFileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool ReceivePacket(TSFTPPacket * APacket, TRemoteFile *& AFile)
  {
    void * Token = nullptr;
    bool Result;
    try__finally
    {
      Result = TSFTPFixedLenQueue::ReceivePacket(APacket, SSH_FXP_EXTENDED_REPLY, asNo, &Token);
    }
    __finally
    {
       AFile = static_cast<TRemoteFile *>(Token);
    } end_try__finally
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * ARequest) override
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile * File = FFileList->GetAs<TRemoteFile>(FIndex);
      DebugAssert(File != nullptr);
      ++FIndex;
      if (!File)
        continue;

      Result = !File->GetIsDirectory();
      if (Result)
      {
        DebugAssert(IsRealFile(File->FileName));

        ARequest->ChangeType(SSH_FXP_EXTENDED);
        ARequest->AddString(SFTP_EXT_CHECK_FILE_NAME);
        FFileSystem->AddPathString(*ARequest, FFileSystem->LocalCanonify(File->FullFileName));
        ARequest->AddString(FAlg);
        ARequest->AddInt64(0); // offset
        ARequest->AddInt64(0); // length (0 = till end)
        ARequest->AddCardinal(0); // block size (0 = no blocks or "one block")

        ARequest->Token = File;
      }
    }

    return Result;
  }

  virtual bool SendRequest() override
  {
    const bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool End(TSFTPPacket * /*Response*/) override
  {
    return (FRequests->GetCount() == 0);
  }

private:
  UnicodeString FAlg;
  TStrings * FFileList{nullptr};
  int32_t FIndex{0};
};

// #pragma warn .inl

class TSFTPBusy final : public TObject
{
  NB_DISABLE_COPY(TSFTPBusy)
public:
  TSFTPBusy() = delete;
  explicit TSFTPBusy(TSFTPFileSystem * AFileSystem) noexcept :
    FFileSystem(AFileSystem)
  {
    FFileSystem = AFileSystem;
    DebugAssert(FFileSystem != nullptr);
    FFileSystem->BusyStart();
  }

  virtual ~TSFTPBusy() noexcept override
  {
    FFileSystem->BusyEnd();
  }

private:
  TSFTPFileSystem * FFileSystem{nullptr};
};
#if 0
//===========================================================================
moved to FileSystems.h
struct TOpenRemoteFileParams
{
  UnicodeString FileName;
  UnicodeString RemoteFileName;
  TFileOperationProgressType * OperationProgress;
  const TCopyParamType * CopyParam;
  int Params;
  bool Resume;
  bool Resuming;
  TOverwriteMode OverwriteMode;
  int64_t DestFileSize; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams * FileParams;
  bool Confirmed;
  bool DontRecycle;
  bool Recycled;
  TRights RecycledRights;
};
#endif // #if 0
//===========================================================================
TSFTPFileSystem::TSFTPFileSystem(TTerminal * ATerminal) noexcept :
  TCustomFileSystem(OBJECT_CLASS_TSFTPFileSystem, ATerminal)
{
  FCodePage = GetSessionData()->GetCodePageAsNumber();
}

void TSFTPFileSystem::Init(void * Data /* TSecureShell*/)
{
  DebugAssert(Data);
  FSecureShell = static_cast<TSecureShell *>(Data);
  DebugAssert(FSecureShell);
  FFileSystemInfoValid = false;
  FVersion = nb::NPOS;
  FPacketReservations = std::make_unique<TList>();
  ResetConnection();
  FBusy = 0;
  FAvoidBusy = false;
  FUtfStrings = asOff;
  FUtfDisablingAnnounced = true;
  FSignedTS = false;
  FSupport = std::make_unique<TSFTPSupport>();
  FFixedPaths = nullptr;
  FFileSystemInfoValid = false;

  FChecksumAlgs = std::make_unique<TStringList>();
  FChecksumSftpAlgs = std::make_unique<TStringList>();
  // List as defined by draft-ietf-secsh-filexfer-extensions-00
  // MD5 moved to the back
  RegisterChecksumAlg(Sha1ChecksumAlg, "sha1");
  RegisterChecksumAlg(Sha224ChecksumAlg, "sha224");
  RegisterChecksumAlg(Sha256ChecksumAlg, "sha256");
  RegisterChecksumAlg(Sha384ChecksumAlg, "sha384");
  RegisterChecksumAlg(Sha512ChecksumAlg, "sha512");
  RegisterChecksumAlg(Md5ChecksumAlg, "md5");
  RegisterChecksumAlg(Crc32ChecksumAlg, "crc32");
}

TSFTPFileSystem::~TSFTPFileSystem() noexcept
{
//  SAFE_DESTROY(FSupport);
  NoPacketReservations();
//  SAFE_DESTROY(FPacketReservations);
//  SAFE_DESTROY(FFixedPaths);
  SAFE_DESTROY(FSecureShell);
}

void TSFTPFileSystem::Open()
{
  NoPacketReservations();
  ResetConnection();
  // this is used for reconnects only
  FSecureShell->Open();
}

void TSFTPFileSystem::NoPacketReservations()
{
  // After closing, we can only possibly have "discard" reservations of the not-read responses to the last requests
  // (typically to SSH_FXP_CLOSE)
  for (int32_t I = 0; I < FPacketReservations->Count; I++)
  {
    DebugAssert(FPacketReservations->GetItem(I) == nullptr);
  }
}

void TSFTPFileSystem::Close()
{
  FSecureShell->Close();
}

bool TSFTPFileSystem::GetActive() const
{
  return FSecureShell->GetActive();
}

void TSFTPFileSystem::CollectUsage()
{
  FSecureShell->CollectUsage();

  UnicodeString VersionCounter;
  switch (FVersion)
  {
    case 0:
      VersionCounter = L"OpenedSessionsSFTP0";
      break;
    case 1:
      VersionCounter = L"OpenedSessionsSFTP1";
      break;
    case 2:
      VersionCounter = L"OpenedSessionsSFTP2";
      break;
    case 3:
      VersionCounter = L"OpenedSessionsSFTP3";
      break;
    case 4:
      VersionCounter = L"OpenedSessionsSFTP4";
      break;
    case 5:
      VersionCounter = L"OpenedSessionsSFTP5";
      break;
    case 6:
      VersionCounter = L"OpenedSessionsSFTP6";
      break;
    default:
      DebugFail();
  }
  FTerminal->Configuration->Usage->Inc(VersionCounter);
}

const TSessionInfo & TSFTPFileSystem::GetSessionInfo() const
{
  return FSecureShell->GetSessionInfo();
}

const TFileSystemInfo & TSFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  if (!FFileSystemInfoValid)
  {
    FFileSystemInfo.AdditionalInfo.Clear();

    if (!IsCapable(fcRename))
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(FS_RENAME_NOT_SUPPORTED) + L"\r\n\r\n";
    }

    if (!FExtensions.IsEmpty())
    {
      FFileSystemInfo.AdditionalInfo +=
        LoadStr(SFTP_EXTENSION_INFO) + L"\r\n" +
        FExtensions;
    }
    else
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_NO_EXTENSION_INFO) + L"\r\n";
    }

    FFileSystemInfo.ProtocolBaseName = L"SFTP";
    FFileSystemInfo.ProtocolName = FMTLOAD(SFTP_PROTOCOL_NAME2, FVersion);
    FTerminal->SaveCapabilities(FFileSystemInfo);

    FFileSystemInfoValid = true;
  }

  return FFileSystemInfo;
}

bool TSFTPFileSystem::TemporaryTransferFile(const UnicodeString & AFileName)
{
  return (base::GetPartialFileExtLen(AFileName) > 0);
}

bool TSFTPFileSystem::GetStoredCredentialsTried() const
{
  return FSecureShell->GetStoredCredentialsTried();
}

UnicodeString TSFTPFileSystem::GetUserName() const
{
  return FSecureShell->ShellGetUserName();
}

void TSFTPFileSystem::Idle()
{
  // Keep session alive
  if ((GetSessionData()->GetPingType() != ptOff) &&
      ((Now() - FSecureShell->GetLastDataSent()) > GetSessionData()->GetPingIntervalDT()))
  {
    if ((GetSessionData()->GetPingType() == ptDummyCommand) &&
        FSecureShell->GetReady())
    {
      FTerminal->LogEvent("Sending dummy command to keep session alive.");
      TSFTPPacket Packet(SSH_FXP_REALPATH, FCodePage);
      AddPathString(Packet, L"/");
      SendPacketAndReceiveResponse(&Packet, &Packet);
    }
    else
    {
      FSecureShell->KeepAlive();
    }
  }

  FSecureShell->Idle();
}

void TSFTPFileSystem::ResetConnection()
{
  FPacketReservations->Clear();
  // FPacketNumbers = VarArrayCreate(OPENARRAY(int, (0, 1)), varLongWord);
  FPacketNumbers.clear();
  FNotLoggedRequests.clear();
  FPreviousLoggedPacket = 0;
  FNotLoggedWritePackets = FNotLoggedReadPackets = FNotLoggedStatusPackets = FNotLoggedDataPackets = 0;
}

bool TSFTPFileSystem::IsCapable(int32_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability) {
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcLocking:
    case fcAclChangingFiles: // pending implementation
    case fcMoveOverExistingFile:
      return false;

    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
    case fcPreservingTimestampUpload:
    case fcSecondaryShell:
    case fcRemoveCtrlZUpload:
    case fcRemoveBOMUpload:
    case fcPreservingTimestampDirs:
    case fcTransferOut:
    case fcTransferIn:
      return true;

    case fcMoveToQueue:
    case fcResumeSupport:
    case fcSkipTransfer:
    case fcParallelTransfers:
    case fcParallelFileTransfers:
      return !FTerminal->IsEncryptingFiles();

    case fcRename:
    case fcRemoteMove:
      return (FVersion >= 2);

    case fcSymbolicLink:
    case fcResolveSymlink:
      return (FVersion >= 3) && !FTerminal->IsEncryptingFiles();

    case fcModeChanging:
    case fcModeChangingUpload:
      return !FSupport->Loaded ||
        FLAGSET(FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS);

    case fcGroupOwnerChangingByID:
      return (FVersion <= 3);

    case fcOwnerChanging:
    case fcGroupChanging:
      return
        ((FVersion >= 4) &&
         (!FSupport->Loaded ||
          FLAGSET(FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP)));

    case fcNativeTextMode:
      return !FTerminal->IsEncryptingFiles() && (FVersion >= 4);

    case fcTextMode:
      return
        !FTerminal->IsEncryptingFiles() &&
        ((FVersion >= 4) ||
         (strcmp(GetEOL(), EOLToStr(FTerminal->Configuration->LocalEOLType())) != 0));

    case fcUserGroupListing:
      return SupportsExtension(SFTP_EXT_OWNER_GROUP);

    case fcLoadingAdditionalProperties:
      // We allow loading properties only, if "supported" extension is supported and
      // the server supports "permissions" and/or "owner/group" attributes
      // (no other attributes are loaded).
      // This is here only because of VShell
      // (it supports owner/group, but does not include them into response to
      // SSH_FXP_READDIR)
      // and Bitvise (the same as VShell, but it does not even bother to provide "supported" extension until 6.21)
      // No other use is known.
      return
        (FSupport->Loaded &&
         ((FSupport->AttributeMask &
           (SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_OWNERGROUP)) != 0)) ||
        (FSecureShell->FSshImplementation == sshiBitvise);

    case fcCheckingSpaceAvailable:
      return
        // extension announced in extension list of by
        // SFTP_EXT_SUPPORTED/SFTP_EXT_SUPPORTED2 extension
        // (SFTP version 5 and newer only)
        SupportsExtension(SFTP_EXT_SPACE_AVAILABLE) ||
        // extension announced by proprietary SFTP_EXT_STATVFS extension
        FSupportsStatVfsV2 ||
        // Bitvise (until 6.21) fails to report it's supported extensions.
        (FSecureShell->GetSshImplementation() == sshiBitvise);

    case fcCalculatingChecksum:
      return
        !FTerminal->IsEncryptingFiles() &&
        (// Specification says that "check-file" should be announced,
         // yet Vandyke VShell (as of 4.0.3) announce "check-file-name"
         SupportsExtension(SFTP_EXT_CHECK_FILE) ||
         SupportsExtension(SFTP_EXT_CHECK_FILE_NAME) ||
         // see above
         (FSecureShell->GetSshImplementation() == sshiBitvise));

    case fcRemoteCopy:
      return
        // Implemented by ProFTPD/mod_sftp, OpenSSH (since 9.0) and Bitvise WinSSHD (without announcing it)
        SupportsExtension(SFTP_EXT_COPY_FILE) ||
        SupportsExtension(SFTP_EXT_COPY_DATA) ||
        // see above
        (FSecureShell->GetSshImplementation() == sshiBitvise);

    case fcHardLink:
      return
        (FVersion >= 6) ||
        FSupportsHardlink;

    case fcChangePassword:
      return FSecureShell->CanChangePassword();

    default:
      DebugFail();
      return false;
  }
}

bool TSFTPFileSystem::SupportsExtension(const UnicodeString & Extension) const
{
  return (FSupportedExtensions->IndexOf(Extension) >= 0);
}

void TSFTPFileSystem::BusyStart()
{
  if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
  {
    FBusyToken = ::BusyStart();
  }
  FBusy++;
  DebugAssert(FBusy < 10);
}

void TSFTPFileSystem::BusyEnd()
{
  DebugAssert(FBusy > 0);
  FBusy--;
  if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
  {
    ::BusyEnd(FBusyToken);
    FBusyToken = nullptr;
  }
}

// size + message number + type
constexpr uint32_t SFTPPacketOverhead = 4 + 4 + 1;

uint32_t TSFTPFileSystem::TransferBlockSize(
  uint32_t Overhead, TFileOperationProgressType * OperationProgress,
  uint32_t AMinPacketSize, uint32_t AMaxPacketSize) const
{
  constexpr uint32_t MinPacketSize = 32 * 1024;
  uint32_t MaxPacketSize = FSecureShell->MaxPacketSize();
  bool MaxPacketSizeValid = (MaxPacketSize > 0);
  const int64_t CPSRounded = TEncryption::RoundToBlock(nb::ToInt64(OperationProgress->CPS()));
  uint32_t Result = nb::ToUInt32(CPSRounded);
  if ((FMaxPacketSize > 0) &&
      ((FMaxPacketSize < MaxPacketSize) || !MaxPacketSizeValid))
  {
    MaxPacketSize = FMaxPacketSize;
    MaxPacketSizeValid = true;
  }

  if (Result == 0)
  {
    Result = nb::ToUInt32(TFileOperationProgressType::StaticBlockSize());
  }

  if (Result < MinPacketSize)
  {
    Result = MinPacketSize;
  }

  if (MaxPacketSizeValid)
  {
    Overhead += SFTPPacketOverhead;
    if (MaxPacketSize < Overhead)
    {
      // do not send another request
      // (generally should happen only if upload buffer is full)
      Result = 0;
    }
    else
    {
      MaxPacketSize -= Overhead;
      if (Result > MaxPacketSize)
      {
        const int64_t MaxPacketSizeRounded = TEncryption::RoundToBlockDown(MaxPacketSize);
        if (MaxPacketSizeRounded > 0)
        {
          MaxPacketSize = nb::ToUInt32(MaxPacketSizeRounded);
        }
        Result = MaxPacketSize;
      }
    }
  }

  Result = nb::ToUInt32(OperationProgress->AdjustToCPSLimit(Result));

  return Result;
}

uint32_t TSFTPFileSystem::UploadBlockSize(const RawByteString & Handle,
  TFileOperationProgressType * OperationProgress) const
{
  // handle length + offset + data size
  constexpr uint32_t UploadPacketOverhead =
    sizeof(uint32_t) + sizeof(int64_t) + sizeof(uint32_t);
  return TransferBlockSize(nb::ToUInt32(UploadPacketOverhead + Handle.Length()), OperationProgress,
      nb::ToUInt32(GetSessionData()->GetSFTPMinPacketSize()),
      nb::ToUInt32(GetSessionData()->GetSFTPMaxPacketSize()));
}

uint32_t TSFTPFileSystem::DownloadBlockSize(
  TFileOperationProgressType * OperationProgress) const
{
  uint32_t Result = TransferBlockSize(sizeof(uint32_t), OperationProgress,
      nb::ToUInt32(GetSessionData()->GetSFTPMinPacketSize()),
      nb::ToUInt32(GetSessionData()->GetSFTPMaxPacketSize()));
  if (FSupport->Loaded && (FSupport->MaxReadSize > 0) &&
      (Result > FSupport->MaxReadSize))
  {
    Result = FSupport->MaxReadSize;
  }
  // Never ask for more than we can accept (overhead here should correctly not include the "size" field)
  if (Result + SFTPPacketOverhead > SFTP_MAX_PACKET_LEN)
  {
    Result = SFTP_MAX_PACKET_LEN - SFTPPacketOverhead;
  }
  return Result;
}

void TSFTPFileSystem::Progress(TFileOperationProgressType * OperationProgress)
{
  FTerminal->Progress(OperationProgress);
}

void TSFTPFileSystem::LogPacket(const TSFTPPacket * Packet, TLogLineType Type)
{
  nb::vector_t<UnicodeString> NotLogged(4);
  #define ADD_NOT_LOGGED(V, N) \
    do { if (FNotLogged##V##Packets > 0) \
    { \
      NotLogged.emplace_back(FORMAT(L"%d SSH_FXP_"#N, FNotLogged##V##Packets)); \
      FNotLogged##V##Packets = 0; \
    } } while(0)
  ADD_NOT_LOGGED(Write, WRITE);
  ADD_NOT_LOGGED(Read, READ);
  ADD_NOT_LOGGED(Data, DATA);
  ADD_NOT_LOGGED(Status, STATUS);
  if (!NotLogged.empty())
  {
    UnicodeString S;
    for (size_t Index = 0; Index < NotLogged.size(); Index++)
    {
      AddToList(S, NotLogged[Index], (Index < NotLogged.size() - 1 ? L", " : L" and "));
    }
    FTerminal->LogEvent(FORMAT(L"Skipped %s packets", S));
  }
  FTerminal->Log->Add(
    Type, FORMAT(L"Type: %s, Size: %d, Number: %d", Packet->TypeName(), nb::ToInt32(Packet->GetLength()), Packet->GetMessageNumber()));
  if (FTerminal->Configuration->ActualLogProtocol >= 2)
  {
    FTerminal->Log->Add(Type, Packet->Dump());
  }
}

void TSFTPFileSystem::SendPacket(const TSFTPPacket * Packet)
{
  // putting here for a lack of better place
  if (!FUtfDisablingAnnounced && (FUtfStrings == asOff))
  {
    FTerminal->LogEvent("Strings received in non-UTF-8 encoding in a previous packet, will not use UTF-8 anymore");
    FUtfDisablingAnnounced = true;
  }

  BusyStart();
  try__finally
  {
    if (FTerminal->Log->Logging && (FTerminal->Configuration->ActualLogProtocol >= 0))
    {
      if ((FPreviousLoggedPacket != SSH_FXP_READ &&
           FPreviousLoggedPacket != SSH_FXP_WRITE) ||
          (Packet->GetType() != FPreviousLoggedPacket) ||
          (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
      {
        LogPacket(Packet, llInput);
        FPreviousLoggedPacket = Packet->GetType();
      }
      else
      {
        if (Packet->GetType() == SSH_FXP_WRITE)
        {
          FNotLoggedWritePackets++;
        }
        else if (DebugAlwaysTrue(Packet->GetType() == SSH_FXP_READ))
        {
          FNotLoggedReadPackets++;
        }
        FNotLoggedRequests.emplace(Packet->MessageNumber());
      }
    }
    FSecureShell->Send(Packet->GetSendData(), Packet->GetSendLength());
  }
  __finally
  {
    this->BusyEnd();
  } end_try__finally
}

SSH_FX_TYPE TSFTPFileSystem::GotStatusPacket(
  TSFTPPacket * Packet, SSH_FX_TYPE AllowStatus, bool DoNotForceLog)
{
  const SSH_FX_TYPE Code = static_cast<SSH_FX_TYPE>(Packet->GetCardinal());

  static int32_t Messages[] = {
    SFTP_STATUS_OK,
    SFTP_STATUS_EOF,
    SFTP_STATUS_NO_SUCH_FILE,
    SFTP_STATUS_PERMISSION_DENIED,
    SFTP_STATUS_FAILURE,
    SFTP_STATUS_BAD_MESSAGE,
    SFTP_STATUS_NO_CONNECTION,
    SFTP_STATUS_CONNECTION_LOST,
    SFTP_STATUS_OP_UNSUPPORTED,
    SFTP_STATUS_INVALID_HANDLE,
    SFTP_STATUS_NO_SUCH_PATH,
    SFTP_STATUS_FILE_ALREADY_EXISTS,
    SFTP_STATUS_WRITE_PROTECT,
    SFTP_STATUS_NO_MEDIA,
    SFTP_STATUS_NO_SPACE_ON_FILESYSTEM,
    SFTP_STATUS_QUOTA_EXCEEDED,
    SFTP_STATUS_UNKNOWN_PRINCIPAL,
    SFTP_STATUS_LOCK_CONFLICT,
    SFTP_STATUS_DIR_NOT_EMPTY,
    SFTP_STATUS_NOT_A_DIRECTORY,
    SFTP_STATUS_INVALID_FILENAME,
    SFTP_STATUS_LINK_LOOP,
    SFTP_STATUS_CANNOT_DELETE,
    SFTP_STATUS_INVALID_PARAMETER,
    SFTP_STATUS_FILE_IS_A_DIRECTORY,
    SFTP_STATUS_BYTE_RANGE_LOCK_CONFLICT,
    SFTP_STATUS_BYTE_RANGE_LOCK_REFUSED,
    SFTP_STATUS_DELETE_PENDING,
    SFTP_STATUS_FILE_CORRUPT,
    SFTP_STATUS_OWNER_INVALID,
    SFTP_STATUS_GROUP_INVALID,
    SFTP_STATUS_NO_MATCHING_BYTE_RANGE_LOCK
  };

  if ((AllowStatus & (0x01LL << Code)) == 0)
  {
    int32_t Message;
    if (nb::ToUInt32(Code) >= _countof(Messages))
    {
      Message = SFTP_STATUS_UNKNOWN;
    }
    else
    {
      Message = Messages[Code];
    }
    UnicodeString MessageStr = LoadStr(Message);
    UnicodeString ServerMessage;
    UnicodeString LanguageTag;
    if ((FVersion >= 3) ||
        // if version is not decided yet (i.e. this is status response
        // to the init request), go on, only if there are any more data
        ((FVersion < 0) && (Packet->GetRemainingLength() > 0)))
    {
      // message is in UTF only since SFTP specification 01 (specification 00
      // is also version 3)
      // (in other words, always use UTF unless server is known to be buggy)
      // ServerMessage = Packet->GetString(FUtfStrings);
      // SSH-2.0-Maverick_SSHD and SSH-2.0-CIGNA SFTP Server Ready! omit the language tag
      // and I believe I've seen one more server doing the same.
      if (Packet->GetRemainingLength() > 0)
      {
        ServerMessage = Packet->GetString(FUtfStrings);
        LanguageTag = Packet->GetAnsiString();
        if ((FVersion >= 5) && (Message == SFTP_STATUS_UNKNOWN_PRINCIPAL))
        {
          UnicodeString Principals;
          while (Packet->GetNextData() != nullptr)
          {
            if (!Principals.IsEmpty())
            {
              Principals += L", ";
            }
            Principals += Packet->GetAnsiString();
          }
          MessageStr = FORMAT(MessageStr, Principals);
        }
      }
    }
    else
    {
      ServerMessage = LoadStr(SFTP_SERVER_MESSAGE_UNSUPPORTED);
    }
    if (FTerminal->Log->Logging &&
        (FTerminal->Configuration->ActualLogProtocol >= 0))
    {
      FTerminal->GetLog()->Add(llOutput, FORMAT("Status code: %d, Message: %d, Server: %s, Language: %s ",
        nb::ToInt32(Code), nb::ToInt32(Packet->GetMessageNumber()), ServerMessage, LanguageTag));
    }
    if (!LanguageTag.IsEmpty())
    {
      LanguageTag = FORMAT(" (%s)", LanguageTag);
    }
    UnicodeString HelpKeyword;
    switch (Code)
    {
      case SSH_FX_FAILURE:
        HelpKeyword = HELP_SFTP_STATUS_FAILURE;
        break;

      case SSH_FX_PERMISSION_DENIED:
        HelpKeyword = HELP_SFTP_STATUS_PERMISSION_DENIED;
        break;
    }
    UnicodeString Error = FMTLOAD(SFTP_ERROR_FORMAT3, MessageStr,
      Code, LanguageTag, ServerMessage);
    if (Code == SSH_FX_FAILURE)
    {
      // FTerminal->Configuration->Usage->Inc("SftpFailureErrors");
      Error += L"\n\n" + LoadStr(SFTP_STATUS_4);
    }
    FTerminal->TerminalError(nullptr, Error, HelpKeyword);
    return 0;
  }
  else
  {
    if (!DoNotForceLog || (Code != 0))
    {
      if (FTerminal->Configuration->ActualLogProtocol >= 0)
      {
        FTerminal->Log->Add(llOutput, FORMAT("Status code: %d", Code));
      }
    }
  }
  return Code;
}

void TSFTPFileSystem::RemoveReservation(int32_t Reservation)
{
  for (int32_t Index = Reservation + 1; Index < FPacketReservations->GetCount(); ++Index)
  {
    FPacketNumbers[Index - 1] = FPacketNumbers[Index];
  }
  TSFTPPacket * Packet = FPacketReservations->GetAs<TSFTPPacket>(Reservation);
  if (Packet)
  {
    DebugAssert((Packet->GetReservedBy() == nullptr) || (Packet->GetReservedBy() == this));
    Packet->SetReservedBy(nullptr);
  }
  FPacketReservations->Delete(Reservation);
}

int32_t TSFTPFileSystem::PacketLength(uint8_t * LenBuf, SSH_FXP_TYPE ExpectedType) const
{
  const int32_t Length = GET_32BIT(LenBuf);
  if (Length > SFTP_MAX_PACKET_LEN)
  {
    UnicodeString Message = FMTLOAD(SFTP_PACKET_TOO_BIG,
      nb::ToInt32(Length), SFTP_MAX_PACKET_LEN);
    if (ExpectedType == SSH_FXP_VERSION)
    {
      const RawByteString LenString(reinterpret_cast<char *>(LenBuf), 4);
      Message = FMTLOAD(SFTP_PACKET_TOO_BIG_INIT_EXPLAIN,
        Message, DisplayableStr(LenString));
    }
    FTerminal->FatalError(nullptr, Message, HELP_SFTP_PACKET_TOO_BIG);
  }
  return Length;
}

const TSessionData * TSFTPFileSystem::GetSessionData() const
{
  return FTerminal->GetSessionData();
}

bool TSFTPFileSystem::PeekPacket()
{
  uint8_t * Buf = nullptr;
  bool Result = FSecureShell->Peek(Buf, 4);
  if (Result)
  {
    const int32_t Length = PacketLength(Buf, static_cast<SSH_FXP_TYPE>(-1));
    Result = FSecureShell->Peek(Buf, 4 + Length);
  }
  return Result;
}

SSH_FX_TYPE TSFTPFileSystem::ReceivePacket(TSFTPPacket * Packet,
  SSH_FXP_TYPE ExpectedType, SSH_FX_TYPE AllowStatus, bool TryOnly)
{
  const TSFTPBusy Busy(this);

  SSH_FX_TYPE Result = SSH_FX_OK;
  int32_t Reservation = FPacketReservations->IndexOf(Packet);
  bool NotLogged{false};

  if ((Reservation < 0) || (Packet->GetCapacity() == 0))
  {
    bool IsReserved;
    do
    {
      IsReserved = false;
      NotLogged = false;

      DebugAssert(Packet);

      if (TryOnly && !PeekPacket())
      {
        // Reset packet in case it was filled by previous out-of-order
        // reserved packet
        *Packet = TSFTPPacket(SSH_FXP_NONE, FCodePage);
      }
      else
      {
        uint8_t LenBuf[4]{};
        FSecureShell->Receive(LenBuf, sizeof(LenBuf));
        const int32_t Length = PacketLength(LenBuf, ExpectedType);
        Packet->SetCapacity(Length);
        FSecureShell->Receive(Packet->GetData(), nb::ToSizeT(Length));
        Packet->DataUpdated(Length);

        const bool ResponseToNotLoggedRequest = FNotLoggedRequests.find(Packet->MessageNumber()) != FNotLoggedRequests.end();
        if (ResponseToNotLoggedRequest)
          FNotLoggedRequests.erase(Packet->MessageNumber());
        if (FTerminal->Log->Logging && (FTerminal->Configuration->ActualLogProtocol >= 0))
        {
          if (!ResponseToNotLoggedRequest ||
              (Packet->Type != SSH_FXP_STATUS && Packet->Type != SSH_FXP_DATA) ||
              (FTerminal->Configuration->ActualLogProtocol >= 1))
          {
            LogPacket(Packet, llOutput);
          }
          else
          {
            if (Packet->Type == SSH_FXP_STATUS)
            {
              FNotLoggedStatusPackets++;
            }
            else if (DebugAlwaysTrue(Packet->Type == SSH_FXP_DATA))
            {
              FNotLoggedDataPackets++;
            }
            NotLogged = true; // used only for the response we wait for
          }
        }

        if ((Reservation < 0) ||
            Packet->GetMessageNumber() != FPacketNumbers[Reservation])
        {
          for (int32_t Index = 0; Index < FPacketReservations->GetCount(); ++Index)
          {
            const uint32_t MessageNumber = nb::ToUInt32(FPacketNumbers[Index]);
            if (MessageNumber == Packet->GetMessageNumber())
            {
              TSFTPPacket * ReservedPacket = FPacketReservations->GetAs<TSFTPPacket>(Index);
              IsReserved = true;
              if (ReservedPacket)
              {
                FTerminal->LogEvent(0, L"Storing reserved response");
                *ReservedPacket = *Packet;
              }
              else
              {
                FTerminal->LogEvent(0, L"Discarding reserved response");
                RemoveReservation(Index);
                if ((Reservation >= 0) && (Reservation > Index))
                {
                  Reservation--;
                  DebugAssert(Reservation == FPacketReservations->IndexOf(Packet));
                }
              }
              break;
            }
          }
        }
      }
    }
    while (IsReserved);
  }

  if ((Packet->GetCapacity() == 0) && DebugAlwaysTrue(TryOnly))
  {
    // noop
  }
  else
  {
    // before we removed the reservation after check for packet type,
    // but if it raises exception, removal is unnecessarily
    // postponed until the packet is removed
    // (and it has not worked anyway until recent fix to UnreserveResponse)
    if (Reservation >= 0)
    {
      DebugAssert(Packet->GetMessageNumber() == FPacketNumbers[Reservation]);
      RemoveReservation(Reservation);
    }

    if (ExpectedType != static_cast<SSH_FXP_TYPE>(-1))
    {
      if (Packet->GetType() == SSH_FXP_STATUS)
      {
        if (AllowStatus < 0)
        {
          AllowStatus = (ExpectedType == SSH_FXP_STATUS ? asOK : asNo);
        }
        Result = GotStatusPacket(Packet, AllowStatus, NotLogged);
      }
      else if (ExpectedType != Packet->GetType())
      {
        FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_TYPE, nb::ToInt32(Packet->GetType())));
      }
    }
  }

  return Result;
}

void TSFTPFileSystem::ReserveResponse(const TSFTPPacket * Packet,
  TSFTPPacket * Response)
{
  if (Response != nullptr)
  {
    DebugAssert(FPacketReservations->IndexOf(Response) < 0);
    // mark response as not received yet
    Response->SetCapacity(0);
    Response->SetReservedBy(this);
  }
  FPacketReservations->Add(Response);
  if (nb::ToSizeT(FPacketReservations->GetCount()) >= FPacketNumbers.size())
  {
    FPacketNumbers.resize(FPacketReservations->GetCount() + 10);
  }
  FPacketNumbers[FPacketReservations->GetCount() - 1] = Packet->GetMessageNumber();
}

void TSFTPFileSystem::UnreserveResponse(TSFTPPacket * Response)
{
  const int32_t Reservation = FPacketReservations->IndexOf(Response);
  if (Response->GetCapacity() != 0)
  {
    // added check for already received packet
    // (it happens when the reserved response is received out of order,
    // unexpectedly soon, and then receivepacket() on the packet
    // is not actually called, due to exception)
    RemoveReservation(Reservation);
  }
  else
  {
    if (Reservation >= 0)
    {
      // we probably do not remove the item at all, because
      // we must remember that the response was expected, so we skip it
      // in receivepacket()
      FPacketReservations->SetItem(Reservation, nullptr);
    }
  }
}

SSH_FX_TYPE TSFTPFileSystem::ReceiveResponse(
  const TSFTPPacket * Packet, TSFTPPacket * AResponse, SSH_FXP_TYPE ExpectedType,
  SSH_FX_TYPE AllowStatus, bool TryOnly)
{
  SSH_FX_TYPE Result;
  const uint32_t MessageNumber = Packet->GetMessageNumber();
  TSFTPPacket * Response = (AResponse ? AResponse : new TSFTPPacket(SSH_FXP_NONE, FCodePage));
  try__finally
  {
    Result = ReceivePacket(Response, ExpectedType, AllowStatus, TryOnly);
    if (MessageNumber != Response->GetMessageNumber())
    {
      FTerminal->FatalError(nullptr, FMTLOAD(SFTP_MESSAGE_NUMBER,
        nb::ToInt32(Response->GetMessageNumber()), nb::ToInt32(MessageNumber)));
    }
  }
  __finally
  {
    if (!AResponse)
    {
      delete Response;
    }
  } end_try__finally
  return Result;
}

SSH_FX_TYPE TSFTPFileSystem::SendPacketAndReceiveResponse(
  const TSFTPPacket * Packet, TSFTPPacket * Response, SSH_FXP_TYPE ExpectedType,
  SSH_FX_TYPE AllowStatus)
{
  const TSFTPBusy Busy(this);
  SendPacket(Packet);
  const SSH_FX_TYPE Result = ReceiveResponse(Packet, Response, ExpectedType, AllowStatus);
  return Result;
}

UnicodeString TSFTPFileSystem::GetRealPath(const UnicodeString & APath)
{
  if (FTerminal->SessionData->FSFTPRealPath == asOff)
  {
    return LocalCanonify(APath);
  }
  else
  {
    try
    {
      FTerminal->LogEvent(0, FORMAT("Getting real path for '%s'", APath));

      TSFTPPacket Packet(SSH_FXP_REALPATH, FCodePage);
      AddPathString(Packet, APath);

      // In SFTP-6 new optional field control-byte is added that defaults to
      // SSH_FXP_REALPATH_NO_CHECK=0x01, meaning it won't fail, if the path does not exist.
      // That differs from SFTP-5 recommendation that
      // "The server SHOULD fail the request if the path is not present on the server."
      // Earlier versions had no recommendation, though canonical SFTP-3 implementation
      // in OpenSSH fails.

      // While we really do not care much, we anyway set the flag to ~ & 0x01 to make the request fail.
      // First for consistency.
      // Second to workaround a bug in ProFTPD/mod_sftp version 1.3.5rc1 through 1.3.5-stable
      // that sends a completely malformed response for non-existing paths,
      // when SSH_FXP_REALPATH_NO_CHECK (even implicitly) is used.
      // See http://bugs.proftpd.org/show_bug.cgi?id=4160

      // Note that earlier drafts of SFTP-6 (filexfer-07 and -08) had optional compose-path field
      // before control-byte field. If we ever use this against a server conforming to those drafts,
      // it may cause trouble.
      if (FVersion >= 6)
      {
        if (FSecureShell->GetSshImplementation() != sshiProFTPD)
        {
          Packet.AddByte(SSH_FXP_REALPATH_STAT_ALWAYS);
        }
        else
        {
          // Cannot use SSH_FXP_REALPATH_STAT_ALWAYS as ProFTPD does wrong bitwise test
          // so it incorrectly evaluates SSH_FXP_REALPATH_STAT_ALWAYS (0x03) as
          // SSH_FXP_REALPATH_NO_CHECK (0x01). The only value conforming to the
          // specification, yet working with ProFTPD is SSH_FXP_REALPATH_STAT_IF (0x02).
          Packet.AddByte(SSH_FXP_REALPATH_STAT_IF);
        }
      }
      SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_NAME);
      if (Packet.GetCardinal() != 1)
      {
        FTerminal->FatalError(nullptr, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
      }

      UnicodeString RealDir = base::UnixExcludeTrailingBackslash(Packet.GetPathString(FUtfStrings));
      // do not cache, as particularly when called from CreateDirectory > Canonify,
      // we would cache an unencrypted path to a directory we want to create encrypted,
      // what would prevent the encryption later.
      RealDir = FTerminal->DecryptFileName(RealDir, true, true);
      // ignore rest of SSH_FXP_NAME packet

      FTerminal->LogEvent(0, FORMAT("Real path is '%s'", RealDir));

      return RealDir;
    }
    catch(Exception & E)
    {
      if (FTerminal->GetActive())
      {
        throw ExtException(&E, FMTLOAD(SFTP_REALPATH_ERROR, APath));
      }
      else
      {
        throw;
      }
    }
  }
  return UnicodeString();
}

UnicodeString TSFTPFileSystem::GetRealPath(const UnicodeString & APath, const UnicodeString & ABaseDir)
{
  UnicodeString Path;

  if (base::UnixIsAbsolutePath(APath))
  {
    Path = APath;
  }
  else
  {
    if (!APath.IsEmpty())
    {
      // this condition/block was outside (before) current block
      // but it did not work when Path was empty
      if (!ABaseDir.IsEmpty())
      {
        Path = base::UnixIncludeTrailingBackslash(ABaseDir);
      }
      Path = Path + APath;
    }
    if (Path.IsEmpty())
    {
      Path = base::UnixIncludeTrailingBackslash(L".");
    }
  }
  return GetRealPath(Path);
}

UnicodeString TSFTPFileSystem::LocalCanonify(const UnicodeString & APath) const
{
  TODO("improve (handle .. etc.)")
  if (base::UnixIsAbsolutePath(APath) ||
      (!FCurrentDirectory.IsEmpty() && base::UnixSamePath(FCurrentDirectory, APath)))
  {
    return APath;
  }
  return base::AbsolutePath(FCurrentDirectory, APath);
}

UnicodeString TSFTPFileSystem::Canonify(const UnicodeString & APath)
{
  // inspired by canonify() from PSFTP.C
  UnicodeString Result;
  FTerminal->LogEvent(FORMAT("Canonifying: \"%s\"", APath));
  const UnicodeString Path = LocalCanonify(APath);
  bool TryParent = false;
  try
  {
    Result = GetRealPath(Path);
  }
  catch(...)
  {
    if (FTerminal->GetActive())
    {
      TryParent = true;
    }
    else
    {
      throw;
    }
  }

  if (TryParent)
  {
    const UnicodeString Path2 = base::UnixExcludeTrailingBackslash(Path);
    const UnicodeString Name = base::UnixExtractFileName(Path2);
    if (!IsRealFile(Name))
    {
      Result = Path;
    }
    else
    {
      const UnicodeString Path3 = base::UnixExtractFilePath(Path2);
      try
      {
        Result = GetRealPath(Path3);
        Result = TPath::Join(Result, Name);
      }
      catch(...)
      {
        if (FTerminal->GetActive())
        {
          Result = Path;
        }
        else
        {
          throw;
        }
      }
    }
  }

  FTerminal->LogEvent(FORMAT("Canonified: \"%s\"", Result));

  return Result;
}

UnicodeString TSFTPFileSystem::GetAbsolutePath(const UnicodeString & APath, bool Local) const
{
  return const_cast<TSFTPFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TSFTPFileSystem::GetAbsolutePath(const UnicodeString & APath, bool Local)
{
  if (Local)
  {
    return LocalCanonify(APath);
  }
  return GetRealPath(APath, GetCurrentDirectory());
}

UnicodeString TSFTPFileSystem::GetHomeDirectory()
{
  if (FHomeDirectory.IsEmpty())
  {
    FHomeDirectory = GetRealPath(THISDIRECTORY);
    // Prevent infinite recursion when the server is broken
    if (FHomeDirectory.IsEmpty())
    {
      FHomeDirectory = ROOTDIRECTORY;
    }
  }
  return FHomeDirectory;
}

void TSFTPFileSystem::LoadFile(TRemoteFile * AFile, TSFTPPacket * Packet,
  bool Complete)
{
  Packet->GetFile(AFile, FVersion, FTerminal->SessionData->GetDSTMode(), FUtfStrings, FSignedTS, Complete);
}

TRemoteFile * TSFTPFileSystem::LoadFile(TSFTPPacket * Packet,
  TRemoteFile * ALinkedByFile, const UnicodeString & AFileName,
  TRemoteFileList * TempFileList, bool Complete)
{
  std::unique_ptr<TRemoteFile> File(std::make_unique<TRemoteFile>(ALinkedByFile));
  try__catch
  {
    File->SetTerminal(FTerminal);
    if (!AFileName.IsEmpty())
    {
      File->SetFileName(AFileName);
    }
    // to get full path for symlink completion
    File->SetDirectory(TempFileList);
    LoadFile(File.get(), Packet, Complete);
    File->SetDirectory(nullptr);
  }
  __catch__removed
  {
    // delete File;
    // throw;
  } end_try__catch
  return File.release();
}

UnicodeString TSFTPFileSystem::GetCurrentDirectory() const
{
  return FCurrentDirectory;
}

void TSFTPFileSystem::DoStartup()
{
  // do not know yet
  FVersion = -1;
  FFileSystemInfoValid = false;
  TSFTPPacket Packet1(SSH_FXP_INIT, FCodePage);
  uint32_t MaxVersion = GetSessionData()->GetSFTPMaxVersion();
  if (MaxVersion > SFTPMaxVersion)
  {
    MaxVersion = SFTPMaxVersion;
  }
  Packet1.AddCardinal(nb::ToUInt32(MaxVersion));

  try
  {
    SendPacketAndReceiveResponse(&Packet1, &Packet1, SSH_FXP_VERSION);
  }
  catch(Exception & E)
  {
    FTerminal->FatalError(&E, LoadStr(SFTP_INITIALIZE_ERROR), HELP_SFTP_INITIALIZE_ERROR);
  }

  FVersion = Packet1.GetCardinal();
  FTerminal->LogEvent(FORMAT("SFTP version %d negotiated.", FVersion));
  if (FVersion > MaxVersion)
  {
    // This happens with ProFTPD:
    // https://github.com/proftpd/proftpd/issues/1200
    FTerminal->LogEvent(L"Got higher version than asked for.");
  }
  if (FVersion < SFTPMinVersion || FVersion > SFTPMaxVersion)
  {
    FTerminal->FatalError(nullptr, FMTLOAD(SFTP_VERSION_NOT_SUPPORTED,
      FVersion, SFTPMinVersion, SFTPMaxVersion));
  }

  FExtensions = EmptyStr;
  FEOL = "\r\n";
  FSupport->Loaded = false;
  FSupportsStatVfsV2 = false;
  FSupportsHardlink = false;
  bool SupportsLimits = false;
  FFixedPaths.reset();
  // OpenSSH announce extensions directly in the SSH_FXP_VERSION packet only.
  // Bitvise uses "supported2" extension for some (mostly the standard ones) and SSH_FXP_VERSION for other.
  // ProFTPD uses "supported2" extension for the standard extensions. And repeats them along with non-standard in the SSH_FXP_VERSION.
  std::unique_ptr<TStrings> SupportedExtensions(std::make_unique<TStringList>());

  if (FVersion >= 3)
  {
    while (Packet1.GetNextData() != nullptr)
    {
      const UnicodeString ExtensionName = Packet1.GetAnsiString();
      const RawByteString ExtensionData = Packet1.GetRawByteString();
      const UnicodeString ExtensionDisplayData = DisplayableStr(ExtensionData);

      if (ExtensionName == SFTP_EXT_NEWLINE)
      {
        FEOL = AnsiString(ExtensionData);
        FTerminal->LogEvent(FORMAT("Server requests EOL sequence %s.",
          ExtensionDisplayData));
        if (FEOL.Length() < 1 || FEOL.Length() > 2)
        {
          FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_EOL, ExtensionDisplayData));
        }
      }
      // do not allow "supported" to override "supported2" if both are received
      else if (((ExtensionName == SFTP_EXT_SUPPORTED) && !FSupport->Loaded) ||
               (ExtensionName == SFTP_EXT_SUPPORTED2))
      {
        FSupport->Reset();
        TSFTPPacket SupportedStruct(ExtensionData, FCodePage);
        FSupport->Loaded = true;
        FSupport->AttributeMask = SupportedStruct.GetCardinal();
        FSupport->AttributeBits = SupportedStruct.GetCardinal();
        FSupport->OpenFlags = SupportedStruct.GetCardinal();
        FSupport->AccessMask = SupportedStruct.GetCardinal();
        FSupport->MaxReadSize = SupportedStruct.GetCardinal();
        std::unique_ptr<TStrings> ExtensionsLog(std::make_unique<TStringList>());
        if (ExtensionName == SFTP_EXT_SUPPORTED)
        {
          while (SupportedStruct.GetNextData() != nullptr)
          {
            const UnicodeString Extension = SupportedStruct.GetAnsiString();
            ExtensionsLog->Add(Extension);
            SupportedExtensions->Add(Extension);
          }
        }
        else
        {
          // note that supported-open-block-vector, supported-block-vector,
          // attrib-extension-count and attrib-extension-names fields
          // were added only in rev 08, while "supported2" was defined in rev 07
          FSupport->OpenBlockVector = SupportedStruct.GetSmallCardinal();
          FSupport->BlockVector = SupportedStruct.GetSmallCardinal();
          uint32_t ExtensionCount;
          ExtensionCount = SupportedStruct.GetCardinal();
          for (uint32_t Index = 0; Index < ExtensionCount; ++Index)
          {
            FSupport->AttribExtensions->Add(SupportedStruct.GetAnsiString());
          }
          ExtensionCount = SupportedStruct.GetCardinal();
          for (uint32_t Index = 0; Index < ExtensionCount; ++Index)
          {
            const UnicodeString Extension = SupportedStruct.GetAnsiString();
            SupportedExtensions->Add(Extension);
            ExtensionsLog->Add(Extension);
          }
        }

        if (FTerminal->GetLog()->GetLogging())
        {
          FTerminal->LogEvent(FORMAT(
            "Server support information (%s):\n"
            "  Attribute mask: %x, Attribute bits: %x, Open flags: %x\n"
            "  Access mask: %x, Open block vector: %x, Block vector: %x, Max read size: %d\n",
            ExtensionName,
             nb::ToInt32(FSupport->AttributeMask),
             nb::ToInt32(FSupport->AttributeBits),
             nb::ToInt32(FSupport->OpenFlags),
             nb::ToInt32(FSupport->AccessMask),
             nb::ToInt32(FSupport->OpenBlockVector),
             nb::ToInt32(FSupport->BlockVector),
             nb::ToInt32(FSupport->MaxReadSize)));
          FTerminal->LogEvent(FORMAT("  Attribute extensions (%d)\n", FSupport->AttribExtensions->GetCount()));
          for (int32_t Index = 0; Index < FSupport->AttribExtensions->GetCount(); ++Index)
          {
            FTerminal->LogEvent(
              FORMAT("    %s", FSupport->AttribExtensions->GetString(Index)));
          }
          FTerminal->LogEvent(FORMAT("  Extensions (%d)\n", ExtensionsLog->GetCount()));
          for (int32_t Index = 0; Index < ExtensionsLog->GetCount(); ++Index)
          {
            FTerminal->LogEvent(
              FORMAT("    %s", ExtensionsLog->GetString(Index)));
          }
        }
      }
      else if (ExtensionName == SFTP_EXT_VENDOR_ID)
      {
        TSFTPPacket VendorIdStruct(ExtensionData, FCodePage);
        const UnicodeString VendorName(VendorIdStruct.GetAnsiString());
        const UnicodeString ProductName(VendorIdStruct.GetAnsiString());
        const UnicodeString ProductVersion(VendorIdStruct.GetAnsiString());
        int64_t ProductBuildNumber = VendorIdStruct.GetInt64();
        FTerminal->LogEvent(FORMAT("Server software: %s %s (%d) by %s",
          ProductName, ProductVersion, nb::ToInt32(ProductBuildNumber), VendorName));
      }
      else if (ExtensionName == SFTP_EXT_FSROOTS)
      {
        FTerminal->LogEvent("File system roots:\n");
        DebugAssert(FFixedPaths == nullptr);
        FFixedPaths = std::make_unique<TStringList>();
        try
        {
          TSFTPPacket RootsPacket(ExtensionData, FCodePage);
          while (RootsPacket.GetNextData() != nullptr)
          {
            uint32_t Dummy = RootsPacket.GetCardinal();
            if (Dummy != 1)
            {
              break;
            }
            else
            {
              uint8_t Drive = RootsPacket.GetByte();
              uint8_t MaybeType = RootsPacket.GetByte();
              FTerminal->LogEvent(FORMAT("  %s: (type %d)", static_cast<char>(Drive), nb::ToInt32(MaybeType)));
              FFixedPaths->Add(FORMAT("%s:", static_cast<char>(Drive)));
            }
          }
        }
        catch(Exception & E)
        {
          DEBUG_PRINTF("before FTerminal->HandleException");
          FFixedPaths->Clear();
          FTerminal->LogEvent(FORMAT("Failed to decode %s extension",
            ExtensionName));
          FTerminal->HandleException(&E);
        }
      }
      else if (ExtensionName == SFTP_EXT_VERSIONS)
      {
        // first try legacy decoding according to incorrect encoding
        // (structure-like) as of VShell (bug no longer present as of 4.0.3).
        TSFTPPacket VersionsPacket(ExtensionData, FCodePage);
        uint32_t StringSize;
        if (VersionsPacket.CanGetString(StringSize) &&
            (StringSize == VersionsPacket.GetRemainingLength()))
        {
          const UnicodeString Versions = VersionsPacket.GetAnsiString();
          FTerminal->LogEvent(FORMAT("SFTP versions supported by the server (VShell format): %s",
            Versions));
        }
        else
        {
          // if that fails, fallback to proper decoding
          FTerminal->LogEvent(FORMAT("SFTP versions supported by the server: %s",
            AnsiToString(ExtensionData)));
        }
      }
      else if (ExtensionName == SFTP_EXT_STATVFS)
      {
        const UnicodeString StatVfsVersion = AnsiToString(ExtensionData);
        if (StatVfsVersion == SFTP_EXT_STATVFS_VALUE_V2)
        {
          FSupportsStatVfsV2 = true;
          FTerminal->LogEvent(FORMAT("Supports %s extension version %s", ExtensionName, StatVfsVersion));
        }
        else
        {
          FTerminal->LogEvent(FORMAT("Unsupported %s extension version %s", ExtensionName, StatVfsVersion));
        }
      }
      else if (ExtensionName == SFTP_EXT_HARDLINK)
      {
        const UnicodeString HardlinkVersion = AnsiToString(ExtensionData);
        if (HardlinkVersion == SFTP_EXT_HARDLINK_VALUE_V1)
        {
          FSupportsHardlink = true;
          FTerminal->LogEvent(FORMAT("Supports %s extension version %s", ExtensionName, HardlinkVersion));
        }
        else
        {
          FTerminal->LogEvent(FORMAT("Unsupported %s extension version %s", ExtensionName, HardlinkVersion));
        }
      }
      else if (ExtensionName == SFTP_EXT_LIMITS)
      {
        const UnicodeString LimitsVersion = AnsiToString(ExtensionData);
        if (LimitsVersion == SFTP_EXT_LIMITS_VALUE_V1)
        {
          SupportsLimits = true;
          FTerminal->LogEvent(FORMAT(L"Supports %s extension version %s", ExtensionName, LimitsVersion));
        }
        else
        {
          FTerminal->LogEvent(FORMAT(L"Unsupported %s extension version %s", ExtensionName, LimitsVersion));
        }
      }
      else if ((ExtensionName == SFTP_EXT_COPY_FILE) ||
               (ExtensionName == SFTP_EXT_COPY_DATA) ||
               (ExtensionName == SFTP_EXT_SPACE_AVAILABLE) ||
               (ExtensionName == SFTP_EXT_CHECK_FILE) ||
               (ExtensionName == SFTP_EXT_POSIX_RENAME))
      {
        FTerminal->LogEvent(FORMAT(L"Supports extension %s=%s", ExtensionName, ExtensionDisplayData));
      }
      else
      {
        FTerminal->LogEvent(0, FORMAT("Unknown server extension %s=%s", ExtensionName, ExtensionDisplayData));
      }

      UnicodeString Line = ExtensionName;
      if (!ExtensionDisplayData.IsEmpty())
      {
        Line += FORMAT(L"=%s", ExtensionDisplayData);
      }
      FExtensions += FORMAT(L"  %s\r\n", Line);

      SupportedExtensions->Add(ExtensionName);
    }
  }

  if (FVersion < 4)
  {
    // currently enable the bug for all servers (really known on OpenSSH)
    FSignedTS = (GetSessionData()->GetSFTPBug(sbSignedTS) == asOn) ||
      (GetSessionData()->GetSFTPBug(sbSignedTS) == asAuto);
    if (FSignedTS)
    {
      FTerminal->LogEvent("We believe the server has signed timestamps bug");
    }
  }
  else
  {
    FSignedTS = false;
  }

  switch (GetSessionData()->GetNotUtf())
  {
    case asOff:
      FUtfStrings = asOn;
      FTerminal->LogEvent("We will use UTF-8 strings as configured");
      break;

    case asAuto:
      // Nb, Foxit server does not exist anymore
      if (GetSessionInfo().SshImplementation.Pos("Foxit-WAC-Server") == 1)
      {
        FUtfStrings = asOff;
        FTerminal->LogEvent("We will not use UTF-8 strings as the server is known not to use them");
      }
      else
      {
        if (FVersion >= 4)
        {
          FTerminal->LogEvent("We will use UTF-8 strings as it is mandatory with SFTP version 4 and newer");
          FUtfStrings = asOn;
        }
        else
        {
          FTerminal->LogEvent("We will use UTF-8 strings until server sends an invalid UTF-8 string as with SFTP version 3 and older UTF-8 strings are not mandatory");
          FUtfStrings = asAuto;
          FUtfDisablingAnnounced = false;
        }
      }
      break;

    case asOn:
      FTerminal->LogEvent("We will not use UTF-8 strings as configured");
      FUtfStrings = asOff;
      break;

    default:
      DebugFail();
      break;
  }

  FMaxPacketSize = nb::ToUInt32(GetSessionData()->GetSFTPMaxPacketSize());
  if (FMaxPacketSize == 0)
  {
    uint32_t PacketPayload = 4;
    if (SupportsLimits)
    {
      // TSFTPPacket Packet(-1, FCodePage); //for tests
      TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
      Packet.AddString(RawByteString(SFTP_EXT_LIMITS));
      SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);
      uint32_t MaxPacketSize = nb::ToUInt32(std::min(std::numeric_limits<int64_t>::max(), Packet.GetInt64()));
      FTerminal->LogEvent(FORMAT(L"Limiting packet size to server's limit of %d + %d bytes",
        nb::ToInt32(MaxPacketSize), nb::ToInt32(PacketPayload)));
      FMaxPacketSize = MaxPacketSize + PacketPayload;
    }
    else if ((FSecureShell->GetSshImplementation() == sshiOpenSSH) && (FVersion == 3) && !FSupport->Loaded)
    {
      FMaxPacketSize = PacketPayload + (256 * 1024); // len + 256kB payload
      FTerminal->LogEvent(FORMAT("Limiting packet size to OpenSSH sftp-server limit of %d bytes",
        nb::ToInt32(FMaxPacketSize)));
    }
    // full string is "1.77 sshlib: Momentum SSH Server",
    // possibly it is sshlib-related
    else if (GetSessionInfo().SshImplementation.Pos(L"Momentum SSH Server") != 0)
    {
      FMaxPacketSize = PacketPayload + (32 * 1024);
      FTerminal->LogEvent(FORMAT("Limiting packet size to Momentum sftp-server limit of %d bytes",
        nb::ToInt32(FMaxPacketSize)));
    }
  }

  FSupportedExtensions.reset(FTerminal->ProcessFeatures(SupportedExtensions.get()));

  if (SupportsExtension(SFTP_EXT_VENDOR_ID))
  {
    TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
    Packet.AddString(SFTP_EXT_VENDOR_ID);
    Packet.AddString(FTerminal->Configuration->GetCompanyName());
    Packet.AddString(FTerminal->Configuration->GetProductName());
    Packet.AddString(FTerminal->Configuration->GetProductVersion());
    Packet.AddInt64(LOWORD(FTerminal->Configuration->GetFixedApplicationInfo()->dwFileVersionLS));
    SendPacket(&Packet);
    // we are not interested in the response, do not wait for it
    ReserveResponse(&Packet, nullptr);
  }

}

char * TSFTPFileSystem::GetEOL() const
{
  if (FVersion >= 4)
  {
    DebugAssert(!FEOL.IsEmpty());
    return ToCharPtr(FEOL);
  }
  return EOLToStr(GetSessionData()->GetEOLType());
}

void TSFTPFileSystem::LookupUsersGroups()
{
  DebugAssert(SupportsExtension(SFTP_EXT_OWNER_GROUP));

  TSFTPPacket PacketOwners(SSH_FXP_EXTENDED, FCodePage);
  TSFTPPacket PacketGroups(SSH_FXP_EXTENDED, FCodePage);

  TSFTPPacket * Packets[] = { &PacketOwners, &PacketGroups };
  TRemoteTokenList * Lists[] = { FTerminal->GetUsers(), FTerminal->GetGroups() };
  constexpr wchar_t ListTypes[] = { OGQ_LIST_OWNERS, OGQ_LIST_GROUPS };

  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(Packets)); ++Index)
  {
    TSFTPPacket * Packet = Packets[Index];
    Packet->AddString(RawByteString(SFTP_EXT_OWNER_GROUP));
    Packet->AddByte(static_cast<uint8_t>(ListTypes[Index]));
    SendPacket(Packet);
    ReserveResponse(Packet, Packet);
  }

  for (int32_t Index = 0; Index < nb::ToIntPtr(_countof(Packets)); ++Index)
  {
    TSFTPPacket * Packet = Packets[Index];

    ReceiveResponse(Packet, Packet, SSH_FXP_EXTENDED_REPLY, asOpUnsupported);

    if ((Packet->GetType() != SSH_FXP_EXTENDED_REPLY) ||
      (Packet->GetAnsiString() != SFTP_EXT_OWNER_GROUP_REPLY))
    {
      FTerminal->LogEvent(FORMAT("Invalid response to %s", SFTP_EXT_OWNER_GROUP));
    }
    else
    {
      TRemoteTokenList &List = *Lists[Index];
      const uint32_t Count = Packet->GetCardinal();

      List.Clear();
      for (uint32_t Item = 0; Item < Count; Item++)
      {
        TRemoteToken Token(Packet->GetString(FUtfStrings));
        List.Add(Token);
        if (&List == FTerminal->GetGroups())
        {
          FTerminal->GetMembership()->Add(Token);
        }
      }
    }
  }
}

void TSFTPFileSystem::ReadCurrentDirectory()
{
  if (!FDirectoryToChangeTo.IsEmpty())
  {
    FCurrentDirectory = FDirectoryToChangeTo;
    FDirectoryToChangeTo.Clear();
  }
  else if (FCurrentDirectory.IsEmpty())
  {
    // this happens only after startup when default remote directory is not specified
    FCurrentDirectory = GetHomeDirectory();
  }
}

void TSFTPFileSystem::HomeDirectory()
{
  ChangeDirectory(GetHomeDirectory());
}

void TSFTPFileSystem::TryOpenDirectory(const UnicodeString & Directory)
{
  FTerminal->LogEvent(FORMAT("Trying to open directory \"%s\".", Directory));
  TRemoteFile * File = nullptr;
  CustomReadFile(Directory, File, SSH_FXP_LSTAT, nullptr, asOpUnsupported);
  if (File == nullptr)
  {
    // File can be NULL only when server does not support SSH_FXP_LSTAT.
    // Fallback to legacy solution, which in turn does not allow entering
    // traverse-only (chmod 110) directories.
    // This is workaround for https://www.ftpshell.com/
    TSFTPPacket Packet(SSH_FXP_OPENDIR, FCodePage);
    AddPathString(Packet, base::UnixExcludeTrailingBackslash(Directory));
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);
    const RawByteString Handle = Packet.GetFileHandle();
    Packet.ChangeType(SSH_FXP_CLOSE);
    Packet.AddString(Handle);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS, asAll);
  }
  else
  {
    SAFE_DESTROY(File);
  }
}

void TSFTPFileSystem::AnnounceFileListOperation()
{
}

void TSFTPFileSystem::ChangeDirectory(const UnicodeString & Directory)
{
  const UnicodeString Current = !FDirectoryToChangeTo.IsEmpty() ? FDirectoryToChangeTo : FCurrentDirectory;
  const UnicodeString Path = GetRealPath(Directory, Current);

  // to verify existence of directory try to open it (SSH_FXP_REALPATH succeeds
  // for invalid paths on some systems, like CygWin)
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FDirectoryToChangeTo = Path;
}

void TSFTPFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FDirectoryToChangeTo = base::UnixExcludeTrailingBackslash(Directory);
}

void TSFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  DebugAssert(FileList && !FileList->GetDirectory().IsEmpty());

  // UnicodeString Directory;
  const UnicodeString Directory = base::UnixExcludeTrailingBackslash(LocalCanonify(FileList->GetDirectory()));
  FTerminal->LogEvent(FORMAT("Listing directory \"%s\".", Directory));

  // moved before SSH_FXP_OPENDIR, so directory listing does not retain
  // old data (e.g. parent directory) when reading fails
  FileList->Reset();

  TSFTPPacket Packet(SSH_FXP_OPENDIR, FCodePage);
  RawByteString Handle;

  try
  {
    AddPathString(Packet, Directory);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

    Handle = Packet.GetFileHandle();
  }
  catch(...)
  {
    if (FTerminal->GetActive())
    {
      FileList->AddFile(new TRemoteParentDirectory(FTerminal));
    }
    throw;
  }

  TSFTPPacket Response(SSH_FXP_NONE, FCodePage);
  try__finally
  {
    bool isEOF = false;
    int32_t Total = 0;
    bool HasParentDirectory = false;
    TRemoteFile * File = nullptr;

    Packet.ChangeType(SSH_FXP_READDIR);
    Packet.AddString(Handle);

    SendPacket(&Packet);

    do
    {
      ReceiveResponse(&Packet, &Response);
      if (Response.GetType() == SSH_FXP_NAME)
      {
        TSFTPPacket ListingPacket(Response, FCodePage);

        Packet.ChangeType(SSH_FXP_READDIR);
        Packet.AddString(Handle);

        SendPacket(&Packet);
        ReserveResponse(&Packet, &Response);

        const uint32_t Count = ListingPacket.GetCardinal();

        int32_t ResolvedLinks = 0;
        for (uint32_t Index = 0; !isEOF && (Index < Count); ++Index)
        {
          // File = LoadFile(&ListingPacket, nullptr, L"", FileList);
          std::unique_ptr<TRemoteFile> AFile(LoadFile(&ListingPacket, nullptr, L"", FileList));
          File = AFile.get();
          if (FTerminal->IsValidFile(File))
          {
            FileList->AddFile(AFile.release());
            if (FTerminal->IsEncryptingFiles() && // optimization
                IsRealFile(File->FileName))
            {
              const UnicodeString FullFileName = base::UnixExcludeTrailingBackslash(File->FullFileName);
              const UnicodeString FileName = base::UnixExtractFileName(FTerminal->DecryptFileName(FullFileName, false, false));
              if (File->FileName != FileName)
              {
                File->SetEncrypted();
              }
              File->FileName = FileName;
            }
            if (FTerminal->Configuration->ActualLogProtocol >= 1)
            {
              FTerminal->LogEvent(FORMAT("Read file '%s' from listing", File->GetFileName()));
            }
            if (File->GetLinkedFile() != nullptr)
            {
              ResolvedLinks++;
            }
            if (File->GetIsParentDirectory())
            {
              HasParentDirectory = true;
            }
            Total++;

            if (Total % 10 == 0)
            {
              FTerminal->DoReadDirectoryProgress(Total, ResolvedLinks, isEOF);
              if (isEOF)
              {
                FTerminal->LogEvent(L"Listing directory cancelled.");
                FTerminal->DoReadDirectoryProgress(-2, 0, isEOF);
              }
            }
          }
        }

        if (!isEOF &&
            (FVersion >= 6) &&
            // As of 7.0.9 the Cerberus SFTP server always sets the end-of-list to true.
            // Fixed in 7.0.10.
            (FSecureShell->GetSshImplementation() != sshiCerberus) &&
            ListingPacket.CanGetBool())
        {
          isEOF = ListingPacket.GetBool();
        }

        if (Count == 0)
        {
          FTerminal->LogEvent("Empty directory listing packet. Aborting directory reading.");
          isEOF = true;
        }
      }
      else if (Response.GetType() == SSH_FXP_STATUS)
      {
        isEOF = (GotStatusPacket(&Response, asEOF, false) == SSH_FX_EOF);
      }
      else
      {
        FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_TYPE, nb::ToInt32(Response.GetType())));
      }
    }
    while (!isEOF);

    if (Total == 0)
    {
      bool Failure = false;
      // no point reading parent of root directory,
      // moreover CompleteFTP terminates session upon attempt to do so
      if (base::IsUnixRootPath(FileList->GetDirectory()))
      {
        File = nullptr;
      }
      else
      {
        // Empty file list -> probably "permission denied", we
        // at least get link to parent directory ("..")
        try
        {
          FTerminal->SetExceptionOnFail(true);
          try__finally
          {
            File = FTerminal->ReadFile(base::UnixCombinePaths(FileList->GetDirectory(), PARENTDIRECTORY));
          }
          __finally
          {
            FTerminal->SetExceptionOnFail(false);
          } end_try__finally
        }
        catch(Exception & E)
        {
          if (rtti::isa<EFatal>(&E))
          {
            throw;
          }
          else
          {
            File = nullptr;
            Failure = true;
          }
        }
      }

      // on some systems even getting ".." fails, we create dummy ".." instead
      if (File == nullptr)
      {
        File = new TRemoteParentDirectory(FTerminal);
      }

      DebugAssert(File && File->GetIsParentDirectory());
      FileList->AddFile(File);

      if (Failure)
      {
        throw ExtException(
          nullptr, FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory()),
          HELP_EMPTY_DIRECTORY);
      }
    }
    else
    {
      if (!HasParentDirectory)
      {
        FileList->AddFile(new TRemoteParentDirectory(FTerminal));
      }
    }
  }
  __finally
  {
    if (FTerminal->GetActive())
    {
      Packet.ChangeType(SSH_FXP_CLOSE);
      Packet.AddString(Handle);
      SendPacket(&Packet);
      // we are not interested in the response, do not wait for it
      ReserveResponse(&Packet, nullptr);
    }
  } end_try__finally
}

void TSFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& AFile)
{
  DebugAssert(SymlinkFile);
  if (!SymlinkFile)
    return;
  DebugAssert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later

  // need to use full filename when resolving links within subdirectory
  // (i.e. for download)
  const UnicodeString FileName = LocalCanonify(
    SymlinkFile->GetHaveFullFileName() ? SymlinkFile->GetFullFileName() : SymlinkFile->GetFileName());

  TSFTPPacket ReadLinkPacket(SSH_FXP_READLINK, FCodePage);
  AddPathString(ReadLinkPacket, FileName);
  SendPacket(&ReadLinkPacket);
  ReserveResponse(&ReadLinkPacket, &ReadLinkPacket);

  // send second request before reading response to first one
  // (performance benefit)
  TSFTPPacket AttrsPacket(SSH_FXP_STAT, FCodePage);
  AddPathString(AttrsPacket, FileName);
  if (FVersion >= 4)
  {
    AttrsPacket.AddCardinal(SSH_FILEXFER_ATTR_COMMON);
  }
  SendPacket(&AttrsPacket);
  ReserveResponse(&AttrsPacket, &AttrsPacket);

  ReceiveResponse(&ReadLinkPacket, &ReadLinkPacket, SSH_FXP_NAME);
  if (ReadLinkPacket.GetCardinal() != 1)
  {
    FTerminal->FatalError(nullptr, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
  }
  // Not sure about the DontCache parameter here. Actually we should not get here for encrypted sessions.
  DebugAssert(!FTerminal->IsEncryptingFiles());
  SymlinkFile->LinkTo = FTerminal->DecryptFileName(ReadLinkPacket.GetPathString(FUtfStrings), true, true);
  FTerminal->LogEvent(FORMAT("Link resolved to \"%s\".", SymlinkFile->GetLinkTo()));

  ReceiveResponse(&AttrsPacket, &AttrsPacket, SSH_FXP_ATTRS);
  // SymlinkFile->FileName was used instead SymlinkFile->LinkTo before, why?
  AFile = LoadFile(&AttrsPacket, SymlinkFile,
    base::UnixExtractFileName(SymlinkFile->GetLinkTo()));
}

void TSFTPFileSystem::ReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile)
{
  CustomReadFile(AFileName, AFile, SSH_FXP_LSTAT);
}

bool TSFTPFileSystem::RemoteFileExists(const UnicodeString & FullPath,
  TRemoteFile ** AFile)
{
  bool Result;
  try
  {
    TRemoteFile * File = nullptr;
    CustomReadFile(FullPath, File, SSH_FXP_LSTAT, nullptr, asNoSuchFile);
    Result = (File != nullptr);
    if (Result)
    {
      if (AFile)
      {
        *AFile = File;
      }
      else
      {
        SAFE_DESTROY(File);
      }
    }
  }
  catch(...)
  {
    if (!FTerminal->GetActive())
    {
      throw;
    }
    Result = false;
  }
  return Result;
}

void TSFTPFileSystem::SendCustomReadFile(TSFTPPacket * Packet,
  TSFTPPacket * Response, uint32_t Flags)
{
  if (FVersion >= 4)
  {
    Packet->AddCardinal(Flags);
  }
  SendPacket(Packet);
  ReserveResponse(Packet, Response);
}

void TSFTPFileSystem::CustomReadFile(const UnicodeString & AFileName,
  TRemoteFile *& AFile, SSH_FXP_TYPE Type, TRemoteFile * ALinkedByFile,
  SSH_FX_TYPE AllowStatus)
{
  constexpr SSH_FILEXFER_ATTR_TYPE Flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS |
    SSH_FILEXFER_ATTR_ACCESSTIME | SSH_FILEXFER_ATTR_MODIFYTIME |
    SSH_FILEXFER_ATTR_OWNERGROUP;
  TSFTPPacket Packet(Type, FCodePage);
  const UnicodeString FullName = LocalCanonify(AFileName);
  AddPathString(Packet, FullName);
  SendCustomReadFile(&Packet, &Packet, Flags);
  ReceiveResponse(&Packet, &Packet, SSH_FXP_ATTRS, AllowStatus);

  if (Packet.GetType() == SSH_FXP_ATTRS)
  {
    AFile = LoadFile(&Packet, ALinkedByFile, base::UnixExtractFileName(AFileName));
    if (FTerminal->IsFileEncrypted(FullName))
    {
      AFile->SetEncrypted();
    }
  }
  else
  {
    DebugAssert(AllowStatus > 0);
    AFile = nullptr;
  }
}

void TSFTPFileSystem::DoDeleteFile(const UnicodeString & AFileName, SSH_FXP_TYPE Type)
{
  TSFTPPacket Packet(Type, FCodePage);
  const UnicodeString RealFileName = LocalCanonify(AFileName);
  AddPathString(Packet, RealFileName);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}

void TSFTPFileSystem::DeleteFile(const UnicodeString & AFileName,
  const TRemoteFile * AFile, int32_t AParams, TRmSessionAction & Action)
{
  uint8_t Type;
  if (FTerminal->DeleteContentsIfDirectory(AFileName, AFile, AParams, Action))
  {
    Type = SSH_FXP_RMDIR;
  }
  else
  {
    Type = SSH_FXP_REMOVE;
  }

  DoDeleteFile(AFileName, Type);
}

void TSFTPFileSystem::RenameFile(
  const UnicodeString & AFileName, const TRemoteFile * /*File*/, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  const bool UsePosixRename = FTerminal->SessionData->UsePosixRename;
  TSFTPPacket Packet(UsePosixRename ? SSH_FXP_EXTENDED : SSH_FXP_RENAME, FCodePage);
  if (UsePosixRename)
  {
    Packet.AddString(SFTP_EXT_POSIX_RENAME);
  }
  const UnicodeString RealName = LocalCanonify(AFileName);
  const bool Encrypted = FTerminal->IsFileEncrypted(RealName);
  AddPathString(Packet, RealName);
  UnicodeString TargetName;
  if (base::UnixExtractFilePath(ANewName).IsEmpty())
  {
    // rename case (TTerminal::RenameFile)
    TargetName = base::UnixExtractFilePath(RealName) + ANewName;
  }
  else
  {
    TargetName = LocalCanonify(ANewName);
  }
  AddPathString(Packet, TargetName, Encrypted);
  if (UsePosixRename && (FVersion >= 5))
  {
    Packet.AddCardinal(0);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}

void TSFTPFileSystem::DoCloseRemoteIfOpened(const RawByteString & Handle)
{
  if (!Handle.IsEmpty())
  {
    TSFTPPacket Packet(SSH_FXP_CLOSE, FCodePage);
    Packet.AddString(Handle);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
  }
}

void TSFTPFileSystem::CopyFile(
  const UnicodeString & AFileName, const TRemoteFile * AFile, const UnicodeString & ANewName, bool DebugUsedArg(Overwrite))
{
  const UnicodeString FileNameCanonical = Canonify(AFileName);
  const bool Encrypted = FTerminal->IsFileEncrypted(FileNameCanonical);
  const UnicodeString NewNameCanonical = Canonify(ANewName);

  if (SupportsExtension(SFTP_EXT_COPY_FILE) || (FSecureShell->FSshImplementation == sshiBitvise))
  {
    TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
    Packet.AddString(SFTP_EXT_COPY_FILE);
    AddPathString(Packet, FileNameCanonical);
    AddPathString(Packet, NewNameCanonical, Encrypted);
    Packet.AddBool(false);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
  }
  else
  {
    DebugAssert(SupportsExtension(SFTP_EXT_COPY_DATA));

    const int64_t Size = DebugAlwaysTrue(AFile != nullptr) ? AFile->Size : -1;
    RawByteString SourceRemoteHandle, DestRemoteHandle;
    try__finally
    {
      SourceRemoteHandle = SFTPOpenRemoteFile(FileNameCanonical, SSH_FXF_READ, Encrypted, Size);
      // SFTP_EXT_COPY_FILE does not allow overwriting existing files
      // (the specification does not mandate it, but it is implemented like that both in ProFTPD and Bitvise).
      // So using SSH_FXF_EXCL for consistency.
      DestRemoteHandle = SFTPOpenRemoteFile(NewNameCanonical, SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_EXCL, Encrypted, Size);

      TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
      Packet.AddString(SFTP_EXT_COPY_DATA);
      Packet.AddString(SourceRemoteHandle);
      Packet.AddInt64(0);
      Packet.AddInt64(0); // until EOF
      Packet.AddString(DestRemoteHandle);
      Packet.AddInt64(0);
      SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
    }
    __finally
    {
      if (FTerminal->Active)
      {
        DoCloseRemoteIfOpened(SourceRemoteHandle);
        DoCloseRemoteIfOpened(DestRemoteHandle);
      }
    } end_try__finally
  }
}

void TSFTPFileSystem::CreateDirectory(const UnicodeString & ADirName, bool Encrypt)
{
  TSFTPPacket Packet(SSH_FXP_MKDIR, FCodePage);
  const UnicodeString CanonifiedName = Canonify(ADirName);
  AddPathString(Packet, CanonifiedName, Encrypt);
  Packet.AddProperties(nullptr, 0, true, FVersion, FUtfStrings, nullptr);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}

void TSFTPFileSystem::CreateLink(const UnicodeString & AFileName,
  const UnicodeString & PointTo, bool Symbolic)
{
  // Cerberus server does not even respond to LINK or SYMLINK,
  // Although its log says:
  // Unrecognized SFTP client command: (20)
  // Unknown SFTP packet - Sending Unsupported OP response

  DebugAssert(FVersion >= 3); // links are supported with SFTP version 3 and later
  const bool UseLink = (FVersion >= 6);
  const bool UseHardlink = !Symbolic && !UseLink && FSupportsHardlink;
  TSFTPPacket Packet(UseHardlink ? SSH_FXP_EXTENDED : (UseLink ? SSH_FXP_LINK : SSH_FXP_SYMLINK), FCodePage);
  if (UseHardlink)
  {
    Packet.AddString(SFTP_EXT_HARDLINK);
  }

  bool Buggy;
  // OpenSSH hardlink extension always uses the "wrong" order
  // as it's defined as such to mimic OpenSSH symlink bug
  if (UseHardlink)
  {
    Buggy = true; //sic
  }
  else
  {
    if (GetSessionData()->GetSFTPBug(sbSymlink) == asOn)
    {
      Buggy = true;
      FTerminal->LogEvent("Forcing workaround for SFTP link bug");
    }
    else if (GetSessionData()->GetSFTPBug(sbSymlink) == asOff)
    {
      Buggy = false;
    }
    else
    {
      if (UseLink)
      {
        if (FSecureShell->GetSshImplementation() == sshiProFTPD)
        {
          // ProFTPD/mod_sftp followed OpenSSH symlink bug even for link implementation.
          // This will be fixed with the next release with
          // SSH version string bumped to "mod_sftp/1.0.0"
          // http://bugs.proftpd.org/show_bug.cgi?id=4080
          UnicodeString ProFTPDVerStr = GetSessionInfo().SshImplementation;
          CutToChar(ProFTPDVerStr, L'/', false);
          const int32_t ProFTPDMajorVer = ::StrToIntDef(CutToChar(ProFTPDVerStr, L'.', false), 0);
          Buggy = (ProFTPDMajorVer == 0);
          if (Buggy)
          {
            FTerminal->LogEvent("We believe the server has SFTP link bug");
          }
        }
        else
        {
          Buggy = false;
        }
      }
      else
      {
        // ProFTPD/mod_sftp deliberately follows OpenSSH bug.
        // Though we should get here with ProFTPD only when user forced
        // SFTP version < 6 or when connecting to an ancient version of ProFTPD.
        Buggy =
          (FSecureShell->GetSshImplementation() == sshiOpenSSH) ||
          (FSecureShell->GetSshImplementation() == sshiProFTPD);
        if (Buggy)
        {
          FTerminal->LogEvent("We believe the server has SFTP symlink bug");
        }
      }
    }
  }

  UnicodeString FinalPointTo = PointTo;
  const UnicodeString FinalFileName = Canonify(AFileName);

  if (!Symbolic)
  {
    FinalPointTo = Canonify(PointTo);
  }

  // creating symlinks is not allowed when encryption is enabled, so we are not considering encryption here
  if (!Buggy)
  {
    AddPathString(Packet, FinalFileName);
    AddPathString(Packet, FinalPointTo);
  }
  else
  {
    AddPathString(Packet, FinalPointTo);
    AddPathString(Packet, FinalFileName);
  }

  if (UseLink)
  {
    Packet.AddBool(Symbolic);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}

void TSFTPFileSystem::ChangeFileProperties(const UnicodeString & AFileName,
  const TRemoteFile * /* AFile */, const TRemoteProperties * AProperties,
  TChmodSessionAction & Action)
{
  DebugAssert(AProperties != nullptr);
  if (!AProperties)
    return;

  TRemoteFile * File = nullptr;

  const UnicodeString RealFileName = LocalCanonify(AFileName);
  ReadFile(RealFileName, File);

  try__finally
  {
    std::unique_ptr<TRemoteFile> FilePtr(File);
    DebugAssert(FilePtr.get());
    if (FilePtr->GetIsDirectory() && FTerminal->CanRecurseToDirectory(FilePtr.get()) && AProperties->Recursive)
    {
      try
      {
        FTerminal->ProcessDirectory(AFileName, nb::bind(&TTerminal::ChangeFileProperties, FTerminal),
          nb::ToPtr(const_cast<TRemoteProperties *>(AProperties)));
      }
      catch(...)
      {
        Action.Cancel();
        throw;
      }
    }

    // SFTP can change owner and group at the same time only, not individually.
    // Fortunately we know current owner/group, so if only one is present,
    // we can supplement the other.
    TRemoteProperties Properties(*AProperties);
    if (Properties.Valid.Contains(vpGroup) &&
        !Properties.Valid.Contains(vpOwner))
    {
      Properties.Owner = File->GetFileOwner();
      Properties.Valid << vpOwner;
    }
    else if (Properties.Valid.Contains(vpOwner) &&
             !Properties.Valid.Contains(vpGroup))
    {
      Properties.Group = File->GetFileGroup();
      Properties.Valid << vpGroup;
    }

    TSFTPPacket Packet(SSH_FXP_SETSTAT, FCodePage);
    AddPathString(Packet, RealFileName);
    Packet.AddProperties(&Properties, *File->GetRights(), File->GetIsDirectory(), FVersion, FUtfStrings, &Action);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
  }
  __finally__removed
  {
    // delete File;
  } end_try__finally
}

bool TSFTPFileSystem::LoadFilesProperties(TStrings * AFileList)
{
  bool Result = false;
  // without knowledge of server's capabilities, this all make no sense
  if (FSupport->Loaded || (FSecureShell->GetSshImplementation() == sshiBitvise))
  {
    TFileOperationProgressType Progress(nb::bind(&TTerminal::DoProgress, FTerminal), nb::bind(&TTerminal::DoFinished, FTerminal));
    FTerminal->OperationStart(Progress, foGetProperties, osRemote, AFileList->Count);

    constexpr uint32_t LoadFilesPropertiesQueueLen = 5;
    TSFTPLoadFilesPropertiesQueue Queue(this, FCodePage);
    try__finally
    {
      if (Queue.Init(LoadFilesPropertiesQueueLen, AFileList))
      {
        TRemoteFile * File = nullptr;
        TSFTPPacket Packet(SSH_FXP_NONE, FCodePage);
        bool Next;
        do
        {
          Next = Queue.ReceivePacket(&Packet, File);
          DebugAssert((Packet.GetType() == SSH_FXP_ATTRS) || (Packet.GetType() == SSH_FXP_STATUS));
          if ((Packet.GetType() == SSH_FXP_ATTRS) && File)
          {
            DebugAssert(File != nullptr);
            Progress.SetFile(File->GetFileName());
            LoadFile(File, &Packet);
            Result = true;
            TOnceDoneOperation OnceDoneOperation;
            Progress.Finish(File->GetFileName(), true, OnceDoneOperation);
          }

          if (Progress.GetCancel() != csContinue)
          {
            Next = false;
          }
        }
        while (Next);
      }
    }
    __finally
    {
      Queue.DisposeSafe();
      FTerminal->OperationStop(Progress);
    } end_try__finally
    // queue is discarded here
  }

  return Result;
}

void TSFTPFileSystem::CalculateFilesChecksum(
  const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent && OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  FTerminal->CalculateSubFoldersChecksum(Alg, FileList, std::move(OnCalculatedChecksum), OperationProgress, FirstLevel);

  static int32_t CalculateFilesChecksumQueueLen = 5;
  TSFTPCalculateFilesChecksumQueue Queue(this, FCodePage);
  TOnceDoneOperation OnceDoneOperation; // not used
  try__finally
  {
    UnicodeString SftpAlg;
    const int32_t Index = FChecksumAlgs->IndexOf(Alg);
    if (Index >= 0)
    {
      SftpAlg = FChecksumSftpAlgs->Strings[Index];
    }
    else
    {
      // try user-specified alg
      SftpAlg = Alg;
    }

    if (Queue.Init(CalculateFilesChecksumQueueLen, SftpAlg, FileList))
    {
      TSFTPPacket Packet(SSH_FXP_NONE, FCodePage);
      bool Next;
      do
      {
        bool Success = false;
        TRemoteFile * File = nullptr;

        try__finally
        {
          TChecksumSessionAction Action(FTerminal->GetActionLog());
          try
          {
            Next = Queue.ReceivePacket(&Packet, File);
            DebugAssert(Packet.GetType() == SSH_FXP_EXTENDED_REPLY);

            OperationProgress->SetFile(File->GetFileName());
            Action.SetFileName(FTerminal->GetAbsolutePath(File->GetFullFileName(), true));

            // skip alg
            nb::used(Packet.GetAnsiString());
            const UnicodeString Checksum = BytesToHex(nb::ToUInt8Ptr(Packet.GetNextData(Packet.GetRemainingLength())), Packet.GetRemainingLength(), false);
            if (!OnCalculatedChecksum.empty())
            {
              OnCalculatedChecksum(File->GetFileName(), Alg, Checksum);
            }
            Action.Checksum(Alg, Checksum);

            Success = true;
          }
          catch(Exception & E)
          {
            FTerminal->RollbackAction(Action, OperationProgress, &E);

            // Error formatting expanded from inline to avoid strange exceptions
            const UnicodeString Error =
              FMTLOAD(CHECKSUM_ERROR,
                (File != nullptr ? File->GetFullFileName() : L""));
            FTerminal->CommandError(&E, Error);
            TODO("retries? resume?");
            Next = false;
          }

        }
        __finally
        {
          if (FirstLevel && File)
          {
            OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
          }
        } end_try__finally

        if (OperationProgress->GetCancel() != csContinue)
        {
          Next = false;
        }
      }
      while (Next);
    }
  }
  __finally
  {
    Queue.DisposeSafe();
  } end_try__finally
  // queue is discarded here
}

UnicodeString TSFTPFileSystem::CalculateFilesChecksumInitialize(const UnicodeString & Alg)
{
  return FindIdent(Alg, FChecksumAlgs.get());
}

void TSFTPFileSystem::CustomCommandOnFile(const UnicodeString & /*AFileName*/,
    const TRemoteFile * /*AFile*/, const UnicodeString & /*Command*/, int32_t /*AParams*/,
    TCaptureOutputEvent && /*OutputEvent*/)
{
  DebugFail();
}

void TSFTPFileSystem::AnyCommand(const UnicodeString & /*Command*/,
  TCaptureOutputEvent && /*OutputEvent*/)
{
  DebugFail();
}

TStrings * TSFTPFileSystem::GetFixedPaths() const
{
  return FFixedPaths.get();
}

void TSFTPFileSystem::SpaceAvailable(const UnicodeString & APath,
  TSpaceAvailable & ASpaceAvailable)
{
  if (SupportsExtension(SFTP_EXT_SPACE_AVAILABLE) ||
      // See comment in IsCapable
      (FSecureShell->GetSshImplementation() == sshiBitvise))
  {
    TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
    Packet.AddString(SFTP_EXT_SPACE_AVAILABLE);
    AddPathString(Packet, LocalCanonify(APath));

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);

    ASpaceAvailable.BytesOnDevice = Packet.GetInt64();
    ASpaceAvailable.UnusedBytesOnDevice = Packet.GetInt64();
    ASpaceAvailable.BytesAvailableToUser = Packet.GetInt64();
    ASpaceAvailable.UnusedBytesAvailableToUser = Packet.GetInt64();
    // bytes-per-allocation-unit was added later to the protocol
    // (revision 07, while the extension was defined already in rev 06),
    // be tolerant
    if (Packet.CanGetCardinal())
    {
      ASpaceAvailable.BytesPerAllocationUnit = Packet.GetCardinal();
    }
    else if (Packet.CanGetSmallCardinal())
    {
      // See http://bugs.proftpd.org/show_bug.cgi?id=4079
      FTerminal->LogEvent("Assuming ProFTPD/mod_sftp bug of 2-byte bytes-per-allocation-unit field");
      ASpaceAvailable.BytesPerAllocationUnit = Packet.GetSmallCardinal();
    }
    else
    {
      FTerminal->LogEvent("Missing bytes-per-allocation-unit field");
    }
  }
  else if (DebugAlwaysTrue(FSupportsStatVfsV2))
  {
    // https://github.com/openssh/openssh-portable/blob/master/PROTOCOL
    TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
    Packet.AddString(SFTP_EXT_STATVFS);
    AddPathString(Packet, LocalCanonify(APath));

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);

    const int64_t BlockSize = Packet.GetInt64(); // file system block size
    const int64_t FundamentalBlockSize = Packet.GetInt64(); // fundamental fs block size
    const int64_t Blocks = Packet.GetInt64(); // number of blocks (unit f_frsize)
    const int64_t FreeBlocks = Packet.GetInt64(); // free blocks in file system
    const int64_t AvailableBlocks = Packet.GetInt64(); // free blocks for non-root
    const int64_t FileINodes = Packet.GetInt64(); // total file inodes
    const int64_t FreeFileINodes = Packet.GetInt64(); // free file inodes
    const int64_t AvailableFileINodes = Packet.GetInt64(); // free file inodes for to non-root
    const int64_t SID = Packet.GetInt64(); // file system id
    int64_t Flags = Packet.GetInt64(); // bit mask of f_flag values
    const int64_t NameMax = Packet.GetInt64(); // maximum filename length

    FTerminal->LogEvent(FORMAT("Block size: %s", ::Int64ToStr(BlockSize)));
    FTerminal->LogEvent(FORMAT("Fundamental block size: %s", ::Int64ToStr(FundamentalBlockSize)));
    FTerminal->LogEvent(FORMAT("Total blocks: %s", ::Int64ToStr(Blocks)));
    FTerminal->LogEvent(FORMAT("Free blocks: %s", ::Int64ToStr(FreeBlocks)));
    FTerminal->LogEvent(FORMAT("Free blocks for non-root: %s", ::Int64ToStr(AvailableBlocks)));
    FTerminal->LogEvent(FORMAT("Total file inodes: %s", ::Int64ToStr(FileINodes)));
    FTerminal->LogEvent(FORMAT("Free file inodes: %s", ::Int64ToStr(FreeFileINodes)));
    FTerminal->LogEvent(FORMAT("Free file inodes for non-root: %s", ::Int64ToStr(AvailableFileINodes)));
    FTerminal->LogEvent(FORMAT("File system ID: %s", BytesToHex(nb::ToUInt8Ptr(&SID), sizeof(SID))));
    UnicodeString FlagStr;
    if (FLAGSET(Flags, SFTP_EXT_STATVFS_ST_RDONLY))
    {
      AddToList(FlagStr, "read-only", L",");
      Flags -= SFTP_EXT_STATVFS_ST_RDONLY;
    }
    if (FLAGSET(Flags, SFTP_EXT_STATVFS_ST_NOSUID))
    {
      AddToList(FlagStr, "no-setuid", L",");
      Flags -= SFTP_EXT_STATVFS_ST_NOSUID;
    }
    if (Flags != 0)
    {
      AddToList(FlagStr, UnicodeString("0x") + ::IntToHex(nb::ToUInt32(Flags), 2), L",");
    }
    if (FlagStr.IsEmpty())
    {
      FlagStr = "none";
    }
    FTerminal->LogEvent(FORMAT("Flags: %s", FlagStr));
    FTerminal->LogEvent(FORMAT("Max name length: %s", ::Int64ToStr(NameMax)));

    ASpaceAvailable.BytesOnDevice = BlockSize * Blocks;
    ASpaceAvailable.UnusedBytesOnDevice = BlockSize * FreeBlocks;
    ASpaceAvailable.BytesAvailableToUser = 0;
    ASpaceAvailable.UnusedBytesAvailableToUser = BlockSize * AvailableBlocks;
    ASpaceAvailable.BytesPerAllocationUnit =
      (BlockSize > UINT_MAX /*std::numeric_limits<uint32_t>::max()*/) ? 0 : nb::ToUInt32(BlockSize);
  }
}

// transfer protocol

void TSFTPFileSystem::CopyToRemote(TStrings * AFilesToCopy,
  const UnicodeString & ATargetDir, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  const TAutoFlag AvoidBusyFlag(FAvoidBusy);
  FTerminal->DoCopyToRemote(AFilesToCopy, ATargetDir, CopyParam, Params, OperationProgress, tfPreCreateDir, OnceDoneOperation);
}

void TSFTPFileSystem::SFTPConfirmOverwrite(
  const UnicodeString & ASourceFullFileName, UnicodeString & ATargetFileName,
  const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress,
  TOverwriteMode & OverwriteMode, const TOverwriteFileParams * FileParams)
{
  const bool CanAppend = !FTerminal->IsEncryptingFiles() && ((FVersion < 4) || !OperationProgress->AsciiTransfer);
  bool CanResume =
    (FileParams != nullptr) &&
    (FileParams->DestSize < FileParams->SourceSize);
  uint32_t Answer;

  {
    const TSuspendFileOperationProgress Suspend(OperationProgress);
    uint32_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;

    // possibly we can allow alternate resume at least in some cases
    if (CanAppend)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[5];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(APPEND_BUTTON);
    Aliases[0].GroupWith = qaNo;
    Aliases[0].GroupedShiftState = ssAlt;
    Aliases[1] = TQueryButtonAlias::CreateAllAsYesToNewerGroupedWithYes();
    Aliases[2] = TQueryButtonAlias::CreateIgnoreAsRenameGroupedWithNo();
    Aliases[3] = TQueryButtonAlias::CreateYesToAllGroupedWithYes();
    Aliases[4] = TQueryButtonAlias::CreateNoToAllGroupedWithNo();
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.NoBatchAnswers = qaIgnore | qaRetry | qaAll;
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = _countof(Aliases);
    Answer = FTerminal->ConfirmFileOverwrite(
      ASourceFullFileName, ATargetFileName, FileParams,
      Answers, &QueryParams,
      ReverseOperationSide(OperationProgress->GetSide()),
      CopyParam, AParams, OperationProgress);
  }

  if (CanAppend &&
      ((Answer == qaRetry) || (Answer == qaSkip)))
  {
    OperationProgress->LockUserSelections();
    try__finally
    {
      // duplicated in TTerminal::ConfirmFileOverwrite
      const bool CanAlternateResume =
        FileParams ? (FileParams->DestSize < FileParams->SourceSize) && !OperationProgress->GetAsciiTransfer() : false;
      const TBatchOverwrite BatchOverwrite =
        FTerminal->EffectiveBatchOverwrite(ASourceFullFileName, CopyParam, AParams, OperationProgress, true);
      // when mode is forced by batch, never query user
      if (BatchOverwrite == boAppend)
      {
        OverwriteMode = omAppend;
      }
      else if (CanAlternateResume &&
               ((BatchOverwrite == boResume) || (BatchOverwrite == boAlternateResume)))
      {
        OverwriteMode = omResume;
      }
      // no other option, but append
      else if (!CanAlternateResume)
      {
        OverwriteMode = omAppend;
      }
      else
      {
        const TQueryParams Params(0, HELP_APPEND_OR_RESUME);

        {
          const TSuspendFileOperationProgress Suspend(OperationProgress);
          Answer = FTerminal->QueryUser(FORMAT(LoadStr(APPEND_OR_RESUME2), ASourceFullFileName),
            nullptr, qaYes | qaNo | qaNoToAll | qaCancel, &Params);
        }

        switch (Answer)
        {
          case qaYes:
            OverwriteMode = omAppend;
            break;

          case qaNo:
            OverwriteMode = omResume;
            break;

          case qaNoToAll:
            OverwriteMode = omResume;
            OperationProgress->SetBatchOverwrite(boAlternateResume);
            break;

          default: DebugFail();
            [[fallthrough]];
          case qaCancel:
            OperationProgress->SetCancelAtLeast(csCancel);
            Abort();
            break;
        }
      }
    }
    __finally
    {
      OperationProgress->UnlockUserSelections();
    } end_try__finally
  }
  else if (Answer == qaIgnore)
  {
    if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName, LoadStr(RENAME_TITLE), L"",
          LoadStr(RENAME_PROMPT2), true, 0, ATargetFileName))
    {
      OverwriteMode = omOverwrite;
    }
    else
    {
      OperationProgress->SetCancelAtLeast(csCancel);
      Abort();
    }
  }
  else
  {
    OverwriteMode = omOverwrite;
    switch (Answer)
    {
      case qaCancel:
        OperationProgress->SetCancelAtLeast(csCancel);
        Abort();
        break;

      case qaNo:
        throw ESkipFile();
    }
  }
}

bool TSFTPFileSystem::SFTPConfirmResume(const UnicodeString & DestFileName,
  bool PartialBiggerThanSource, TFileOperationProgressType * OperationProgress)
{
  bool ResumeTransfer = false;
  DebugAssert(OperationProgress);
  if (PartialBiggerThanSource)
  {
    uint32_t Answer;
    {
      const TSuspendFileOperationProgress Suspend(OperationProgress);
      const TQueryParams Params(qpAllowContinueOnError, HELP_PARTIAL_BIGGER_THAN_SOURCE);
      Answer = FTerminal->QueryUser(
        FMTLOAD(PARTIAL_BIGGER_THAN_SOURCE, DestFileName), nullptr,
          qaOK | qaAbort, &Params, qtWarning);
    }

    if (Answer == qaAbort)
    {
      OperationProgress->SetCancelAtLeast(csCancel);
      Abort();
    }
    ResumeTransfer = false;
  }
  else if (FTerminal->GetConfiguration()->GetConfirmResume())
  {
    uint32_t Answer;

    {
      const TSuspendFileOperationProgress Suspend(OperationProgress);
      const TQueryParams Params(qpAllowContinueOnError | qpNeverAskAgainCheck,
        HELP_RESUME_TRANSFER);
      // "abort" replaced with "cancel" to unify with "append/resume" query
      Answer = FTerminal->QueryUser(
        FMTLOAD(RESUME_TRANSFER2, DestFileName), nullptr, qaYes | qaNo | qaCancel,
        &Params);
    }

    switch (Answer) {
      case qaNeverAskAgain:
        FTerminal->GetConfiguration()->SetConfirmResume(false);
        [[fallthrough]];
      case qaYes:
        ResumeTransfer = true;
        break;

      case qaNo:
        ResumeTransfer = false;
        break;

      case qaCancel:
        OperationProgress->SetCancelAtLeast(csCancel);
        Abort();
        break;
    }
  }
  else
  {
    ResumeTransfer = true;
  }
  return ResumeTransfer;
}

bool TSFTPFileSystem::DoesFileLookLikeSymLink(TRemoteFile * File) const
{
  return
    (FVersion < 4) &&
    ((*File->Rights() & TRights::rfAll) == TRights::rfAll) &&
    (File->Size < 100);
}

void TSFTPFileSystem::Source(
  TLocalFileHandle & AHandle, const UnicodeString & ATargetDir, UnicodeString & ADestFileName,
  const TCopyParamType * CopyParam, int32_t Params,
  TFileOperationProgressType * OperationProgress, uint32_t Flags,
  TUploadSessionAction & Action, bool & ChildError)
{
  UnicodeString DestFullName = LocalCanonify(ATargetDir + ADestFileName);
  UnicodeString DestPartialFullName;
  bool ResumeAllowed;
  bool ResumeTransfer = false;
  bool DestFileExists = false;
  TRights DestRights;

  int64_t ResumeOffset = 0;

  // should we check for interrupted transfer?
  ResumeAllowed =
    !OperationProgress->AsciiTransfer &&
    CopyParam->AllowResume(OperationProgress->LocalSize, ADestFileName) &&
    IsCapable(fcRename) &&
    !FTerminal->IsEncryptingFiles() &&
    (CopyParam->FOnTransferIn.empty());

  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;

  TOverwriteFileParams FileParams;
  FileParams.SourceSize = OperationProgress->GetLocalSize();
  FileParams.SourceTimestamp = AHandle.Modification;

  if (ResumeAllowed)
  {
    DestPartialFullName = DestFullName + PartialExt;

    if (FLAGCLEAR(Flags, tfNewDirectory))
    {
      FTerminal->LogEvent("Checking existence of file.");
      TRemoteFile * File = nullptr;
      DestFileExists = RemoteFileExists(DestFullName, &File);
      std::unique_ptr<TRemoteFile> FilePtr(File);

      OperationProgress->Progress();

      if (DestFileExists)
      {
        FTerminal->LogEvent(FORMAT("File exists: %s", FTerminal->GetRemoteFileInfo(FilePtr.get())));
        OpenParams.DestFileSize = FilePtr->Resolve()->GetSize();
        FileParams.DestSize = OpenParams.DestFileSize;
        FileParams.DestTimestamp = FilePtr->GetModification();
        DestRights = *FilePtr->GetRights();
        // If destination file is symlink, never do resumable transfer,
        // as it would delete the symlink.
        if (FilePtr->GetIsSymLink())
        {
          ResumeAllowed = false;
          FTerminal->LogEvent("Existing file is symbolic link, not doing resumable transfer.");
        }
        // Also bit of heuristics to detect symlink on SFTP-3 and older
        // (which does not indicate symlink in SSH_FXP_ATTRS).
        // if file has all permissions and is small, then it is likely symlink.
        // also it is not likely that such a small file (if it is not symlink)
        // gets overwritten by large file (that would trigger resumable transfer).
        else if (DoesFileLookLikeSymLink(FilePtr.get()))
        {
          ResumeAllowed = false;
          FTerminal->LogEvent("Existing file looks like a symbolic link, not doing resumable transfer.");
        }
        // Also never do resumable transfer for file owned by other user
        // as deleting and recreating the file would change ownership.
        // This won't for work for SFTP-3 (OpenSSH) as it does not provide
        // owner name (only UID) and we know only logged in username (not UID)
        else if (!FilePtr->GetFileOwner().GetName().IsEmpty() && !base::SameUserName(FilePtr->GetFileOwner().GetName(), FTerminal->GetUserName()))
        {
          ResumeAllowed = false;
          FTerminal->LogEvent(
            FORMAT("Existing file is owned by another user [%s], not doing resumable transfer.", File->GetFileOwner().GetName()));
        }

        FilePtr.reset();
        // delete File;
        // File = nullptr;
      }

      if (ResumeAllowed)
      {
        FTerminal->LogEvent("Checking existence of partially transferred file.");
        if (RemoteFileExists(DestPartialFullName, &File))
        {
          ResumeOffset = FilePtr->Resolve()->GetSize(); // Though partial file should not be symlink
          FilePtr.reset();
          // delete File;
          // File = nullptr;

          bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->GetLocalSize());
          if (FLAGCLEAR(Params, cpNoConfirmation) &&
              FLAGCLEAR(Params, cpResume) &&
              !CopyParam->ResumeTransfer(AHandle.FileName))
          {
            ResumeTransfer = SFTPConfirmResume(ADestFileName,
              PartialBiggerThanSource, OperationProgress);
          }
          else
          {
            ResumeTransfer = !PartialBiggerThanSource;
          }

          if (!ResumeTransfer)
          {
            DoDeleteFile(DestPartialFullName, SSH_FXP_REMOVE);
            OperationProgress->Progress();
          }
          else
          {
            FTerminal->LogEvent("Resuming file transfer.");
          }
        }
        else
        {
          // partial upload file does not exist, check for full file
          if (DestFileExists)
          {
            const UnicodeString PrevDestFileName = ADestFileName;
            SFTPConfirmOverwrite(AHandle.FileName, ADestFileName,
              CopyParam, Params, OperationProgress, OpenParams.OverwriteMode, &FileParams);
            if (PrevDestFileName != ADestFileName)
            {
              // update paths in case user changes the file name
              DestFullName = LocalCanonify(ATargetDir + ADestFileName);
              DestPartialFullName = DestFullName + PartialExt;
              FTerminal->LogEvent("Checking existence of new file.");
              DestFileExists = RemoteFileExists(DestFullName, nullptr);
            }
          }
        }
      }
    }
  }

  // will the transfer be resumable?
  bool DoResume = (ResumeAllowed && (OpenParams.OverwriteMode == omOverwrite));

  const UnicodeString RemoteFileName = DoResume ? DestPartialFullName : DestFullName;
  OpenParams.FileName = AHandle.FileName;
  OpenParams.RemoteFileName = RemoteFileName;
  OpenParams.Resume = DoResume;
  OpenParams.Resuming = ResumeTransfer;
  OpenParams.OperationProgress = OperationProgress;
  OpenParams.CopyParam = CopyParam;
  OpenParams.Params = Params;
  OpenParams.FileParams = &FileParams;
  OpenParams.Confirmed = (!CopyParam->FOnTransferIn.empty()) && FLAGCLEAR(Params, cpAppend);
  OpenParams.DontRecycle = false;
  OpenParams.Recycled = false;

  FTerminal->LogEvent(0, L"Opening remote file.");
  FTerminal->FileOperationLoop(nb::bind(&TSFTPFileSystem::SFTPOpenRemote, this), OperationProgress, folAllowSkip,
    FMTLOAD(SFTP_CREATE_FILE_ERROR, OpenParams.RemoteFileName),
    &OpenParams);
  OperationProgress->Progress();

  if (OpenParams.RemoteFileName != RemoteFileName)
  {
    DebugAssert(!DoResume);
    DebugAssert(base::UnixExtractFilePath(OpenParams.RemoteFileName) == base::UnixExtractFilePath(RemoteFileName));
    DestFullName = OpenParams.RemoteFileName;
    const UnicodeString NewFileName = base::UnixExtractFileName(DestFullName);
    DebugAssert(ADestFileName != NewFileName);
    ADestFileName = NewFileName;
  }

  Action.Destination(DestFullName);

  bool TransferFinished = false;
  // int64_t DestWriteOffset = 0;
  TSFTPPacket CloseRequest(SSH_FXP_NONE, FCodePage);
  bool PreserveRights = CopyParam->PreserveRights && (CopyParam->FOnTransferIn.empty());
  bool PreserveExistingRights = (DoResume && DestFileExists) || OpenParams.Recycled;
  bool SetRights = (PreserveExistingRights || PreserveRights);
  bool PreserveTime = CopyParam->PreserveTime && (CopyParam->FOnTransferIn.empty());
  bool SetProperties = (PreserveTime || SetRights);
  TSFTPPacket PropertiesRequest(SSH_FXP_SETSTAT, FCodePage);
  TSFTPPacket PropertiesResponse(SSH_FXP_NONE, FCodePage);
  TRights Rights;
  if (SetProperties)
  {
    AddPathString(PropertiesRequest, DestFullName);
    if (PreserveRights)
    {
      Rights = CopyParam->RemoteFileRights(AHandle.Attrs);
    }
    else if (PreserveExistingRights)
    {
      if (DestFileExists)
      {
        Rights = DestRights;
      }
      else
      {
        Rights = OpenParams.RecycledRights;
      }
    }
    else
    {
      DebugAssert(!SetRights);
    }

    uint16_t RightsNumber = Rights.GetNumberSet();
    PropertiesRequest.AddProperties(
      SetRights ? &RightsNumber : nullptr, nullptr, nullptr,
      PreserveTime ? &AHandle.MTime : nullptr,
      nullptr, nullptr, false, FVersion, FUtfStrings);
  }

  try__finally
  {
    int64_t DestWriteOffset = 0;
    if (OpenParams.OverwriteMode == omAppend)
    {
      FTerminal->LogEvent("Appending file.");
      DestWriteOffset = OpenParams.DestFileSize;
    }
    else if (ResumeTransfer || (OpenParams.OverwriteMode == omResume))
    {
      if (OpenParams.OverwriteMode == omResume)
      {
        FTerminal->LogEvent("Resuming file transfer (append style).");
        ResumeOffset = OpenParams.DestFileSize;
      }
      FileSeek(static_cast<THandle>(AHandle.Handle), ResumeOffset, soFromBeginning);
      OperationProgress->AddResumed(ResumeOffset);
    }

    TEncryption Encryption(FTerminal->GetEncryptKey());
    const bool Encrypt = FTerminal->IsFileEncrypted(DestFullName, CopyParam->EncryptNewFiles);
    TSFTPUploadQueue Queue(this, FCodePage, (Encrypt ? &Encryption : nullptr));
    try__finally
    {
      const int32_t ConvertParams =
        FLAGMASK(CopyParam->GetRemoveCtrlZ(), cpRemoveCtrlZ) |
        FLAGMASK(CopyParam->GetRemoveBOM(), cpRemoveBOM);
      Queue.Init(AHandle.FileName, AHandle.Handle, std::move(const_cast<TCopyParamType * >(CopyParam)->FOnTransferIn), OperationProgress,
        OpenParams.RemoteFileHandle,
        DestWriteOffset + OperationProgress->GetTransferredSize(),
        ConvertParams);

      while (Queue.Continue())
      {
        if (OperationProgress->GetCancel())
        {
          if (OperationProgress->ClearCancelFile())
          {
            throw ESkipFile();
          }
          else
          {
            Abort();
          }
        }
      }

      // send close request before waiting for pending read responses
      SFTPCloseRemote(OpenParams.RemoteFileHandle, ADestFileName,
        OperationProgress, false, true, &CloseRequest);
      OpenParams.RemoteFileHandle = L"";

      // when resuming is disabled, we can send "set properties"
      // request before waiting for pending read/close responses
      if (SetProperties && !DoResume)
      {
        SendPacket(&PropertiesRequest);
        ReserveResponse(&PropertiesRequest, &PropertiesResponse);
      }
      // No error so far, processes pending responses and throw on first error
      Queue.DisposeSafeWithErrorHandling();
    }
    __finally
    {
      // Either queue is empty now (noop call then),
      // or some error occurred (in that case, process remaining responses, ignoring other errors)
      Queue.DisposeSafe();
      Encryption.Finalize();
    } end_try__finally

    TransferFinished = true;
    // queue is discarded here
  }
  __finally
  {
    if (FTerminal->GetActive())
    {
      // if file transfer was finished, the close request was already sent
      if (!OpenParams.RemoteFileHandle.IsEmpty())
      {
        SFTPCloseRemote(OpenParams.RemoteFileHandle, ADestFileName,
          OperationProgress, TransferFinished, true, &CloseRequest);
      }
      // wait for the response
      SFTPCloseRemote(OpenParams.RemoteFileHandle, ADestFileName,
        OperationProgress, TransferFinished, false, &CloseRequest);

      // delete file if transfer was not completed, resuming was not allowed and
      // we were not appending (incl. alternate resume),
      // shortly after plain transfer completes (eq. !ResumeAllowed)
      if (!TransferFinished && !DoResume && (OpenParams.OverwriteMode == omOverwrite))
      {
        DoDeleteFile(OpenParams.RemoteFileName, SSH_FXP_REMOVE);
      }
    }
  } end_try__finally

  OperationProgress->Progress();

  if (DoResume)
  {
    if (DestFileExists)
    {
      FILE_OPERATION_LOOP_BEGIN
      {
        if (GetSessionData()->GetOverwrittenToRecycleBin() &&
            !GetSessionData()->GetRecycleBinPath().IsEmpty())
        {
          FTerminal->RecycleFile(DestFullName, nullptr);
        }
        else
        {
          DoDeleteFile(DestFullName, SSH_FXP_REMOVE);
        }
      }
      FILE_OPERATION_LOOP_END(
        FMTLOAD(DELETE_ON_RESUME_ERROR,
          (base::UnixExtractFileName(DestFullName), DestFullName)));
    }

    // originally this was before CLOSE (last __finally statement),
    // on VShell it failed
    FILE_OPERATION_LOOP_BEGIN
    {
      this->RenameFile(OpenParams.RemoteFileName, nullptr, ADestFileName, false);
    }
    FILE_OPERATION_LOOP_END_CUSTOM(
      FMTLOAD(RENAME_AFTER_RESUME_ERROR,
        (base::UnixExtractFileName(OpenParams.RemoteFileName), ADestFileName)),
      folAllowSkip, HELP_RENAME_AFTER_RESUME_ERROR);
  }

  if (SetProperties)
  {
    std::unique_ptr<TTouchSessionAction> TouchAction;
    if (PreserveTime)
    {
      TDateTime MDateTime = ::UnixToDateTime(AHandle.MTime, GetSessionData()->GetDSTMode());
      FTerminal->LogEvent(FORMAT("Preserving timestamp [%s]",
        StandardTimestamp(MDateTime)));
      TouchAction = std::make_unique<TTouchSessionAction>(FTerminal->GetActionLog(), DestFullName,
        MDateTime);
    }
    std::unique_ptr<TChmodSessionAction> ChmodAction;
    // do record chmod only if it was explicitly requested,
    // not when it was implicitly performed to apply timestamp
    // of overwritten file to new file
    if (PreserveRights)
    {
      ChmodAction = std::make_unique<TChmodSessionAction>(FTerminal->GetActionLog(), DestFullName, Rights);
    }
    try
    {
      // when resuming is enabled, the set properties request was not sent yet
      if (DoResume)
      {
        SendPacket(&PropertiesRequest);
      }
      bool Resend = false;
      FILE_OPERATION_LOOP_BEGIN
      {
        try
        {
          TSFTPPacket DummyResponse(SSH_FXP_NONE, FCodePage);
          TSFTPPacket * Response = &PropertiesResponse;
          if (Resend)
          {
            PropertiesRequest.Reuse();
            SendPacket(&PropertiesRequest);
            // ReceiveResponse currently cannot receive twice into same packet,
            // so DummyResponse is temporary workaround
            Response = &DummyResponse;
          }
          Resend = true;
          ReceiveResponse(&PropertiesRequest, Response, SSH_FXP_STATUS,
            asOK | FLAGMASK(CopyParam->GetIgnorePermErrors(), asPermDenied));
        }
        catch(...)
        {
          if (FTerminal->GetActive() &&
              (!PreserveRights && !PreserveTime))
          {
            DebugAssert(DoResume);
            FTerminal->LogEvent("Ignoring error preserving permissions of overwritten file");
          }
          else
          {
            throw;
          }
        }
      }
      FILE_OPERATION_LOOP_END_CUSTOM(
        FMTLOAD(PRESERVE_TIME_PERM_ERROR3, ADestFileName),
        folAllowSkip, HELP_PRESERVE_TIME_PERM_ERROR);
    }
    catch(Exception & E)
    {
      if (TouchAction != nullptr)
      {
        TouchAction->Rollback(&E);
      }
      if (ChmodAction != nullptr)
      {
        ChmodAction->Rollback(&E);
      }
      ChildError = true;
      throw;
    }
  }
}

RawByteString TSFTPFileSystem::SFTPOpenRemoteFile(
  const UnicodeString & AFileName, SSH_FXF_TYPE OpenType, bool EncryptNewFiles, int64_t Size)
{
  TSFTPPacket Packet(SSH_FXP_OPEN, FCodePage);

  AddPathString(Packet, AFileName, EncryptNewFiles);
  if (FVersion < 5)
  {
    Packet.AddCardinal(OpenType);
  }
  else
  {
    const ACE4_TYPE Access =
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_READ), ACE4_READ_DATA) |
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_WRITE), ACE4_WRITE_DATA | ACE4_APPEND_DATA);

    SSH_FXF_TYPE Flags;

    if (FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_EXCL))
    {
      Flags = SSH_FXF_CREATE_NEW;
    }
    else if (FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_TRUNC))
    {
      Flags = SSH_FXF_CREATE_TRUNCATE;
    }
    else if (FLAGSET(OpenType, SSH_FXF_CREAT))
    {
      Flags = SSH_FXF_OPEN_OR_CREATE;
    }
    else
    {
      Flags = SSH_FXF_OPEN_EXISTING;
    }

    Flags |=
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_APPEND), SSH_FXF_ACCESS_APPEND_DATA) |
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_TEXT), SSH_FXF_ACCESS_TEXT_MODE);

    Packet.AddCardinal(Access);
    Packet.AddCardinal(Flags);
  }

  const bool SendSize =
    (Size >= 0) &&
    FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_TRUNC) &&
    // Particularly VanDyke VShell (4.0.3) does not support SSH_FILEXFER_ATTR_ALLOCATION_SIZE
    // (it fails open request when the attribute is included).
    // It's SFTP-6 attribute, so support structure should be available.
    // It's actually not with VShell. But VShell supports the SSH_FILEXFER_ATTR_ALLOCATION_SIZE.
    // All servers should support SSH_FILEXFER_ATTR_SIZE (SFTP < 6)
    (!FSupport->Loaded || FLAGSET(FSupport->AttributeMask, Packet.AllocationSizeAttribute(FVersion)));
  Packet.AddProperties(nullptr, nullptr, nullptr, nullptr, nullptr,
    SendSize ? &Size : nullptr, false, FVersion, FUtfStrings);

  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

  return Packet.GetFileHandle();
}

int32_t TSFTPFileSystem::SFTPOpenRemote(void * AOpenParams, void * /*Param2*/)
{
  TOpenRemoteFileParams * OpenParams = static_cast<TOpenRemoteFileParams *>(AOpenParams);
  DebugAssert(OpenParams);
  TFileOperationProgressType * OperationProgress = OpenParams->OperationProgress;

  SSH_FXF_TYPE OpenType = 0;
  bool Success = false;
  bool ConfirmOverwriting = false;

  do
  {
    try
    {
      ConfirmOverwriting =
        !OpenParams->Confirmed && !OpenParams->Resume &&
        FTerminal->CheckRemoteFile(OpenParams->FileName, OpenParams->CopyParam, OpenParams->Params, OperationProgress);
      OpenType = SSH_FXF_WRITE | SSH_FXF_CREAT;
      // when we want to preserve overwritten files, we need to find out that
      // they exist first... even if overwrite confirmation is disabled.
      // but not when we already know we are not going to overwrite (but e.g. to append)
      if ((ConfirmOverwriting || (GetSessionData()->FOverwrittenToRecycleBin && !OpenParams->DontRecycle)) &&
          (OpenParams->OverwriteMode == omOverwrite))
      {
        OpenType |= SSH_FXF_EXCL;
      }
      if (!OpenParams->Resuming && (OpenParams->OverwriteMode == omOverwrite))
      {
        OpenType |= SSH_FXF_TRUNC;
      }
      if ((FVersion >= 4) && OpenParams->OperationProgress->GetAsciiTransfer())
      {
        OpenType |= SSH_FXF_TEXT;
      }

      OpenParams->RemoteFileHandle =
        SFTPOpenRemoteFile(OpenParams->RemoteFileName, OpenType, OpenParams->CopyParam->EncryptNewFiles, OperationProgress->LocalSize);

      Success = true;
    }
    catch(Exception & E)
    {
      if (!OpenParams->Confirmed && (OpenType & SSH_FXF_EXCL) && FTerminal->GetActive())
      {
        FTerminal->LogEvent(FORMAT("Cannot create new file \"%s\", checking if it exists already", OpenParams->RemoteFileName));

        bool ThrowOriginal = false;
        std::unique_ptr<TRemoteFile> File;

        // When exclusive opening of file fails, try to detect if file exists.
        // When file does not exist, failure was probably caused by 'permission denied'
        // or similar error. In this case throw original exception.
        try
        {
          OperationProgress->Progress();
          TRemoteFile * LocalFile = nullptr;
          const UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
          ReadFile(RealFileName, LocalFile);
          File.reset(LocalFile);
          File->FullFileName = RealFileName;
          OpenParams->DestFileSize = File->GetSize(); // Resolve symlinks?
          if ((OpenParams->FileParams != nullptr) && (File != nullptr))
          {
            OpenParams->FileParams->DestTimestamp = File->GetModification();
            OpenParams->FileParams->DestSize = OpenParams->DestFileSize;
          }
        }
        catch(...)
        {
          if (!FTerminal->GetActive())
          {
            throw;
          }
          else
          {
            ThrowOriginal = true;
          }
        }

        if (ThrowOriginal)
        {
          throw;
        }

        // we may get here even if confirmation is disabled,
        // when we have preserving of overwritten files enabled
        if (ConfirmOverwriting)
        {
          OperationProgress->Progress();
          // confirmation duplicated in SFTPSource for resumable file transfers.
          UnicodeString RemoteFileNameOnly = base::UnixExtractFileName(OpenParams->RemoteFileName);
          SFTPConfirmOverwrite(OpenParams->FileName, RemoteFileNameOnly,
            OpenParams->CopyParam, OpenParams->Params, OperationProgress, OpenParams->OverwriteMode, OpenParams->FileParams);
          if (RemoteFileNameOnly != base::UnixExtractFileName(OpenParams->RemoteFileName))
          {
            OpenParams->RemoteFileName =
              base::UnixExtractFilePath(OpenParams->RemoteFileName) + RemoteFileNameOnly;
            // no longer points to a relevant file
            File.reset(nullptr);
          }
          OpenParams->Confirmed = true;
        }
        else
        {
          DebugAssert(GetSessionData()->GetOverwrittenToRecycleBin());
        }

        if ((OpenParams->OverwriteMode == omOverwrite) &&
            GetSessionData()->GetOverwrittenToRecycleBin() &&
            !FTerminal->GetSessionData()->GetRecycleBinPath().IsEmpty())
        {
          bool IsSymLink = (File != nullptr) && File->IsSymLink;
          if (!IsSymLink && (File != nullptr) && DoesFileLookLikeSymLink(File.get()) && IsCapable(fcResolveSymlink))
          {
            FTerminal->LogEvent(L"Existing file looks like a symbolic link, checking if it really is.");
            try
            {
              OperationProgress->Progress();
              TRemoteFile * LinkedFile;
              ReadSymlink(File.get(), LinkedFile);
              delete LinkedFile;
              IsSymLink = true;
            }
            catch(...)
            {
              if (!FTerminal->Active)
              {
                throw;
              }
            }
          }

          if (IsSymLink)
          {
            FTerminal->LogEvent(L"Existing file is a symbolic link, it will not be moved to a recycle bin.");
            OpenParams->DontRecycle = true;
          }
          else
          {
            OperationProgress->Progress();
            if (!FTerminal->RecycleFile(OpenParams->RemoteFileName, nullptr))
            {
              // Allow normal overwrite
              OpenParams->DontRecycle = true;
            }
            else
            {
              OpenParams->Recycled = true;
              OpenParams->RecycledRights = *File->Rights();
            }
          }
        }
      }
      else if (FTerminal->GetActive())
      {
        // if file overwriting was confirmed, it means that the file already exists,
        // if not, check now
        if (!OpenParams->Confirmed)
        {
          bool ThrowOriginal = false;

          // When file does not exist, failure was probably caused by 'permission denied'
          // or similar error. In this case throw original exception.
          try
          {
            TRemoteFile * File = nullptr;
            const UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
            ReadFile(RealFileName, File);
            SAFE_DESTROY(File);
          }
          catch(...)
          {
            if (!FTerminal->GetActive())
            {
              throw;
            }
            else
            {
              ThrowOriginal = true;
            }
          }

          if (ThrowOriginal)
          {
            throw;
          }
        }

        // now we know that the file exists

        if (FTerminal->FileOperationLoopQuery(E, OperationProgress,
              FMTLOAD(SFTP_OVERWRITE_FILE_ERROR2, OpenParams->RemoteFileName),
              folAllowSkip, LoadStr(SFTP_OVERWRITE_DELETE_BUTTON)))
        {
          OperationProgress->Progress();
          int32_t Params = dfNoRecursive;
          FTerminal->DeleteFile(OpenParams->RemoteFileName, nullptr, &Params);
        }
      }
      else
      {
        throw;
      }
    }
  }
  while (!Success);

  return 0;
}

void TSFTPFileSystem::SFTPCloseRemote(const RawByteString & Handle,
  const UnicodeString & AFileName, TFileOperationProgressType * OperationProgress,
  bool TransferFinished, bool Request, TSFTPPacket * Packet)
{
  // Moving this out of SFTPSource() fixed external exception 0xC0000029 error
  FILE_OPERATION_LOOP_BEGIN
  {
    try
    {
      TSFTPPacket CloseRequest(SSH_FXP_NONE, FCodePage);
      TSFTPPacket * P = (Packet == nullptr) ? &CloseRequest : Packet;

      if (Request)
      {
        P->ChangeType(SSH_FXP_CLOSE);
        P->AddString(Handle);
        SendPacket(P);
        ReserveResponse(P, Packet);
      }
      else
      {
        DebugAssert(Packet != nullptr);
        ReceiveResponse(P, Packet, SSH_FXP_STATUS);
      }
    }
    catch(...)
    {
      if (!FTerminal->GetActive() || TransferFinished)
      {
        throw;
      }
    }
  }
  FILE_OPERATION_LOOP_END(FMTLOAD(SFTP_CLOSE_FILE_ERROR, AFileName));
}

void TSFTPFileSystem::CopyToLocal(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  int32_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  const TAutoFlag AvoidBusyFlag(FAvoidBusy);
  FTerminal->DoCopyToLocal(AFilesToCopy, TargetDir, CopyParam, Params, OperationProgress, tfNone, OnceDoneOperation);
}

void TSFTPFileSystem::DirectorySunk(
  const UnicodeString & ADestFullName, const TRemoteFile * AFile, const TCopyParamType * CopyParam)
{
  if (CopyParam->GetPreserveTime() && CopyParam->GetPreserveTimeDirs())
  {
    // FILE_FLAG_BACKUP_SEMANTICS is needed to "open" directory
    HANDLE LocalFileHandle =
      CreateFile(
        ApiPath(ADestFullName).data(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (LocalFileHandle == INVALID_HANDLE_VALUE)
    {
      const int32_t SetFileTimeError = nb::ToInt32(::GetLastError());
      FTerminal->LogEvent(
        FORMAT("Preserving directory timestamp failed, ignoring: %s", ::SysErrorMessageForError(SetFileTimeError)));
    }
    else
    {
      FTerminal->UpdateTargetTime(
        LocalFileHandle, AFile->GetModification(), AFile->GetModificationFmt(), FTerminal->GetSessionData()->GetDSTMode());
      SAFE_CLOSE_HANDLE(LocalFileHandle);
    }
  }
}

void TSFTPFileSystem::WriteLocalFile(
  const TCopyParamType * CopyParam, TStream * FileStream, TFileBuffer & BlockBuf, const UnicodeString & ALocalFileName,
  TFileOperationProgressType * OperationProgress)
{
  if (!CopyParam->FOnTransferOut.empty())
  {
    BlockBuf.WriteToOut(std::move(const_cast<TCopyParamType * >(CopyParam)->FOnTransferOut), FTerminal, BlockBuf.Size);
  }
  else
  {
    FILE_OPERATION_LOOP_BEGIN
    {
      BlockBuf.WriteToStream(FileStream, BlockBuf.Size);
    }
    FILE_OPERATION_LOOP_END(FMTLOAD(WRITE_ERROR, ALocalFileName));
  }

  OperationProgress->AddLocallyUsed(BlockBuf.Size);
}

void TSFTPFileSystem::Sink(
  const UnicodeString & AFileName, const TRemoteFile * AFile,
  const UnicodeString & ATargetDir, UnicodeString & ADestFileName, int32_t Attrs,
  const TCopyParamType * CopyParam, int32_t AParams, TFileOperationProgressType * OperationProgress,
  uint32_t /*AFlags*/, TDownloadSessionAction & Action)
{
  // resume has no sense for temporary downloads
  bool ResumeAllowed =
    FLAGCLEAR(AParams, cpTemporary) &&
    !OperationProgress->GetAsciiTransfer() &&
    CopyParam->AllowResume(OperationProgress->TransferSize, ADestFileName) &&
    !FTerminal->IsEncryptingFiles() &&
    CopyParam->FOnTransferOut.empty() &&
    (CopyParam->PartOffset < 0);

  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  TStream * FileStream = nullptr; // TODO: use std::unique_ptr<>
  bool DeleteLocalFile = false;
  RawByteString RemoteHandle;
  UnicodeString DestFullName = ATargetDir + ADestFileName;
  UnicodeString LocalFileName = DestFullName;
  TOverwriteMode OverwriteMode = omOverwrite;
  UnicodeString ExpandedDestFullName;

  try__finally
  {
    bool ResumeTransfer = false;
    UnicodeString DestPartialFullName;

    if (ResumeAllowed)
    {
      DestPartialFullName = DestFullName + PartialExt;
      LocalFileName = DestPartialFullName;

      FTerminal->LogEvent("Checking existence of partially transferred file.");
      if (base::FileExists(ApiPath(DestPartialFullName)))
      {
        FTerminal->LogEvent("Partially transferred file exists.");
        int64_t ResumeOffset;
        FTerminal->OpenLocalFile(DestPartialFullName, GENERIC_WRITE,
          nullptr, &LocalFileHandle, nullptr, nullptr, nullptr, &ResumeOffset);

        const bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->GetTransferSize());
        if (FLAGCLEAR(AParams, cpNoConfirmation))
        {
          ResumeTransfer = SFTPConfirmResume(ADestFileName, PartialBiggerThanSource, OperationProgress);
        }
        else
        {
          ResumeTransfer = !PartialBiggerThanSource;
          if (!ResumeTransfer)
          {
            FTerminal->LogEvent("Partially transferred file is bigger than original file.");
          }
        }

        if (!ResumeTransfer)
        {
          SAFE_CLOSE_HANDLE(LocalFileHandle);
          LocalFileHandle = INVALID_HANDLE_VALUE;
          FTerminal->DoDeleteLocalFile(DestPartialFullName);
        }
        else
        {
          FTerminal->LogEvent("Resuming file transfer.");
          FileSeek(static_cast<THandle>(LocalFileHandle), ResumeOffset, soFromBeginning);
          OperationProgress->AddResumed(ResumeOffset);
        }
      }

      OperationProgress->Progress();
    }

    // first open source file, not to lose the destination file,
    // if we cannot open the source one in the first place
    FTerminal->LogEvent("Opening remote file.");
    FILE_OPERATION_LOOP_BEGIN
    {
      SSH_FXF_TYPE OpenType = SSH_FXF_READ;
      if ((FVersion >= 4) && OperationProgress->GetAsciiTransfer())
      {
        OpenType |= SSH_FXF_TEXT;
      }
      RemoteHandle = SFTPOpenRemoteFile(AFileName, OpenType);
      OperationProgress->Progress();
    }
    FILE_OPERATION_LOOP_END(FMTLOAD(SFTP_OPEN_FILE_ERROR, AFileName));

    FILETIME AcTime;
    nb::ClearStruct(AcTime);
    FILETIME WrTime;
    nb::ClearStruct(WrTime);
    TSFTPPacket RemoteFilePacket(SSH_FXP_FSTAT, FCodePage);
    RemoteFilePacket.AddString(RemoteHandle);
    SendCustomReadFile(&RemoteFilePacket, &RemoteFilePacket, SSH_FILEXFER_ATTR_MODIFYTIME);
    ReceiveResponse(&RemoteFilePacket, &RemoteFilePacket);
    OperationProgress->Progress();

    TDateTime Modification = AFile->GetModification(); // fallback
    TModificationFmt ModificationFmt = AFile->ModificationFmt;
    // ignore errors
    if (RemoteFilePacket.GetType() == SSH_FXP_ATTRS)
    {
      // load file, avoid completion (resolving symlinks) as we do not need that
      const std::unique_ptr<TRemoteFile> File(
        LoadFile(&RemoteFilePacket, nullptr, base::UnixExtractFileName(AFileName), nullptr, false));
      if (File->GetModification() != TDateTime())
      {
        Modification = File->GetModification();
        ModificationFmt = File->ModificationFmt;
      }
    }

    if ((Attrs >= 0) && !ResumeTransfer)
    {
      int64_t DestFileSize;
      int64_t MTime;
      FTerminal->OpenLocalFile(
        DestFullName, GENERIC_WRITE, nullptr, &LocalFileHandle, nullptr, &MTime, nullptr, &DestFileSize, false);

      FTerminal->LogEvent("Confirming overwriting of file.");
      TOverwriteFileParams FileParams;
      FileParams.SourceSize = OperationProgress->GetTransferSize();
      FileParams.SourceTimestamp = Modification;
      FileParams.DestTimestamp = ::UnixToDateTime(MTime,
        FTerminal->GetSessionData()->GetDSTMode());
      FileParams.DestSize = DestFileSize;
      const UnicodeString PrevDestFileName = ADestFileName;
      SFTPConfirmOverwrite(AFileName, ADestFileName, CopyParam, AParams, OperationProgress, OverwriteMode, &FileParams);
      if (PrevDestFileName != ADestFileName)
      {
        DestFullName = ATargetDir + ADestFileName;
        DestPartialFullName = DestFullName + PartialExt;
        if (ResumeAllowed)
        {
          if (base::FileExists(ApiPath(DestPartialFullName)))
          {
            FTerminal->DoDeleteLocalFile(DestPartialFullName);
          }
          LocalFileName = DestPartialFullName;
        }
        else
        {
          LocalFileName = DestFullName;
        }
      }

      if (OverwriteMode == omOverwrite)
      {
        // is NULL when overwriting read-only file
        if (LocalFileHandle)
        {
          SAFE_CLOSE_HANDLE(LocalFileHandle);
          LocalFileHandle = INVALID_HANDLE_VALUE;
        }
      }
      else
      {
        // is NULL when overwriting read-only file, so following will
        // probably fail anyway
        if ((LocalFileHandle == INVALID_HANDLE_VALUE) || (LocalFileHandle == nullptr))
        {
          FTerminal->OpenLocalFile(DestFullName, GENERIC_WRITE, nullptr, &LocalFileHandle, nullptr, nullptr, nullptr, nullptr);
        }
        ResumeAllowed = false;
        FileSeek(static_cast<THandle>(LocalFileHandle), DestFileSize, soFromBeginning);
        if (OverwriteMode == omAppend)
        {
          FTerminal->LogEvent("Appending to file.");
        }
        else
        {
          FTerminal->LogEvent("Resuming file transfer (append style).");
          DebugAssert(OverwriteMode == omResume);
          OperationProgress->AddResumed(DestFileSize);
        }
      }
    }

    Action.Destination(ExpandUNCFileName(DestFullName));

    if (CopyParam->FOnTransferOut.empty())
    {
      // if not already opened (resume, append...), create new empty file
      if (!CheckHandle(LocalFileHandle))
      {
        if (!FTerminal->CreateLocalFile(LocalFileName, OperationProgress,
             &LocalFileHandle, FLAGSET(AParams, cpNoConfirmation)))
        {
          throw ESkipFile();
        }
      }
      DebugAssert(CheckHandle(LocalFileHandle));

      DeleteLocalFile = true;

      FileStream = new TSafeHandleStream(static_cast<THandle>(LocalFileHandle));
    }

    // at end of this block queue is discarded
    {
      TSFTPDownloadQueue Queue(this, FCodePage);
      try__finally
      {
        TSFTPPacket DataPacket(SSH_FXP_NONE, FCodePage);

        int32_t QueueLen = nb::ToInt32(OperationProgress->TransferSize / DownloadBlockSize(OperationProgress)) + 1;
        if ((QueueLen > GetSessionData()->GetSFTPDownloadQueue()) ||
            (QueueLen < 0))
        {
          QueueLen = FTerminal->GetSessionData()->GetSFTPDownloadQueue();
        }
        if (QueueLen < 1)
        {
          QueueLen = 1;
        }
        const int64_t Offset = OperationProgress->TransferredSize + std::max(CopyParam->PartOffset, 0LL);
        Queue.Init(QueueLen, RemoteHandle, Offset, CopyParam->PartSize, OperationProgress);

        bool Eof = false;
        bool PrevIncomplete = false;
        int32_t GapFillCount = 0;
        int32_t GapCount = 0;
        uint32_t Missing = 0;
        uint32_t DataLen = 0;
        uint32_t BlockSize = 0;
        bool ConvertToken = false;
        TEncryption Encryption(FTerminal->GetEncryptKey());
        const bool Decrypt = FTerminal->IsFileEncrypted(AFileName);

        while (!Eof)
        {
          if (Missing > 0)
          {
            Queue.InitFillGapRequest(Offset + OperationProgress->GetTransferredSize(), Missing, &DataPacket);
            GapFillCount++;
            SendPacketAndReceiveResponse(&DataPacket, &DataPacket, SSH_FXP_DATA, asEOF);
          }
          else
          {
            Queue.ReceivePacket(&DataPacket, BlockSize);
          }

          if (DataPacket.GetType() == SSH_FXP_STATUS)
          {
            // must be SSH_FX_EOF, any other status packet would raise exception
            Eof = true;
            // close file right away, before waiting for pending responses
            SFTPCloseRemote(RemoteHandle, ADestFileName, OperationProgress, true, true, nullptr);
            RemoteHandle = L""; // do not close file again in __finally block
          }

          if (!Eof)
          {
            if ((Missing == 0) && PrevIncomplete)
            {
              // This can happen only if last request returned less bytes
              // than expected, but exactly number of bytes missing to last
              // known file size, but actually EOF was not reached.
              // Can happen only when filesize has changed since directory
              // listing and server returns less bytes than requested and
              // file has some special file size.
              FTerminal->LogEvent(FORMAT(
                "Received incomplete data packet before end of file, offset: %s, size: %d, requested: %d",
                ::Int64ToStr(OperationProgress->GetTransferredSize()), nb::ToInt32(DataLen), nb::ToInt32(BlockSize)));
              FTerminal->TerminalError(nullptr, LoadStr(SFTP_INCOMPLETE_BEFORE_EOF));
            }

            // Buffer for one block of data
            TFileBuffer BlockBuf;

            DataLen = DataPacket.GetCardinal();

            PrevIncomplete = false;
            if (Missing > 0)
            {
              DebugAssert(DataLen <= Missing);
              Missing -= DataLen;
            }
            else if (DataLen < BlockSize)
            {
              if (OperationProgress->GetTransferredSize() + DataLen !=
                    OperationProgress->GetTransferSize())
              {
                // with native text transfer mode (SFTP>=4), do not bother about
                // getting less than requested, read offset is ignored anyway
                if ((FVersion < 4) || !OperationProgress->GetAsciiTransfer())
                {
                  GapCount++;
                  Missing = nb::ToUInt32(BlockSize - DataLen);
                }
              }
              else
              {
                PrevIncomplete = true;
              }
            }

            DebugAssert(DataLen <= BlockSize);
            BlockBuf.Insert(0, reinterpret_cast<const char *>(DataPacket.GetNextData(DataLen)), DataLen);
            DataPacket.DataConsumed(DataLen);
            OperationProgress->AddTransferred(DataLen);

            if ((FVersion >= 6) && DataPacket.CanGetBool() && (Missing == 0))
            {
              Eof = DataPacket.GetBool();
            }

            if ((CopyParam->PartSize >= 0) &&
                (OperationProgress->TransferredSize >= CopyParam->PartSize))
            {
              Eof = true;
            }

            if (OperationProgress->AsciiTransfer)
            {
              DebugAssert(!ResumeTransfer && !ResumeAllowed);

              const int64_t PrevBlockSize = BlockBuf.GetSize();
              BlockBuf.Convert(GetEOL(), FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
              OperationProgress->SetLocalSize(OperationProgress->GetLocalSize() - PrevBlockSize + BlockBuf.GetSize());
            }

            if (Decrypt)
            {
              Encryption.Decrypt(BlockBuf);
            }

            WriteLocalFile(CopyParam, FileStream, BlockBuf, LocalFileName, OperationProgress);
          }

          if (OperationProgress->GetCancel() != csContinue)
          {
            if (OperationProgress->ClearCancelFile())
            {
              throw ESkipFile();
            }
            else
            {
              Abort();
            }
          }
        }

        if (GapCount > 0)
        {
          FTerminal->LogEvent(FORMAT("%d requests to fill %d data gaps were issued.", GapFillCount, GapCount));
        }

        if (Decrypt)
        {
          TFileBuffer BlockBuf;
          if (Encryption.DecryptEnd(BlockBuf))
          {
            WriteLocalFile(CopyParam, FileStream, BlockBuf, LocalFileName, OperationProgress);
          }
        }
        Encryption.Finalize();
      }
      __finally
      {
        Queue.DisposeSafe();
      } end_try__finally
      // queue is discarded here
    }

    if (CopyParam->FOnTransferOut.empty())
    {
      DebugAssert(CheckHandle(LocalFileHandle));
      if (CopyParam->GetPreserveTime())
      {
        FTerminal->UpdateTargetTime(LocalFileHandle, Modification, ModificationFmt, FTerminal->GetSessionData()->GetDSTMode());
      }

      SAFE_CLOSE_HANDLE(LocalFileHandle);
      LocalFileHandle = nullptr;

      if (ResumeAllowed)
      {
        // See also DoRenameLocalFileForce
        FILE_OPERATION_LOOP_BEGIN
        {
          if (base::FileExists(DestFullName))
          {
            DeleteFileChecked(DestFullName);
          }
          THROWOSIFFALSE(base::RenameFile(DestPartialFullName, DestFullName));
        }
        FILE_OPERATION_LOOP_END(FMTLOAD(RENAME_AFTER_RESUME_ERROR, base::UnixExtractFileName(DestPartialFullName), ADestFileName));
      }

      DeleteLocalFile = false;

      FTerminal->UpdateTargetAttrs(DestFullName, AFile, CopyParam, Attrs);
    }

  }
  __finally
  {
    if (CheckHandle(LocalFileHandle))
    {
      SAFE_CLOSE_HANDLE(LocalFileHandle);
    }

    if (FileStream != nullptr)
    {
      SAFE_DESTROY(FileStream);
    }

    if (FTerminal && DeleteLocalFile && (!ResumeAllowed || OperationProgress->GetLocallyUsed() == 0) &&
        (OverwriteMode == omOverwrite))
    {
      FTerminal->DoDeleteLocalFile(LocalFileName);
    }

    // if the transfer was finished, the file is usually closed already
    // (except for special cases like SFTPv6 EOF indication or partial file download)
    if (FTerminal && FTerminal->GetActive() && !RemoteHandle.IsEmpty())
    {
      // do not wait for response
      SFTPCloseRemote(RemoteHandle, ADestFileName, OperationProgress, true, true, nullptr);
    }
  } end_try__finally
}

void TSFTPFileSystem::RegisterChecksumAlg(const UnicodeString & Alg, const UnicodeString & SftpAlg)
{
  FChecksumAlgs->Add(Alg);
  FChecksumSftpAlgs->Add(SftpAlg);
}

void TSFTPFileSystem::GetSupportedChecksumAlgs(TStrings * Algs)
{
  Algs->AddStrings(FChecksumAlgs.get());
}

void TSFTPFileSystem::LockFile(const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TSFTPFileSystem::UnlockFile(const UnicodeString & /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}

void TSFTPFileSystem::UpdateFromMain(TCustomFileSystem * /*MainFileSystem*/)
{
  // noop
}

void TSFTPFileSystem::ClearCaches()
{
  // noop
}

void TSFTPFileSystem::AddPathString(TSFTPPacket & Packet, const UnicodeString & Value, bool EncryptNewFiles)
{
  const UnicodeString EncryptedPath = FTerminal->EncryptFileName(Value, EncryptNewFiles);
  Packet.AddPathString(EncryptedPath, FUtfStrings);
}

