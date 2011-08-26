//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "SftpFileSystem.h"

#include "PuttyTools.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "SecureShell.h"

#include <memory>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
//---------------------------------------------------------------------------
#define SSH_FX_OK                                 0
#define SSH_FX_EOF                                1
#define SSH_FX_NO_SUCH_FILE                       2
#define SSH_FX_PERMISSION_DENIED                  3
#define SSH_FX_OP_UNSUPPORTED                     8

#define SSH_FXP_INIT               1
#define SSH_FXP_VERSION            2
#define SSH_FXP_OPEN               3
#define SSH_FXP_CLOSE              4
#define SSH_FXP_READ               5
#define SSH_FXP_WRITE              6
#define SSH_FXP_LSTAT              7
#define SSH_FXP_FSTAT              8
#define SSH_FXP_SETSTAT            9
#define SSH_FXP_FSETSTAT           10
#define SSH_FXP_OPENDIR            11
#define SSH_FXP_READDIR            12
#define SSH_FXP_REMOVE             13
#define SSH_FXP_MKDIR              14
#define SSH_FXP_RMDIR              15
#define SSH_FXP_REALPATH           16
#define SSH_FXP_STAT               17
#define SSH_FXP_RENAME             18
#define SSH_FXP_READLINK           19
#define SSH_FXP_SYMLINK            20
#define SSH_FXP_STATUS             101
#define SSH_FXP_HANDLE             102
#define SSH_FXP_DATA               103
#define SSH_FXP_NAME               104
#define SSH_FXP_ATTRS              105
#define SSH_FXP_EXTENDED           200
#define SSH_FXP_EXTENDED_REPLY     201
#define SSH_FXP_ATTRS              105

#define SSH_FILEXFER_ATTR_SIZE              0x00000001
#define SSH_FILEXFER_ATTR_UIDGID            0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS       0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME         0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED          0x80000000
#define SSH_FILEXFER_ATTR_ACCESSTIME        0x00000008
#define SSH_FILEXFER_ATTR_CREATETIME        0x00000010
#define SSH_FILEXFER_ATTR_MODIFYTIME        0x00000020
#define SSH_FILEXFER_ATTR_ACL               0x00000040
#define SSH_FILEXFER_ATTR_OWNERGROUP        0x00000080
#define SSH_FILEXFER_ATTR_SUBSECOND_TIMES   0x00000100
#define SSH_FILEXFER_ATTR_BITS              0x00000200
#define SSH_FILEXFER_ATTR_EXTENDED          0x80000000

#define SSH_FILEXFER_ATTR_COMMON \
  (SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_OWNERGROUP | \
   SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACCESSTIME | \
   SSH_FILEXFER_ATTR_MODIFYTIME)

#define SSH_FILEXFER_TYPE_REGULAR          1
#define SSH_FILEXFER_TYPE_DIRECTORY        2
#define SSH_FILEXFER_TYPE_SYMLINK          3
#define SSH_FILEXFER_TYPE_SPECIAL          4
#define SSH_FILEXFER_TYPE_UNKNOWN          5

#define SSH_FXF_READ            0x00000001
#define SSH_FXF_WRITE           0x00000002
#define SSH_FXF_APPEND          0x00000004
#define SSH_FXF_CREAT           0x00000008
#define SSH_FXF_TRUNC           0x00000010
#define SSH_FXF_EXCL            0x00000020
#define SSH_FXF_TEXT            0x00000040

#define SSH_FXF_ACCESS_DISPOSITION        0x00000007
#define     SSH_FXF_CREATE_NEW            0x00000000
#define     SSH_FXF_CREATE_TRUNCATE       0x00000001
#define     SSH_FXF_OPEN_EXISTING         0x00000002
#define     SSH_FXF_OPEN_OR_CREATE        0x00000003
#define     SSH_FXF_TRUNCATE_EXISTING     0x00000004
#define SSH_FXF_ACCESS_APPEND_DATA        0x00000008
#define SSH_FXF_ACCESS_APPEND_DATA_ATOMIC 0x00000010
#define SSH_FXF_ACCESS_TEXT_MODE          0x00000020
#define SSH_FXF_ACCESS_READ_LOCK          0x00000040
#define SSH_FXF_ACCESS_WRITE_LOCK         0x00000080
#define SSH_FXF_ACCESS_DELETE_LOCK        0x00000100

#define ACE4_READ_DATA         0x00000001
#define ACE4_LIST_DIRECTORY    0x00000001
#define ACE4_WRITE_DATA        0x00000002
#define ACE4_ADD_FILE          0x00000002
#define ACE4_APPEND_DATA       0x00000004
#define ACE4_ADD_SUBDIRECTORY  0x00000004
#define ACE4_READ_NAMED_ATTRS  0x00000008
#define ACE4_WRITE_NAMED_ATTRS 0x00000010
#define ACE4_EXECUTE           0x00000020
#define ACE4_DELETE_CHILD      0x00000040
#define ACE4_READ_ATTRIBUTES   0x00000080
#define ACE4_WRITE_ATTRIBUTES  0x00000100
#define ACE4_DELETE            0x00010000
#define ACE4_READ_ACL          0x00020000
#define ACE4_WRITE_ACL         0x00040000
#define ACE4_WRITE_OWNER       0x00080000
#define ACE4_SYNCHRONIZE       0x00100000

#define SSH_FILEXFER_ATTR_FLAGS_HIDDEN           0x00000004

#define SFTP_MAX_PACKET_LEN   1024000
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
//---------------------------------------------------------------------------
#define OGQ_LIST_OWNERS 0x01
#define OGQ_LIST_GROUPS 0x02
//---------------------------------------------------------------------------
const int SFTPMinVersion = 0;
const int SFTPMaxVersion = 5;
const int SFTPNoMessageNumber = -1;

const int asNo =            0;
const int asOK =            1 << SSH_FX_OK;
const int asEOF =           1 << SSH_FX_EOF;
const int asPermDenied =    1 << SSH_FX_PERMISSION_DENIED;
const int asOpUnsupported = 1 << SSH_FX_OP_UNSUPPORTED;
const int asNoSuchFile =    1 << SSH_FX_NO_SUCH_FILE;
const int asAll = 0xFFFF;

const int tfFirstLevel =   0x01;
const int tfNewDirectory = 0x02;
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
//---------------------------------------------------------------------------
#define SFTP_PACKET_ALLOC_DELTA 256
//---------------------------------------------------------------------------
#pragma warn -inl
//---------------------------------------------------------------------------
struct TSFTPSupport
{
  TSFTPSupport() :
    AttribExtensions(new TStringList()),
    Extensions(new TStringList())
  {
    Reset();
  }

  ~TSFTPSupport()
  {
    delete AttribExtensions;
    delete Extensions;
  }

  void Reset()
  {
    AttributeMask = 0;
    AttributeBits = 0;
    OpenFlags = 0;
    AccessMask = 0;
    MaxReadSize = 0;
    OpenBlockMasks = 0;
    BlockMasks = 0;
    AttribExtensions->Clear();
    Extensions->Clear();
    Loaded = false;
  }

  unsigned int AttributeMask;
  unsigned int AttributeBits;
  unsigned int OpenFlags;
  unsigned int AccessMask;
  unsigned int MaxReadSize;
  unsigned int OpenBlockMasks;
  unsigned int BlockMasks;
  TStrings * AttribExtensions;
  TStrings * Extensions;
  bool Loaded;
};
//---------------------------------------------------------------------------
class TSFTPPacket
{
public:
  TSFTPPacket()
  {
    Init();
  }

  TSFTPPacket(const TSFTPPacket & Source)
  {
    Init();
    *this = Source;
  }

  TSFTPPacket(unsigned char AType)
  {
    Init();
    ChangeType(AType);
  }

  TSFTPPacket(const char * Source, unsigned int Len)
  {
    Init();
    FLength = Len;
    Capacity = FLength;
    memcpy(Data, Source, Len);
  }

  TSFTPPacket(const std::wstring & Source)
  {
    Init();
    FLength = Source.Length();
    Capacity = FLength;
    memcpy(Data, Source.c_str(), Source.Length());
  }

  ~TSFTPPacket()
  {
    if (FData != NULL)
    {
      delete[] (FData - FSendPrefixLen);
    }
    if (FReservedBy) FReservedBy->UnreserveResponse(this);
  }

  void ChangeType(unsigned char AType)
  {
    FPosition = 0;
    FLength = 0;
    Capacity = 0;
    FType = AType;
    AddByte(FType);
    if ((FType != 1) && (FType != SSH_FXP_INIT))
    {
      AssignNumber();
      AddCardinal(FMessageNumber);
    }
  }

  void Reuse()
  {
    AssignNumber();

    assert(Length >= 5);

    // duplicated in AddCardinal()
    unsigned char Buf[4];
    PUT_32BIT(Buf, FMessageNumber);

    memcpy(FData + 1, Buf, sizeof(Buf));
  }

  void AddByte(unsigned char Value)
  {
    Add(&Value, sizeof(Value));
  }

  void AddCardinal(unsigned long Value)
  {
    // duplicated in Reuse()
    unsigned char Buf[4];
    PUT_32BIT(Buf, Value);
    Add(&Buf, sizeof(Buf));
  }

  void AddInt64(__int64 Value)
  {
    AddCardinal((unsigned long)(Value >> 32));
    AddCardinal((unsigned long)(Value & 0xFFFFFFFF));
  }

  void AddData(const void * Data, int ALength)
  {
    AddCardinal(ALength);
    Add(Data, ALength);
  }

  void AddString(const std::wstring Value)
  {
    AddCardinal(Value.Length());
    Add(Value.c_str(), Value.Length());
  }

  inline void AddUtfString(const std::wstring Value)
  {
    AddString(EncodeUTF(Value));
  }

  inline void AddString(const std::wstring Value, bool Utf)
  {
    if (Utf)
    {
      AddUtfString(Value);
    }
    else
    {
      AddString(Value);
    }
  }

  // now purposeless alias to AddString
  inline void AddPathString(const std::wstring Value, bool Utf)
  {
    AddString(Value, Utf);
  }

  void AddProperties(unsigned short * Rights, TRemoteToken * Owner,
    TRemoteToken * Group, __int64 * MTime, __int64 * ATime,
    __int64 * Size, bool IsDirectory, int Version, bool Utf)
  {
    int Flags = 0;
    if (Size != NULL)
    {
      Flags |= SSH_FILEXFER_ATTR_SIZE;
    }
    // both or neither
    assert((Owner != NULL) == (Group != NULL));
    if ((Owner != NULL) && (Group != NULL))
    {
      if (Version < 4)
      {
        assert(Owner->IDValid && Group->IDValid);
        Flags |= SSH_FILEXFER_ATTR_UIDGID;
      }
      else
      {
        assert(Owner->NameValid && Group->NameValid);
        Flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
      }
    }
    if (Rights != NULL)
    {
      Flags |= SSH_FILEXFER_ATTR_PERMISSIONS;
    }
    if ((Version < 4) && ((MTime != NULL) || (ATime != NULL)))
    {
      Flags |= SSH_FILEXFER_ATTR_ACMODTIME;
    }
    if ((Version >= 4) && (ATime != NULL))
    {
      Flags |= SSH_FILEXFER_ATTR_ACCESSTIME;
    }
    if ((Version >= 4) && (MTime != NULL))
    {
      Flags |= SSH_FILEXFER_ATTR_MODIFYTIME;
    }
    AddCardinal(Flags);

    if (Version >= 4)
    {
      AddByte(static_cast<unsigned char>(IsDirectory ?
        SSH_FILEXFER_TYPE_DIRECTORY : SSH_FILEXFER_TYPE_REGULAR));
    }

    if (Size != NULL)
    {
      AddInt64(*Size);
    }

    if ((Owner != NULL) && (Group != NULL))
    {
      if (Version < 4)
      {
        assert(Owner->IDValid && Group->IDValid);
        AddCardinal(Owner->ID);
        AddCardinal(Group->ID);
      }
      else
      {
        assert(Owner->NameValid && Group->NameValid);
        AddString(Owner->Name, Utf);
        AddString(Group->Name, Utf);
      }
    }

    if (Rights != NULL)
    {
      AddCardinal(*Rights);
    }

    if ((Version < 4) && ((MTime != NULL) || (ATime != NULL)))
    {
      // any way to reflect sbSignedTS here?
      // (note that casting __int64 > 2^31 < 2^32 to unsigned long is wrapped,
      // thus we never can set time after 2038, even if the server supports it)
      AddCardinal(static_cast<unsigned long>(ATime != NULL ? *ATime : *MTime));
      AddCardinal(static_cast<unsigned long>(MTime != NULL ? *MTime : *ATime));
    }
    if ((Version >= 4) && (ATime != NULL))
    {
      AddInt64(*ATime);
    }
    if ((Version >= 4) && (MTime != NULL))
    {
      AddInt64(*MTime);
    }
  }

  void AddProperties(const TRemoteProperties * Properties,
    unsigned short BaseRights, bool IsDirectory, int Version, bool Utf,
    TChmodSessionAction * Action)
  {
    enum { valNone = 0, valRights = 0x01, valOwner = 0x02, valGroup = 0x04,
      valMTime = 0x08, valATime = 0x10 } Valid = valNone;
    unsigned short RightsNum = 0;
    TRemoteToken Owner;
    TRemoteToken Group;
    __int64 MTime;
    __int64 ATime;

    if (Properties != NULL)
    {
      if (Properties->Valid.Contains(vpGroup))
      {
        Valid |= valGroup;
        Group = Properties->Group;
      }

      if (Properties->Valid.Contains(vpOwner))
      {
        Valid |= valOwner;
        Owner = Properties->Owner;
      }

      if (Properties->Valid.Contains(vpRights))
      {
        Valid |= valRights;
        TRights Rights = BaseRights;
        Rights |= Properties->Rights.NumberSet;
        Rights &= (unsigned short)~Properties->Rights.NumberUnset;
        if (IsDirectory && Properties->AddXToDirectories)
        {
          Rights.AddExecute();
        }
        RightsNum = Rights;

        if (Action != NULL)
        {
          Action->Rights(Rights);
        }
      }

      if (Properties->Valid.Contains(vpLastAccess))
      {
        Valid |= valATime;
        ATime = Properties->LastAccess;
      }

      if (Properties->Valid.Contains(vpModification))
      {
        Valid |= valMTime;
        MTime = Properties->Modification;
      }
    }

    AddProperties(
      Valid & valRights ? &RightsNum : NULL,
      Valid & valOwner ? &Owner : NULL,
      Valid & valGroup ? &Group : NULL,
      Valid & valMTime ? &MTime : NULL,
      Valid & valATime ? &ATime : NULL,
      NULL, IsDirectory, Version, Utf);
  }

  char GetByte()
  {
    Need(sizeof(char));
    char Result = FData[FPosition];
    FPosition++;
    return Result;
  }

  unsigned long GetCardinal()
  {
    unsigned long Result;
    Need(sizeof(Result));
    Result = GET_32BIT(FData + FPosition);
    FPosition += sizeof(Result);
    return Result;
  }

  unsigned long GetSmallCardinal()
  {
    unsigned long Result;
    Need(2);
    Result = (FData[FPosition] << 8) + FData[FPosition + 1];
    FPosition += 2;
    return Result;
  }

  __int64 GetInt64()
  {
    __int64 Hi = GetCardinal();
    __int64 Lo = GetCardinal();
    return (Hi << 32) + Lo;
  }

  std::wstring GetString()
  {
    std::wstring Result;
    unsigned long Len = GetCardinal();
    Need(Len);
    // cannot happen anyway as Need() would raise exception
    assert(Len < SFTP_MAX_PACKET_LEN);
    Result.SetLength(Len);
    memcpy(Result.c_str(), FData + FPosition, Len);
    FPosition += Len;
    return Result;
  }

  inline std::wstring GetUtfString()
  {
    std::wstring Result = DecodeUTF(GetString());
    return Result;
  }

  inline std::wstring GetString(bool Utf)
  {
    if (Utf)
    {
      return GetUtfString();
    }
    else
    {
      return GetString();
    }
  }

  // now purposeless alias to GetString
  inline std::wstring GetPathString(bool Utf)
  {
    return GetString(Utf);
  }

