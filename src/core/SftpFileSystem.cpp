//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

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
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(Self->FTerminal, ALLOW_SKIP, MESSAGE, OPERATION)
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
#define SFTP_EXT_OWNER_GROUP L"owner-group-query@generic-extensions"
#define SFTP_EXT_OWNER_GROUP_REPLY L"owner-group-query-reply@generic-extensions"
#define SFTP_EXT_NEWLINE L"newline"
#define SFTP_EXT_SUPPORTED L"supported"
#define SFTP_EXT_SUPPORTED2 L"supported2"
#define SFTP_EXT_FSROOTS L"fs-roots@vandyke.com"
#define SFTP_EXT_VENDOR_ID L"vendor-id"
#define SFTP_EXT_VERSIONS L"versions"
#define SFTP_EXT_SPACE_AVAILABLE L"space-available"
#define SFTP_EXT_CHECK_FILE L"check-file"
#define SFTP_EXT_CHECK_FILE_NAME L"check-file-name"
//---------------------------------------------------------------------------
#define OGQ_LIST_OWNERS 0x01
#define OGQ_LIST_GROUPS 0x02
//---------------------------------------------------------------------------
const size_t SFTPMinVersion = 0;
const size_t SFTPMaxVersion = 5;
const size_t SFTPNoMessageNumber = NPOS;

const int asNo =            0;
const int asOK =            1 << SSH_FX_OK;
const int asEOF =           1 << SSH_FX_EOF;
const int asPermDenied =    1 << SSH_FX_PERMISSION_DENIED;
const int asOpUnsupported = 1 << SSH_FX_OP_UNSUPPORTED;
const int asNoSuchFile =    1 << SSH_FX_NO_SUCH_FILE;
const int asAll = 0xFFFF;

//---------------------------------------------------------------------------
#ifndef GET_32BIT
#define GET_32BIT(cp) \
    (((unsigned long)(unsigned char)(cp)[0] << 24) | \
    ((unsigned long)(unsigned char)(cp)[1] << 16) | \
    ((unsigned long)(unsigned char)(cp)[2] << 8) | \
    ((unsigned long)(unsigned char)(cp)[3]))
#endif
#ifndef PUT_32BIT
#define PUT_32BIT(cp, value) { \
    (cp)[0] = (unsigned char)((value) >> 24); \
    (cp)[1] = (unsigned char)((value) >> 16); \
    (cp)[2] = (unsigned char)((value) >> 8); \
    (cp)[3] = (unsigned char)(value); }
#endif
//---------------------------------------------------------------------------
#define SFTP_PACKET_ALLOC_DELTA 256
//---------------------------------------------------------------------------
struct TSFTPSupport
{
    TSFTPSupport() :
        AttribExtensions(new System::TStringList()),
        Extensions(new System::TStringList())
    {
        Reset();
    }

    ~TSFTPSupport()
    {
        delete AttribExtensions;
        delete Extensions;
    }

    void __fastcall Reset()
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

    System::TStrings *AttribExtensions;
    System::TStrings *Extensions;
    size_t AttributeMask;
    size_t AttributeBits;
    size_t OpenFlags;
    size_t AccessMask;
    size_t MaxReadSize;
    size_t OpenBlockMasks;
    size_t BlockMasks;
    bool Loaded;
};
//---------------------------------------------------------------------------
class TSFTPPacket
{
public:
    explicit TSFTPPacket(unsigned int codePage)
    {
        Init(codePage);
    }

    explicit TSFTPPacket(const TSFTPPacket &Source, unsigned int codePage)
    {
        Init(codePage);
        *this = Source;
    }

    explicit TSFTPPacket(size_t AType, unsigned int codePage)
    {
        Init(codePage);
        ChangeType(AType);
    }

    explicit TSFTPPacket(const char *Source, size_t Len, unsigned int codePage)
    {
        Init(codePage);
        FLength = Len;
        SetCapacity (FLength);
        memmove(GetData(), Source, Len);
    }

    explicit TSFTPPacket(const UnicodeString Source, unsigned int codePage)
    {
        Init(codePage);
        FLength = Source.Length() * sizeof(wchar_t);
        SetCapacity(FLength);
        memmove(GetData(), Source.c_str(), FLength);
    }

    ~TSFTPPacket()
    {
        if (FData != NULL)
        {
            delete[] (FData - FSendPrefixLen);
        }
        if (FReservedBy) { FReservedBy->UnreserveResponse(this); }
    }

    void __fastcall ChangeType(size_t AType)
    {
        FPosition = 0;
        FLength = 0;
        SetCapacity (0);
        FType = static_cast<unsigned char>(AType);
        AddByte(FType);
        if ((FType != static_cast<unsigned char>(-1)) && (FType != SSH_FXP_INIT))
        {
            AssignNumber();
            AddCardinal(FMessageNumber);
        }
    }

    void __fastcall Reuse()
    {
        AssignNumber();

        assert(GetLength() >= 5);

        // duplicated in AddCardinal()
        unsigned char Buf[4];
        PUT_32BIT(Buf, FMessageNumber);

        memmove(FData + 1, Buf, sizeof(Buf));
    }

    void __fastcall AddByte(unsigned char Value)
    {
        Add(&Value, sizeof(Value));
    }

    void __fastcall AddCardinal(size_t Value)
    {
        // duplicated in Reuse()
        unsigned char Buf[4];
        PUT_32BIT(Buf, Value);
        Add(&Buf, sizeof(Buf));
    }

    void __fastcall AddInt64(__int64 Value)
    {
        AddCardinal(static_cast<size_t>(Value >> 32));
        AddCardinal(static_cast<size_t>(Value & 0xFFFFFFFF));
    }

    void __fastcall AddData(const void *Data, size_t ALength)
    {
        AddCardinal(ALength);
        Add(Data, ALength);
    }

    void __fastcall AddStringA(const std::string &ValueA)
    {
        // std::string ValueA = System::W2MB(Value.c_str(), FCodePage);
        AddCardinal(ValueA.Length());
        Add(ValueA.c_str(), ValueA.Length());
    }

    void __fastcall AddStringW(const UnicodeString ValueW)
    {
        std::string ValueA = System::W2MB(ValueW.c_str(), FCodePage);
        AddStringA(ValueA);
    }

    inline void __fastcall AddString(const UnicodeString Value)
    {
        AddStringW(Value);
    }

    // now purposeless alias to AddString
    inline void __fastcall AddPathString(const UnicodeString Value)
    {
        AddString(Value);
    }

    void __fastcall AddProperties(unsigned short *Rights, TRemoteToken *Owner,
                       TRemoteToken *Group, __int64 *MTime, __int64 *ATime,
                       __int64 *Size, bool IsDirectory, size_t Version)
    {
        size_t Flags = 0;
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
                // assert(Owner->GetIDValid() && Group->GetIDValid());
                Flags |= SSH_FILEXFER_ATTR_UIDGID;
            }
            else
            {
                assert(Owner->GetNameValid() && Group->GetNameValid());
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
                // assert(Owner->GetIDValid() && Group->GetIDValid());
                AddCardinal(Owner->GetID());
                AddCardinal(Group->GetID());
            }
            else
            {
                assert(Owner->GetNameValid() && Group->GetNameValid());
                AddString(Owner->GetName());
                AddString(Group->GetName());
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
            AddCardinal(static_cast<size_t>(ATime != NULL ? *ATime : *MTime));
            AddCardinal(static_cast<size_t>(MTime != NULL ? *MTime : *ATime));
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

    void __fastcall AddProperties(const TRemoteProperties *Properties,
                       unsigned short BaseRights, bool IsDirectory, size_t Version,
                       TChmodSessionAction *Action)
    {
        enum ValidEnum { valNone = 0, valRights = 0x01, valOwner = 0x02, valGroup = 0x04,
                         valMTime = 0x08, valATime = 0x10
                       };
        int Valid = valNone;
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
                TRights Rights(BaseRights);
                Rights |= Properties->Rights.GetNumberSet();
                Rights &= static_cast<unsigned short>(~Properties->Rights.GetNumberUnset());
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
            NULL, IsDirectory, Version);
    }

    char GetByte()
    {
        Need(sizeof(char));
        char Result = FData[FPosition];
        FPosition++;
        return Result;
    }

    size_t GetCardinal()
    {
        size_t Result = 0;
        Need(sizeof(int));
        Result = GET_32BIT(FData + FPosition);
        FPosition += sizeof(int);
        return Result;
    }

