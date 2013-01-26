//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "FileBuffer.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
char * EOLToStr(TEOLType EOLType)
{
  switch (EOLType) {
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
}
//---------------------------------------------------------------------------
TFileBuffer::~TFileBuffer()
{
  delete FMemory;
}
//---------------------------------------------------------------------------
void TFileBuffer::SetSize(__int64 Value)
{
  if (FMemory->Size != Value)
  {
    FMemory->Size = Value;
  }
}
//---------------------------------------------------------------------------
void TFileBuffer::SetPosition(__int64 Value)
{
  FMemory->Position = Value;
}
//---------------------------------------------------------------------------
__int64 TFileBuffer::GetPosition() const
{
  return FMemory->Position;
}
//---------------------------------------------------------------------------
void TFileBuffer::SetMemory(TMemoryStream * Value)
{
  if (FMemory != Value)
  {
    if (FMemory) { delete FMemory; }
    FMemory = Value;
  }
}
//---------------------------------------------------------------------------
__int64 TFileBuffer::ReadStream(TStream * Stream, const __int64 Len, bool ForceLen)
{
  __int64 Result = 0;
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
      Result = Stream->Read(GetData() + GetPosition(), (__int64)Len);
    }
    if (Result != Len)
    {
      SetSize(GetSize() - Len + Result);
    }
    FMemory->Seek(Len, soFromCurrent);
  }
  catch(EReadError &)
  {
    RaiseLastOSError();
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 TFileBuffer::LoadStream(TStream * Stream, const __int64 Len, bool ForceLen)
{
  CALLSTACK;
  FMemory->Seek(0, soFromBeginning);
  return ReadStream(Stream, Len, ForceLen);
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(char * Source, char * Dest, int Params,
  bool & Token)
{
  assert(strlen(Source) <= 2);
  assert(strlen(Dest) <= 2);

  if (FLAGSET(Params, cpRemoveBOM) && (GetSize() >= 3) &&
      (memcmp(GetData(), Bom.c_str(), Bom.size()) == 0))
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

  char * Ptr = GetData();

  // one character source EOL
  if (!Source[1])
  {
    bool PrevToken = Token;
    Token = false;

    for (intptr_t Index = 0; Index < GetSize(); ++Index)
    {
      // EOL already in wanted format, make sure to pass unmodified
      if ((Index < GetSize() - 1) && (*Ptr == Dest[0]) && (*(Ptr+1) == Dest[1]))
      {
        ++Index;
        Ptr++;
      }
      // last buffer ended with the first char of wanted EOL format,
      // which got expanded to wanted format.
      // now we got the second char, so get rid of it.
      else if ((Index == 0) && PrevToken && (*Ptr == Dest[1]))
      {
        Delete(Index, 1);
      }
      else if (*Ptr == Source[0])
      {
        if ((*Ptr == Dest[0]) && (Index == GetSize() - 1))
        {
          Token = true;
        }

        *Ptr = Dest[0];
        if (Dest[1])
        {
          Insert(Index+1, Dest+1, 1);
          ++Index;
          Ptr = GetData() + Index;
        }
      }
      Ptr++;
    }
  }
  // two character source EOL
  else
  {
    intptr_t Index;
    for (Index = 0; Index < GetSize() - 1; ++Index)
    {
      if ((*Ptr == Source[0]) && (*(Ptr+1) == Source[1]))
      {
        *Ptr = Dest[0];
        if (Dest[1])
        {
          *(Ptr+1) = Dest[1];
          ++Index; ++Ptr;
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
void TFileBuffer::Convert(TEOLType Source, TEOLType Dest, int Params,
  bool & Token)
{
  Convert(EOLToStr(Source), EOLToStr(Dest), Params, Token);
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(char * Source, TEOLType Dest, int Params,
  bool & Token)
{
  Convert(Source, EOLToStr(Dest), Params, Token);
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(TEOLType Source, char * Dest, int Params,
  bool & Token)
{
  Convert(EOLToStr(Source), Dest, Params, Token);
}
//---------------------------------------------------------------------------
void TFileBuffer::Insert(__int64 Index, const char * Buf, size_t Len)
{
  SetSize(GetSize() + Len);
  memmove(GetData() + Index + Len, GetData() + Index, (size_t)(GetSize() - Index - Len));
  memmove(GetData() + Index, Buf, Len);
}
//---------------------------------------------------------------------------
void TFileBuffer::Delete(__int64 Index, size_t Len)
{
  memmove(GetData() + Index, GetData() + Index + Len, (size_t)(GetSize() - Index - Len));
  SetSize(GetSize() - Len);
}
//---------------------------------------------------------------------------
void TFileBuffer::WriteToStream(TStream * Stream, const __int64 Len)
{
  try
  {
    Stream->WriteBuffer(GetData() + GetPosition(), Len);
    FMemory->Seek(Len, soFromCurrent);
  }
  catch(EWriteError &)
  {
    RaiseLastOSError();
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSafeHandleStream::TSafeHandleStream(THandle AHandle) :
  THandleStream(AHandle)
{
}
//---------------------------------------------------------------------------
__int64 TSafeHandleStream::Read(void * Buffer, __int64 Count)
{
  __int64 Result = ::FileRead(FHandle, Buffer, Count);
  if (Result == static_cast<__int64>(-1))
  {
    RaiseLastOSError();
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 TSafeHandleStream::Write(const void * Buffer, __int64 Count)
{
  __int64 Result = ::FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
}
