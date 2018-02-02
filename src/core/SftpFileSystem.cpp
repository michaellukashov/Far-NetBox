
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <Exceptions.h>
#include <WideStrUtils.hpp>
#include <memory>

#include "SftpFileSystem.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "SecureShell.h"
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
__removed #define FILE_OPERATION_LOOP_TERMINAL FTerminal
//---------------------------------------------------------------------------
static const SSH_FX_TYPES
  SSH_FX_OK = 0,
  SSH_FX_EOF = 1,
  SSH_FX_NO_SUCH_FILE = 2,
  SSH_FX_PERMISSION_DENIED = 3,
  SSH_FX_FAILURE = 4,
  SSH_FX_OP_UNSUPPORTED = 8;

static const SSH_FXP_TYPES
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

static const SSH_FILEXFER_ATTR_TYPES
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

static const SSH_FILEXFER_ATTR_TYPES
SSH_FILEXFER_ATTR_COMMON =
  (SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_OWNERGROUP |
   SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACCESSTIME |
   SSH_FILEXFER_ATTR_MODIFYTIME);

static const SSH_FILEXFER_TYPES
  SSH_FILEXFER_TYPE_REGULAR = 1,
  SSH_FILEXFER_TYPE_DIRECTORY = 2,
  SSH_FILEXFER_TYPE_SYMLINK = 3,
  SSH_FILEXFER_TYPE_SPECIAL = 4,
  SSH_FILEXFER_TYPE_UNKNOWN = 5;

static const SSH_FXF_TYPES
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

static const ACE4_TYPES
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

static const uint32_t SSH_FILEXFER_ATTR_FLAGS_HIDDEN = 0x00000004;

typedef uint8_t SSH_FXP_REALPATH_TYPES;
static const SSH_FXP_REALPATH_TYPES
  SSH_FXP_REALPATH_NO_CHECK = 0x00000001,
  SSH_FXP_REALPATH_STAT_IF = 0x00000002,
  SSH_FXP_REALPATH_STAT_ALWAYS = 0x00000003;

static const intptr_t SFTP_MAX_PACKET_LEN = 1000 * 1024;
//---------------------------------------------------------------------------
#define SFTP_EXT_OWNER_GROUP "owner-group-query@generic-extensions"
#define SFTP_EXT_OWNER_GROUP_REPLY "owner-group-query-reply@generic-extensions"
#define SFTP_EXT_NEWLINE "newline"
#define SFTP_EXT_SUPPORTED "supported"
#define SFTP_EXT_SUPPORTED2 "supported2"
#define SFTP_EXT_FSROOTS "fs-roots@vandyke.com"
#define SFTP_EXT_VENDOR_ID "vendor-id"
#define SFTP_EXT_VERSIONS "versions"
#define SFTP_EXT_SPACE_AVAILABLE "space-available"
#define SFTP_EXT_CHECK_FILE "check-file"
#define SFTP_EXT_CHECK_FILE_NAME "check-file-name"
#define SFTP_EXT_STATVFS "statvfs@openssh.com"
#define SFTP_EXT_STATVFS_VALUE_V2 L"2"
#define SFTP_EXT_STATVFS_ST_RDONLY 0x1
#define SFTP_EXT_STATVFS_ST_NOSUID 0x2
#define SFTP_EXT_HARDLINK "hardlink@openssh.com"
#define SFTP_EXT_HARDLINK_VALUE_V1 L"1"
#define SFTP_EXT_COPY_FILE "copy-file"
//---------------------------------------------------------------------------
static const wchar_t OGQ_LIST_OWNERS = 0x01;
static const wchar_t OGQ_LIST_GROUPS = 0x02;
//---------------------------------------------------------------------------
static const uint32_t SFTPNoMessageNumber = ToUInt32(-1);

static const SSH_FX_TYPES asNo =            0;
static const SSH_FX_TYPES asOK =            1 << SSH_FX_OK;
static const SSH_FX_TYPES asEOF =           1 << SSH_FX_EOF;
static const SSH_FX_TYPES asPermDenied =    1 << SSH_FX_PERMISSION_DENIED;
static const SSH_FX_TYPES asOpUnsupported = 1 << SSH_FX_OP_UNSUPPORTED;
static const SSH_FX_TYPES asNoSuchFile =    1 << SSH_FX_NO_SUCH_FILE;
static const SSH_FX_TYPES asAll = static_cast<SSH_FX_TYPES>(0xFFFF);
#if 0
//---------------------------------------------------------------------------
#define GET_32BIT(cp) \
    (((unsigned long)(unsigned char)(cp)[0] << 24) | \
    ((unsigned long)(unsigned char)(cp)[1] << 16) | \
    ((unsigned long)(unsigned char)(cp)[2] << 8) | \
    ((unsigned long)(unsigned char)(cp)[3]))

#define PUT_32BIT(cp, value) { \
    (cp)[0] = (unsigned char)((value) >> 24); \
    (cp)[1] = (unsigned char)((value) >> 16); \
    (cp)[2] = (unsigned char)((value) >> 8); \
    (cp)[3] = (unsigned char)(value); }
#endif // #if 0
//---------------------------------------------------------------------------
static const uintptr_t SFTP_PACKET_ALLOC_DELTA = 256;
//---------------------------------------------------------------------------
__removed #pragma warn -inl
//---------------------------------------------------------------------------
struct TSFTPSupport : public TObject
{
  NB_DISABLE_COPY(TSFTPSupport)
public:
  TSFTPSupport() :
    AttributeMask(0),
    AttributeBits(0),
    OpenFlags(0),
    AccessMask(0),
    MaxReadSize(0),
    OpenBlockVector(0),
    BlockVector(0),
    AttribExtensions(new TStringList()),
    Extensions(new TStringList()),
    Loaded(false)
  {
    Reset();
  }

  ~TSFTPSupport()
  {
    SAFE_DESTROY(AttribExtensions);
    SAFE_DESTROY(Extensions);
  }

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
    Extensions->Clear();
    Loaded = false;
  }

  SSH_FILEXFER_ATTR_TYPES AttributeMask;
  uint32_t AttributeBits;
  uint32_t OpenFlags;
  uint32_t AccessMask;
  uint32_t MaxReadSize;
  uint32_t OpenBlockVector;
  uint32_t BlockVector;
  TStrings *AttribExtensions;
  TStrings *Extensions;
  bool Loaded;
};
//---------------------------------------------------------------------------
class TSFTPPacket : public TObject
{
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSFTPPacket); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPPacket) || TObject::is(Kind); }
public:
  explicit TSFTPPacket(uintptr_t CodePage) :
    TObject(OBJECT_CLASS_TSFTPPacket)
  {
    Init(CodePage);
  }

  explicit TSFTPPacket(TObjectClassId Kind, uintptr_t CodePage) :
    TObject(Kind)
  {
    Init(CodePage);
  }

  explicit TSFTPPacket(const TSFTPPacket &Source) :
    TObject(OBJECT_CLASS_TSFTPPacket)
  {
    this->operator=(Source);
  }

  explicit TSFTPPacket(const TSFTPPacket &Source, uintptr_t CodePage) :
    TObject(OBJECT_CLASS_TSFTPPacket)
  {
    Init(CodePage);
    *this = Source;
  }

  explicit TSFTPPacket(SSH_FXP_TYPES AType, uintptr_t CodePage) :
    TObject(OBJECT_CLASS_TSFTPPacket)
  {
    Init(CodePage);
    ChangeType(AType);
  }

  explicit TSFTPPacket(const uint8_t *Source, uintptr_t Len, uintptr_t CodePage) :
    TObject(OBJECT_CLASS_TSFTPPacket)
  {
    Init(CodePage);
    FLength = Len;
    SetCapacity(FLength);
    memmove(GetData(), Source, Len);
  }

  explicit TSFTPPacket(const RawByteString Source, uintptr_t CodePage) :
    TObject(OBJECT_CLASS_TSFTPPacket)
  {
    Init(CodePage);
    FLength = ToUIntPtr(Source.Length());
    SetCapacity(FLength);
    memmove(GetData(), Source.c_str(), Source.Length());
  }

  virtual ~TSFTPPacket()
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

  void ChangeType(SSH_FXP_TYPES AType)
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
    uint8_t Buf[4];
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
    uint8_t Buf[4];
    PUT_32BIT(Buf, Value);
    Add(&Buf, sizeof(Buf));
  }

  void AddInt64(int64_t Value)
  {
    AddCardinal(ToUInt32(Value >> 32));
    AddCardinal(ToUInt32(Value & 0xFFFFFFFF));
  }

  void AddData(const void *Data, int32_t ALength)
  {
    AddCardinal(ALength);
    Add(Data, ALength);
  }

  void AddStringW(const UnicodeString ValueW)
  {
    AddString(::W2MB(ValueW.c_str(), static_cast<UINT>(FCodePage)).c_str());
  }

  void AddString(const RawByteString Value)
  {
    AddCardinal(ToUInt32(Value.Length()));
    Add(Value.c_str(), Value.Length());
  }

  void AddUtfString(const UTF8String Value)
  {
    AddString(Value);
  }

  void AddUtfString(const UnicodeString Value)
  {
    AddUtfString(UTF8String(Value));
  }

  void AddString(const UnicodeString Value, TAutoSwitch /*Utf*/)
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
  inline void AddPathString(const UnicodeString Value, TAutoSwitch Utf)
  {
    AddString(Value, Utf);
  }

  static SSH_FILEXFER_ATTR_TYPES AllocationSizeAttribute(intptr_t Version)
  {
    return (Version >= 6) ? SSH_FILEXFER_ATTR_ALLOCATION_SIZE : SSH_FILEXFER_ATTR_SIZE;
  }

  void AddProperties(uint16_t *Rights, TRemoteToken *Owner,
    TRemoteToken *Group, int64_t *MTime, int64_t *ATime,
    int64_t *Size, bool IsDirectory, intptr_t Version, TAutoSwitch Utf)
  {
    SSH_FILEXFER_ATTR_TYPES Flags = 0;
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
        AddCardinal(ToUInt32(Owner->GetID()));
        AddCardinal(ToUInt32(Group->GetID()));
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
      AddCardinal(ToUInt32(ATime != nullptr ? *ATime : MTime != nullptr ? *MTime : 0));
      AddCardinal(ToUInt32(MTime != nullptr ? *MTime : ATime != nullptr ? *ATime : 0));
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

  void AddProperties(const TRemoteProperties *Properties,
    uint16_t BaseRights, bool IsDirectory, intptr_t Version, TAutoSwitch Utf,
    TChmodSessionAction *Action)
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
        TRights Rights = TRights(BaseRights);
        Rights |= Properties->Rights.GetNumberSet();
        Rights &= static_cast<uint16_t>(~Properties->Rights.GetNumberUnset());
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
    uint8_t Result = FData[FPosition];
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
    uint32_t Result = PeekCardinal();
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
    uint32_t Result = (FData[FPosition] << 8) + FData[FPosition + 1];
    DataConsumed(2);
    return Result;
  }

  bool CanGetSmallCardinal() const
  {
    return (GetRemainingLength() >= 2);
  }

  int64_t GetInt64() const
  {
    int64_t Hi = GetCardinal();
    int64_t Lo = GetCardinal();
    return (Hi << 32) + Lo;
  }

  RawByteString GetRawByteString() const
  {
    RawByteString Result;
    uint32_t Len = GetCardinal();
    Need(Len);
    // cannot happen anyway as Need() would raise exception
    DebugAssert(Len < SFTP_MAX_PACKET_LEN);
    Result.SetLength(Len);
    memmove(ToPtr(Result.c_str()), FData + FPosition, Len);
    DataConsumed(Len);
    return Result;
  }

  bool CanGetString(uint32_t &Size) const
  {
    bool Result = CanGetCardinal();
    if (Result)
    {
      uint32_t Len = PeekCardinal();
      Size = (sizeof(Len) + Len);
      Result = (Size <= GetRemainingLength());
    }
    return Result;
  }

  // For reading strings that are character strings (not byte strings as
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

  void GetFile(TRemoteFile *AFile, intptr_t Version, TDSTMode DSTMode, TAutoSwitch &Utf, bool SignedTS, bool Complete)
  {
    DebugAssert(AFile);
    SSH_FILEXFER_ATTR_TYPES Flags;
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
    Flags = GetCardinal();
    if (Version >= 4)
    {
      uint8_t FXType = GetByte();
      // -:regular, D:directory, L:symlink, S:special, U:unknown
      // O:socket, C:char device, B:block device, F:fifo

      // SSH-2.0-cryptlib returns file type 0 in response to SSH_FXP_LSTAT,
      // handle this undefined value as "unknown"
      static wchar_t *Types = const_cast<wchar_t *>(L"U-DLSUOCBF");
      if (FXType > static_cast<uint8_t>(nb::StrLength(Types)))
      {
        throw Exception(FMTLOAD(SFTP_UNKNOWN_FILE_TYPE, ToInt(FXType)));
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
      GetInt64(); // skip
    }
    // SSH-2.0-3.2.0 F-SECURE SSH - Process Software MultiNet
    // sets SSH_FILEXFER_ATTR_UIDGID for v4, but does not include the UID/GUID
    if ((Flags & SSH_FILEXFER_ATTR_UIDGID) && (Version < 4))
    {
      AFile->GetFileOwner().SetID(GetCardinal());
      AFile->GetFileGroup().SetID(GetCardinal());
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
            ToInt64(GetCardinal()) :
            ToInt64(GetCardinal()),
          DSTMode));
        AFile->SetModification(::UnixToDateTime(
          SignedTS ?
            ToInt64(GetCardinal()) :
            ToInt64(GetCardinal()),
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
          GetCardinal(); // skip access time subseconds
        }
      }
      if (Flags & SSH_FILEXFER_ATTR_CREATETIME)
      {
        GetInt64(); // skip create time
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip create time subseconds
        }
      }
      if (Flags & SSH_FILEXFER_ATTR_MODIFYTIME)
      {
        AFile->SetModification(::UnixToDateTime(GetInt64(), DSTMode));
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip modification time subseconds
        }
      }
      // SFTP-6
      if (Flags & SSH_FILEXFER_ATTR_CTIME)
      {
        GetInt64(); // skip attribute modification time
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip attribute modification time subseconds
        }
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_ACL)
    {
      GetRawByteString();
    }

    if (Flags & SSH_FILEXFER_ATTR_BITS)
    {
      // while SSH_FILEXFER_ATTR_BITS is defined for SFTP5 only, vandyke 2.3.3 sets it
      // for SFTP4 as well
      SSH_FILEXFER_ATTR_TYPES Bits = GetCardinal();
      if (Version >= 6)
      {
        uint32_t BitsValid = GetCardinal();
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
      GetByte();
    }
    if (Flags & SSH_FILEXFER_ATTR_MIME_TYPE)
    {
      GetAnsiString();
    }
    if (Flags & SSH_FILEXFER_ATTR_LINK_COUNT)
    {
      GetCardinal();
    }
    if (Flags & SSH_FILEXFER_ATTR_UNTRANSLATED_NAME)
    {
      GetPathString(Utf);
    }

    if ((Version < 4) && (GetType() != SSH_FXP_ATTRS))
    {
      try
      {
        // update permissions and user/group name
        // modification time and filename is ignored
        AFile->SetListingStr(ListingStr);
      }
      catch (...)
      {
        // ignore any error while parsing listing line,
        // SFTP specification do not recommend to parse it
        ParsingFailed = true;
      }
    }

    if (GetType() == SSH_FXP_ATTRS || Version >= 4 || ParsingFailed)
    {
      wchar_t Type = FILETYPE_DEFAULT;
      if (FLAGSET(Flags, SSH_FILEXFER_ATTR_PERMISSIONS))
      {
        AFile->GetRights()->SetNumber(static_cast<uint16_t>(Permissions & TRights::rfAllSpecials));
        if (FLAGSET(Permissions, TRights::rfDirectory))
        {
          Type = FILETYPE_DIRECTORY;
        }
      }

      if (Version < 4)
      {
        AFile->SetType(Type);
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_EXTENDED)
    {
      uint32_t ExtendedCount = GetCardinal();
      for (uint32_t Index = 0; Index < ExtendedCount; ++Index)
      {
        GetRawByteString(); // skip extended_type
        GetRawByteString(); // skip extended_data
      }
    }

    if (Complete)
    {
      AFile->Complete();
    }
  }

  uint8_t *GetNextData(uintptr_t Size = 0)
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

  void DataUpdated(uintptr_t ALength)
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

  void LoadFromFile(const UnicodeString AFileName)
  {
    std::unique_ptr<TStringList> DumpLines(new TStringList());
    RawByteString Dump;
    try__finally
    {
      DumpLines->LoadFromFile(AFileName);
      Dump = RawByteString(AnsiString(DumpLines->GetText()));
    }
    __finally__removed
    ({
      delete DumpLines;
    })

    SetCapacity(20 * 1024);
    uint8_t Byte[3];
    ClearArray(Byte);
    intptr_t Index = 1;
    uintptr_t Length = 0;
    while (Index < Dump.Length())
    {
      char C = Dump[Index];
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
          ClearArray(Byte);
        }
      }
      ++Index;
    }
    DataUpdated(Length);
  }

  UnicodeString Dump() const
  {
    UnicodeString Result;
    for (uintptr_t Index = 0; Index < GetLength(); ++Index)
    {
      Result += ByteToHex(GetData()[Index]) + L",";
      if (((Index + 1) % 25) == 0)
      {
        Result += L"\n";
      }
    }
    return Result;
  }

  TSFTPPacket &operator=(const TSFTPPacket &Source)
  {
    SetCapacity(0);
    Add(Source.GetData(), Source.GetLength());
    DataUpdated(Source.GetLength());
    FPosition = Source.FPosition;
    FReservedBy = Source.FReservedBy;
    FCodePage = Source.FCodePage;
    return *this;
  }

  __property unsigned int Length = { read = FLength };
  __property unsigned int RemainingLength = { read = GetRemainingLength };
  __property unsigned char * Data = { read = FData };
  __property unsigned char * SendData = { read = GetSendData };
  __property unsigned int SendLength = { read = GetSendLength };
  __property unsigned int Capacity = { read = FCapacity, write = SetCapacity };
  __property unsigned char Type = { read = FType };
  __property unsigned char RequestType = { read = GetRequestType };
  __property unsigned int MessageNumber = { read = FMessageNumber, write = FMessageNumber };
  __property TSFTPFileSystem * ReservedBy = { read = FReservedBy, write = FReservedBy };
  __property UnicodeString TypeName = { read = GetTypeName };

  uintptr_t GetLength() const { return FLength; }
  uint8_t *GetData() const { return FData; }
  uintptr_t GetCapacity() const { return FCapacity; }
  SSH_FXP_TYPES GetType() const { return FType; }
  uintptr_t GetMessageNumber() const { return ToUIntPtr(FMessageNumber); }
  void SetMessageNumber(uint32_t Value) { FMessageNumber = Value; }
  TSFTPFileSystem *GetReservedBy() const { return FReservedBy; }
  void SetReservedBy(TSFTPFileSystem *Value) { FReservedBy = Value; }