    size_t GetSmallCardinal()
    {
        size_t Result = 0;
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

    std::string GetStringA()
    {
        std::string ResultA;
        size_t Len = GetCardinal();
        Need(Len);
        // cannot happen anyway as Need() would raise exception
        assert(Len < SFTP_MAX_PACKET_LEN);
        ResultA.SetLength(Len);
        memmove(const_cast<char *>(ResultA.c_str()), FData + FPosition, Len);
        FPosition += Len;
        // DEBUG_PRINTF(L"Result = %s", System::MB2W(ResultA.c_str(), FCodePage).c_str());
        return ResultA;
    }

    UnicodeString GetStringW()
    {
        UnicodeString Result = System::MB2W(GetStringA().c_str(), FCodePage);
        return Result;
    }

    inline std::string GetString()
    {
        return GetStringA();
    }

    // now purposeless alias to GetString
    inline UnicodeString GetPathString()
    {
        UnicodeString result = System::MB2W(GetString().c_str(), FCodePage);
        // DEBUG_PRINTF(L"result = %s", result.c_str());
        return result;
    }

    void __fastcall GetFile(TRemoteFile *File, size_t Version, TDSTMode DSTMode, bool SignedTS, bool Complete)
    {
        assert(File);
        size_t Flags;
        UnicodeString ListingStr;
        size_t Permissions = 0;
        bool ParsingFailed = false;
        if (GetType() != SSH_FXP_ATTRS)
        {
            File->SetFileName(GetPathString());
            if (Version < 4)
            {
                ListingStr = GetStringW();
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
            static char *Types = "U-DLSUOCBF";
            if (FXType < 0 || FXType > static_cast<char>(strlen(Types)))
            {
                throw ExtException(FMTLOAD(SFTP_UNKNOWN_FILE_TYPE, static_cast<int>(FXType)));
            }
            File->SetType(Types[FXType]);
        }
        if (Flags & SSH_FILEXFER_ATTR_SIZE)
        {
            File->SetSize(GetInt64());
        }
        // SSH-2.0-3.2.0 F-SECURE SSH - Process Software MultiNet
        // sets SSH_FILEXFER_ATTR_UIDGID for v4, but does not include the UID/GUID
        if ((Flags & SSH_FILEXFER_ATTR_UIDGID) && (Version < 4))
        {
            File->GetOwner().SetID(GetCardinal());
            File->GetGroup().SetID(GetCardinal());
        }
        if (Flags & SSH_FILEXFER_ATTR_OWNERGROUP)
        {
            assert(Version >= 4);
            File->GetOwner().SetName(GetStringW());
            File->GetGroup().SetName(GetStringW());
        }
        if (Flags & SSH_FILEXFER_ATTR_PERMISSIONS)
        {
            Permissions = GetCardinal();
        }
        if (Version < 4)
        {
            if (Flags & SSH_FILEXFER_ATTR_ACMODTIME)
            {
                File->SetLastAccess(UnixToDateTime(
                                        SignedTS ?
                                        static_cast<__int64>(static_cast<signed long>(GetCardinal())) :
                                        static_cast<__int64>(GetCardinal()),
                                        DSTMode));
                File->SetModification(UnixToDateTime(
                                          SignedTS ?
                                          static_cast<__int64>(static_cast<signed long>(GetCardinal())) :
                                          static_cast<__int64>(GetCardinal()),
                                          DSTMode));
            }
        }
        else
        {
            if (Flags & SSH_FILEXFER_ATTR_ACCESSTIME)
            {
                File->SetLastAccess(UnixToDateTime(GetInt64(), DSTMode));
                if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
                {
                    GetCardinal(); // skip access time subseconds
                }
            }
            else
            {
                File->SetLastAccess(System::Now());
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
                File->SetModification(UnixToDateTime(GetInt64(), DSTMode));
                if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
                {
                    GetCardinal(); // skip modification time subseconds
                }
            }
            else
            {
                File->SetModification(System::Now());
            }
        }

        if (Flags & SSH_FILEXFER_ATTR_ACL)
        {
            GetStringA();
        }

        if (Flags & SSH_FILEXFER_ATTR_BITS)
        {
            // while SSH_FILEXFER_ATTR_BITS is defined for SFTP5 only, vandyke 2.3.3 sets it
            // for SFTP4 as well
            size_t Bits = GetCardinal();
            if (FLAGSET(Bits, SSH_FILEXFER_ATTR_FLAGS_HIDDEN))
            {
                File->SetIsHidden(true);
            }
        }

        if ((Version < 4) && (GetType() != SSH_FXP_ATTRS))
        {
            try
            {
                // update permissions and user/group name
                // modification time and filename is ignored
                File->SetListingStr(ListingStr);
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
            char Type = '-';
            if (FLAGSET(Flags, SSH_FILEXFER_ATTR_PERMISSIONS))
            {
                File->GetRights()->SetNumber(static_cast<unsigned short>(Permissions & TRights::rfAllSpecials));
                if (FLAGSET(Permissions, TRights::rfDirectory))
                {
                    Type = FILETYPE_DIRECTORY;
                }
            }

            if (Version < 4)
            {
                File->SetType(Type);
            }
        }

        if (Flags & SSH_FILEXFER_ATTR_EXTENDED)
        {
            size_t ExtendedCount = GetCardinal();
            for (size_t Index = 0; Index < ExtendedCount; Index++)
            {
                GetStringA(); // skip extended_type
                GetStringA(); // skip extended_data
            }
        }

        if (Complete)
        {
            File->Complete();
        }
    }

    char *GetNextData(size_t Size = 0)
    {
        if (Size > 0)
        {
            Need(Size);
        }
        return FPosition < FLength ? FData + FPosition : NULL;
    }

    void __fastcall DataUpdated(size_t ALength)
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

    void __fastcall LoadFromFile(const UnicodeString FileName)
    {
        System::TStringList *DumpLines = new System::TStringList();
        UnicodeString Dump;
        {
            BOOST_SCOPE_EXIT ( (&DumpLines) )
            {
                delete DumpLines;
            } BOOST_SCOPE_EXIT_END
            DumpLines->LoadFromFile(FileName);
            Dump = DumpLines->GetText();
        }

        SetCapacity(1 * 1024 * 1024); // 20480);
        wchar_t Byte[3];
        memset(Byte, '\0', sizeof(Byte));
        size_t Index = 0;
        size_t Length = 0;
        while (Index < Dump.Length())
        {
            wchar_t C = Dump[Index];
            if (((C >= L'0') && (C <= L'9')) || ((C >= L'A') && (C <= L'Z')))
            {
                if (Byte[0] == L'\0')
                {
                    Byte[0] = C;
                }
                else
                {
                    Byte[1] = C;
                    assert(GetLength() < GetCapacity());
                    GetData()[GetLength()] = static_cast<unsigned char>(HexToInt(Byte));
                    Length++;
                    memset(Byte, '\0', sizeof(Byte));
                }
            }
            Index++;
        }
        DataUpdated(GetLength());
    }

    UnicodeString Dump() const
    {
        UnicodeString Result;
        for (size_t Index = 0; Index < GetLength(); Index++)
        {
            Result += CharToHex(GetData()[Index]) + L",";
            if (((Index + 1) % 25) == 0)
            {
                Result += L"\n";
            }
        }
        return Result;
    }

    TSFTPPacket &operator = (const TSFTPPacket &Source)
    {
        SetCapacity(0);
        Add(Source.GetData(), Source.GetLength());
        DataUpdated(Source.GetLength());
        FPosition = Source.FPosition;
        return *this;
    }

    size_t GetLength() const { return FLength; }
    size_t GetRemainingLength() const
    {
        return GetLength() - FPosition;
    }

    char *GetData() const { return FData; }
    char *GetSendData() const
    {
        char *Result = FData - FSendPrefixLen;
        // this is not strictly const-object operation
        PUT_32BIT(Result, GetLength());
        return Result;
    }

    size_t GetSendLength() const
    {
        return FSendPrefixLen + GetLength();
    }

    size_t GetCapacity() const { return FCapacity; }
    void __fastcall SetCapacity(size_t ACapacity)
    {
        if (ACapacity != GetCapacity())
        {
            FCapacity = ACapacity;
            if (FCapacity > 0)
            {
                char *NData = (new char[FCapacity + FSendPrefixLen]) + FSendPrefixLen;
                if (FData)
                {
                    memmove(NData - FSendPrefixLen, FData - FSendPrefixLen,
                           (FLength < FCapacity ? FLength : FCapacity) + FSendPrefixLen);
                    delete[] (FData - FSendPrefixLen);
                }
                FData = NData;
            }
            else
            {
                if (FData) { delete[] (FData - FSendPrefixLen); }
                FData = NULL;
            }
            if (FLength > FCapacity) { FLength = FCapacity; }
        }
    }

    unsigned char GetType() const { return FType; }
    unsigned char GetRequestType()
    {
        if (FMessageNumber != SFTPNoMessageNumber)
        {
            return static_cast<unsigned char>(FMessageNumber & 0xFF);
        }
        else
        {
            assert(GetType() == SSH_FXP_VERSION);
            return SSH_FXP_INIT;
        }
    }

    size_t GetMessageNumber() const { return FMessageNumber; }
    void __fastcall SetMessageNumber(size_t value) { FMessageNumber = value; }
    TSFTPFileSystem *GetReservedBy() const { return FReservedBy; }
    void __fastcall SetReservedBy(TSFTPFileSystem *value) { FReservedBy = value; }
    UnicodeString GetTypeName() const
    {
#define TYPE_CASE(TYPE) case TYPE: return System::MB2W(#TYPE, FCodePage)
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
            TYPE_CASE(SSH_FXP_STATUS);
            TYPE_CASE(SSH_FXP_HANDLE);
            TYPE_CASE(SSH_FXP_DATA);
            TYPE_CASE(SSH_FXP_NAME);
            TYPE_CASE(SSH_FXP_ATTRS);
            TYPE_CASE(SSH_FXP_EXTENDED);
            TYPE_CASE(SSH_FXP_EXTENDED_REPLY);
        default:
            return FORMAT(L"Unknown message (%d)", static_cast<int>(GetType()));
        }
    }


private:
    char *FData;
    size_t FLength;
    size_t FCapacity;
    size_t FPosition;
    unsigned char FType;
    size_t FMessageNumber;
    TSFTPFileSystem *FReservedBy;

    static int FMessageCounter;
    static const int FSendPrefixLen = 4;
    unsigned int FCodePage;
    TSFTPPacket *Self;

    void __fastcall Init(unsigned int codePage)
    {
        FData = NULL;
        FCapacity = 0;
        FLength = 0;
        FPosition = 0;
        FMessageNumber = SFTPNoMessageNumber;
        FType = static_cast<unsigned char>(-1);
        FReservedBy = NULL;
        FCodePage = codePage;
        Self = this;
    }

    void __fastcall AssignNumber()
    {
        // this is not strictly thread-safe, but as it is accessed from multiple
        // threads only for multiple connection, it is not problem if two threads get
        // the same number
        FMessageNumber = (FMessageCounter << 8) + FType;
        FMessageCounter++;
    }

    inline void __fastcall Add(const void *AData, size_t ALength)
    {
        if (GetLength() + ALength > GetCapacity())
        {
            SetCapacity(GetLength() + ALength + SFTP_PACKET_ALLOC_DELTA);
        }
        memmove(FData + GetLength(), AData, ALength);
        FLength += ALength;
    }

    inline void __fastcall Need(size_t Size)
    {
        if (FPosition + Size > FLength)
        {
            throw ExtException(FMTLOAD(SFTP_PACKET_ERROR, static_cast<int>(FPosition), static_cast<int>(Size), static_cast<int>(FLength)));
        }
    }
};
//---------------------------------------------------------------------------
int TSFTPPacket::FMessageCounter = 0;
//---------------------------------------------------------------------------
class TSFTPQueue
{
public:
    TSFTPQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage)
    {
        FFileSystem = AFileSystem;
        assert(FFileSystem);
        FRequests = new System::TList();
        FResponses = new System::TList();
        FCodePage = codePage;
    }

    virtual ~TSFTPQueue()
    {
        TSFTPQueuePacket *Request;
        TSFTPPacket *Response;

        assert(FResponses->GetCount() == FRequests->GetCount());
        for (size_t Index = 0; Index < FRequests->GetCount(); Index++)
        {
            Request = reinterpret_cast<TSFTPQueuePacket *>(FRequests->GetItem(Index));
            assert(Request);
            delete Request;

            Response = reinterpret_cast<TSFTPPacket *>(FResponses->GetItem(Index));
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

    virtual void __fastcall Dispose()
    {
        assert(FFileSystem->FTerminal->GetActive());

        TSFTPQueuePacket *Request;
        TSFTPPacket *Response;

        while (FRequests->GetCount())
        {
            assert(FResponses->GetCount());

            Request = reinterpret_cast<TSFTPQueuePacket *>(FRequests->GetItem(0));
            assert(Request);

            Response = reinterpret_cast<TSFTPPacket *>(FResponses->GetItem(0));
            assert(Response);

            try
            {
                FFileSystem->ReceiveResponse(Request, Response);
            }
            catch (const std::exception &E)
            {
                if (FFileSystem->FTerminal->GetActive())
                {
                    FFileSystem->FTerminal->LogEvent(L"Error while disposing the SFTP queue.");
                    FFileSystem->FTerminal->GetLog()->AddException(&E);
                }
                else
                {
                    FFileSystem->FTerminal->LogEvent(L"Fatal error while disposing the SFTP queue.");
                    throw;
                }
            }

            FRequests->Delete(0);
            delete Request;
            FResponses->Delete(0);
            delete Response;
        }
    }

    void __fastcall DisposeSafe()
    {
        if (FFileSystem->FTerminal->GetActive())
        {
            Dispose();
        }
    }

    bool ReceivePacket(TSFTPPacket *Packet,
                       size_t ExpectedType = -1, size_t AllowStatus = -1, void **Token = NULL)
    {
        assert(FRequests->GetCount());
        bool Result;
        TSFTPQueuePacket *Request = NULL;
        TSFTPPacket *Response = NULL;
        {
            BOOST_SCOPE_EXIT ( (&Request) (&Response) )
            {
                delete Request;
                delete Response;
            } BOOST_SCOPE_EXIT_END
            Request = reinterpret_cast<TSFTPQueuePacket *>(FRequests->GetItem(0));
            FRequests->Delete(0);
            assert(Request);
            if (Token != NULL)
            {
                *Token = Request->Token;
            }

            Response = reinterpret_cast<TSFTPPacket *>(FResponses->GetItem(0));
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

        return Result;
    }

    bool Next(size_t ExpectedType = -1, size_t AllowStatus = -1)
    {
        return ReceivePacket(NULL, ExpectedType, AllowStatus);
    }

protected:
    System::TList *FRequests;
    System::TList *FResponses;
    TSFTPFileSystem *FFileSystem;
    unsigned int FCodePage;

    class TSFTPQueuePacket : public TSFTPPacket
    {
    public:
        TSFTPQueuePacket(unsigned int codePage) :
            TSFTPPacket(codePage)
        {
            Token = NULL;
        }

        void *Token;
    };

    virtual bool InitRequest(TSFTPQueuePacket *Request) = 0;

    virtual bool End(TSFTPPacket *Response) = 0;

    virtual void __fastcall SendPacket(TSFTPQueuePacket *Packet)
    {
        FFileSystem->SendPacket(Packet);
    }

    // sends as many requests as allowed by implementation
    virtual bool SendRequests() = 0;

    virtual bool SendRequest()
    {
        TSFTPQueuePacket *Request = NULL;
        try
        {
            Request = new TSFTPQueuePacket(FCodePage);
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
            TSFTPPacket *Response = new TSFTPPacket(FCodePage);
            FRequests->Add(static_cast<System::TObject *>(static_cast<void *>(Request)));
            FResponses->Add(static_cast<System::TObject *>(static_cast<void *>(Response)));

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
    TSFTPFixedLenQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage) : TSFTPQueue(AFileSystem, codePage)
    {
        FMissedRequests = 0;
    }
    virtual ~TSFTPFixedLenQueue()
    {}

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
    TSFTPAsynchronousQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage) : TSFTPQueue(AFileSystem, codePage)
    {
        FFileSystem->FSecureShell->RegisterReceiveHandler(boost::bind(&TSFTPAsynchronousQueue::ReceiveHandler, this, _1));
        FReceiveHandlerRegistered = true;
    }

    virtual ~TSFTPAsynchronousQueue()
    {
        UnregisterReceiveHandler();
    }

    virtual void __fastcall Dispose()
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
    void ReceiveHandler(System::TObject * /*Sender*/)
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

    void __fastcall UnregisterReceiveHandler()
    {
        if (FReceiveHandlerRegistered)
        {
            FReceiveHandlerRegistered = false;
            FFileSystem->FSecureShell->UnregisterReceiveHandler(boost::bind(&TSFTPAsynchronousQueue::ReceiveHandler, this, _1));
        }
    }

private:
    bool FReceiveHandlerRegistered;
};
//---------------------------------------------------------------------------
class TSFTPDownloadQueue : public TSFTPFixedLenQueue
{
public:
    TSFTPDownloadQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage) :
        TSFTPFixedLenQueue(AFileSystem, codePage)
    {
    }
    virtual ~TSFTPDownloadQueue()
    {}

    bool Init(int QueueLen, const std::string &AHandle, __int64 ATransfered,
              TFileOperationProgressType *AOperationProgress)
    {
        FHandle = AHandle;
        FTransfered = ATransfered;
        OperationProgress = AOperationProgress;

        return TSFTPFixedLenQueue::Init(QueueLen);
    }

    void __fastcall InitFillGapRequest(__int64 Offset, size_t Missing,
                            TSFTPPacket *Packet)
    {
        InitRequest(Packet, Offset, Missing);
    }

    bool ReceivePacket(TSFTPPacket *Packet, size_t &BlockSize)
    {
        void *Token;
        bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_DATA, asEOF, &Token);
        BlockSize = reinterpret_cast<size_t>(Token);
        return Result;
    }

protected:
    virtual bool InitRequest(TSFTPQueuePacket *Request)
    {
        size_t BlockSize = FFileSystem->DownloadBlockSize(OperationProgress);
        InitRequest(Request, FTransfered, BlockSize);
        Request->Token = reinterpret_cast<void *>(BlockSize);
        FTransfered += BlockSize;
        return true;
    }

    void __fastcall InitRequest(TSFTPPacket *Request, __int64 Offset,
                     size_t Size)
    {
        Request->ChangeType(SSH_FXP_READ);
        Request->AddStringA(FHandle);
        Request->AddInt64(Offset);
        Request->AddCardinal(Size);
    }

    virtual bool End(TSFTPPacket *Response)
    {
        return (Response->GetType() != SSH_FXP_DATA);
    }

private:
    TFileOperationProgressType *OperationProgress;
    __int64 FTransfered;
    std::string FHandle;
};
//---------------------------------------------------------------------------
class TSFTPUploadQueue : public TSFTPAsynchronousQueue
{
public:
    TSFTPUploadQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage) :
        TSFTPAsynchronousQueue(AFileSystem, codePage)
    {
        FStream = NULL;
        OperationProgress = NULL;
        FLastBlockSize = 0;
        FEnd = false;
        FConvertToken = false;
        Self = this;
    }

    virtual ~TSFTPUploadQueue()
    {
        delete FStream;
    }

    bool Init(const UnicodeString AFileName,
              HANDLE AFile, TFileOperationProgressType *AOperationProgress,
              const std::string &AHandle, __int64 ATransfered)
    {
        FFileName = AFileName;
        FStream = new TSafeHandleStream(static_cast<HANDLE>(AFile));
        OperationProgress = AOperationProgress;
        FHandle = AHandle;
        FTransfered = ATransfered;

        return TSFTPAsynchronousQueue::Init();
    }

protected:
    virtual bool InitRequest(TSFTPQueuePacket *Request)
    {
        FTerminal = FFileSystem->FTerminal;
        // Buffer for one block of data
        TFileBuffer BlockBuf;

        size_t BlockSize = GetBlockSize();
        bool Result = (BlockSize > 0);

        if (Result)
        {
            FILE_OPERATION_LOOP(FMTLOAD(READ_ERROR, FFileName.c_str()),
                BlockBuf.LoadStream(FStream, BlockSize, false);
            );

            FEnd = (BlockBuf.GetSize() == 0);
            Result = !FEnd;
            if (Result)
            {
                OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

                // We do ASCII transfer: convert EOL of current block
                if (OperationProgress->AsciiTransfer)
                {
                    __int64 PrevBufSize = BlockBuf.GetSize();
                    BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
                                     FFileSystem->GetEOL(), cpRemoveCtrlZ | cpRemoveBOM, FConvertToken);
                    // update transfer size with difference arised from EOL conversion
                    OperationProgress->ChangeTransferSize(OperationProgress->TransferSize -
                                                          PrevBufSize + BlockBuf.GetSize());
                }

                if (FFileSystem->FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
                {
                    FFileSystem->FTerminal->LogEvent(FORMAT(L"Write request offset: %d, len: %d",
                                                            int(FTransfered), int(BlockBuf.GetSize())));
                }

                Request->ChangeType(SSH_FXP_WRITE);
                Request->AddStringA(FHandle);
                Request->AddInt64(FTransfered);
                Request->AddData(BlockBuf.GetData(), BlockBuf.GetSize());
                FLastBlockSize = static_cast<size_t>(BlockBuf.GetSize());

                FTransfered += BlockBuf.GetSize();
            }
        }
        FTerminal = NULL;
        return Result;
    }

    virtual void __fastcall SendPacket(TSFTPQueuePacket *Packet)
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

    inline size_t GetBlockSize()
    {
        return FFileSystem->UploadBlockSize(FHandle, OperationProgress);
    }

    virtual bool End(TSFTPPacket * /*Response*/)
    {
        return FEnd;
    }

private:
    System::TStream *FStream;
    TFileOperationProgressType *OperationProgress;
    UnicodeString FFileName;
    size_t FLastBlockSize;
    bool FEnd;
    __int64 FTransfered;
    std::string FHandle;
    bool FConvertToken;
    TTerminal *FTerminal;
    TSFTPUploadQueue *Self;
};
//---------------------------------------------------------------------------
class TSFTPLoadFilesPropertiesQueue : public TSFTPFixedLenQueue
{
public:
    TSFTPLoadFilesPropertiesQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage) :
        TSFTPFixedLenQueue(AFileSystem, codePage)
    {
        FIndex = 0;
    }
    virtual ~TSFTPLoadFilesPropertiesQueue()
    {}

    bool Init(int QueueLen, System::TStrings *FileList)
    {
        FFileList = FileList;

        return TSFTPFixedLenQueue::Init(QueueLen);
    }

    bool ReceivePacket(TSFTPPacket *Packet, TRemoteFile *& File)
    {
        void *Token;
        bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_ATTRS, asAll, &Token);
        File = static_cast<TRemoteFile *>(Token);
        return Result;
    }

protected:
    virtual bool InitRequest(TSFTPQueuePacket *Request)
    {
        bool Result = false;
        while (!Result && (FIndex < FFileList->GetCount()))
        {
            TRemoteFile *File = reinterpret_cast<TRemoteFile *>(FFileList->GetObjects(FIndex));
            FIndex++;

            bool MissingRights =
                (FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS) &&
                 File->GetRights()->GetUnknown());
            bool MissingOwnerGroup =
                (FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP) &&
                 !File->GetOwner().GetIsSet() || !File->GetGroup().GetIsSet());

            Result = (MissingRights || MissingOwnerGroup);
            if (Result)
            {
                Request->ChangeType(SSH_FXP_LSTAT);
                Request->AddPathString(FFileSystem->LocalCanonify(File->GetFileName()));
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
    System::TStrings *FFileList;
    size_t FIndex;
};
//---------------------------------------------------------------------------
class TSFTPCalculateFilesChecksumQueue : public TSFTPFixedLenQueue
{
public:
    TSFTPCalculateFilesChecksumQueue(TSFTPFileSystem *AFileSystem, unsigned int codePage) :
        TSFTPFixedLenQueue(AFileSystem, codePage)
    {
        FIndex = 0;
    }
    virtual ~TSFTPCalculateFilesChecksumQueue()
    {}

    bool Init(int QueueLen, const UnicodeString Alg, System::TStrings *FileList)
    {
        FAlg = Alg;
        FFileList = FileList;

        return TSFTPFixedLenQueue::Init(QueueLen);
    }

    bool ReceivePacket(TSFTPPacket *Packet, TRemoteFile *& File)
    {
        void *Token;
        bool Result;
        {
            BOOST_SCOPE_EXIT ( (&File) (&Token) )
            {
                File = static_cast<TRemoteFile *>(Token);
            } BOOST_SCOPE_EXIT_END
            Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_EXTENDED_REPLY, asNo, &Token);
        }
        return Result;
    }