  void GetFile(TRemoteFile * File, int Version, TDSTMode DSTMode, bool Utf, bool SignedTS, bool Complete)
  {
    assert(File);
    unsigned int Flags;
    std::wstring ListingStr;
    unsigned long Permissions = 0;
    bool ParsingFailed = false;
    if (Type != SSH_FXP_ATTRS)
    {
      File->FileName = GetPathString(Utf);
      if (Version < 4)
      {
        ListingStr = GetString();
      }
    }
    Flags = GetCardinal();
    if (Version >= 4)
    {
      char FXType = GetByte();
      // -:regular, D:directory, L:symlink, S:special, U:unknown
      // O:socket, C:char devide, B:block device, F:fifo

      // SSH-2.0-cryptlib returns file type 0 in response to SSH_FXP_LSTAT,
      // handle this undefined value as "unknown"
      static char* Types = "U-DLSUOCBF";
      if (FXType < 0 || FXType > (char)strlen(Types))
      {
        throw std::exception(FMTLOAD(SFTP_UNKNOWN_FILE_TYPE, (int(FXType))));
      }
      File->Type = Types[FXType];
    }
    if (Flags & SSH_FILEXFER_ATTR_SIZE)
    {
      File->Size = GetInt64();
    }
    // SSH-2.0-3.2.0 F-SECURE SSH - Process Software MultiNet
    // sets SSH_FILEXFER_ATTR_UIDGID for v4, but does not include the UID/GUID
    if ((Flags & SSH_FILEXFER_ATTR_UIDGID) && (Version < 4))
    {
      File->Owner.ID = GetCardinal();
      File->Group.ID = GetCardinal();
    }
    if (Flags & SSH_FILEXFER_ATTR_OWNERGROUP)
    {
      assert(Version >= 4);
      File->Owner.Name = GetString(Utf);
      File->Group.Name = GetString(Utf);
    }
    if (Flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      Permissions = GetCardinal();
    }
    if (Version < 4)
    {
      if (Flags & SSH_FILEXFER_ATTR_ACMODTIME)
      {
        File->LastAccess = UnixToDateTime(
          SignedTS ?
            static_cast<__int64>(static_cast<signed long>(GetCardinal())) :
            static_cast<__int64>(GetCardinal()),
          DSTMode);
        File->Modification = UnixToDateTime(
          SignedTS ?
            static_cast<__int64>(static_cast<signed long>(GetCardinal())) :
            static_cast<__int64>(GetCardinal()),
          DSTMode);
      }
    }
    else
    {
      if (Flags & SSH_FILEXFER_ATTR_ACCESSTIME)
      {
        File->LastAccess = UnixToDateTime(GetInt64(), DSTMode);
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip access time subseconds
        }
      }
      else
      {
        File->LastAccess = Now();
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
        File->Modification = UnixToDateTime(GetInt64(), DSTMode);
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip modification time subseconds
        }
      }
      else
      {
        File->Modification = Now();
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_ACL)
    {
      GetString();
    }

    if (Flags & SSH_FILEXFER_ATTR_BITS)
    {
      // while SSH_FILEXFER_ATTR_BITS is defined for SFTP5 only, vandyke 2.3.3 sets it
      // for SFTP4 as well
      unsigned long Bits = GetCardinal();
      if (FLAGSET(Bits, SSH_FILEXFER_ATTR_FLAGS_HIDDEN))
      {
        File->IsHidden = true;
      }
    }

    if ((Version < 4) && (Type != SSH_FXP_ATTRS))
    {
      try
      {
        // update permissions and user/group name
        // modification time and filename is ignored
        File->ListingStr = ListingStr;
      }
      catch(...)
      {
        // ignore any error while parsing listing line,
        // SFTP specification do not recommend to parse it
        ParsingFailed = true;
      }
    }

    if (Type == SSH_FXP_ATTRS || Version >= 4 || ParsingFailed)
    {
      Char Type = '-';
      if (FLAGSET(Flags, SSH_FILEXFER_ATTR_PERMISSIONS))
      {
        File->Rights->Number = (unsigned short)(Permissions & TRights::rfAllSpecials);
        if (FLAGSET(Permissions, TRights::rfDirectory))
        {
          Type = FILETYPE_DIRECTORY;
        }
      }

      if (Version < 4)
      {
        File->Type = Type;
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_EXTENDED)
    {
      unsigned int ExtendedCount = GetCardinal();
      for (unsigned int Index = 0; Index < ExtendedCount; Index++)
      {
        GetString(); // skip extended_type
        GetString(); // skip extended_data
      }
    }

    if (Complete)
    {
      File->Complete();
    }
  }

  char * GetNextData(unsigned int Size = 0)
  {
    if (Size > 0)
    {
      Need(Size);
    }
    return FPosition < FLength ? FData + FPosition : NULL;
  }

  void DataUpdated(int ALength)
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

  void LoadFromFile(const std::wstring FileName)
  {
    TStringList * DumpLines = new TStringList();
    std::wstring Dump;
    try
    {
      DumpLines->LoadFromFile(FileName);
      Dump = DumpLines->Text;
    }
    __finally
    {
      delete DumpLines;
    }

    Capacity = 20480;
    char Byte[3];
    memset(Byte, '\0', sizeof(Byte));
    int Index = 1;
    unsigned int Length = 0;
    while (Index < Dump.Length())
    {
      char C = Dump[Index];
      if (((C >= '0') && (C <= '9')) || ((C >= 'A') && (C <= 'Z')))
      {
        if (Byte[0] == '\0')
        {
          Byte[0] = C;
        }
        else
        {
          Byte[1] = C;
          assert(Length < Capacity);
          Data[Length] = static_cast<unsigned char>(HexToInt(Byte));
          Length++;
          memset(Byte, '\0', sizeof(Byte));
        }
      }
      Index++;
    }
    DataUpdated(Length);
  }

  std::wstring Dump() const
  {
    std::wstring Result;
    for (unsigned int Index = 0; Index < Length; Index++)
    {
      Result += CharToHex(Data[Index]) + ",";
      if (((Index + 1) % 25) == 0)
      {
        Result += "\n";
      }
    }
    return Result;
  }

  TSFTPPacket & operator = (const TSFTPPacket & Source)
  {
    Capacity = 0;
    Add(Source.Data, Source.Length);
    DataUpdated(Source.Length);
    FPosition = Source.FPosition;
    return *this;
  }

  __property unsigned int Length = { read = FLength };
  __property unsigned int RemainingLength = { read = GetRemainingLength };
  __property char * Data = { read = FData };
  __property char * SendData = { read = GetSendData };
  __property unsigned int SendLength = { read = GetSendLength };
  __property unsigned int Capacity = { read = FCapacity, write = SetCapacity };
  __property unsigned char Type = { read = FType };
  __property unsigned char RequestType = { read = GetRequestType };
  __property unsigned int MessageNumber = { read = FMessageNumber, write = FMessageNumber };
  __property TSFTPFileSystem * ReservedBy = { read = FReservedBy, write = FReservedBy };
  __property std::wstring TypeName = { read = GetTypeName };

private:
  char * FData;
  unsigned int FLength;
  unsigned int FCapacity;
  unsigned int FPosition;
  unsigned char FType;
  unsigned int FMessageNumber;
  TSFTPFileSystem * FReservedBy;

  static int FMessageCounter;
  static const FSendPrefixLen = 4;

  void Init()
  {
    FData = NULL;
    FCapacity = 0;
    FLength = 0;
    FPosition = 0;
    FMessageNumber = SFTPNoMessageNumber;
    FType = -1;
    FReservedBy = NULL;
  }

  void AssignNumber()
  {
    // this is not strictly thread-safe, but as it is accessed from multiple
    // threads only for multiple connection, it is not problem if two threads get
    // the same number
    FMessageNumber = (FMessageCounter << 8) + FType;
    FMessageCounter++;
  }

  unsigned char GetRequestType()
  {
    if (FMessageNumber != SFTPNoMessageNumber)
    {
      return (unsigned char)(FMessageNumber & 0xFF);
    }
    else
    {
      assert(Type == SSH_FXP_VERSION);
      return SSH_FXP_INIT;
    }
  }

  inline void Add(const void * AData, int ALength)
  {
    if (Length + ALength > Capacity)
    {
      Capacity = Length + ALength + SFTP_PACKET_ALLOC_DELTA;
    }
    memcpy(FData + Length, AData, ALength);
    FLength += ALength;
  }

  void SetCapacity(unsigned int ACapacity)
  {
    if (ACapacity != Capacity)
    {
      FCapacity = ACapacity;
      if (FCapacity > 0)
      {
        char * NData = (new char[FCapacity + FSendPrefixLen]) + FSendPrefixLen;
        if (FData)
        {
          memcpy(NData - FSendPrefixLen, FData - FSendPrefixLen,
            (FLength < FCapacity ? FLength : FCapacity) + FSendPrefixLen);
          delete[] (FData - FSendPrefixLen);
        }
        FData = NData;
      }
      else
      {
        if (FData) delete[] (FData - FSendPrefixLen);
        FData = NULL;
      }
      if (FLength > FCapacity) FLength = FCapacity;
    }
  }

  std::wstring GetTypeName() const
  {
    #define TYPE_CASE(TYPE) case TYPE: return #TYPE
    switch (Type) {
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
      TYPE_CASE(SSH_FXP_STATUS);
      TYPE_CASE(SSH_FXP_HANDLE);
      TYPE_CASE(SSH_FXP_DATA);
      TYPE_CASE(SSH_FXP_NAME);
      TYPE_CASE(SSH_FXP_ATTRS);
      TYPE_CASE(SSH_FXP_EXTENDED);
      TYPE_CASE(SSH_FXP_EXTENDED_REPLY);
      default:
        return FORMAT("Unknown message (%d)", (int(Type)));
    }
  }

  char * GetSendData() const
  {
    char * Result = FData - FSendPrefixLen;
    // this is not strictly const-object operation
    PUT_32BIT(Result, Length);
    return Result;
  }

  unsigned int GetSendLength() const
  {
    return FSendPrefixLen + Length;
  }

  unsigned int GetRemainingLength() const
  {
    return Length - FPosition;
  }

  inline void Need(unsigned int Size)
  {
    if (FPosition + Size > FLength)
    {
      throw std::exception(FMTLOAD(SFTP_PACKET_ERROR, (int(FPosition), int(Size), int(FLength))));
    }
  }
};
//---------------------------------------------------------------------------
int TSFTPPacket::FMessageCounter = 0;
//---------------------------------------------------------------------------
class TSFTPQueue
{
public:
  TSFTPQueue(TSFTPFileSystem * AFileSystem)
  {
    FFileSystem = AFileSystem;
    assert(FFileSystem);
    FRequests = new TList();
    FResponses = new TList();
  }

  ~TSFTPQueue()
  {
    TSFTPQueuePacket * Request;
    TSFTPPacket * Response;

    assert(FResponses->GetCount() == FRequests->GetCount());
    for (int Index = 0; Index < FRequests->GetCount(); Index++)
    {
      Request = static_cast<TSFTPQueuePacket*>(FRequests->GetItem(Index));
      assert(Request);
      delete Request;

      Response = static_cast<TSFTPPacket*>(FResponses->GetItem(Index));
      assert(Response);
      delete Response;
    }
    delete FRequests;
    delete FResponses;
  }

  bool Init()
  {
    return SendRequests();
  }

  virtual void Dispose()
  {
    assert(FFileSystem->FTerminal->GetActive());

    TSFTPQueuePacket * Request;
    TSFTPPacket * Response;

    while (FRequests->GetCount())
    {
      assert(FResponses->GetCount());

      Request = static_cast<TSFTPQueuePacket*>(FRequests->Items[0]);
      assert(Request);

      Response = static_cast<TSFTPPacket*>(FResponses->Items[0]);
      assert(Response);

      try
      {
        FFileSystem->ReceiveResponse(Request, Response);
      }
      catch(std::exception & E)
      {
        if (FFileSystem->FTerminal->GetActive())
        {
          FFileSystem->FTerminal->LogEvent("Error while disposing the SFTP queue.");
          FFileSystem->FTerminal->Log->AddException(&E);
        }
        else
        {
          FFileSystem->FTerminal->LogEvent("Fatal error while disposing the SFTP queue.");
          throw;
        }
      }

      FRequests->Delete(0);
      delete Request;
      FResponses->Delete(0);
      delete Response;
    }
  }

  void DisposeSafe()
  {
    if (FFileSystem->FTerminal->GetActive())
    {
      Dispose();
    }
  }

  bool ReceivePacket(TSFTPPacket * Packet,
    int ExpectedType = -1, int AllowStatus = -1, void ** Token = NULL)
  {
    assert(FRequests->GetCount());
    bool Result;
    TSFTPQueuePacket * Request = NULL;
    TSFTPPacket * Response = NULL;
    try
    {
      Request = static_cast<TSFTPQueuePacket*>(FRequests->Items[0]);
      FRequests->Delete(0);
      assert(Request);
      if (Token != NULL)
      {
        *Token = Request->Token;
      }

      Response = static_cast<TSFTPPacket*>(FResponses->Items[0]);
      FResponses->Delete(0);
      assert(Response);

      FFileSystem->ReceiveResponse(Request, Response,
        ExpectedType, AllowStatus);

      if (Packet)
      {
        *Packet = *Response;
      }

      Result = !End(Response);
      if (Result)
      {
        SendRequests();
      }
    }
    __finally
    {
      delete Request;
      delete Response;
    }

    return Result;
  }

  bool Next(int ExpectedType = -1, int AllowStatus = -1)
  {
    return ReceivePacket(NULL, ExpectedType, AllowStatus);
  }

protected:
  TList * FRequests;
  TList * FResponses;
  TSFTPFileSystem * FFileSystem;

  class TSFTPQueuePacket : public TSFTPPacket
  {
  public:
    TSFTPQueuePacket() :
      TSFTPPacket()
    {
      Token = NULL;
    }

    void * Token;
  };

  virtual bool InitRequest(TSFTPQueuePacket * Request) = 0;

  virtual bool End(TSFTPPacket * Response) = 0;

  virtual void SendPacket(TSFTPQueuePacket * Packet)
  {
    FFileSystem->SendPacket(Packet);
  }

  // sends as many requests as allowed by implementation
  virtual bool SendRequests() = 0;

  virtual bool SendRequest()
  {
    TSFTPQueuePacket * Request = NULL;
    try
    {
      Request = new TSFTPQueuePacket();
      if (!InitRequest(Request))
      {
        delete Request;
        Request = NULL;
      }
    }
    catch(...)
    {
      delete Request;
      throw;
    }

    if (Request != NULL)
    {
      TSFTPPacket * Response = new TSFTPPacket();
      FRequests->Add(Request);
      FResponses->Add(Response);

      // make sure the response is reserved before actually ending the message
      // as we may receive response asynchronously before SendPacket finishes
      FFileSystem->ReserveResponse(Request, Response);
      SendPacket(Request);
    }

    return (Request != NULL);
  }
};
//---------------------------------------------------------------------------
class TSFTPFixedLenQueue : public TSFTPQueue
{
public:
  TSFTPFixedLenQueue(TSFTPFileSystem * AFileSystem) : TSFTPQueue(AFileSystem)
  {
    FMissedRequests = 0;
  }

  bool Init(int QueueLen)
  {
    FMissedRequests = QueueLen - 1;
    return TSFTPQueue::Init();
  }

protected:
  int FMissedRequests;

  // sends as many requests as allowed by implementation
  virtual bool SendRequests()
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
  TSFTPAsynchronousQueue(TSFTPFileSystem * AFileSystem) : TSFTPQueue(AFileSystem)
  {
    FFileSystem->FSecureShell->RegisterReceiveHandler(ReceiveHandler);
    FReceiveHandlerRegistered = true;
  }

  virtual ~TSFTPAsynchronousQueue()
  {
    UnregisterReceiveHandler();
  }

  virtual void Dispose()
  {
    // we do not want to receive asynchronous notifications anymore,
    // while waiting synchronously for pending responses
    UnregisterReceiveHandler();
    TSFTPQueue::Dispose();
  }

  bool Continue()
  {
    return SendRequest();
  }

protected:

  // event handler for incoming data
  void ReceiveHandler(TObject * /*Sender*/)
  {
    while (FFileSystem->PeekPacket() && ReceivePacketAsynchronously())
    {
      // loop
    }
  }

  virtual bool ReceivePacketAsynchronously() = 0;

  // sends as many requests as allowed by implementation
  virtual bool SendRequests()
  {
    // noop
    return true;
  }

  void UnregisterReceiveHandler()
  {
    if (FReceiveHandlerRegistered)
    {
      FReceiveHandlerRegistered = false;
      FFileSystem->FSecureShell->UnregisterReceiveHandler(ReceiveHandler);
    }
  }

private:
  bool FReceiveHandlerRegistered;
};
//---------------------------------------------------------------------------
class TSFTPDownloadQueue : public TSFTPFixedLenQueue
{
public:
  TSFTPDownloadQueue(TSFTPFileSystem * AFileSystem) :
    TSFTPFixedLenQueue(AFileSystem)
  {
  }

  bool Init(int QueueLen, const std::wstring AHandle, __int64 ATransfered,
    TFileOperationProgressType * AOperationProgress)
  {
    FHandle = AHandle;
    FTransfered = ATransfered;
    OperationProgress = AOperationProgress;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  void InitFillGapRequest(__int64 Offset, unsigned long Missing,
    TSFTPPacket * Packet)
  {
    InitRequest(Packet, Offset, Missing);
  }

  bool ReceivePacket(TSFTPPacket * Packet, unsigned long & BlockSize)
  {
    void * Token;
    bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_DATA, asEOF, &Token);
    BlockSize = reinterpret_cast<unsigned long>(Token);
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    unsigned int BlockSize = FFileSystem->DownloadBlockSize(OperationProgress);
    InitRequest(Request, FTransfered, BlockSize);
    Request->Token = reinterpret_cast<void*>(BlockSize);
    FTransfered += BlockSize;
    return true;
  }

  void InitRequest(TSFTPPacket * Request, __int64 Offset,
    unsigned long Size)
  {
    Request->ChangeType(SSH_FXP_READ);
    Request->AddString(FHandle);
    Request->AddInt64(Offset);
    Request->AddCardinal(Size);
  }

  virtual bool End(TSFTPPacket * Response)
  {
    return (Response->Type != SSH_FXP_DATA);
  }

private:
  TFileOperationProgressType * OperationProgress;
  __int64 FTransfered;
  std::wstring FHandle;
};
//---------------------------------------------------------------------------
class TSFTPUploadQueue : public TSFTPAsynchronousQueue
{
public:
  TSFTPUploadQueue(TSFTPFileSystem * AFileSystem) :
    TSFTPAsynchronousQueue(AFileSystem)
  {
    FStream = NULL;
    OperationProgress = NULL;
    FLastBlockSize = 0;
    FEnd = false;
    FConvertToken = false;
  }