private:
  uint8_t *FData;
  uintptr_t FLength;
  uintptr_t FCapacity;
  mutable uintptr_t FPosition;
  SSH_FXP_TYPES FType;
  uint32_t FMessageNumber;
  TSFTPFileSystem *FReservedBy;

  static uint32_t FMessageCounter;
  static const intptr_t FSendPrefixLen = 4;
  uintptr_t FCodePage;

  void Init(uintptr_t CodePage)
  {
    FData = nullptr;
    FCapacity = 0;
    FLength = 0;
    FPosition = 0;
    FMessageNumber = SFTPNoMessageNumber;
    FType = static_cast<SSH_FXP_TYPES>(-1);
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

  void Add(const void *AData, uintptr_t ALength)
  {
    if (GetLength() + ALength > GetCapacity())
    {
      SetCapacity(GetLength() + ALength + SFTP_PACKET_ALLOC_DELTA);
    }
    memmove(FData + GetLength(), AData, ALength);
    FLength += ALength;
  }

  void SetCapacity(uintptr_t ACapacity)
  {
    if (ACapacity != GetCapacity())
    {
      FCapacity = ACapacity;
      if (FCapacity > 0)
      {
        uint8_t *NData = nb::calloc<uint8_t *>(1, FCapacity + FSendPrefixLen);
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
#define TYPE_CASE(TYPE) case TYPE: return MB_TEXT(#TYPE)
    switch (GetType())
    {
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
      return FORMAT("Unknown message (%d)", ToInt(GetType()));
    }
  }

  uint8_t *GetSendData() const
  {
    uint8_t *Result = FData - FSendPrefixLen;
    // this is not strictly const-object operation
    PUT_32BIT(Result, GetLength());
    return Result;
  }

  uintptr_t GetSendLength() const
  {
    return FSendPrefixLen + GetLength();
  }

  uintptr_t GetRemainingLength() const
  {
    return GetLength() - FPosition;
  }

private:
  void Need(uintptr_t Size) const
  {
    if (Size > GetRemainingLength())
    {
      throw Exception(FMTLOAD(SFTP_PACKET_ERROR, ToInt(FPosition), ToInt(Size), ToInt(FLength)));
    }
  }

  uint32_t PeekCardinal() const
  {
    Need(sizeof(uint32_t));
    uint8_t *cp = FData + FPosition;
    uint32_t Result = GET_32BIT(cp);
    return Result;
  }

  UnicodeString GetUtfString(TAutoSwitch &Utf) const
  {
    DebugAssert(Utf != asOff);
    UnicodeString Result;
    RawByteString S = GetRawByteString();

    if (Utf == asAuto)
    {
      nb::TEncodeType EncodeType = DetectUTF8Encoding(S);
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

class TSFTPQueuePacket : public TSFTPPacket
{
  NB_DISABLE_COPY(TSFTPQueuePacket)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSFTPQueuePacket); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPQueuePacket) || TSFTPPacket::is(Kind); }
public:
  explicit TSFTPQueuePacket(uintptr_t CodePage) :
    TSFTPPacket(OBJECT_CLASS_TSFTPQueuePacket, CodePage),
    Token(nullptr)
  {
  }

  void *Token;
};
//---------------------------------------------------------------------------
uint32_t TSFTPPacket::FMessageCounter = 0;
//---------------------------------------------------------------------------
class TSFTPQueue : public TObject
{
  NB_DISABLE_COPY(TSFTPQueue)
public:
  static inline bool classof(const TObject *Obj) { return Obj->is(OBJECT_CLASS_TSFTPQueue); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSFTPQueue) || TObject::is(Kind); }
public:
  explicit __fastcall TSFTPQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) :
    TObject(OBJECT_CLASS_TSFTPQueue),
    FRequests(new TList()),
    FResponses(new TList()),
    FFileSystem(AFileSystem),
    FCodePage(CodePage)
  {
    DebugAssert(FFileSystem);
  }

  virtual __fastcall ~TSFTPQueue()
  {
    DebugAssert(FResponses->GetCount() == FRequests->GetCount());
    for (intptr_t Index = 0; Index < FRequests->GetCount(); ++Index)
    {
      TSFTPQueuePacket *Request = FRequests->GetAs<TSFTPQueuePacket>(Index);
      DebugAssert(Request);
      SAFE_DESTROY(Request);

      TSFTPPacket *Response = FResponses->GetAs<TSFTPPacket>(Index);
      DebugAssert(Response);
      SAFE_DESTROY(Response);
    }
    SAFE_DESTROY(FRequests);
    SAFE_DESTROY(FResponses);
  }

  bool __fastcall Init()
  {
    return SendRequests();
  }

  virtual void __fastcall Dispose(SSH_FXP_TYPES ExpectedType, SSH_FX_TYPES AllowStatus)
  {
    DebugAssert(FFileSystem->FTerminal->GetActive());

    while (FRequests->GetCount())
    {
      DebugAssert(FResponses->GetCount());

      std::unique_ptr<TSFTPQueuePacket> Request(FRequests->GetAs<TSFTPQueuePacket>(0));
      std::unique_ptr<TSFTPPacket> Response(FResponses->GetAs<TSFTPPacket>(0));

      // Particularly when ExpectedType >= 0, the ReceiveResponse may throw, and we have to remove the packets from queue
      FRequests->Delete(0);
      FResponses->Delete(0);

      try
      {
        ReceiveResponse(Request.get(), Response.get(), ExpectedType, AllowStatus);
      }
      catch (Exception &E)
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

  void __fastcall DisposeSafe(SSH_FXP_TYPES ExpectedType = -1, SSH_FX_TYPES AllowStatus = -1)
  {
    if (FFileSystem->FTerminal->GetActive())
    {
      Dispose(ExpectedType, AllowStatus);
    }
  }

  bool __fastcall ReceivePacket(TSFTPPacket *Packet,
    SSH_FXP_TYPES ExpectedType = -1, SSH_FX_TYPES AllowStatus = -1, void **Token = nullptr, bool TryOnly = false)
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
        FRequests->Insert(0, Request.get());
        Request.release();
        FResponses->Insert(0, Response.get());
        Response.release();
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
    ({
      delete Request;
      delete Response;
    })

    return Result;
  }

  bool __fastcall Next(SSH_FXP_TYPES ExpectedType = -1, SSH_FX_TYPES AllowStatus = -1)
  {
    return ReceivePacket(nullptr, ExpectedType, AllowStatus);
  }

protected:
  TList *FRequests;
  TList *FResponses;
  TSFTPFileSystem *FFileSystem;
  uintptr_t FCodePage;

  virtual bool __fastcall InitRequest(TSFTPQueuePacket *Request) = 0;

  virtual bool __fastcall End(TSFTPPacket *Response) = 0;

  virtual void __fastcall SendPacket(TSFTPQueuePacket *Packet)
  {
    FFileSystem->SendPacket(Packet);
  }

  virtual void __fastcall ReceiveResponse(
    const TSFTPPacket *Packet, TSFTPPacket *Response, SSH_FXP_TYPES ExpectedType = -1,
    SSH_FX_TYPES AllowStatus = -1, bool TryOnly = false)
  {
    FFileSystem->ReceiveResponse(Packet, Response, ExpectedType, AllowStatus, TryOnly);
  }

  // sends as many requests as allowed by implementation
  virtual bool SendRequests() = 0;

  virtual bool __fastcall SendRequest()
  {
    std::unique_ptr<TSFTPQueuePacket> Request(new TSFTPQueuePacket(FCodePage));
    try__catch
    {
      if (!InitRequest(Request.get()))
      {
        Request.reset();
      }
    }
    catch__removed 
    ({
      delete Request;
      throw;
    })

    if (Request.get() != nullptr)
    {
      TSFTPPacket *Response = new TSFTPPacket(FCodePage);
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
//---------------------------------------------------------------------------
class TSFTPFixedLenQueue : public TSFTPQueue
{
public:
  explicit __fastcall TSFTPFixedLenQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) :
    TSFTPQueue(AFileSystem, CodePage),
    FMissedRequests(0)
  {
  }

  virtual __fastcall ~TSFTPFixedLenQueue()
  {
  }

  bool Init(intptr_t QueueLen)
  {
    FMissedRequests = QueueLen - 1;
    return TSFTPQueue::Init();
  }

protected:
  intptr_t FMissedRequests;

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
//---------------------------------------------------------------------------
class TSFTPAsynchronousQueue : public TSFTPQueue
{
public:
  explicit __fastcall TSFTPAsynchronousQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) : TSFTPQueue(AFileSystem, CodePage)
  {
    FFileSystem->FSecureShell->RegisterReceiveHandler(nb::bind(&TSFTPAsynchronousQueue::ReceiveHandler, this));
    FReceiveHandlerRegistered = true;
  }

  virtual __fastcall ~TSFTPAsynchronousQueue()
  {
    UnregisterReceiveHandler();
  }

  virtual void __fastcall Dispose(SSH_FXP_TYPES ExpectedType = -1, SSH_FX_TYPES AllowStatus = -1) override
  {
    // we do not want to receive asynchronous notifications anymore,
    // while waiting synchronously for pending responses
    UnregisterReceiveHandler();
    TSFTPQueue::Dispose(ExpectedType, AllowStatus);
  }

  bool __fastcall Continue()
  {
    return SendRequest();
  }

protected:

  // event handler for incoming data
  void __fastcall ReceiveHandler(TObject * /*Sender*/)
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
    catch (Exception &E)  // prevent crash when server unexpectedly closes connection
    {
      DebugUsedParam(E);
      DEBUG_PRINTF("ReceiveHandler: %s\n", E.Message);
    }
  }

  virtual bool __fastcall ReceivePacketAsynchronously() = 0;

  // sends as many requests as allowed by implementation
  virtual bool __fastcall SendRequests() override
  {
    // noop
    return true;
  }

  void __fastcall UnregisterReceiveHandler()
  {
    if (FReceiveHandlerRegistered)
    {
      FReceiveHandlerRegistered = false;
      FFileSystem->FSecureShell->UnregisterReceiveHandler(nb::bind(&TSFTPAsynchronousQueue::ReceiveHandler, this));
    }
  }

private:
  bool FReceiveHandlerRegistered;
};
//---------------------------------------------------------------------------
class TSFTPDownloadQueue : public TSFTPFixedLenQueue
{
  NB_DISABLE_COPY(TSFTPDownloadQueue)
public:
  explicit TSFTPDownloadQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) :
    TSFTPFixedLenQueue(AFileSystem, CodePage),
    OperationProgress(nullptr),
    FTransferred(0)
  {
  }

  virtual __fastcall ~TSFTPDownloadQueue()
  {
  }

  bool __fastcall Init(intptr_t QueueLen, RawByteString AHandle, int64_t ATransferred,
    TFileOperationProgressType *AOperationProgress)
  {
    FHandle = AHandle;
    FTransferred = ATransferred;
    OperationProgress = AOperationProgress;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  void __fastcall InitFillGapRequest(int64_t Offset, uint32_t MissingLen,
    TSFTPPacket *Packet)
  {
    InitRequest(Packet, Offset, MissingLen);
  }

  bool __fastcall ReceivePacket(TSFTPPacket *Packet, uintptr_t &BlockSize)
  {
    void *Token;
    bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_DATA, asEOF, &Token);
    BlockSize = reinterpret_cast<uintptr_t>(Token);
    return Result;
  }

protected:
  virtual bool __fastcall InitRequest(TSFTPQueuePacket *Request) override
  {
    uint32_t BlockSize = FFileSystem->DownloadBlockSize(OperationProgress);
    InitRequest(Request, FTransferred, BlockSize);
    Request->Token = ToPtr(BlockSize);
    FTransferred += BlockSize;
    return true;
  }

  void __fastcall InitRequest(TSFTPPacket *Request, int64_t Offset,
    uint32_t Size) const
  {
    Request->ChangeType(SSH_FXP_READ);
    Request->AddString(FHandle);
    Request->AddInt64(Offset);
    Request->AddCardinal(Size);
  }

  virtual bool __fastcall End(TSFTPPacket *Response) override
  {
    return (Response->GetType() != SSH_FXP_DATA);
  }

private:
  TFileOperationProgressType *OperationProgress;
  int64_t FTransferred;
  RawByteString FHandle;
};
//---------------------------------------------------------------------------
class TSFTPUploadQueue : public TSFTPAsynchronousQueue
{
  NB_DISABLE_COPY(TSFTPUploadQueue)
public:
  explicit TSFTPUploadQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) :
    TSFTPAsynchronousQueue(AFileSystem, CodePage),
    FStream(nullptr),
    FTerminal(nullptr),
    OperationProgress(nullptr),
    FLastBlockSize(0),
    FEnd(false),
    FTransferred(0),
    FConvertToken(false),
    FConvertParams(0)
  {
  }

  virtual __fastcall ~TSFTPUploadQueue()
  {
    SAFE_DESTROY(FStream);
  }

  bool __fastcall Init(const UnicodeString AFileName,
    HANDLE AFile, TFileOperationProgressType *AOperationProgress,
    RawByteString AHandle, int64_t ATransferred,
    intptr_t ConvertParams)
  {
    FFileName = AFileName;
    FStream = new TSafeHandleStream(AFile);
    OperationProgress = AOperationProgress;
    FHandle = AHandle;
    FTransferred = ATransferred;
    FConvertParams = ConvertParams;

    return TSFTPAsynchronousQueue::Init();
  }

  void __fastcall DisposeSafeWithErrorHandling()
  {
    DisposeSafe(SSH_FXP_STATUS);
  }

protected:
  virtual bool __fastcall InitRequest(TSFTPQueuePacket *Request) override
  {
    FTerminal = FFileSystem->FTerminal;
    // Buffer for one block of data
    TFileBuffer BlockBuf;

    intptr_t BlockSize = GetBlockSize();
    bool Result = (BlockSize > 0);

    if (Result)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(READ_ERROR, FFileName), "",
      [&]()
      {
        BlockBuf.LoadStream(FStream, BlockSize, false);
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(READ_ERROR, (FFileName)));

      FEnd = (BlockBuf.GetSize() == 0);
      Result = !FEnd;
      if (Result)
      {
        OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

        // We do ASCII transfer: convert EOL of current block
        if (OperationProgress->GetAsciiTransfer())
        {
          int64_t PrevBufSize = BlockBuf.GetSize();
          BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
            FFileSystem->GetEOL(), FConvertParams, FConvertToken);
          // update transfer size with difference raised from EOL conversion
          OperationProgress->ChangeTransferSize(OperationProgress->GetTransferSize() -
            PrevBufSize + BlockBuf.GetSize());
        }

        if (FFileSystem->FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
        {
          FFileSystem->FTerminal->LogEvent(FORMAT("Write request offset: %d, len: %d",
            int(FTransferred), int(BlockBuf.GetSize())));
        }

        Request->ChangeType(SSH_FXP_WRITE);
        Request->AddString(FHandle);
        Request->AddInt64(FTransferred);
        Request->AddData(BlockBuf.GetData(), ToUInt32(BlockBuf.GetSize()));
        FLastBlockSize = ToUInt32(BlockBuf.GetSize());

        FTransferred += BlockBuf.GetSize();
      }
    }

    FTerminal = nullptr;
    return Result;
  }

  virtual void __fastcall SendPacket(TSFTPQueuePacket *Packet) override
  {
    TSFTPAsynchronousQueue::SendPacket(Packet);
    OperationProgress->AddTransferred(FLastBlockSize);
  }

  virtual void __fastcall ReceiveResponse(
    const TSFTPPacket *Packet, TSFTPPacket *Response, SSH_FXP_TYPES ExpectedType = -1,
    SSH_FX_TYPES AllowStatus = -1, bool TryOnly = false) override
  {
    TSFTPAsynchronousQueue::ReceiveResponse(Packet, Response, ExpectedType, AllowStatus, TryOnly);
    if (Response->GetCapacity() > 0)
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

  virtual bool __fastcall ReceivePacketAsynchronously() override
  {
    // do not read response to close request
    bool Result = (FRequests->GetCount() > 0);
    if (Result)
    {
      // Try only: We cannot read from the socket here as we are already called
      // from TSecureShell::HandleNetworkEvents as it would cause a recursion
      // that would potentially make PuTTY code process the SSH packets in wrong order.
      ReceivePacket(nullptr, SSH_FXP_STATUS, -1, nullptr, true);
    }
    return Result;
  }

  inline intptr_t __fastcall GetBlockSize() const
  {
    return FFileSystem->UploadBlockSize(FHandle, OperationProgress);
  }

  virtual bool __fastcall End(TSFTPPacket * /*Response*/) override
  {
    return FEnd;
  }

private:
  TStream *FStream;
  TTerminal *FTerminal;
  TFileOperationProgressType *OperationProgress;
  UnicodeString FFileName;
  uint32_t FLastBlockSize;
  bool FEnd;
  int64_t FTransferred;
  RawByteString FHandle;
  bool FConvertToken;
  intptr_t FConvertParams;
};
//---------------------------------------------------------------------------
class TSFTPLoadFilesPropertiesQueue : public TSFTPFixedLenQueue
{
  NB_DISABLE_COPY(TSFTPLoadFilesPropertiesQueue)
public:
  explicit TSFTPLoadFilesPropertiesQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) :
    TSFTPFixedLenQueue(AFileSystem, CodePage),
    FFileList(nullptr),
    FIndex(0)
  {
  }

  virtual __fastcall ~TSFTPLoadFilesPropertiesQueue()
  {
  }

  bool __fastcall Init(uintptr_t QueueLen, TStrings *AFileList)
  {
    FFileList = AFileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool __fastcall ReceivePacket(TSFTPPacket *Packet, TRemoteFile *&File)
  {
    void *Token;
    bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_ATTRS, asAll, &Token);
    File = get_as<TRemoteFile>(Token);
    return Result;
  }

