//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Common.h"
#include "FileBuffer.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
char * __fastcall EOLToStr(TEOLType EOLType)
{
    switch (EOLType)
    {
    case eolLF: return "\n";
    case eolCRLF: return "\r\n";
    case eolCR: return "\r";
    default: assert(false); return "";
    }
}
//---------------------------------------------------------------------------
TFileBuffer::TFileBuffer()
{
    FMemory = new TMemoryStream();
    FSize = 0;
}
//---------------------------------------------------------------------------
TFileBuffer::~TFileBuffer()
{
    delete FMemory;
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::SetSize(__int64 value)
{
    if (FSize != value)
    {
        FMemory->SetSize(value);
        FSize = value;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::SetPosition(__int64 value)
{
    FMemory->SetPosition(value);
}
//---------------------------------------------------------------------------
__int64 __fastcall TFileBuffer::GetPosition() const
{
    return FMemory->GetPosition();
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::SetMemory(TMemoryStream *value)
{
    if (FMemory != value)
    {
        if (FMemory) { delete FMemory; }
        FMemory = value;
    }
}
//---------------------------------------------------------------------------
size_t __fastcall TFileBuffer::ReadStream(TStream *Stream, size_t  Len, bool ForceLen)
{
    size_t Result = 0;
    try
    {
        SetSize(GetPosition() + Len);
        // C++5
        // FMemory->SetSize(FMemory->Position + Len);
        if (ForceLen)
        {
            Stream->ReadBuffer(GetData() + GetPosition(), Len);
            Result = Len;
        }
        else
        {
            Result = Stream->Read(GetData() + GetPosition(), Len);
        }
        if (Result != Len)
        {
            SetSize(GetSize() - Len + Result);
        }
        FMemory->Seek(Len, soFromCurrent);
    }
    catch (const EReadError &)
    {
        ::RaiseLastOSError();
    }
    return Result;
}
//---------------------------------------------------------------------------
size_t __fastcall TFileBuffer::LoadStream(TStream *Stream, size_t Len, bool ForceLen)
{
    FMemory->Seek(0, soFromBeginning);
    return ReadStream(Stream, Len, ForceLen);
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::Convert(char *Source, char *Dest, int Params,
                          bool & /*Token*/)
{
    assert(strlen(Source) <= 2);
    assert(strlen(Dest) <= 2);

    if (FLAGSET(Params, cpRemoveBOM) && (GetSize() >= 3) &&
            (memcmp(GetData(), "\xEF\xBB\xBF", 3) == 0))
    {
        Delete(0, 3);
    }

    if (FLAGSET(Params, cpRemoveCtrlZ) && (GetSize() > 0) && ((*(GetData() + GetSize() - 1)) == '\x1A'))
    {
        Delete(GetSize() - 1, 1);
    }

    if (strcmp(Source, Dest) == 0)
    {
        return;
    }

    char *Ptr = GetData();

    // one character source EOL
    if (!Source[1])
    {
        // Disabled, not worth risking it is not safe enough, for the bugfix release
#if 0
        bool PrevToken = Token;
        Token = false;
#endif

        for (size_t Index = 0; Index < GetSize(); Index++)
        {
            // EOL already in wanted format, make sure to pass unmodified
            if ((Index < GetSize() - 1) && (*Ptr == Dest[0]) && (*(Ptr+1) == Dest[1]))
            {
                Index++;
                Ptr++;
            }
#if 0
            // last buffer ended with the first char of wanted EOL format,
            // which got expanded to wanted format.
            // now we got the second char, so get rid of it.
            else if ((Index == 0) && PrevToken && (*Ptr == Dest[1]))
            {
                Delete(Index, 1);
            }
#endif
            else if (*Ptr == Source[0])
            {
#if 0
                if ((*Ptr == Dest[0]) && (Index == GetSize() - 1))
                {
                    Token = true;
                }
#endif

                *Ptr = Dest[0];
                if (Dest[1])
                {
                    Insert(Index+1, Dest+1, 1);
                    Index++;
                    Ptr = GetData() + Index;
                }
            }
            Ptr++;
        }
    }
    // two character source EOL
    else
    {
        size_t Index;
        for (Index = 0; Index < GetSize() - 1; Index++)
        {
            if ((*Ptr == Source[0]) && (*(Ptr+1) == Source[1]))
            {
                *Ptr = Dest[0];
                if (Dest[1])
                {
                    *(Ptr+1) = Dest[1];
                    Index++; Ptr++;
                }
                else
                {
                    Delete(Index+1, 1);
                    Ptr = GetData() + Index;
                }
            }
            Ptr++;
        }
        if ((Index < GetSize()) && (*Ptr == Source[0]))
        {
            Delete(Index, 1);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::Convert(TEOLType Source, TEOLType Dest, int Params,
                          bool &Token)
{
    Convert(EOLToStr(Source), EOLToStr(Dest), Params, Token);
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::Convert(char *Source, TEOLType Dest, int Params,
                          bool &Token)
{
    Convert(Source, EOLToStr(Dest), Params, Token);
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::Convert(TEOLType Source, char *Dest, int Params,
                          bool &Token)
{
    Convert(EOLToStr(Source), Dest, Params, Token);
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::Insert(size_t Index, const char *Buf, size_t Len)
{
    SetSize(GetSize() + Len);
    memmove(GetData() + Index + Len, GetData() + Index, GetSize() - Index - Len);
    memmove(GetData() + Index, Buf, Len);
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::Delete(size_t Index, size_t Len)
{
    memmove(GetData() + Index, GetData() + Index + Len, GetSize() - Index - Len);
    SetSize(GetSize() - Len);
}
//---------------------------------------------------------------------------
void __fastcall TFileBuffer::WriteToStream(TStream *Stream, size_t Len)
{
    try
    {
        Stream->WriteBuffer(GetData() + GetPosition(), Len);
        FMemory->Seek(Len, soFromCurrent);
    }
    catch (const EWriteError &)
    {
        ::RaiseLastOSError();
    }
}
//---------------------------------------------------------------------------
TSafeHandleStream::TSafeHandleStream(HANDLE AHandle) :
    THandleStream(AHandle)
{
}

TSafeHandleStream::~TSafeHandleStream()
{
}

//---------------------------------------------------------------------------
__int64 __fastcall TSafeHandleStream::Read(void *Buffer, __int64 Count)
{
    __int64 Result = ::FileRead(FHandle, Buffer, Count);
    if (Result == static_cast<__int64>(-1))
    {
        ::RaiseLastOSError();
    }
    return Result;
}
//---------------------------------------------------------------------------
__int64 __fastcall TSafeHandleStream::Write(const void *Buffer, __int64 Count)
{
   __int64 Result = ::FileWrite(FHandle, Buffer, Count);
    if (Result == -1)
    {
        ::RaiseLastOSError();
    }
    return Result;
};