  virtual ~TSFTPUploadQueue()
  {
    delete FStream;
  }

  bool Init(const std::wstring AFileName,
    HANDLE AFile, TFileOperationProgressType * AOperationProgress,
    const std::wstring AHandle, __int64 ATransfered)
  {
    FFileName = AFileName;
    FStream = new TSafeHandleStream((THandle)AFile);
    OperationProgress = AOperationProgress;
    FHandle = AHandle;
    FTransfered = ATransfered;

    return TSFTPAsynchronousQueue::Init();
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    TTerminal * FTerminal = FFileSystem->FTerminal;
    // Buffer for one block of data
    TFileBuffer BlockBuf;

    unsigned long BlockSize = GetBlockSize();
    bool Result = (BlockSize > 0);

    if (Result)
    {
      FILE_OPERATION_LOOP(FMTLOAD(READ_ERROR, (FFileName)),
        BlockBuf.LoadStream(FStream, BlockSize, false);
      );

      FEnd = (BlockBuf.Size == 0);
      Result = !FEnd;
      if (Result)
      {
        OperationProgress->AddLocalyUsed(BlockBuf.Size);

        // We do ASCII transfer: convert EOL of current block
        if (OperationProgress->AsciiTransfer)
        {
          __int64 PrevBufSize = BlockBuf.Size;
          BlockBuf.Convert(FTerminal->Configuration->LocalEOLType,
            FFileSystem->GetEOL(), cpRemoveCtrlZ | cpRemoveBOM, FConvertToken);
          // update transfer size with difference arised from EOL conversion
          OperationProgress->ChangeTransferSize(OperationProgress->TransferSize -
            PrevBufSize + BlockBuf.Size);
        }

        if (FFileSystem->FTerminal->Configuration->->GetActualLogProtocol() >= 1)
        {
          FFileSystem->FTerminal->LogEvent(FORMAT("Write request offset: %d, len: %d",
            (int(FTransfered), int(BlockBuf.Size))));
        }

        Request->ChangeType(SSH_FXP_WRITE);
        Request->AddString(FHandle);
        Request->AddInt64(FTransfered);
        Request->AddData(BlockBuf.Data, BlockBuf.Size);
        FLastBlockSize = BlockBuf.Size;

        FTransfered += BlockBuf.Size;
      }
    }

    return Result;
  }

  virtual void SendPacket(TSFTPQueuePacket * Packet)
  {
    TSFTPAsynchronousQueue::SendPacket(Packet);
    OperationProgress->AddTransfered(FLastBlockSize);
  }

  virtual bool ReceivePacketAsynchronously()
  {
    // do not read response to close request
    bool Result = (FRequests->GetCount() > 0);
    if (Result)
    {
      ReceivePacket(NULL, SSH_FXP_STATUS);
    }
    return Result;
  }

  inline int GetBlockSize()
  {
    return FFileSystem->UploadBlockSize(FHandle, OperationProgress);
  }

  virtual bool End(TSFTPPacket * /*Response*/)
  {
    return FEnd;
  }

private:
  TStream * FStream;
  TFileOperationProgressType * OperationProgress;
  std::wstring FFileName;
  unsigned long FLastBlockSize;
  bool FEnd;
  __int64 FTransfered;
  std::wstring FHandle;
  bool FConvertToken;
};
//---------------------------------------------------------------------------
class TSFTPLoadFilesPropertiesQueue : public TSFTPFixedLenQueue
{
public:
  TSFTPLoadFilesPropertiesQueue(TSFTPFileSystem * AFileSystem) :
    TSFTPFixedLenQueue(AFileSystem)
  {
    FIndex = 0;
  }

  bool Init(int QueueLen, TStrings * FileList)
  {
    FFileList = FileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool ReceivePacket(TSFTPPacket * Packet, TRemoteFile *& File)
  {
    void * Token;
    bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_ATTRS, asAll, &Token);
    File = static_cast<TRemoteFile *>(Token);
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->GetObject(FIndex]);
      FIndex++;