protected:
  virtual bool __fastcall InitRequest(TSFTPQueuePacket *Request) override
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile *File = FFileList->GetAs<TRemoteFile>(FIndex);
      ++FIndex;

      bool MissingRights =
        (FFileSystem->FSupport->Loaded &&
         FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS) &&
         File->GetRights()->GetUnknown());
      bool MissingOwnerGroup =
        (FFileSystem->FSecureShell->GetSshImplementation() == sshiBitvise) ||
        ((FFileSystem->FSupport->Loaded &&
          FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP) &&
          !File->GetFileOwner().GetIsSet()) || !File->GetFileGroup().GetIsSet());

      Result = (MissingRights || MissingOwnerGroup);
      if (Result)
      {
        Request->ChangeType(SSH_FXP_LSTAT);
        Request->AddPathString(FFileSystem->LocalCanonify(File->GetFileName()),
          FFileSystem->FUtfStrings);
        if (FFileSystem->FVersion >= 4)
        {
          Request->AddCardinal(
            FLAGMASK(MissingRights, SSH_FILEXFER_ATTR_PERMISSIONS) |
            FLAGMASK(MissingOwnerGroup, SSH_FILEXFER_ATTR_OWNERGROUP));
        }
        Request->Token = File;
      }
    }

    return Result;
  }

  virtual bool __fastcall SendRequest() override
  {
    bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool __fastcall End(TSFTPPacket * /*Response*/) override
  {
    return (FRequests->GetCount() == 0);
  }

private:
  TStrings *FFileList;
  intptr_t FIndex;
};
//---------------------------------------------------------------------------
class TSFTPCalculateFilesChecksumQueue : public TSFTPFixedLenQueue
{
  NB_DISABLE_COPY(TSFTPCalculateFilesChecksumQueue)
public:
  explicit TSFTPCalculateFilesChecksumQueue(TSFTPFileSystem *AFileSystem, uintptr_t CodePage) :
    TSFTPFixedLenQueue(AFileSystem, CodePage),
    FFileList(nullptr),
    FIndex(0)
  {
  }

  virtual __fastcall ~TSFTPCalculateFilesChecksumQueue()
  {
  }

  bool __fastcall Init(intptr_t QueueLen, UnicodeString Alg, TStrings *AFileList)
  {
    FAlg = Alg;
    FFileList = AFileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool __fastcall ReceivePacket(TSFTPPacket *Packet, TRemoteFile *&File)
  {
    void *Token = nullptr;
    bool Result;
    try__finally
    {
      SCOPE_EXIT
      {
        File = get_as<TRemoteFile>(Token);
      };
      Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_EXTENDED_REPLY, asNo, &Token);
    }
    __finally__removed
    ({
      File = static_cast<TRemoteFile *>(Token);
    })
    return Result;
  }

protected:
  virtual bool __fastcall InitRequest(TSFTPQueuePacket *Request) override
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile *File = FFileList->GetAs<TRemoteFile>(FIndex);
      DebugAssert(File != nullptr);
      ++FIndex;
      if (!File)
        continue;

      Result = !File->GetIsDirectory();
      if (Result)
      {
        DebugAssert(!File->GetIsParentDirectory() && !File->GetIsThisDirectory());

        Request->ChangeType(SSH_FXP_EXTENDED);
        Request->AddString(SFTP_EXT_CHECK_FILE_NAME);
        Request->AddPathString(FFileSystem->LocalCanonify(File->GetFullFileName()),
          FFileSystem->FUtfStrings);
        Request->AddString(FAlg);
        Request->AddInt64(0); // offset
        Request->AddInt64(0); // length (0 = till end)
        Request->AddCardinal(0); // block size (0 = no blocks or "one block")

        Request->Token = File;
      }
    }

    return Result;
  }

  virtual bool __fastcall SendRequest() override
  {
    bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool __fastcall End(TSFTPPacket * /*Response*/) override
  {
    return (FRequests->GetCount() == 0);
  }

private:
  UnicodeString FAlg;
  TStrings *FFileList;
  intptr_t FIndex;
};
//---------------------------------------------------------------------------
__removed #pragma warn .inl
//---------------------------------------------------------------------------
class TSFTPBusy : public TObject
{
  NB_DISABLE_COPY(TSFTPBusy)
public:
  explicit TSFTPBusy(TSFTPFileSystem *FileSystem) :
    FFileSystem(FileSystem)
  {
    DebugAssert(FFileSystem != nullptr);
    FFileSystem->BusyStart();
  }

  ~TSFTPBusy()
  {
    FFileSystem->BusyEnd();
  }

private:
  TSFTPFileSystem *FFileSystem;
};
//===========================================================================
#if 0
// moved to FileSystems.h
struct TOpenRemoteFileParams
{
  UnicodeString FileName;
  UnicodeString RemoteFileName;
  TFileOperationProgressType *OperationProgress;
  const TCopyParamType *CopyParam;
  int Params;
  bool Resume;
  bool Resuming;
  TSFTPOverwriteMode OverwriteMode;
  __int64 DestFileSize; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams *FileParams;
  bool Confirmed;
};
#endif // #if 0
//===========================================================================
__fastcall TSFTPFileSystem::TSFTPFileSystem(TTerminal *ATerminal) :
  TCustomFileSystem(OBJECT_CLASS_TSFTPFileSystem, ATerminal),
  FSecureShell(nullptr),
  FFileSystemInfoValid(false),
  FVersion(0),
  FPacketReservations(nullptr),
  FPreviousLoggedPacket(0),
  FNotLoggedPackets(0),
  FBusy(0),
  FBusyToken(nullptr),
  FAvoidBusy(false),
  FExtensions(nullptr),
  FSupport(nullptr),
  FUtfStrings(asAuto),
  FUtfDisablingAnnounced(false),
  FSignedTS(false),
  FFixedPaths(nullptr),
  FMaxPacketSize(0),
  FSupportsStatVfsV2(false),
  FSupportsHardlink(false)
{
  FCodePage = GetSessionData()->GetCodePageAsNumber();
}