protected:
    virtual bool InitRequest(TSFTPQueuePacket *Request)
    {
        bool Result = false;
        while (!Result && (FIndex < FFileList->GetCount()))
        {
            TRemoteFile *File = static_cast<TRemoteFile *>(FFileList->GetObjects(FIndex));
            assert(File != NULL);
            FIndex++;

            Result = !File->GetIsDirectory();
            if (Result)
            {
                assert(!File->GetIsParentDirectory() && !File->GetIsThisDirectory());

                Request->ChangeType(SSH_FXP_EXTENDED);
                Request->AddStringW(SFTP_EXT_CHECK_FILE_NAME);
                Request->AddPathString(FFileSystem->LocalCanonify(File->GetFullFileName()));
                Request->AddStringW(FAlg);
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
    UnicodeString FAlg;
    System::TStrings *FFileList;
    size_t FIndex;
};
//---------------------------------------------------------------------------
class TSFTPBusy
{
public:
    TSFTPBusy(TSFTPFileSystem *FileSystem)
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
    TSFTPFileSystem *FFileSystem;
};

//===========================================================================
TSFTPFileSystem::TSFTPFileSystem(TTerminal *ATerminal) :
    TCustomFileSystem(ATerminal)
{
    Self = this;
}

void __fastcall TSFTPFileSystem::Init(TSecureShell *SecureShell)
{
    FSecureShell = SecureShell;
    FFileSystemInfoValid = false;
    FVersion = NPOS;
    FPacketReservations = new System::TList();
    FPreviousLoggedPacket = 0;
    FNotLoggedPackets = 0;
    FBusy = 0;
    FAvoidBusy = false;
    FUtfStrings = false;
    FUtfNever = false;
    FSignedTS = false;
    FSupport = new TSFTPSupport();
    FExtensions = new System::TStringList();
    FFixedPaths = NULL;
    Self = this;
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
void __fastcall TSFTPFileSystem::Open()
{
    ResetConnection();
    FSecureShell->Open();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Close()
{
    FSecureShell->Close();
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::GetActive()
{
    return FSecureShell->GetActive();
}
//---------------------------------------------------------------------------
const TSessionInfo &TSFTPFileSystem::GetSessionInfo()
{
    return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TSessionData *TSFTPFileSystem::GetSessionData() const
{
    return FTerminal->GetSessionData();
}
//---------------------------------------------------------------------------
const TFileSystemInfo &TSFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
    if (!FFileSystemInfoValid)
    {
        FFileSystemInfo.AdditionalInfo = L"";

        if (!IsCapable(fcRename))
        {
            FFileSystemInfo.AdditionalInfo += LoadStr(FS_RENAME_NOT_SUPPORTED) + L"\r\n\r\n";
        }

        if (FExtensions->GetCount() > 0)
        {
            UnicodeString Name;
            UnicodeString Value;
            UnicodeString Line;
            FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_EXTENSION_INFO) + L"\r\n";
            for (size_t Index = 0; Index < FExtensions->GetCount(); Index++)
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
                    Line = FORMAT(L"%s=%s", Name.c_str(), DisplayableStr(Value).c_str());
                }
                FFileSystemInfo.AdditionalInfo += FORMAT(L"  %s\r\n", Line.c_str());
            }
        }
        else
        {
            FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_NO_EXTENSION_INFO) + L"\r\n";
        }

        FFileSystemInfo.ProtocolBaseName = L"SFTP";
        FFileSystemInfo.ProtocolName = FMTLOAD(SFTP_PROTOCOL_NAME2, FVersion);
        for (int Index = 0; Index < fcCount; Index++)
        {
            FFileSystemInfo.IsCapable[Index] = IsCapable(static_cast<TFSCapability>(Index));
        }

        FFileSystemInfoValid = true;
    }

    return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::TemporaryTransferFile(const UnicodeString FileName)
{
    return AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::GetStoredCredentialsTried()
{
    return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::GetUserName()
{
    return FSecureShell->GetUserName();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::Idle()
{
    // Keep session alive
    if ((GetSessionData()->GetPingType() != ptOff) &&
            ((System::Now() - FSecureShell->GetLastDataSent()) > GetSessionData()->GetPingIntervalDT()))
    {
        if ((GetSessionData()->GetPingType() == ptDummyCommand) &&
                FSecureShell->GetReady())
        {
            TSFTPPacket Packet(SSH_FXP_REALPATH, GetSessionData()->GetCodePageAsNumber());
            Packet.AddPathString(L"/");
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
    /*
    for (size_t i = 0; i < FPacketReservations->GetCount(); i++)
    {
        assert(FPacketReservations->GetItem(i) == NULL);
        delete static_cast<TSFTPPacket *>(FPacketReservations->GetItem(i));
    }
    */
    FPacketReservations->Clear();
    FPacketNumbers.clear();
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::IsCapable(int Capability) const
{
    assert(FTerminal);
    switch (Capability)
    {
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
               strcmp(GetEOL(), EOLToStr(FTerminal->GetConfiguration()->GetLocalEOLType())) != 0;

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
bool TSFTPFileSystem::SupportsExtension(const UnicodeString Extension) const
{
    return FSupport->Loaded && (FSupport->Extensions->IndexOf(Extension.c_str()) >= 0);
}
//---------------------------------------------------------------------------
inline void __fastcall TSFTPFileSystem::BusyStart()
{
    if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
    {
        Busy(true);
    }
    FBusy++;
    assert(FBusy < 10);
}
//---------------------------------------------------------------------------
inline void __fastcall TSFTPFileSystem::BusyEnd()
{
    assert(FBusy > 0);
    FBusy--;
    if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
    {
        Busy(false);
    }
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::TransferBlockSize(size_t Overhead,
        TFileOperationProgressType *OperationProgress,
        size_t MinPacketSize,
        size_t MaxPacketSize)
{
    // DEBUG_PRINTF(L"MinPacketSize = %d, MaxPacketSize = %d", MinPacketSize, MaxPacketSize);
    const size_t minPacketSize = MinPacketSize ? MinPacketSize : 4096;

    // size + message number + type
    const size_t SFTPPacketOverhead = 4 + 4 + 1;
    size_t AMinPacketSize = static_cast<size_t>(FSecureShell->MinPacketSize());
    size_t AMaxPacketSize = static_cast<size_t>(FSecureShell->MaxPacketSize());
    bool MaxPacketSizeValid = (AMaxPacketSize > 0);
    size_t Result = OperationProgress->CPS();

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

    // DEBUG_PRINTF(L"Result1 = %u", Result);
    Result = OperationProgress->AdjustToCPSLimit(Result);
    // DEBUG_PRINTF(L"minPacketSize = %u, MaxPacketSize = %u", minPacketSize, MaxPacketSize);
    // DEBUG_PRINTF(L"Result2 = %u", Result);
    return Result;
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::UploadBlockSize(const std::string &Handle,
                                        TFileOperationProgressType *OperationProgress)
{
    // handle length + offset + data size
    const size_t UploadPacketOverhead =
        sizeof(size_t) + sizeof(__int64) + sizeof(size_t);
    return TransferBlockSize(UploadPacketOverhead + Handle.Length(), OperationProgress,
                             GetSessionData()->GetSFTPMinPacketSize(),
                             GetSessionData()->GetSFTPMaxPacketSize());
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::DownloadBlockSize(
    TFileOperationProgressType *OperationProgress)
{
    size_t Result = TransferBlockSize(sizeof(size_t), OperationProgress,
                                      GetSessionData()->GetSFTPMinPacketSize(),
                                      GetSessionData()->GetSFTPMaxPacketSize());
    if (FSupport->Loaded && (FSupport->MaxReadSize > 0) &&
            (Result > FSupport->MaxReadSize))
    {
        Result = FSupport->MaxReadSize;
    }
    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SendPacket(const TSFTPPacket *Packet)
{
    BusyStart();
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->BusyEnd();
        } BOOST_SCOPE_EXIT_END
        if (FTerminal->GetLog()->GetLogging())
        {
            if ((FPreviousLoggedPacket != SSH_FXP_READ &&
                    FPreviousLoggedPacket != SSH_FXP_WRITE) ||
                    (Packet->GetType() != FPreviousLoggedPacket) ||
                    (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
            {
                if (FNotLoggedPackets)
                {
                    FTerminal->LogEvent(FORMAT(L"%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
                                               (FNotLoggedPackets)));
                    FNotLoggedPackets = 0;
                }
                FTerminal->GetLog()->Add(llInput, FORMAT(L"Type: %s, Size: %d, Number: %d",
                                         Packet->GetTypeName().c_str(), (int)Packet->GetLength(), (int)Packet->GetMessageNumber()));
                if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2)
                {
                    FTerminal->GetLog()->Add(llInput, Packet->Dump());
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
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::GotStatusPacket(TSFTPPacket *Packet,
                                        size_t AllowStatus)
{
    size_t Code = Packet->GetCardinal();

    static int Messages[] =
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
        UnicodeString MessageStr = LoadStr(Message);
        UnicodeString ServerMessage;
        UnicodeString LanguageTag;
        if ((FVersion >= 3) ||
                // if version is not decided yet (i.e. this is status response
                // to the init request), go on only if there are any more data
                ((FVersion < 0) && (Packet->GetRemainingLength() > 0)))
        {
            // message is in UTF only since SFTP specification 01 (specification 00
            // is also version 3)
            // (in other words, always use UTF unless server is know to be buggy)
            ServerMessage = Packet->GetStringW();
            // SSH-2.0-Maverick_SSHD omits the language tag
            // and I believe I've seen one more server doind the same.
            // On the next instance, we should probably already implement workround.
            LanguageTag = Packet->GetStringW();
            if ((FVersion >= 5) && (Message == SFTP_STATUS_UNKNOWN_PRINCIPAL))
            {
                UnicodeString Principals;
                while (Packet->GetNextData() != NULL)
                {
                    if (!Principals.IsEmpty())
                    {
                        Principals += L", ";
                    }
                    Principals += Packet->GetStringW();
                }
                MessageStr = FORMAT(MessageStr.c_str(), Principals.c_str());
            }
        }
        else
        {
            ServerMessage = LoadStr(SFTP_SERVER_MESSAGE_UNSUPPORTED);
        }
        if (FTerminal->GetLog()->GetLogging())
        {
            FTerminal->GetLog()->Add(llOutput, FORMAT(L"Status code: %d, Message: %d, Server: %s, Language: %s ",
                                     int(Code), (int)Packet->GetMessageNumber(), ServerMessage.c_str(), LanguageTag.c_str()));
        }
        if (!LanguageTag.IsEmpty())
        {
            LanguageTag = FORMAT(L" (%s)", LanguageTag.c_str());
        }
        UnicodeString Error = FMTLOAD(SFTP_ERROR_FORMAT2, MessageStr.c_str(),
                                     int(Code), LanguageTag.c_str(), ServerMessage.c_str(), int(Packet->GetRequestType()));
        DEBUG_PRINTF(L"Error = %s", Error.c_str());
        FTerminal->TerminalError(NULL, Error);
        return 0;
    }
    else
    {
        if (!FNotLoggedPackets || Code)
        {
            FTerminal->GetLog()->Add(llOutput, FORMAT(L"Status code: %d", static_cast<int>(Code)));
        }
        return Code;
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::RemoveReservation(size_t Reservation)
{
    for (size_t Index = Reservation+1; Index < FPacketReservations->GetCount(); Index++)
    {
        FPacketNumbers[Index-1] = FPacketNumbers[Index];
    }
    TSFTPPacket *Packet = static_cast<TSFTPPacket *>(FPacketReservations->GetItem(Reservation));
    if (Packet)
    {
        assert(Packet->GetReservedBy() == this);
        Packet->SetReservedBy(NULL);
    }
    FPacketReservations->Delete(Reservation);
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::PacketLength(char *LenBuf, size_t ExpectedType)
{
    size_t Length = GET_32BIT(LenBuf);
    if (Length > SFTP_MAX_PACKET_LEN)
    {
        UnicodeString Message = FMTLOAD(SFTP_PACKET_TOO_BIG,
                                       Length, SFTP_MAX_PACKET_LEN);
        if (ExpectedType == SSH_FXP_VERSION)
        {
            UnicodeString LenString(System::MB2W(LenBuf), 4);
            Message = FMTLOAD(SFTP_PACKET_TOO_BIG_INIT_EXPLAIN,
                              Message.c_str(), DisplayableStr(LenString).c_str());
        }
        FTerminal->FatalError(NULL, Message);
    }
    return Length;
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::PeekPacket()
{
    bool Result;
    char *Buf;
    Result = FSecureShell->Peek(Buf, 4);
    if (Result)
    {
        size_t Length = PacketLength(Buf, NPOS);
        Result = FSecureShell->Peek(Buf, 4 + Length);
    }
    return Result;
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::ReceivePacket(TSFTPPacket *Packet,
                                      size_t ExpectedType, size_t AllowStatus)
{
    TSFTPBusy Busy(this);

    size_t Result = SSH_FX_OK;
    size_t Reservation = FPacketReservations->IndexOf(reinterpret_cast<System::TObject *>(Packet));

    if ((Reservation < 0) || (Packet->GetCapacity() == 0))
    {
        bool IsReserved;
        do
        {
            IsReserved = false;

            assert(Packet);
            char LenBuf[4];
            FSecureShell->Receive(LenBuf, sizeof(LenBuf));
            size_t Length = PacketLength(LenBuf, ExpectedType);
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
                        FTerminal->LogEvent(FORMAT(L"%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
                                                   FNotLoggedPackets));
                        FNotLoggedPackets = 0;
                    }
                    FTerminal->GetLog()->Add(llOutput, FORMAT(L"Type: %s, Size: %d, Number: %d",
                                             Packet->GetTypeName().c_str(), (int)Packet->GetLength(), (int)Packet->GetMessageNumber()));
                    if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2)
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
                TSFTPPacket *ReservedPacket;
                size_t MessageNumber;
                for (size_t Index = 0; Index < FPacketNumbers.Length(); Index++)
                {
                    MessageNumber = FPacketNumbers[Index];
                    if (MessageNumber == Packet->GetMessageNumber())
                    {
                        ReservedPacket = static_cast<TSFTPPacket *>(FPacketReservations->GetItem(Index));
                        IsReserved = true;
                        if (ReservedPacket)
                        {
                            FTerminal->LogEvent(L"Storing reserved response");
                            *ReservedPacket = *Packet;
                        }
                        else
                        {
                            FTerminal->LogEvent(L"Discarding reserved response");
                            RemoveReservation(Index);
                            if ((Reservation >= 0) && (Reservation > Index))
                            {
                                Reservation--;
                                assert(Reservation == FPacketReservations->IndexOf(reinterpret_cast<System::TObject *>(Packet)));
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
        assert(Packet->GetMessageNumber() == FPacketNumbers[Reservation]);
        RemoveReservation(Reservation);
    }

    if (ExpectedType >= 0)
    {
        if (Packet->GetType() == SSH_FXP_STATUS)
        {
            if (static_cast<int>(AllowStatus) < 0)
            {
                AllowStatus = (ExpectedType == SSH_FXP_STATUS ? asOK : asNo);
            }
            Result = GotStatusPacket(Packet, AllowStatus);
        }
        else if (ExpectedType != Packet->GetType())
        {
            FTerminal->FatalError(NULL, FMTLOAD(SFTP_INVALID_TYPE, static_cast<int>(Packet->GetType())));
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReserveResponse(const TSFTPPacket *Packet,
                                      TSFTPPacket *Response)
{
    if (Response != NULL)
    {
        assert(FPacketReservations->IndexOf(reinterpret_cast<System::TObject *>(Response)) < 0);
        // mark response as not received yet
        Response->SetCapacity(0);
        Response->SetReservedBy(this);
    }
    FPacketReservations->Add(reinterpret_cast<System::TObject *>(Response));
    if (FPacketNumbers.Length() <= FPacketReservations->GetCount())
    {
        FPacketNumbers.SetLength(FPacketReservations->GetCount() + 10);
    }
    // DEBUG_PRINTF(L"Packet->GetMessageNumber = %d", Packet->GetMessageNumber());
    FPacketNumbers[FPacketReservations->GetCount() - 1] = Packet->GetMessageNumber();
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::UnreserveResponse(TSFTPPacket *Response)
{
    size_t Reservation = FPacketReservations->IndexOf(reinterpret_cast<System::TObject *>(Response));
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
            FPacketReservations->SetItem(Reservation, NULL);
        }
    }
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::ReceiveResponse(
    const TSFTPPacket *Packet, TSFTPPacket *Response, size_t ExpectedType,
    size_t AllowStatus)
{
    size_t Result;
    size_t MessageNumber = Packet->GetMessageNumber();
    TSFTPPacket *AResponse = (Response ? Response : new TSFTPPacket(GetSessionData()->GetCodePageAsNumber()));
    {
        BOOST_SCOPE_EXIT ( (&Response) (&AResponse) )
        {
            if (!Response)
            {
                delete AResponse;
            }
        } BOOST_SCOPE_EXIT_END
        Result = ReceivePacket(AResponse, ExpectedType, AllowStatus);
        if (MessageNumber != AResponse->GetMessageNumber())
        {
            FTerminal->FatalError(NULL, FMTLOAD(SFTP_MESSAGE_NUMBER,
                                                (int)AResponse->GetMessageNumber(), (int)MessageNumber));
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
size_t TSFTPFileSystem::SendPacketAndReceiveResponse(
    const TSFTPPacket *Packet, TSFTPPacket *Response, size_t ExpectedType,
    size_t AllowStatus)
{
    size_t Result;
    TSFTPBusy Busy(this);
    SendPacket(Packet);
    Result = ReceiveResponse(Packet, Response, ExpectedType, AllowStatus);
    return Result;
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::RealPath(const UnicodeString Path)
{
    try
    {
        FTerminal->LogEvent(FORMAT(L"Getting real path for '%s'",
                                   Path.c_str()));

        TSFTPPacket Packet(SSH_FXP_REALPATH, GetSessionData()->GetCodePageAsNumber());
        Packet.AddPathString(Path);
        SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_NAME);
        if (Packet.GetCardinal() != 1)
        {
            FTerminal->FatalError(NULL, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
        }

        UnicodeString RealDir = UnixExcludeTrailingBackslash(Packet.GetPathString());
        // ignore rest of SSH_FXP_NAME packet

        FTerminal->LogEvent(FORMAT(L"Real path is '%s'", RealDir.c_str()));

        return RealDir;
    }
    catch (const std::exception &E)
    {
        if (FTerminal->GetActive())
        {
            throw ExtException(FMTLOAD(SFTP_REALPATH_ERROR, Path.c_str()), &E);
        }
        else
        {
            throw;
        }
    }
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::RealPath(const UnicodeString Path,
                                       const UnicodeString BaseDir)
{
    UnicodeString APath;

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
        if (APath.IsEmpty()) { APath = UnixIncludeTrailingBackslash(L"."); }
    }
    return RealPath(APath);
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::LocalCanonify(const UnicodeString Path)
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
UnicodeString TSFTPFileSystem::Canonify(const UnicodeString Path)
{
    // inspired by canonify() from PSFTP.C
    UnicodeString path = Path;
    UnicodeString Result;
    FTerminal->LogEvent(FORMAT(L"Canonifying: \"%s\"", path.c_str()));
    path = LocalCanonify(path);
    bool TryParent = false;
    try
    {
        Result = RealPath(path);
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
        UnicodeString APath = UnixExcludeTrailingBackslash(path);
        UnicodeString Name = UnixExtractFileName(APath);
        if (Name == THISDIRECTORY || Name == PARENTDIRECTORY)
        {
            Result = path;
        }
        else
        {
            UnicodeString FPath = UnixExtractFilePath(APath);
            try
            {
                Result = RealPath(FPath);
                Result = UnixIncludeTrailingBackslash(Result) + Name;
            }
            catch(...)
            {
                if (FTerminal->GetActive())
                {
                    Result = path;
                }
                else
                {
                    throw;
                }
            }
        }
    }

    FTerminal->LogEvent(FORMAT(L"Canonified: \"%s\"", Result.c_str()));

    return Result;
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::AbsolutePath(const UnicodeString Path, bool Local)
{
    if (Local)
    {
        return LocalCanonify(Path);
    }
    else
    {
        return RealPath(Path, GetCurrentDirectory());
    }
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::GetHomeDirectory()
{
    if (FHomeDirectory.IsEmpty())
    {
        FHomeDirectory = RealPath(THISDIRECTORY);
    }
    return FHomeDirectory;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::LoadFile(TRemoteFile *File, TSFTPPacket *Packet,
                               bool Complete)
{
    Packet->GetFile(File, FVersion, GetSessionData()->GetDSTMode(),
                    FSignedTS, Complete);
}
//---------------------------------------------------------------------------
TRemoteFile *TSFTPFileSystem::LoadFile(TSFTPPacket *Packet,
                                       TRemoteFile *ALinkedByFile, const UnicodeString FileName,
                                       TRemoteFileList *TempFileList, bool Complete)
{
    TRemoteFile *File = new TRemoteFile(ALinkedByFile);
    try
    {
        File->SetTerminal(FTerminal);
        if (!FileName.IsEmpty())
        {
            File->SetFileName(FileName);
        }
        // to get full path for symlink completion
        File->SetDirectory(TempFileList);
        LoadFile(File, Packet, Complete);
        File->SetDirectory(NULL);
    }
    catch(...)
    {
        delete File;
        throw;
    }
    return File;
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::GetCurrentDirectory()
{
    return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DoStartup()
{
    // do not know yet
    FVersion = NPOS;
    FFileSystemInfoValid = false;
    TSFTPPacket Packet(SSH_FXP_INIT, GetSessionData()->GetCodePageAsNumber());
    size_t MaxVersion = GetSessionData()->GetSFTPMaxVersion();
    if (MaxVersion > SFTPMaxVersion)
    {
        MaxVersion = SFTPMaxVersion;
    }
    Packet.AddCardinal(MaxVersion);

    try
    {
        SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_VERSION);
    }
    catch (const std::exception &E)
    {
        FTerminal->FatalError(&E, LoadStr(SFTP_INITIALIZE_ERROR));
    }

    FVersion = Packet.GetCardinal();
    FTerminal->LogEvent(FORMAT(L"SFTP version %d negotiated.", FVersion));
    if (FVersion < 0 || FVersion < SFTPMinVersion || FVersion > SFTPMaxVersion)
    {
        FTerminal->FatalError(NULL, FMTLOAD(SFTP_VERSION_NOT_SUPPORTED,
                                            FVersion, SFTPMinVersion, SFTPMaxVersion));
    }

    FExtensions->Clear();
    FEOL = L"\r\n";
    FSupport->Loaded = false;
    SAFE_DESTROY(FFixedPaths);

    if (FVersion >= 3)
    {
        while (Packet.GetNextData() != NULL)
        {
            UnicodeString ExtensionName = Packet.GetStringW();
            UnicodeString ExtensionData = Packet.GetStringW();
            UnicodeString ExtensionDisplayData = DisplayableStr(ExtensionData);
            // DEBUG_PRINTF(L"ExtensionName = %s", ExtensionName.c_str());

            if (ExtensionName == SFTP_EXT_NEWLINE)
            {
                FEOL = ExtensionData;
                FTerminal->LogEvent(FORMAT(L"Server requests EOL sequence %s.",
                                           ExtensionDisplayData.c_str()));
                if (FEOL.Length() < 1 || FEOL.Length() > 2)
                {
                    FTerminal->FatalError(NULL, FMTLOAD(SFTP_INVALID_EOL, ExtensionDisplayData.c_str()));
                }
            }
            // do not allow "supported" to override "supported2" if both are received
            else if (((ExtensionName == SFTP_EXT_SUPPORTED) && !FSupport->Loaded) ||
                     (ExtensionName == SFTP_EXT_SUPPORTED2))
            {
                FSupport->Reset();
                TSFTPPacket SupportedStruct(ExtensionData, GetSessionData()->GetCodePageAsNumber());
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
                        FSupport->Extensions->Add(SupportedStruct.GetStringW());
                    }
                }
                else
                {
                    FSupport->OpenBlockMasks = SupportedStruct.GetSmallCardinal();
                    FSupport->BlockMasks = SupportedStruct.GetSmallCardinal();
                    size_t ExtensionCount;
                    ExtensionCount = SupportedStruct.GetCardinal();
                    for (size_t i = 0; i < ExtensionCount; i++)
                    {
                        FSupport->AttribExtensions->Add(SupportedStruct.GetStringW());
                    }
                    ExtensionCount = SupportedStruct.GetCardinal();
                    for (size_t i = 0; i < ExtensionCount; i++)
                    {
                        FSupport->Extensions->Add(SupportedStruct.GetStringW());
                    }
                }

                if (FTerminal->GetLog()->GetLogging())
                {
                    FTerminal->LogEvent(FORMAT(
                                            L"Server support information:\n"
                                            L"  Attribute mask: %x, Attribute bits: %x, Open flags: %x\n"
                                            L"  Access mask: %x, Open block masks: %x, Block masks: %x, Max read size: %d\n",
                                            int(FSupport->AttributeMask),
                                            int(FSupport->AttributeBits),
                                            int(FSupport->OpenFlags),
                                            int(FSupport->AccessMask),
                                            int(FSupport->OpenBlockMasks),
                                            int(FSupport->BlockMasks),
                                            int(FSupport->MaxReadSize)));
                    FTerminal->LogEvent(FORMAT(   L"  Attribute extensions (%d)\n", FSupport->AttribExtensions->GetCount()));
                    for (size_t Index = 0; Index < FSupport->AttribExtensions->GetCount(); Index++)
                    {
                        FTerminal->LogEvent(
                            FORMAT(L"    %s", FSupport->AttribExtensions->GetStrings(Index).c_str()));
                    }
                    FTerminal->LogEvent(FORMAT(   L"  Extensions (%d)\n", FSupport->Extensions->GetCount()));
                    for (size_t Index = 0; Index < FSupport->Extensions->GetCount(); Index++)
                    {
                        FTerminal->LogEvent(
                            FORMAT(L"    %s", FSupport->Extensions->GetStrings(Index).c_str()));
                    }
                }
            }
            else if (ExtensionName == SFTP_EXT_VENDOR_ID)
            {
                TSFTPPacket VendorIdStruct(ExtensionData, GetSessionData()->GetCodePageAsNumber());
                UnicodeString VendorName(VendorIdStruct.GetStringW());
                UnicodeString ProductName(VendorIdStruct.GetStringW());
                UnicodeString ProductVersion(VendorIdStruct.GetStringW());
                __int64 ProductBuildNumber = VendorIdStruct.GetInt64();
                FTerminal->LogEvent(FORMAT(L"Server software: %s %s (%d) by %s",
                                           ProductName.c_str(), ProductVersion.c_str(), int(ProductBuildNumber), VendorName.c_str()));
            }
            else if (ExtensionName == SFTP_EXT_FSROOTS)
            {
                FTerminal->LogEvent(L"File system roots:\n");
                assert(FFixedPaths == NULL);
                FFixedPaths = new System::TStringList();
                try
                {
                    TSFTPPacket RootsPacket(ExtensionData, GetSessionData()->GetCodePageAsNumber());
                    while (RootsPacket.GetNextData() != NULL)
                    {
                        size_t Dummy = RootsPacket.GetCardinal();
                        if (Dummy != 1)
                        {
                            break;
                        }
                        else
                        {
                            char Drive = RootsPacket.GetByte();
                            char MaybeType = RootsPacket.GetByte();
                            FTerminal->LogEvent(FORMAT(L"  %c: (type %d)", Drive, static_cast<int>(MaybeType)));
                            FFixedPaths->Add(FORMAT(L"%c:", Drive));
                        }
                    }
                }
                catch (const std::exception &E)
                {
                    DEBUG_PRINTF(L"before FTerminal->HandleException");
                    FFixedPaths->Clear();
                    FTerminal->LogEvent(FORMAT(L"Failed to decode %s extension",
                                               SFTP_EXT_FSROOTS));
                    FTerminal->HandleException(&E);
                }
            }
            else if (ExtensionName == SFTP_EXT_VERSIONS)
            {
                try
                {
                    // first try legacy decoding according to incorrect encoding
                    // (structure-like) as of VShell.
                    TSFTPPacket VersionsPacket(ExtensionData, GetSessionData()->GetCodePageAsNumber());
                    UnicodeString Versions = VersionsPacket.GetStringW();
                    if (VersionsPacket.GetNextData() != NULL)
                    {
                        System::Abort();
                    }
                    FTerminal->LogEvent(FORMAT(L"SFTP versions supported by the server (VShell format): %s",
                                               Versions.c_str()));
                }
                catch(...)
                {
                    // if that fails, fallback to proper decoding
                    FTerminal->LogEvent(FORMAT(L"SFTP versions supported by the server: %s",
                                               ExtensionData.c_str()));
                }
            }
            else
            {
                FTerminal->LogEvent(FORMAT(L"Unknown server extension %s=%s",
                                           ExtensionName.c_str(), ExtensionDisplayData.c_str()));
            }
            FExtensions->SetValue(ExtensionName, ExtensionData);
        }

        if (SupportsExtension(SFTP_EXT_VENDOR_ID))
        {
            TSFTPPacket Packet(SSH_FXP_EXTENDED, GetSessionData()->GetCodePageAsNumber());
            Packet.AddStringW(SFTP_EXT_VENDOR_ID);
            Packet.AddStringW(FTerminal->GetConfiguration()->GetCompanyName());
            Packet.AddStringW(FTerminal->GetConfiguration()->GetProductName());
            Packet.AddStringW(FTerminal->GetConfiguration()->GetProductVersion());
            Packet.AddInt64(LOWORD(FTerminal->GetConfiguration()->GetFixedApplicationInfo().dwFileVersionLS));
            SendPacket(&Packet);
            // we are not interested in the response, do not wait for it
            ReserveResponse(&Packet, NULL);
        }
    }

    if (FVersion < 4)
    {
        // currently enable the bug for all servers (really known on OpenSSH)
        FSignedTS = (GetSessionData()->GetSFTPBug(sbSignedTS) == asOn) ||
                    (GetSessionData()->GetSFTPBug(sbSignedTS) == asAuto);
        if (FSignedTS)
        {
            FTerminal->LogEvent(L"We believe the server has signed timestamps bug");
        }
    }
    else
    {
        FSignedTS = false;
    }

    // use UTF when forced or ...
    // when "auto" and version is at least 4 and the server is not know not to use UTF
    FUtfNever = ((::Pos(GetSessionInfo().SshImplementation, L"Foxit-WAC-Server")) == 0) ||
                (GetSessionData()->GetNotUtf() == asOn);
    FUtfStrings =
        (GetSessionData()->GetNotUtf() == asOff) ||
        ((GetSessionData()->GetNotUtf() == asAuto) &&
         (FVersion >= 4) && !FUtfNever);

    if (FUtfStrings)
    {
        FTerminal->LogEvent(L"We will use UTF-8 strings when appropriate");
    }
    else if (FUtfNever)
    {
        FTerminal->LogEvent(L"We will never use UTF-8 strings");
    }
    else
    {
        FTerminal->LogEvent(L"We will use UTF-8 strings for status messages only");
    }

    FOpenSSH =
        // Sun SSH is based on OpenSSH (suffers the same bugs)
        (::Pos(GetSessionInfo().SshImplementation, L"OpenSSH") == 0) ||
        (::Pos(GetSessionInfo().SshImplementation, L"Sun_SSH") == 0);

    FMaxPacketSize = GetSessionData()->GetSFTPMaxPacketSize();
    if (FMaxPacketSize == 0)
    {
        if (FOpenSSH && (FVersion == 3) && !FSupport->Loaded)
        {
            FMaxPacketSize = 4 + (256 * 1024); // len + 256kB payload
            FTerminal->LogEvent(FORMAT(L"Limiting packet size to OpenSSH sftp-server limit of %d bytes",
                                       int(FMaxPacketSize)));
        }
        // full string is "1.77 sshlib: Momentum SSH Server",
        // possibly it is sshlib-related
        else if (::Pos(GetSessionInfo().SshImplementation, L"Momentum SSH Server") >= 0)
        {
            FMaxPacketSize = 4 + (32 * 1024);
            FTerminal->LogEvent(FORMAT(L"Limiting packet size to Momentum sftp-server limit of %d bytes",
                                       int(FMaxPacketSize)));
        }
    }
}
//---------------------------------------------------------------------------
char *TSFTPFileSystem::GetEOL() const
{
    if (FVersion >= 4)
    {
        assert(!FEOL.IsEmpty());
        return const_cast<char *>(System::W2MB(FEOL.c_str()).c_str());
    }
    else
    {
        return EOLToStr(GetSessionData()->GetEOLType());
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::LookupUsersGroups()
{
    assert(SupportsExtension(SFTP_EXT_OWNER_GROUP));

    TSFTPPacket PacketOwners(SSH_FXP_EXTENDED, GetSessionData()->GetCodePageAsNumber());
    TSFTPPacket PacketGroups(SSH_FXP_EXTENDED, GetSessionData()->GetCodePageAsNumber());

    TSFTPPacket *Packets[] = { &PacketOwners, &PacketGroups };
    TRemoteTokenList *Lists[] = { &FTerminal->FUsers, &FTerminal->FGroups };
    char ListTypes[] = { OGQ_LIST_OWNERS, OGQ_LIST_GROUPS };

    for (int Index = 0; Index < LENOF(Packets); Index++)
    {
        TSFTPPacket *Packet = Packets[Index];
        Packet->AddStringW(SFTP_EXT_OWNER_GROUP);
        Packet->AddByte(ListTypes[Index]);
        SendPacket(Packet);
        ReserveResponse(Packet, Packet);
    }

    for (int Index = 0; Index < LENOF(Packets); Index++)
    {
        TSFTPPacket *Packet = Packets[Index];

        ReceiveResponse(Packet, Packet, SSH_FXP_EXTENDED_REPLY, asOpUnsupported);

        if ((Packet->GetType() != SSH_FXP_EXTENDED_REPLY) ||
                (Packet->GetStringW() != SFTP_EXT_OWNER_GROUP_REPLY))
        {
            FTerminal->LogEvent(FORMAT(L"Invalid response to %s", SFTP_EXT_OWNER_GROUP));
        }
        else
        {
            TRemoteTokenList &List = *Lists[Index];
            size_t Count = Packet->GetCardinal();

            List.Clear();
            for (size_t Item = 0; Item < Count; Item++)
            {
                TRemoteToken Token(Packet->GetStringW());
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
void __fastcall TSFTPFileSystem::ReadCurrentDirectory()
{
    if (!FDirectoryToChangeTo.IsEmpty())
    {
        FCurrentDirectory = FDirectoryToChangeTo;
        FDirectoryToChangeTo = L"";
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
    FTerminal->LogEvent(FORMAT(L"Trying to open directory \"%s\".", Directory.c_str()));
    TRemoteFile *File;
    CustomReadFile(Directory, File, SSH_FXP_LSTAT, NULL, asOpUnsupported);
    if (File == NULL)
    {
        // File can be NULL only when server does not support SSH_FXP_LSTAT.
        // Fallback to legacy solution, which in turn does not allow entering
        // traverse-only (chmod 110) directories.
        // This is workaround for http://www.ftpshell.com/
        TSFTPPacket Packet(SSH_FXP_OPENDIR, GetSessionData()->GetCodePageAsNumber());
        Packet.AddPathString(UnixExcludeTrailingBackslash(Directory));
        SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);
        std::string Handle = Packet.GetStringA();
        Packet.ChangeType(SSH_FXP_CLOSE);
        Packet.AddStringA(Handle);
        SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS, asAll);
    }
    else
    {
        delete File;
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::AnnounceFileListOperation()
{
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ChangeDirectory(const UnicodeString Directory)
{
    UnicodeString Path, Current;

    Current = !FDirectoryToChangeTo.IsEmpty() ? FDirectoryToChangeTo : FCurrentDirectory;
    Path = RealPath(Directory, Current);

    // to verify existence of directory try to open it (SSH_FXP_REALPATH succeeds
    // for invalid paths on some systems, like CygWin)
    TryOpenDirectory(Path);

    // if open dir did not fail, directory exists -> success.
    FDirectoryToChangeTo = Path;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CachedChangeDirectory(const UnicodeString Directory)
{
    FDirectoryToChangeTo = UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadDirectory(TRemoteFileList *FileList)
{
    assert(FileList && !FileList->GetDirectory().IsEmpty());

    UnicodeString Directory;
    Directory = UnixExcludeTrailingBackslash(LocalCanonify(FileList->GetDirectory()));
    FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".", Directory.c_str()));

    // moved before SSH_FXP_OPENDIR, so directory listing does not retain
    // old data (e.g. parent directory) when reading fails
    FileList->Clear();

    TSFTPPacket Packet(SSH_FXP_OPENDIR, GetSessionData()->GetCodePageAsNumber());
    std::string Handle;

    try
    {
        Packet.AddPathString(Directory);

        SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

        Handle = Packet.GetStringA();
    }
    catch(...)
    {
        if (FTerminal->GetActive())
        {
            FileList->AddFile(new TRemoteParentDirectory(FTerminal));
        }
        throw;
    }

    TSFTPPacket Response(GetSessionData()->GetCodePageAsNumber());
    {
        BOOST_SCOPE_EXIT ( (&Self) (&Packet) (&Handle) )
        {
            if (Self->FTerminal->GetActive())
            {
                Packet.ChangeType(SSH_FXP_CLOSE);
                Packet.AddStringA(Handle);
                Self->SendPacket(&Packet);
                // we are not interested in the response, do not wait for it
                Self->ReserveResponse(&Packet, NULL);
            }
        } BOOST_SCOPE_EXIT_END
        bool isEOF = false;
        size_t Total = 0;
        TRemoteFile *File;

        Packet.ChangeType(SSH_FXP_READDIR);
        Packet.AddStringA(Handle);

        SendPacket(&Packet);

        do
        {
            ReceiveResponse(&Packet, &Response);
            if (Response.GetType() == SSH_FXP_NAME)
            {
                TSFTPPacket ListingPacket(Response, GetSessionData()->GetCodePageAsNumber());

                Packet.ChangeType(SSH_FXP_READDIR);
                Packet.AddStringA(Handle);

                SendPacket(&Packet);
                ReserveResponse(&Packet, &Response);

                size_t Count = ListingPacket.GetCardinal();

                for (size_t Index = 0; !isEOF && (Index < Count); Index++)
                {
                    File = LoadFile(&ListingPacket, NULL, L"", FileList);
                    if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
                    {
                        FTerminal->LogEvent(FORMAT(L"Read file '%s' from listing", File->GetFileName().c_str()));
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
                    FTerminal->LogEvent(L"Empty directory listing packet. Aborting directory reading.");
                    isEOF = true;
                }
            }
            else if (Response.GetType() == SSH_FXP_STATUS)
            {
                isEOF = (GotStatusPacket(&Response, asEOF) == SSH_FX_EOF);
            }
            else
            {
                FTerminal->FatalError(NULL, FMTLOAD(SFTP_INVALID_TYPE, static_cast<int>(Response.GetType())));
            }
        }
        while (!isEOF);

        if (Total == 0)
        {
            bool Failure = false;
            // no point reading parent of root directory,
            // moreover CompleteFTP terminates session upon attempt to do so
            if (::IsUnixRootPath(FileList->GetDirectory()))
            {
                File = NULL;
            }
            else
            {
                // Empty file list -> probably "permision denied", we
                // at least get link to parent directory ("..")
                try
                {
                    FTerminal->SetExceptionOnFail(true);
                    {
                        BOOST_SCOPE_EXIT ( (&Self) )
                        {
                            Self->FTerminal->SetExceptionOnFail(false);
                        } BOOST_SCOPE_EXIT_END
                        File = NULL;
                        FTerminal->ReadFile(
                            UnixIncludeTrailingBackslash(FileList->GetDirectory()) + PARENTDIRECTORY, File);
                    }
                }
                catch (const std::exception &E)
                {
                    if (::InheritsFrom<std::exception, EFatal>(&E))
                    {
                        throw;
                    }
                    else
                    {
                        File = NULL;
                        Failure = true;
                    }
                }
            }

            // on some systems even getting ".." fails, we create dummy ".." instead
            if (File == NULL)
            {
                File = new TRemoteParentDirectory(FTerminal);
            }

            assert(File && File->GetIsParentDirectory());
            FileList->AddFile(File);

            if (Failure)
            {
                throw ExtException(FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory().c_str()));
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadSymlink(TRemoteFile *SymlinkFile,
                                  TRemoteFile *& File)
{
    assert(SymlinkFile && SymlinkFile->GetIsSymLink());
    assert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later

    // need to use full filename when resolving links within subdirectory
    // (i.e. for download)
    UnicodeString FileName = LocalCanonify(
                                SymlinkFile->GetDirectory() != NULL ? SymlinkFile->GetFullFileName() : SymlinkFile->GetFileName());

    TSFTPPacket ReadLinkPacket(SSH_FXP_READLINK, GetSessionData()->GetCodePageAsNumber());
    ReadLinkPacket.AddPathString(FileName);
    SendPacket(&ReadLinkPacket);
    ReserveResponse(&ReadLinkPacket, &ReadLinkPacket);

    // send second request before reading response to first one
    // (performance benefit)
    TSFTPPacket AttrsPacket(SSH_FXP_STAT, GetSessionData()->GetCodePageAsNumber());
    AttrsPacket.AddPathString(FileName);
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
    SymlinkFile->SetLinkTo(ReadLinkPacket.GetPathString());

    ReceiveResponse(&AttrsPacket, &AttrsPacket, SSH_FXP_ATTRS);
    // SymlinkFile->FileName was used instead SymlinkFile->LinkTo before, why?
    File = LoadFile(&AttrsPacket, SymlinkFile,
                    UnixExtractFileName(SymlinkFile->GetLinkTo()));
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ReadFile(const UnicodeString FileName,
                               TRemoteFile *& File)
{
    CustomReadFile(FileName, File, SSH_FXP_LSTAT);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::RemoteFileExists(const UnicodeString FullPath,
                                       TRemoteFile **File)
{
    bool Result;
    try
    {
        TRemoteFile *AFile;
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
void __fastcall TSFTPFileSystem::SendCustomReadFile(TSFTPPacket *Packet,
        TSFTPPacket *Response,
        // const std::string RemoteHandle,
        const UnicodeString FileName,
        size_t Flags)
{
    if ((Packet->GetType() == SSH_FXP_STAT) || (Packet->GetType() == SSH_FXP_LSTAT))
    {
        Packet->AddPathString(LocalCanonify(FileName));
    }
    else
    {
        assert(Packet->GetType() == SSH_FXP_FSTAT);
        // actually handle, not filename
        Packet->AddStringW(FileName);
    }

    if (FVersion >= 4)
    {
        Packet->AddCardinal(Flags);
    }
    SendPacket(Packet);
    ReserveResponse(Packet, Response);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CustomReadFile(const UnicodeString FileName,
                                     TRemoteFile *& File, char Type, TRemoteFile *ALinkedByFile,
                                     size_t AllowStatus)
{
    size_t Flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS |
                   SSH_FILEXFER_ATTR_ACCESSTIME | SSH_FILEXFER_ATTR_MODIFYTIME |
                   SSH_FILEXFER_ATTR_OWNERGROUP;
    TSFTPPacket Packet(Type, GetSessionData()->GetCodePageAsNumber());
    SendCustomReadFile(&Packet, &Packet, FileName, Flags);
    ReceiveResponse(&Packet, &Packet, SSH_FXP_ATTRS, AllowStatus);

    if (Packet.GetType() == SSH_FXP_ATTRS)
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
void __fastcall TSFTPFileSystem::DoDeleteFile(const UnicodeString FileName, char Type)
{
    TSFTPPacket Packet(Type, GetSessionData()->GetCodePageAsNumber());
    UnicodeString RealFileName = LocalCanonify(FileName);
    Packet.AddPathString(RealFileName);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DeleteFile(const UnicodeString FileName,
                                 const TRemoteFile *File, int Params, TRmSessionAction &Action)
{
    char Type;
    if (File && File->GetIsDirectory() && !File->GetIsSymLink())
    {
        if (FLAGCLEAR(Params, dfNoRecursive))
        {
            try
            {
                FTerminal->ProcessDirectory(FileName, boost::bind(&TTerminal::DeleteFile, FTerminal, _1, _2, _3), &Params);
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
void __fastcall TSFTPFileSystem::RenameFile(const UnicodeString FileName,
                                 const UnicodeString NewName)
{
    TSFTPPacket Packet(SSH_FXP_RENAME, GetSessionData()->GetCodePageAsNumber());
    UnicodeString RealName = LocalCanonify(FileName);
    Packet.AddPathString(RealName);
    UnicodeString TargetName;
    if (UnixExtractFilePath(NewName).IsEmpty())
    {
        // rename case (TTerminal::RenameFile)
        TargetName = UnixExtractFilePath(RealName) + NewName;
    }
    else
    {
        TargetName = LocalCanonify(NewName);
    }
    Packet.AddPathString(TargetName);
    if (FVersion >= 5)
    {
        Packet.AddCardinal(0);
    }
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CopyFile(const UnicodeString /*FileName*/,
                               const UnicodeString /*NewName*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CreateDirectory(const UnicodeString DirName)
{
    TSFTPPacket Packet(SSH_FXP_MKDIR, GetSessionData()->GetCodePageAsNumber());
    UnicodeString CanonifiedName = Canonify(DirName);
    Packet.AddPathString(CanonifiedName);
    Packet.AddProperties(NULL, 0, true, FVersion, NULL);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CreateLink(const UnicodeString FileName,
                                 const UnicodeString PointTo, bool Symbolic)
{
    USEDPARAM(Symbolic);
    assert(Symbolic); // only symlinks are supported by SFTP
    assert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later
    TSFTPPacket Packet(SSH_FXP_SYMLINK, GetSessionData()->GetCodePageAsNumber());

    bool Buggy = (GetSessionData()->GetSFTPBug(sbSymlink) == asOn) ||
                 ((GetSessionData()->GetSFTPBug(sbSymlink) == asAuto) && FOpenSSH);

    if (!Buggy)
    {
        Packet.AddPathString(Canonify(FileName));
        Packet.AddPathString(PointTo);
    }
    else
    {
        FTerminal->LogEvent(L"We believe the server has SFTP symlink bug");
        Packet.AddPathString(PointTo);
        Packet.AddPathString(Canonify(FileName));
    }
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::ChangeFileProperties(const UnicodeString FileName,
        const TRemoteFile * /*File*/, const TRemoteProperties *AProperties,
        TChmodSessionAction &Action)
{
    assert(AProperties != NULL);

    TRemoteFile *File;

    UnicodeString RealFileName = LocalCanonify(FileName);
    ReadFile(RealFileName, File);

    {
        BOOST_SCOPE_EXIT ( (&File) )
        {
            delete File;
        } BOOST_SCOPE_EXIT_END
        assert(File);

        if (File->GetIsDirectory() && !File->GetIsSymLink() && AProperties->Recursive)
        {
            try
            {
                FTerminal->ProcessDirectory(FileName, boost::bind(&TTerminal::ChangeFileProperties, FTerminal, _1, _2, _3),
                                            static_cast<void *>(const_cast<TRemoteProperties *>(AProperties)));
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
            Properties.Owner = File->GetOwner();
            Properties.Valid << vpOwner;
        }
        else if (Properties.Valid.Contains(vpOwner) &&
                 !Properties.Valid.Contains(vpGroup))
        {
            Properties.Group = File->GetGroup();
            Properties.Valid << vpGroup;
        }

        TSFTPPacket Packet(SSH_FXP_SETSTAT, GetSessionData()->GetCodePageAsNumber());
        Packet.AddPathString(RealFileName);
        Packet.AddProperties(&Properties, *File->GetRights(), File->GetIsDirectory(), FVersion, &Action);
        SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
    }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::LoadFilesProperties(System::TStrings *FileList)
{
    bool Result = false;
    // without knowledge of server's capabilities, this all make no sense
    if (FSupport->Loaded)
    {
        TFileOperationProgressType *Progress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, FTerminal, _1, _2),
                boost::bind(&TTerminal::DoFinished, FTerminal, _1, _2, _3, _4, _5, _6));
        Progress->Start(foGetProperties, osRemote, FileList->GetCount());

        FTerminal->FOperationProgress = Progress;

        static int LoadFilesPropertiesQueueLen = 5;
        TSFTPLoadFilesPropertiesQueue Queue(this, GetSessionData()->GetCodePageAsNumber());
        {
            BOOST_SCOPE_EXIT ( (&Self) (&Queue) (&Progress) )
            {
                Queue.DisposeSafe();
                Self->FTerminal->FOperationProgress = NULL;
                Progress->Stop();
                delete Progress;
            } BOOST_SCOPE_EXIT_END
            if (Queue.Init(LoadFilesPropertiesQueueLen, FileList))
            {
                TRemoteFile *File;
                TSFTPPacket Packet(GetSessionData()->GetCodePageAsNumber());
                bool Next;
                do
                {
                    Next = Queue.ReceivePacket(&Packet, File);
                    assert((Packet.GetType() == SSH_FXP_ATTRS) || (Packet.GetType() == SSH_FXP_STATUS));
                    if (Packet.GetType() == SSH_FXP_ATTRS)
                    {
                        assert(File);
                        Progress->SetFile(File->GetFileName());
                        LoadFile(File, &Packet);
                        Result = true;
                        TOnceDoneOperation OnceDoneOperation;
                        Progress->Finish(File->GetFileName(), true, OnceDoneOperation);
                    }

                    if (Progress->Cancel != csContinue)
                    {
                        Next = false;
                    }
                }
                while (Next);
            }
        }
        // queue is discarded here
    }

    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::DoCalculateFilesChecksum(const UnicodeString Alg,
        System::TStrings *FileList, System::TStrings *Checksums,
        TCalculatedChecksumEvent *OnCalculatedChecksum,
        TFileOperationProgressType *OperationProgress, bool FirstLevel)
{
    TOnceDoneOperation OnceDoneOperation; // not used

    // recurse into subdirectories only if we have callback function
    if (OnCalculatedChecksum != NULL)
    {
        for (size_t Index = 0; Index < FileList->GetCount(); Index++)
        {
            TRemoteFile *File = static_cast<TRemoteFile *>(FileList->GetObjects(Index));
            assert(File != NULL);
            if (File->GetIsDirectory() && !File->GetIsSymLink() &&
                    !File->GetIsParentDirectory() && !File->GetIsThisDirectory())
            {
                OperationProgress->SetFile(File->GetFileName());
                TRemoteFileList *SubFiles =
                    FTerminal->CustomReadDirectoryListing(File->GetFullFileName(), false);

                if (SubFiles != NULL)
                {
                    System::TStrings *SubFileList = new System::TStringList();
                    bool Success = false;
                    {
                        BOOST_SCOPE_EXIT ( (&SubFiles) (&SubFileList) (&FirstLevel)
                                           (&File) (&Success) (&OnceDoneOperation) (&OperationProgress) )
                        {
                            delete SubFiles;
                            delete SubFileList;

                            if (FirstLevel)
                            {
                                OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
                            }
                        } BOOST_SCOPE_EXIT_END

                        OperationProgress->SetFile(File->GetFileName());

                        for (size_t index = 0; index < SubFiles->GetCount(); index++)
                        {
                            TRemoteFile *SubFile = SubFiles->GetFile(index);
                            SubFileList->AddObject(SubFile->GetFullFileName(), SubFile);
                        }

                        // do not collect checksums for files in subdirectories,
                        // only send back checksums via callback
                        DoCalculateFilesChecksum(Alg, SubFileList, NULL,
                                                 OnCalculatedChecksum, OperationProgress, false);

                        Success = true;
                    }
                }
            }
        }
    }

    static int CalculateFilesChecksumQueueLen = 5;
    TSFTPCalculateFilesChecksumQueue Queue(this, GetSessionData()->GetCodePageAsNumber());
    {
        BOOST_SCOPE_EXIT ( (&Queue) )
        {
            Queue.DisposeSafe();
        } BOOST_SCOPE_EXIT_END
        if (Queue.Init(CalculateFilesChecksumQueueLen, Alg, FileList))
        {
            TSFTPPacket Packet(GetSessionData()->GetCodePageAsNumber());
            bool Next = false;
            calculatedchecksum_signal_type sig;
            if (OnCalculatedChecksum)
            {
                sig.connect(*OnCalculatedChecksum);
            }
            do
            {
                bool Success = false;
                UnicodeString alg;
                UnicodeString Checksum;
                TRemoteFile *File = NULL;

                {
                    BOOST_SCOPE_EXIT ( (&FirstLevel) (&File)
                                       (&Success) (&OnceDoneOperation) (&OperationProgress) )
                    {
                        if (FirstLevel)
                        {
                            OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
                        }
                    } BOOST_SCOPE_EXIT_END
                    try
                    {
                        Next = Queue.ReceivePacket(&Packet, File);
                        assert(Packet.GetType() == SSH_FXP_EXTENDED_REPLY);

                        OperationProgress->SetFile(File->GetFileName());

                        alg = Packet.GetStringW();
                        // Checksum = StrToHex(UnicodeString(System::MB2W(Packet.GetNextData(Packet.GetRemainingLength()), Packet.GetRemainingLength())));
                        Checksum = StrToHex(UnicodeString(System::MB2W(std::string(Packet.GetNextData(Packet.GetRemainingLength()), Packet.GetRemainingLength()).c_str())));
                        sig(File->GetFileName(), alg, Checksum);

                        Success = true;
                    }
                    catch (const std::exception &E)
                    {
                        FTerminal->CommandError(&E, FMTLOAD(CHECKSUM_ERROR,
                                                            File != NULL ? File->GetFullFileName().c_str() : L""));
                        // TODO: retries? resume?
                        Next = false;
                    }

                    if (Checksums != NULL)
                    {
                        Checksums->Add(L"");
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
    // queue is discarded here
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CalculateFilesChecksum(const UnicodeString Alg,
        System::TStrings *FileList, System::TStrings *Checksums,
        TCalculatedChecksumEvent *OnCalculatedChecksum)
{
    TFileOperationProgressType *Progress = new TFileOperationProgressType(boost::bind(&TTerminal::DoProgress, FTerminal, _1, _2),
            boost::bind(&TTerminal::DoFinished, FTerminal, _1, _2, _3, _4, _5, _6));
    Progress->Start(foCalculateChecksum, osRemote, FileList->GetCount());

    FTerminal->FOperationProgress = Progress;

    {
        BOOST_SCOPE_EXIT ( (&Self) (&Progress) )
        {
            Self->FTerminal->FOperationProgress = NULL;
            Progress->Stop();
            delete Progress;
        } BOOST_SCOPE_EXIT_END
        DoCalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum,
                                 Progress, true);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CustomCommandOnFile(const UnicodeString /*FileName*/,
        const TRemoteFile * /*File*/, const UnicodeString /*Command*/, int /*Params*/,
        const TCaptureOutputEvent & /*OutputEvent*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::AnyCommand(const UnicodeString /*Command*/,
                                 const TCaptureOutputEvent * /*OutputEvent*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::FileUrl(const UnicodeString FileName)
{
    return FTerminal->FileUrl(L"sftp", FileName);
}
//---------------------------------------------------------------------------
System::TStrings *TSFTPFileSystem::GetFixedPaths()
{
    return FFixedPaths;
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SpaceAvailable(const UnicodeString Path,
                                     TSpaceAvailable &ASpaceAvailable)
{
    TSFTPPacket Packet(SSH_FXP_EXTENDED, GetSessionData()->GetCodePageAsNumber());
    Packet.AddStringW(SFTP_EXT_SPACE_AVAILABLE);
    Packet.AddPathString(LocalCanonify(Path));
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
void __fastcall TSFTPFileSystem::CopyToRemote(System::TStrings *FilesToCopy,
                                   const UnicodeString TargetDir, const TCopyParamType *CopyParam,
                                   int Params, TFileOperationProgressType *OperationProgress,
                                   TOnceDoneOperation &OnceDoneOperation)
{
    assert(FilesToCopy && OperationProgress);

    UnicodeString FileName, FileNameOnly;
    UnicodeString FullTargetDir = UnixIncludeTrailingBackslash(TargetDir);
    size_t Index = 0;
    while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
    {
        bool Success = false;
        FileName = FilesToCopy->GetStrings(Index);
        FileNameOnly = ExtractFileName(FileName, false);
        // DEBUG_PRINTF(L"FileName = %s, FileNameOnly = %s", FileName.c_str(), FileNameOnly.c_str());
        assert(!FAvoidBusy);
        FAvoidBusy = true;

        {
            BOOST_SCOPE_EXIT ( (&Self) (&FileName) (&Success) (&OnceDoneOperation)
                               (&OperationProgress) )
            {
                Self->FAvoidBusy = false;
                OperationProgress->Finish(FileName, Success, OnceDoneOperation);
            } BOOST_SCOPE_EXIT_END
            try
            {
                if (GetSessionData()->GetCacheDirectories())
                {
                    FTerminal->DirectoryModified(TargetDir, false);

                    if (::DirectoryExists(ExtractFilePath(FileName)))
                    {
                        FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir) +
                                                     FileNameOnly, true);
                    }
                }
                SFTPSourceRobust(FileName, FullTargetDir, CopyParam, Params, OperationProgress,
                                 tfFirstLevel);
                Success = true;
            }
            catch (const EScpSkipFile &E)
            {
                DEBUG_PRINTF(L"before FTerminal->HandleException");
                SUSPEND_OPERATION (
                    if (!FTerminal->HandleException(&E))
                    {
                        throw;
                    };
                );
            }
        }
        Index++;
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPConfirmOverwrite(UnicodeString &FileName,
        int Params, TFileOperationProgressType *OperationProgress,
        TOverwriteMode &OverwriteMode, const TOverwriteFileParams *FileParams)
{
    bool CanAppend = (FVersion < 4) || !OperationProgress->AsciiTransfer;
    bool CanResume =
        (FileParams != NULL) &&
        (FileParams->DestSize < FileParams->SourceSize);
    int Answer = 0;
    SUSPEND_OPERATION
    (
        // abort = "append"
        // retry = "resume"
        // all = "yes to newer"
        // ignore = "rename"
        int Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;

        // possibly we can allow alternate resume at least in some cases
        if (CanAppend)
        {
            Answers |= qaAbort | qaRetry;
        }
        TQueryButtonAlias Aliases[4];
        Aliases[0].Button = qaAbort;
        Aliases[0].Alias = LoadStr(APPEND_BUTTON);
        Aliases[1].Button = qaRetry;
        Aliases[1].Alias = LoadStr(RESUME_BUTTON);
        Aliases[2].Button = qaAll;
        Aliases[2].Alias = LoadStr(YES_TO_NEWER_BUTTON);
        Aliases[3].Button = qaIgnore;
        Aliases[3].Alias = LoadStr(RENAME_BUTTON);
        TQueryParams QueryParams(qpNeverAskAgainCheck);
        QueryParams.NoBatchAnswers = qaIgnore | qaAbort | qaRetry | qaAll;
        QueryParams.Aliases = Aliases;
        QueryParams.AliasesCount = LENOF(Aliases);
        Answer = FTerminal->ConfirmFileOverwrite(FileName, FileParams,
                 Answers, &QueryParams,
                 OperationProgress->Side == osLocal ? osRemote : osLocal,
                 Params, OperationProgress);
    );

    if (CanAppend &&
            ((Answer == qaAbort) || (Answer == qaRetry) || (Answer == qaSkip)))
    {
        switch (Answer)
        {
        // append
        case qaAbort:
            Params |= cpAppend;
            break;
            // resume
        case qaRetry:
            Params |= cpResume;
            break;
        }
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
        else if (CanAlternateResume && CanResume &&
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
            TQueryParams params(0, HELP_APPEND_OR_RESUME);
            SUSPEND_OPERATION
            (
                Answer = FTerminal->QueryUser(FORMAT(LoadStr(APPEND_OR_RESUME).c_str(), FileName.c_str()),
                                              NULL, qaYes | qaNo | qaNoToAll | qaCancel, &params);
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
                System::Abort();
                break;
            }
        }
    }
    else if (Answer == qaIgnore)
    {
        if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName, LoadStr(RENAME_TITLE), L"",
                                  LoadStr(RENAME_PROMPT2),
                                  true, 0, FileName))
        {
            OverwriteMode = omOverwrite;
        }
        else
        {
            if (!OperationProgress->Cancel)
            {
                OperationProgress->Cancel = csCancel;
            }
            System::Abort();
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
            System::Abort();
            break;

        case qaNo:
            THROW_SKIP_FILE_NULL;
        }
    }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::SFTPConfirmResume(const UnicodeString DestFileName,
                                        bool PartialBiggerThanSource, TFileOperationProgressType *OperationProgress)
{
    bool ResumeTransfer = false;
    assert(OperationProgress);
    if (PartialBiggerThanSource)
    {
        int Answer;
        SUSPEND_OPERATION
        (
            TQueryParams Params(qpAllowContinueOnError, HELP_PARTIAL_BIGGER_THAN_SOURCE);
            Answer = FTerminal->QueryUser(
                         FMTLOAD(PARTIAL_BIGGER_THAN_SOURCE, DestFileName.c_str()),
                         NULL,
                         qaOK | qaAbort, &Params, qtWarning);
        )

        if (Answer == qaAbort)
        {
            if (!OperationProgress->Cancel)
            {
                OperationProgress->Cancel = csCancel;
            }
            System::Abort();
        }
        ResumeTransfer = false;
    }
    else if (FTerminal->GetConfiguration()->GetConfirmResume())
    {
        int Answer;
        SUSPEND_OPERATION
        (
            TQueryParams Params(qpAllowContinueOnError | qpNeverAskAgainCheck,
                                HELP_RESUME_TRANSFER);
            // "abort" replaced with "cancel" to unify with "append/resume" query
            Answer = FTerminal->QueryUser(
                         FMTLOAD(RESUME_TRANSFER, DestFileName.c_str()),
                         NULL, qaYes | qaNo | qaCancel,
                         &Params);
        );

        switch (Answer)
        {
        case qaNeverAskAgain:
            FTerminal->GetConfiguration()->SetConfirmResume(false);
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
            System::Abort();
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
void __fastcall TSFTPFileSystem::SFTPSourceRobust(const UnicodeString FileName,
                                       const UnicodeString TargetDir, const TCopyParamType *CopyParam, int Params,
                                       TFileOperationProgressType *OperationProgress, unsigned int Flags)
{
    // DEBUG_PRINTF(L"FileName = %s, TargetDir = %s", FileName.c_str(), TargetDir.c_str());
    // the same in TFTPFileSystem
    bool Retry;

    TUploadSessionAction Action(FTerminal->GetActionLog());
    TOpenRemoteFileParams OpenParams;
    OpenParams.OverwriteMode = omOverwrite;
    TOverwriteFileParams FileParams;
    do
    {
        Retry = false;
        bool ChildError = false;
        try
        {
            SFTPSource(FileName, TargetDir, CopyParam, Params,
                       &OpenParams, &FileParams, OperationProgress,
                       Flags, Action, ChildError);
        }
        catch (std::exception &E)
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
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPSource(const UnicodeString FileName,
                                 const UnicodeString TargetDir, const TCopyParamType *CopyParam, int Params,
                                 TOpenRemoteFileParams *OpenParams,
                                 TOverwriteFileParams *FileParams,
                                 TFileOperationProgressType *OperationProgress, unsigned int Flags,
                                 TUploadSessionAction &Action, bool &ChildError)
{
    FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

    Action.FileName(ExpandUNCFileName(FileName));

    OperationProgress->SetFile(FileName, false);

    if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
    {
        FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
        THROW_SKIP_FILE_NULL;
    }

    HANDLE File = 0;
    __int64 MTime = 0, ATime = 0;
    __int64 Size = 0;

    FTerminal->OpenLocalFile(FileName, GENERIC_READ, &OpenParams->LocalFileAttrs,
                             &File, NULL, &MTime, &ATime, &Size);

    bool Dir = FLAGSET(OpenParams->LocalFileAttrs, faDirectory);

    {
        BOOST_SCOPE_EXIT ( (&File) )
        {
            if (File != NULL)
            {
                ::CloseHandle(File);
            }
        } BOOST_SCOPE_EXIT_END
        OperationProgress->SetFileInProgress();

        if (Dir)
        {
            Action.Cancel();
            SFTPDirectorySource(IncludeTrailingBackslash(FileName), TargetDir,
                                OpenParams->LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
        }
        else
        {
            // File is regular file (not directory)
            assert(File);

            UnicodeString DestFileName = CopyParam->ChangeFileName(ExtractFileName(FileName, false),
                                        osLocal, FLAGSET(Flags, tfFirstLevel));
            UnicodeString DestFullName = LocalCanonify(TargetDir + DestFileName);
            UnicodeString DestPartialFullName;
            bool ResumeAllowed;
            bool ResumeTransfer = false;
            bool DestFileExists = false;
            TRights DestRights;

            __int64 ResumeOffset = 0;

            FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", FileName.c_str()));

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
                UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
                L" transfer mode selected.");

            // should we check for interrupted transfer?
            ResumeAllowed = !OperationProgress->AsciiTransfer &&
                            CopyParam->AllowResume(OperationProgress->LocalSize) &&
                            IsCapable(fcRename);
            OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

            FileParams->SourceSize = OperationProgress->LocalSize;
            FileParams->SourceTimestamp = UnixToDateTime(MTime,
                                         GetSessionData()->GetDSTMode());

            if (ResumeAllowed)
            {
                DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();

                if (FLAGCLEAR(Flags, tfNewDirectory))
                {
                    FTerminal->LogEvent(L"Checking existence of file.");
                    TRemoteFile *file = NULL;
                    DestFileExists = RemoteFileExists(DestFullName, &file);
                    if (DestFileExists)
                    {
                        OpenParams->DestFileSize = file->GetSize();
                        FileParams->DestSize = OpenParams->DestFileSize;
                        FileParams->DestTimestamp = file->GetModification();
                        DestRights = *file->GetRights();
                        // if destination file is symlink, never do resumable transfer,
                        // as it would delete the symlink.
                        // also bit of heuristics to detect symlink on SFTP-3 and older
                        // (which does not indicate symlink in SSH_FXP_ATTRS).
                        // if file has all permissions and is small, then it is likely symlink.
                        // also it is not likely that such a small file (if it is not symlink)
                        // gets overwritten by large file (that would trigger resumable transfer).
                        if (file->GetIsSymLink() ||
                                ((FVersion < 4) &&
                                 ((*file->GetRights() & static_cast<unsigned short>(TRights::rfAll)) ==
                                  static_cast<unsigned short>(TRights::rfAll)) &&
                                 (file->GetSize() < 100)))
                        {
                            ResumeAllowed = false;
                            OperationProgress->SetResumeStatus(rsDisabled);
                        }

                        delete file;
                        file = NULL;
                    }

                    if (ResumeAllowed)
                    {
                        FTerminal->LogEvent(L"Checking existence of partially transfered file.");
                        if (RemoteFileExists(DestPartialFullName, &file))
                        {
                            ResumeOffset = file->GetSize();
                            delete file;
                            file = NULL;

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
                                DoDeleteFile(DestPartialFullName, SSH_FXP_REMOVE);
                            }
                            else
                            {
                                FTerminal->LogEvent(L"Resuming file transfer.");
                            }
                        }
                        else
                        {
                            // partial upload file does not exists, check for full file
                            if (DestFileExists)
                            {
                                UnicodeString PrevDestFileName = DestFileName;
                                SFTPConfirmOverwrite(DestFileName,
                                                     Params, OperationProgress, OpenParams->OverwriteMode, FileParams);
                                if (PrevDestFileName != DestFileName)
                                {
                                    // update paths in case user changes the file name
                                    DestFullName = LocalCanonify(TargetDir + DestFileName);
                                    DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
                                    FTerminal->LogEvent(L"Checking existence of new file.");
                                    DestFileExists = RemoteFileExists(DestFullName, NULL);
                                }
                            }
                        }
                    }
                }
            }

            // will the transfer be resumable?
            bool DoResume = (ResumeAllowed && (OpenParams->OverwriteMode == omOverwrite));

            UnicodeString RemoteFileName = DoResume ? DestPartialFullName : DestFullName;
            OpenParams->RemoteFileName = RemoteFileName;
            OpenParams->Resume = DoResume;
            OpenParams->Resuming = ResumeTransfer;
            OpenParams->OperationProgress = OperationProgress;
            OpenParams->CopyParam = CopyParam;
            OpenParams->Params = Params;
            OpenParams->FileParams = FileParams;
            OpenParams->Confirmed = false;

            FTerminal->LogEvent(L"Opening remote file.");
            FTerminal->FileOperationLoop(boost::bind(&TSFTPFileSystem::SFTPOpenRemote, this, _1, _2), OperationProgress, true,
                                         FMTLOAD(SFTP_CREATE_FILE_ERROR, OpenParams->RemoteFileName.c_str()),
                                         OpenParams);

            if (OpenParams->RemoteFileName != RemoteFileName)
            {
                assert(!DoResume);
                assert(UnixExtractFilePath(OpenParams->RemoteFileName) == UnixExtractFilePath(RemoteFileName));
                DestFullName = OpenParams->RemoteFileName;
                UnicodeString NewFileName = UnixExtractFileName(DestFullName);
                assert(DestFileName != NewFileName);
                DestFileName = NewFileName;
            }

            Action.Destination(DestFullName);

            bool TransferFinished = false;
            __int64 DestWriteOffset = 0;
            TSFTPPacket CloseRequest(GetSessionData()->GetCodePageAsNumber());
            bool SetRights = ((DoResume && DestFileExists) || CopyParam->GetPreserveRights());
            bool SetProperties = (CopyParam->GetPreserveTime() || SetRights);
            TSFTPPacket PropertiesRequest(SSH_FXP_SETSTAT, GetSessionData()->GetCodePageAsNumber());
            TSFTPPacket PropertiesResponse(GetSessionData()->GetCodePageAsNumber());
            TRights Rights;
            if (SetProperties)
            {
                PropertiesRequest.AddPathString(DestFullName);
                if (CopyParam->GetPreserveRights())
                {
                    Rights = CopyParam->RemoteFileRights(OpenParams->LocalFileAttrs);
                }
                else if (DoResume && DestFileExists)
                {
                    Rights = DestRights;
                }
                else
                {
                    assert(!SetRights);
                }

                unsigned short RightsNumber = Rights.GetNumberSet();
                PropertiesRequest.AddProperties(
                    SetRights ? &RightsNumber : NULL, NULL, NULL,
                    CopyParam->GetPreserveTime() ? &MTime : NULL,
                    CopyParam->GetPreserveTime() ? &ATime : NULL,
                    NULL, false, FVersion);
            }

            {
                BOOST_SCOPE_EXIT ( (&Self) (&TransferFinished) (&OpenParams)
                                   (&CloseRequest) (&DoResume) (&DestFileName) (&OperationProgress) )
                {
                    if (Self->FTerminal->GetActive())
                    {
                        // if file transfer was finished, the close request was already sent
                        if (!OpenParams->RemoteFileHandle.IsEmpty())
                        {
                            Self->SFTPCloseRemote(OpenParams->RemoteFileHandle, DestFileName,
                                                  OperationProgress, TransferFinished, true, &CloseRequest);
                        }
                        // wait for the response
                        Self->SFTPCloseRemote(OpenParams->RemoteFileHandle, DestFileName,
                                              OperationProgress, TransferFinished, false, &CloseRequest);

                        // delete file if transfer was not completed, resuming was not allowed and
                        // we were not appending (incl. alternate resume),
                        // shortly after plain transfer completes (eq. !ResumeAllowed)
                        if (!TransferFinished && !DoResume && (OpenParams->OverwriteMode == omOverwrite))
                        {
                            Self->DoDeleteFile(OpenParams->RemoteFileName, SSH_FXP_REMOVE);
                        }
                    }
                } BOOST_SCOPE_EXIT_END
                if (OpenParams->OverwriteMode == omAppend)
                {
                    FTerminal->LogEvent(L"Appending file.");
                    DestWriteOffset = OpenParams->DestFileSize;
                }
                else if (ResumeTransfer || (OpenParams->OverwriteMode == omResume))
                {
                    if (OpenParams->OverwriteMode == omResume)
                    {
                        FTerminal->LogEvent(L"Resuming file transfer (append style).");
                        ResumeOffset = OpenParams->DestFileSize;
                    }
                    FileSeek(static_cast<HANDLE>(File), ResumeOffset, 0);
                    OperationProgress->AddResumed(ResumeOffset);
                }

                TSFTPUploadQueue Queue(this, GetSessionData()->GetCodePageAsNumber());
                {
                    BOOST_SCOPE_EXIT ( (&Queue) )
                    {
                        Queue.DisposeSafe();
                    } BOOST_SCOPE_EXIT_END
                    Queue.Init(FileName, File, OperationProgress,
                               OpenParams->RemoteFileHandle,
                               DestWriteOffset + OperationProgress->TransferedSize);

                    while (Queue.Continue())
                    {
                        if (OperationProgress->Cancel)
                        {
                            System::Abort();
                        }
                    }

                    // send close request before waiting for pending read responses
                    SFTPCloseRemote(OpenParams->RemoteFileHandle, DestFileName,
                                    OperationProgress, false, true, &CloseRequest);
                    OpenParams->RemoteFileHandle = "";

                    // when resuming is disabled, we can send "set properties"
                    // request before waiting for pending read/close responses
                    if (SetProperties && !DoResume)
                    {
                        SendPacket(&PropertiesRequest);
                        ReserveResponse(&PropertiesRequest, &PropertiesResponse);
                    }
                }

                TransferFinished = true;
                // queue is discarded here
            }

            if (DoResume)
            {
                if (DestFileExists)
                {
                    FILE_OPERATION_LOOP(FMTLOAD(DELETE_ON_RESUME_ERROR,
                                                UnixExtractFileName(DestFullName).c_str(), DestFullName.c_str()),
                        if (GetSessionData()->GetOverwrittenToRecycleBin())
                        {
                            FTerminal->RecycleFile(DestFullName, NULL);
                        }
                        else
                        {
                            DoDeleteFile(DestFullName, SSH_FXP_REMOVE);
                        }
                    );
                }

                // originally this was before CLOSE (last catch (...) statement),
                // on VShell it failed
                FILE_OPERATION_LOOP(FMTLOAD(RENAME_AFTER_RESUME_ERROR,
                                            UnixExtractFileName(OpenParams->RemoteFileName.c_str()).c_str(), DestFileName.c_str()),
                    RenameFile(OpenParams->RemoteFileName, DestFileName);
                );
            }

            if (SetProperties)
            {
                std::auto_ptr<TTouchSessionAction> TouchAction;
                if (CopyParam->GetPreserveTime())
                {
                    TouchAction.reset(new TTouchSessionAction(FTerminal->GetActionLog(), DestFullName,
                                      UnixToDateTime(MTime, GetSessionData()->GetDSTMode())));
                }
                std::auto_ptr<TChmodSessionAction> ChmodAction;
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
                    FILE_OPERATION_LOOP(FMTLOAD(PRESERVE_TIME_PERM_ERROR, DestFileName.c_str()),
                        TSFTPPacket DummyResponse(GetSessionData()->GetCodePageAsNumber());
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
                    );
                }
                catch (const std::exception &E)
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

    /* TODO : Delete also read-only files. */
    if (FLAGSET(Params, cpDelete))
    {
        if (!Dir)
        {
            FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
                THROWOSIFFALSE(::DeleteFile(FileName));
            )
        }
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(OpenParams->LocalFileAttrs, faArchive))
    {
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
            THROWOSIFFALSE(FileSetAttr(FileName, OpenParams->LocalFileAttrs & ~faArchive) == 0);
        )
    }
}
//---------------------------------------------------------------------------
std::string TSFTPFileSystem::SFTPOpenRemoteFile(
    const UnicodeString FileName, size_t OpenType, __int64 Size)
{
    TSFTPPacket Packet(SSH_FXP_OPEN, GetSessionData()->GetCodePageAsNumber());

    Packet.AddPathString(FileName);
    if (FVersion < 5)
    {
        Packet.AddCardinal(OpenType);
    }
    else
    {
        size_t Access =
            FLAGMASK(FLAGSET(OpenType, SSH_FXF_READ), ACE4_READ_DATA) |
            FLAGMASK(FLAGSET(OpenType, SSH_FXF_WRITE), ACE4_WRITE_DATA | ACE4_APPEND_DATA);

        size_t Flags;

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
                         SendSize ? &Size : NULL, false, FVersion);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

    return Packet.GetStringA();
}
//---------------------------------------------------------------------------
int TSFTPFileSystem::SFTPOpenRemote(void *AOpenParams, void * /*Param2*/)
{
    TOpenRemoteFileParams *OpenParams = static_cast<TOpenRemoteFileParams *>(AOpenParams);
    assert(OpenParams);
    TFileOperationProgressType *OperationProgress = OpenParams->OperationProgress;

    size_t OpenType = 0;
    bool Success = false;
    bool ConfirmOverwriting = false;

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
            if ((ConfirmOverwriting || GetSessionData()->GetOverwrittenToRecycleBin()) &&
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
        catch (const std::exception &E)
        {
            if (!OpenParams->Confirmed && (OpenType & SSH_FXF_EXCL) && FTerminal->GetActive())
            {
                bool ThrowOriginal = false;

                // When exclusive opening of file fails, try to detect if file exists.
                // When file does not exist, failure was probably caused by 'permission denied'
                // or similar error. In this case throw original exception.
                try
                {
                    TRemoteFile *File = NULL;
                    UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
                    ReadFile(RealFileName, File);
                    assert(File);
                    OpenParams->DestFileSize = File->GetSize();
                    if (OpenParams->FileParams != NULL)
                    {
                        OpenParams->FileParams->DestTimestamp = File->GetModification();
                        OpenParams->FileParams->DestSize = OpenParams->DestFileSize;
                    }
                    // file exists (otherwise exception was thrown)
                    SAFE_DESTROY(File);
                }
                catch (...)
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
                    UnicodeString RemoteFileNameOnly = UnixExtractFileName(OpenParams->RemoteFileName);
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
                    assert(GetSessionData()->GetOverwrittenToRecycleBin());
                }

                if ((OpenParams->OverwriteMode == omOverwrite) &&
                        GetSessionData()->GetOverwrittenToRecycleBin())
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
                        TRemoteFile *File;
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
                                                      FMTLOAD(SFTP_OVERWRITE_FILE_ERROR, OpenParams->RemoteFileName.c_str()),
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
void __fastcall TSFTPFileSystem::SFTPCloseRemote(const std::string &Handle,
                                      const UnicodeString FileName, TFileOperationProgressType *OperationProgress,
                                      bool TransferFinished, bool Request, TSFTPPacket *Packet)
{
    // Moving this out of SFTPSource() fixed external exception 0xC0000029 error
    FILE_OPERATION_LOOP(FMTLOAD(SFTP_CLOSE_FILE_ERROR, FileName.c_str()),
        try
        {
            TSFTPPacket CloseRequest(GetSessionData()->GetCodePageAsNumber());
            TSFTPPacket *P = (Packet == NULL ? &CloseRequest : Packet);

            if (Request)
            {
                P->ChangeType(SSH_FXP_CLOSE);
                P->AddStringA(Handle);
                SendPacket(P);
                ReserveResponse(P, Packet);
            }
            else
            {
                assert(Packet != NULL);
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
    );
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPDirectorySource(const UnicodeString DirectoryName,
        const UnicodeString TargetDir, int Attrs, const TCopyParamType *CopyParam,
        int Params, TFileOperationProgressType *OperationProgress, unsigned int Flags)
{
    UnicodeString DestDirectoryName = CopyParam->ChangeFileName(
                                         ExtractFileName(ExcludeTrailingBackslash(DirectoryName), false), osLocal,
                                         FLAGSET(Flags, tfFirstLevel));
    UnicodeString DestFullName = UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);

    OperationProgress->SetFile(DirectoryName);

    bool CreateDir = false;
    try
    {
        TryOpenDirectory(DestFullName);
    }
    catch (...)
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
        if (CopyParam->GetPreserveRights())
        {
            Properties.Valid = TValidProperties() << vpRights;
            Properties.Rights = CopyParam->RemoteFileRights(Attrs);
        }
        FTerminal->CreateDirectory(DestFullName, &Properties);
        Flags |= tfNewDirectory;
    }

    WIN32_FIND_DATA SearchRec;
    bool FindOK = false;
    HANDLE findHandle = 0;
    FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
        UnicodeString path = DirectoryName + L"*.*";
        findHandle = FindFirstFile(path.c_str(), &SearchRec);
        FindOK = (findHandle != 0);
    );

    {
        BOOST_SCOPE_EXIT ( (&findHandle) )
        {
            ::FindClose(findHandle);
        } BOOST_SCOPE_EXIT_END
        while (FindOK && !OperationProgress->Cancel)
        {
            UnicodeString FileName = DirectoryName + SearchRec.cFileName;
            try
            {
                if ((wcscmp(SearchRec.cFileName, THISDIRECTORY) != 0) && (wcscmp(SearchRec.cFileName, PARENTDIRECTORY) != 0))
                {
                    SFTPSourceRobust(FileName, DestFullName, CopyParam, Params, OperationProgress,
                                     Flags & ~tfFirstLevel);
                }
            }
            catch (const EScpSkipFile &E)
            {
                DEBUG_PRINTF(L"before FTerminal->HandleException");
                // If ESkipFile occurs, just log it and continue with next file
                SUSPEND_OPERATION (
                    // here a message to user was displayed, which was not appropriate
                    // when user refused to overwrite the file in subdirectory.
                    // hopefuly it won't be missing in other situations.
                    if (!FTerminal->HandleException(&E)) throw;
                );
            }
            FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
                FindOK = (::FindNextFile(findHandle, &SearchRec) != 0);
            );
        };
    }

    /* TODO : Delete also read-only directories. */
    /* TODO : Show error message on failure. */
    if (!OperationProgress->Cancel)
    {
        if (FLAGSET(Params, cpDelete))
        {
            RemoveDir(DirectoryName);
        }
        else if (CopyParam->GetClearArchive() && FLAGSET(Attrs, faArchive))
        {
            FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
                THROWOSIFFALSE(FileSetAttr(DirectoryName, Attrs & ~faArchive) == 0);
            )
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::CopyToLocal(System::TStrings *FilesToCopy,
                                  const UnicodeString TargetDir, const TCopyParamType *CopyParam,
                                  int Params, TFileOperationProgressType *OperationProgress,
                                  TOnceDoneOperation &OnceDoneOperation)
{
    assert(FilesToCopy && OperationProgress);

    UnicodeString FileName;
    UnicodeString FullTargetDir = IncludeTrailingBackslash(TargetDir);
    const TRemoteFile *File;
    bool Success;
    size_t Index = 0;
    while (Index < FilesToCopy->GetCount() && !OperationProgress->Cancel)
    {
        Success = false;
        FileName = FilesToCopy->GetStrings(Index);
        File = static_cast<TRemoteFile *>(FilesToCopy->GetObjects(Index));

        assert(!FAvoidBusy);
        FAvoidBusy = true;

        {
            BOOST_SCOPE_EXIT ( (&Self) (&FileName) (&Success) (&OnceDoneOperation)
                               (&OperationProgress))
            {
                Self->FAvoidBusy = false;
                OperationProgress->Finish(FileName, Success, OnceDoneOperation);
            } BOOST_SCOPE_EXIT_END
            try
            {
                SFTPSinkRobust(LocalCanonify(FileName), File, FullTargetDir, CopyParam,
                               Params, OperationProgress, tfFirstLevel);
                Success = true;
            }
            catch (const EScpSkipFile &E)
            {
                DEBUG_PRINTF(L"before FTerminal->HandleException");
                SUSPEND_OPERATION (
                    if (!FTerminal->HandleException(&E)) throw;
                );
            }
            catch (...)
            {
                // TODO: remove the block?
                throw;
            }
        }
        Index++;
    }
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPSinkRobust(const UnicodeString FileName,
                                     const TRemoteFile *File, const UnicodeString TargetDir,
                                     const TCopyParamType *CopyParam, int Params,
                                     TFileOperationProgressType *OperationProgress, unsigned int Flags)
{
    // the same in TFTPFileSystem
    bool Retry;

    TDownloadSessionAction Action(FTerminal->GetActionLog());

    do
    {
        Retry = false;
        bool ChildError = false;
        try
        {
            SFTPSink(FileName, File, TargetDir, CopyParam, Params, OperationProgress,
                     Flags, Action, ChildError);
        }
        catch (std::exception &E)
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
            if (!File->GetIsDirectory())
            {
                // prevent overwrite and resume confirmations
                Params |= cpNoConfirmation;
            }
        }
    }
    while (Retry);
}
//---------------------------------------------------------------------------
void __fastcall TSFTPFileSystem::SFTPSink(const UnicodeString FileName,
                               const TRemoteFile *File, const UnicodeString TargetDir,
                               const TCopyParamType *CopyParam, int Params,
                               TFileOperationProgressType *OperationProgress, unsigned int Flags,
                               TDownloadSessionAction &Action, bool &ChildError)
{

    Action.FileName(FileName);

    UnicodeString OnlyFileName = UnixExtractFileName(FileName);

    assert(File);
    TFileMasks::TParams MaskParams;
    MaskParams.Size = File->GetSize();

    if (!CopyParam->AllowTransfer(FileName, osRemote, File->GetIsDirectory(), MaskParams))
    {
        FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
        THROW_SKIP_FILE_NULL;
    }

    FTerminal->LogEvent(FORMAT(L"File: \"%s\"", FileName.c_str()));

    OperationProgress->SetFile(OnlyFileName);

    UnicodeString DestFileName = CopyParam->ChangeFileName(OnlyFileName,
                                osRemote, FLAGSET(Flags, tfFirstLevel));
    UnicodeString DestFullName = TargetDir + DestFileName;

    if (File->GetIsDirectory())
    {
        Action.Cancel();
        if (!File->GetIsSymLink())
        {
            int Attrs = 0;
            FILE_OPERATION_LOOP (FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()),
                Attrs = FileGetAttr(DestFullName);
                if ((Attrs & faDirectory) == 0)
                {
                    EXCEPTION;
                }
            );

            FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()),
                if (!::ForceDirectories(DestFullName))
                {
                    RaiseLastOSError();
                }
            );

            TSinkFileParams SinkFileParams;
            SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
            SinkFileParams.CopyParam = CopyParam;
            SinkFileParams.Params = Params;
            SinkFileParams.OperationProgress = OperationProgress;
            SinkFileParams.Skipped = false;
            SinkFileParams.Flags = Flags & ~tfFirstLevel;

            FTerminal->ProcessDirectory(FileName, boost::bind(&TSFTPFileSystem::SFTPSinkFile, this, _1, _2, _3), &SinkFileParams);

            // Do not delete directory if some of its files were skip.
            // Throw "skip file" for the directory to avoid __fastcall attempt to deletion
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
        FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to local directory started.", FileName.c_str()));

        UnicodeString DestPartialFullName;
        bool ResumeAllowed;
        bool ResumeTransfer = false;
        __int64 ResumeOffset;

        // Will we use ASCII of BINARY file tranfer?
        OperationProgress->SetAsciiTransfer(
            CopyParam->UseAsciiTransfer(FileName, osRemote, MaskParams));
        FTerminal->LogEvent(UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
                            L" transfer mode selected.");

        // Suppose same data size to transfer as to write
        // (not true with ASCII transfer)
        OperationProgress->SetTransferSize(File->GetSize());
        OperationProgress->SetLocalSize(OperationProgress->TransferSize);

        // resume has no sense for temporary downloads
        ResumeAllowed = ((Params & cpTemporary) == 0) &&
                        !OperationProgress->AsciiTransfer &&
                        CopyParam->AllowResume(OperationProgress->TransferSize);
        OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

        int Attrs = 0;
        FILE_OPERATION_LOOP (FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()),
            if ((Attrs >= 0) && (Attrs & faDirectory))
            {
                EXCEPTION;
            }
        );

        OperationProgress->TransferingFile = false; // not set with SFTP protocol

        HANDLE LocalHandle = NULL;
        System::TStream *FileStream = NULL;
        bool DeleteLocalFile = false;
        std::string RemoteHandle;
        UnicodeString LocalFileName = DestFullName;
        TOverwriteMode OverwriteMode = omOverwrite;

        {
            BOOST_SCOPE_EXIT ( (&Self) (&LocalHandle) (&FileStream) (&DeleteLocalFile)
                               (&ResumeAllowed) (&OverwriteMode) (&LocalFileName)
                               (&DestFileName) (&RemoteHandle)
                               (&OperationProgress) )
            {
                if (LocalHandle) { ::CloseHandle(LocalHandle); }
                if (FileStream) { delete FileStream; }
                if (DeleteLocalFile && (!ResumeAllowed || OperationProgress->LocallyUsed == 0) &&
                        (OverwriteMode == omOverwrite))
                {
                    FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, LocalFileName.c_str()),
                        THROWOSIFFALSE(::DeleteFile(LocalFileName));
                    )
                }

                // if the transfer was finished, the file is closed already
                if (Self->FTerminal->GetActive() && !RemoteHandle.IsEmpty())
                {
                    // do not wait for response
                    Self->SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress,
                                          true, true, NULL);
                }
            } BOOST_SCOPE_EXIT_END
            if (ResumeAllowed)
            {
                DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
                LocalFileName = DestPartialFullName;

                FTerminal->LogEvent(L"Checking existence of partially transfered file.");
                if (FileExists(DestPartialFullName))
                {
                    FTerminal->OpenLocalFile(DestPartialFullName, GENERIC_WRITE,
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
                        ::CloseHandle(LocalHandle);
                        LocalHandle = NULL;
                        FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, DestPartialFullName.c_str()),
                            THROWOSIFFALSE(::DeleteFile(DestPartialFullName));
                        )
                    }
                    else
                    {
                        FTerminal->LogEvent(L"Resuming file transfer.");
                        FileSeek(static_cast<HANDLE>(LocalHandle), ResumeOffset, 0);
                        OperationProgress->AddResumed(ResumeOffset);
                    }
                }
            }

            if ((Attrs >= 0) && !ResumeTransfer)
            {
                __int64 DestFileSize;
                __int64 MTime;
                try
                {
                    FTerminal->OpenLocalFile(DestFullName, GENERIC_WRITE,
                                             NULL, &LocalHandle, NULL, &MTime, NULL, &DestFileSize, false);

                    FTerminal->LogEvent(L"Confirming overwriting of file.");
                    TOverwriteFileParams FileParams;
                    FileParams.SourceSize = OperationProgress->TransferSize;
                    FileParams.SourceTimestamp = File->GetModification();
                    FileParams.DestTimestamp = UnixToDateTime(MTime,
                                               GetSessionData()->GetDSTMode());
                    FileParams.DestSize = DestFileSize;
                    UnicodeString PrevDestFileName = DestFileName;
                    SFTPConfirmOverwrite(DestFileName, Params, OperationProgress, OverwriteMode, &FileParams);
                    if (PrevDestFileName != DestFileName)
                    {
                        DestFullName = TargetDir + DestFileName;
                        DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
                        if (ResumeAllowed)
                        {
                            if (FileExists(DestPartialFullName))
                            {
                                FILE_OPERATION_LOOP (FMTLOAD(DELETE_LOCAL_FILE_ERROR, DestPartialFullName.c_str()),
                                    THROWOSIFFALSE(::DeleteFile(DestPartialFullName));
                                )
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
                        // is NULL when overwritting read-only file
                        if (LocalHandle)
                        {
                            ::CloseHandle(LocalHandle);
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
                        FileSeek(static_cast<HANDLE>(LocalHandle), DestFileSize, 0);
                        if (OverwriteMode == omAppend)
                        {
                            FTerminal->LogEvent(L"Appending to file.");
                        }
                        else
                        {
                            FTerminal->LogEvent(L"Resuming file transfer (append style).");
                            assert(OverwriteMode == omResume);
                            OperationProgress->AddResumed(DestFileSize);
                        }
                    }
                }
                catch (const EOSError &E)
                {
                    if (E.ErrorCode != ERROR_FILE_NOT_FOUND)
                    {
                        throw;
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

            FTerminal->LogEvent(L"Opening remote file.");
            FILE_OPERATION_LOOP (FMTLOAD(SFTP_OPEN_FILE_ERROR, FileName.c_str()),
                size_t OpenType = SSH_FXF_READ;
                if ((FVersion >= 4) && OperationProgress->AsciiTransfer)
                {
                    OpenType |= SSH_FXF_TEXT;
                }
                RemoteHandle = SFTPOpenRemoteFile(FileName, OpenType);
            );

            TSFTPPacket RemoteFilePacket(SSH_FXP_FSTAT, GetSessionData()->GetCodePageAsNumber());
            if (CopyParam->GetPreserveTime())
            {
                SendCustomReadFile(&RemoteFilePacket, &RemoteFilePacket, FileName,
                                   SSH_FILEXFER_ATTR_MODIFYTIME);
            }

            FileStream = new TSafeHandleStream(static_cast<HANDLE>(LocalHandle));

            // at end of this block queue is discarded
            {
                TSFTPDownloadQueue Queue(this, GetSessionData()->GetCodePageAsNumber());
                {
                    BOOST_SCOPE_EXIT ( (&Queue) )
                    {
                        Queue.DisposeSafe();
                    } BOOST_SCOPE_EXIT_END
                    TSFTPPacket DataPacket(GetSessionData()->GetCodePageAsNumber());

                    size_t QueueLen = File->GetSize() / DownloadBlockSize(OperationProgress) + 1;
                    if ((QueueLen > GetSessionData()->GetSFTPDownloadQueue()) ||
                            (QueueLen < 0))
                    {
                        QueueLen = GetSessionData()->GetSFTPDownloadQueue();
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
                    size_t Missing = 0;
                    size_t DataLen = 0;
                    size_t BlockSize = 0;
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

                        if (DataPacket.GetType() == SSH_FXP_STATUS)
                        {
                            // must be SSH_FX_EOF, any other status packet would raise exception
                            Eof = true;
                            // close file right away, before waiting for pending responses
                            SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress,
                                            true, true, NULL);
                            RemoteHandle = ""; // do not close file again in catch (...) block
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
                                                        L"Received incomplete data packet before end of file, "
                                                        L"offset: %s, size: %d, requested: %d",
                                                        Int64ToStr(OperationProgress->TransferedSize).c_str(), static_cast<int>(DataLen),
                                                        static_cast<int>(BlockSize)));
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
                                if (OperationProgress->TransferedSize + static_cast<__int64>(DataLen) !=
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

                                size_t PrevBlockSize = BlockBuf.GetSize();
                                BlockBuf.Convert(GetEOL(), FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                                OperationProgress->SetLocalSize(
                                    OperationProgress->LocalSize - PrevBlockSize + BlockBuf.GetSize());
                            }

                            FILE_OPERATION_LOOP (FMTLOAD(WRITE_ERROR, LocalFileName.c_str()),
                                BlockBuf.WriteToStream(FileStream, BlockBuf.GetSize());
                            );

                            OperationProgress->AddLocallyUsed(BlockBuf.GetSize());
                        }

                        if (OperationProgress->Cancel == csCancel)
                        {
                            System::Abort();
                        }
                    };

                    if (GapCount > 0)
                    {
                        FTerminal->LogEvent(FORMAT(
                                                L"%d requests to fill %d data gaps were issued.",
                                                GapFillCount, GapCount));
                    }
                }
                // queue is discarded here
            }

            if (CopyParam->GetPreserveTime())
            {
                ReceiveResponse(&RemoteFilePacket, &RemoteFilePacket);

                const TRemoteFile *AFile = File;
                {
                    BOOST_SCOPE_EXIT ( (&AFile) (&File) )
                    {
                        if (AFile != File)
                        {
                            delete AFile;
                        }
                    } BOOST_SCOPE_EXIT_END
                    // ignore errors
                    if (RemoteFilePacket.GetType() == SSH_FXP_ATTRS)
                    {
                        // load file, avoid __fastcall completion (resolving symlinks) as we do not need that
                        AFile = LoadFile(&RemoteFilePacket, NULL, UnixExtractFileName(FileName),
                                         NULL, false);
                    }

                    FILETIME AcTime = DateTimeToFileTime(AFile->GetLastAccess(),
                                                         GetSessionData()->GetDSTMode());
                    FILETIME WrTime = DateTimeToFileTime(AFile->GetModification(),
                                                         GetSessionData()->GetDSTMode());
                    SetFileTime(LocalHandle, NULL, &AcTime, &WrTime);
                }
            }

            ::CloseHandle(LocalHandle);
            LocalHandle = NULL;

            if (ResumeAllowed)
            {
                FILE_OPERATION_LOOP(FMTLOAD(RENAME_AFTER_RESUME_ERROR,
                                            ExtractFileName(DestPartialFullName, true).c_str(), DestFileName.c_str()),

                    if (FileExists(DestFullName))
                    {
                        THROWOSIFFALSE(::DeleteFile(DestFullName));
                    }
                    if (!::RenameFile(DestPartialFullName, DestFullName))
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
            int NewAttrs = CopyParam->LocalFileAttrs(*File->GetRights());
            if ((NewAttrs & Attrs) != NewAttrs)
            {
                FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()),
                    THROWOSIFFALSE(FileSetAttr(DestFullName, Attrs | NewAttrs) == 0);
                );
            }

        }
    }

    if (Params & cpDelete)
    {
        ChildError = true;
        // If file is directory, do not delete it recursively, because it should be
        // empty already. If not, it should not be deleted (some files were
        // skipped or some new files were copied to it, while we were downloading)
        int params = dfNoRecursive;
        FTerminal->DeleteFile(FileName, File, &params);
        ChildError = false;
    }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSinkFile(const UnicodeString FileName,
                                   const TRemoteFile *File, void *Param)
{
    TSinkFileParams *Params = static_cast<TSinkFileParams *>(Param);
    assert(Params->OperationProgress);
    try
    {
        SFTPSinkRobust(FileName, File, Params->TargetDir, Params->CopyParam,
                       Params->Params, Params->OperationProgress, Params->Flags);
    }
    catch (const EScpSkipFile &E)
    {
        TFileOperationProgressType *OperationProgress = Params->OperationProgress;

        Params->Skipped = true;
        DEBUG_PRINTF(L"before FTerminal->HandleException");
        SUSPEND_OPERATION (
            if (!FTerminal->HandleException(&E)) throw;
        );

        if (OperationProgress->Cancel)
        {
            System::Abort();
        }
    }
}