      bool MissingRights =
        (FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS) &&
           File->Rights->Unknown);
      bool MissingOwnerGroup =
        (FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP) &&
           !File->Owner.IsSet || !File->Group.IsSet);

      Result = (MissingRights || MissingOwnerGroup);
      if (Result)
      {
        Request->ChangeType(SSH_FXP_LSTAT);
        Request->AddPathString(FFileSystem->LocalCanonify(File->FileName),
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

  virtual bool SendRequest()
  {
    bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool End(TSFTPPacket * /*Response*/)
  {
    return (FRequests->GetCount() == 0);
  }

private:
  TStrings * FFileList;
  int FIndex;
};
//---------------------------------------------------------------------------
class TSFTPCalculateFilesChecksumQueue : public TSFTPFixedLenQueue
{
public:
  TSFTPCalculateFilesChecksumQueue(TSFTPFileSystem * AFileSystem) :
    TSFTPFixedLenQueue(AFileSystem)
  {
    FIndex = 0;
  }

  bool Init(int QueueLen, const std::wstring & Alg, TStrings * FileList)
  {
    FAlg = Alg;
    FFileList = FileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool ReceivePacket(TSFTPPacket * Packet, TRemoteFile *& File)
  {
    void * Token;
    bool Result;
    try
    {
      Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_EXTENDED_REPLY, asNo, &Token);
    }
    __finally
    {
      File = static_cast<TRemoteFile *>(Token);
    }
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->GetObject(FIndex]);
      assert(File != NULL);
      FIndex++;

      Result = !File->IsDirectory;
      if (Result)
      {
        assert(!File->IsParentDirectory && !File->IsThisDirectory);

        Request->ChangeType(SSH_FXP_EXTENDED);
        Request->AddString(SFTP_EXT_CHECK_FILE_NAME);
        Request->AddPathString(FFileSystem->LocalCanonify(File->FullFileName),
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

  virtual bool SendRequest()
  {
    bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool End(TSFTPPacket * /*Response*/)
  {
    return (FRequests->GetCount() == 0);
  }

private:
  std::wstring FAlg;
  TStrings * FFileList;
  int FIndex;
};
//---------------------------------------------------------------------------
#pragma warn .inl
//---------------------------------------------------------------------------
class TSFTPBusy
{
public:
  TSFTPBusy(TSFTPFileSystem * FileSystem)
  {
    FFileSystem = FileSystem;
    assert(FFileSystem != NULL);
    FFileSystem->BusyStart();
  }

  ~TSFTPBusy()
  {
    FFileSystem->BusyEnd();
  }

private:
  TSFTPFileSystem * FFileSystem;
};
//===========================================================================
struct TOpenRemoteFileParams
{
  int LocalFileAttrs;
  std::wstring RemoteFileName;
  TFileOperationProgressType * OperationProgress;
  const TCopyParamType * CopyParam;
  int Params;
  bool Resume;
  bool Resuming;
  TSFTPOverwriteMode OverwriteMode;
  __int64 DestFileSize; // output
  std::wstring RemoteFileHandle; // output
  TOverwriteFileParams * FileParams;
  bool Confirmed;
};
//---------------------------------------------------------------------------
struct TSinkFileParams
{
  std::wstring TargetDir;
  const TCopyParamType * CopyParam;
  int Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  unsigned int Flags;
};
//===========================================================================
TSFTPFileSystem::TSFTPFileSystem(TTerminal * ATerminal,
  TSecureShell * SecureShell):
  TCustomFileSystem(ATerminal)
{
  FSecureShell = SecureShell;
  FPacketReservations = new TList();
  FPacketNumbers = VarArrayCreate(OPENARRAY(int, (0, 1)), varLongWord);
  FPreviousLoggedPacket = 0;
  FNotLoggedPackets = 0;
  FBusy = 0;
  FAvoidBusy = false;
  FUtfStrings = false;
  FUtfNever = false;
  FSignedTS = false;
  FSupport = new TSFTPSupport();
  FExtensions = new TStringList();
  FFixedPaths = NULL;
  FFileSystemInfoValid = false;
}
//---------------------------------------------------------------------------
TSFTPFileSystem::~TSFTPFileSystem()
{
  delete FSupport;
  ResetConnection();
  delete FPacketReservations;
  delete FExtensions;
  delete FFixedPaths;
  delete FSecureShell;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::Open()
{
  FSecureShell->Open();
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::Close()
{
  FSecureShell->Close();
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::GetActive()
{
  return FSecureShell->GetActive();
}
//---------------------------------------------------------------------------
const TSessionInfo & TSFTPFileSystem::GetSessionInfo()
{
  return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TSFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  if (!FFileSystemInfoValid)
  {
    FFileSystemInfo.AdditionalInfo = "";

    if (!IsCapable(fcRename))
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(FS_RENAME_NOT_SUPPORTED) + "\r\n\r\n";
    }

    if (FExtensions->GetCount() > 0)
    {
      std::wstring Name;
      std::wstring Value;
      std::wstring Line;
      FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_EXTENSION_INFO) + "\r\n";
      for (int Index = 0; Index < FExtensions->GetCount(); Index++)
      {
        std::wstring Name = FExtensions->Names[Index];
        std::wstring Value = FExtensions->Values[Name];
        std::wstring Line;
        if (Value.IsEmpty())
        {
          Line = Name;
        }
        else
        {
          Line = FORMAT("%s=%s", (Name, DisplayableStr(Value)));
        }
        FFileSystemInfo.AdditionalInfo += FORMAT("  %s\r\n", (Line));
      }
    }
    else
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_NO_EXTENSION_INFO) + "\r\n";
    }

    FFileSystemInfo.ProtocolBaseName = "SFTP";
    FFileSystemInfo.ProtocolName = FMTLOAD(SFTP_PROTOCOL_NAME2, (FVersion));
    for (int Index = 0; Index < fcCount; Index++)
    {
      FFileSystemInfo.IsCapable[Index] = IsCapable((TFSCapability)Index);
    }

    FFileSystemInfoValid = true;
  }

  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::TemporaryTransferFile(const std::wstring & FileName)
{
  return AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::GetStoredCredentialsTried()
{
  return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::GetUserName()
{
  return FSecureShell->UserName;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::Idle()
{
  // Keep session alive
  if ((FTerminal->GetSessionData()->PingType != ptOff) &&
      (Now() - FSecureShell->LastDataSent > FTerminal->GetSessionData()->PingIntervalDT))
  {
    if ((FTerminal->GetSessionData()->PingType == ptDummyCommand) &&
        FSecureShell->Ready)
    {
      TSFTPPacket Packet(SSH_FXP_REALPATH);
      Packet.AddPathString("/", FUtfStrings);
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
void TSFTPFileSystem::ResetConnection()
{
  // there must be no valid packet reservation at the end
  for (int i = 0; i < FPacketReservations->GetCount(); i++)
  {
    assert(FPacketReservations->Items[i] == NULL);
    delete (TSFTPPacket *)FPacketReservations->Items[i];
  }
  FPacketReservations->Clear();
  FPacketNumbers.Clear();
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::IsCapable(int Capability) const
{
  assert(FTerminal);
  switch (Capability) {
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcRemoteCopy:
      return false;

    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
    case fcPreservingTimestampUpload:
    case fcSecondaryShell:
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
        strcmp(GetEOL(), EOLToStr(FTerminal->Configuration->LocalEOLType)) != 0;

    case fcUserGroupListing:
      return SupportsExtension(SFTP_EXT_OWNER_GROUP);

    case fcLoadingAdditionalProperties:
      // we allow loading properties only, if "suported" extension is supported and
      // the server support "permissions" and/or "owner/group" attributes
      // (no other attributes are loaded)
      return FSupport->Loaded &&
        ((FSupport->AttributeMask &
          (SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_OWNERGROUP)) != 0);

    case fcCheckingSpaceAvailable:
      return SupportsExtension(SFTP_EXT_SPACE_AVAILABLE);

    case fcCalculatingChecksum:
      return SupportsExtension(SFTP_EXT_CHECK_FILE);

    default:
      assert(false);
      return false;
  }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::SupportsExtension(const std::wstring & Extension) const
{
  return FSupport->Loaded && (FSupport->Extensions->IndexOf(Extension) >= 0);
}
//---------------------------------------------------------------------------
inline void TSFTPFileSystem::BusyStart()
{
  if (FBusy == 0 && FTerminal->UseBusyCursor && !FAvoidBusy)
  {
    Busy(true);
  }
  FBusy++;
  assert(FBusy < 10);
}
//---------------------------------------------------------------------------
inline void TSFTPFileSystem::BusyEnd()
{
  assert(FBusy > 0);
  FBusy--;
  if (FBusy == 0 && FTerminal->UseBusyCursor && !FAvoidBusy)
  {
    Busy(false);
  }
}
//---------------------------------------------------------------------------
unsigned long TSFTPFileSystem::TransferBlockSize(unsigned long Overhead,
  TFileOperationProgressType * OperationProgress, unsigned long MaxPacketSize)
{
  const unsigned long MinPacketSize = 4096;
  // size + message number + type
  const unsigned long SFTPPacketOverhead = 4 + 4 + 1;
  unsigned long AMaxPacketSize = FSecureShell->MaxPacketSize();
  bool MaxPacketSizeValid = (AMaxPacketSize > 0);
  unsigned long Result = OperationProgress->CPS();

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
    Result = OperationProgress->StaticBlockSize();
  }

  if (Result < MinPacketSize)
  {
    Result = MinPacketSize;
  }

  if (MaxPacketSizeValid)
  {
    Overhead += SFTPPacketOverhead;
    if (AMaxPacketSize < Overhead)
    {
      // do not send another request
      // (generally should happen only if upload buffer if full)
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

  Result = OperationProgress->AdjustToCPSLimit(Result);

  return Result;
}
//---------------------------------------------------------------------------
unsigned long TSFTPFileSystem::UploadBlockSize(const std::wstring & Handle,
  TFileOperationProgressType * OperationProgress)
{
  // handle length + offset + data size
  const unsigned long UploadPacketOverhead =
    sizeof(unsigned long) + sizeof(__int64) + sizeof(unsigned long);
  return TransferBlockSize(UploadPacketOverhead + Handle.Length(), OperationProgress);
}
//---------------------------------------------------------------------------
unsigned long TSFTPFileSystem::DownloadBlockSize(
  TFileOperationProgressType * OperationProgress)
{
  unsigned long Result = TransferBlockSize(sizeof(unsigned long), OperationProgress);
  if (FSupport->Loaded && (FSupport->MaxReadSize > 0) &&
      (Result > FSupport->MaxReadSize))
  {
    Result = FSupport->MaxReadSize;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SendPacket(const TSFTPPacket * Packet)
{
  BusyStart();
  try
  {
    if (FTerminal->Log->Logging)
    {
      if ((FPreviousLoggedPacket != SSH_FXP_READ &&
           FPreviousLoggedPacket != SSH_FXP_WRITE) ||
          (Packet->Type != FPreviousLoggedPacket) ||
          (FTerminal->Configuration->->GetActualLogProtocol() >= 1))
      {
        if (FNotLoggedPackets)
        {
          FTerminal->LogEvent(FORMAT("%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
            (FNotLoggedPackets)));
          FNotLoggedPackets = 0;
        }
        FTerminal->Log->Add(llInput, FORMAT("Type: %s, Size: %d, Number: %d",
          (Packet->TypeName, (int)Packet->Length, (int)Packet->MessageNumber)));
        if (FTerminal->Configuration->->GetActualLogProtocol() >= 2)
        {
          FTerminal->Log->Add(llInput, Packet->Dump());
        }
        FPreviousLoggedPacket = Packet->Type;
      }
      else
      {
        FNotLoggedPackets++;
      }
    }
    FSecureShell->Send(Packet->SendData, Packet->SendLength);
  }
  __finally
  {
    BusyEnd();
  }
}
//---------------------------------------------------------------------------
unsigned long TSFTPFileSystem::GotStatusPacket(TSFTPPacket * Packet,
  int AllowStatus)
{
  unsigned long Code = Packet->GetCardinal();

  static int Messages[] = {
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
    SFTP_STATUS_FILE_CORRUPT
  };
  int Message;
  if ((AllowStatus & (0x01 << Code)) == 0)
  {
    if (Code >= LENOF(Messages))
    {
      Message = SFTP_STATUS_UNKNOWN;
    }
    else
    {
      Message = Messages[Code];
    }
    std::wstring MessageStr = LoadStr(Message);
    std::wstring ServerMessage;
    std::wstring LanguageTag;
    if ((FVersion >= 3) ||
        // if version is not decided yet (i.e. this is status response
        // to the init request), go on only if there are any more data
        ((FVersion < 0) && (Packet->RemainingLength > 0)))
    {
      // message is in UTF only since SFTP specification 01 (specification 00
      // is also version 3)
      // (in other words, always use UTF unless server is know to be buggy)
      ServerMessage = Packet->GetString(!FUtfNever);
      // SSH-2.0-Maverick_SSHD omits the language tag
      // and I believe I've seen one more server doind the same.
      // On the next instance, we should probably already implement workround.
      LanguageTag = Packet->GetString();
      if ((FVersion >= 5) && (Message == SFTP_STATUS_UNKNOWN_PRINCIPAL))
      {
        std::wstring Principals;
        while (Packet->GetNextData() != NULL)
        {
          if (!Principals.IsEmpty())
          {
            Principals += ", ";
          }
          Principals += Packet->GetString();
        }
        MessageStr = FORMAT(MessageStr, (Principals));
      }
    }
    else
    {
      ServerMessage = LoadStr(SFTP_SERVER_MESSAGE_UNSUPPORTED);
    }
    if (FTerminal->Log->Logging)
    {
      FTerminal->Log->Add(llOutput, FORMAT("Status code: %d, Message: %d, Server: %s, Language: %s ",
        (int(Code), (int)Packet->MessageNumber, ServerMessage, LanguageTag)));
    }
    if (!LanguageTag.IsEmpty())
    {
      LanguageTag = FORMAT(" (%s)", (LanguageTag));
    }
    std::wstring Error = FMTLOAD(SFTP_ERROR_FORMAT2, (MessageStr,
      int(Code), LanguageTag, ServerMessage, int(Packet->RequestType)));
    FTerminal->TerminalError(NULL, Error);
    return 0;
  }
  else
  {
    if (!FNotLoggedPackets || Code)
    {
      FTerminal->Log->Add(llOutput, FORMAT("Status code: %d", ((int)Code)));
    }
    return Code;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::RemoveReservation(int Reservation)
{
  for (int Index = Reservation+1; Index < FPacketReservations->GetCount(); Index++)
  {
    FPacketNumbers.PutElement(FPacketNumbers.GetElement(Index), Index-1);
  }
  TSFTPPacket * Packet = (TSFTPPacket *)FPacketReservations->Items[Reservation];
  if (Packet)
  {
    assert(Packet->ReservedBy == this);
    Packet->ReservedBy = NULL;
  }
  FPacketReservations->Delete(Reservation);
}
//---------------------------------------------------------------------------
inline int TSFTPFileSystem::PacketLength(char * LenBuf, int ExpectedType)
{
  int Length = GET_32BIT(LenBuf);
  if (Length > SFTP_MAX_PACKET_LEN)
  {
    std::wstring Message = FMTLOAD(SFTP_PACKET_TOO_BIG, (
      int(Length), SFTP_MAX_PACKET_LEN));
    if (ExpectedType == SSH_FXP_VERSION)
    {
      std::wstring LenString(LenBuf, 4);
      Message = FMTLOAD(SFTP_PACKET_TOO_BIG_INIT_EXPLAIN,
        (Message, DisplayableStr(LenString)));
    }
    FTerminal->FatalError(NULL, Message);
  }
  return Length;
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::PeekPacket()
{
  bool Result;
  char * Buf;
  Result = FSecureShell->Peek(Buf, 4);
  if (Result)
  {
    int Length = PacketLength(Buf, -1);
    Result = FSecureShell->Peek(Buf, 4 + Length);
  }
  return Result;
}
//---------------------------------------------------------------------------
int TSFTPFileSystem::ReceivePacket(TSFTPPacket * Packet,
  int ExpectedType, int AllowStatus)
{
  TSFTPBusy Busy(this);

  int Result = SSH_FX_OK;
  int Reservation = FPacketReservations->IndexOf(Packet);

  if (Reservation < 0 || Packet->Capacity == 0)
  {
    bool IsReserved;
    do
    {
      IsReserved = false;

      assert(Packet);
      char LenBuf[4];
      FSecureShell->Receive(LenBuf, sizeof(LenBuf));
      int Length = PacketLength(LenBuf, ExpectedType);
      Packet->Capacity = Length;
      FSecureShell->Receive(Packet->Data, Length);
      Packet->DataUpdated(Length);

      if (FTerminal->Log->Logging)
      {
        if ((FPreviousLoggedPacket != SSH_FXP_READ &&
             FPreviousLoggedPacket != SSH_FXP_WRITE) ||
            (Packet->Type != SSH_FXP_STATUS && Packet->Type != SSH_FXP_DATA) ||
            (FTerminal->Configuration->->GetActualLogProtocol() >= 1))
        {
          if (FNotLoggedPackets)
          {
            FTerminal->LogEvent(FORMAT("%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
              (FNotLoggedPackets)));
            FNotLoggedPackets = 0;
          }
          FTerminal->Log->Add(llOutput, FORMAT("Type: %s, Size: %d, Number: %d",
            (Packet->TypeName, (int)Packet->Length, (int)Packet->MessageNumber)));
          if (FTerminal->Configuration->->GetActualLogProtocol() >= 2)
          {
            FTerminal->Log->Add(llOutput, Packet->Dump());
          }
        }
        else
        {
          FNotLoggedPackets++;
        }
      }

      if (Reservation < 0 ||
          Packet->MessageNumber != (unsigned int)FPacketNumbers.GetElement(Reservation))
      {
        TSFTPPacket * ReservedPacket;
        unsigned int MessageNumber;
        for (int Index = 0; Index < FPacketReservations->GetCount(); Index++)
        {
          MessageNumber = (unsigned int)FPacketNumbers.GetElement(Index);
          if (MessageNumber == Packet->MessageNumber)
          {
            ReservedPacket = (TSFTPPacket *)FPacketReservations->GetItem(Index);
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
                assert(Reservation == FPacketReservations->IndexOf(Packet));
              }
            }
            break;
          }
        }
      }
    }
    while (IsReserved);
  }

  // before we removed the reservation after check for packet type,
  // but if it raises exception, removal is unnecessarily
  // postponed until the packet is removed
  // (and it have not worked anyway until recent fix to UnreserveResponse)
  if (Reservation >= 0)
  {
    assert(Packet->MessageNumber == (unsigned int)FPacketNumbers.GetElement(Reservation));
    RemoveReservation(Reservation);
  }

  if (ExpectedType >= 0)
  {
    if (Packet->Type == SSH_FXP_STATUS)
    {
      if (AllowStatus < 0)
      {
        AllowStatus = (ExpectedType == SSH_FXP_STATUS ? asOK : asNo);
      }
      Result = GotStatusPacket(Packet, AllowStatus);
    }
    else if (ExpectedType != Packet->Type)
    {
      FTerminal->FatalError(NULL, FMTLOAD(SFTP_INVALID_TYPE, ((int)Packet->Type)));
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReserveResponse(const TSFTPPacket * Packet,
  TSFTPPacket * Response)
{
  if (Response != NULL)
  {
    assert(FPacketReservations->IndexOf(Response) < 0);
    // mark response as not received yet
    Response->Capacity = 0;
    Response->ReservedBy = this;
  }
  FPacketReservations->Add(Response);
  if (FPacketReservations->GetCount() >= FPacketNumbers.ArrayHighBound())
  {
    FPacketNumbers.ArrayRedim(FPacketReservations->GetCount() + 10);
  }
  FPacketNumbers.PutElement(Packet->MessageNumber, FPacketReservations->GetCount() - 1);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::UnreserveResponse(TSFTPPacket * Response)
{
  int Reservation = FPacketReservations->IndexOf(Response);
  if (Response->Capacity != 0)
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
      // we must remember that the respose was expected, so we skip it
      // in receivepacket()
      FPacketReservations->Items[Reservation] = NULL;
    }
  }
}
//---------------------------------------------------------------------------
int TSFTPFileSystem::ReceiveResponse(
  const TSFTPPacket * Packet, TSFTPPacket * Response, int ExpectedType,
  int AllowStatus)
{
  int Result;
  unsigned int MessageNumber = Packet->MessageNumber;
  TSFTPPacket * AResponse = (Response ? Response : new TSFTPPacket());
  try
  {
    Result = ReceivePacket(AResponse, ExpectedType, AllowStatus);
    if (MessageNumber != AResponse->MessageNumber)
    {
      FTerminal->FatalError(NULL, FMTLOAD(SFTP_MESSAGE_NUMBER,
        ((int)AResponse->MessageNumber, (int)MessageNumber)));
    }
  }
  __finally
  {
    if (!Response)
    {
      delete AResponse;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int TSFTPFileSystem::SendPacketAndReceiveResponse(
  const TSFTPPacket * Packet, TSFTPPacket * Response, int ExpectedType,
  int AllowStatus)
{
  int Result;
  TSFTPBusy Busy(this);
  SendPacket(Packet);
  Result = ReceiveResponse(Packet, Response, ExpectedType, AllowStatus);
  return Result;
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::RealPath(const std::wstring Path)
{
  try
  {
    FTerminal->LogEvent(FORMAT("Getting real path for '%s'",
      (Path)));

    TSFTPPacket Packet(SSH_FXP_REALPATH);
    Packet.AddPathString(Path, FUtfStrings);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_NAME);
    if (Packet.GetCardinal() != 1)
    {
      FTerminal->FatalError(NULL, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
    }

    std::wstring RealDir = UnixExcludeTrailingBackslash(Packet.GetPathString(FUtfStrings));
    // ignore rest of SSH_FXP_NAME packet

    FTerminal->LogEvent(FORMAT("Real path is '%s'", (RealDir)));

    return RealDir;
  }
  catch(std::exception & E)
  {
    if (FTerminal->GetActive())
    {
      throw ExtException(&E, FMTLOAD(SFTP_REALPATH_ERROR, (Path)));
    }
    else
    {
      throw;
    }
  }
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::RealPath(const std::wstring Path,
  const std::wstring BaseDir)
{
  std::wstring APath;

  if (TTerminal::IsAbsolutePath(Path))
  {
    APath = Path;
  }
  else
  {
    if (!Path.IsEmpty())
    {
      // this condition/block was outside (before) current block
      // but it dod not work when Path was empty
      if (!BaseDir.IsEmpty())
      {
        APath = UnixIncludeTrailingBackslash(BaseDir);
      }
      APath = APath + Path;
    }
    if (APath.IsEmpty()) APath = UnixIncludeTrailingBackslash(".");
  }
  return RealPath(APath);
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::LocalCanonify(const std::wstring & Path)
{
  // TODO: improve (handle .. etc.)
  if (TTerminal::IsAbsolutePath(Path) ||
      (!FCurrentDirectory.IsEmpty() && UnixComparePaths(FCurrentDirectory, Path)))
  {
    return Path;
  }
  else
  {
    return ::AbsolutePath(FCurrentDirectory, Path);
  }
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::Canonify(std::wstring Path)
{
  // inspired by canonify() from PSFTP.C
  std::wstring Result;
  FTerminal->LogEvent(FORMAT("Canonifying: \"%s\"", (Path)));
  Path = LocalCanonify(Path);
  bool TryParent = false;
  try
  {
    Result = RealPath(Path);
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
    std::wstring APath = UnixExcludeTrailingBackslash(Path);
    std::wstring Name = UnixExtractFileName(APath);
    if (Name == "." || Name == "..")
    {
      Result = Path;
    }
    else
    {
      std::wstring FPath = UnixExtractFilePath(APath);
      try
      {
        Result = RealPath(FPath);
        Result = UnixIncludeTrailingBackslash(Result) + Name;
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

  FTerminal->LogEvent(FORMAT("Canonified: \"%s\"", (Result)));

  return Result;
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::AbsolutePath(std::wstring Path, bool Local)
{
  if (Local)
  {
    return LocalCanonify(Path);
  }
  else
  {
    return RealPath(Path, CurrentDirectory);
  }
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::GetHomeDirectory()
{
  if (FHomeDirectory.IsEmpty())
  {
    FHomeDirectory = RealPath(".");
  }
  return FHomeDirectory;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::LoadFile(TRemoteFile * File, TSFTPPacket * Packet,
  bool Complete)
{
  Packet->GetFile(File, FVersion, FTerminal->GetSessionData()->DSTMode,
    FUtfStrings, FSignedTS, Complete);
}
//---------------------------------------------------------------------------
TRemoteFile * TSFTPFileSystem::LoadFile(TSFTPPacket * Packet,
  TRemoteFile * ALinkedByFile, const std::wstring FileName,
  TRemoteFileList * TempFileList, bool Complete)
{
  TRemoteFile * File = new TRemoteFile(ALinkedByFile);
  try
  {
    File->Terminal = FTerminal;
    if (!FileName.IsEmpty())
    {
      File->FileName = FileName;
    }
    // to get full path for symlink completion
    File->Directory = TempFileList;
    LoadFile(File, Packet, Complete);
    File->Directory = NULL;
  }
  catch(...)
  {
    delete File;
    throw;
  }
  return File;
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DoStartup()
{
  // do not know yet
  FVersion = -1;
  FFileSystemInfoValid = false;
  TSFTPPacket Packet(SSH_FXP_INIT);
  int MaxVersion = FTerminal->GetSessionData()->SFTPMaxVersion;
  if (MaxVersion > SFTPMaxVersion)
  {
    MaxVersion = SFTPMaxVersion;
  }
  Packet.AddCardinal(MaxVersion);

  try
  {
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_VERSION);
  }
  catch(std::exception &E)
  {
    FTerminal->FatalError(&E, LoadStr(SFTP_INITIALIZE_ERROR));
  }

  FVersion = Packet.GetCardinal();
  FTerminal->LogEvent(FORMAT("SFTP version %d negotiated.", (FVersion)));
  if (FVersion < SFTPMinVersion || FVersion > SFTPMaxVersion)
  {
    FTerminal->FatalError(NULL, FMTLOAD(SFTP_VERSION_NOT_SUPPORTED,
      (FVersion, SFTPMinVersion, SFTPMaxVersion)));
  }

  FExtensions->Clear();
  FEOL = "\r\n";
  FSupport->Loaded = false;
  SAFE_DESTROY(FFixedPaths);

  if (FVersion >= 3)
  {
    while (Packet.GetNextData() != NULL)
    {
      std::wstring ExtensionName = Packet.GetString();
      std::wstring ExtensionData = Packet.GetString();
      std::wstring ExtensionDisplayData = DisplayableStr(ExtensionData);

      if (ExtensionName == SFTP_EXT_NEWLINE)
      {
        FEOL = ExtensionData;
        FTerminal->LogEvent(FORMAT("Server requests EOL sequence %s.",
          (ExtensionDisplayData)));
        if (FEOL.Length() < 1 || FEOL.Length() > 2)
        {
          FTerminal->FatalError(NULL, FMTLOAD(SFTP_INVALID_EOL, (ExtensionDisplayData)));
        }
      }
      // do not allow "supported" to override "supported2" if both are received
      else if (((ExtensionName == SFTP_EXT_SUPPORTED) && !FSupport->Loaded) ||
               (ExtensionName == SFTP_EXT_SUPPORTED2))
      {
        FSupport->Reset();
        TSFTPPacket SupportedStruct(ExtensionData);
        FSupport->Loaded = true;
        FSupport->AttributeMask = SupportedStruct.GetCardinal();
        FSupport->AttributeBits = SupportedStruct.GetCardinal();
        FSupport->OpenFlags = SupportedStruct.GetCardinal();
        FSupport->AccessMask = SupportedStruct.GetCardinal();
        FSupport->MaxReadSize = SupportedStruct.GetCardinal();
        if (ExtensionName == SFTP_EXT_SUPPORTED)
        {
          while (SupportedStruct.GetNextData() != NULL)
          {
            FSupport->Extensions->Add(SupportedStruct.GetString());
          }
        }
        else
        {
          FSupport->OpenBlockMasks = SupportedStruct.GetSmallCardinal();
          FSupport->BlockMasks = SupportedStruct.GetSmallCardinal();
          unsigned int ExtensionCount;
          ExtensionCount = SupportedStruct.GetCardinal();
          for (unsigned int i = 0; i < ExtensionCount; i++)
          {
            FSupport->AttribExtensions->Add(SupportedStruct.GetString());
          }
          ExtensionCount = SupportedStruct.GetCardinal();
          for (unsigned int i = 0; i < ExtensionCount; i++)
          {
            FSupport->Extensions->Add(SupportedStruct.GetString());
          }
        }

        if (FTerminal->Log->Logging)
        {
          FTerminal->LogEvent(FORMAT(
            "Server support information:\n"
            "  Attribute mask: %x, Attribute bits: %x, Open flags: %x\n"
            "  Access mask: %x, Open block masks: %x, Block masks: %x, Max read size: %d\n",
            (int(FSupport->AttributeMask),
             int(FSupport->AttributeBits),
             int(FSupport->OpenFlags),
             int(FSupport->AccessMask),
             int(FSupport->OpenBlockMasks),
             int(FSupport->BlockMasks),
             int(FSupport->MaxReadSize))));
          FTerminal->LogEvent(FORMAT(   "  Attribute extensions (%d)\n", (FSupport->AttribExtensions->GetCount())));
          for (int Index = 0; Index < FSupport->AttribExtensions->GetCount(); Index++)
          {
            FTerminal->LogEvent(
              FORMAT("    %s", (FSupport->AttribExtensions->GetString(Index))));
          }
          FTerminal->LogEvent(FORMAT(   "  Extensions (%d)\n", (FSupport->Extensions->GetCount())));
          for (int Index = 0; Index < FSupport->Extensions->GetCount(); Index++)
          {
            FTerminal->LogEvent(
              FORMAT("    %s", (FSupport->Extensions->GetString(Index))));
          }
        }
      }
      else if (ExtensionName == SFTP_EXT_VENDOR_ID)
      {
        TSFTPPacket VendorIdStruct(ExtensionData);
        std::wstring VendorName(VendorIdStruct.GetString());
        std::wstring ProductName(VendorIdStruct.GetString());
        std::wstring ProductVersion(VendorIdStruct.GetString());
        __int64 ProductBuildNumber = VendorIdStruct.GetInt64();
        FTerminal->LogEvent(FORMAT("Server software: %s %s (%d) by %s",
          (ProductName, ProductVersion, int(ProductBuildNumber), VendorName)));
      }
      else if (ExtensionName == SFTP_EXT_FSROOTS)
      {
        FTerminal->LogEvent("File system roots:\n");
        assert(FFixedPaths == NULL);
        FFixedPaths = new TStringList();
        try
        {
          TSFTPPacket RootsPacket(ExtensionData);
          while (RootsPacket.GetNextData() != NULL)
          {
            unsigned long Dummy = RootsPacket.GetCardinal();
            if (Dummy != 1)
            {
              break;
            }
            else
            {
              char Drive = RootsPacket.GetByte();
              char MaybeType = RootsPacket.GetByte();
              FTerminal->LogEvent(FORMAT("  %s: (type %d)", (Drive, (int)MaybeType)));
              FFixedPaths->Add(FORMAT("%s:", (Drive)));
            }
          }
        }
        catch(std::exception & E)
        {
          FFixedPaths->Clear();
          FTerminal->LogEvent(FORMAT("Failed to decode %s extension",
            (SFTP_EXT_FSROOTS)));
          FTerminal->HandleException(&E);
        }
      }
      else if (ExtensionName == SFTP_EXT_VERSIONS)
      {
        try
        {
          // first try legacy decoding according to incorrect encoding
          // (structure-like) as of VShell.
          TSFTPPacket VersionsPacket(ExtensionData);
          std::wstring Versions = VersionsPacket.GetString();
          if (VersionsPacket.GetNextData() != NULL)
          {
            Abort();
          }
          FTerminal->LogEvent(FORMAT("SFTP versions supported by the server (VShell format): %s",
            (Versions)));
        }
        catch(...)
        {
          // if that fails, fallback to proper decoding
          FTerminal->LogEvent(FORMAT("SFTP versions supported by the server: %s",
            (ExtensionData)));
        }
      }
      else
      {
        FTerminal->LogEvent(FORMAT("Unknown server extension %s=%s",
          (ExtensionName, ExtensionDisplayData)));
      }
      FExtensions->Values[ExtensionName] = ExtensionData;
    }

    if (SupportsExtension(SFTP_EXT_VENDOR_ID))
    {
      TSFTPPacket Packet(SSH_FXP_EXTENDED);
      Packet.AddString(SFTP_EXT_VENDOR_ID);
      Packet.AddString(FTerminal->Configuration->CompanyName);
      Packet.AddString(FTerminal->Configuration->ProductName);
      Packet.AddString(FTerminal->Configuration->ProductVersion);
      Packet.AddInt64(LOWORD(FTerminal->Configuration->FixedApplicationInfo->dwFileVersionLS));
      SendPacket(&Packet);
      // we are not interested in the response, do not wait for it
      ReserveResponse(&Packet, NULL);
    }
  }

  if (FVersion < 4)
  {
    // currently enable the bug for all servers (really known on OpenSSH)
    FSignedTS = (FTerminal->GetSessionData()->SFTPBug[sbSignedTS] == asOn) ||
      (FTerminal->GetSessionData()->SFTPBug[sbSignedTS] == asAuto);
    if (FSignedTS)
    {
      FTerminal->LogEvent("We believe the server has signed timestamps bug");
    }
  }
  else
  {
    FSignedTS = false;
  }

  // use UTF when forced or ...
  // when "auto" and version is at least 4 and the server is not know not to use UTF
  FUtfNever = (GetSessionInfo().SshImplementation.Pos("Foxit-WAC-Server") == 1) ||
    (FTerminal->GetSessionData()->NotUtf == asOn);
  FUtfStrings =
    (FTerminal->GetSessionData()->NotUtf == asOff) ||
    ((FTerminal->GetSessionData()->NotUtf == asAuto) &&
      (FVersion >= 4) && !FUtfNever);

  if (FUtfStrings)
  {
    FTerminal->LogEvent("We will use UTF-8 strings when appropriate");
  }
  else if (FUtfNever)
  {
    FTerminal->LogEvent("We will never use UTF-8 strings");
  }
  else
  {
    FTerminal->LogEvent("We will use UTF-8 strings for status messages only");
  }

  FOpenSSH =
    // Sun SSH is based on OpenSSH (suffers the same bugs)
    (GetSessionInfo().SshImplementation.Pos("OpenSSH") == 1) ||
    (GetSessionInfo().SshImplementation.Pos("Sun_SSH") == 1);

  FMaxPacketSize = FTerminal->GetSessionData()->SFTPMaxPacketSize;
  if (FMaxPacketSize == 0)
  {
    if (FOpenSSH && (FVersion == 3) && !FSupport->Loaded)
    {
      FMaxPacketSize = 4 + (256 * 1024); // len + 256kB payload
      FTerminal->LogEvent(FORMAT("Limiting packet size to OpenSSH sftp-server limit of %d bytes",
        (int(FMaxPacketSize))));
    }
    // full string is "1.77 sshlib: Momentum SSH Server",
    // possibly it is sshlib-related
    else if (GetSessionInfo().SshImplementation.Pos("Momentum SSH Server") != 0)
    {
      FMaxPacketSize = 4 + (32 * 1024);
      FTerminal->LogEvent(FORMAT("Limiting packet size to Momentum sftp-server limit of %d bytes",
        (int(FMaxPacketSize))));
    }
  }
}
//---------------------------------------------------------------------------
char * TSFTPFileSystem::GetEOL() const
{
  if (FVersion >= 4)
  {
    assert(!FEOL.IsEmpty());
    return FEOL.c_str();
  }
  else
  {
    return EOLToStr(FTerminal->GetSessionData()->EOLType);
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::LookupUsersGroups()
{
  assert(SupportsExtension(SFTP_EXT_OWNER_GROUP));

  TSFTPPacket PacketOwners(SSH_FXP_EXTENDED);
  TSFTPPacket PacketGroups(SSH_FXP_EXTENDED);

  TSFTPPacket * Packets[] = { &PacketOwners, &PacketGroups };
  TRemoteTokenList * Lists[] = { &FTerminal->FUsers, &FTerminal->FGroups };
  char ListTypes[] = { OGQ_LIST_OWNERS, OGQ_LIST_GROUPS };

  for (int Index = 0; Index < LENOF(Packets); Index++)
  {
    TSFTPPacket * Packet = Packets[Index];
    Packet->AddString(SFTP_EXT_OWNER_GROUP);
    Packet->AddByte(ListTypes[Index]);
    SendPacket(Packet);
    ReserveResponse(Packet, Packet);
  }

  for (int Index = 0; Index < LENOF(Packets); Index++)
  {
    TSFTPPacket * Packet = Packets[Index];

    ReceiveResponse(Packet, Packet, SSH_FXP_EXTENDED_REPLY, asOpUnsupported);

    if ((Packet->Type != SSH_FXP_EXTENDED_REPLY) ||
        (Packet->GetString() != SFTP_EXT_OWNER_GROUP_REPLY))
    {
      FTerminal->LogEvent(FORMAT("Invalid response to %s", (SFTP_EXT_OWNER_GROUP)));
    }
    else
    {
      TRemoteTokenList & List = *Lists[Index];
      unsigned long Count = Packet->GetCardinal();

      List.Clear();
      for (unsigned long Item = 0; Item < Count; Item++)
      {
        TRemoteToken Token(Packet->GetString(!FUtfNever));
        List.Add(Token);
        if (&List == &FTerminal->FGroups)
        {
          FTerminal->FMembership.Add(Token);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadCurrentDirectory()
{
  if (!FDirectoryToChangeTo.IsEmpty())
  {
    FCurrentDirectory = FDirectoryToChangeTo;
    FDirectoryToChangeTo = "";
  }
  else if (FCurrentDirectory.IsEmpty())
  {
    // this happens only after startup when default remote directory is not specified
    FCurrentDirectory = GetHomeDirectory();
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::HomeDirectory()
{
  ChangeDirectory(GetHomeDirectory());
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::TryOpenDirectory(const std::wstring Directory)
{
  FTerminal->LogEvent(FORMAT("Trying to open directory \"%s\".", (Directory)));
  TRemoteFile * File;
  CustomReadFile(Directory, File, SSH_FXP_LSTAT, NULL, asOpUnsupported);
  if (File == NULL)
  {
    // File can be NULL only when server does not support SSH_FXP_LSTAT.
    // Fallback to legacy solution, which in turn does not allow entering
    // traverse-only (chmod 110) directories.
    // This is workaround for http://www.ftpshell.com/
    TSFTPPacket Packet(SSH_FXP_OPENDIR);
    Packet.AddPathString(UnixExcludeTrailingBackslash(Directory), FUtfStrings);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);
    std::wstring Handle = Packet.GetString();
    Packet.ChangeType(SSH_FXP_CLOSE);
    Packet.AddString(Handle);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS, asAll);
  }
  else
  {
    delete File;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::AnnounceFileListOperation()
{
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ChangeDirectory(const std::wstring Directory)
{
  std::wstring Path, Current;

  Current = !FDirectoryToChangeTo.IsEmpty() ? FDirectoryToChangeTo : FCurrentDirectory;
  Path = RealPath(Directory, Current);

  // to verify existence of directory try to open it (SSH_FXP_REALPATH succeeds
  // for invalid paths on some systems, like CygWin)
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FDirectoryToChangeTo = Path;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CachedChangeDirectory(const std::wstring Directory)
{
  FDirectoryToChangeTo = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList && !FileList->Directory.IsEmpty());

  std::wstring Directory;
  Directory = UnixExcludeTrailingBackslash(LocalCanonify(FileList->Directory));
  FTerminal->LogEvent(FORMAT("Listing directory \"%s\".", (Directory)));

  // moved before SSH_FXP_OPENDIR, so directory listing does not retain
  // old data (e.g. parent directory) when reading fails
  FileList->Clear();

  TSFTPPacket Packet(SSH_FXP_OPENDIR);
  std::wstring Handle;

  try
  {
    Packet.AddPathString(Directory, FUtfStrings);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

    Handle = Packet.GetString();
  }
  catch(...)
  {
    if (FTerminal->GetActive())
    {
      FileList->AddFile(new TRemoteParentDirectory(FTerminal));
    }
    throw;
  }

  TSFTPPacket Response;
  try
  {
    bool isEOF = false;
    int Total = 0;
    TRemoteFile * File;

    Packet.ChangeType(SSH_FXP_READDIR);
    Packet.AddString(Handle);

    SendPacket(&Packet);

    do
    {
      ReceiveResponse(&Packet, &Response);
      if (Response.Type == SSH_FXP_NAME)
      {
        TSFTPPacket ListingPacket = Response;

        Packet.ChangeType(SSH_FXP_READDIR);
        Packet.AddString(Handle);

        SendPacket(&Packet);
        ReserveResponse(&Packet, &Response);

        unsigned int Count = ListingPacket.GetCardinal();

        for (unsigned long Index = 0; !isEOF && (Index < Count); Index++)
        {
          File = LoadFile(&ListingPacket, NULL, "", FileList);
          if (FTerminal->Configuration->->GetActualLogProtocol() >= 1)
          {
            FTerminal->LogEvent(FORMAT("Read file '%s' from listing", (File->FileName)));
          }
          FileList->AddFile(File);
          Total++;

          if (Total % 10 == 0)
          {
            FTerminal->DoReadDirectoryProgress(Total, isEOF);
            if (isEOF)
            {
              FTerminal->DoReadDirectoryProgress(-2, isEOF);
            }
          }
        }

        if (Count == 0)
        {
          FTerminal->LogEvent("Empty directory listing packet. Aborting directory reading.");
          isEOF = true;
        }
      }
      else if (Response.Type == SSH_FXP_STATUS)
      {
        isEOF = (GotStatusPacket(&Response, asEOF) == SSH_FX_EOF);
      }
      else
      {
        FTerminal->FatalError(NULL, FMTLOAD(SFTP_INVALID_TYPE, ((int)Response.Type)));
      }
    }
    while (!isEOF);

    if (Total == 0)
    {
      // Empty file list -> probably "permision denied", we
      // at least get link to parent directory ("..")
      try
      {
        FTerminal->ExceptionOnFail = true;
        try
        {
          File = NULL;
          FTerminal->ReadFile(
            UnixIncludeTrailingBackslash(FileList->Directory) + PARENTDIRECTORY, File);
        }
        __finally
        {
          FTerminal->ExceptionOnFail = false;
        }
      }
      catch(std::exception &E)
      {
        if (E.InheritsFrom(__classid(EFatal))) throw;
          else File = NULL;
      }

      // on some systems even getting ".." fails, we create dummy ".." instead
      bool Failure = (File == NULL);
      if (Failure)
      {
        File = new TRemoteParentDirectory(FTerminal);
      }

      assert(File && File->IsParentDirectory);
      FileList->AddFile(File);

      if (Failure)
      {
        throw std::exception(FMTLOAD(EMPTY_DIRECTORY, (FileList->Directory)));
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
      ReserveResponse(&Packet, NULL);
    }
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& File)
{
  assert(SymlinkFile && SymlinkFile->IsSymLink);
  assert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later

  // need to use full filename when resolving links within subdirectory
  // (i.e. for download)
  std::wstring FileName = LocalCanonify(
    SymlinkFile->Directory != NULL ? SymlinkFile->FullFileName : SymlinkFile->FileName);

  TSFTPPacket ReadLinkPacket(SSH_FXP_READLINK);
  ReadLinkPacket.AddPathString(FileName, FUtfStrings);
  SendPacket(&ReadLinkPacket);
  ReserveResponse(&ReadLinkPacket, &ReadLinkPacket);

  // send second request before reading response to first one
  // (performance benefit)
  TSFTPPacket AttrsPacket(SSH_FXP_STAT);
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
    FTerminal->FatalError(NULL, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
  }
  SymlinkFile->LinkTo = ReadLinkPacket.GetPathString(FUtfStrings);

  ReceiveResponse(&AttrsPacket, &AttrsPacket, SSH_FXP_ATTRS);
  // SymlinkFile->FileName was used instead SymlinkFile->LinkTo before, why?
  File = LoadFile(&AttrsPacket, SymlinkFile,
    UnixExtractFileName(SymlinkFile->LinkTo));
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadFile(const std::wstring FileName,
  TRemoteFile *& File)
{
  CustomReadFile(FileName, File, SSH_FXP_LSTAT);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::RemoteFileExists(const std::wstring FullPath,
  TRemoteFile ** File)
{
  bool Result;
  try
  {
    TRemoteFile * AFile;
    CustomReadFile(FullPath, AFile, SSH_FXP_LSTAT, NULL, asNoSuchFile);
    Result = (AFile != NULL);
    if (Result)
    {
      if (File)
      {
        *File = AFile;
      }
      else
      {
        delete AFile;
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
//---------------------------------------------------------------------------
void TSFTPFileSystem::SendCustomReadFile(TSFTPPacket * Packet,
  TSFTPPacket * Response, const std::wstring FileName, unsigned long Flags)
{
  if ((Packet->Type == SSH_FXP_STAT) || (Packet->Type == SSH_FXP_LSTAT))
  {
    Packet->AddPathString(LocalCanonify(FileName), FUtfStrings);
  }
  else
  {
    assert(Packet->Type == SSH_FXP_FSTAT);
    // actualy handle, not filename
    Packet->AddString(FileName);
  }

  if (FVersion >= 4)
  {
    Packet->AddCardinal(Flags);
  }
  SendPacket(Packet);
  ReserveResponse(Packet, Response);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CustomReadFile(const std::wstring FileName,
  TRemoteFile *& File, char Type, TRemoteFile * ALinkedByFile,
  int AllowStatus)
{
  unsigned long Flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS |
    SSH_FILEXFER_ATTR_ACCESSTIME | SSH_FILEXFER_ATTR_MODIFYTIME |
    SSH_FILEXFER_ATTR_OWNERGROUP;
  TSFTPPacket Packet(Type);
  SendCustomReadFile(&Packet, &Packet, FileName, Flags);
  ReceiveResponse(&Packet, &Packet, SSH_FXP_ATTRS, AllowStatus);

  if (Packet.Type == SSH_FXP_ATTRS)
  {
    File = LoadFile(&Packet, ALinkedByFile, UnixExtractFileName(FileName));
  }
  else
  {
    assert(AllowStatus > 0);
    File = NULL;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DoDeleteFile(const std::wstring FileName, char Type)
{
  TSFTPPacket Packet(Type);
  std::wstring RealFileName = LocalCanonify(FileName);
  Packet.AddPathString(RealFileName, FUtfStrings);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DeleteFile(const std::wstring FileName,
  const TRemoteFile * File, int Params, TRmSessionAction & Action)
{
  char Type;
  if (File && File->IsDirectory && !File->IsSymLink)
  {
    if (FLAGCLEAR(Params, dfNoRecursive))
    {
      try
      {
        FTerminal->ProcessDirectory(FileName, FTerminal->DeleteFile, &Params);
      }
      catch(...)
      {
        Action.Cancel();
        throw;
      }
    }
    Type = SSH_FXP_RMDIR;
  }
  else
  {
    Type = SSH_FXP_REMOVE;
  }

  DoDeleteFile(FileName, Type);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::RenameFile(const std::wstring FileName,
  const std::wstring NewName)
{
  TSFTPPacket Packet(SSH_FXP_RENAME);
  std::wstring RealName = LocalCanonify(FileName);
  Packet.AddPathString(RealName, FUtfStrings);
  std::wstring TargetName;
  if (UnixExtractFilePath(NewName).IsEmpty())
  {
    // rename case (TTerminal::RenameFile)
    TargetName = UnixExtractFilePath(RealName) + NewName;
  }
  else
  {
    TargetName = LocalCanonify(NewName);
  }
  Packet.AddPathString(TargetName, FUtfStrings);
  if (FVersion >= 5)
  {
    Packet.AddCardinal(0);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CopyFile(const std::wstring /*FileName*/,
  const std::wstring /*NewName*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CreateDirectory(const std::wstring DirName)
{
  TSFTPPacket Packet(SSH_FXP_MKDIR);
  std::wstring CanonifiedName = Canonify(DirName);
  Packet.AddPathString(CanonifiedName, FUtfStrings);
  Packet.AddProperties(NULL, 0, true, FVersion, FUtfStrings, NULL);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CreateLink(const std::wstring FileName,
  const std::wstring PointTo, bool Symbolic)
{
  USEDPARAM(Symbolic);
  assert(Symbolic); // only symlinks are supported by SFTP
  assert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later
  TSFTPPacket Packet(SSH_FXP_SYMLINK);

  bool Buggy = (FTerminal->GetSessionData()->SFTPBug[sbSymlink] == asOn) ||
    ((FTerminal->GetSessionData()->SFTPBug[sbSymlink] == asAuto) && FOpenSSH);

  if (!Buggy)
  {
    Packet.AddPathString(Canonify(FileName), FUtfStrings);
    Packet.AddPathString(PointTo, FUtfStrings);
  }
  else
  {
    FTerminal->LogEvent("We believe the server has SFTP symlink bug");
    Packet.AddPathString(PointTo, FUtfStrings);
    Packet.AddPathString(Canonify(FileName), FUtfStrings);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ChangeFileProperties(const std::wstring FileName,
  const TRemoteFile * /*File*/, const TRemoteProperties * AProperties,
  TChmodSessionAction & Action)
{
  assert(AProperties != NULL);

  TRemoteFile * File;

  std::wstring RealFileName = LocalCanonify(FileName);
  ReadFile(RealFileName, File);

  try
  {
    assert(File);

    if (File->IsDirectory && !File->IsSymLink && AProperties->Recursive)
    {
      try
      {
        FTerminal->ProcessDirectory(FileName, FTerminal->ChangeFileProperties,
          (void*)AProperties);
      }
      catch(...)
      {
        Action.Cancel();
        throw;
      }
    }

    // SFTP can change owner and group at the same time only, not individually.
    // Fortunatelly we know current owner/group, so if only one is present,
    // we can supplement the other.
    TRemoteProperties Properties(*AProperties);
    if (Properties.Valid.Contains(vpGroup) &&
        !Properties.Valid.Contains(vpOwner))
    {
      Properties.Owner = File->Owner;
      Properties.Valid << vpOwner;
    }
    else if (Properties.Valid.Contains(vpOwner) &&
             !Properties.Valid.Contains(vpGroup))
    {
      Properties.Group = File->Group;
      Properties.Valid << vpGroup;
    }

    TSFTPPacket Packet(SSH_FXP_SETSTAT);
    Packet.AddPathString(RealFileName, FUtfStrings);
    Packet.AddProperties(&Properties, *File->Rights, File->IsDirectory, FVersion, FUtfStrings, &Action);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
  }
  __finally
  {
    delete File;
  }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::LoadFilesProperties(TStrings * FileList)
{
  bool Result = false;
  // without knowledge of server's capabilities, this all make no sense
  if (FSupport->Loaded)
  {
    TFileOperationProgressType Progress(&FTerminal->DoProgress, &FTerminal->DoFinished);
    Progress.Start(foGetProperties, osRemote, FileList->GetCount());

    FTerminal->FOperationProgress = &Progress;

    static int LoadFilesPropertiesQueueLen = 5;
    TSFTPLoadFilesPropertiesQueue Queue(this);
    try
    {
      if (Queue.Init(LoadFilesPropertiesQueueLen, FileList))
      {
        TRemoteFile * File;
        TSFTPPacket Packet;
        bool Next;
        do
        {
          Next = Queue.ReceivePacket(&Packet, File);
          assert((Packet.Type == SSH_FXP_ATTRS) || (Packet.Type == SSH_FXP_STATUS));
          if (Packet.Type == SSH_FXP_ATTRS)
          {
            Progress.SetFile(File->FileName);
            assert(File != NULL);
            LoadFile(File, &Packet);
            Result = true;
            TOnceDoneOperation OnceDoneOperation;
            Progress.Finish(File->FileName, true, OnceDoneOperation);
          }

          if (Progress.Cancel != csContinue)
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
      FTerminal->FOperationProgress = NULL;
      Progress.Stop();
    }
    // queue is discarded here
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DoCalculateFilesChecksum(const std::wstring & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  TOnceDoneOperation OnceDoneOperation; // not used

  // recurse into subdirectories only if we have callback function
  if (OnCalculatedChecksum != NULL)
  {
    for (int Index = 0; Index < FileList->GetCount(); Index++)
    {
      TRemoteFile * File = (TRemoteFile *)FileList->GetObject(Index);
      assert(File != NULL);
      if (File->IsDirectory && !File->IsSymLink &&
          !File->IsParentDirectory && !File->IsThisDirectory)
      {
        OperationProgress->SetFile(File->FileName);
        TRemoteFileList * SubFiles =
          FTerminal->CustomReadDirectoryListing(File->FullFileName, false);

        if (SubFiles != NULL)
        {
          TStrings * SubFileList = new TStringList();
          bool Success = false;
          try
          {
            OperationProgress->SetFile(File->FileName);

            for (int Index = 0; Index < SubFiles->GetCount(); Index++)
            {
              TRemoteFile * SubFile = SubFiles->Files[Index];
              SubFileList->AddObject(SubFile->FullFileName, SubFile);
            }

            // do not collect checksums for files in subdirectories,
            // only send back checksums via callback
            DoCalculateFilesChecksum(Alg, SubFileList, NULL,
              OnCalculatedChecksum, OperationProgress, false);

            Success = true;
          }
          __finally
          {
            delete SubFiles;
            delete SubFileList;

            if (FirstLevel)
            {
              OperationProgress->Finish(File->FileName, Success, OnceDoneOperation);
            }
          }
        }
      }
    }
  }

  static int CalculateFilesChecksumQueueLen = 5;
  TSFTPCalculateFilesChecksumQueue Queue(this);
  try
  {
    if (Queue.Init(CalculateFilesChecksumQueueLen, Alg, FileList))
    {
      TSFTPPacket Packet;
      bool Next;
      do
      {
        bool Success = false;
        std::wstring Alg;
        std::wstring Checksum;
        TRemoteFile * File = NULL;

        try
        {
          try
          {
            Next = Queue.ReceivePacket(&Packet, File);
            assert(Packet.Type == SSH_FXP_EXTENDED_REPLY);

            OperationProgress->SetFile(File->FileName);

            Alg = Packet.GetString();
            Checksum = StrToHex(std::wstring(Packet.GetNextData(Packet.RemainingLength), Packet.RemainingLength));
            OnCalculatedChecksum(File->FileName, Alg, Checksum);

            Success = true;
          }
          catch (std::exception & E)
          {
            FTerminal->CommandError(&E, FMTLOAD(CHECKSUM_ERROR,
              (File != NULL ? File->FullFileName : std::wstring(""))));
            // TODO: retries? resume?
            Next = false;
          }

          if (Checksums != NULL)
          {
            Checksums->Add("");
          }
        }
        __finally
        {
          if (FirstLevel)
          {
            OperationProgress->Finish(File->FileName, Success, OnceDoneOperation);
          }
        }

        if (OperationProgress->Cancel != csContinue)
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
  }
  // queue is discarded here
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CalculateFilesChecksum(const std::wstring & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  TFileOperationProgressType Progress(&FTerminal->DoProgress, &FTerminal->DoFinished);
  Progress.Start(foCalculateChecksum, osRemote, FileList->GetCount());

  FTerminal->FOperationProgress = &Progress;

  try
  {
    DoCalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum,
      &Progress, true);
  }
  __finally
  {
    FTerminal->FOperationProgress = NULL;
    Progress.Stop();
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CustomCommandOnFile(const std::wstring /*FileName*/,
    const TRemoteFile * /*File*/, std::wstring /*Command*/, int /*Params*/,
    TCaptureOutputEvent /*OutputEvent*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::AnyCommand(const std::wstring /*Command*/,
  TCaptureOutputEvent /*OutputEvent*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::FileUrl(const std::wstring FileName)
{
  return FTerminal->FileUrl("sftp", FileName);
}
//---------------------------------------------------------------------------
TStrings * TSFTPFileSystem::GetFixedPaths()
{
  return FFixedPaths;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SpaceAvailable(const std::wstring Path,
  TSpaceAvailable & ASpaceAvailable)
{
  TSFTPPacket Packet(SSH_FXP_EXTENDED);
  Packet.AddString(SFTP_EXT_SPACE_AVAILABLE);
  Packet.AddPathString(LocalCanonify(Path), FUtfStrings);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);
  ASpaceAvailable.BytesOnDevice = Packet.GetInt64();
  ASpaceAvailable.UnusedBytesOnDevice = Packet.GetInt64();
  ASpaceAvailable.BytesAvailableToUser = Packet.GetInt64();
  ASpaceAvailable.UnusedBytesAvailableToUser = Packet.GetInt64();
  ASpaceAvailable.BytesPerAllocationUnit = Packet.GetCardinal();
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void TSFTPFileSystem::CopyToRemote(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  assert(FilesToCopy && OperationProgress);

  std::wstring FileName, FileNameOnly;
  std::wstring FullTargetDir = UnixIncludeTrailingBackslash(TargetDir);
  int Index = 0;
  while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    bool Success = false;
    FileName = FilesToCopy->GetString(Index);
    FileNameOnly = ExtractFileName(FileName);
    assert(!FAvoidBusy);
    FAvoidBusy = true;

    try
    {
      try
      {
        if (FTerminal->GetSessionData()->CacheDirectories)
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (DirectoryExists(FileName))
          {
            FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir)+
              FileNameOnly, true);
          }
        }
        SFTPSourceRobust(FileName, FullTargetDir, CopyParam, Params, OperationProgress,
          tfFirstLevel);
        Success = true;
      }
      catch(EScpSkipFile & E)
      {
        SUSPEND_OPERATION (
          if (!FTerminal->HandleException(&E))
          {
            throw;
          };
        );
      }
    }
    __finally
    {
      FAvoidBusy = false;
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    }
    Index++;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPConfirmOverwrite(std::wstring & FileName,
  int Params, TFileOperationProgressType * OperationProgress,
  TSFTPOverwriteMode & OverwriteMode, const TOverwriteFileParams * FileParams)
{
  bool CanAppend = (FVersion < 4) || !OperationProgress->AsciiTransfer;
  int Answer;
  SUSPEND_OPERATION
  (
    int Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;

    // possibly we can allow alternate resume at least in some cases
    if (CanAppend)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[3];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(APPEND_BUTTON);
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.NoBatchAnswers = qaIgnore | qaRetry | qaAll;
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = LENOF(Aliases);
    Answer = FTerminal->ConfirmFileOverwrite(FileName, FileParams,
      Answers, &QueryParams,
      OperationProgress->Side == osLocal ? osRemote : osLocal,
      Params, OperationProgress);
  );

  if (CanAppend &&
      ((Answer == qaRetry) || (Answer == qaSkip)))
  {
    // duplicated in TTerminal::ConfirmFileOverwrite
    bool CanAlternateResume =
      (FileParams->DestSize < FileParams->SourceSize) && !OperationProgress->AsciiTransfer;
    TBatchOverwrite BatchOverwrite =
      FTerminal->EffectiveBatchOverwrite(Params, OperationProgress, true);
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
      SUSPEND_OPERATION
      (
        Answer = FTerminal->QueryUser(FORMAT(LoadStr(APPEND_OR_RESUME), (FileName)),
          NULL, qaYes | qaNo | qaNoToAll | qaCancel, &Params);
      );

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
          OperationProgress->BatchOverwrite = boAlternateResume;
          break;

        default: assert(false); //fallthru
        case qaCancel:
          if (!OperationProgress->Cancel)
          {
            OperationProgress->Cancel = csCancel;
          }
          Abort();
          break;
      }
    }
  }
  else if (Answer == qaIgnore)
  {
    if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName, LoadStr(RENAME_TITLE), "",
          LoadStr(RENAME_PROMPT2), true, 0, FileName))
    {
      OverwriteMode = omOverwrite;
    }
    else
    {
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      Abort();
    }
  }
  else
  {
    OverwriteMode = omOverwrite;
    switch (Answer)
    {
      case qaCancel:
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
        Abort();
        break;

      case qaNo:
        THROW_SKIP_FILE_NULL;
    }
  }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::SFTPConfirmResume(const std::wstring DestFileName,
  bool PartialBiggerThanSource, TFileOperationProgressType * OperationProgress)
{
  bool ResumeTransfer;
  assert(OperationProgress);
  if (PartialBiggerThanSource)
  {
    int Answer;
    SUSPEND_OPERATION
    (
      TQueryParams Params(qpAllowContinueOnError, HELP_PARTIAL_BIGGER_THAN_SOURCE);
      Answer = FTerminal->QueryUser(
        FMTLOAD(PARTIAL_BIGGER_THAN_SOURCE, (DestFileName)), NULL,
          qaOK | qaAbort, &Params, qtWarning);
    )

    if (Answer == qaAbort)
    {
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      Abort();
    }
    ResumeTransfer = false;
  }
  else if (FTerminal->Configuration->ConfirmResume)
  {
    int Answer;
    SUSPEND_OPERATION
    (
      TQueryParams Params(qpAllowContinueOnError | qpNeverAskAgainCheck,
        HELP_RESUME_TRANSFER);
      // "abort" replaced with "cancel" to unify with "append/resume" query
      Answer = FTerminal->QueryUser(
        FMTLOAD(RESUME_TRANSFER, (DestFileName)), NULL, qaYes | qaNo | qaCancel,
        &Params);
    );

    switch (Answer) {
      case qaNeverAskAgain:
        FTerminal->Configuration->ConfirmResume = false;
      case qaYes:
        ResumeTransfer = true;
        break;

      case qaNo:
        ResumeTransfer = false;
        break;

      case qaCancel:
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
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
void TSFTPFileSystem::SFTPSourceRobust(const std::wstring FileName,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, unsigned int Flags)
{
  // the same in TFTPFileSystem
  bool Retry;

  TUploadSessionAction Action(FTerminal->Log);

  do
  {
    Retry = false;
    bool ChildError = false;
    try
    {
      SFTPSource(FileName, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action, ChildError);
    }
    catch(std::exception & E)
    {
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        if (!ChildError)
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (Retry)
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite and resume confirmations
      // (should not be set for directories!)
      Params |= cpNoConfirmation;
      // enable resume even if we are uploading into new directory
      Flags &= ~tfNewDirectory;
    }
  }
  while (Retry);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSource(const std::wstring FileName,
  const std::wstring TargetDir, const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, unsigned int Flags,
  TUploadSessionAction & Action, bool & ChildError)
{
  FTerminal->LogEvent(FORMAT("File: \"%s\"", (FileName)));

  Action.FileName(ExpandUNCFileName(FileName));

  OperationProgress->SetFile(FileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    FTerminal->LogEvent(FORMAT("File \"%s\" excluded from transfer", (FileName)));
    THROW_SKIP_FILE_NULL;
  }

  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;

  HANDLE File;
  __int64 MTime, ATime;
  __int64 Size;

  FTerminal->OpenLocalFile(FileName, GENERIC_READ, &OpenParams.LocalFileAttrs,
    &File, NULL, &MTime, &ATime, &Size);

  bool Dir = FLAGSET(OpenParams.LocalFileAttrs, faDirectory);

  try
  {
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      SFTPDirectorySource(IncludeTrailingBackslash(FileName), TargetDir,
        OpenParams.LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
      Action.Cancel();
    }
    else
    {
      // File is regular file (not directory)
      assert(File);

      std::wstring DestFileName = CopyParam->ChangeFileName(ExtractFileName(FileName),
        osLocal, FLAGSET(Flags, tfFirstLevel));
      std::wstring DestFullName = LocalCanonify(TargetDir + DestFileName);
      std::wstring DestPartinalFullName;
      bool ResumeAllowed;
      bool ResumeTransfer = false;
      bool DestFileExists = false;
      TRights DestRights;

      __int64 ResumeOffset;

      FTerminal->LogEvent(FORMAT("Copying \"%s\" to remote directory started.", (FileName)));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->LocalSize);
      OperationProgress->TransferingFile = false;

      // Will we use ASCII of BINARY file tranfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(FileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        std::wstring((OperationProgress->AsciiTransfer ? "Ascii" : "Binary")) +
          " transfer mode selected.");

      // should we check for interrupted transfer?
      ResumeAllowed = !OperationProgress->AsciiTransfer &&
        CopyParam->AllowResume(OperationProgress->LocalSize) &&
        IsCapable(fcRename);
      OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

      TOverwriteFileParams FileParams;
      FileParams.SourceSize = OperationProgress->LocalSize;
      FileParams.SourceTimestamp = UnixToDateTime(MTime,
        FTerminal->GetSessionData()->DSTMode);

      if (ResumeAllowed)
      {
        DestPartinalFullName = DestFullName + FTerminal->Configuration->PartialExt;

        if (FLAGCLEAR(Flags, tfNewDirectory))
        {
          FTerminal->LogEvent("Checking existence of file.");
          TRemoteFile * File = NULL;
          DestFileExists = RemoteFileExists(DestFullName, &File);
          if (DestFileExists)
          {
            OpenParams.DestFileSize = File->Size;
            FileParams.DestSize = OpenParams.DestFileSize;
            FileParams.DestTimestamp = File->Modification;
            DestRights = *File->Rights;
            // if destination file is symlink, never do resumable transfer,
            // as it would delete the symlink.
            // also bit of heuristics to detect symlink on SFTP-3 and older
            // (which does not indicate symlink in SSH_FXP_ATTRS).
            // if file has all permissions and is small, then it is likely symlink.
            // also it is not likely that such a small file (if it is not symlink)
            // gets overwritten by large file (that would trigger resumable transfer).
            if (File->IsSymLink ||
                ((FVersion < 4) &&
                 ((*File->Rights & TRights::rfAll) == TRights::rfAll) &&
                 (File->Size < 100)))
            {
              ResumeAllowed = false;
              OperationProgress->SetResumeStatus(rsDisabled);
            }

            delete File;
            File = NULL;
          }

          if (ResumeAllowed)
          {
            FTerminal->LogEvent("Checking existence of partially transfered file.");
            if (RemoteFileExists(DestPartinalFullName, &File))
            {
              ResumeOffset = File->Size;
              delete File;
              File = NULL;

              bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->LocalSize);
              if (FLAGCLEAR(Params, cpNoConfirmation) &&
                  FLAGCLEAR(Params, cpResume))
              {
                ResumeTransfer = SFTPConfirmResume(DestFileName,
                  PartialBiggerThanSource, OperationProgress);
              }
              else
              {
                ResumeTransfer = !PartialBiggerThanSource;
              }

              if (!ResumeTransfer)
              {
                DoDeleteFile(DestPartinalFullName, SSH_FXP_REMOVE);
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
                std::wstring PrevDestFileName = DestFileName;
                SFTPConfirmOverwrite(DestFileName,
                  Params, OperationProgress, OpenParams.OverwriteMode, &FileParams);
                if (PrevDestFileName != DestFileName)
                {
                  // update paths in case user changes the file name
                  DestFullName = LocalCanonify(TargetDir + DestFileName);
                  DestPartinalFullName = DestFullName + FTerminal->Configuration->PartialExt;
                  FTerminal->LogEvent("Checking existence of new file.");
                  DestFileExists = RemoteFileExists(DestFullName, NULL);
                }
              }
            }
          }
        }
      }

      // will the transfer be resumable?
      bool DoResume = (ResumeAllowed && (OpenParams.OverwriteMode == omOverwrite));

      std::wstring RemoteFileName = DoResume ? DestPartinalFullName : DestFullName;
      OpenParams.RemoteFileName = RemoteFileName;
      OpenParams.Resume = DoResume;
      OpenParams.Resuming = ResumeTransfer;
      OpenParams.OperationProgress = OperationProgress;
      OpenParams.CopyParam = CopyParam;
      OpenParams.Params = Params;
      OpenParams.FileParams = &FileParams;
      OpenParams.Confirmed = false;

      FTerminal->LogEvent("Opening remote file.");
      FTerminal->FileOperationLoop(SFTPOpenRemote, OperationProgress, true,
        FMTLOAD(SFTP_CREATE_FILE_ERROR, (OpenParams.RemoteFileName)),
        &OpenParams);

      if (OpenParams.RemoteFileName != RemoteFileName)
      {
        assert(!DoResume);
        assert(UnixExtractFilePath(OpenParams.RemoteFileName) == UnixExtractFilePath(RemoteFileName));
        DestFullName = OpenParams.RemoteFileName;
        std::wstring NewFileName = UnixExtractFileName(DestFullName);
        assert(DestFileName != NewFileName);
        DestFileName = NewFileName;
      }

      Action.Destination(DestFullName);

      bool TransferFinished = false;
      __int64 DestWriteOffset = 0;
      TSFTPPacket CloseRequest;
      bool SetRights = ((DoResume && DestFileExists) || CopyParam->PreserveRights);
      bool SetProperties = (CopyParam->PreserveTime || SetRights);
      TSFTPPacket PropertiesRequest(SSH_FXP_SETSTAT);
      TSFTPPacket PropertiesResponse;
      TRights Rights;
      if (SetProperties)
      {
        PropertiesRequest.AddPathString(DestFullName, FUtfStrings);
        if (CopyParam->PreserveRights)
        {
          Rights = CopyParam->RemoteFileRights(OpenParams.LocalFileAttrs);
        }
        else if (DoResume && DestFileExists)
        {
          Rights = DestRights;
        }
        else
        {
          assert(!SetRights);
        }

        unsigned short RightsNumber = Rights.NumberSet;
        PropertiesRequest.AddProperties(
          SetRights ? &RightsNumber : NULL, NULL, NULL,
          CopyParam->PreserveTime ? &MTime : NULL,
          CopyParam->PreserveTime ? &ATime : NULL,
          NULL, false, FVersion, FUtfStrings);
      }

      try
      {
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
          FileSeek((THandle)File, ResumeOffset, 0);
          OperationProgress->AddResumed(ResumeOffset);
        }

        TSFTPUploadQueue Queue(this);
        try
        {
          Queue.Init(FileName, File, OperationProgress,
            OpenParams.RemoteFileHandle,
            DestWriteOffset + OperationProgress->TransferedSize);

          while (Queue.Continue())
          {
            if (OperationProgress->Cancel)
            {
              Abort();
            }
          }

          // send close request before waiting for pending read responses
          SFTPCloseRemote(OpenParams.RemoteFileHandle, DestFileName,
            OperationProgress, false, true, &CloseRequest);
          OpenParams.RemoteFileHandle = "";

          // when resuming is disabled, we can send "set properties"
          // request before waiting for pending read/close responses
          if (SetProperties && !DoResume)
          {
            SendPacket(&PropertiesRequest);
            ReserveResponse(&PropertiesRequest, &PropertiesResponse);
          }
        }
        __finally
        {
          Queue.DisposeSafe();
        }

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
      }

      if (DoResume)
      {
        if (DestFileExists)
        {
          FILE_OPERATION_LOOP(FMTLOAD(DELETE_ON_RESUME_ERROR,
              (UnixExtractFileName(DestFullName), DestFullName)),

            if (FTerminal->GetSessionData()->OverwrittenToRecycleBin)
            {
              FTerminal->RecycleFile(DestFullName, NULL);
            }
            else
            {
              DoDeleteFile(DestFullName, SSH_FXP_REMOVE);
            }
          );
        }

        // originally this was before CLOSE (last __finally statement),
        // on VShell it failed
        FILE_OPERATION_LOOP(FMTLOAD(RENAME_AFTER_RESUME_ERROR,
            (UnixExtractFileName(OpenParams.RemoteFileName), DestFileName)),
          RenameFile(OpenParams.RemoteFileName, DestFileName);
        );
      }

      if (SetProperties)
      {
        std::auto_ptr<TTouchSessionAction> TouchAction;
        if (CopyParam->PreserveTime)
        {
          TouchAction.reset(new TTouchSessionAction(FTerminal->Log, DestFullName,
            UnixToDateTime(MTime, FTerminal->GetSessionData()->DSTMode)));
        }
        std::auto_ptr<TChmodSessionAction> ChmodAction;
        // do record chmod only if it was explicitly requested,
        // not when it was implicitly performed to apply timestamp
        // of overwritten file to new file
        if (CopyParam->PreserveRights)
        {
          ChmodAction.reset(new TChmodSessionAction(FTerminal->Log, DestFullName, Rights));
        }
        try
        {
          // when resuming is enabled, the set properties request was not sent yet
          if (DoResume)
          {
            SendPacket(&PropertiesRequest);
          }
          bool Resend = false;
          FILE_OPERATION_LOOP(FMTLOAD(PRESERVE_TIME_PERM_ERROR, (DestFileName)),
            TSFTPPacket DummyResponse;
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
              asOK | FLAGMASK(CopyParam->IgnorePermErrors, asPermDenied));
          );
        }
        catch(std::exception & E)
        {
          if (TouchAction.get() != NULL)
          {
            TouchAction->Rollback(&E);
          }
          if (ChmodAction.get() != NULL)
          {
            ChmodAction->Rollback(&E);
          }
          ChildError = true;
          throw;
        }
      }
    }
  }
  __finally
  {
    if (File != NULL)
    {
      CloseHandle(File);
    }
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, (FileName)),
        THROWOSIFFALSE(Sysutils::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->ClearArchive && FLAGSET(OpenParams.LocalFileAttrs, faArchive))
  {
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, (FileName)),
      THROWOSIFFALSE(FileSetAttr(FileName, OpenParams.LocalFileAttrs & ~faArchive) == 0);
    )
  }
}
//---------------------------------------------------------------------------
std::wstring TSFTPFileSystem::SFTPOpenRemoteFile(
  const std::wstring & FileName, unsigned int OpenType, __int64 Size)
{
  TSFTPPacket Packet(SSH_FXP_OPEN);

  Packet.AddPathString(FileName, FUtfStrings);
  if (FVersion < 5)
  {
    Packet.AddCardinal(OpenType);
  }
  else
  {
    unsigned long Access =
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_READ), ACE4_READ_DATA) |
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_WRITE), ACE4_WRITE_DATA | ACE4_APPEND_DATA);

    unsigned long Flags;

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
    FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_TRUNC);
  Packet.AddProperties(NULL, NULL, NULL, NULL, NULL,
    SendSize ? &Size : NULL, false, FVersion, FUtfStrings);

  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

  return Packet.GetString();
}
//---------------------------------------------------------------------------
int TSFTPFileSystem::SFTPOpenRemote(void * AOpenParams, void * /*Param2*/)
{
  TOpenRemoteFileParams * OpenParams = (TOpenRemoteFileParams *)AOpenParams;
  assert(OpenParams);
  TFileOperationProgressType * OperationProgress = OpenParams->OperationProgress;

  int OpenType;
  bool Success = false;
  bool ConfirmOverwriting;

  do
  {
    try
    {
      ConfirmOverwriting =
        !OpenParams->Confirmed && !OpenParams->Resume &&
        FTerminal->CheckRemoteFile(OpenParams->Params, OperationProgress);
      OpenType = SSH_FXF_WRITE | SSH_FXF_CREAT;
      // when we want to preserve overwritten files, we need to find out that
      // they exist first... even if overwrite confirmation is disabled.
      // but not when we already know we are not going to overwrite (but e.g. to append)
      if ((ConfirmOverwriting || FTerminal->GetSessionData()->OverwrittenToRecycleBin) &&
          (OpenParams->OverwriteMode == omOverwrite))
      {
        OpenType |= SSH_FXF_EXCL;
      }
      if (!OpenParams->Resuming && (OpenParams->OverwriteMode == omOverwrite))
      {
        OpenType |= SSH_FXF_TRUNC;
      }
      if ((FVersion >= 4) && OpenParams->OperationProgress->AsciiTransfer)
      {
        OpenType |= SSH_FXF_TEXT;
      }

      OpenParams->RemoteFileHandle = SFTPOpenRemoteFile(
        OpenParams->RemoteFileName, OpenType, OperationProgress->LocalSize);

      Success = true;
    }
    catch(std::exception & E)
    {
      if (!OpenParams->Confirmed && (OpenType & SSH_FXF_EXCL) && FTerminal->GetActive())
      {
        bool ThrowOriginal = false;

        // When exclusive opening of file fails, try to detect if file exists.
        // When file does not exist, failure was probably caused by 'permission denied'
        // or similar error. In this case throw original exception.
        try
        {
          TRemoteFile * File;
          std::wstring RealFileName = LocalCanonify(OpenParams->RemoteFileName);
          ReadFile(RealFileName, File);
          OpenParams->DestFileSize = File->Size;
          if (OpenParams->FileParams != NULL)
          {
            OpenParams->FileParams->DestTimestamp = File->Modification;
            OpenParams->FileParams->DestSize = OpenParams->DestFileSize;
          }
          // file exists (otherwise exception was thrown)
          assert(File);
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

        // we may get here even if confirmation is disabled,
        // when we have preserving of overwritten files enabled
        if (ConfirmOverwriting)
        {
          // confirmation duplicated in SFTPSource for resumable file transfers.
          std::wstring RemoteFileNameOnly = UnixExtractFileName(OpenParams->RemoteFileName);
          SFTPConfirmOverwrite(RemoteFileNameOnly,
            OpenParams->Params, OperationProgress, OpenParams->OverwriteMode, OpenParams->FileParams);
          if (RemoteFileNameOnly != UnixExtractFileName(OpenParams->RemoteFileName))
          {
            OpenParams->RemoteFileName =
              UnixExtractFilePath(OpenParams->RemoteFileName) + RemoteFileNameOnly;
          }
          OpenParams->Confirmed = true;
        }
        else
        {
          assert(FTerminal->GetSessionData()->OverwrittenToRecycleBin);
        }

        if ((OpenParams->OverwriteMode == omOverwrite) &&
            FTerminal->GetSessionData()->OverwrittenToRecycleBin)
        {
          FTerminal->RecycleFile(OpenParams->RemoteFileName, NULL);
        }
      }
      else if (FTerminal->GetActive())
      {
        // if file overwritting was confirmed, it means that the file already exists,
        // if not, check now
        if (!OpenParams->Confirmed)
        {
          bool ThrowOriginal = false;

          // When file does not exist, failure was probably caused by 'permission denied'
          // or similar error. In this case throw original exception.
          try
          {
            TRemoteFile * File;
            std::wstring RealFileName = LocalCanonify(OpenParams->RemoteFileName);
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
              FMTLOAD(SFTP_OVERWRITE_FILE_ERROR, (OpenParams->RemoteFileName)),
              true, LoadStr(SFTP_OVERWRITE_DELETE_BUTTON)))
        {
          int Params = dfNoRecursive;
          FTerminal->DeleteFile(OpenParams->RemoteFileName, NULL, &Params);
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
void TSFTPFileSystem::SFTPCloseRemote(const std::wstring Handle,
  const std::wstring FileName, TFileOperationProgressType * OperationProgress,
  bool TransferFinished, bool Request, TSFTPPacket * Packet)
{
  // Moving this out of SFTPSource() fixed external exception 0xC0000029 error
  FILE_OPERATION_LOOP(FMTLOAD(SFTP_CLOSE_FILE_ERROR, (FileName)),
    try
    {
      TSFTPPacket CloseRequest;
      TSFTPPacket * P = (Packet == NULL ? &CloseRequest : Packet);

      if (Request)
      {
        P->ChangeType(SSH_FXP_CLOSE);
        P->AddString(Handle);
        SendPacket(P);
        ReserveResponse(P, Packet);
      }
      else
      {
        assert(Packet != NULL);
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
  );
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPDirectorySource(const std::wstring DirectoryName,
  const std::wstring TargetDir, int Attrs, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags)
{
  std::wstring DestDirectoryName = CopyParam->ChangeFileName(
    ExtractFileName(ExcludeTrailingBackslash(DirectoryName)), osLocal,
    FLAGSET(Flags, tfFirstLevel));
  std::wstring DestFullName = UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);

  OperationProgress->SetFile(DirectoryName);

  bool CreateDir = false;
  try
  {
    TryOpenDirectory(DestFullName);
  }
  catch(...)
  {
    if (FTerminal->GetActive())
    {
      // opening directory failed, it probably does not exists, try to
      // create it
      CreateDir = true;
    }
    else
    {
      throw;
    }
  }

  if (CreateDir)
  {
    TRemoteProperties Properties;
    if (CopyParam->PreserveRights)
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(Attrs);
    }
    FTerminal->CreateDirectory(DestFullName, &Properties);
    Flags |= tfNewDirectory;
  }

  int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  TSearchRec SearchRec;
  bool FindOK;

  FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, (DirectoryName)),
    FindOK = (bool)(FindFirst(DirectoryName + "*.*",
      FindAttrs, SearchRec) == 0);
  );

  try
  {
    while (FindOK && !OperationProgress->Cancel)
    {
      std::wstring FileName = DirectoryName + SearchRec.Name;
      try
      {
        if ((SearchRec.Name != ".") && (SearchRec.Name != ".."))
        {
          SFTPSourceRobust(FileName, DestFullName, CopyParam, Params, OperationProgress,
            Flags & ~tfFirstLevel);
        }
      }
      catch (EScpSkipFile &E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        SUSPEND_OPERATION (
          // here a message to user was displayed, which was not appropriate
          // when user refused to overwrite the file in subdirectory.
          // hopefuly it won't be missing in other situations.
          if (!FTerminal->HandleException(&E)) throw;
        );
      }

      FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, (DirectoryName)),
        FindOK = (FindNext(SearchRec) == 0);
      );
    };
  }
  __finally
  {
    FindClose(SearchRec);
  }

  /* TODO : Delete also read-only directories. */
  /* TODO : Show error message on failure. */
  if (!OperationProgress->Cancel)
  {
    if (FLAGSET(Params, cpDelete))
    {
      RemoveDir(DirectoryName);
    }
    else if (CopyParam->ClearArchive && FLAGSET(Attrs, faArchive))
    {
      FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, (DirectoryName)),
        THROWOSIFFALSE(FileSetAttr(DirectoryName, Attrs & ~faArchive) == 0);
      )
    }
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CopyToLocal(TStrings * FilesToCopy,
  const std::wstring TargetDir, const TCopyParamType * CopyParam,
  int Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  assert(FilesToCopy && OperationProgress);

  std::wstring FileName;
  std::wstring FullTargetDir = IncludeTrailingBackslash(TargetDir);
  const TRemoteFile * File;
  bool Success;
  int Index = 0;
  while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    Success = false;
    FileName = FilesToCopy->GetString(Index);
    File = (TRemoteFile *)FilesToCopy->GetObject(Index);

    assert(!FAvoidBusy);
    FAvoidBusy = true;

    try
    {
      try
      {
        SFTPSinkRobust(LocalCanonify(FileName), File, FullTargetDir, CopyParam,
          Params, OperationProgress, tfFirstLevel);
        Success = true;
      }
      catch(EScpSkipFile & E)
      {
        SUSPEND_OPERATION (
          if (!FTerminal->HandleException(&E)) throw;
        );
      }
      catch(...)
      {
        // TODO: remove the block?
        throw;
      }
    }
    __finally
    {
      FAvoidBusy = false;
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    }
    Index++;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSinkRobust(const std::wstring FileName,
  const TRemoteFile * File, const std::wstring TargetDir,
  const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, unsigned int Flags)
{
  // the same in TFTPFileSystem
  bool Retry;

  TDownloadSessionAction Action(FTerminal->Log);

  do
  {
    Retry = false;
    bool ChildError = false;
    try
    {
      SFTPSink(FileName, File, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action, ChildError);
    }
    catch(std::exception & E)
    {
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        if (!ChildError)
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (Retry)
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      assert(File != NULL);
      if (!File->IsDirectory)
      {
        // prevent overwrite and resume confirmations
        Params |= cpNoConfirmation;
      }
    }
  }
  while (Retry);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSink(const std::wstring FileName,
  const TRemoteFile * File, const std::wstring TargetDir,
  const TCopyParamType * CopyParam, int Params,
  TFileOperationProgressType * OperationProgress, unsigned int Flags,
  TDownloadSessionAction & Action, bool & ChildError)
{

  Action.FileName(FileName);

  std::wstring OnlyFileName = UnixExtractFileName(FileName);

  TFileMasks::TParams MaskParams;
  MaskParams.Size = File->Size;

  if (!CopyParam->AllowTransfer(FileName, osRemote, File->IsDirectory, MaskParams))
  {
    FTerminal->LogEvent(FORMAT("File \"%s\" excluded from transfer", (FileName)));
    THROW_SKIP_FILE_NULL;
  }

  assert(File);
  FTerminal->LogEvent(FORMAT("File: \"%s\"", (FileName)));

  OperationProgress->SetFile(OnlyFileName);

  std::wstring DestFileName = CopyParam->ChangeFileName(OnlyFileName,
    osRemote, FLAGSET(Flags, tfFirstLevel));
  std::wstring DestFullName = TargetDir + DestFileName;

  if (File->IsDirectory)
  {
    Action.Cancel();
    if (!File->IsSymLink)
    {
      FILE_OPERATION_LOOP (FMTLOAD(NOT_DIRECTORY_ERROR, (DestFullName)),
        int Attrs = FileGetAttr(DestFullName);
        if ((Attrs & faDirectory) == 0) EXCEPTION;
      );

      FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, (DestFullName)),
        if (!ForceDirectories(DestFullName)) RaiseLastOSError();
      );

      TSinkFileParams SinkFileParams;
      SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
      SinkFileParams.CopyParam = CopyParam;
      SinkFileParams.Params = Params;
      SinkFileParams.OperationProgress = OperationProgress;
      SinkFileParams.Skipped = false;
      SinkFileParams.Flags = Flags & ~tfFirstLevel;

      FTerminal->ProcessDirectory(FileName, SFTPSinkFile, &SinkFileParams);

      // Do not delete directory if some of its files were skip.
      // Throw "skip file" for the directory to avoid attempt to deletion
      // of any parent directory
      if ((Params & cpDelete) && SinkFileParams.Skipped)
      {
        THROW_SKIP_FILE_NULL;
      }
    }
    else
    {
      // file is symlink to directory, currently do nothing, but it should be
      // reported to user
    }
  }
  else
  {
    FTerminal->LogEvent(FORMAT("Copying \"%s\" to local directory started.", (FileName)));

    std::wstring DestPartinalFullName;
    bool ResumeAllowed;
    bool ResumeTransfer = false;
    __int64 ResumeOffset;

    // Will we use ASCII of BINARY file tranfer?
    OperationProgress->SetAsciiTransfer(
      CopyParam->UseAsciiTransfer(FileName, osRemote, MaskParams));
    FTerminal->LogEvent(std::wstring((OperationProgress->AsciiTransfer ? "Ascii" : "Binary")) +
      " transfer mode selected.");

    // Suppose same data size to transfer as to write
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(File->Size);
    OperationProgress->SetLocalSize(OperationProgress->TransferSize);

    // resume has no sense for temporary downloads
    ResumeAllowed = ((Params & cpTemporary) == 0) &&
      !OperationProgress->AsciiTransfer &&
      CopyParam->AllowResume(OperationProgress->TransferSize);
    OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

    int Attrs;
    FILE_OPERATION_LOOP (FMTLOAD(NOT_FILE_ERROR, (DestFullName)),
      Attrs = FileGetAttr(DestFullName);
      if ((Attrs >= 0) && (Attrs & faDirectory)) EXCEPTION;
    );

    OperationProgress->TransferingFile = false; // not set with SFTP protocol

    HANDLE LocalHandle = NULL;
    TStream * FileStream = NULL;
    bool DeleteLocalFile = false;
    std::wstring RemoteHandle;
    std::wstring LocalFileName = DestFullName;
    TSFTPOverwriteMode OverwriteMode = omOverwrite;

    try
    {
      if (ResumeAllowed)
      {
        DestPartinalFullName = DestFullName + FTerminal->Configuration->PartialExt;
        LocalFileName = DestPartinalFullName;

        FTerminal->LogEvent("Checking existence of partially transfered file.");
        if (FileExists(DestPartinalFullName))
        {
          FTerminal->OpenLocalFile(DestPartinalFullName, GENERIC_WRITE,
            NULL, &LocalHandle, NULL, NULL, NULL, &ResumeOffset);

          bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->TransferSize);
          if (FLAGCLEAR(Params, cpNoConfirmation))
          {
            ResumeTransfer = SFTPConfirmResume(DestFileName,
              PartialBiggerThanSource, OperationProgress);
          }
          else
          {
            ResumeTransfer = !PartialBiggerThanSource;
          }

          if (!ResumeTransfer)
          {
            CloseHandle(LocalHandle);
            LocalHandle = NULL;
            FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, (DestPartinalFullName)),
              THROWOSIFFALSE(Sysutils::DeleteFile(DestPartinalFullName));
            )
          }
          else
          {
            FTerminal->LogEvent("Resuming file transfer.");
            FileSeek((THandle)LocalHandle, ResumeOffset, 0);
            OperationProgress->AddResumed(ResumeOffset);
          }
        }
      }

      if ((Attrs >= 0) && !ResumeTransfer)
      {
        __int64 DestFileSize;
        __int64 MTime;
        FTerminal->OpenLocalFile(DestFullName, GENERIC_WRITE,
          NULL, &LocalHandle, NULL, &MTime, NULL, &DestFileSize, false);

        FTerminal->LogEvent("Confirming overwriting of file.");
        TOverwriteFileParams FileParams;
        FileParams.SourceSize = OperationProgress->TransferSize;
        FileParams.SourceTimestamp = File->Modification;
        FileParams.DestTimestamp = UnixToDateTime(MTime,
          FTerminal->GetSessionData()->DSTMode);
        FileParams.DestSize = DestFileSize;
        std::wstring PrevDestFileName = DestFileName;
        SFTPConfirmOverwrite(DestFileName, Params, OperationProgress, OverwriteMode, &FileParams);
        if (PrevDestFileName != DestFileName)
        {
          DestFullName = TargetDir + DestFileName;
          DestPartinalFullName = DestFullName + FTerminal->Configuration->PartialExt;
          if (ResumeAllowed)
          {
            if (FileExists(DestPartinalFullName))
            {
              FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, (DestPartinalFullName)),
                THROWOSIFFALSE(Sysutils::DeleteFile(DestPartinalFullName));
              )
            }
            LocalFileName = DestPartinalFullName;
          }
          else
          {
            LocalFileName = DestFullName;
          }
        }

        if (OverwriteMode == omOverwrite)
        {
          // is NULL when overwritting read-only file
          if (LocalHandle)
          {
            CloseHandle(LocalHandle);
            LocalHandle = NULL;
          }
        }
        else
        {
          // is NULL when overwritting read-only file, so following will
          // probably fail anyway
          if (LocalHandle == NULL)
          {
            FTerminal->OpenLocalFile(DestFullName, GENERIC_WRITE,
              NULL, &LocalHandle, NULL, NULL, NULL, NULL);
          }
          ResumeAllowed = false;
          FileSeek((THandle)LocalHandle, DestFileSize, 0);
          if (OverwriteMode == omAppend)
          {
            FTerminal->LogEvent("Appending to file.");
          }
          else
          {
            FTerminal->LogEvent("Resuming file transfer (append style).");
            assert(OverwriteMode == omResume);
            OperationProgress->AddResumed(DestFileSize);
          }
        }
      }

      // if not already opened (resume, append...), create new empty file
      if (!LocalHandle)
      {
        if (!FTerminal->CreateLocalFile(LocalFileName, OperationProgress,
               &LocalHandle, FLAGSET(Params, cpNoConfirmation)))
        {
          THROW_SKIP_FILE_NULL;
        }
      }
      assert(LocalHandle);

      Action.Destination(ExpandUNCFileName(DestFullName));

      DeleteLocalFile = true;

      FTerminal->LogEvent("Opening remote file.");
      FILE_OPERATION_LOOP (FMTLOAD(SFTP_OPEN_FILE_ERROR, (FileName)),
        int OpenType = SSH_FXF_READ;
        if ((FVersion >= 4) && OperationProgress->AsciiTransfer)
        {
          OpenType |= SSH_FXF_TEXT;
        }
        RemoteHandle = SFTPOpenRemoteFile(FileName, OpenType);
      );

      TSFTPPacket RemoteFilePacket(SSH_FXP_FSTAT);
      if (CopyParam->PreserveTime)
      {
        SendCustomReadFile(&RemoteFilePacket, &RemoteFilePacket, RemoteHandle,
          SSH_FILEXFER_ATTR_MODIFYTIME);
      }

      FileStream = new TSafeHandleStream((THandle)LocalHandle);

      // at end of this block queue is discarded
      {
        TSFTPDownloadQueue Queue(this);
        try
        {
          TSFTPPacket DataPacket;

          int QueueLen = int(File->Size / DownloadBlockSize(OperationProgress)) + 1;
          if ((QueueLen > FTerminal->GetSessionData()->SFTPDownloadQueue) ||
              (QueueLen < 0))
          {
            QueueLen = FTerminal->GetSessionData()->SFTPDownloadQueue;
          }
          if (QueueLen < 1)
          {
            QueueLen = 1;
          }
          Queue.Init(QueueLen, RemoteHandle, OperationProgress->TransferedSize,
            OperationProgress);

          bool Eof = false;
          bool PrevIncomplete = false;
          int GapFillCount = 0;
          int GapCount = 0;
          unsigned long Missing = 0;
          unsigned long DataLen = 0;
          unsigned long BlockSize;
          bool ConvertToken = false;

          while (!Eof)
          {
            if (Missing > 0)
            {
              Queue.InitFillGapRequest(OperationProgress->TransferedSize, Missing,
                &DataPacket);
              GapFillCount++;
              SendPacketAndReceiveResponse(&DataPacket, &DataPacket,
                SSH_FXP_DATA, asEOF);
            }
            else
            {
              Queue.ReceivePacket(&DataPacket, BlockSize);
            }

            if (DataPacket.Type == SSH_FXP_STATUS)
            {
              // must be SSH_FX_EOF, any other status packet would raise exception
              Eof = true;
              // close file right away, before waiting for pending responses
              SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress,
                true, true, NULL);
              RemoteHandle = ""; // do not close file again in __finally block
            }

            if (!Eof)
            {
              if ((Missing == 0) && PrevIncomplete)
              {
                // This can happen only if last request returned less bytes
                // than expected, but exacly number of bytes missing to last
                // known file size, but actually EOF was not reached.
                // Can happen only when filesize has changed since directory
                // listing and server returns less bytes than requested and
                // file has some special file size.
                FTerminal->LogEvent(FORMAT(
                  "Received incomplete data packet before end of file, "
                  "offset: %s, size: %d, requested: %d",
                  (IntToStr(OperationProgress->TransferedSize), int(DataLen),
                  int(BlockSize))));
                FTerminal->TerminalError(NULL, LoadStr(SFTP_INCOMPLETE_BEFORE_EOF));
              }

              // Buffer for one block of data
              TFileBuffer BlockBuf;

              DataLen = DataPacket.GetCardinal();

              PrevIncomplete = false;
              if (Missing > 0)
              {
                assert(DataLen <= Missing);
                Missing -= DataLen;
              }
              else if (DataLen < BlockSize)
              {
                if (OperationProgress->TransferedSize + DataLen !=
                      OperationProgress->TransferSize)
                {
                  // with native text transfer mode (SFTP>=4), do not bother about
                  // getting less than requested, read offset is ignored anyway
                  if ((FVersion < 4) || !OperationProgress->AsciiTransfer)
                  {
                    GapCount++;
                    Missing = BlockSize - DataLen;
                  }
                }
                else
                {
                  PrevIncomplete = true;
                }
              }

              assert(DataLen <= BlockSize);
              BlockBuf.Insert(0, DataPacket.GetNextData(DataLen), DataLen);
              OperationProgress->AddTransfered(DataLen);

              if (OperationProgress->AsciiTransfer)
              {
                assert(!ResumeTransfer && !ResumeAllowed);

                unsigned int PrevBlockSize = BlockBuf.Size;
                BlockBuf.Convert(GetEOL(), FTerminal->Configuration->LocalEOLType, 0, ConvertToken);
                OperationProgress->SetLocalSize(
                  OperationProgress->LocalSize - PrevBlockSize + BlockBuf.Size);
              }

              FILE_OPERATION_LOOP (FMTLOAD(WRITE_ERROR, (LocalFileName)),
                BlockBuf.WriteToStream(FileStream, BlockBuf.Size);
              );

              OperationProgress->AddLocalyUsed(BlockBuf.Size);
            }

            if (OperationProgress->Cancel == csCancel)
            {
              Abort();
            }
          };

          if (GapCount > 0)
          {
            FTerminal->LogEvent(FORMAT(
              "%d requests to fill %d data gaps were issued.",
              (GapFillCount, GapCount)));
          }
        }
        __finally
        {
          Queue.DisposeSafe();
        }
        // queue is discarded here
      }

      if (CopyParam->PreserveTime)
      {
        ReceiveResponse(&RemoteFilePacket, &RemoteFilePacket);

        const TRemoteFile * AFile = File;
        try
        {
          // ignore errors
          if (RemoteFilePacket.Type == SSH_FXP_ATTRS)
          {
            // load file, avoid completion (resolving symlinks) as we do not need that
            AFile = LoadFile(&RemoteFilePacket, NULL, UnixExtractFileName(FileName),
              NULL, false);
          }

          FILETIME AcTime = DateTimeToFileTime(AFile->LastAccess,
            FTerminal->GetSessionData()->DSTMode);
          FILETIME WrTime = DateTimeToFileTime(AFile->Modification,
            FTerminal->GetSessionData()->DSTMode);
          SetFileTime(LocalHandle, NULL, &AcTime, &WrTime);
        }
        __finally
        {
          if (AFile != File)
          {
            delete AFile;
          }
        }
      }

      CloseHandle(LocalHandle);
      LocalHandle = NULL;

      if (ResumeAllowed)
      {
        FILE_OPERATION_LOOP(FMTLOAD(RENAME_AFTER_RESUME_ERROR,
            (ExtractFileName(DestPartinalFullName), DestFileName)),

          if (FileExists(DestFullName))
          {
            THROWOSIFFALSE(Sysutils::DeleteFile(DestFullName));
          }
          if (!Sysutils::RenameFile(DestPartinalFullName, DestFullName))
          {
            RaiseLastOSError();
          }
        );
      }

      DeleteLocalFile = false;

      if (Attrs == -1)
      {
        Attrs = faArchive;
      }
      int NewAttrs = CopyParam->LocalFileAttrs(*File->Rights);
      if ((NewAttrs & Attrs) != NewAttrs)
      {
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, (DestFullName)),
          THROWOSIFFALSE(FileSetAttr(DestFullName, Attrs | NewAttrs) == 0);
        );
      }

    }
    __finally
    {
      if (LocalHandle) CloseHandle(LocalHandle);
      if (FileStream) delete FileStream;
      if (DeleteLocalFile && (!ResumeAllowed || OperationProgress->LocalyUsed == 0) &&
          (OverwriteMode == omOverwrite))
      {
        FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, (LocalFileName)),
          THROWOSIFFALSE(Sysutils::DeleteFile(LocalFileName));
        )
      }

      // if the transfer was finished, the file is closed already
      if (FTerminal->GetActive() && !RemoteHandle.IsEmpty())
      {
        // do not wait for response
        SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress,
          true, true, NULL);
      }
    }
  }

  if (Params & cpDelete)
  {
    ChildError = true;
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    int Params = dfNoRecursive;
    FTerminal->DeleteFile(FileName, File, &Params);
    ChildError = false;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSinkFile(std::wstring FileName,
  const TRemoteFile * File, void * Param)
{
  TSinkFileParams * Params = (TSinkFileParams *)Param;
  assert(Params->OperationProgress);
  try
  {
    SFTPSinkRobust(FileName, File, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch(EScpSkipFile & E)
  {
    TFileOperationProgressType * OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    SUSPEND_OPERATION (
      if (!FTerminal->HandleException(&E)) throw;
    );

    if (OperationProgress->Cancel)
    {
      Abort();
    }
  }
}