void TSFTPFileSystem::Init(void *Data)
{
  FSecureShell = get_as<TSecureShell>(Data);
  DebugAssert(FSecureShell);
  FFileSystemInfoValid = false;
  FVersion = NPOS;
  FPacketReservations = new TList();
  FPacketNumbers.clear();
  FPreviousLoggedPacket = 0;
  FNotLoggedPackets = 0;
  FBusy = 0;
  FAvoidBusy = false;
  FUtfStrings = asOff;
  FUtfDisablingAnnounced = true;
  FSignedTS = false;
  FSupport = new TSFTPSupport();
  FExtensions = new TStringList();
  FFixedPaths = nullptr;
  FFileSystemInfoValid = false;

  FChecksumAlgs.reset(new TStringList());
  FChecksumSftpAlgs.reset(new TStringList());
  // List as defined by draft-ietf-secsh-filexfer-extensions-00
  // MD5 moved to the back
  RegisterChecksumAlg(Sha1ChecksumAlg, L"sha1");
  RegisterChecksumAlg(Sha224ChecksumAlg, L"sha224");
  RegisterChecksumAlg(Sha256ChecksumAlg, L"sha256");
  RegisterChecksumAlg(Sha384ChecksumAlg, L"sha384");
  RegisterChecksumAlg(Sha512ChecksumAlg, L"sha512");
  RegisterChecksumAlg(Md5ChecksumAlg, L"md5");
  RegisterChecksumAlg(Crc32ChecksumAlg, L"crc32");
}
//---------------------------------------------------------------------------
__fastcall TSFTPFileSystem::~TSFTPFileSystem()
{
  SAFE_DESTROY(FSupport);
  ResetConnection();
  SAFE_DESTROY(FPacketReservations);
  SAFE_DESTROY(FExtensions);
  SAFE_DESTROY(FFixedPaths);
  SAFE_DESTROY(FSecureShell);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Open()
{
  // this is used for reconnects only
  ResetConnection();
  FSecureShell->Open();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Close()
{
  FSecureShell->Close();
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::GetActive() const
{
  return FSecureShell->GetActive();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CollectUsage()
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
  __removed FTerminal->Configuration->Usage->Inc(VersionCounter);
}
//---------------------------------------------------------------------------
const TSessionInfo & __fastcall TSFTPFileSystem::GetSessionInfo() const
{
  return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & __fastcall TSFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  if (!FFileSystemInfoValid)
  {
    FFileSystemInfo.AdditionalInfo.Clear();

    if (!IsCapable(fcRename))
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(FS_RENAME_NOT_SUPPORTED) + L"\r\n\r\n";
    }

    if (FExtensions->GetCount() > 0)
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_EXTENSION_INFO) + L"\r\n";
      for (intptr_t Index = 0; Index < FExtensions->GetCount(); ++Index)
      {
        UnicodeString Name = FExtensions->GetName(Index);
        UnicodeString Value = FExtensions->GetValue(Name);
        UnicodeString Line;
        if (Value.IsEmpty())
        {
          Line = Name;
        }
        else
        {
          Line = FORMAT("%s=%s", Name, DisplayableStr(Value));
        }
        FFileSystemInfo.AdditionalInfo += FORMAT("  %s\r\n", Line);
      }
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
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::TemporaryTransferFile(const UnicodeString AFileName)
{
  return ::SameText(base::UnixExtractFileExt(AFileName), PARTIAL_EXT);
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::GetStoredCredentialsTried() const
{
  return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::RemoteGetUserName() const
{
  return FSecureShell->ShellGetUserName();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Idle()
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
      Packet.AddPathString(ROOTDIRECTORY, FUtfStrings);
      SendPacketAndReceiveResponse(&Packet, &Packet);
    }
    else
    {
      FSecureShell->KeepAlive();
    }
  }

  FSecureShell->Idle();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ResetConnection()
{
  // there must be no valid packet reservation at the end
  for (intptr_t Index = 0; Index < FPacketReservations->GetCount(); ++Index)
  {
    DebugAssert(FPacketReservations->GetItem(Index) == nullptr);
    TSFTPPacket *Item = FPacketReservations->GetAs<TSFTPPacket>(Index);
    SAFE_DESTROY(Item);
  }
  FPacketReservations->Clear();
  FPacketNumbers.clear();
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::IsCapable(intptr_t Capability) const
{
  DebugAssert(FTerminal);
  switch (Capability)
  {
    case fcAnyCommand:
    case fcShellAnyCommand:
      return false;

    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
    case fcPreservingTimestampUpload:
    case fcSecondaryShell:
    case fcRemoveCtrlZUpload:
    case fcRemoveBOMUpload:
    case fcMoveToQueue:
    case fcPreservingTimestampDirs:
    case fcResumeSupport:
    case fsSkipTransfer:
    case fsParallelTransfers:
      return true;

    case fcRename:
    case fcRemoteMove:
      return (FVersion >= 2);

    case fcSymbolicLink:
    case fcResolveSymlink:
      return (FVersion >= 3);

    case fcModeChanging:
    case fcModeChangingUpload:
      return !FSupport->Loaded ||
        FLAGSET(FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS);

    case fcGroupOwnerChangingByID:
      return (FVersion <= 3);

    case fcOwnerChanging:
    case fcGroupChanging:
      return
        (FVersion <= 3) ||
        ((FVersion >= 4) &&
         (!FSupport->Loaded ||
          FLAGSET(FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP)));

    case fcNativeTextMode:
      return (FVersion >= 4);

    case fcTextMode:
      return (FVersion >= 4) ||
        strcmp(GetEOL(), EOLToStr(FTerminal->GetConfiguration()->GetLocalEOLType())) != 0;

    case fcUserGroupListing:
      return SupportsExtension(SFTP_EXT_OWNER_GROUP);

    case fcLoadingAdditionalProperties:
      // We allow loading properties only, if "supported" extension is supported and
      // the server supports "permissions" and/or "owner/group" attributes
      // (no other attributes are loaded).
      // This is here only because of VShell
      // (it supports owner/group, but does not include them into response to
      // SSH_FXP_READDIR)
      // and Bitwise (the same as VShell, but it does not even bother to provide "supported" extension until 6.21)
      // No other use is known.
      return
        (FSupport->Loaded &&
         ((FSupport->AttributeMask &
           (SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_OWNERGROUP)) != 0)) ||
        (FSecureShell->GetSshImplementation() == sshiBitvise);

    case fcCheckingSpaceAvailable:
      return
        // extension announced in extension list of by
        // SFTP_EXT_SUPPORTED/SFTP_EXT_SUPPORTED2 extension
        // (SFTP version 5 and newer only)
        SupportsExtension(SFTP_EXT_SPACE_AVAILABLE) ||
        // extension announced by proprietary SFTP_EXT_STATVFS extension
        FSupportsStatVfsV2 ||
        // Bitwise (until 6.21) fails to report it's supported extensions.
        (FSecureShell->GetSshImplementation() == sshiBitvise);

    case fcCalculatingChecksum:
      return
        // Specification says that "check-file" should be announced,
        // yet Vandyke VShell (as of 4.0.3) announce "check-file-name"
        // https://forums.vandyke.com/showthread.php?t=11597
        SupportsExtension(SFTP_EXT_CHECK_FILE) ||
        SupportsExtension(SFTP_EXT_CHECK_FILE_NAME) ||
        // see above
        (FSecureShell->GetSshImplementation() == sshiBitvise);

    case fcRemoteCopy:
      return
        SupportsExtension(SFTP_EXT_COPY_FILE) ||
        // see above
        (FSecureShell->GetSshImplementation() == sshiBitvise);

    case fcHardLink:
      return
        (FVersion >= 6) ||
        FSupportsHardlink;

    case fcLocking:
      return false;

    case fcChangePassword:
      return FSecureShell->CanChangePassword();

    default:
      DebugFail();
      return false;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::SupportsExtension(const UnicodeString Extension) const
{
  return FSupport->Loaded && (FSupport->Extensions->IndexOf(Extension) >= 0);
}
//---------------------------------------------------------------------------
inline void __fastcall TSFTPFileSystem::BusyStart()
{
  if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
  {
    FBusyToken = ::BusyStart();
  }
  FBusy++;
  DebugAssert(FBusy < 10);
}
//---------------------------------------------------------------------------
inline void __fastcall TSFTPFileSystem::BusyEnd()
{
  DebugAssert(FBusy > 0);
  FBusy--;
  if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
  {
    ::BusyEnd(FBusyToken);
    FBusyToken = nullptr;
  }
}
//---------------------------------------------------------------------------
uint32_t __fastcall TSFTPFileSystem::TransferBlockSize(uint32_t Overhead,
  TFileOperationProgressType *OperationProgress,
  uint32_t MinPacketSize,
  uint32_t MaxPacketSize) const
{
  const uint32_t minPacketSize = MinPacketSize ? MinPacketSize : 32 * 1024;

  // size + message number + type
  const uint32_t SFTPPacketOverhead = 4 + 4 + 1;
  uint32_t AMinPacketSize = FSecureShell->MinPacketSize();
  (void)AMinPacketSize;
  uint32_t AMaxPacketSize = FSecureShell->MaxPacketSize();
  bool MaxPacketSizeValid = (AMaxPacketSize > 0);
  uint32_t Result = ToUInt32(OperationProgress->CPS());

  if ((MaxPacketSize > 0) &&
    ((MaxPacketSize < AMaxPacketSize) || !MaxPacketSizeValid))
  {
    AMaxPacketSize = MaxPacketSize;
    MaxPacketSizeValid = true;
  }

  if ((FMaxPacketSize > 0) &&
    ((FMaxPacketSize < AMaxPacketSize) || !MaxPacketSizeValid))
  {
    AMaxPacketSize = FMaxPacketSize;
    MaxPacketSizeValid = true;
  }

  if (Result == 0)
  {
    Result = ToUInt32(OperationProgress->StaticBlockSize());
  }

  if (Result < minPacketSize)
  {
    Result = minPacketSize;
  }

  if (MaxPacketSizeValid)
  {
    Overhead += SFTPPacketOverhead;
    if (AMaxPacketSize < Overhead)
    {
      // do not send another request
      // (generally should happen only if upload buffer is full)
      Result = 0;
    }
    else
    {
      AMaxPacketSize -= Overhead;
      if (Result > AMaxPacketSize)
      {
        Result = AMaxPacketSize;
      }
    }
  }

  Result = ToUInt32(OperationProgress->AdjustToCPSLimit(Result));

  return Result;
}
//---------------------------------------------------------------------------
uint32_t __fastcall TSFTPFileSystem::UploadBlockSize(RawByteString Handle,
  TFileOperationProgressType *OperationProgress) const
{
  // handle length + offset + data size
  const uintptr_t UploadPacketOverhead =
    sizeof(uint32_t) + sizeof(int64_t) + sizeof(uint32_t);
  return TransferBlockSize(ToUInt32(UploadPacketOverhead + Handle.Length()), OperationProgress,
      ToUInt32(GetSessionData()->GetSFTPMinPacketSize()),
      ToUInt32(GetSessionData()->GetSFTPMaxPacketSize()));
}
//---------------------------------------------------------------------------
uint32_t __fastcall TSFTPFileSystem::DownloadBlockSize(
  TFileOperationProgressType *OperationProgress) const
{
  uint32_t Result = TransferBlockSize(sizeof(uint32_t), OperationProgress,
      ToUInt32(GetSessionData()->GetSFTPMinPacketSize()),
      ToUInt32(GetSessionData()->GetSFTPMaxPacketSize()));
  if (FSupport->Loaded && (FSupport->MaxReadSize > 0) &&
    (Result > FSupport->MaxReadSize))
  {
    Result = FSupport->MaxReadSize;
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Progress(TFileOperationProgressType *OperationProgress)
{
  FTerminal->Progress(OperationProgress);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SendPacket(const TSFTPPacket *Packet)
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
    SCOPE_EXIT
    {
      this->BusyEnd();
    };
    if (FTerminal->GetLog()->GetLogging())
    {
      if ((FPreviousLoggedPacket != SSH_FXP_READ &&
           FPreviousLoggedPacket != SSH_FXP_WRITE) ||
          (Packet->GetType() != FPreviousLoggedPacket) ||
          (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
      {
        if (FNotLoggedPackets)
        {
          FTerminal->LogEvent(FORMAT("%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
              FNotLoggedPackets));
          FNotLoggedPackets = 0;
        }
        FTerminal->GetLog()->Add(llInput, FORMAT("Type: %s, Size: %d, Number: %d",
          Packet->GetTypeName(), ToInt(Packet->GetLength()), ToInt(Packet->GetMessageNumber())));
        if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2)
        {
          __removed FTerminal->GetLog()->Add(llInput, Packet->Dump());
        }
        FPreviousLoggedPacket = Packet->GetType();
      }
      else
      {
        FNotLoggedPackets++;
      }
    }
    FSecureShell->Send(Packet->GetSendData(), Packet->GetSendLength());
  }
  __finally__removed
  ({
    this->BusyEnd();
  })
}
//---------------------------------------------------------------------------
SSH_FX_TYPES __fastcall TSFTPFileSystem::GotStatusPacket(TSFTPPacket *Packet,
  SSH_FX_TYPES AllowStatus)
{
  SSH_FX_TYPES Code = Packet->GetCardinal();

  static intptr_t Messages[] =
  {
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
    intptr_t Message;
    if (ToUInt32(Code) >= _countof(Messages))
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
      if (Packet->GetRemainingLength() > 0)
      {
        // message is in UTF only since SFTP specification 01 (specification 00
        // is also version 3)
        // (in other words, always use UTF unless server is known to be buggy)
        ServerMessage = Packet->GetString(FUtfStrings);
      }
      // SSH-2.0-Maverick_SSHD and SSH-2.0-CIGNA SFTP Server Ready! omit the language tag
      // and I believe I've seen one more server doing the same.
      if (Packet->GetRemainingLength() > 0)
      {
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
    if (FTerminal->GetLog()->GetLogging())
    {
      FTerminal->GetLog()->Add(llOutput, FORMAT("Status code: %d, Message: %d, Server: %s, Language: %s ",
        ToInt(Code), ToInt(Packet->GetMessageNumber()), ServerMessage, LanguageTag));
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
      int(Code), LanguageTag, ServerMessage);
    if (Code == SSH_FX_FAILURE)
    {
      __removed FTerminal->Configuration->Usage->Inc(L"SftpFailureErrors");
      Error += L"\n\n" + LoadStr(SFTP_STATUS_4);
    }
    FTerminal->TerminalError(nullptr, Error, HelpKeyword);
    return 0;
  }
  if (!FNotLoggedPackets || Code)
  {
    FTerminal->GetLog()->Add(llOutput, FORMAT("Status code: %d", ToInt(Code)));
  }
  return Code;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoveReservation(intptr_t Reservation)
{
  for (intptr_t Index = Reservation + 1; Index < FPacketReservations->GetCount(); ++Index)
  {
    FPacketNumbers[Index - 1] = FPacketNumbers[Index];
  }
  TSFTPPacket *Packet = FPacketReservations->GetAs<TSFTPPacket>(Reservation);
  if (Packet)
  {
    DebugAssert((Packet->GetReservedBy() == nullptr) || (Packet->GetReservedBy() == this));
    Packet->SetReservedBy(nullptr);
  }
  FPacketReservations->Delete(Reservation);
}
//---------------------------------------------------------------------------
intptr_t __fastcall TSFTPFileSystem::PacketLength(uint8_t *LenBuf, SSH_FXP_TYPES ExpectedType) const
{
  intptr_t Length = GET_32BIT(LenBuf);
  if (Length > SFTP_MAX_PACKET_LEN)
  {
    UnicodeString Message = FMTLOAD(SFTP_PACKET_TOO_BIG,
      int(Length), SFTP_MAX_PACKET_LEN);
    if (ExpectedType == SSH_FXP_VERSION)
    {
      RawByteString LenString(reinterpret_cast<char *>(LenBuf), 4);
      Message = FMTLOAD(SFTP_PACKET_TOO_BIG_INIT_EXPLAIN,
        Message, DisplayableStr(LenString));
    }
    FTerminal->FatalError(nullptr, Message, HELP_SFTP_PACKET_TOO_BIG);
  }
  return Length;
}
//---------------------------------------------------------------------------
const TSessionData *TSFTPFileSystem::GetSessionData() const
{
  return FTerminal->GetSessionData();
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::PeekPacket()
{
  uint8_t *Buf = nullptr;
  bool Result = FSecureShell->Peek(Buf, 4);
  if (Result)
  {
    intptr_t Length = PacketLength(Buf, static_cast<SSH_FXP_TYPES>(-1));
    Result = FSecureShell->Peek(Buf, 4 + Length);
  }
  return Result;
}
//---------------------------------------------------------------------------
SSH_FX_TYPES __fastcall TSFTPFileSystem::ReceivePacket(TSFTPPacket *Packet,
  SSH_FXP_TYPES ExpectedType, SSH_FX_TYPES AllowStatus, bool TryOnly)
{
  TSFTPBusy Busy(this);

  SSH_FX_TYPES Result = SSH_FX_OK;
  intptr_t Reservation = FPacketReservations->IndexOf(Packet);

  if ((Reservation < 0) || (Packet->GetCapacity() == 0))
  {
    bool IsReserved;
    do
    {
      IsReserved = false;

      DebugAssert(Packet);

      if (TryOnly && !PeekPacket())
      {
        // Reset packet in case it was filled by previous out-of-order
        // reserved packet
        *Packet = TSFTPPacket(FCodePage);
      }
      else
      {
        uint8_t LenBuf[4];
        FSecureShell->Receive(LenBuf, sizeof(LenBuf));
        intptr_t Length = PacketLength(LenBuf, ExpectedType);
        Packet->SetCapacity(Length);
        FSecureShell->Receive(Packet->GetData(), Length);
        Packet->DataUpdated(Length);

        if (FTerminal->GetLog()->GetLogging())
        {
          if ((FPreviousLoggedPacket != SSH_FXP_READ &&
              FPreviousLoggedPacket != SSH_FXP_WRITE) ||
            (Packet->GetType() != SSH_FXP_STATUS && Packet->GetType() != SSH_FXP_DATA) ||
            (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
          {
            if (FNotLoggedPackets)
            {
              FTerminal->LogEvent(FORMAT("%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
                  FNotLoggedPackets));
              FNotLoggedPackets = 0;
            }
            FTerminal->GetLog()->Add(llOutput, FORMAT("Type: %s, Size: %d, Number: %d",
                Packet->GetTypeName(), ToInt(Packet->GetLength()), ToInt(Packet->GetMessageNumber())));
            if (False && (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2))
            {
              FTerminal->GetLog()->Add(llOutput, Packet->Dump());
            }
          }
          else
          {
            FNotLoggedPackets++;
          }
        }

        if ((Reservation < 0) ||
          Packet->GetMessageNumber() != FPacketNumbers[Reservation])
        {
          for (intptr_t Index = 0; Index < FPacketReservations->GetCount(); ++Index)
          {
            uint32_t MessageNumber = ToUInt32(FPacketNumbers[Index]);
            if (MessageNumber == Packet->GetMessageNumber())
            {
              TSFTPPacket *ReservedPacket = FPacketReservations->GetAs<TSFTPPacket>(Index);
              IsReserved = true;
              if (ReservedPacket)
              {
                FTerminal->LogEvent("Storing reserved response");
                *ReservedPacket = *Packet;
              }
              else
              {
                FTerminal->LogEvent("Discarding reserved response");
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
    // (and it have not worked anyway until recent fix to UnreserveResponse)
    if (Reservation >= 0)
    {
      DebugAssert(Packet->GetMessageNumber() == FPacketNumbers[Reservation]);
      RemoveReservation(Reservation);
    }

    if (ExpectedType != static_cast<SSH_FXP_TYPES>(-1))
    {
      if (Packet->GetType() == SSH_FXP_STATUS)
      {
        if (AllowStatus < 0)
        {
          AllowStatus = (ExpectedType == SSH_FXP_STATUS ? asOK : asNo);
        }
        Result = GotStatusPacket(Packet, AllowStatus);
      }
      else if (ExpectedType != Packet->GetType())
      {
        FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_TYPE, ToInt(Packet->GetType())));
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReserveResponse(const TSFTPPacket *Packet,
  TSFTPPacket *Response)
{
  if (Response != nullptr)
  {
    DebugAssert(FPacketReservations->IndexOf(Response) < 0);
    // mark response as not received yet
    Response->SetCapacity(0);
    Response->SetReservedBy(this);
  }
  FPacketReservations->Add(Response);
  if (static_cast<size_t>(FPacketReservations->GetCount()) >= FPacketNumbers.size())
  {
    FPacketNumbers.resize(FPacketReservations->GetCount() + 10);
  }
  FPacketNumbers[FPacketReservations->GetCount() - 1] = Packet->GetMessageNumber();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::UnreserveResponse(TSFTPPacket *Response)
{
  intptr_t Reservation = FPacketReservations->IndexOf(Response);
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
//---------------------------------------------------------------------------
SSH_FX_TYPES __fastcall TSFTPFileSystem::ReceiveResponse(
  const TSFTPPacket *Packet, TSFTPPacket *AResponse, SSH_FXP_TYPES ExpectedType,
  SSH_FX_TYPES AllowStatus, bool TryOnly)
{
  SSH_FX_TYPES Result;
  uintptr_t MessageNumber = Packet->GetMessageNumber();
  TSFTPPacket *Response = (AResponse ? AResponse : new TSFTPPacket(FCodePage));
  try__finally
  {
    SCOPE_EXIT
    {
      if (!AResponse)
      {
        delete Response;
      }
    };
    Result = ReceivePacket(AResponse, ExpectedType, AllowStatus, TryOnly);
    if (MessageNumber != AResponse->GetMessageNumber())
    {
      FTerminal->FatalError(nullptr, FMTLOAD(SFTP_MESSAGE_NUMBER,
          ToInt(AResponse->GetMessageNumber()),
          ToInt(MessageNumber)));
    }
  }
  __finally__removed
  ({
    if (!Response)
    {
      delete AResponse;
    }
  })
  return Result;
}
//---------------------------------------------------------------------------
SSH_FX_TYPES __fastcall TSFTPFileSystem::SendPacketAndReceiveResponse(const TSFTPPacket *Packet, TSFTPPacket *Response, SSH_FXP_TYPES ExpectedType,
  SSH_FX_TYPES AllowStatus)
{
  TSFTPBusy Busy(this);
  SendPacket(Packet);
  SSH_FX_TYPES Result = ReceiveResponse(Packet, Response, ExpectedType, AllowStatus);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::GetRealPath(const UnicodeString APath)
{
  try
  {
    FTerminal->LogEvent(FORMAT("Getting real path for '%s'",
      APath));

    TSFTPPacket Packet(SSH_FXP_REALPATH, FCodePage);
    Packet.AddPathString(APath, FUtfStrings);

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
    // ignore rest of SSH_FXP_NAME packet

    FTerminal->LogEvent(FORMAT("Real path is '%s'", RealDir));

    return RealDir;
  }
  catch (Exception &E)
  {
    if (FTerminal->GetActive())
    {
      throw ExtException(&E, FMTLOAD(SFTP_REALPATH_ERROR, APath));
    }
    throw;
  }
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::GetRealPath(UnicodeString APath,
  UnicodeString ABaseDir)
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
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::LocalCanonify(UnicodeString APath) const
{
  TODO("improve (handle .. etc.)");
  if (base::UnixIsAbsolutePath(APath) ||
    (!FCurrentDirectory.IsEmpty() && base::UnixSamePath(FCurrentDirectory, APath)))
  {
    return APath;
  }
  return base::AbsolutePath(FCurrentDirectory, APath);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::Canonify(UnicodeString APath)
{
  // inspired by canonify() from PSFTP.C
  UnicodeString Result;
  FTerminal->LogEvent(FORMAT("Canonifying: \"%s\"", APath));
  UnicodeString Path = LocalCanonify(APath);
  bool TryParent = false;
  try
  {
    Result = GetRealPath(Path);
  }
  catch (...)
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
    UnicodeString Path2 = base::UnixExcludeTrailingBackslash(Path);
    UnicodeString Name = base::UnixExtractFileName(Path2);
    if (Name == THISDIRECTORY || Name == PARENTDIRECTORY)
    {
      Result = Path;
    }
    else
    {
      UnicodeString FPath = base::UnixExtractFilePath(Path2);
      try
      {
        Result = GetRealPath(FPath);
        Result = base::UnixIncludeTrailingBackslash(Result) + Name;
      }
      catch (...)
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
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::GetAbsolutePath(UnicodeString APath, bool Local) const
{
  return const_cast<TSFTPFileSystem *>(this)->GetAbsolutePath(APath, Local);
}

UnicodeString TSFTPFileSystem::GetAbsolutePath(UnicodeString APath, bool Local)
{
  if (Local)
  {
    return LocalCanonify(APath);
  }
  return GetRealPath(APath, RemoteGetCurrentDirectory());
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::GetHomeDirectory()
{
  if (FHomeDirectory.IsEmpty())
  {
    FHomeDirectory = GetRealPath(THISDIRECTORY);
  }
  return FHomeDirectory;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::LoadFile(TRemoteFile *AFile, TSFTPPacket *Packet,
  bool Complete)
{
  Packet->GetFile(AFile, FVersion, GetSessionData()->GetDSTMode(),
    FUtfStrings, FSignedTS, Complete);
}
//---------------------------------------------------------------------------
TRemoteFile * __fastcall TSFTPFileSystem::LoadFile(TSFTPPacket *Packet,
  TRemoteFile *ALinkedByFile, const UnicodeString AFileName,
  TRemoteFileList *TempFileList, bool Complete)
{
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(ALinkedByFile));
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
  catch__removed
  ({
    delete File;
    throw;
  })
  return File.release();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSFTPFileSystem::RemoteGetCurrentDirectory() const
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DoStartup()
{
  // do not know yet
  FVersion = -1;
  FFileSystemInfoValid = false;
  TSFTPPacket Packet1(SSH_FXP_INIT, FCodePage);
  intptr_t MaxVersion = GetSessionData()->GetSFTPMaxVersion();
  if (MaxVersion > SFTPMaxVersion)
  {
    MaxVersion = SFTPMaxVersion;
  }
  Packet1.AddCardinal(ToUInt32(MaxVersion));

  try
  {
    SendPacketAndReceiveResponse(&Packet1, &Packet1, SSH_FXP_VERSION);
  }
  catch (Exception &E)
  {
    FTerminal->FatalError(&E, LoadStr(SFTP_INITIALIZE_ERROR), HELP_SFTP_INITIALIZE_ERROR);
  }

  FVersion = Packet1.GetCardinal();
  FTerminal->LogEvent(FORMAT("SFTP version %d negotiated.", FVersion));
  if (FVersion < SFTPMinVersion || FVersion > SFTPMaxVersion)
  {
    FTerminal->FatalError(nullptr, FMTLOAD(SFTP_VERSION_NOT_SUPPORTED,
      FVersion, SFTPMinVersion, SFTPMaxVersion));
  }

  FExtensions->Clear();
  FEOL = "\r\n";
  FSupport->Loaded = false;
  FSupportsStatVfsV2 = false;
  FSupportsHardlink = false;
  SAFE_DESTROY(FFixedPaths);

  if (FVersion >= 3)
  {
    while (Packet1.GetNextData() != nullptr)
    {
      UnicodeString ExtensionName = Packet1.GetAnsiString();
      RawByteString ExtensionData = Packet1.GetRawByteString();
      UnicodeString ExtensionDisplayData = DisplayableStr(ExtensionData);

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
        if (ExtensionName == SFTP_EXT_SUPPORTED)
        {
          while (SupportedStruct.GetNextData() != nullptr)
          {
            FSupport->Extensions->Add(SupportedStruct.GetAnsiString());
          }
        }
        else
        {
          // note that supported-open-block-vector, supported-block-vector,
          // attrib-extension-count and attrib-extension-names fields
          // were added only in rev 08, while "supported2" was defined in rev 07
          FSupport->OpenBlockVector = SupportedStruct.GetSmallCardinal();
          FSupport->BlockVector = SupportedStruct.GetSmallCardinal();
          uint32_t ExtensionCount = SupportedStruct.GetCardinal();
          for (uint32_t Index = 0; Index < ExtensionCount; ++Index)
          {
            FSupport->AttribExtensions->Add(SupportedStruct.GetAnsiString());
          }
          ExtensionCount = SupportedStruct.GetCardinal();
          for (uint32_t Index = 0; Index < ExtensionCount; ++Index)
          {
            FSupport->Extensions->Add(SupportedStruct.GetAnsiString());
          }
        }

        if (FTerminal->GetLog()->GetLogging())
        {
          FTerminal->LogEvent(FORMAT(
            L"Server support information (%s):\n"
             L"  Attribute mask: %x, Attribute bits: %x, Open flags: %x\n"
             L"  Access mask: %x, Open block vector: %x, Block vector: %x, Max read size: %d\n",
            ExtensionName,
             int(FSupport->AttributeMask),
             int(FSupport->AttributeBits),
             int(FSupport->OpenFlags),
             int(FSupport->AccessMask),
             int(FSupport->OpenBlockVector),
             int(FSupport->BlockVector),
             int(FSupport->MaxReadSize)));
          FTerminal->LogEvent(FORMAT("  Attribute extensions (%d)\n", FSupport->AttribExtensions->GetCount()));
          for (intptr_t Index = 0; Index < FSupport->AttribExtensions->GetCount(); ++Index)
          {
            FTerminal->LogEvent(
              FORMAT("    %s", FSupport->AttribExtensions->GetString(Index)));
          }
          FTerminal->LogEvent(FORMAT("  Extensions (%d)\n", FSupport->Extensions->GetCount()));
          for (intptr_t Index = 0; Index < FSupport->Extensions->GetCount(); ++Index)
          {
            FTerminal->LogEvent(
              FORMAT("    %s", FSupport->Extensions->GetString(Index)));
          }
        }
      }
      else if (ExtensionName == SFTP_EXT_VENDOR_ID)
      {
        TSFTPPacket VendorIdStruct(ExtensionData, FCodePage);
        UnicodeString VendorName(VendorIdStruct.GetAnsiString());
        UnicodeString ProductName(VendorIdStruct.GetAnsiString());
        UnicodeString ProductVersion(VendorIdStruct.GetAnsiString());
        int64_t ProductBuildNumber = VendorIdStruct.GetInt64();
        FTerminal->LogEvent(FORMAT("Server software: %s %s (%d) by %s",
            ProductName, ProductVersion, int(ProductBuildNumber), VendorName));
      }
      else if (ExtensionName == SFTP_EXT_FSROOTS)
      {
        FTerminal->LogEvent("File system roots:\n");
        DebugAssert(FFixedPaths == nullptr);
        FFixedPaths = new TStringList();
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
            uint8_t Drive = RootsPacket.GetByte();
            uint8_t MaybeType = RootsPacket.GetByte();
            FTerminal->LogEvent(FORMAT("  %s: (type %d)", static_cast<char>(Drive), ToInt(MaybeType)));
            FFixedPaths->Add(FORMAT("%s:", static_cast<char>(Drive)));
          }
        }
        catch (Exception &E)
        {
          DEBUG_PRINTF("before FTerminal->HandleException");
          FFixedPaths->Clear();
          FTerminal->LogEvent(FORMAT("Failed to decode %s extension",
            SFTP_EXT_FSROOTS));
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
          UnicodeString Versions = VersionsPacket.GetAnsiString();
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
        UnicodeString StatVfsVersion = AnsiToString(ExtensionData);
        if (StatVfsVersion == SFTP_EXT_STATVFS_VALUE_V2)
        {
          FSupportsStatVfsV2 = true;
          FTerminal->LogEvent(FORMAT("Supports %s extension version %s", ExtensionName, ExtensionDisplayData));
        }
        else
        {
          FTerminal->LogEvent(FORMAT("Unsupported %s extension version %s", ExtensionName, ExtensionDisplayData));
        }
      }
      else if (ExtensionName == SFTP_EXT_HARDLINK)
      {
        UnicodeString HardlinkVersion = AnsiToString(ExtensionData);
        if (HardlinkVersion == SFTP_EXT_HARDLINK_VALUE_V1)
        {
          FSupportsHardlink = true;
          FTerminal->LogEvent(FORMAT("Supports %s extension version %s", ExtensionName, ExtensionDisplayData));
        }
        else
        {
          FTerminal->LogEvent(FORMAT("Unsupported %s extension version %s", ExtensionName, ExtensionDisplayData));
        }
      }
      else
      {
        FTerminal->LogEvent(FORMAT("Unknown server extension %s=%s",
          ExtensionName, ExtensionDisplayData));
      }
      FExtensions->SetValue(ExtensionName, ExtensionDisplayData);
    }

    if (SupportsExtension(SFTP_EXT_VENDOR_ID))
    {
      const TConfiguration *Configuration = FTerminal->GetConfiguration();
      TSFTPPacket Packet2(SSH_FXP_EXTENDED, FCodePage);
      Packet2.AddString(RawByteString(SFTP_EXT_VENDOR_ID));
      Packet2.AddString(Configuration->GetCompanyName());
      Packet2.AddString(Configuration->GetProductName());
      Packet2.AddString(Configuration->GetProductVersion());
      Packet2.AddInt64(LOWORD(FTerminal->GetConfiguration()->GetFixedApplicationInfo()->dwFileVersionLS));
      SendPacket(&Packet2);
      // we are not interested in the response, do not wait for it
      ReceiveResponse(&Packet2, &Packet2);
      //ReserveResponse(&Packet, nullptr);
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

  const TSessionInfo &Info = GetSessionInfo();
  switch (GetSessionData()->GetNotUtf())
  {
    case asOff:
      FUtfStrings = asOn;
      FTerminal->LogEvent("We will use UTF-8 strings as configured");
      break;

    case asAuto:
      // Nb, Foxit server does not exist anymore
      if (GetSessionInfo().SshImplementation.Pos(L"Foxit-WAC-Server") == 1)
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

  FMaxPacketSize = ToUInt32(GetSessionData()->GetSFTPMaxPacketSize());
  if (FMaxPacketSize == 0)
  {
    if ((FSecureShell->GetSshImplementation() == sshiOpenSSH) && (FVersion == 3) && !FSupport->Loaded)
    {
      FMaxPacketSize = 4 + (256 * 1024); // len + 256kB payload
      FTerminal->LogEvent(FORMAT("Limiting packet size to OpenSSH sftp-server limit of %d bytes",
        int(FMaxPacketSize)));
    }
    // full string is "1.77 sshlib: Momentum SSH Server",
    // possibly it is sshlib-related
    else if (Info.SshImplementation.Pos(L"Momentum SSH Server") != 0)
    {
      FMaxPacketSize = 4 + (32 * 1024);
      FTerminal->LogEvent(FORMAT("Limiting packet size to Momentum sftp-server limit of %d bytes",
        int(FMaxPacketSize)));
    }
  }
}
//---------------------------------------------------------------------------
char * __fastcall TSFTPFileSystem::GetEOL() const
{
  if (FVersion >= 4)
  {
    DebugAssert(!FEOL.IsEmpty());
    return ToChar(FEOL);
  }
  return EOLToStr(GetSessionData()->GetEOLType());
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::LookupUsersGroups()
{
  DebugAssert(SupportsExtension(SFTP_EXT_OWNER_GROUP));

  TSFTPPacket PacketOwners(SSH_FXP_EXTENDED, FCodePage);
  TSFTPPacket PacketGroups(SSH_FXP_EXTENDED, FCodePage);

  TSFTPPacket *Packets[] = { &PacketOwners, &PacketGroups };
  TRemoteTokenList *Lists[] = { FTerminal->GetUsers(), FTerminal->GetGroups() };
  wchar_t ListTypes[] = { OGQ_LIST_OWNERS, OGQ_LIST_GROUPS };

  for (intptr_t Index = 0; Index < ToIntPtr(_countof(Packets)); ++Index)
  {
    TSFTPPacket *Packet = Packets[Index];
    Packet->AddString(RawByteString(SFTP_EXT_OWNER_GROUP));
    Packet->AddByte(static_cast<uint8_t>(ListTypes[Index]));
    SendPacket(Packet);
    ReserveResponse(Packet, Packet);
  }

  for (intptr_t Index = 0; Index < ToIntPtr(_countof(Packets)); ++Index)
  {
    TSFTPPacket *Packet = Packets[Index];

    ReceiveResponse(Packet, Packet, SSH_FXP_EXTENDED_REPLY, asOpUnsupported);

    if ((Packet->GetType() != SSH_FXP_EXTENDED_REPLY) ||
      (Packet->GetAnsiString() != SFTP_EXT_OWNER_GROUP_REPLY))
    {
      FTerminal->LogEvent(FORMAT("Invalid response to %s", SFTP_EXT_OWNER_GROUP));
    }
    else
    {
      TRemoteTokenList &List = *Lists[Index];
      uint32_t Count = Packet->GetCardinal();

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
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadCurrentDirectory()
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
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::HomeDirectory()
{
  ChangeDirectory(GetHomeDirectory());
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::TryOpenDirectory(const UnicodeString Directory)
{
  FTerminal->LogEvent(FORMAT("Trying to open directory \"%s\".", Directory));
  TRemoteFile *File = nullptr;
  CustomReadFile(Directory, File, SSH_FXP_LSTAT, nullptr, asOpUnsupported);
  if (File == nullptr)
  {
    // File can be NULL only when server does not support SSH_FXP_LSTAT.
    // Fallback to legacy solution, which in turn does not allow entering
    // traverse-only (chmod 110) directories.
    // This is workaround for http://www.ftpshell.com/
    TSFTPPacket Packet(SSH_FXP_OPENDIR, FCodePage);
    Packet.AddPathString(base::UnixExcludeTrailingBackslash(Directory), FUtfStrings);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);
    RawByteString Handle = Packet.GetFileHandle();
    Packet.ChangeType(SSH_FXP_CLOSE);
    Packet.AddString(Handle);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS, asAll);
  }
  else
  {
    SAFE_DESTROY(File);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::AnnounceFileListOperation()
{
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ChangeDirectory(UnicodeString Directory)
{
  UnicodeString Current = !FDirectoryToChangeTo.IsEmpty() ? FDirectoryToChangeTo : FCurrentDirectory;
  UnicodeString Path = GetRealPath(Directory, Current);

  // to verify existence of directory try to open it (SSH_FXP_REALPATH succeeds
  // for invalid paths on some systems, like CygWin)
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FDirectoryToChangeTo = Path;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CachedChangeDirectory(const UnicodeString Directory)
{
  FDirectoryToChangeTo = base::UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadDirectory(TRemoteFileList *FileList)
{
  DebugAssert(FileList && !FileList->GetDirectory().IsEmpty());

  UnicodeString Directory;
  Directory = base::UnixExcludeTrailingBackslash(LocalCanonify(FileList->GetDirectory()));
  FTerminal->LogEvent(FORMAT("Listing directory \"%s\".", Directory));

  // moved before SSH_FXP_OPENDIR, so directory listing does not retain
  // old data (e.g. parent directory) when reading fails
  FileList->Reset();

  TSFTPPacket Packet(SSH_FXP_OPENDIR, FCodePage);
  RawByteString Handle;

  try
  {
    Packet.AddPathString(Directory, FUtfStrings);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

    Handle = Packet.GetFileHandle();
  }
  catch (...)
  {
    if (FTerminal->GetActive())
    {
      FileList->AddFile(new TRemoteParentDirectory(FTerminal));
    }
    throw;
  }

  TSFTPPacket Response(FCodePage);
  try__finally
  {
    SCOPE_EXIT
    {
      if (FTerminal->GetActive())
      {
        Packet.ChangeType(SSH_FXP_CLOSE);
        Packet.AddString(Handle);
        SendPacket(&Packet);
        // we are not interested in the response, do not wait for it
        ReserveResponse(&Packet, nullptr);
      }
    };
    bool isEOF = false;
    intptr_t Total = 0;
    bool HasParentDirectory = false;
    TRemoteFile *File = nullptr;

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

        uint32_t Count = ListingPacket.GetCardinal();

        intptr_t ResolvedLinks = 0;
        for (uint32_t Index = 0; !isEOF && (Index < Count); ++Index)
        {
          File = LoadFile(&ListingPacket, nullptr, L"", FileList);
          if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
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
          FileList->AddFile(File);
          Total++;

          if (Total % 10 == 0)
          {
            FTerminal->DoReadDirectoryProgress(Total, ResolvedLinks, isEOF);
            if (isEOF)
            {
              FTerminal->DoReadDirectoryProgress(-2, 0, isEOF);
            }
          }
        }

        if ((FVersion >= 6) &&
          // As of 7.0.9 the Cerberus SFTP server always sets the end-of-list to true.
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
        isEOF = (GotStatusPacket(&Response, asEOF) == SSH_FX_EOF);
      }
      else
      {
        FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_TYPE, ToInt(Response.GetType())));
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
            SCOPE_EXIT
            {
              FTerminal->SetExceptionOnFail(false);
            };
            File = nullptr;
            FTerminal->ReadFile(
              base::UnixIncludeTrailingBackslash(FileList->GetDirectory()) + PARENTDIRECTORY, File);
          }
          __finally__removed
          ({
            FTerminal->SetExceptionOnFail(false);
          })
        }
        catch (Exception &E)
        {
          if (isa<EFatal>(&E))
          {
            throw;
          }
          File = nullptr;
          Failure = true;
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
  __finally__removed
  ({
    if (FTerminal->Active)
    {
      Packet.ChangeType(SSH_FXP_CLOSE);
      Packet.AddString(Handle);
      SendPacket(&Packet);
      // we are not interested in the response, do not wait for it
      ReserveResponse(&Packet, nullptr);
    }
  })
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadSymlink(TRemoteFile *SymlinkFile,
  TRemoteFile *&AFile)
{
  DebugAssert(SymlinkFile && SymlinkFile->GetIsSymLink());
  if (!SymlinkFile)
    return;
  DebugAssert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later

  // need to use full filename when resolving links within subdirectory
  // (i.e. for download)
  UnicodeString FileName = LocalCanonify(
    SymlinkFile->GetDirectory() != nullptr ? SymlinkFile->GetFullFileName() : SymlinkFile->GetFileName());

  TSFTPPacket ReadLinkPacket(SSH_FXP_READLINK, FCodePage);
  ReadLinkPacket.AddPathString(FileName, FUtfStrings);
  SendPacket(&ReadLinkPacket);
  ReserveResponse(&ReadLinkPacket, &ReadLinkPacket);

  // send second request before reading response to first one
  // (performance benefit)
  TSFTPPacket AttrsPacket(SSH_FXP_STAT, FCodePage);
  AttrsPacket.AddPathString(FileName, FUtfStrings);
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
  SymlinkFile->SetLinkTo(ReadLinkPacket.GetPathString(FUtfStrings));
  FTerminal->LogEvent(FORMAT("Link resolved to \"%s\".", SymlinkFile->GetLinkTo()));

  ReceiveResponse(&AttrsPacket, &AttrsPacket, SSH_FXP_ATTRS);
  // SymlinkFile->FileName was used instead SymlinkFile->LinkTo before, why?
  AFile = LoadFile(&AttrsPacket, SymlinkFile,
    base::UnixExtractFileName(SymlinkFile->GetLinkTo()));
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadFile(const UnicodeString AFileName,
  TRemoteFile *&AFile)
{
  CustomReadFile(AFileName, AFile, SSH_FXP_LSTAT);
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::RemoteFileExists(const UnicodeString FullPath,
  TRemoteFile ** AFile)
{
  bool Result;
  try
  {
    TRemoteFile *File = nullptr;
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
  catch (...)
  {
    if (!FTerminal->GetActive())
    {
      throw;
    }
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SendCustomReadFile(TSFTPPacket *Packet,
  TSFTPPacket *Response, uint32_t Flags)
{
  if (FVersion >= 4)
  {
    Packet->AddCardinal(Flags);
  }
  SendPacket(Packet);
  ReserveResponse(Packet, Response);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CustomReadFile(UnicodeString AFileName,
  TRemoteFile *&AFile, SSH_FXP_TYPES Type, TRemoteFile *ALinkedByFile,
  SSH_FX_TYPES AllowStatus)
{
  SSH_FILEXFER_ATTR_TYPES Flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS |
    SSH_FILEXFER_ATTR_ACCESSTIME | SSH_FILEXFER_ATTR_MODIFYTIME |
    SSH_FILEXFER_ATTR_OWNERGROUP;
  TSFTPPacket Packet(Type, FCodePage);
  Packet.AddPathString(LocalCanonify(AFileName), FUtfStrings);
  SendCustomReadFile(&Packet, &Packet, Flags);
  ReceiveResponse(&Packet, &Packet, SSH_FXP_ATTRS, AllowStatus);

  if (Packet.GetType() == SSH_FXP_ATTRS)
  {
    AFile = LoadFile(&Packet, ALinkedByFile, base::UnixExtractFileName(AFileName));
  }
  else
  {
    DebugAssert(AllowStatus > 0);
    AFile = nullptr;
  }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DoDeleteFile(const UnicodeString AFileName, SSH_FXP_TYPES Type)
{
  TSFTPPacket Packet(Type, FCodePage);
  UnicodeString RealFileName = LocalCanonify(AFileName);
  Packet.AddPathString(RealFileName, FUtfStrings);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoteDeleteFile(const UnicodeString AFileName,
  const TRemoteFile *AFile, intptr_t AParams, TRmSessionAction &Action)
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
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoteRenameFile(const UnicodeString AFileName, const TRemoteFile * /*File*/,
  const UnicodeString ANewName)
{
  TSFTPPacket Packet(SSH_FXP_RENAME, FCodePage);
  UnicodeString RealName = LocalCanonify(AFileName);
  Packet.AddPathString(RealName, FUtfStrings);
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
  Packet.AddPathString(TargetName, FUtfStrings);
  if (FVersion >= 5)
  {
    Packet.AddCardinal(0);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoteCopyFile(const UnicodeString AFileName, const TRemoteFile * /*AFile*/,
  const UnicodeString ANewName)
{
  // Implemented by ProFTPD/mod_sftp and Bitvise WinSSHD (without announcing it)
  DebugAssert(SupportsExtension(SFTP_EXT_COPY_FILE) || (FSecureShell->GetSshImplementation() == sshiBitvise));
  TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
  Packet.AddString(SFTP_EXT_COPY_FILE);
  Packet.AddPathString(Canonify(AFileName), FUtfStrings);
  Packet.AddPathString(Canonify(ANewName), FUtfStrings);
  Packet.AddBool(false);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoteCreateDirectory(const UnicodeString ADirName)
{
  TSFTPPacket Packet(SSH_FXP_MKDIR, FCodePage);
  UnicodeString CanonifiedName = Canonify(ADirName);
  Packet.AddPathString(CanonifiedName, FUtfStrings);
  Packet.AddProperties(nullptr, 0, true, FVersion, FUtfStrings, nullptr);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoteCreateLink(const UnicodeString AFileName,
  UnicodeString PointTo, bool Symbolic)
{
  // Cerberus server does not even respond to LINK or SYMLINK,
  // Although its log says:
  // Unrecognized SFTP client command: (20)
  // Unknown SFTP packet - Sending Unsupported OP response

  DebugAssert(FVersion >= 3); // links are supported with SFTP version 3 and later
  bool UseLink = (FVersion >= 6);
  bool UseHardlink = !Symbolic && !UseLink && FSupportsHardlink;
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
          intptr_t ProFTPDMajorVer = ::StrToIntDef(CutToChar(ProFTPDVerStr, L'.', false), 0);
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
  UnicodeString FinalFileName = Canonify(AFileName);

  if (!Symbolic)
  {
    FinalPointTo = Canonify(PointTo);
  }

  if (!Buggy)
  {
    Packet.AddPathString(FinalFileName, FUtfStrings);
    Packet.AddPathString(FinalPointTo, FUtfStrings);
  }
  else
  {
    Packet.AddPathString(FinalPointTo, FUtfStrings);
    Packet.AddPathString(FinalFileName, FUtfStrings);
  }

  if (UseLink)
  {
    Packet.AddBool(Symbolic);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ChangeFileProperties(const UnicodeString AFileName,
  const TRemoteFile * /*AFile*/, const TRemoteProperties *AProperties,
  TChmodSessionAction &Action)
{
  DebugAssert(AProperties != nullptr);
  if (!AProperties)
    return;

  TRemoteFile *File = nullptr;

  UnicodeString RealFileName = LocalCanonify(AFileName);
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
          ToPtr(const_cast<TRemoteProperties *>(AProperties)));
      }
      catch (...)
      {
        Action.Cancel();
        throw;
      }
    }

    // SFTP can change owner and group at the same time only, not individually.
    // Fortunately we know current owner/group, so if only one is present,
    // we can supplement the other.
    if (AProperties)
    {
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
      Packet.AddPathString(RealFileName, FUtfStrings);
      Packet.AddProperties(&Properties, *File->GetRights(), File->GetIsDirectory(), FVersion, FUtfStrings, &Action);
      SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
    }
  }
  __finally__removed
  ({
    delete File;
  })
}
//---------------------------------------------------------------------------
bool __fastcall TSFTPFileSystem::LoadFilesProperties(TStrings *AFileList)
{
  bool Result = false;
  // without knowledge of server's capabilities, this all make no sense
  if (FSupport->Loaded || (FSecureShell->GetSshImplementation() == sshiBitvise))
  {
    TFileOperationProgressType Progress(nb::bind(&TTerminal::DoProgress, FTerminal), nb::bind(&TTerminal::DoFinished, FTerminal));
    Progress.Start(foGetProperties, osRemote, AFileList->GetCount());

    FTerminal->SetOperationProgress(&Progress); //-V506

    static int LoadFilesPropertiesQueueLen = 5;
    TSFTPLoadFilesPropertiesQueue Queue(this, FCodePage);
    try__finally
    {
      SCOPE_EXIT
      {
        Queue.DisposeSafe();
        FTerminal->SetOperationProgress(nullptr);
        Progress.Stop();
      };

      static intptr_t LoadFilesPropertiesQueueLen = 5;
      if (Queue.Init(LoadFilesPropertiesQueueLen, AFileList))
      {
        TRemoteFile *File = nullptr;
        TSFTPPacket Packet(FCodePage);
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
    __finally__removed
    ({
      Queue.DisposeSafe();
      FTerminal->FOperationProgress = nullptr;
      Progress.Stop();
    })
    // queue is discarded here
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DoCalculateFilesChecksum(
  const UnicodeString Alg, const UnicodeString SftpAlg,
  TStrings *AFileList, TStrings *Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum,
  TFileOperationProgressType *OperationProgress, bool FirstLevel)
{
  TOnceDoneOperation OnceDoneOperation; // not used

  // recurse into subdirectories only if we have callback function
  if (OnCalculatedChecksum != nullptr)
  {
    for (intptr_t Index1 = 0; Index1 < AFileList->GetCount(); ++Index1)
    {
      TRemoteFile *File = AFileList->GetAs<TRemoteFile>(Index1);
      DebugAssert(File != nullptr);
      if (File && File->GetIsDirectory() && FTerminal->CanRecurseToDirectory(File) &&
        !File->GetIsParentDirectory() && !File->GetIsThisDirectory())
      {
        OperationProgress->SetFile(File->GetFileName());
        std::unique_ptr<TRemoteFileList> SubFiles(
          FTerminal->CustomReadDirectoryListing(File->GetFullFileName(), false));

        if (SubFiles.get() != nullptr)
        {
          std::unique_ptr<TStrings> SubFileList(new TStringList());
          bool Success = false;
          try__finally
          {
            SCOPE_EXIT
            {
              if (FirstLevel && File)
              {
                OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
              }
            };
            OperationProgress->SetFile(File->GetFileName());

            for (intptr_t Index2 = 0; Index2 < SubFiles->GetCount(); ++Index2)
            {
              TRemoteFile *SubFile = SubFiles->GetFile(Index2);
              SubFileList->AddObject(SubFile->GetFullFileName(), SubFile);
            }

            // do not collect checksums for files in subdirectories,
            // only send back checksums via callback
            DoCalculateFilesChecksum(Alg, SftpAlg, SubFileList.get(), nullptr,
              OnCalculatedChecksum, OperationProgress, false);

            Success = true;
          }
          __finally__removed
          ({
            delete SubFiles;
            delete SubFileList;

            if (FirstLevel)
            {
              OperationProgress->Finish(File->FileName, Success, OnceDoneOperation);
            }
          })
        }
      }
    }
  }

  static int CalculateFilesChecksumQueueLen = 5;
  TSFTPCalculateFilesChecksumQueue Queue(this, FCodePage);
  try__finally
  {
    SCOPE_EXIT
    {
      Queue.DisposeSafe();
    };
    if (Queue.Init(CalculateFilesChecksumQueueLen, SftpAlg, AFileList))
    {
      TSFTPPacket Packet(FCodePage);
      bool Next;
      do
      {
        bool Success = false;
        UnicodeString Checksum;
        TRemoteFile *File = nullptr;

        try__finally
        {
          SCOPE_EXIT
          {
            if (FirstLevel && File)
            {
              OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
            }
          };
          TChecksumSessionAction Action(FTerminal->GetActionLog());
          try
          {
            Next = Queue.ReceivePacket(&Packet, File);
            DebugAssert(Packet.GetType() == SSH_FXP_EXTENDED_REPLY);

            OperationProgress->SetFile(File->GetFileName());
            Action.SetFileName(FTerminal->GetAbsolutePath(File->GetFullFileName(), true));

            // skip alg
            Packet.GetAnsiString();
            Checksum = BytesToHex(reinterpret_cast<const unsigned char *>(Packet.GetNextData(Packet.GetRemainingLength())), Packet.GetRemainingLength(), false);
            if (OnCalculatedChecksum != nullptr)
            {
              OnCalculatedChecksum(File->GetFileName(), Alg, Checksum);
            }
            Action.Checksum(Alg, Checksum);

            Success = true;
          }
          catch (Exception &E)
          {
            FTerminal->RollbackAction(Action, OperationProgress, &E);

            // Error formatting expanded from inline to avoid strange exceptions
            UnicodeString Error =
              FMTLOAD(CHECKSUM_ERROR,
                (File != nullptr ? File->GetFullFileName() : L""));
            FTerminal->CommandError(&E, Error);
            TODO("retries? resume?");
            Next = false;
          }

          if (Checksums != nullptr)
          {
            Checksums->Add(Checksum);
          }
        }
        __finally__removed
        ({
          if (FirstLevel && File)
          {
            OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
          }
        })

        if (OperationProgress->GetCancel() != csContinue)
        {
          Next = false;
        }
      }
      while (Next);
    }
  }
  __finally__removed
  ({
    Queue.DisposeSafe();
  })
  // queue is discarded here
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CalculateFilesChecksum(const UnicodeString Alg,
  TStrings *AFileList, TStrings *Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  TFileOperationProgressType Progress(nb::bind(&TTerminal::DoProgress, FTerminal), nb::bind(&TTerminal::DoFinished, FTerminal));
  Progress.Start(foCalculateChecksum, osRemote, AFileList->GetCount());

  UnicodeString NormalizedAlg = FindIdent(Alg, FChecksumAlgs.get());
  UnicodeString SftpAlg;
  intptr_t Index = FChecksumAlgs->IndexOf(NormalizedAlg);
  if (Index >= 0)
  {
    SftpAlg = FChecksumSftpAlgs->GetString(Index);
  }
  else
  {
    // try user-specified alg
    SftpAlg = NormalizedAlg;
  }

  FTerminal->SetOperationProgress(&Progress); //-V506

  try__finally
  {
    SCOPE_EXIT
    {
      FTerminal->SetOperationProgress(nullptr);
      Progress.Stop();
    };
    DoCalculateFilesChecksum(NormalizedAlg, SftpAlg, AFileList, Checksums, OnCalculatedChecksum,
      &Progress, true);
  }
  __finally__removed
  ({
    FTerminal->FOperationProgress = nullptr;
    Progress.Stop();
  })
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CustomCommandOnFile(const UnicodeString /*AFileName*/,
  const TRemoteFile * /*AFile*/, const UnicodeString /*Command*/, intptr_t /*AParams*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::AnyCommand(const UnicodeString /*Command*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
TStrings * __fastcall TSFTPFileSystem::GetFixedPaths() const
{
  return FFixedPaths;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SpaceAvailable(const UnicodeString APath,
  TSpaceAvailable &ASpaceAvailable)
{
  if (SupportsExtension(SFTP_EXT_SPACE_AVAILABLE) ||
    // See comment in IsCapable
    (FSecureShell->GetSshImplementation() == sshiBitvise))
  {
    TSFTPPacket Packet(SSH_FXP_EXTENDED, FCodePage);
    Packet.AddString(SFTP_EXT_SPACE_AVAILABLE);
    Packet.AddPathString(LocalCanonify(APath), FUtfStrings);

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
    Packet.AddPathString(LocalCanonify(APath), FUtfStrings);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);

    int64_t BlockSize = Packet.GetInt64(); // file system block size
    int64_t FundamentalBlockSize = Packet.GetInt64(); // fundamental fs block size
    int64_t Blocks = Packet.GetInt64(); // number of blocks (unit f_frsize)
    int64_t FreeBlocks = Packet.GetInt64(); // free blocks in file system
    int64_t AvailableBlocks = Packet.GetInt64(); // free blocks for non-root
    int64_t FileINodes = Packet.GetInt64(); // total file inodes
    int64_t FreeFileINodes = Packet.GetInt64(); // free file inodes
    int64_t AvailableFileINodes = Packet.GetInt64(); // free file inodes for to non-root
    int64_t SID = Packet.GetInt64(); // file system id
    int64_t Flags = Packet.GetInt64(); // bit mask of f_flag values
    int64_t NameMax = Packet.GetInt64(); // maximum filename length

    FTerminal->LogEvent(FORMAT("Block size: %s", ::Int64ToStr(BlockSize)));
    FTerminal->LogEvent(FORMAT("Fundamental block size: %s", ::Int64ToStr(FundamentalBlockSize)));
    FTerminal->LogEvent(FORMAT("Total blocks: %s", ::Int64ToStr(Blocks)));
    FTerminal->LogEvent(FORMAT("Free blocks: %s", ::Int64ToStr(FreeBlocks)));
    FTerminal->LogEvent(FORMAT("Free blocks for non-root: %s", ::Int64ToStr(AvailableBlocks)));
    FTerminal->LogEvent(FORMAT("Total file inodes: %s", ::Int64ToStr(FileINodes)));
    FTerminal->LogEvent(FORMAT("Free file inodes: %s", ::Int64ToStr(FreeFileINodes)));
    FTerminal->LogEvent(FORMAT("Free file inodes for non-root: %s", ::Int64ToStr(AvailableFileINodes)));
    FTerminal->LogEvent(FORMAT("File system ID: %s", BytesToHex(reinterpret_cast<const uint8_t *>(&SID), sizeof(SID))));
    UnicodeString FlagStr;
    if (FLAGSET(Flags, SFTP_EXT_STATVFS_ST_RDONLY))
    {
      AddToList(FlagStr, L"read-only", L",");
      Flags -= SFTP_EXT_STATVFS_ST_RDONLY;
    }
    if (FLAGSET(Flags, SFTP_EXT_STATVFS_ST_NOSUID))
    {
      AddToList(FlagStr, L"no-setuid", L",");
      Flags -= SFTP_EXT_STATVFS_ST_NOSUID;
    }
    if (Flags != 0)
    {
      AddToList(FlagStr, UnicodeString(L"0x") + ::IntToHex(ToUIntPtr(Flags), 2), L",");
    }
    if (FlagStr.IsEmpty())
    {
      FlagStr = L"none";
    }
    FTerminal->LogEvent(FORMAT("Flags: %s", FlagStr));
    FTerminal->LogEvent(FORMAT("Max name length: %s", ::Int64ToStr(NameMax)));

    ASpaceAvailable.BytesOnDevice = BlockSize * Blocks;
    ASpaceAvailable.UnusedBytesOnDevice = BlockSize * FreeBlocks;
    ASpaceAvailable.BytesAvailableToUser = 0;
    ASpaceAvailable.UnusedBytesAvailableToUser = BlockSize * AvailableBlocks;
    ASpaceAvailable.BytesPerAllocationUnit =
      (BlockSize > UINT_MAX /*std::numeric_limits<uint32_t>::max()*/) ? 0 : ToUInt32(BlockSize);
  }
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CopyToRemote(TStrings *AFilesToCopy,
  const UnicodeString ATargetDir, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress,
  TOnceDoneOperation &OnceDoneOperation)
{
  volatile TAutoFlag AvoidBusyFlag(FAvoidBusy);
  FTerminal->DoCopyToRemote(AFilesToCopy, ATargetDir, CopyParam, Params, OperationProgress, tfPreCreateDir, OnceDoneOperation);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPConfirmOverwrite(
  const UnicodeString ASourceFullFileName, UnicodeString &ATargetFileName,
  const TCopyParamType *CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
  const TOverwriteFileParams *FileParams,
  TOverwriteMode &OverwriteMode)
{
  bool CanAppend = (FVersion < 4) || !OperationProgress->GetAsciiTransfer();
  bool CanResume =
    (FileParams != nullptr) &&
    (FileParams->DestSize < FileParams->SourceSize);
  uint32_t Answer;

  {
    volatile TSuspendFileOperationProgress Suspend(OperationProgress);
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
    Aliases[0].GrouppedShiftState = ssAlt;
    Aliases[1] = TQueryButtonAlias::CreateAllAsYesToNewerGrouppedWithYes();
    Aliases[2] = TQueryButtonAlias::CreateIgnoreAsRenameGrouppedWithNo();
    Aliases[3] = TQueryButtonAlias::CreateYesToAllGrouppedWithYes();
    Aliases[4] = TQueryButtonAlias::CreateNoToAllGrouppedWithNo();
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
      SCOPE_EXIT
      {
        OperationProgress->UnlockUserSelections();
      };
      // duplicated in TTerminal::ConfirmFileOverwrite
      bool CanAlternateResume =
        FileParams ? (FileParams->DestSize < FileParams->SourceSize) && !OperationProgress->GetAsciiTransfer() : false;
      TBatchOverwrite BatchOverwrite =
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
        TQueryParams Params(0, HELP_APPEND_OR_RESUME);

        {
          volatile TSuspendFileOperationProgress Suspend(OperationProgress);
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

        default:
          DebugFail(); //fallthru
        case qaCancel:
          OperationProgress->SetCancelAtLeast(csCancel);
          Abort();
          break;
        }
      }
    }
    __finally__removed
    ({
      OperationProgress->UnlockUserSelections();
    })
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
//---------------------------------------------------------------------------
bool TSFTPFileSystem::SFTPConfirmResume(const UnicodeString DestFileName,
  bool PartialBiggerThanSource, TFileOperationProgressType *OperationProgress)
{
  bool ResumeTransfer = false;
  DebugAssert(OperationProgress);
  if (PartialBiggerThanSource)
  {
    uint32_t Answer;
    {
      volatile TSuspendFileOperationProgress Suspend(OperationProgress);
      TQueryParams Params(qpAllowContinueOnError, HELP_PARTIAL_BIGGER_THAN_SOURCE);
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
      volatile TSuspendFileOperationProgress Suspend(OperationProgress);
      TQueryParams Params(qpAllowContinueOnError | qpNeverAskAgainCheck,
        HELP_RESUME_TRANSFER);
      // "abort" replaced with "cancel" to unify with "append/resume" query
      Answer = FTerminal->QueryUser(
        FMTLOAD(RESUME_TRANSFER2, DestFileName), nullptr, qaYes | qaNo | qaCancel,
        &Params);
    }

    switch (Answer)
    {
    case qaNeverAskAgain:
      FTerminal->GetConfiguration()->SetConfirmResume(false);
    //FALLTHROU
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
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Source(
  TLocalFileHandle &AHandle, const UnicodeString ATargetDir, UnicodeString &ADestFileName,
  const TCopyParamType *CopyParam, intptr_t Params,
  TFileOperationProgressType *OperationProgress, uintptr_t Flags,
  TUploadSessionAction &Action, bool &ChildError)
{
  UnicodeString DestFullName = LocalCanonify(ATargetDir + ADestFileName);
  UnicodeString DestPartialFullName;
  __removed bool ResumeAllowed;
  bool ResumeTransfer = false;
  bool DestFileExists = false;
  TRights DestRights;

  int64_t ResumeOffset;

  // should we check for interrupted transfer?
  bool ResumeAllowed = !OperationProgress->GetAsciiTransfer() &&
    CopyParam->AllowResume(OperationProgress->GetLocalSize()) &&
    IsCapable(fcRename);

  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;

  TOverwriteFileParams FileParams;
  FileParams.SourceSize = OperationProgress->GetLocalSize();
  FileParams.SourceTimestamp = AHandle.Modification;

  if (ResumeAllowed)
  {
    DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();

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
        OpenParams.DestFileSize = FilePtr->GetSize();
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
        else if ((FVersion < 4) &&
                 ((*FilePtr->GetRights() & TRights::rfAll) == TRights::rfAll) &&
                 (FilePtr->GetSize() < 100))
        {
          ResumeAllowed = false;
          FTerminal->LogEvent("Existing file looks like a symbolic link, not doing resumable transfer.");
        }
        // Also never do resumable transfer for file owned by other user
        // as deleting and recreating the file would change ownership.
        // This won't for work for SFTP-3 (OpenSSH) as it does not provide
        // owner name (only UID) and we know only logged in user name (not UID)
        else if (!FilePtr->GetFileOwner().GetName().IsEmpty() && !base::SameUserName(FilePtr->GetFileOwner().GetName(), FTerminal->TerminalGetUserName()))
        {
          ResumeAllowed = false;
          FTerminal->LogEvent(
            FORMAT("Existing file is owned by another user [%s], not doing resumable transfer.", File->GetFileOwner().GetName()));
        }

        FilePtr.reset();
        __removed delete File;
        __removed File = NULL;
      }

      if (ResumeAllowed)
      {
        FTerminal->LogEvent("Checking existence of partially transferred file.");
        if (RemoteFileExists(DestPartialFullName, &File))
        {
          ResumeOffset = FilePtr->GetSize();
          FilePtr.reset();
          __removed delete File;
          __removed File = NULL;

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
          // partial upload file does not exists, check for full file
          if (DestFileExists)
          {
            UnicodeString PrevDestFileName = ADestFileName;
            SFTPConfirmOverwrite(AHandle.FileName, ADestFileName,
              CopyParam, Params, OperationProgress, &FileParams, OpenParams.OverwriteMode);
            if (PrevDestFileName != ADestFileName)
            {
              // update paths in case user changes the file name
              DestFullName = LocalCanonify(ATargetDir + ADestFileName);
              DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
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

  UnicodeString RemoteFileName = DoResume ? DestPartialFullName : DestFullName;
  OpenParams.FileName = AHandle.FileName;
  OpenParams.RemoteFileName = RemoteFileName;
  OpenParams.Resume = DoResume;
  OpenParams.Resuming = ResumeTransfer;
  OpenParams.OperationProgress = OperationProgress;
  OpenParams.CopyParam = CopyParam;
  OpenParams.Params = Params;
  OpenParams.FileParams = &FileParams;
  OpenParams.Confirmed = false;

  FTerminal->LogEvent("Opening remote file.");
  FTerminal->FileOperationLoop(nb::bind(&TSFTPFileSystem::SFTPOpenRemote, this), OperationProgress, folAllowSkip,
    FMTLOAD(SFTP_CREATE_FILE_ERROR, OpenParams.RemoteFileName),
    &OpenParams);
  OperationProgress->Progress();

  if (OpenParams.RemoteFileName != RemoteFileName)
  {
    DebugAssert(!DoResume);
    DebugAssert(base::UnixExtractFilePath(OpenParams.RemoteFileName) == base::UnixExtractFilePath(RemoteFileName));
    DestFullName = OpenParams.RemoteFileName;
    UnicodeString NewFileName = base::UnixExtractFileName(DestFullName);
    DebugAssert(ADestFileName != NewFileName);
    ADestFileName = NewFileName;
  }

  Action.Destination(DestFullName);

  bool TransferFinished = false;
  __removed int64_t DestWriteOffset = 0;
  TSFTPPacket CloseRequest(FCodePage);
  bool SetRights = ((DoResume && DestFileExists) || CopyParam->GetPreserveRights());
  bool SetProperties = (CopyParam->GetPreserveTime() || SetRights);
  TSFTPPacket PropertiesRequest(SSH_FXP_SETSTAT, FCodePage);
  TSFTPPacket PropertiesResponse(FCodePage);
  TRights Rights;
  if (SetProperties)
  {
    PropertiesRequest.AddPathString(DestFullName, FUtfStrings);
    if (CopyParam->GetPreserveRights())
    {
      Rights = CopyParam->RemoteFileRights(AHandle.Attrs);
    }
    else if (DoResume && DestFileExists)
    {
      Rights = DestRights;
    }
    else
    {
      DebugAssert(!SetRights);
    }

    uint16_t RightsNumber = Rights.GetNumberSet();
    PropertiesRequest.AddProperties(
      SetRights ? &RightsNumber : nullptr, nullptr, nullptr,
      CopyParam->GetPreserveTime() ? &AHandle.MTime : nullptr,
      nullptr, nullptr, false, FVersion, FUtfStrings);
  }

  try__finally
  {
    SCOPE_EXIT
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
    };

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
      FileSeek((THandle)AHandle.Handle, ResumeOffset, 0);
      OperationProgress->AddResumed(ResumeOffset);
    }

    TSFTPUploadQueue Queue(this, FCodePage);
    try__finally
    {
      SCOPE_EXIT
      {
        // Either queue is empty now (noop call then),
        // or some error occured (in that case, process remaining responses, ignoring other errors)
        Queue.DisposeSafe();
      };
      int ConvertParams =
        FLAGMASK(CopyParam->GetRemoveCtrlZ(), cpRemoveCtrlZ) |
        FLAGMASK(CopyParam->GetRemoveBOM(), cpRemoveBOM);
      Queue.Init(AHandle.FileName, AHandle.Handle, OperationProgress,
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
    __finally__removed
    ({
      // Either queue is empty now (noop call then),
      // or some error occured (in that case, process remaining responses, ignoring other errors)
      Queue.DisposeSafe();
    })

    TransferFinished = true;
    // queue is discarded here
  }
  __finally__removed
  ({
    if (FTerminal->Active)
    {
      // if file transfer was finished, the close request was already sent
      if (!OpenParams.RemoteFileHandle.IsEmpty())
      {
        SFTPCloseRemote(OpenParams.RemoteFileHandle, DestFileName,
          OperationProgress, TransferFinished, true, &CloseRequest);
      }
      // wait for the response
      SFTPCloseRemote(OpenParams.RemoteFileHandle, DestFileName,
        OperationProgress, TransferFinished, false, &CloseRequest);

      // delete file if transfer was not completed, resuming was not allowed and
      // we were not appending (incl. alternate resume),
      // shortly after plain transfer completes (eq. !ResumeAllowed)
      if (!TransferFinished && !DoResume && (OpenParams.OverwriteMode == omOverwrite))
      {
        DoDeleteFile(OpenParams.RemoteFileName, SSH_FXP_REMOVE);
      }
    }
  })

  OperationProgress->Progress();

  if (DoResume)
  {
    if (DestFileExists)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(DELETE_ON_RESUME_ERROR, base::UnixExtractFileName(DestFullName),
        DestFullName), "",
      [&]()
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
      });
      __removed FILE_OPERATION_LOOP_END(
      __removed   FMTLOAD(DELETE_ON_RESUME_ERROR,
      __removed     (UnixExtractFileName(DestFullName), DestFullName)));
    }

    // originally this was before CLOSE (last __finally statement),
    // on VShell it failed
    FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
      FMTLOAD(RENAME_AFTER_RESUME_ERROR,
        base::UnixExtractFileName(OpenParams.RemoteFileName), ADestFileName),
      HELP_RENAME_AFTER_RESUME_ERROR,
    [&]()
    {
      this->RemoteRenameFile(OpenParams.RemoteFileName, nullptr, ADestFileName);
    });
    __removed FILE_OPERATION_LOOP_END_CUSTOM(
    __removed   FMTLOAD(RENAME_AFTER_RESUME_ERROR,
    __removed     (UnixExtractFileName(OpenParams.RemoteFileName), DestFileName)),
    __removed   folAllowSkip, HELP_RENAME_AFTER_RESUME_ERROR);
  }

  if (SetProperties)
  {
    std::unique_ptr<TTouchSessionAction> TouchAction;
    if (CopyParam->GetPreserveTime())
    {
      TDateTime MDateTime = ::UnixToDateTime(AHandle.MTime, GetSessionData()->GetDSTMode());
      FTerminal->LogEvent(FORMAT("Preserving timestamp [%s]",
        StandardTimestamp(MDateTime)));
      TouchAction.reset(new TTouchSessionAction(FTerminal->GetActionLog(), DestFullName,
        MDateTime));
    }
    std::unique_ptr<TChmodSessionAction> ChmodAction;
    // do record chmod only if it was explicitly requested,
    // not when it was implicitly performed to apply timestamp
    // of overwritten file to new file
    if (CopyParam->GetPreserveRights())
    {
      ChmodAction.reset(new TChmodSessionAction(FTerminal->GetActionLog(), DestFullName, Rights));
    }
    try
    {
      // when resuming is enabled, the set properties request was not sent yet
      if (DoResume)
      {
        SendPacket(&PropertiesRequest);
      }
      bool Resend = false;
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(PRESERVE_TIME_PERM_ERROR3, ADestFileName),
        HELP_PRESERVE_TIME_PERM_ERROR,
      [&]()
      {
        try
        {
          TSFTPPacket DummyResponse(FCodePage);
          TSFTPPacket *Response = &PropertiesResponse;
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
        catch (...)
        {
          if (FTerminal->GetActive() &&
              (!CopyParam->GetPreserveRights() && !CopyParam->GetPreserveTime()))
          {
            DebugAssert(DoResume);
            FTerminal->LogEvent("Ignoring error preserving permissions of overwritten file");
          }
          else
          {
            throw;
          }
        }
      });
      __removed FILE_OPERATION_LOOP_END_CUSTOM(
      __removed   FMTLOAD(PRESERVE_TIME_PERM_ERROR3, (DestFileName)),
      __removed   folAllowSkip, HELP_PRESERVE_TIME_PERM_ERROR);
    }
    catch(Exception & E)
    {
      if (TouchAction.get() != nullptr)
      {
        TouchAction->Rollback(&E);
      }
      if (ChmodAction.get() != nullptr)
      {
        ChmodAction->Rollback(&E);
      }
      ChildError = true;
      throw;
    }
  }
}
//---------------------------------------------------------------------------
RawByteString __fastcall TSFTPFileSystem::SFTPOpenRemoteFile(
  const UnicodeString AFileName, SSH_FXF_TYPES OpenType, int64_t Size)
{
  TSFTPPacket Packet(SSH_FXP_OPEN, FCodePage);

  Packet.AddPathString(AFileName, FUtfStrings);
  if (FVersion < 5)
  {
    Packet.AddCardinal(OpenType);
  }
  else
  {
    ACE4_TYPES Access =
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_READ), ACE4_READ_DATA) |
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_WRITE), ACE4_WRITE_DATA | ACE4_APPEND_DATA);

    SSH_FXF_TYPES Flags;

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

  bool SendSize =
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
//---------------------------------------------------------------------------
intptr_t __fastcall TSFTPFileSystem::SFTPOpenRemote(void *AOpenParams, void * /*Param2*/)
{
  TOpenRemoteFileParams *OpenParams = get_as<TOpenRemoteFileParams>(AOpenParams);
  DebugAssert(OpenParams);
  TFileOperationProgressType *OperationProgress = OpenParams->OperationProgress;

  SSH_FXF_TYPES OpenType = 0;
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
      if ((ConfirmOverwriting || GetSessionData()->GetOverwrittenToRecycleBin()) &&
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

      OpenParams->RemoteFileHandle = SFTPOpenRemoteFile(
        OpenParams->RemoteFileName, OpenType, OperationProgress->GetLocalSize());

      Success = true;
    }
    catch (Exception &E)
    {
      if (!OpenParams->Confirmed && (OpenType & SSH_FXF_EXCL) && FTerminal->GetActive())
      {
        FTerminal->LogEvent(FORMAT("Cannot create new file \"%s\", checking if it exists already", OpenParams->RemoteFileName));

        bool ThrowOriginal = false;

        // When exclusive opening of file fails, try to detect if file exists.
        // When file does not exist, failure was probably caused by 'permission denied'
        // or similar error. In this case throw original exception.
        try
        {
          OperationProgress->Progress();
          TRemoteFile *File = nullptr;
          UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
          ReadFile(RealFileName, File);
          std::unique_ptr<TRemoteFile> FilePtr(File);
          DebugAssert(FilePtr.get());
          OpenParams->DestFileSize = FilePtr->GetSize();
          if ((OpenParams->FileParams != nullptr) && (File != nullptr))
          {
            OpenParams->FileParams->DestTimestamp = File->GetModification();
            OpenParams->FileParams->DestSize = OpenParams->DestFileSize;
          }
          // file exists (otherwise exception was thrown)
          __removed SAFE_DESTROY(File);
        }
        catch (...)
        {
          if (!FTerminal->GetActive())
          {
            throw;
          }
          ThrowOriginal = true;
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
            OpenParams->CopyParam, OpenParams->Params, OperationProgress, OpenParams->FileParams, OpenParams->OverwriteMode);
          if (RemoteFileNameOnly != base::UnixExtractFileName(OpenParams->RemoteFileName))
          {
            OpenParams->RemoteFileName =
              base::UnixExtractFilePath(OpenParams->RemoteFileName) + RemoteFileNameOnly;
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
          OperationProgress->Progress();
          FTerminal->RecycleFile(OpenParams->RemoteFileName, nullptr);
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
            TRemoteFile *File = nullptr;
            UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
            ReadFile(RealFileName, File);
            SAFE_DESTROY(File);
          }
          catch (...)
          {
            if (!FTerminal->GetActive())
            {
              throw;
            }
            ThrowOriginal = true;
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
          intptr_t Params = dfNoRecursive;
          FTerminal->RemoteDeleteFile(OpenParams->RemoteFileName, nullptr, &Params);
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
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPCloseRemote(const RawByteString Handle,
  const UnicodeString AFileName, TFileOperationProgressType *OperationProgress,
  bool TransferFinished, bool Request, TSFTPPacket *Packet)
{
  // Moving this out of SFTPSource() fixed external exception 0xC0000029 error
  FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
    FMTLOAD(SFTP_CLOSE_FILE_ERROR, AFileName), "",
  [&]()
  {
    try
    {
      TSFTPPacket CloseRequest(FCodePage);
      TSFTPPacket *P = (Packet == nullptr ? &CloseRequest : Packet);

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
    catch (...)
    {
      if (!FTerminal->GetActive() || TransferFinished)
      {
        throw;
      }
    }
  });
  __removed FILE_OPERATION_LOOP_END(FMTLOAD(SFTP_CLOSE_FILE_ERROR, (FileName)));
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CopyToLocal(TStrings *AFilesToCopy,
  const UnicodeString TargetDir, const TCopyParamType *CopyParam,
  intptr_t Params, TFileOperationProgressType *OperationProgress,
  TOnceDoneOperation &OnceDoneOperation)
{
  volatile TAutoFlag AvoidBusyFlag(FAvoidBusy);
  FTerminal->DoCopyToLocal(AFilesToCopy, TargetDir, CopyParam, Params, OperationProgress, tfNone, OnceDoneOperation);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DirectorySunk(
  const UnicodeString ADestFullName, const TRemoteFile *AFile, const TCopyParamType *CopyParam)
{
  if (CopyParam->GetPreserveTime() && CopyParam->GetPreserveTimeDirs())
  {
    // FILE_FLAG_BACKUP_SEMANTICS is needed to "open" directory
    HANDLE LocalHandle =
      CreateFile(
        ApiPath(ADestFullName).c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, 0);

    if (LocalHandle == INVALID_HANDLE_VALUE)
    {
      int SetFileTimeError = GetLastError();
      FTerminal->LogEvent(
        FORMAT("Preserving directory timestamp failed, ignoring: %s", ::SysErrorMessageForError(SetFileTimeError)));
    }
    else
    {
      FTerminal->UpdateTargetTime(LocalHandle, AFile->GetModification(), FTerminal->GetSessionData()->GetDSTMode());
      SAFE_CLOSE_HANDLE(LocalHandle);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Sink(
  const UnicodeString AFileName, const TRemoteFile *AFile,
  const UnicodeString ATargetDir, UnicodeString &ADestFileName, uintptr_t Attrs,
  const TCopyParamType * CopyParam, intptr_t AParams, TFileOperationProgressType *OperationProgress,
  uintptr_t /*AFlags*/, TDownloadSessionAction &Action)
{
  // resume has no sense for temporary downloads
  bool ResumeAllowed =
    FLAGCLEAR(AParams, cpTemporary) &&
    !OperationProgress->GetAsciiTransfer() &&
    CopyParam->AllowResume(OperationProgress->GetTransferSize());

  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  TStream *FileStream = nullptr;
  bool DeleteLocalFile = false;
  RawByteString RemoteHandle;
  UnicodeString DestFullName = ATargetDir + ADestFileName;
  UnicodeString LocalFileName = DestFullName;
  TOverwriteMode OverwriteMode = omOverwrite;
  UnicodeString ExpandedDestFullName;

  try__finally
  {
    SCOPE_EXIT
    {
      SAFE_CLOSE_HANDLE(LocalFileHandle);
      if (FileStream)
      {
        SAFE_DESTROY(FileStream);
      }
      if (DeleteLocalFile && (!ResumeAllowed || OperationProgress->GetLocallyUsed() == 0) &&
        (OverwriteMode == omOverwrite))
      {
        FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
          FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, LocalFileName), "",
        [&]()
        {
          THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(LocalFileName)));
        });
        __removed FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (LocalFileName)));
      }

      // if the transfer was finished, the file is closed already
      if (FTerminal && FTerminal->GetActive() && !RemoteHandle.IsEmpty())
      {
        // do not wait for response
        SFTPCloseRemote(RemoteHandle, ADestFileName, OperationProgress,
          true, true, nullptr);
      }
    };
    bool ResumeTransfer = false;
    UnicodeString DestPartialFullName;

    if (ResumeAllowed)
    {
      DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
      LocalFileName = DestPartialFullName;

      FTerminal->LogEvent("Checking existence of partially transferred file.");
      if (FileExists(ApiPath(DestPartialFullName)))
      {
        FTerminal->LogEvent("Partially transferred file exists.");
        int64_t ResumeOffset;
        FTerminal->TerminalOpenLocalFile(DestPartialFullName, GENERIC_WRITE,
          nullptr, &LocalFileHandle, nullptr, nullptr, nullptr, &ResumeOffset);

        bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->GetTransferSize());
        if (FLAGCLEAR(AParams, cpNoConfirmation))
        {
          ResumeTransfer = SFTPConfirmResume(ADestFileName, PartialBiggerThanSource, OperationProgress);
        }
        else
        {
          ResumeTransfer = !PartialBiggerThanSource;
          if (!ResumeTransfer)
          {
            FTerminal->LogEvent("Partially transferred file is bigger that original file.");
          }
        }

        if (!ResumeTransfer)
        {
          SAFE_CLOSE_HANDLE(LocalFileHandle);
          LocalFileHandle = INVALID_HANDLE_VALUE;
          FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
            FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestPartialFullName), "",
          [&]()
          {
            THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(DestPartialFullName)));
          });
          __removed FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (DestPartialFullName)));
        }
        else
        {
          FTerminal->LogEvent("Resuming file transfer.");
          FileSeek((THandle)LocalFileHandle, ResumeOffset, 0);
          OperationProgress->AddResumed(ResumeOffset);
        }
      }

      OperationProgress->Progress();
    }

    // first open source file, not to loose the destination file,
    // if we cannot open the source one in the first place
    FTerminal->LogEvent("Opening remote file.");
    FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
      FMTLOAD(SFTP_OPEN_FILE_ERROR, AFileName), "",
    [&]()
    {
      SSH_FXF_TYPES OpenType = SSH_FXF_READ;
      if ((FVersion >= 4) && OperationProgress->GetAsciiTransfer())
      {
        OpenType |= SSH_FXF_TEXT;
      }
      RemoteHandle = SFTPOpenRemoteFile(AFileName, OpenType);
      OperationProgress->Progress();
    });
    __removed FILE_OPERATION_LOOP_END(FMTLOAD(SFTP_OPEN_FILE_ERROR, (FileName)));

    FILETIME AcTime;
    ClearStruct(AcTime);
    FILETIME WrTime;
    ClearStruct(WrTime);
    TSFTPPacket RemoteFilePacket(SSH_FXP_FSTAT, FCodePage);
    RemoteFilePacket.AddString(RemoteHandle);
    SendCustomReadFile(&RemoteFilePacket, &RemoteFilePacket, SSH_FILEXFER_ATTR_MODIFYTIME);
    ReceiveResponse(&RemoteFilePacket, &RemoteFilePacket);
    OperationProgress->Progress();

    TDateTime Modification = AFile->GetModification(); // fallback
    // ignore errors
    if (RemoteFilePacket.GetType() == SSH_FXP_ATTRS)
    {
      // load file, avoid completion (resolving symlinks) as we do not need that
      std::unique_ptr<TRemoteFile> File(
        LoadFile(&RemoteFilePacket, nullptr, base::UnixExtractFileName(AFileName), nullptr, false));
      if (File->GetModification() != TDateTime())
      {
        Modification = File->GetModification();
      }
    }

    if ((Attrs >= 0) && !ResumeTransfer)
    {
      int64_t DestFileSize;
      int64_t MTime;
      FTerminal->TerminalOpenLocalFile(
        DestFullName, GENERIC_WRITE, nullptr, &LocalFileHandle, nullptr, &MTime, nullptr, &DestFileSize, false);

      FTerminal->LogEvent("Confirming overwriting of file.");
      TOverwriteFileParams FileParams;
      FileParams.SourceSize = OperationProgress->GetTransferSize();
      FileParams.SourceTimestamp = Modification;
      FileParams.DestTimestamp = ::UnixToDateTime(MTime,
        FTerminal->GetSessionData()->GetDSTMode());
      FileParams.DestSize = DestFileSize;
      UnicodeString PrevDestFileName = ADestFileName;
      SFTPConfirmOverwrite(AFileName, ADestFileName, CopyParam, AParams, OperationProgress, &FileParams, OverwriteMode);
      if (PrevDestFileName != ADestFileName)
      {
        DestFullName = ATargetDir + ADestFileName;
        DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
        if (ResumeAllowed)
        {
          if (FileExists(ApiPath(DestPartialFullName)))
          {
            FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
              FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestPartialFullName), "",
            [&]()
            {
              THROWOSIFFALSE(Sysutils::RemoveFile(ApiPath(DestPartialFullName)));
            });
            __removed FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (DestPartialFullName)));
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
          LocalFileHandle = NULL;
        }
      }
      else
      {
        // is NULL when overwriting read-only file, so following will
        // probably fail anyway
        if (LocalFileHandle == NULL)
        {
          FTerminal->TerminalOpenLocalFile(DestFullName, GENERIC_WRITE, NULL, &LocalFileHandle, NULL, NULL, NULL, NULL);
        }
        ResumeAllowed = false;
        FileSeek((THandle)LocalFileHandle, DestFileSize, 0);
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

    // if not already opened (resume, append...), create new empty file
    if (!LocalFileHandle)
    {
      if (!FTerminal->TerminalCreateLocalFile(LocalFileName, OperationProgress,
             FLAGSET(AParams, cpResume),
             FLAGSET(AParams, cpNoConfirmation),
             &LocalFileHandle))
      {
        throw ESkipFile();
      }
    }
    DebugAssert(LocalFileHandle);

    DeleteLocalFile = true;

    FileStream = new TSafeHandleStream((THandle)LocalFileHandle);

    // at end of this block queue is discarded
    {
      TSFTPDownloadQueue Queue(this, FCodePage);
      try__finally
      {
        SCOPE_EXIT
        {
          Queue.DisposeSafe();
        };
        TSFTPPacket DataPacket(FCodePage);

        intptr_t QueueLen = intptr_t(AFile->GetSize() / DownloadBlockSize(OperationProgress)) + 1;
        if ((QueueLen > GetSessionData()->GetSFTPDownloadQueue()) ||
            (QueueLen < 0))
        {
          QueueLen = FTerminal->GetSessionData()->GetSFTPDownloadQueue();
        }
        if (QueueLen < 1)
        {
          QueueLen = 1;
        }
        Queue.Init(QueueLen, RemoteHandle, OperationProgress->GetTransferredSize(), OperationProgress);

        bool Eof = false;
        bool PrevIncomplete = false;
        intptr_t GapFillCount = 0;
        intptr_t GapCount = 0;
        uint32_t Missing = 0;
        uint32_t DataLen = 0;
        uintptr_t BlockSize;
        bool ConvertToken = false;

        while (!Eof)
        {
          if (Missing > 0)
          {
            Queue.InitFillGapRequest(OperationProgress->GetTransferredSize(), Missing, &DataPacket);
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
            SFTPCloseRemote(RemoteHandle, ADestFileName, OperationProgress, true, true, NULL);
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
                L"Received incomplete data packet before end of file, offset: %s, size: %d, requested: %d",
                (::IntToStr(OperationProgress->GetTransferredSize()), int(DataLen), int(BlockSize))));
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
                  Missing = ToUInt32(BlockSize - DataLen);
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

            if (OperationProgress->GetAsciiTransfer())
            {
              DebugAssert(!ResumeTransfer && !ResumeAllowed);

              uintptr_t PrevBlockSize = BlockBuf.GetSize();
              BlockBuf.Convert(GetEOL(), FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
              OperationProgress->SetLocalSize(OperationProgress->GetLocalSize() - PrevBlockSize + BlockBuf.GetSize());
            }

            FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
              FMTLOAD(WRITE_ERROR, LocalFileName), "",
            [&]()
            {
              BlockBuf.WriteToStream(FileStream, BlockBuf.GetSize());
            });
            __removed FILE_OPERATION_LOOP_END(FMTLOAD(WRITE_ERROR, (LocalFileName)));

            OperationProgress->AddLocallyUsed(BlockBuf.GetSize());
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
        };

        if (GapCount > 0)
        {
          FTerminal->LogEvent(FORMAT("%d requests to fill %d data gaps were issued.", GapFillCount, GapCount));
        }
      }
      __finally__removed 
      ({
        Queue.DisposeSafe();
      })
      // queue is discarded here
    }

    if (CopyParam->GetPreserveTime())
    {
      FTerminal->UpdateTargetTime(LocalFileHandle, Modification, FTerminal->GetSessionData()->GetDSTMode());
    }

    SAFE_CLOSE_HANDLE(LocalFileHandle);
    LocalFileHandle = nullptr;

    if (ResumeAllowed)
    {
      FileOperationLoopCustom(FTerminal, OperationProgress, folAllowSkip,
        FMTLOAD(RENAME_AFTER_RESUME_ERROR, base::ExtractFileName(DestPartialFullName, true), ADestFileName),
        "",
      [&]()
      {
        if (FileExists(ApiPath(DestFullName)))
        {
          DeleteFileChecked(DestFullName);
        }
        THROWOSIFFALSE(Sysutils::RenameFile(DestPartialFullName, DestFullName));
      });
      __removed FILE_OPERATION_LOOP_END(FMTLOAD(RENAME_AFTER_RESUME_ERROR, (ExtractFileName(DestPartialFullName), ADestFileName)));
    }

    DeleteLocalFile = false;

    FTerminal->UpdateTargetAttrs(DestFullName, AFile, CopyParam, Attrs);

  }
  __finally__removed
  ({
    if (LocalHandle)
    {
      CloseHandle(LocalFileHandle);
    }

    if (FileStream != NULL)
    {
      delete FileStream;
    }

    if (DeleteLocalFile && (!ResumeAllowed || OperationProgress->LocallyUsed == 0) &&
        (OverwriteMode == omOverwrite))
    {
      FILE_OPERATION_LOOP_BEGIN
      {
        THROWOSIFFALSE(Sysutils::DeleteFile(ApiPath(LocalFileName)));
      }
      FILE_OPERATION_LOOP_END(FMTLOAD(DELETE_LOCAL_FILE_ERROR, (LocalFileName)));
    }

    // if the transfer was finished, the file is closed already
    if (FTerminal->Active && !RemoteHandle.IsEmpty())
    {
      // do not wait for response
      SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress, true, true, NULL);
    }
  })
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RegisterChecksumAlg(const UnicodeString Alg, const UnicodeString SftpAlg)
{
  FChecksumAlgs->Add(Alg);
  FChecksumSftpAlgs->Add(SftpAlg);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::GetSupportedChecksumAlgs(TStrings *Algs)
{
  Algs->AddStrings(FChecksumAlgs.get());
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::LockFile(const UnicodeString /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::UnlockFile(const UnicodeString /*FileName*/, const TRemoteFile * /*File*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::UpdateFromMain(TCustomFileSystem * /*MainFileSystem*/)
{
  // noop
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ClearCaches()
{
  // noop
}

